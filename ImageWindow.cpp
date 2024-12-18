#include "pch.h"
#include "ImageWindow.h"
#include "FastStack.h"
#include "FastStackToolBar.h"

ImageLabel::ImageLabel(const QImage* img, QWidget* parent) : m_image(img), QLabel(parent) {
    this->setAttribute(Qt::WA_Hover);
    installEventFilter(this);
    this->setMouseTracking(true);
}

void ImageLabel::paintEvent(QPaintEvent* event) {

    QPainter p(this);

    if (m_image != nullptr)
        p.drawImage(QPoint(0, 0), *m_image);

    if (m_mask_visible && m_mask != nullptr)
        p.drawImage(QPoint(0, 0), *m_mask);
}

void ImageLabel::enterEvent(QEnterEvent* event) {
    mousePos(event->pos());
}

void ImageLabel::leaveEvent(QEvent* event) {
    mouseLeave();
}

bool ImageLabel::eventFilter(QObject* object, QEvent* event) {

    if (event->type() == QEvent::HoverMove)
        mousePos(reinterpret_cast<QMouseEvent*>(event)->pos());
    
    return false;
}


MaskSelectionDialog::MaskSelectionDialog(QWidget* image_window, QMdiArea* workspace_parent) : ProcessDialog("MaskSelection", QSize(400, 115), workspace_parent, false, false) {

    m_iw = image_window;

    ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &MaskSelectionDialog::apply, &MaskSelectionDialog::showPreview, &MaskSelectionDialog::resetDialog);
    this->setModal(true);

    QLabel* label = new QLabel("Current Mask:   ",this);
    label->move(10, 12);

    m_current_mask = new QLineEdit(this);
    m_current_mask->setReadOnly(true);
    m_current_mask->setText("No Mask");
    m_current_mask->setFixedWidth(250);
    m_current_mask->move(115, 10);

    m_new_mask = new ComboBox(this);
    m_new_mask->addItem("No Mask Selected");
    m_new_mask->setFixedWidth(250);
    m_new_mask->move(75, 50);

    auto iw = reinterpret_cast<const ImageWindow8*>(m_iw);
    for (auto sw : m_workspace->subWindowList()) {
        auto mask = reinterpret_cast<const ImageWindow8*>(sw->widget());
        if (mask->name() != iw->name() && iw->source().isSameSize(mask->source()) && mask->channels() <= iw->channels())
            m_new_mask->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());
    }

    if (iw->mask() != nullptr) {
        m_current_mask->setText(iw->mask()->name());
        m_new_mask->setCurrentText(iw->mask()->name());
    }

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->show();
}

void MaskSelectionDialog::apply() {
    auto ptr = imageRecast<uint8_t>(m_iw);

    for (auto sw : m_workspace->subWindowList()) {
        auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
        if (iw->name() == m_new_mask->currentText()) {
            m_current_mask->setText(iw->name());
            switch (ptr->type()) {
            case ImageType::UBYTE:
                ptr->setMask(iw);
                break;
            case ImageType::USHORT:
                imageRecast<uint16_t>(ptr)->setMask(iw);
                break;
            case ImageType::FLOAT:
                imageRecast<float>(ptr)->setMask(iw);
                break;
            }
            break;
        }
        else if (m_new_mask->currentIndex() == 0) {
            m_current_mask->setText("No Mask");
            ptr->removeMask();
        }
    }

    this->close();
}


ImageWindowMenu::ImageWindowMenu(QMdiArea* workspace, QWidget* parent) : m_parent(parent), QMenu(parent){
    m_workspace = workspace;
    //this->addAction(tr("Mask"), this, &ImageWindowMenu::addMaskMenu);
    addMaskMenu();

}

void ImageWindowMenu::removeMask() {
    ImageWindow8* iw = imageRecast(parentWidget());
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->removeMask();
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->removeMask();
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->removeMask();
    }
}

void ImageWindowMenu::showMask(bool show) {
    ImageWindow8* iw = imageRecast(parentWidget());
    switch(iw->type()) {
    case ImageType::UBYTE:
        return iw->showMask(show);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->showMask(show);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->showMask(show);
    }
}

void ImageWindowMenu::invertMask(bool invert) {
    ImageWindow8* iw = imageRecast(parentWidget());
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->invertMask(invert);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->invertMask(invert);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->invertMask(invert);
    }
}

void ImageWindowMenu::enableMask(bool enable) {
    ImageWindow8* iw = imageRecast(parentWidget());
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->enableMask(enable);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->enableMask(enable);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->enableMask(enable);
    }
}

void ImageWindowMenu::setMaskColor(const QColor& color) {
    ImageWindow8* iw = imageRecast(parentWidget());
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->setMaskColor(color);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->setMaskColor(color);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->setMaskColor(color);
    }
}

