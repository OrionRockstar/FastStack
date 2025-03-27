#include "pch.h"
#include "ImageWindow.h"
#include "FastStack.h"
#include "FastStackToolBar.h"
#include "ImageGeometryDialogs.h"
#include "ImageWindowMenu.h"


ImageLabel::ImageLabel(const QImage& img, QWidget* parent) : m_image(&img), QLabel(parent) {

    this->setAttribute(Qt::WA_Hover);
    installEventFilter(this);
    this->setCursor(m_cross_cursor);
}

void ImageLabel::paintEvent(QPaintEvent* event) {

    QPainter p(this);
    p.setOpacity(m_opacity);

    if (m_image != nullptr)
        p.drawImage(QPoint(0, 0), *m_image);

    if (m_mask_visible && m_mask != nullptr)
        p.drawImage(QPoint(0, 0), *m_mask);
}

bool ImageLabel::eventFilter(QObject* object, QEvent* event) {

    if (event->type() == QEvent::MouseButtonPress)
        if (reinterpret_cast<QMouseEvent*>(event)->buttons() == Qt::LeftButton)
            this->setCursor(m_pan_cursor);
        
    if (event->type() == QEvent::MouseButtonRelease)
        this->resetCursor();

    return false;
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

    this->setFixedWidth(m_width);
    this->setMinimumHeight(m_width * 1);
    this->resize(m_width, height);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QPalette pal = QPalette();
    pal.setColor(QPalette::ButtonText, Qt::white);
    this->setPalette(pal);

    m_bg = new QButtonGroup(this);
    m_bg->setExclusive(false);

    m_bg->addButton(m_reset_pb = new PushButton("R", this));
    m_reset_pb->setToolTip("Reset display");
    connect(m_reset_pb, &QPushButton::released, this, [this]() { emit resetReleased(); });

    m_bg->addButton(m_hist_pb = new PushButton("H", this));
    m_hist_pb->setToolTip("Histogram");
    connect(m_hist_pb, &QPushButton::released, this, [this]() { emit histogramReleased(); });

    m_bg->addButton(m_stats_pb = new PushButton("S", this));
    m_stats_pb->setToolTip("Statistics");
    connect(m_stats_pb, &QPushButton::released, this, [this]() { emit statisticsReleased(); });

    m_bg->addButton(m_img3D_pb = new PushButton("3D", this));
    m_img3D_pb->setToolTip("3D Image");
    connect(m_img3D_pb, &QPushButton::released, this, [this]() { emit image3DReleased(); });

    m_bg->addButton(m_stf_pb = new CheckablePushButton("STF", this));
    m_stf_pb->setToolTip("Screen Transfer Function");
    m_stf_pb->setFont(QFont("Segoe UI", 8));
    connect(m_stf_pb, &QPushButton::released, this, [this]() { emit stfClicked(m_stf_pb->isChecked()); });

    m_bg->addButton(m_zoom_win_pb = new CheckablePushButton("Z", this));
    m_zoom_win_pb->setToolTip("Zoom Window");
    connect(m_zoom_win_pb, &QPushButton::released, this, [this]() { emit zoomWindowClicked(m_zoom_win_pb->isChecked()); });

    for (auto pb : m_bg->buttons()) {
        pb->resize(m_width, m_width);
        pb->setPalette(pal);
        dynamic_cast<QPushButton*>(pb)->setFlat(true);
        pb->move(0, height + (m_bg->id(pb) + 1) * m_width);
    }


    this->show();
}

void ImageWindowToolbar::resizeEvent(QResizeEvent* e) {

    QWidget::resizeEvent(e);

    for (auto pb : m_bg->buttons()) 
        pb->move(0, height() + (m_bg->id(pb) + 1) * m_width);
}





SubWindow::SubWindow(QWidget* widget) {

    this->setWidget(widget);
    this->setMinimumSize(200, 200);
    this->setWindowFlags(Qt::SubWindow | Qt::WindowShadeButtonHint);
    
    m_pal.setColor(QPalette::Active, QPalette::Text, QColor(69, 0, 128, 169));
    m_pal.setColor(QPalette::Inactive, QPalette::Text, Qt::white);
    m_pal.setColor(QPalette::Active, QPalette::Highlight, QColor(39, 39, 39));
    m_pal.setColor(QPalette::Light, QColor(69, 0, 128,169));
    m_pal.setColor(QPalette::Inactive, QPalette::Window, QColor(126, 126, 126));

    this->setPalette(m_pal);


    m_opaque_pal.setColor(QPalette::Active, QPalette::Highlight, QColor(39, 39, 39, 140));
    m_opaque_pal.setColor(QPalette::Active, QPalette::Window, QColor(39, 39, 39, 140));
    m_opaque_pal.setColor(QPalette::Light, QColor(69, 0, 128, 140));


    //m_timer = new QTimer(this);
    //m_timer->setSingleShot(true);
    //connect(m_timer, &QTimer::timeout, this, &SubWindow::makeOpaque);
}

void SubWindow::makeOpaque() {

    this->setPalette(m_opaque_pal);
    if (this->widget() != nullptr)
        imageRecast<>(this->widget())->setOpaque();
}

