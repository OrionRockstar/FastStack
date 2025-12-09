#include "pch.h"
#include "ImageWindow.h"
#include "FastStack.h"
#include "FastStackToolBar.h"
#include "ImageGeometryDialogs.h"
#include "ImageWindowMenu.h"
#include "ProcessDialog.h"


ImageLabel::ImageLabel(const QImage& img, QWidget* parent) : m_image(&img), QLabel(parent) {

    this->setAttribute(Qt::WA_Hover);
    this->setCrossCursor();
}

void ImageLabel::paintEvent(QPaintEvent* e ) {

    QPainter p(this);
    p.setOpacity(m_opacity);

    if (m_image != nullptr)
        p.drawImage(QPoint(0,0), *m_image);
}





ImageWindowLabel::ImageWindowLabel(const QImage& img, const ImageWindowBase* iw, QWidget* parent) : m_iw(iw), ImageLabel(img, parent) {

    installEventFilter(this);
}

void ImageWindowLabel::paintEvent(QPaintEvent* e) {

    ImageLabel::paintEvent(e);

    QPainter p(this);
    p.setOpacity(opacity());

    if (m_mask_visible && m_mask != nullptr)
        p.drawImage(QPoint(0, 0), *m_mask);

    p.setRenderHint(QPainter::Antialiasing);
    
    if (m_iw->psfVector()) {
        QPointF offset = reinterpret_cast<const ImageWindow8*>(m_iw)->sourceOffset().toQPointF(); 
        auto f = m_iw->factor();
        QRect r = rect();
        r.adjust(-50*f, -50*f, 50*f, 50*f);

        for (const PSF& psf : *m_iw->psfVector()) {
            float dx = (psf.xc - offset.x()) * f;
            float dy = (psf.yc - offset.y()) * f;

            if (&psf == m_iw->selectedPSF()) {
                p.setPen(Qt::green);
                p.drawLine(QPointF(dx,0), QPointF(dx,height()));
                p.drawLine(QPointF(0, dy), QPointF(width(), dy));
            }
            else {
                if (psf.saturated)
                    p.setPen(QColor( 0, 229, 229));
                else
                    p.setPen(QColor(229, 0, 229));
            }
            if (r.contains(dx, dy)) {
                p.translate(dx, dy);
                p.rotate(math::radiansToDegrees(psf.theta));
                p.drawEllipse(QPointF(0.0f, 0.0f), f * psf.fwhmx, f * psf.fwhmy);
                p.resetTransform();
            }
        }
    }
}