void ImageWindowMenu::addMaskMenu() {

    auto iw = reinterpret_cast<ImageWindow8*>(parentWidget());

    m_mask = new QMenu("Mask", this);
    this->addMenu(m_mask);
    m_mask->addAction(tr("Select Mask"), this, &ImageWindowMenu::addSelectMaskSelection);

    m_mask->addAction(tr("Remove Mask"), this, &ImageWindowMenu::removeMask);

    QWidgetAction* wa = new QWidgetAction(m_mask);
    wa->setText("Enable Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskEnabled());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::enableMask);

    wa = new QWidgetAction(m_mask);
    wa->setText("Show Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskVisible());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::showMask);

    wa = new QWidgetAction(m_mask);
    wa->setText("Invert Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskInverted());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::invertMask);


    addMaskColorSelection();
}

void ImageWindowMenu::addSelectMaskSelection() {
    if (m_msd == nullptr) {
        m_msd = new MaskSelectionDialog(parentWidget(), m_workspace);
        connect(m_msd, &ProcessDialog::onClose, this, [this]() {m_msd = nullptr; });
    }
}

void ImageWindowMenu::addMaskColorSelection() {

    QMenu* mask_color_menu = new QMenu("Mask Color", this);
    m_mask->addMenu(mask_color_menu);

    auto iw = reinterpret_cast<ImageWindow8*>(parentWidget());
    int id = std::find(m_mask_colors.begin(), m_mask_colors.end(), iw->maskColor()) - m_mask_colors.begin();

    for (int i = 0; i < m_color_names.size(); ++i) {
        QWidgetAction* wa = new QWidgetAction(m_mask);
        QPixmap pm(18, 18);
        pm.fill(m_mask_colors[i]);
        wa->setIcon(pm);
        wa->setText(m_color_names[i]);
        wa->setCheckable(true);

        if (i == id)
            wa->setChecked(true);

        mask_color_menu->addAction(wa);
        connect(wa, &QWidgetAction::toggled, this, [this, i]() { this->setMaskColor(m_mask_colors[i]); });
    }
}





static QImage::Format qimageFormat(int channels = 3) {

    switch (channels) {
    case 1:
        return QImage::Format_Grayscale8;
    case 3:
        return QImage::Format_RGB888;
    default:
        return QImage::Format_RGB888;
    }
}





ImageWindowToolbar::ImageWindowToolbar(int height, QWidget* parent) : QWidget(parent) {

    this->setFixedWidth(25);
    this->setMinimumHeight(width() * 2);
    this->resize(width(),height);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QPalette pal = QPalette();
    pal.setColor(QPalette::ButtonText, Qt::white);
    this->setPalette(pal);

    m_stf_pb = new CheckablePushButton("STF", this);
    m_stf_pb->setToolTip("Screen Transfer Function");
    m_stf_pb->resize(width(), width());
    m_stf_pb->setFont(QFont("Segoe UI", 8));
    m_stf_pb->setPalette(pal);
    m_stf_pb->setFlat(true);
    m_stf_pb->move(0, height - (4 * m_stf_pb->height()));
    connect(m_stf_pb, &QPushButton::released, this, [this]() { emit stfClicked(m_stf_pb->isChecked()); });

    /*m_zoom_win_pb = new CheckablePushButton("Z", this);
    m_zoom_win_pb->setToolTip("Zoom Window");
    m_zoom_win_pb->resize(width(), width());
    m_zoom_win_pb->setPalette(pal);
    m_zoom_win_pb->setFlat(true);
    m_zoom_win_pb->move(0, height - (4 * m_zoom_win_pb->height()));

    connect(m_zoom_win_pb, &QPushButton::released, this, [this]() { emit zoomWindowClicked(m_zoom_win_pb->isChecked()); });*/


    m_img3D_pb = new PushButton("3D", this);
    m_img3D_pb->setToolTip("3D Image");
    m_img3D_pb->resize(width(), width());
    m_img3D_pb->setPalette(pal);
    m_img3D_pb->setFlat(true);
    m_img3D_pb->move(0, height - (3 * m_img3D_pb->height()));
    connect(m_img3D_pb, &QPushButton::released, this, [this]() { emit image3DReleased(); });

    m_stats_pb = new PushButton("S", this);
    m_stats_pb->setToolTip("Statistics");
    m_stats_pb->resize(width(), width());
    m_stats_pb->setPalette(pal);
    m_stats_pb->setFlat(true);
    m_stats_pb->move(0, height - (2 * m_stats_pb->height()));
    connect(m_stats_pb, &QPushButton::released, this, [this]() { emit statisticsReleased(); });

    m_hist_pb = new PushButton("H", this);
    m_hist_pb->setToolTip("Histogram");
    m_hist_pb->resize(width(), width());
    m_hist_pb->setPalette(pal);
    m_hist_pb->setFlat(true);
    m_hist_pb->move(0, height - m_hist_pb->height());
    connect(m_hist_pb, &QPushButton::released, this, [this]() { emit histogramReleased(); });

    this->show();
}

void ImageWindowToolbar::resizeEvent(QResizeEvent* e) {

    QWidget::resizeEvent(e);

    m_stats_pb->move(0, height() - (2 * m_stats_pb->height()));
    m_hist_pb->move(0, height() - m_hist_pb->height());
}





template<typename T>
ImageWindow<T>::ImageWindow(Image<T>& img, QString name, QWidget* parent) : m_name(name), QWidget(parent) {

    m_workspace = reinterpret_cast<QMdiArea*>(parent);

    int count = 0;
    for (auto sw : m_workspace->subWindowList()) {
        auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
        if (name == ptr->name())
            m_name = name + "_" + QString::number(++count);
    }

    setAcceptDrops(true);
    installEventFilter(this);

    img.moveTo(m_source);

    int factor = computeZoomFactor(true);
    m_factor_poll = (factor == 1) ? factor : -factor;
    m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

    this->setWindowTitle("1:" + QString(std::to_string(abs(m_factor_poll)).c_str()) + " " + m_name);

    m_drows = m_source.rows() * m_factor;
    m_dcols = m_source.cols() * m_factor;
    m_dchannels = m_source.channels();
    m_display = QImage(cols(), rows(), qimageFormat(channels()));
    this->resize(cols(), rows());

    m_toolbar = new ImageWindowToolbar(rows(), this);
    connect(m_toolbar, &ImageWindowToolbar::statisticsReleased, this, &ImageWindow<T>::openStatisticsDialog);
    connect(m_toolbar, &ImageWindowToolbar::histogramReleased, this, &ImageWindow<T>::openHistogramDialog);
    connect(m_toolbar, &ImageWindowToolbar::image3DReleased, this, &ImageWindow<T>::openImage3DDialog);
    connect(m_toolbar, &ImageWindowToolbar::stfClicked, this, &ImageWindow<T>::showSTFImage);
    //connect(m_toolbar, &WindowToolbar::zoomWindowClicked, this, &ImageWindow<T>::showZoomWindow);

    m_sa = new IWScrollArea(this);
    m_sa->setGeometry(m_toolbar->width(), 0, cols(), rows());
    instantiateScrollBars();

    m_image_label = new ImageLabel(&m_display, this);
    m_image_label->setGeometry(m_toolbar->width(), 0, cols(), rows());
    m_image_label->setCursor(m_cursor);
    
    QIcon icon;
    icon.addFile("./Icons//fast_stack_icon_fs2.png");
    this->setWindowIcon(icon);

    auto ptr = reinterpret_cast<Workspace*>(parent);
    m_sw_parent = ptr->addSubWindow(this);
    m_sw_parent->setGeometry(ptr->m_offsetx, ptr->m_offsety, cols() + (2 * m_border_width) + m_toolbar->width(), rows() + (m_titlebar_height + m_border_width));
    ptr->UpdateOffsets();
    m_sw_parent->setWindowFlags(Qt::SubWindow | Qt::WindowShadeButtonHint);

    QPalette pal;
    pal.setColor(QPalette::Inactive, QPalette::ColorRole::Text, Qt::white);
    pal.setColor(QPalette::Active, QPalette::Highlight, QColor(39, 39, 39));
    pal.setColor(QPalette::Inactive, QPalette::Window, QColor(126, 126, 126));
    m_sw_parent->setPalette(pal);
    
    connect(m_ws.get(), &WindowSignals::windowOpened, ptr, &Workspace::receiveOpen);
    connect(m_ws.get(), &WindowSignals::windowClosed, ptr, &Workspace::receiveClose);

    connect(m_ws.get(), &WindowSignals::imageInfo, reinterpret_cast<FastStack*>(parent->parentWidget())->toolbar()->imageInformationLabel(), &ImageInformationLabel::displayText);
    connect(m_ws.get(), &WindowSignals::pixelValue, reinterpret_cast<FastStack*>(parent->parentWidget())->toolbar()->pixelValueLabel(), &PixelValueLabel::displayText);

    connect(m_image_label, &ImageLabel::mousePos, this, &ImageWindow<T>::mouseEnterMove_ImageLabel);
    connect(m_image_label, &ImageLabel::mouseLeave, this, &ImageWindow<T>::mouseLeave_ImageLabel);

    displayImage();
    m_sw_parent->show();
}

template<typename T>
int ImageWindow<T>::computeZoomFactor(bool workspace)const {

    QSize size = screen()->availableSize();

    if (workspace)
        size = m_workspace->size();

    int width = size.width();
    int height = size.height();

    int factor = 1;
    for (; factor < 10; ++factor) {

        int new_cols = m_source.cols() / factor;
        int new_rows = m_source.rows() / factor;

        if (new_cols < 0.75 * width && new_rows < 0.75 * height)
            break;
    }

    return factor;
}

template<typename T>
void ImageWindow<T>::setMask(const ImageWindow<uint8_t>* mask) {

    if (mask->channels() > channels())
        return;

    m_mask = mask;
    connect(m_mask->windowSignals(), &WindowSignals::windowClosed, this, &ImageWindow<T>::removeMask);

    m_mask_display = QImage(cols(), rows(), QImage::Format_ARGB32);
    m_image_label->setMask(&m_mask_display);

    this->displayImage();

    if (previewExists())
        previewRecast<T>(this->preview())->updatePreview();
}

template<typename T>
void ImageWindow<T>::removeMask() {
    m_mask = nullptr;
    m_mask_display = QImage();
    m_image_label->removeMask();
    m_image_label->draw();

    if (previewExists()) {
        switch (type()) {
        case ImageType::UBYTE:
            return reinterpret_cast<PreviewWindow8*>(m_preview.get())->updatePreview(false);
        case ImageType::USHORT:
            return reinterpret_cast<PreviewWindow16*>(m_preview.get())->updatePreview(false);
        case ImageType::FLOAT:
            return reinterpret_cast<PreviewWindow32*>(m_preview.get())->updatePreview(false);
        }
    }
}

template<typename T>
void ImageWindow<T>::enableMask(bool enable) { 
    m_mask_enabled = enable; 

    if (previewExists())
        reinterpret_cast<PreviewWindow<T>*>(preview())->updatePreview(false);
}

template<typename T>
void ImageWindow<T>::invertMask(bool invert) {
    m_invert_mask = invert;
    displayImage();

    if(previewExists())
        reinterpret_cast<PreviewWindow<T>*>(preview())->updatePreview(false);
}

template<typename T>
void ImageWindow<T>::showMask(bool show) {
    m_show_mask = show;
    m_image_label->showMask(show);
    displayImage();
}


template<typename T>
void ImageWindow<T>::showPreview(QDialog* preview) {

    if (m_preview == nullptr) {
        if (preview == nullptr)
            m_preview = std::unique_ptr<QDialog>(new PreviewWindow<T>(this));
        else
            m_preview = std::unique_ptr<QDialog>(preview);
    }
}

template<typename T>
void ImageWindow<T>::closePreview() {
    m_preview.reset(nullptr);
}

template<typename T>
void ImageWindow<T>::showZoomWindow(bool show) {

    //if (m_draw_zoom_win && m_zoom_window == nullptr) {
        //int factor = computeZoomFactor(true);
        //int w = m_source.cols() / (factor * factor);
        //int h = m_source.rows() / (factor * factor);

        //m_zoom_window = std::make_unique<ZoomPreviewWindow>(QRect(0, 0, w, h), m_label);
    //}

    //else
        //m_zoom_window.reset();
}


template<typename T>
void ImageWindow<T>::convertToGrayscale() {

    m_source.RGBtoGray();
    m_dchannels = m_source.channels();

    m_display = QImage(cols(), rows(), QImage::Format::Format_Grayscale8);

    m_ws->imageInfo(m_sw_parent);
    displayImage();
    updateStatisticsDialog();
    updateHistogramDialog();
}

template<typename T>
void ImageWindow<T>::mouseEnterMove_ImageLabel(const QPoint& p)const {

    float x = (p.x() / m_factor) + m_sourceOffX;
    float y = (p.y() / m_factor) + m_sourceOffY;

    m_ws->pixelValue(m_sw_parent, { x,y });
}

template<typename T>
void ImageWindow<T>::mouseLeave_ImageLabel()const {
    m_ws->pixelValue(nullptr, { 0,0 });
}


template<typename T>
void ImageWindow<T>::wheelEvent(QWheelEvent* event) {

    int dy = event->angleDelta().y() / 120;

    if (abs(dy) != 1)
        return;

    //limits image zooming range
    if (m_factor_poll + dy > m_max_factor_poll)
        return;
    
    else if (m_factor_poll + dy < m_min_factor_poll) 
        return;

    m_factor_poll += dy;

    //allows smooth transition between change in zoom_factors
    if (m_factor_poll < -1)
        m_factor = 1.0 / -m_factor_poll;

    else if ((m_factor_poll == -1 || m_factor_poll == 0) && dy > 0)
        m_factor = m_factor_poll = 1;
    
    else if (m_factor_poll >= 1)
        m_factor = m_factor_poll;

    else if ((m_factor_poll == 1 || m_factor_poll == 0) && dy < 0) {
        m_factor = 0.5; 
        m_factor_poll = -2;
    }


    QPoint p = event->scenePosition().toPoint();

    zoom(p.x(), p.y());

    if (m_zoom_window != nullptr)
        m_zoom_window->scaleWindow(dy);

    mouseEnterMove_ImageLabel(p);

    QString str = QString::number(abs(m_factor_poll));

    if (m_factor > 1)
        this->setWindowTitle(str + ":1" + " " + m_name);
    else
        this->setWindowTitle("1:" + str + " " + m_name);
}

template<typename T>
void ImageWindow<T>::dragEnterEvent(QDragEnterEvent* event) {

    if (event->mimeData()->hasFormat("process"))
        event->acceptProposedAction();
}

template<typename T>
void ImageWindow<T>::dropEvent(QDropEvent* event) {

    auto ptr = reinterpret_cast<Workspace*>(m_workspace);
    ptr->setActiveSubWindow(m_sw_parent);
    reinterpret_cast<ProcessDialog*>(event->source())->processDropped();
    event->acceptProposedAction();
}

template<typename T>
void ImageWindow<T>::mousePressEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) {
        m_image_label->setCursor(QCursor(Qt::CursorShape::SizeAllCursor));
        QPoint p = event->scenePosition().toPoint();

        m_mouseX = p.x();
        m_mouseY = p.y();

        if (m_draw_zoom_win) {
            m_zoom_window = std::make_unique<ZoomWindow>(*this, m_image_label);
            m_zoom_window->startCorner(p);
        }
    }

    if (event->buttons() == Qt::RightButton) {
        ImageWindowMenu* menu = new ImageWindowMenu(m_workspace, this);
        menu->exec(event->globalPos());
    }
}

template<typename T>
void ImageWindow<T>::mouseMoveEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) {
        QPoint p = event->scenePosition().toPoint();
        if (m_draw_zoom_win)
            m_zoom_window->endCorner(p);

        else {
            pan(p.x(), p.y());

            if (m_zoom_window)
                m_zoom_window->updatePos();
        }     
    }
}