/*void SubWindow::mousePressEvent(QMouseEvent* e) {

    QMdiSubWindow::mousePressEvent(e);

    if (e->buttons() == Qt::LeftButton)
        m_timer->start(500);

}

void SubWindow::mouseMoveEvent(QMouseEvent* e) {

    QMdiSubWindow::mouseMoveEvent(e);

    if (e->buttons() == Qt::LeftButton && m_timer->id() != Qt::TimerId::Invalid) {
        m_timer->stop();
        makeOpaque();
    }

}

void SubWindow::mouseReleaseEvent(QMouseEvent* e) {

    QMdiSubWindow::mouseReleaseEvent(e);

    if (e->button() == Qt::LeftButton) {
        if (m_timer->id() != Qt::TimerId::Invalid)
            m_timer->stop();
        else {
            this->setPalette(m_pal);
            if (widget() != nullptr)
                imageRecast<>(widget())->unsetOpaque();
        }
    }
}*/



template<typename T>
ImageWindow<T>::ImageWindow(Image<T>&& img, QString name, QWidget* parent) : m_source(std::move(img)), m_name(name), QWidget(parent) {

    m_workspace = reinterpret_cast<QMdiArea*>(parent);

    int count = 0;
    for (auto sw : m_workspace->subWindowList()) {
        auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
        if (name == ptr->name())
            m_name = name + "_" + QString::number(++count);
    }

    setAcceptDrops(true);
    installEventFilter(this);
    //setMinimumSize(100, 100);

    int factor = computeBinFactor(true);
    m_factor_poll = (factor == 1) ? factor : -factor;
    m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

    this->setWindowTitle("1:" + QString(std::to_string(abs(m_factor_poll)).c_str()) + " " + m_name);

    m_drows = m_source.rows() * m_factor;
    m_dcols = m_source.cols() * m_factor;
    m_dchannels = m_source.channels();
    m_default_size = { m_dcols,m_drows };
    m_display = QImage(cols(), rows(), qimageFormat(channels()));

    m_toolbar = new ImageWindowToolbar(rows(), this);
    connect(m_toolbar, &ImageWindowToolbar::resetReleased, this, &ImageWindow<T>::resetWindowSize);
    connect(m_toolbar, &ImageWindowToolbar::statisticsReleased, this, &ImageWindow<T>::openStatisticsDialog);
    connect(m_toolbar, &ImageWindowToolbar::histogramReleased, this, &ImageWindow<T>::openHistogramDialog);
    connect(m_toolbar, &ImageWindowToolbar::image3DReleased, this, &ImageWindow<T>::openImage3DDialog);
    connect(m_toolbar, &ImageWindowToolbar::stfClicked, this, &ImageWindow<T>::showSTFImage);
    connect(m_toolbar, &ImageWindowToolbar::zoomWindowClicked, this, &ImageWindow<T>::enableZoomWindowMode);

    m_sa = new IWScrollArea(this);
    m_sa->setGeometry(m_toolbar->width(), 0, cols(), rows());
    instantiateScrollBars();

    m_image_label = new ImageLabel(m_display, m_sa);
    m_image_label->setGeometry(0, 0, cols(), rows());
    m_image_label->installEventFilter(this);

    auto ptr = reinterpret_cast<Workspace*>(parent);
    m_sw_parent = dynamic_cast<SubWindow*>(ptr->addSubWindow(new SubWindow(this)));
    m_sw_parent->setGeometry(ptr->m_offsetx, ptr->m_offsety, cols() + (2 * m_border_width) + m_toolbar->width(), rows() + (m_titlebar_height + m_border_width));
    ptr->UpdateOffsets();

    m_image_label->installEventFilter(this);

    auto ftb = reinterpret_cast<FastStack*>(parent->parentWidget())->toolbar();
    connect(m_iis.get(), &ImageInfoSignals::imageInfo, ftb->imageInformationLabel(), &ImageInformationLabel::displayText);
    connect(m_iis.get(), &ImageInfoSignals::pixelValue, ftb->pixelValueLabel(), &PixelValueLabel::displayText);

    emit m_ws->windowCreated();
    emit dynamic_cast<Workspace*>(m_workspace)->imageWindowCreated();

    displayImage();
    m_sw_parent->show();
}

template<typename T>
int ImageWindow<T>::computeBinFactor(bool workspace)const {

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
        preview()->updatePreview();
}

template<typename T>
void ImageWindow<T>::removeMask() {

    m_mask = nullptr;
    m_mask_display = QImage();
    m_image_label->removeMask();
    m_image_label->draw();

    if (previewExists())
        preview()->updatePreview(false);
    
}

template<typename T>
void ImageWindow<T>::enableMask(bool enable) { 

    m_mask_enabled = enable; 

    if (previewExists())
        preview()->updatePreview(false);
}

template<typename T>
void ImageWindow<T>::invertMask(bool invert) {

    m_invert_mask = invert;
    displayImage();

    if(previewExists())
        preview()->updatePreview(false);
}

template<typename T>
void ImageWindow<T>::showMask(bool show) {

    m_show_mask = show;
    m_image_label->setMaskVisible(show);
    displayImage();
}

template<typename T>
void ImageWindow<T>::setZoomWindowColor(const QColor& color) {
    
    m_zwc = color;

    if (m_zoom_window)
        m_zoom_window->setPenColor(m_zwc);
}

template<typename T>
void ImageWindow<T>::showPreview(PreviewWindow<T>* preview) {

    if (m_preview == nullptr) {
        if (preview == nullptr)
            m_preview = std::make_unique<PreviewWindow<T>>(this, false);
        else
            m_preview = std::unique_ptr<PreviewWindow<T>>(preview);

        connect(m_preview.get()->windowSignals(), &WindowSignals::windowClosed, this, [this]() { m_preview.release(); });
    }

}