bool ImageWindowLabel::eventFilter(QObject* object, QEvent* event) {

    if (event->type() == QEvent::MouseButtonPress)
        if (reinterpret_cast<QMouseEvent*>(event)->buttons() == Qt::LeftButton)
            this->setCursor(m_pan_cursor);

    if (event->type() == QEvent::MouseButtonRelease)
        this->setCrossCursor();

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





ImageWindowBase::ImageWindowBase(const QString& name, Workspace* parent) : m_name(name), m_workspace(parent), QWidget(parent) {

    using IWB = ImageWindowBase;

    int count = 0;
    for (auto sw : m_workspace->subWindowList()) {
        auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
        if (name == ptr->name() || name + QString::number(count) == ptr->name())
            count++;
    }

    m_name = name + ((count != 0) ? QString::number(count) : "");

    m_sw_parent = m_workspace->addImageSubWindow(new ImageSubWindow(this));

    m_toolbar = new ImageWindowToolbar(this);
    m_sa = new IWScrollArea(this);
    m_sa->setHorizontalScrollBar(m_sbX = new ScrollBar());
    m_sa->setVerticalScrollBar(m_sbY = new ScrollBar());
    m_image_label = new ImageWindowLabel(m_display, this, m_sa);
    m_image_label->installEventFilter(this);


    m_toolbar->connectToolbar(this, &IWB::resetWindowSize, &IWB::openHistogramDialog, 
        &IWB::openStatisticsDialog, &IWB::openImage3DDialog, &IWB::enableSTF, &IWB::enableZoomWindowMode, &IWB::openPSFDialog);

    auto setOpacity = [this](float opacity) {
        m_image_label->setOpacity(opacity);
        m_sa->setOpacity(opacity);
        m_toolbar->setOpacity(opacity);
        m_sbY->setOpacity(opacity);
        m_sbX->setOpacity(opacity);
        update();
    };
    connect(m_sw_parent, &ImageSubWindow::actionTriggered, this, std::bind(setOpacity, 0.55));
    connect(m_sw_parent, &ImageSubWindow::actionFinished, this, std::bind(setOpacity, 1.0));

    auto ftb = reinterpret_cast<FastStack*>(m_workspace->parentWidget())->toolbar();
    connect(this, &IWB::pixelValue, ftb->pixelValueLabel(), &PixelValueLabel::displayText);
    setAcceptDrops(true);
    this->setAttribute(Qt::WA_DeleteOnClose);
}

int ImageWindowBase::computeBinFactor(const QSize& img_size, bool to_workspace)const {

    QSize size = screen()->availableSize();
    if (to_workspace)
        size = m_workspace->size();

    int width = size.width();
    int height = size.height();
    int factor = 1;
    for (; factor < 10; ++factor) {
        int new_cols = img_size.width() / factor;
        int new_rows = img_size.height() / factor;

        if (new_cols < 0.75 * width && new_rows < 0.75 * height)
            break;
    }

    return factor;
}

void ImageWindowBase::setZoomWindowColor(const QColor& color) {

    m_zwc = color;
    if (m_zoom_window)
        m_zoom_window->setPenColor(m_zwc);
}

void ImageWindowBase::closeEvent(QCloseEvent* e) {

    emit windowClosed();
    m_workspace->removeImageSubWindow(m_sw_parent);
    e->accept();
}

void ImageWindowBase::dragEnterEvent(QDragEnterEvent* e) {

    if (e->mimeData()->hasFormat("process"))
        e->acceptProposedAction();
}

void ImageWindowBase::dropEvent(QDropEvent* e) {

    auto ptr = reinterpret_cast<Workspace*>(m_workspace);
    ptr->setActiveSubWindow(m_sw_parent);
    reinterpret_cast<ProcessDialog*>(e->source())->processDropped();
    e->acceptProposedAction();
}

void ImageWindowBase::resiseEvent(QResizeEvent* e) {

    m_toolbar->setHeight(height());
    m_sa->setGeometry(m_toolbar->width(), 0, width() - m_toolbar->width(), height());
}

void ImageWindowBase::wheelEvent(QWheelEvent* e) {

    int dy = e->angleDelta().y() / 120;

    if (abs(dy) != 1)
        return;

    //limits image zooming range
    if (factorPoll() + dy > m_max_factor_poll)
        return;

    else if (factorPoll() + dy < m_min_factor_poll)
        return;

    m_factor_poll += dy;

    //allows smooth transition between change in zoom_factors
    if (factorPoll() < -1)
        m_factor = 1.0 / -factorPoll();

    else if ((factorPoll() == -1 || factorPoll() == 0) && dy > 0)
        m_factor = m_factor_poll = 1;

    else if (factorPoll() >= 1)
        m_factor = factorPoll();

    else if ((factorPoll() == 1 || factorPoll() == 0) && dy < 0) {
        m_factor = 0.5;
        m_factor_poll = -2;
    }

    QString str = QString::number(abs(factorPoll()));

    if (factor() > 1)
        this->setWindowTitle(str + ":1" + " " + m_name);
    else
        this->setWindowTitle("1:" + str + " " + m_name);
}

void ImageWindowBase::enableZoomWindowMode(bool enable) {

    m_enable_zoom_win = m_draw_zoom_win = enable;

    if (m_draw_zoom_win)
        m_image_label->setCursor(m_pencil_cursor);

    else {
        m_image_label->setCrossCursor();
        if (m_zoom_window)
            m_zoom_window->close();
    }
}

void ImageWindowBase::onZoomWindowClose() {

    m_zoom_window.release();

    m_draw_zoom_win = m_enable_zoom_win;

    if (m_draw_zoom_win)
        m_image_label->setCursor(m_pencil_cursor);

    emit zoomWindowClosed();
}




ImageWindowToolbar::ImageWindowToolbar(QWidget* parent) : QWidget(parent) {

    this->setFixedWidth(m_width);
    this->setMinimumHeight(m_width * 1);
    this->resize(m_width, m_width);
    this->setAttribute(Qt::WA_NoMousePropagation);

    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(39, 39, 39));
    pal.setColor(QPalette::ButtonText, Qt::white);
    this->setPalette(pal);

    m_bg = new QButtonGroup(this);
    m_bg->setExclusive(false);

    m_bg->addButton(m_reset_pb = new FlatPushButton("R", this));
    m_reset_pb->setToolTip("Reset display");

    m_bg->addButton(m_hist_pb = new FlatPushButton("H", this));
    m_hist_pb->setToolTip("Histogram");

    m_bg->addButton(m_stats_pb = new FlatPushButton("S", this));
    m_stats_pb->setToolTip("Statistics");

    m_bg->addButton(m_img3D_pb = new FlatPushButton("3D", this));
    m_img3D_pb->setToolTip("3D Image");

    m_bg->addButton(m_stf_pb = new FlatCheckablePushButton("STF", this));
    m_stf_pb->setToolTip("Screen Transfer Function");
    m_stf_pb->setFont(QFont("Segoe UI", 8));

    m_bg->addButton(m_zoom_win_pb = new FlatCheckablePushButton("Z", this));
    m_zoom_win_pb->setToolTip("Zoom Window");

    m_bg->addButton(m_psf_pb = new FlatPushButton("PSF", this));
    m_psf_pb->setToolTip("Point-Spread-Function");

    for (auto pb : m_bg->buttons()) {
        pb->resize(m_width, m_width);
        pb->setPalette(pal);
        pb->move(0, height() + (m_bg->id(pb) + 1) * m_width);
    }

    this->show();
}

void ImageWindowToolbar::resizeEvent(QResizeEvent* e) {

    QWidget::resizeEvent(e);

    for (auto pb : m_bg->buttons())
        pb->move(0, height() + (m_bg->id(pb) + 1) * m_width);
}