template<typename T>
void ImageWindow<T>::mouseReleaseEvent(QMouseEvent* event) {

    if (m_draw_zoom_win) {
        QRect g = m_zoom_window->geometry();
        m_zoom_window = std::make_unique<ZoomWindow>(*this, g, m_image_label);
        connect(m_zoom_window->windowSignals(), &WindowSignals::windowClosed, this, [this]() { m_zoom_window.reset(); });
        m_draw_zoom_win = false;
    }

    m_image_label->setCursor(m_cursor);
}

template<typename T>
void ImageWindow<T>::closeEvent(QCloseEvent* close) {
    closePreview();
    m_ws->windowClosed();
    delete this;
    close->accept();
}

template<typename T>
void ImageWindow<T>::resizeEvent(QResizeEvent* event) {

    if (m_open == false) {
        m_open = true;
        m_ws->windowOpened();
        return;
    }

    int new_cols = size().width();
    int new_rows = size().height();

    if (new_cols <= m_sbY->width() || new_rows <= m_sbX->height())
        return;
    
    m_toolbar->resize(m_toolbar->width(), new_rows);

    m_sa->setGeometry(m_toolbar->width(), 0, new_cols - m_toolbar->width(), new_rows);

    //if window width greater than display image
    if (m_sa->width() >= int((m_source.cols() - m_sourceOffX) * m_factor) - 1) {
        double dx = (new_cols - width()) / m_factor;

        if (dx > 0)
            m_sourceOffX -= dx;

        m_sourceOffX = Clip(m_sourceOffX, 0.0, double(m_source.cols()));
        m_dcols = (m_source.cols() - m_sourceOffX) * m_factor;

       hideHorizontalScrollBar();
    }
    else {
        m_dcols = new_cols;
        showHorizontalScrollBar();
    }

    if (m_sa->height() >= int((m_source.rows() - m_sourceOffY) * m_factor) - 1) {
        double dy = (new_rows - height()) / m_factor;

        if (dy > 0)
            m_sourceOffY -= dy;

        m_sourceOffY = Clip(m_sourceOffY, 0.0, double(m_source.rows()));
        m_drows = (m_source.rows() - m_sourceOffY) * m_factor;

       hideVerticalScrollBar();
    }
    else {
        m_drows = new_rows;
        showVerticalScrollBar();
    }

    resizeDisplay();
    displayImage();
}