template<typename T>
void ImageWindow<T>::closePreview() {

    if (previewExists())
        preview()->close();

    m_preview.release();
}


template<typename T>
void ImageWindow<T>::convertToGrayscale() {

    if (channels() != 3)
        return;

    m_source.RGBtoGray();
    m_dchannels = m_source.channels();

    m_display = QImage(cols(), rows(), QImage::Format::Format_Grayscale8);

    m_compute_stf = true;

    emit imageInfoSignals()->imageInfo(m_sw_parent);
    displayImage();
    updateStatisticsDialog();
    updateHistogramDialog();
}

template<typename T>
void ImageWindow<T>::setOpaque() {

    m_image_label->setOpacity(0.55);
    m_sa->setOpaquePal();
    m_sbX->setOpacity(0.55);
    m_sbY->setOpacity(0.55);
}

template<typename T>
void ImageWindow<T>::unsetOpaque() {

    m_image_label->setOpacity(1.0);
    m_sa->unsetOpaquePal();
    m_sbX->setOpacity(1.0);
    m_sbY->setOpacity(1.0);
}

template<typename T>
void ImageWindow<T>::resetWindowSize() {

    int factor = computeBinFactor(true);
    m_factor_poll = (factor == 1) ? factor : -factor;
    m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);
    m_drows = m_source.rows() * m_factor;
    m_dcols = m_source.cols() * m_factor;

    m_sourceOffX = m_sourceOffY = 0;
    m_sw_parent->resize(cols() + (2 * m_border_width) + m_toolbar->width(), rows() + (m_titlebar_height + m_border_width));

    zoom(0, 0);

    if (m_zoom_window)
        m_zoom_window->scaleWindow();

    QString str = QString::number(abs(m_factor_poll));

    if (m_factor > 1)
        this->setWindowTitle(str + ":1" + " " + m_name);
    else
        this->setWindowTitle("1:" + str + " " + m_name);
}

template<typename T>
void ImageWindow<T>::mouseMove_ImageLabel(const QPoint& p)const {

    float x = (p.x() / m_factor) + m_sourceOffX;
    float y = (p.y() / m_factor) + m_sourceOffY;

    emit imageInfoSignals()->pixelValue(reinterpret_cast<const Image8*>(&m_source), { x,y });
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


    QPoint p = m_image_label->mapFrom(this, event->position().toPoint());

    zoom(p.x(), p.y());

    if (m_zoom_window)
        m_zoom_window->scaleWindow();

    mouseMove_ImageLabel(p);

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

        QPoint p = m_image_label->mapFrom(this, event->pos());

        m_mouseX = p.x();
        m_mouseY = p.y();

        if (m_draw_zoom_win && m_image_label->rect().contains(p)) {
            m_image_label->setAttribute(Qt::WA_TransparentForMouseEvents);
            m_image_label->setCursor(m_pencil_cursor);
            m_zoom_window = std::make_unique<ZoomWindow>(p, m_zwc, *this, m_image_label);
        }
    }

}

template<typename T>
void ImageWindow<T>::mouseMoveEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) {
        QPoint p = m_image_label->mapFrom(this, event->pos());

        if (m_zoom_window && m_draw_zoom_win)
            m_zoom_window->endCorner(p);

        else {

            pan(p.x(), p.y());

            if (m_zoom_window)
                m_zoom_window->moveBy(0, 0);
        }     
    }
}

template<typename T>
void ImageWindow<T>::mouseReleaseEvent(QMouseEvent* event) {

    if (event->button() == Qt::RightButton) {
        auto c = childAt(event->pos());
        if (c == m_image_label || c == m_sa->viewport()) {
            ImageWindowMenu* menu = new ImageWindowMenu(*this, this);
            menu->exec(event->globalPos());

            if (m_draw_zoom_win)
                m_image_label->setCursor(m_pencil_cursor);
        }
    }

    if (m_zoom_window && m_draw_zoom_win) {

        m_image_label->setAttribute(Qt::WA_TransparentForMouseEvents, false);

        m_draw_zoom_win = false;

        connect(m_zoom_window.get(), &ZoomWindow::windowClosed, this, &ImageWindow<T>::onZoomWindowClose);

        emit dynamic_cast<Workspace*>(m_workspace)->zoomWindowCreated();
        emit m_zoom_window->windowMoved();
    }
}

template<typename T>
void ImageWindow<T>::closeEvent(QCloseEvent* close) {

    closePreview();

    emit m_ws->windowClosed();
    emit workspaceRecast(m_workspace)->imageWindowClosed();

    m_workspace->removeSubWindow(m_sw_parent);

    delete this;
    close->accept();
}