void ImageWindowToolbar::connectToolbar(const QObject* receiver, void (IWB::* reset)(), void (IWB::* histogram)(),
    void (IWB::* stats)(), void (IWB::* _3dimg)(), void (IWB::* stf)(bool),
    void (IWB::* zoom_win)(bool), void (IWB::* psf)()) {

    connect(m_reset_pb, &QPushButton::released, receiver, reset);
    connect(m_hist_pb, &QPushButton::released, receiver, histogram);
    connect(m_stats_pb, &QPushButton::released, receiver, stats);
    connect(m_img3D_pb, &QPushButton::released, receiver, _3dimg);
    connect(m_stf_pb, &QPushButton::toggled, receiver, stf);
    connect(m_zoom_win_pb, &QPushButton::toggled, receiver, zoom_win);
    connect(m_psf_pb, &PushButton::released, receiver, psf);
}





template<typename T>
ImageWindow<T>::ImageWindow(Image<T>&& img, QString name, Workspace* parent) : m_source(std::move(img)), ImageWindowBase(name, parent) {

    installEventFilter(this);

    int f = computeBinFactor(QSize(m_source.cols(),m_source.rows()),true);
    m_factor_poll = (f == 1) ? f : -f;
    m_old_factor = m_factor = 1.0 / abs(factorPoll());

    this->setWindowTitle("1:" + QString(std::to_string(abs(factorPoll())).c_str()) + " " + m_name);

    m_drows = m_source.rows() * factor();
    m_dcols = m_source.cols() * factor();
    m_dchannels = m_source.channels();
    m_display = QImage(cols(), rows(), qimageFormat(channels()));

    auto actionX = [this](int action) {
        m_offset.x = m_sbX->sliderPosition() / factor();
        displayImage();
        if (m_zoom_window)
            m_zoom_window->moveBy(0, 0);
    };
    connect(m_sbX, &QScrollBar::actionTriggered, this, actionX);

    auto actionY = [this](int action) {
        m_offset.y = m_sbY->sliderPosition() / factor();
        displayImage();
        if (m_zoom_window)
            m_zoom_window->moveBy(0, 0);
    };
    connect(m_sbY, &QScrollBar::actionTriggered, this, actionY);

    m_sw_parent->resizeToFit(cols() + m_toolbar->width(), rows());

    emit windowCreated();

    displayImage();
    m_sw_parent->show();
}

template<typename T>
void ImageWindow<T>::setMask(const ImageWindow<uint8_t>* mask) {

    if (mask == nullptr)
        return;

    if (mask->channels() > channels())
        return;

    if (!m_source.isSameSize(mask->source()))
        return;

    m_mask = mask;
    connect(m_mask, &ImageWindowBase::windowClosed, this, &ImageWindow<T>::removeMask);
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
void ImageWindow<T>::showPreview(PreviewWindow<T>* preview) {

    if (m_preview == nullptr) {
        if (preview == nullptr)
            m_preview = std::make_unique<PreviewWindow<T>>(this, false);
        else
            m_preview = std::unique_ptr<PreviewWindow<T>>(preview);

        connect(m_preview.get(), &PreviewWindowBase::windowClosed, this, [this]() { m_preview.release(); });
    }
}

template<typename T>
void ImageWindow<T>::convertToGrayscale() {

    if (channels() != 3)
        return;

    m_source.toGrayscale();
    m_dchannels = m_source.channels();

    m_display = QImage(cols(), rows(), QImage::Format::Format_Grayscale8);

    m_compute_stf = true;

    //m_workspace->setActiveSubWindow(m_sw_parent);
    emit m_workspace->imageActivated(reinterpret_cast<Image8*>(&m_source));
    displayImage();
    updateStatisticsDialog();
    updateHistogramDialog();
    //emit im
}

template<typename T>
bool ImageWindow<T>::eventFilter(QObject* object, QEvent* event) {

    if (object == m_image_label) {
        if (event->type() == QEvent::HoverMove) {
            auto p = reinterpret_cast<QHoverEvent*>(event)->posF();
            p = (p / factor()) + m_offset.toQPointF();
            emit pixelValue(reinterpret_cast<const Image8*>(&m_source), p);
        }
        if (event->type() == QEvent::Leave)
            emit pixelValue(nullptr, { 0,0 });
    }

    return false;
}


template<typename T>
void ImageWindow<T>::mousePressEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) {
        QPoint p = m_image_label->mapFrom(this, event->pos());
        m_mousePos = p;
        if (m_image_label->rect().contains(p)) {
            if (m_psf_vector) {
                QPoint pt = p / factor();
                for (const PSF& psf : *m_psf_vector) {
                    if (psf.xc - psf.fwhmx - m_offset.x < pt.x()) {
                        if (pt.x() < psf.xc + psf.fwhmx - m_offset.x) {
                            if (psf.yc - psf.fwhmy - m_offset.y < pt.y()) {
                                if (pt.y() < psf.yc + psf.fwhmy - m_offset.y) {
                                    emit psfSelected(m_selected_psf = &psf);
                                    m_image_label->update();
                                }
                            }
                        }
                    }
                }
            }
        }

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

        emit zoomWindowCreated();
    }
}

template<typename T>
void ImageWindow<T>::resizeEvent(QResizeEvent* event) {

    ImageWindowBase::resiseEvent(event);

    double dx = (width() - event->oldSize().width()) / factor();
    if (dx > 0) {
        if (cols() + dx >= (m_source.cols() - m_offset.x) * factor()) {
            m_offset.x -= dx;
            clipOffsetX();
        }
        else
            m_dcols -= dx;
    }

    double dy = (height() - event->oldSize().height()) / factor();
    if (dy > 0) {
        if (rows() + dy >= (m_source.rows() - m_offset.y) * factor()) {
            m_offset.y -= dy;
            clipOffsetY();
        }
        else
            m_drows -= dy;
    }

    showScrollBars();
    resizeImageLabel();
    displayImage();
}