template<typename T>
bool ImageWindow<T>::eventFilter(QObject* object, QEvent* event) {
   
    if (event->type() == QEvent::HideToParent)
        m_sw_parent->resize(300, m_titlebar_height);

    return false;
}


template<typename T>
void ImageWindow<T>::instantiateScrollBars() {

    m_sbX = new ScrollBar(m_sa);
    m_sbX->setFixedHeight(17);//depends on resolution?!
    m_sa->setHorizontalScrollBar(m_sbX);

    auto actionX = [this](int action) {

        int pos = m_sbX->sliderPosition();

        if (pos == m_sbX->maximum())
            m_sourceOffX = m_source.cols() - cols() / m_factor;
        else
            m_sourceOffX = pos / m_initial_factor;

        displayImage();

        if (m_zoom_window)
            m_zoom_window->updatePos();
    };

    connect(m_sbX, &QScrollBar::actionTriggered, this, actionX);

    //
    //

    m_sbY = new ScrollBar(m_sa);
    m_sbY->setFixedWidth(17);
    m_sa->setVerticalScrollBar(m_sbY);

    auto actionY = [this](int action) {

        int pos = m_sbY->sliderPosition();

        if (pos == m_sbY->maximum())
            m_sourceOffY = m_source.rows() - rows() / m_factor;
        else
            m_sourceOffY = pos / m_initial_factor;

        displayImage();

        if (m_zoom_window)
            m_zoom_window->updatePos();
    };

    connect(m_sbY, &QScrollBar::actionTriggered, this, actionY);
}