template<typename T>
void ImageWindow<T>::resizeEvent(QResizeEvent* event) {

    m_toolbar->resize(m_toolbar->width(), height());

    m_sa->setGeometry(m_toolbar->width(), 0, width() - m_toolbar->width(), height());

    double dx = (width() - event->oldSize().width()) / m_factor;
    if (dx > 0) {
        if (m_dcols + dx >= (m_source.cols() - m_sourceOffX) * m_factor) {
            m_sourceOffX -= dx;
            m_sourceOffX = math::clip(m_sourceOffX, 0.0, m_source.cols() - cols() / m_factor);
        }
        else
            m_dcols -= dx;
    }

    double dy = (height() - event->oldSize().height()) / m_factor;
    if (dy > 0) {
        if (m_drows + dx >= (m_source.rows() - m_sourceOffY) * m_factor) {
            m_sourceOffY -= dy;
            m_sourceOffY = math::clip(m_sourceOffY, 0.0, m_source.cols() - cols() / m_factor);
        }
        else
            m_drows -= dx;
    }

    if(m_zoom_window)
        m_zoom_window->moveBy(0, 0);

    showScrollBars();
    resizeImageLabel();
    displayImage();
}

template<typename T>
bool ImageWindow<T>::eventFilter(QObject* object, QEvent* event) {

    if (object == this && event->type() == QEvent::HideToParent)
        m_sw_parent->resize(300, m_titlebar_height);

    if (object == m_image_label) {
        if (event->type() == QEvent::HoverMove)
            mouseMove_ImageLabel(dynamic_cast<QHoverEvent*>(event)->pos());

        if (event->type() == QEvent::Leave)
            emit imageInfoSignals()->pixelValue(nullptr, { 0,0 });
    }

    return false;
}