template<typename T>
void ImageWindow<T>::wheelEvent(QWheelEvent* event) {

    ImageWindowBase::wheelEvent(event);

    auto p = m_image_label->mapFrom(this, event->position());
    zoom(p.x(), p.y());

    if (m_zoom_window)
        m_zoom_window->scaleWindow();

    p = m_image_label->mapFrom(this, event->position());
    p = (p / factor()) + m_offset.toQPointF();
    emit pixelValue(reinterpret_cast<const Image8*>(&m_source), p);
}

template<typename T>
void ImageWindow<T>::showScrollBars() {

    auto showHSB = [this]() {
        double page_step = (cols() / factor()) * oldFactor();
        m_sbX->setRange(0, m_source.cols() * factor() - page_step);
        m_sbX->setValue(m_offset.x * factor());
        m_sbX->setPageStep(page_step);
        m_sa->showHorizontalScrollBar();
    };

    auto showVSB = [this]() {
        double page_step = (rows() / factor()) * oldFactor();
        m_sbY->setRange(0, m_source.rows() * factor() - page_step);
        m_sbY->setValue(m_offset.y * factor());
        m_sbY->setPageStep(page_step);
        m_sa->showVerticalScrollBar();
    };

    int w = m_source.cols() * factor();
    int h = m_source.rows() * factor();

    if ((width() - m_toolbar->width()) < w)
        showHSB();
    else
        m_sa->hideHorizontalScrollBar();

    if (height() < h)
        showVSB();
    else
        m_sa->hideVerticalScrollBar();

    auto vps = m_sa->viewportSize();

    m_dcols = (w > vps.width()) ? vps.width() : w;
    m_drows = (h > vps.height()) ? vps.height() : h;

    if (m_sa->isVerticalScrollBarOn() && m_sa->isHorizontalScrollBarOff()) {
        if (vps.width() > cols()) {
            m_drows += m_sbY->thickness();
            showVSB();
            m_sa->hideHorizontalScrollBar();
        }
        else {
            m_drows = m_sa->height() - m_sbX->thickness();
            showHSB();
        }
    }

    else if (m_sa->isHorizontalScrollBarOn() && m_sa->isVerticalScrollBarOff()) {
        if (vps.height() > rows()) {
            m_dcols += m_sbX->thickness();
            showHSB();
            m_sa->hideVerticalScrollBar();
        }
        else {
            m_dcols = m_sa->width() - m_sbY->thickness();
            showVSB();
        }
    }

    else if (m_sa->isVerticalScrollBarOn() && m_sa->isHorizontalScrollBarOn()) {
        showHSB();
        showVSB();
    }

    else {
        m_sa->hideHorizontalScrollBar();
        m_sa->hideVerticalScrollBar();
    }


    (m_sa->isHorizontalScrollBarOn()) ? clipOffsetX() : m_offset.x = 0.0;
    (m_sa->isVerticalScrollBarOn()) ? clipOffsetY() : m_offset.y = 0.0;

    m_drows = math::clip(rows(), 0, int(factor() * (m_source.rows() - m_offset.y)));
    m_dcols = math::clip(cols(), 0, int(factor() * (m_source.cols() - m_offset.x)));

}


template<typename T>
void ImageWindow<T>::resizeImageLabel() {

    int dr = (m_sa->isHorizontalScrollBarOn()) ? m_sbX->thickness() : 0;
    int dc = (m_sa->isVerticalScrollBarOn()) ? m_sbY->thickness() : 0;

    int x = math::max<int>(0, (width() - m_toolbar->width() - int(cols() + dc)) / 2);
    int y = math::max<int>(0, (height() - int(rows() + dr)) / 2);

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
        upsampleToWindow(factor());
    else
        binToWindow(1 / factor());

    m_image_label->draw();
}

template<typename T>
void ImageWindow<T>::displayMask() {

    m_mask_display.fill(Qt::black);

    switch (m_mask->type()) {
    case ImageType::UBYTE: {
        auto iw8 = m_mask;
        if (m_factor > 1)
            return upsampleToWindow_Mask(iw8->source(), factor());
        else
            return binToWindow_Mask(iw8->source(), 1 / factor());
    }
    case ImageType::USHORT: {
        auto iw16 = imageRecast<uint16_t>(m_mask);
        if (m_factor > 1)
            return upsampleToWindow_Mask(iw16->source(), factor());
        else
            return binToWindow_Mask(iw16->source(), 1 / factor());
    }
    case ImageType::FLOAT: {
        auto iw32 = imageRecast<float>(m_mask);
        if (m_factor > 1)
            return upsampleToWindow_Mask(iw32->source(), factor());
        else
            return binToWindow_Mask(iw32->source(), 1 / factor());
    }
    }
}


template<typename T>
void ImageWindow<T>::zoom(int x, int y) {

    double x_old = x / oldFactor();
    double x_new = x / factor();
    double y_old = y / oldFactor();
    double y_new = y / factor();
    m_offset += {x_old - x_new, y_old - y_new};

    m_old_factor = factor();

    showScrollBars();
    resizeImageLabel();
    displayImage();
}