template<typename T>
void ImageWindow<T>::showHorizontalScrollBar() {

    double page_step = (m_dcols / m_factor) * m_initial_factor;

    m_sbX->setRange(0, m_source.cols() * m_initial_factor - page_step);
    m_sbX->setValue(m_sourceOffX * m_initial_factor);
    m_sbX->setPageStep(page_step);

    m_sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

template<typename T>
void ImageWindow<T>::hideHorizontalScrollBar() {
    m_sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::showVerticalScrollBar() {

    double page_step = (m_drows / m_factor) * m_initial_factor;

    m_sbY->setRange(0, m_source.rows() * m_initial_factor - page_step);
    m_sbY->setValue(m_sourceOffY * m_initial_factor);
    m_sbY->setPageStep(page_step);

    m_sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

template<typename T>
void ImageWindow<T>::hideVerticalScrollBar() {
    m_sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::showScrollBars() {

    if (width() < m_dcols) {
        m_dcols = width();
        showHorizontalScrollBar();
    }
    else
        hideHorizontalScrollBar();

    if (height() < m_drows) {
        m_drows = height();
        showVerticalScrollBar();
    }
    else
        hideVerticalScrollBar();
}

template<typename T>
bool ImageWindow<T>::isHorizontalScrollBarOn()const {
    return (m_sa->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn);
}

template<typename T>
bool ImageWindow<T>::isVerticalScrollBarOn()const {
    return (m_sa->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn);
}

template<typename T>
bool ImageWindow<T>::isHorizontalScrollBarOff()const {
    return (m_sa->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
}

template<typename T>
bool ImageWindow<T>::isVerticalScrollBarOff()const {
    return (m_sa->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
}


template<typename T>
void ImageWindow<T>::resizeDisplay() {

    if (isVerticalScrollBarOn() && isHorizontalScrollBarOff()) {

        if (m_sa->width() > cols() + m_sbY->width()) {
            for (int i = 0; i < m_sbY->width(); ++i)
                if (rows() < int((m_source.rows() - m_sourceOffY) * m_factor))
                    m_drows++;

            hideHorizontalScrollBar();
        }

        else {
            m_drows = m_sa->height() - m_sbX->height();
            m_dcols = m_sa->width() - m_sbY->width();
            showHorizontalScrollBar();
        }

    }

    else if (isHorizontalScrollBarOn() && isVerticalScrollBarOff()) {

        if (m_sa->height() > rows() + m_sbX->height()) {
            for (int i = 0; i < m_sbX->height(); ++i)
                if (cols() < int((m_source.cols() - m_sourceOffX) * m_factor))
                    m_dcols++;
            hideVerticalScrollBar();
        }

        else {
            m_dcols = m_sa->width() - m_sbY->width();
            m_drows = m_sa->height() - m_sbX->height();
            showVerticalScrollBar();
        }

    }

    else if (isVerticalScrollBarOn() && isHorizontalScrollBarOn()) {
        m_dcols = m_sa->width() - m_sbY->width();
        m_drows = m_sa->height() - m_sbX->height();

        showVerticalScrollBar();
        showHorizontalScrollBar();
    }

    else {
        hideHorizontalScrollBar();
        hideVerticalScrollBar();
    }

    int dr = (isHorizontalScrollBarOn()) ? m_sbX->height() : 0;
    int dc = (isVerticalScrollBarOn()) ? m_sbY->width() : 0;

    QRect label_rect = QRect((width() + m_toolbar->width() - (cols() + dc)) / 2, (height() - (rows() + dr)) / 2, cols(), rows());

    if (label_rect != m_image_label->geometry())
        m_image_label->setGeometry(label_rect);

    if (rows() != m_display.height() || cols() != m_display.height()) {
        m_display = QImage(cols(), rows(), m_display.format());

        if (!m_mask_display.isNull())
            m_mask_display = QImage(cols(), rows(), m_mask_display.format());
    }
}

template<typename T>
void ImageWindow<T>::displayImage() {

    if (m_show_mask && m_mask != nullptr)
        displayMask();

    if (m_factor > 1)
        upsampleToWindow(m_factor);
    else
        binToWindow(1 / m_factor);

    m_image_label->draw();
}

template<typename T>
void ImageWindow<T>::displayMask() {

    m_mask_display.fill(Qt::black);

    switch (m_mask->type()) {
    case ImageType::UBYTE: {
        auto iw8 = m_mask;
        if (m_factor > 1)
            upsampleToWindow_Mask(iw8->source(), m_factor);
        else
            binToWindow_Mask(iw8->source(), 1 / m_factor);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = imageRecast<uint16_t>(m_mask);
        if (m_factor > 1)
            upsampleToWindow_Mask(iw16->source(), m_factor);
        else
            binToWindow_Mask(iw16->source(), 1 / m_factor);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = imageRecast<float>(m_mask);
        if (m_factor > 1)
            upsampleToWindow_Mask(iw32->source(), m_factor);
        else
            binToWindow_Mask(iw32->source(), 1 / m_factor);
        break;
    }
    }

    for (int y = 0; y < rows(); ++y) {
        for (int x = 0; x < cols(); ++x) {

            uint8_t* p = &m_mask_display.scanLine(y)[4 * x];

            T max = Max(Max(*p, *(p + 1)), *(p + 2));
            float n = (max / 255.0);

            *p = n * m_mask_color.blue();
            *(p + 1) = n * m_mask_color.green();
            *(p + 2) = n * m_mask_color.red();


            *(p + 3) = max;
        }
    }
}


template<typename T>
void ImageWindow<T>::zoom(int x, int y) {

    double x_old = x / m_old_factor + m_sourceOffX;
    double x_new = x / m_factor + m_sourceOffX;

    double y_old = y / m_old_factor + m_sourceOffY;
    double y_new = y / m_factor + m_sourceOffY;

    m_sourceOffX += (x_old - x_new);
    m_sourceOffY += (y_old - y_new);

    m_drows = m_source.rows() * m_factor;
    m_dcols = m_source.cols() * m_factor;

    showScrollBars();

    if (isHorizontalScrollBarOff())
        m_sourceOffX = 0.0;
    else 
        m_sourceOffX = Clip(m_sourceOffX, 0.0, m_source.cols() - cols() / m_factor);

    if (isVerticalScrollBarOff())
        m_sourceOffY = 0.0;
    else
        m_sourceOffY = Clip(m_sourceOffY, 0.0, m_source.rows() - rows() / m_factor);

    resizeDisplay();

    m_old_factor = m_factor;

    displayImage();
}

template<typename T>
void ImageWindow<T>::pan(int x, int y) {

    if (m_sbX->isVisible() || m_sbY->isVisible()) {

        m_sourceOffX -= double(x - m_mouseX)/m_factor;
        m_sourceOffY -= double(y - m_mouseY)/m_factor;

        m_mouseX = x;
        m_mouseY = y;

        m_sourceOffX = Clip(m_sourceOffX, 0.0, m_source.cols() - cols() / m_factor);
        m_sourceOffY = Clip(m_sourceOffY, 0.0, m_source.rows() - rows() / m_factor);
        
        m_sbX->setValue(m_sourceOffX * m_initial_factor);
        m_sbY->setValue(m_sourceOffY * m_initial_factor);

        displayImage();
    }
}


template<typename T>
void ImageWindow<T>::binToWindow(int factor) {

    if (m_toolbar->isSTFChecked())
        return binToWindow_stf(factor);

    if (m_dchannels == 3)
        return binToWindow_RGB(factor);

    int factor2 = factor * factor;

    for (int ch = 0; ch < channels(); ++ch) {
        for (int y = 0, y_s = m_sourceOffY; y < rows(); ++y, y_s += factor) {

            for (int x = 0, x_s = m_sourceOffX; x < cols(); ++x, x_s += factor) {

                float pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i) 
                        pix += m_source(x_s + i, y_s + j, ch);
                    
                m_display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(T(pix / factor2));
            }       
        }
    }
}

template<typename T>
void ImageWindow<T>::binToWindow_stf(int factor) {

    int factor2 = factor * factor;

    for (int ch = 0; ch < channels(); ++ch) {
        for (int y = 0, y_s = m_sourceOffY; y < rows(); ++y, y_s += factor) {
            for (int x = 0, x_s = m_sourceOffX; x < cols(); ++x, x_s += factor) {

                float pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += m_source(x_s + i, y_s + j, ch);

                pix = Pixel<float>::toType(T(pix / factor2));
                pix = m_ht.transformPixel(ColorComponent::rgb_k, pix);
                m_display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(pix);
            }
        }
    }
}

template<typename T>
void ImageWindow<T>::binToWindow_RGB(int factor) {

    int factor2 = factor * factor;

    for (int y = 0, y_s = m_sourceOffY; y < rows(); ++y, y_s += factor) {
        uint8_t* p = m_display.scanLine(y);
        for (int x = 0, x_s = m_sourceOffX; x < cols(); ++x, x_s += factor, p += 3) {

            float r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    auto color = m_source.color<>(x_s + i, y_s + j);
                    r += color.red();
                    g += color.green();
                    b += color.blue();
                }
            }

            *p = Pixel<uint8_t>::toType(T(r / factor2));
            *(p + 1) = Pixel<uint8_t>::toType(T(g / factor2));
            *(p + 2) = Pixel<uint8_t>::toType(T(b / factor2));
        }
    }
}

template<typename T>
void ImageWindow<T>::upsampleToWindow(int factor) {

    if (m_toolbar->isSTFChecked())
        return upsampleToWindow_stf(factor);

    if (m_dchannels == 3)
        return upsampleToWindow_RGB(factor);

    double dd = 1.0 / factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        double y_s = m_sourceOffY;
        for (int y = 0; y < rows(); ++y, y_s += dd) {
            double x_s = m_sourceOffX;
            for (int x = 0; x < cols(); ++x, x_s += dd) {

                m_display.scanLine(y)[m_dchannels * (x)+ch] = Pixel<uint8_t>::toType(m_source(x_s, y_s, ch));

            }
        }
    }
}

template<typename T>
void ImageWindow<T>::upsampleToWindow_stf(int factor) {

    double dd = 1.0 / factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        double y_s = m_sourceOffY;
        for (int y = 0; y < rows(); ++y, y_s += dd) {
            double x_s = m_sourceOffX;
            for (int x = 0; x < cols(); ++x, x_s += dd) {

                float pix = m_ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(m_source(x_s, y_s, ch)));
                m_display.scanLine(y)[m_dchannels * (x)+ch] = Pixel<uint8_t>::toType(pix);

            }
        }
    }
}