template<typename T>
void ImageWindow<T>::instantiateScrollBars() {

    m_sbX = new ScrollBar(m_sa);
    m_sa->setHorizontalScrollBar(m_sbX);

    auto actionX = [this](int action) {

        int pos = m_sbX->sliderPosition();

        if (pos == m_sbX->maximum())
            m_sourceOffX = m_source.cols() - cols() / m_factor;
        else
            m_sourceOffX = pos / m_initial_factor;

        displayImage();

        if (m_zoom_window)
            m_zoom_window->moveBy(0, 0);
    };

    connect(m_sbX, &QScrollBar::actionTriggered, this, actionX);

    //
    //

    m_sbY = new ScrollBar(m_sa);
    m_sa->setVerticalScrollBar(m_sbY);

    auto actionY = [this](int action) {

        int pos = m_sbY->sliderPosition();

        if (pos == m_sbY->maximum())
            m_sourceOffY = m_source.rows() - rows() / m_factor;
        else
            m_sourceOffY = pos / m_initial_factor;

        displayImage();

        if (m_zoom_window)
            m_zoom_window->moveBy(0, 0);
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

    int w = m_source.cols() * m_factor;
    int h = m_source.rows() * m_factor;

    if ((width() - m_toolbar->width()) < w)
        showHorizontalScrollBar();  
    else
        hideHorizontalScrollBar();

    if (height() < h)
        showVerticalScrollBar();  
    else
        hideVerticalScrollBar();

    auto vps = m_sa->viewportSize();

    m_dcols = (w > vps.width()) ? vps.width() : w;
    m_drows = (h > vps.height()) ? vps.height() : h;

    if (m_sa->isVerticalScrollBarOn() && m_sa->isHorizontalScrollBarOff()) {
        if (vps.width() > cols()) {
            m_drows += m_sbY->thickness();
            showVerticalScrollBar();
            hideHorizontalScrollBar();
        }
        else {
            m_drows = m_sa->height() - m_sbX->thickness();
            showHorizontalScrollBar();
        }
    }

    else if (m_sa->isHorizontalScrollBarOn() && m_sa->isVerticalScrollBarOff()) {
        if (vps.height() > rows()) {
            m_dcols += m_sbX->thickness();
            showHorizontalScrollBar();
            hideVerticalScrollBar();
        }
        else {
            m_dcols = m_sa->width() - m_sbY->thickness();
            showVerticalScrollBar();
        }
    }

    else if (m_sa->isVerticalScrollBarOn() && m_sa->isHorizontalScrollBarOn()) {}

    else {
        hideHorizontalScrollBar();
        hideVerticalScrollBar();
    }

    m_drows = math::clip(m_drows, 0, int(m_source.rows() * m_factor));
    m_dcols = math::clip(m_dcols, 0, int(m_source.cols() * m_factor));
}


template<typename T>
void ImageWindow<T>::resizeImageLabel() {

    int dr = (m_sa->isHorizontalScrollBarOn()) ? m_sbX->height() : 0;
    int dc = (m_sa->isVerticalScrollBarOn()) ? m_sbY->width() : 0;

    int x = math::max(0, (width() - m_toolbar->width() - (cols() + dc)) / 2);
    int y = math::max(0, (height() - (rows() + dr)) / 2);

    QRect label_rect = QRect(x, y, cols(), rows());

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

            T max = math::max(math::max(*p, *(p + 1)), *(p + 2));
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

    m_old_factor = m_factor;

    showScrollBars();
    resizeImageLabel();

    if (m_sa->isHorizontalScrollBarOff())
        m_sourceOffX = 0.0;
    else 
        m_sourceOffX = math::clip(m_sourceOffX, 0.0, m_source.cols() - cols() / m_factor);

    if (m_sa->isVerticalScrollBarOff())
        m_sourceOffY = 0.0;
    else
        m_sourceOffY = math::clip(m_sourceOffY, 0.0, m_source.rows() - rows() / m_factor);

    displayImage();
}

template<typename T>
void ImageWindow<T>::pan(int x, int y) {

    if (m_sbX->isVisible() || m_sbY->isVisible()) {

        m_sourceOffX -= double(x - m_mouseX)/m_factor;
        m_sourceOffY -= double(y - m_mouseY)/m_factor;

        m_mouseX = x;
        m_mouseY = y;

        m_sourceOffX = math::clip(m_sourceOffX, 0.0, m_source.cols() - cols() / m_factor);
        m_sourceOffY = math::clip(m_sourceOffY, 0.0, m_source.rows() - rows() / m_factor);
        
        m_sbX->setValue(m_sourceOffX * m_initial_factor);
        m_sbY->setValue(m_sourceOffY * m_initial_factor);

        displayImage();
    }
}


template<typename T>
void ImageWindow<T>::binToWindow(int factor) {

    //m_display = m_display.convertToFormat(QImage::Format::Format_Grayscale8);
        //return binToWindow_Colorspace(factor);

    if (m_toolbar->isSTFChecked())
        return binToWindow_stf(factor);

    if (m_dchannels == 3)
        return binToWindow_RGB(factor);

    int factor2 = factor * factor;

    for (int ch = 0; ch < channels(); ++ch) {
        for (int y = 0, y_s = sourceOffsetY(); y < rows(); ++y, y_s += factor) {
            for (int x = 0, x_s = sourceOffsetX(); x < cols(); ++x, x_s += factor) {

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
        for (int y = 0, y_s = sourceOffsetY(); y < rows(); ++y, y_s += factor) {
            for (int x = 0, x_s = sourceOffsetX(); x < cols(); ++x, x_s += factor) {

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

    for (int y = 0, y_s = sourceOffsetY(); y < rows(); ++y, y_s += factor) {
        uint8_t* p = m_display.scanLine(y);
        for (int x = 0, x_s = sourceOffsetX(); x < cols(); ++x, x_s += factor, p += 3) {

            float r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    auto color = m_source.color(x_s + i, y_s + j);
                    r += color.red();
                    g += color.green();
                    b += color.blue();
                }
            }

            *p = Pixel<uint8_t>::toType(T(r / factor2));
            *(p + 1) = Pixel<uint8_t>::toType(T(g / factor2));
            *(p + 2)= Pixel<uint8_t>::toType(T(b / factor2));
        }
    }
}

template<typename T>
void ImageWindow<T>::binToWindow_Colorspace(int factor) {

    int factor2 = factor * factor;

    auto RGBtoColorspace = [this](const Color<double>& color)->double {
        /*using CSC = ColorSpace::Channel;

        auto colorspace = CSC::ciea;

        switch (colorspace) {

        case CSC::red:
            return color.red();
        case CSC::green:
            return color.green();
        case CSC::blue:
            return color.blue();
        case CSC::ciel:
            return ColorSpace::CIEL(color);
        case CSC::ciea:
            return ColorSpace::CIEa(color);
        case CSC::cieb:
            return ColorSpace::CIEb(color);
        default:
            return 0.0;
        }*/
        return 0;
    };

    for (int y = 0, y_s = m_sourceOffY; y < rows(); ++y, y_s += factor) {
        for (int x = 0, x_s = m_sourceOffX; x < cols(); ++x, x_s += factor) {

            double r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    auto color = m_source.color<double>(x_s + i, y_s + j);
                    r += color.red();
                    g += color.green();
                    b += color.blue();
                }
            }

            r /= factor2;
            g /= factor2;
            b /= factor2;

            m_display.scanLine(y)[x] = Pixel<uint8_t>::toType(RGBtoColorspace({ r,g,b }));
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

    for (int ch = 0; ch < channels(); ++ch) {
        double y_s = sourceOffsetY();
        for (int y = 0; y < rows(); ++y, y_s += dd) {
            double x_s = sourceOffsetX();
            for (int x = 0; x < cols(); ++x, x_s += dd) {

                m_display.scanLine(y)[m_dchannels * (x)+ch] = Pixel<uint8_t>::toType(m_source(x_s, y_s, ch));

            }
        }
    }
}

template<typename T>
void ImageWindow<T>::upsampleToWindow_stf(int factor) {

    double dd = 1.0 / factor;

    for (int ch = 0; ch < channels(); ++ch) {
        double y_s = sourceOffsetY();
        for (int y = 0; y < rows(); ++y, y_s += dd) {
            double x_s = sourceOffsetX();
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

    double y_s = sourceOffsetY();
    for (int y = 0; y < rows(); ++y, y_s += dd) {
        double x_s = sourceOffsetX();
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

        m_stats_dialog = std::make_unique<StatisticsDialog>(name(), m_sv, precision, m_workspace);

        connect(m_stats_dialog.get(), &StatisticsDialog::windowClosed, this, [this]() { m_stats_dialog.reset(); });

        auto clipped = [this](bool v) {

            int precision = (m_source.type() == ImageType::FLOAT) ? 7 : 1;

            if (v && m_sv_clipped.empty())
                m_sv_clipped = Statistics::computeStatistics(m_source, v);

            m_stats_dialog->updateStats((v) ? m_sv_clipped : m_sv);
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

        m_stats_dialog->updateStats(sv);
    }
}

template<typename T>
void ImageWindow<T>::openHistogramDialog() {

    if (m_hist_dialog == nullptr) {
        m_hist_dialog = std::make_unique<HistogramDialog>(name(), m_source, m_workspace);
        connect(m_hist_dialog.get(), &HistogramDialog::windowClosed, this, [this]() { m_hist_dialog.reset(); });
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
        m_img3D_dialog = std::make_unique<Image3DDialog>(name(), m_source, m_workspace);
        connect(m_img3D_dialog.get(), &Image3DDialog::windowClosed, this, [this]() { m_img3D_dialog.reset(); });
    }
}

template<typename T>
void ImageWindow<T>::showSTFImage() {

    if (m_compute_stf) {
        m_ht.computeSTFCurve(m_source);
        m_compute_stf = false;
    }

    displayImage();

    if (m_preview) {
        auto crop = dynamic_cast<CropPreview<T>*>(m_preview.get());
        if (crop)
            crop->updatePreview(false);
        else
            previewRecast<T>(m_preview.get())->updatePreview(false);
    }
}

template<typename T>
void ImageWindow<T>::enableZoomWindowMode() {

    m_draw_zoom_win = m_toolbar->isZoomChecked();

    if (m_draw_zoom_win)
        m_image_label->setCursor(m_pencil_cursor);

    else {
        m_image_label->resetCursor();
        if (m_zoom_window)
            m_zoom_window->close();
    }
}

template<typename T>
void ImageWindow<T>::onZoomWindowClose() {

    m_zoom_window.release();

    m_draw_zoom_win = m_toolbar->isZoomChecked();

    if (m_draw_zoom_win)
        m_image_label->setCursor(m_pencil_cursor);

    emit dynamic_cast<Workspace*>(m_workspace)->zoomWindowClosed();
}

template class ImageWindow<uint8_t>;
template class ImageWindow<uint16_t>;
template class ImageWindow<float>;






void ZoomWindow::keepFrameInImage_draw(QRect& rect)const {

    double f = m_iw->factor();

    if (m_iw->sourceOffsetX() * f < -rect.left())
        rect.setLeft(-m_iw->sourceOffsetX() * f);

    if ((m_iw->source().cols() - m_iw->sourceOffsetX()) * f <= rect.right())
        rect.setRight((m_iw->source().cols() - m_iw->sourceOffsetX()) * f - 1);

    if (m_iw->sourceOffsetY() * f < -rect.top())
        rect.setTop(-m_iw->sourceOffsetY() * f);

    if ((m_iw->source().rows() - m_iw->sourceOffsetY()) * f <= rect.bottom())
        rect.setBottom((m_iw->source().rows() - m_iw->sourceOffsetY()) * f - 1);
}

void ZoomWindow::keepFrameInImage_move(QRect& rect)const {

    double f = m_iw->factor();

    if (m_iw->sourceOffsetX() * f < -rect.left())
        rect.moveLeft(-m_iw->sourceOffsetX() * f);

    if ((m_iw->source().cols() - m_iw->sourceOffsetX()) * f <= rect.right())
        rect.moveLeft((m_iw->source().cols() - m_iw->sourceOffsetX()) * f - rect.width());
    
    if (m_iw->sourceOffsetY() * f < -rect.top())
        rect.moveTop(-m_iw->sourceOffsetY() * f);
    
    if ((m_iw->source().rows() - m_iw->sourceOffsetY()) * f <= rect.bottom())
        rect.moveTop((m_iw->source().rows() - m_iw->sourceOffsetY()) * f - rect.height());
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

    keepFrameInImage_draw(frame);

    this->setGeometry(frame);

    frame.adjust(penWidth(), penWidth(), -penWidth(), -penWidth());

    double factor = m_iw->factor();
    float x = frame.x() / factor + m_iw->sourceOffsetX();
    float y = frame.y() / factor + m_iw->sourceOffsetY();
    float w = frame.width() / factor;
    float h = frame.height() / factor;
    m_img_rect = QRectF(x, y, w, h);

    update();
}

void ZoomWindow::scaleWindow() {

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

void ZoomWindow::closeEvent(QCloseEvent* e) {
    emit windowClosed();
    e->accept();
}

void ZoomWindow::mousePressEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
        m_pos = e->pos();
    }
}

void ZoomWindow::mouseMoveEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton) {

        QRect frame = this->geometry();

        QPoint pos = mapToParent(e->pos());
        pos -= m_pos;

        frame.moveTo(pos);

        keepFrameInImage_move(frame);

        auto p = frame.topLeft();
        frame.adjust(penWidth(), penWidth(), -penWidth(), -penWidth());

        double factor = m_iw->factor();
        float x = frame.x() / factor + m_iw->sourceOffsetX();
        float y = frame.y() / factor + m_iw->sourceOffsetY();
        float w = frame.width() / factor;
        float h = frame.height() / factor;
        m_img_rect = QRectF(x, y, w, h);

        this->move(p);      
    }
}

void ZoomWindow::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button() == Qt::LeftButton) {
        setCursor(parentWidget()->cursor());
        emit windowMoved();
    }

    if (e->button() == Qt::RightButton && rect().contains(e->pos()))
        this->close();
    
}

void ZoomWindow::paintEvent(QPaintEvent* e) {

    QPainter p(this);

    QPen pen(penColor());
    pen.setWidth(penWidth());
    p.setPen(pen);

    QRect r = this->rect();
    int t = penWidth() / 2;

    r.adjust(t, t, -t, -t);

    p.drawRect(r);

    pen.setWidth(1);
    p.setPen(pen);

    float mid_x = r.x() + r.width() / 2.0;
    float mid_y = r.y() + r.height() / 2.0;

    float c1 = (mid_y - r.y()) / 5.0;
    QLineF l1 = { QPointF(mid_x, r.y() + 4 * c1), { mid_x, r.y() + 6 * c1 } };
    float c2 = (mid_x - r.x()) / 5.0;
    QLineF l2 = { QPointF(r.x() + 4 * c2, mid_y), { r.x() + 6 * c2, mid_y} };

    float length_2 = math::min(l1.length(), l2.length()) / 2;
    float length_4 = length_2 / 2;

    p.drawLine(QPointF(mid_x, mid_y - length_2), { mid_x, mid_y + length_2 });
    p.drawLine(QPointF(mid_x - length_2, mid_y), { mid_x + length_2, mid_y });
    p.drawRect(QRectF(QPointF(mid_x - length_4, mid_y - length_4), QPointF(mid_x + length_4, mid_y + length_4)));
}






PreviewImageLabel::PreviewImageLabel(const QImage& img, QWidget* parent) : m_image(&img), QLabel(parent) {

    this->setAttribute(Qt::WA_Hover);
    installEventFilter(this);
    //this->setMouseTracking(true);
    this->setCursor(m_cross_cursor);
}

void PreviewImageLabel::paintEvent(QPaintEvent* event) {

    QPainter p(this);
    p.setOpacity(m_opacity);

    if (m_image != nullptr)
        p.drawImage(QPoint(0, 0), *m_image);
}




template<typename T>
PreviewWindow<T>::PreviewWindow(QWidget* image_window, bool ignore_zoom_window) : m_ingore_zoom_window(ignore_zoom_window), m_image_window(imageRecast<T>(image_window)), Dialog(imageRecast<T>(image_window)->workspace()) {

    this->setOpacity(1.0);

    drawArea()->setAutoFillBackground(true);

    auto s = screen()->availableSize();
    s.setWidth(9 * s.width() / 16);
    s.setHeight(2 * s.height() / 3);

    m_scale_factor = computeScaleFactor( QSize(imageWindow()->source().cols(), imageWindow()->source().rows()), s);

    m_drows = imageWindow()->source().rows() * scaleFactor();
    m_dcols = imageWindow()->source().cols() * scaleFactor();
    m_dchannels = imageWindow()->channels();

    this->resizeDialog(s.width(), s.height());

    m_source = Image<T>(rows(), cols(), channels());
    m_display = QImage(cols(), rows(), qimageFormat(channels()));
    m_image_label = new PreviewImageLabel(m_display, drawArea());
    
    int offy = abs(s.height() - (int)rows()) / 2;
    int offx = abs(s.width() - (int)cols()) / 2;

    m_image_label->setGeometry(offx, offy, cols(), rows());

    m_pal.setColor(QPalette::Window, QColor(39, 39, 39));
    m_opaque_pal.setColor(QPalette::Window, QColor(39, 39, 39, 140));

    connect(m_timer, &QTimer::timeout, this, &PreviewWindow<T>::makeOpaque);

    this->installEventFilter(this);
    m_image_label->installEventFilter(this);

    auto ftb = reinterpret_cast<FastStack*>(parentWidget()->parentWidget())->toolbar();
    connect(m_iis.get(), &ImageInfoSignals::previewPixelValue, ftb->pixelValueLabel(), &PixelValueLabel::displayPreviewText);

    setWindowFlags(this->windowFlags() ^ Qt::WindowStaysOnTopHint);

    //setWindowFlags(Qt::WindowStaysOnBottomHint);
    setAttribute(Qt::WA_DeleteOnClose);

    emit windowSignals()->windowCreated();

    show();
}

template<typename T>
void PreviewWindow<T>::makeOpaque() {

    this->titlebar()->setOpaque();
    this->m_image_label->setOpacity(0.55);
    this->setOpacity(0.55);
    drawArea()->setPalette(m_opaque_pal);
    update();
}

template<typename T>
void PreviewWindow<T>::mouseMoveEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton && m_moving) {

        if (m_timer->id() != Qt::TimerId::Invalid) {
            m_timer->stop();
            makeOpaque();
        }

        this->move(geometry().topLeft() + (e->pos() - m_start_pos));
    }
}