template<typename T>
void ImageWindow<T>::pan(int x, int y) {

    QPoint pos;
    if (m_sbX->isVisible() || m_sbY->isVisible()) {
        m_offset -= (PointD(x - m_mousePos.x, y - m_mousePos.y) / factor());
        m_mousePos = {x,y};

        clipOffsetX();
        clipOffsetY();
        
        m_sbX->setValue(m_offset.x * factor());
        m_sbY->setValue(m_offset.y * factor());

        displayImage();
    }
}


template<typename T>
void ImageWindow<T>::binToWindow(int factor) {

    if (m_dchannels == 3)
        return binToWindow_RGB(factor);

    int factor2 = factor * factor;

    for (int ch = 0; ch < channels(); ++ch) {
        for (int y = 0, y_s = m_offset.y; y < rows(); ++y, y_s += factor) {
            for (int x = 0, x_s = m_offset.x; x < cols(); ++x, x_s += factor) {

                float pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += m_source(x_s + i, y_s + j, ch);

                pix /= factor2;

                if (m_enable_stf) {
                    pix = Pixel<float>::toType(T(pix));
                    pix = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, pix));
                }

                m_display.scanLine(y)[channels() * x + ch] = Pixel<uint8_t>::toType(T(pix));
            }       
        }
    }
}

template<typename T>
void ImageWindow<T>::binToWindow_RGB(int factor) {

    int factor2 = factor * factor;

    for (int y = 0, y_s = m_offset.y; y < rows(); ++y, y_s += factor) {
        uint8_t* p = m_display.scanLine(y);
        for (int x = 0, x_s = m_offset.x; x < cols(); ++x, x_s += factor, p += 3) {

            float r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    auto color = m_source.color(x_s + i, y_s + j);
                    r += color.red;
                    g += color.green;
                    b += color.blue;
                }
            }

            r /= factor2;
            g /= factor2;
            b /= factor2;

            if (m_enable_stf) {
                ColorF c = ColorConversion::toColorF(Color<T>(r, g, b));
                r = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, c.red));
                g = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, c.green));
                b = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, c.blue));
            }

            *p = Pixel<uint8_t>::toType(T(r));
            *(p + 1) = Pixel<uint8_t>::toType(T(g));
            *(p + 2)= Pixel<uint8_t>::toType(T(b));
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

    for (int y = 0, y_s = m_offset.y; y < rows(); ++y, y_s += factor) {
        for (int x = 0, x_s = m_offset.x; x < cols(); ++x, x_s += factor) {

            double r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    auto color = m_source.color<double>(x_s + i, y_s + j);
                    r += color.red;
                    g += color.green;
                    b += color.blue;
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

    if (m_dchannels == 3)
        return upsampleToWindow_RGB(factor);

    double dd = 1.0 / factor;

    for (int ch = 0; ch < channels(); ++ch) {
        double y_s = m_offset.y;
        for (int y = 0; y < rows(); ++y, y_s += dd) {
            double x_s = m_offset.x;
            for (int x = 0; x < cols(); ++x, x_s += dd) {
                T pix = m_source(x_s, y_s, ch);

                if (m_enable_stf)
                    pix = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(pix)));

                m_display.scanLine(y)[channels() * x + ch] = Pixel<uint8_t>::toType(pix);
            }
        }
    }
}

template<typename T>
void ImageWindow<T>::upsampleToWindow_RGB(int factor) {

    double dd = 1.0 / factor;    

    double y_s = m_offset.y;
    for (int y = 0; y < rows(); ++y, y_s += dd) {
        double x_s = m_offset.x;
        uint8_t* p = m_display.scanLine(y);
        for (int x = 0; x < cols(); ++x, x_s += dd, p += 3) {

            auto color = m_source.color<T>(x_s, y_s);

            if (m_enable_stf) {
                color.red = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(color.red)));
                color.green = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(color.green)));                //b = Pixel<float>::toType(T(b));
                color.blue = Pixel<T>::toType(m_ht.transformPixel(ColorComponent::rgb_k, Pixel<float>::toType(color.blue)));
            }

            *p = Pixel<uint8_t>::toType(color.red);
            *(p + 1) = Pixel<uint8_t>::toType(color.green);
            *(p + 2) = Pixel<uint8_t>::toType(color.blue);
        }
    }
}