template<typename T>
void ImageWindow<T>::upsampleToWindow_RGB(int factor) {

    double dd = 1.0 / factor;
    double y_s = m_sourceOffY;

    for (int y = 0; y < rows(); ++y, y_s += dd) {
        double x_s = m_sourceOffX;
        uint8_t* p = m_display.scanLine(y);
        for (int x = 0; x < cols(); ++x, x_s += dd, p += 3) {

            auto color = m_source.color<float>(x_s, y_s);
            *p = Pixel<uint8_t>::toType(color.red());
            *(p + 1) = Pixel<uint8_t>::toType(color.green());
            *(p + 2) = Pixel<uint8_t>::toType(color.blue());

        }
    }
}

template<typename T>
void ImageWindow<T>::openStatisticsDialog() {

    if (m_stats_dialog == nullptr) {

        int precision = (m_source.type() == ImageType::FLOAT) ? 7 : 1;

        if (m_sv.empty())
            m_sv = Statistics::computeStatistics(m_source);

        m_stats_dialog = std::make_unique<StatisticsDialog>(name(), m_sv, precision, this);

        connect(m_stats_dialog.get(), &StatisticsDialog::onClose, this, [this]() { m_stats_dialog.reset(); });

        auto clipped = [this](bool v) {

            int precision = (m_source.type() == ImageType::FLOAT) ? 7 : 1;

            if (v && m_sv_clipped.empty())
                m_sv_clipped = Statistics::computeStatistics(m_source, v);

            m_stats_dialog->populateStats((v) ? m_sv_clipped : m_sv, precision);
        };

        connect(m_stats_dialog.get(), &StatisticsDialog::clipped, this, clipped);
    }
}