template<typename T>
void PreviewWindow<T>::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button() == Qt::LeftButton) {

        if (m_timer->id() != Qt::TimerId::Invalid)
            m_timer->stop();

        if (m_moving) {
            this->titlebar()->unsetOpaque();
            this->m_image_label->setOpacity(1.0);
            this->setOpacity(1.0);
            drawArea()->setPalette(m_pal);
            update();
        }

        m_moving = false;
    }
}

template<typename T>
void PreviewWindow<T>::closeEvent(QCloseEvent* close) {

    emit windowSignals()->windowClosed();
    close->accept();
}

template<typename T>
bool PreviewWindow<T>::eventFilter(QObject* obj, QEvent* e) {

    if (obj == m_image_label) {
        if (e->type() == QEvent::HoverMove) {
            auto src = reinterpret_cast<const Image8*>(&m_source);
            auto p = reinterpret_cast<QHoverEvent*>(e)->pos();
            float _s = 1 / scaleFactor();
            if (!m_ingore_zoom_window && imageWindow()->zoomWindow())
                emit m_iis->previewPixelValue(src, p, _s, m_zwtl);

            else
                emit m_iis->previewPixelValue(src, p, _s);
        }
        if (e->type() == QEvent::Leave)
            emit m_iis->previewPixelValue(nullptr, {0,0}, 0);
    }

    return false;
}