template<typename T>
void ImageWindow<T>::resetWindowSize() {

    int factor = computeBinFactor(QSize(m_source.cols(), m_source.rows()), true);
    m_factor_poll = (factor == 1) ? factor : -factor;
    m_old_factor = m_factor = 1.0 / abs(m_factor_poll);
    m_drows = m_source.rows() * m_factor;
    m_dcols = m_source.cols() * m_factor;

    m_offset = { 0,0 };
    m_sw_parent->resizeToFit(cols() + m_toolbar->width(), rows());

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
void ImageWindow<T>::openHistogramDialog() {

    if (m_hist_dialog == nullptr) {
        m_hist_dialog = std::make_unique<HistogramDialog>(name(), m_source, m_workspace);
        connect(m_hist_dialog.get(), &HistogramDialog::windowClosed, this, [this]() { m_hist_dialog.reset(); });
    }
}

template<typename T>
void ImageWindow<T>::updateHistogramDialog() {

    if (m_hist_dialog != nullptr)
        m_hist_dialog->histogramView()->drawHistogramView(m_source, Histogram::Resolution::_16bit);
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
void ImageWindow<T>::openImage3DDialog() {

    if (m_img3D_dialog == nullptr) {
        m_img3D_dialog = std::make_unique<Image3DDialog>(name(), m_source, m_workspace);
        connect(m_img3D_dialog.get(), &Image3DDialog::windowClosed, this, [this]() { m_img3D_dialog.reset(); });
    }
}

template<typename T>
void ImageWindow<T>::enableSTF(bool enable) {

    m_enable_stf = enable;

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
void ImageWindow<T>::openPSFDialog() {

    if (m_psf_dialog == nullptr) {
        m_psf_dialog = std::make_unique<PSFUtilityDialog>(m_source, m_name, m_workspace);
        connect(this, &ImageWindowBase::psfSelected, m_psf_dialog.get(), &PSFUtilityDialog::selectPSF);
        connect(m_psf_dialog.get(), &PSFUtilityDialog::psfSelected, this, [this](const PSF* psf) { m_selected_psf = psf; m_image_label->update(); });
        connect(m_psf_dialog.get(), &PSFUtilityDialog::onDetection, this, [this](PSFVector* psf_vector) {
            m_psf_vector = psf_vector;
            m_image_label->update();
            });
        connect(m_psf_dialog.get(), &PSFUtilityDialog::psfCleared, this, [this]() {
            m_psf_vector = nullptr;
            m_selected_psf = nullptr;
            m_image_label->update();
            });
        connect(m_psf_dialog.get(), &PSFUtilityDialog::windowClosed, this, [this]() { m_psf_dialog.reset(); });
    }
}

template class ImageWindow<uint8_t>;
template class ImageWindow<uint16_t>;
template class ImageWindow<float>;





void ZoomWindow::keepFrameInImage_draw(QRect& rect)const {

    double f = m_iw->factor();
    auto p = m_iw->sourceOffset();

    if (p.x * f < -rect.left())
        rect.setLeft(-p.x * f);

    if ((m_iw->source().cols() - p.x) * f <= rect.right())
        rect.setRight((m_iw->source().cols() - p.x) * f - 1);

    if (p.y * f < -rect.top())
        rect.setTop(-p.y * f);

    if ((m_iw->source().rows() - p.y) * f <= rect.bottom())
        rect.setBottom((m_iw->source().rows() - p.y) * f - 1);
}

void ZoomWindow::keepFrameInImage_move(QRect& rect)const {

    double f = m_iw->factor();
    auto p = m_iw->sourceOffset();

    if (p.x * f < -rect.left())
        rect.moveLeft(-p.x * f);

    if ((m_iw->source().cols() - p.x) * f <= rect.right())
        rect.moveLeft((m_iw->source().cols() - p.x) * f - rect.width());
    
    if (p.y * f < -rect.top())
        rect.moveTop(-p.y * f);
    
    if ((m_iw->source().rows() - p.y) * f <= rect.bottom())
        rect.moveTop((m_iw->source().rows() - p.y) * f - rect.height());
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
    computeImageRect();
    update();
}

void ZoomWindow::scaleWindow() {

    auto p = m_iw->sourceOffset();
    int x = (m_img_rect.x() - p.x) * m_iw->factor();
    int y = (m_img_rect.y() - p.y) * m_iw->factor();

    int w = m_img_rect.width() * m_iw->factor();
    int h = m_img_rect.height() * m_iw->factor();

    this->setGeometry(x, y, w, h);
    update();
}

void ZoomWindow::moveBy(int dx, int dy) {

    auto p = m_iw->sourceOffset();

    int x = (m_img_rect.x() - p.x) * m_iw->factor() + dx;
    int y = (m_img_rect.y() - p.y) * m_iw->factor() + dx;

    this->move(x,y);
}

void ZoomWindow::connectZoomWindow2(const ProcessDialog* receiver, void(ProcessDialog::* func)()) {
    disconnect(m_connection);
    m_connection = connect(this, &ZoomWindow::windowMoved, receiver, func);
}

void ZoomWindow::computeImageRect() {

    auto frame = this->geometry();
    double factor = m_iw->factor();
    auto off = m_iw->sourceOffset();
    float x = frame.x() / factor + off.x;
    float y = frame.y() / factor + off.y;
    float w = frame.width() / factor;
    float h = frame.height() / factor;
    m_img_rect = QRectF(x, y, w, h);
}

void ZoomWindow::defaultMethod() {

    if (!m_iw->previewExists() || m_iw->preview()->isZoomWidnowIgnored())
        return;

    switch (m_iw->source().type()) {
    case ImageType::UBYTE:
        return m_iw->preview()->updatePreview();

    case ImageType::USHORT:
        return imageRecast<uint16_t>(m_iw)->preview()->updatePreview();

    case ImageType::FLOAT: 
        return imageRecast<float>(m_iw)->preview()->updatePreview();
    }
}

void ZoomWindow::closeEvent(QCloseEvent* e) {
    emit windowClosed();
    QWidget::closeEvent(e);
}

void ZoomWindow::mousePressEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton) {

        if (m_frame.isOnFrame(e->pos())) {
            m_current_border = m_frame.rectBorder(e->pos());
            m_start_rect = geometry();
            m_resizing = true;
            this->setMinimumSize(m_pen_width * 3,  m_pen_width * 3);
        }
        else {
            m_moving = true;
            setCursor(Qt::ClosedHandCursor);
        }
        m_pos = e->pos();
    }
}

void ZoomWindow::mouseMoveEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton) {

        if (m_resizing) {
            QPoint dp = e->pos() - m_pos;
            QRect r = this->geometry();
            QRect pr = parentWidget()->rect();

            int left = math::min(r.left() + dp.x(), r.right() - minimumWidth());
            int top = math::min(r.top() + dp.y(), r.bottom() - minimumHeight());

            switch (m_current_border) {

            case RectBorder::TopEdge:
                r.setTop(top);
                break;

            case RectBorder::LeftEdge:;
                r.setLeft(left);
                break;

            case RectBorder::RightEdge:
                r.setRight(m_start_rect.right() + dp.x());
                break;

            case RectBorder::BottomEdge:
                r.setBottom(m_start_rect.bottom() + dp.y());
                break;

            case RectBorder::TopLeftCorner:
                r.setTopLeft({ left,top });
                break;

            case RectBorder::TopRightCorner:
                r.setTopRight(QPoint(m_start_rect.right() + dp.x(), top));
                break;

            case RectBorder::BottomLeftCorner:
                r.setBottomLeft(QPoint(left, m_start_rect.bottom() + dp.y()));
                break;

            case RectBorder::BottomRightCorner:
                r.setBottomRight(m_start_rect.bottomRight() + dp);
                break;
            }

            //keeps in parent
            r.setTop(math::max(r.y(), 0));
            r.setLeft(math::max(r.x(), 0));
            r.setRight(math::min(r.right(), pr.right()));
            r.setBottom(math::min(r.bottom(), pr.bottom()));

            this->setGeometry(r);
        }
        else if (m_moving) {
            QRect frame = this->geometry();
            frame.moveTo(mapToParent(e->pos()) - m_pos);
            keepFrameInImage_move(frame);
            this->move(frame.topLeft());
        }
        computeImageRect();
    }

    else {
        if (m_frame.isOnFrame(e->pos()))
            this->setCursor(m_frame.cursorAt(e->pos()));
        else
            this->unsetCursor();
    }
}