template<typename T>
void ImageWindow<T>::updateStatisticsDialog() {

    m_sv.clear();
    m_sv_clipped.clear();

    if (m_stats_dialog != nullptr) {

        auto& sv = (m_stats_dialog->isChecked()) ? m_sv_clipped : m_sv;
        sv = Statistics::computeStatistics(m_source, m_stats_dialog->isChecked());

        int precision = (m_source.type() == ImageType::FLOAT) ? 7 : 1;
        m_stats_dialog->populateStats(sv, precision);
    }
}

template<typename T>
void ImageWindow<T>::openHistogramDialog() {

    if (m_hist_dialog == nullptr) {
        m_hist_dialog = std::make_unique<HistogramDialog>(name(), m_source, this);
        connect(m_hist_dialog.get(), &HistogramDialog::onClose, this, [this]() { m_hist_dialog.reset(); });
    }
}

template<typename T>
void ImageWindow<T>::updateHistogramDialog() {

    if (m_hist_dialog != nullptr) 
        m_hist_dialog->histogramView()->drawHistogramScene(m_source); 
}

template<typename T>
void ImageWindow<T>::openImage3DDialog() {

    if (m_img3D_dialog == nullptr) {
        m_img3D_dialog = std::make_unique<Image3DDialog>(name(), m_source, this);
        connect(m_img3D_dialog.get(), &Image3DDialog::onClose, this, [this]() { m_img3D_dialog.reset(); });
    }
}

template<typename T>
void ImageWindow<T>::showSTFImage() {

    if (m_compute_stf) {
        m_ht.computeSTFCurve(m_source);
        m_compute_stf = false;
    }

    displayImage();

    if(m_preview)
        previewRecast<T>(m_preview.get())->updatePreview();
}

template class ImageWindow<uint8_t>;
template class ImageWindow<uint16_t>;
template class ImageWindow<float>;





QRect ZoomWindow::frameRect()const {

    float t = m_pen_width / 2;

    QRect rect = this->rect();

    rect.setX(rect.x() + t);
    rect.setY(rect.y() + t);
    rect.setWidth(rect.width() - 2 * t);
    rect.setHeight(rect.height() - 2 * t);


    return rect;
}

bool ZoomWindow::isInBounds(const QRect rect)const {

    double f = m_iw->factor();

    if (m_iw->sourceOffsetX() < -rect.left())
        return false;

    if ((m_iw->source().cols() - m_iw->sourceOffsetX()) * f <= rect.right())
        return false;

    if (m_iw->sourceOffsetY() < -rect.top())
        return false;

    if ((m_iw->source().rows() - m_iw->sourceOffsetY()) * f <= rect.bottom())
        return false;

    return true;
}

void ZoomWindow::startCorner(const QPoint& p) {
    m_pos = p;
    this->setGeometry(QRect(p, QSize(0, 0)));
    update();
}

void ZoomWindow::endCorner(const QPoint& p) {

    QPoint start = m_pos;
    QRect frame;

    if (p.x() < start.x() && p.y() < start.y())
        frame = QRect(p, start);

    else if (p.x() < start.x() && start.y() < p.y())
        frame = QRect(QPoint(p.x(), start.y()), QPoint(start.x(), p.y()));

    else if (start.x() < p.x() && start.y() < p.y())
        frame = QRect(start, p);

    else if (start.x() < p.x() && p.y() < start.y())
        frame = QRect(QPoint(start.x(), p.y()), QPoint(p.x(), start.y()));

    if (!isInBounds(frame))
        return;

    this->setGeometry(frame);
    update();
}

void ZoomWindow::scaleWindow(int direction) {

    if (abs(direction) != 1)
        return;

    int x = (m_img_rect.x() - m_iw->sourceOffsetX()) * m_iw->factor();
    int y = (m_img_rect.y() - m_iw->sourceOffsetY()) * m_iw->factor();

    int w = m_img_rect.width() * m_iw->factor();
    int h = m_img_rect.height() * m_iw->factor();

    this->setGeometry(x, y, w, h);
    update();
}

void ZoomWindow::moveBy(int dx, int dy) {

    int x = (m_img_rect.x() - m_iw->sourceOffsetX()) * m_iw->factor() + dx;
    int y = (m_img_rect.y() - m_iw->sourceOffsetY()) * m_iw->factor() + dy;

    this->move(x, y);
}

void ZoomWindow::updatePos() {
    int x = (m_img_rect.x() - m_iw->sourceOffsetX()) * m_iw->factor();
    int y = (m_img_rect.y() - m_iw->sourceOffsetY()) * m_iw->factor();
    this->move(x, y);
}

void ZoomWindow::mousePressEvent(QMouseEvent* e) {
    if (e->buttons() == Qt::LeftButton)
        m_pos = e->pos();

    if (e->buttons() == Qt::RightButton)
        m_ws.windowClosed();
}