template<typename T>
void PreviewWindow<T>::resizeSource() {

    const Image<T>& src = imageWindow()->source();

    QSize size = QSize(src.cols(), src.rows());
    m_zwtl = { 0,0 };

    if (!m_ingore_zoom_window && imageWindow()->zoomWindow()) {
        QRectF r = imageWindow()->zoomWindow()->imageRectF();
        size = r.size().toSize();
        m_zwtl = r.topLeft();
    }

    m_scale_factor = computeScaleFactor(size, drawArea()->size());

    if (cols() != uint32_t(size.width() * scaleFactor()) || rows() != uint32_t(size.height() * scaleFactor()) || m_source.channels() != src.channels()) {

        m_drows = size.height() * scaleFactor();
        m_dcols = size.width() * scaleFactor();
        m_dchannels = src.channels();

        m_source = Image<T>(rows(), cols(), channels());
        m_display = QImage(cols(), rows(), qimageFormat(channels()));

        int offy = abs(drawArea()->height() - (int)rows()) / 2;
        int offx = abs(drawArea()->width() - (int)cols()) / 2;

        m_image_label->setGeometry(offx, offy, cols(), rows());
    }
}

template<typename T>
void PreviewWindow<T>::updateSource() {

    resizeSource();
    const Image<T>& src = imageWindow()->source();

    float _s = 1 / scaleFactor();

    if (_s >= 1.0f) {
        int f = _s;
        int f2 = f * f;

        for (int ch = 0; ch < channels(); ++ch) {
            for (int y = 0; y < rows(); ++y) {
                float y_s = y * _s + m_zwtl.y();

                for (int x = 0; x < cols(); ++x) {
                    float x_s = x * _s + m_zwtl.x();

                    double pix = 0;

                    for (int j = 0; j < f; ++j)
                        for (int i = 0; i < f; ++i)
                            pix += src(x_s + i, y_s + j, ch);

                    m_source(x, y, ch) = pix / f2;
                }
            }
        }
    }

    else {
        for (int ch = 0; ch < channels(); ++ch) {
            for (int y = 0; y < rows(); ++y) {
                int y_s = y * _s + m_zwtl.y();

                for (int x = 0; x < cols(); ++x) {
                    int x_s = x * _s + m_zwtl.x();

                    m_source(x, y, ch) = src(x_s, y_s, ch);
                }
            }
        }
    }
}