void ZoomWindow::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button() == Qt::LeftButton) {
        m_resizing = false;
        m_moving = false;
        setCursor(parentWidget()->cursor());
        emit windowMoved();
    }

    if (e->button() == Qt::RightButton && rect().contains(e->pos()))
        this->close();
}

void ZoomWindow::resizeEvent(QResizeEvent* e) {

    QWidget::resizeEvent(e);
    m_frame.resize(rect());
}

void ZoomWindow::paintEvent(QPaintEvent* e) {

    QPainter p(this);

    QPen pen(penColor());
    pen.setWidth(m_pen_width);
    p.setPen(pen);

    QRect r = this->rect();
    int t = m_pen_width / 2;

    r.adjust(t, t, -t - 1, -t - 1);
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





PreviewWindowBase::PreviewWindowBase(Workspace* workspace, bool ignore_zoom_window) : m_ingore_zoom_window(ignore_zoom_window), QWidget(workspace) {

    auto s = screen()->availableSize();
    s.setWidth(9 * s.width() / 16);
    s.setHeight(2 * s.height() / 3);
    this->resizeWindow(s.width(), s.height());

    if (workspace)
        this->move((workspace->width() - s.width())/2, (workspace->height() - s.height()) / 2);

    m_titlebar = new DialogTitleBar(this);
    m_titlebar->setGeometry(0, 0, width(), DialogTitleBar::titleBarHeight());

    auto shade = [this]() {
        m_pre_shade_size = size();
        m_min_size = minimumSize();
        QSize s = { math::min(400, width()),DialogTitleBar::titleBarHeight() };
        this->setMinimumSize(s);
        this->resize(s);
    };

    auto unshade = [this]() {
        this->setMinimumSize(m_min_size);
        this->resize(m_pre_shade_size);
    };

    connect(m_titlebar, &DialogTitleBar::shadeWindow, this, shade);
    connect(m_titlebar, &DialogTitleBar::unshadeWindow, this, unshade);

    m_draw_area = new QWidget(this);
    m_draw_area->setCursor(Qt::ArrowCursor);
    drawArea()->setGeometry(m_border_width, DialogTitleBar::titleBarHeight(), width() - m_border_width, height() - DialogTitleBar::titleBarHeight());

    m_image_label = new ImageLabel(m_display, drawArea());

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, std::bind(&PreviewWindowBase::setOpacity, this, 0.55));

    m_image_label->installEventFilter(this);

    auto ftb = reinterpret_cast<FastStack*>(workspace->parentWidget())->toolbar();
    connect(this, &PreviewWindowBase::pixelValue, ftb->pixelValueLabel(), &PixelValueLabel::displayPreviewText);
    //QMovie* movie = new QMovie("./Icons//progress_spinner.gif");
    //m_image_label->setMovie(movie);
    //movie->start();

    setAttribute(Qt::WA_DeleteOnClose);

    emit windowCreated();

    show();
}