void ZoomWindow::mouseMoveEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton) {

        QRect frame = this->geometry();

        QPoint pos = mapToParent(e->pos());
        pos -= m_pos;

        frame.moveTo(pos);

        double f = m_iw->factor();

        if (!isInBounds(frame))
            return;

        float x = frame.x() / f + m_iw->sourceOffsetX();
        float y = frame.y() / f + m_iw->sourceOffsetY();
        float w = frame.width() / f;
        float h = frame.height() / f;
        m_img_rect = QRect(x, y, w, h);

        this->move(pos);
    }
}

void ZoomWindow::paintEvent(QPaintEvent* e) {

    QPainter p(this);

    QPen pen(QColor(75, 0, 130));
    pen.setWidth(m_pen_width);
    p.setPen(pen);

    p.drawRect(frameRect());
}




//need seperate constructor for zoom window as crop preview is dependent on this
template<typename T>
PreviewWindow<T>::PreviewWindow(QWidget* parent) : QDialog(parent) {
    installEventFilter(this);

    auto iw_parent = reinterpret_cast<ImageWindow<T>*>(parent);
    m_bin_factor = iw_parent->computeZoomFactor();

    m_drows = iw_parent->source().rows() / m_bin_factor;
    m_dcols = iw_parent->source().cols() / m_bin_factor;
    m_dchannels = iw_parent->channels();

    this->resize(cols(), rows());

    //dependent on zoom window
    m_source = Image<T>(rows(), cols(), channels());
    m_display = QImage(cols(), rows(), qimageFormat(channels()));
    m_image_label = new ImageLabel(&m_display, this);
    m_image_label->setGeometry(0, 0, cols(), rows());

    m_ws->windowOpened();
    connect(m_ws, &WindowSignals::windowClosed, iw_parent, &ImageWindow<T>::closePreview);

    updateSource();
    displayImage();

    this->setWindowTitle(iw_parent->name() + "Preview: ");

    setWindowFlags(Qt::Tool | Qt::MSWindowsFixedSizeDialogHint);
    setAttribute(Qt::WA_DeleteOnClose);
    show();
}

template<typename T>
void PreviewWindow<T>::closeEvent(QCloseEvent* close) {
    m_ws->windowClosed();
    close->accept();
}

template<typename T>
bool PreviewWindow<T>::eventFilter(QObject* o, QEvent* e) {

    if (o == this && e->type() == QEvent::NonClientAreaMouseButtonPress)
        this->setWindowOpacity(.55);
    if (o == this && e->type() == QEvent::NonClientAreaMouseButtonRelease)
        this->setWindowOpacity(1.0);

    return false;
}

template<typename T>
void PreviewWindow<T>::updateSource() {

    int factor = m_bin_factor;
    int factor2 = factor * factor;

    Image<T>& src = parentWindow()->source();

    if (m_source.channels() != src.channels()) {
        m_dchannels = src.channels();
        m_source = Image<T>(m_drows, m_dcols, m_dchannels);

        m_display.convertTo(qimageFormat(m_dchannels));
    }


    for (int ch = 0; ch < m_dchannels; ++ch) {
        for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
            for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

                double pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += src(x_s + i, y_s + j, ch);

                m_source(x, y, ch) = pix / factor2;
            }
        }
    }
}

template<typename T>
void PreviewWindow<T>::updateSourcefromZoomWindow() {

    Image<T>& src = parentWindow()->source();
    int factor = m_bin_factor;
    int factor2 = factor * factor;

    QRect r =  parentWindow()->zoomWindow()->imageRect();

}

template<typename T>
void PreviewWindow<T>::updatePreview(bool from_parent) {

    if (parentWindow()->maskEnabled() && parentWindow()->maskExists())
        return updatePreview_Mask();

    if (from_parent)
        updateSource();

    displayImage();
}

template<typename T>
void PreviewWindow<T>::updatePreview_Mask() {
    if (!parentWindow()->maskExists())
        return displayImage();

    Image<T> mod = Image<T>(m_source);
    updateSource();

    Image32 mask = [this]() -> Image32 {
        switch (parentWindow()->mask()->type()) {

        case ImageType::UBYTE:
            return this->getMask<uint8_t>(m_bin_factor);

        case ImageType::USHORT:
            return this->getMask<uint16_t>(m_bin_factor);

        case ImageType::FLOAT:
            return this->getMask<float>(m_bin_factor);

        default:
            return this->getMask<uint8_t>(m_bin_factor);

        }
    }();


    bool invert = parentWindow()->maskInverted();

    for (int ch = 0; ch < channels(); ++ch) {
        int mask_ch = (ch < mask.channels()) ? ch : 0;
        for (int y = 0; y < rows(); ++y) {
            for (int x = 0; x < cols(); ++x) {

                float m = mask(x, y, mask_ch);
                m = (invert) ? 1 - m : m;
                m_display.scanLine(y)[channels() * x + ch] = 255 * Pixel<float>::toType(T(m_source(x, y, ch) * (1 - m) + mod(x, y, ch) * m));
            }
        }
    }

    mod.copyTo(m_source);
    m_image_label->draw();
}

template<typename T>
void PreviewWindow<T>::ImagetoQImage_stf() {

    auto ht = parentWindow()->histogramTransformation();

    if (rows() != m_display.height() || cols() != m_display.width() || channels() != m_display.depth() / 3) {

        QImage::Format format;

        if (channels() == 1)
            format = QImage::Format::Format_Grayscale8;

        else if (channels() == 3)
            format = QImage::Format::Format_RGB888;

        m_display = QImage(cols(), rows(), format);
    }

    for (int ch = 0; ch < channels(); ++ch) {
        for (int y = 0; y < rows(); ++y) {
            for (int x = 0; x < cols(); ++x) {
                float pix = ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(m_source(x, y, ch)));
                m_display.scanLine(y)[channels() * x + ch] = Pixel<uint8_t>::toType(pix);
            }
        }
    }
}

template<typename T>
void PreviewWindow<T>::displayImage() {

    if (parentWindow()->toolbar()->isSTFChecked())
        ImagetoQImage_stf();
    else
        ImagetoQImage(m_source, m_display);

    m_image_label->draw();
}

template class PreviewWindow<uint8_t>;
template class PreviewWindow<uint16_t>;
template class PreviewWindow<float>;