template<typename T>
double PreviewWindow<T>::computeScaleFactor(const QSize& s, const QSize& draw_area_size)const {

    if (s.isEmpty())
        return 1.0;

    double rx = double(draw_area_size.width()) / s.width();
    double ry = double(draw_area_size.height()) / s.height();

    return math::min(rx, ry);
}

template<typename T>
void PreviewWindow<T>::updatePreview(bool from_parent) {

    if (imageWindow()->maskEnabled() && imageWindow()->maskExists())
        return updatePreview_Mask();

    if (from_parent) 
        updateSource();

    displayImage();
}

template<typename T>
void PreviewWindow<T>::updatePreview_Mask() {

    if (!imageWindow()->maskExists())
        return displayImage();

    resizeSource();
    Image<T> mod = Image<T>(m_source);
    updateSource();

    Image32 mask = [this]() -> Image32 {
        switch (imageWindow()->mask()->type()) {

        case ImageType::UBYTE:
            return this->getMask<uint8_t>();

        case ImageType::USHORT:
            return this->getMask<uint16_t>();

        case ImageType::FLOAT:
            return this->getMask<float>();

        default:
            return this->getMask<uint8_t>();

        }
    }();


    bool invert = imageWindow()->maskInverted();

    bool apply_stf = imageWindow()->toolbar()->isSTFChecked();
    auto ht = imageWindow()->histogramTransformation();

    for (int ch = 0; ch < channels(); ++ch) {
        int mask_ch = (ch < mask.channels()) ? ch : 0;
        for (int y = 0; y < rows(); ++y) {
            for (int x = 0; x < cols(); ++x) {

                float m = mask(x, y, mask_ch);

                if (invert)
                    m = 1 - m;

                float pix = Pixel<float>::toType(T(m_source(x, y, ch) * (1 - m) + mod(x, y, ch) * m));

                if (apply_stf) 
                    pix = ht.transformPixel(ColorComponent::rgb_k, pix);
                
                m_display.scanLine(y)[channels() * x + ch] = 255 * pix;
            }
        }
    }

    mod.copyTo(m_source);
    m_image_label->draw();
}

template<typename T>
void PreviewWindow<T>::displayImage() {

    if (imageWindow()->toolbar()->isSTFChecked()) {

        auto ht = imageWindow()->histogramTransformation();

        if (rows() != m_display.height() || cols() != m_display.width() || channels() != m_display.depth() / 3)
            m_display = QImage(cols(), rows(), qimageFormat(channels()));

        for (int ch = 0; ch < channels(); ++ch) {
            for (int y = 0; y < rows(); ++y) {
                for (int x = 0; x < cols(); ++x) {
                    float pix = ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(m_source(x, y, ch)));
                        m_display.scanLine(y)[channels() * x + ch] = Pixel<uint8_t>::toType(pix);
                }
            }
        }
    }

    else
        ImagetoQImage(m_source, m_display);

    m_image_label->draw();
}

template class PreviewWindow<uint8_t>;
template class PreviewWindow<uint16_t>;
template class PreviewWindow<float>;