void PreviewWindowBase::resizeWindow(int w, int h) {

    this->resize(w + 2 * m_border_width, h + DialogTitleBar::titleBarHeight() + m_border_width);
}

double PreviewWindowBase::computeScaleFactor(const QSize& n_size)const {

    if (n_size.isEmpty())
        return 1.0;

    auto s = drawArea()->size();
    double rx = double(s.width()) / n_size.width();
    double ry = double(s.height()) / n_size.height();
    return math::min(rx, ry);
}

void PreviewWindowBase::closeEvent(QCloseEvent* close) {
    emit windowClosed();
    close->accept();
}

void PreviewWindowBase::mousePressEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton && childAt(e->pos()) == m_titlebar) {
        m_timer->start(500);
        m_start_pos = e->pos();
        m_moving = true;
    }
}

void PreviewWindowBase::mouseMoveEvent(QMouseEvent* e) {

    if (e->buttons() == Qt::LeftButton && m_moving) {

        if (m_timer->id() != Qt::TimerId::Invalid) {
            m_timer->stop();
            this->setOpacity(0.55);
        }
        this->move(geometry().topLeft() + (e->pos() - m_start_pos));
    }
}

void PreviewWindowBase::mouseReleaseEvent(QMouseEvent* e) {

    if (e->button() == Qt::LeftButton) {

        if (m_timer->id() != Qt::TimerId::Invalid)
            m_timer->stop();

        if (m_moving)
            this->setOpacity(1.0);

        m_moving = false;
    }
}

void PreviewWindowBase::paintEvent(QPaintEvent* e) {

    QPainter p(this);
    p.setOpacity(m_opacity);

    QPen pen;

    pen.setColor(QColor(69, 69, 69));
    pen.setWidth(2 * m_border_width);
    p.setPen(pen);
    p.drawRect(this->rect());

    if (palette().currentColorGroup() == QPalette::Active)
        pen.setColor(QColor(69, 0, 128, 128));
    else
        pen.setColor(QColor(128, 128, 128));

    pen.setWidth(2);
    p.setPen(pen);
    p.drawRect(this->rect());
    p.fillRect(drawArea()->geometry(), palette().color(QPalette::Window));
}

void PreviewWindowBase::resizeEvent(QResizeEvent* e) {

    QWidget::resizeEvent(e);

    m_titlebar->resize(width());
    drawArea()->setGeometry(m_border_width, DialogTitleBar::titleBarHeight(), width() - 2 * m_border_width, height() - DialogTitleBar::titleBarHeight() - m_border_width);
    e->accept();
}





template<typename T>
PreviewWindow<T>::PreviewWindow(ImageWindow<T>* iw, bool ignore_zoomwindow)  : m_image_window(iw), PreviewWindowBase(iw->workspace(), ignore_zoomwindow){

    m_scale_factor = computeScaleFactor(QSize(iw->source().cols(), iw->source().rows()));
}

template<typename T>
void PreviewWindow<T>::resizeSource() {

    const Image<T>& src = m_image_window->source();

    QSize size = QSize(src.cols(), src.rows());
    m_zwtl = { 0,0 };

    if (!m_ingore_zoom_window && m_image_window->zoomWindow()) {
        QRectF r = m_image_window->zoomWindow()->imageRectF();
        size = r.size().toSize();
        m_zwtl = r.topLeft();
    }

    m_scale_factor = computeScaleFactor(size);

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
    const Image<T>& src = m_image_window->source();

    float _s = 1 / scaleFactor();

    if (_s > 1.0f) {
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
void PreviewWindow<T>::updatePreview_Mask() {

    if (!m_image_window->maskExists())
        return displayImage();

    resizeSource();
    Image<T> mod = Image<T>(m_source);
    updateSource();

    Image32 mask = [this]() -> Image32 {
        switch (m_image_window->mask()->type()) {

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

    bool invert = m_image_window->maskInverted();

    bool apply_stf = m_image_window->stfEnabled();
    auto ht = m_image_window->histogramTransformation();

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
void PreviewWindow<T>::updatePreview(bool from_parent) {

    if (m_image_window->maskEnabled() && m_image_window->maskExists())
        return updatePreview_Mask();

    if (from_parent)
        updateSource();

    displayImage();
}

template<typename T>
void PreviewWindow<T>::displayImage() {

    if (m_image_window->stfEnabled()) {

        auto ht = m_image_window->histogramTransformation();

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

template<typename T>
bool PreviewWindow<T>::eventFilter(QObject* obj, QEvent* e) {

    if (obj == m_image_label) {
        if (e->type() == QEvent::HoverMove) {
            auto src = reinterpret_cast<const Image8*>(&m_source);
            auto p = reinterpret_cast<QHoverEvent*>(e)->pos();
            auto ip = p.toPointF() * (1 / scaleFactor());
            if (!m_ingore_zoom_window && imageWindow()->zoomWindow())
                emit pixelValue(src, p, ip + m_zwtl);
            else
                emit pixelValue(src, p, ip);
        }
        if (e->type() == QEvent::Leave)
            emit pixelValue(nullptr, { 0,0 }, { 0,0 });
    }
    return false;
}

template class PreviewWindow<uint8_t>;
template class PreviewWindow<uint16_t>;
template class PreviewWindow<float>;
