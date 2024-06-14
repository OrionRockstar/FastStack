#include "pch.h"
#include "ImageWindow.h"
//#include "HistogramTransformation.h"
#include "FastStack.h"

//change parent to faststack???
template<typename T>
ImageWindow<T>::ImageWindow(Image<T>& img, QString name, QWidget* parent) : m_name(name), QWidget(parent) {

    m_workspace = parent;
    setAcceptDrops(true);
    installEventFilter(this);
    setMouseTracking(true);

    iws = new IWSS();
    img.MoveTo(m_source);

    //tb->move
    //IdealFactor();//= abs(m_factor_poll = -4);
    int factor = IdealZoomFactor(true);

    m_factor_poll = (factor == 1) ? factor : -factor;
    m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

    this->setWindowTitle("1:" + QString(std::to_string(abs(m_factor_poll)).c_str()) + " " + m_name);

    m_drows = m_winRows = m_source.Rows() * m_factor;
    m_dcols = m_winCols = m_source.Cols() * m_factor;
    m_dchannels = m_source.Channels();

    this->resize(m_winCols, m_winRows);

   // QToolBar* tb = new QToolBar(this);
    //tb->setOrientation(Qt::Vertical);
    //tb->setGeometry(m_dcols, 0, 20, 20);
    //tb->setAutoFillBackground(true);
    //tb->setBackgroundRole(QPalette::ColorRole::Dark);

    sa = new QScrollArea(this);
    sa->setGeometry(0, 0, m_winCols, m_winRows);
    sa->setBackgroundRole(QPalette::ColorRole::Dark);
    sa->setAutoFillBackground(true);

    QString corner = "QAbstractScrollArea::corner{background-color: light gray; }";    
    sa->setStyleSheet(corner);

    m_label = new QLabel(this);
    m_label->setGeometry(0, 0, m_dcols, m_drows);
    m_label->setMouseTracking(true);

    InstantiateScrollBars();
    QImage::Format format = QImage::Format::Format_RGB888;

    if (m_dchannels == 1)
        format = QImage::Format::Format_Grayscale8;

    m_display = QImage(m_dcols, m_drows, format);

    BinToWindow(0, 0, 1/m_factor);
    
    output.convertFromImage(m_display);
    m_label->setPixmap(output);

    QIcon icon;
    icon.addFile("C:\\Users\\Zack\\Desktop\\fast_stack_icon_fs2.png");
    this->setWindowIcon(icon);

    this->setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    auto ptr = reinterpret_cast<Workspace*>(parent);
    m_sw_parent = ptr->addSubWindow(this);//subwindow then becomes
    //need to take an account for extra window with when resizing if adding side toolbar
    m_sw_parent->setGeometry(ptr->m_offsetx, ptr->m_offsety, m_winCols + (2 * m_border_width), m_winRows + (m_titlebar_height + m_border_width));//orig 10,35
    ptr->UpdateOffsets();

    m_sw_parent->setWindowFlags(Qt::SubWindow | Qt::WindowShadeButtonHint);

    connect(iws, &IWSS::sendWindowOpen, ptr, &Workspace::receiveOpen);
    connect(iws, &IWSS::sendWindowClose, ptr, &Workspace::receiveClose);

    m_sw_parent->show();

}


template<typename T>
void ImageWindow<T>::sliderPressed_X() {
    m_scrollbarX = sbh->value();
}

template<typename T>
void ImageWindow<T>::sliderPressed_Y() {
    m_scrollbarY = sbv->value();
}

template<typename T>
void ImageWindow<T>::sliderMoved_X(int value) {
    
    if (value == m_scrollbarX)
        return;

    Pan_SliderX(value);
}

template<typename T>
void ImageWindow<T>::sliderMoved_Y(int value) {

    if (value == m_scrollbarY)
        return;

    Pan_SliderY(value);
}

template<typename T>
void ImageWindow<T>::sliderArrowX(int action) {

    if (action == 1 || action == 2) {

        m_scrollbarX = sbh->value();

        //left
        if (action == 2) {
            if (m_scrollbarX == 0)
                return sbh->setSliderPosition(0);
            Pan_SliderX(m_scrollbarX - 1);
        }
        //right
        if (action == 1) {
            if (m_scrollbarX == sbh->maximum())
                return sbh->setSliderPosition(sbh->maximum());
            Pan_SliderX(m_scrollbarX + 1);
        }

        sbh->setSliderPosition(m_scrollbarX);
    }

    if (action == 3 || action == 4) {

        m_scrollbarX = sbh->value();

        int dx = sbh->sliderPosition() - m_scrollbarX;

        if (action == 4 && m_scrollbarX == 0)
            return sbh->setSliderPosition(0);

        if (action == 3 && m_scrollbarX == sbh->maximum())
            return sbh->setSliderPosition(sbh->maximum());

        Pan_SliderX(m_scrollbarX + dx);
    }
}

template<typename T>
void ImageWindow<T>::sliderArrowY(int action) {

    if (action == 1 || action == 2) {

        m_scrollbarY = sbv->value();

        //up
        if (action == 2) {
            if (m_scrollbarY == 0)
                return sbv->setSliderPosition(0);
            Pan_SliderY(m_scrollbarY - 1);
        }
        //down
        if (action == 1) {
            if (m_scrollbarY == sbv->maximum())
                return sbv->setSliderPosition(sbv->maximum());
            Pan_SliderY(m_scrollbarY + 1);
        }

        sbv->setSliderPosition(m_scrollbarY);
    }

    if (action == 3 || action == 4) {

        m_scrollbarY = sbv->value();
        int dy = sbv->sliderPosition() - m_scrollbarY;

        if (action == 4 && m_scrollbarY == 0)
            return sbv->setSliderPosition(0);

        if (action == 3 && m_scrollbarY == sbv->maximum())
            return sbv->setSliderPosition(sbv->maximum());

        Pan_SliderY(m_scrollbarY + dy);
    }
}

template<typename T>
void ImageWindow<T>::sliderWheelX(QWheelEvent* event) {
    int dy = event->angleDelta().y() / 120;
    dy = (dy == 1) ? dy : dy + 3;
    sliderArrowX(dy);
}

template<typename T>
void ImageWindow<T>::sliderWheelY(QWheelEvent * event) {
    int dy = event->angleDelta().y() / 120;
    dy = (dy == 1) ? dy + 1 : dy + 2;
    sliderArrowY(dy);
}



template<typename T>
void ImageWindow<T>::wheelEvent(QWheelEvent* event) {

    int dy = event->angleDelta().y() / 120;

    if (abs(dy) > 1)
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

    Zoom(event->scenePosition().x(), event->scenePosition().y());

    if (m_factor > 1)
        this->setWindowTitle(QString(std::to_string(abs(m_factor_poll)).c_str()) + ":1" + " " + m_name);
    else
        this->setWindowTitle("1:" + QString(std::to_string(abs(m_factor_poll)).c_str()) + " " + m_name);
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
        m_mouseX = event->x();
        m_mouseY = event->y();
    }
}

template<typename T>
void ImageWindow<T>::mouseMoveEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton) 
        Pan(event->x(), event->y());
    
    int x = (event->x() - m_label->x()) / m_factor + m_sourceOffX;
    int y = (event->y() - m_label->y()) / m_factor + m_sourceOffY;
}

template<typename T>
void ImageWindow<T>::closeEvent(QCloseEvent* close) {
    //sendWindowClose();
    iws->sendWindowClose();
    delete this;
    close->accept();
}

template<typename T>
void ImageWindow<T>::resizeEvent(QResizeEvent* event) {

    if (m_open == false) {

        m_open = true;

        iws->sendWindowOpen();

        return;
    }

    int new_cols = size().width();
    int new_rows = size().height();

    if (new_cols <= sbv->width() || new_rows <= sbh->height())
        return;
    
    sa->resize(new_cols, new_rows);

    //if window width greater than display image
    if (new_cols >= int((m_source.Cols() - m_sourceOffX) * m_factor) - 1) {
        double dx = (new_cols - m_winCols) / m_factor;

        if (dx > 0)
            m_sourceOffX -= dx;
        m_sourceOffX = Clip(m_sourceOffX, 0.0, double(m_source.Cols()));
        m_dcols = (m_source.Cols() - m_sourceOffX) * m_factor;

       HideHorizontalScrollBar();
    }

    else {
        m_dcols = new_cols;
        ShowHorizontalScrollBar();
    }

    if (new_rows >= int((m_source.Rows() - m_sourceOffY) * m_factor) - 1) {
        double dy = (new_rows - m_winRows) / m_factor;

        if (dy > 0)
            m_sourceOffY -= dy;

        m_sourceOffY = Clip(m_sourceOffY, 0.0, double(m_source.Rows()));
        m_drows = (m_source.Rows() - m_sourceOffY) * m_factor;

       HideVerticalScrollBar();
    }

    else {
        m_drows = new_rows;
        ShowVerticalScrollBar();
    }

    m_winCols = new_cols;
    m_winRows = new_rows;

    ResizeDisplay();
    DisplayImage();
}

template<typename T>
bool ImageWindow<T>::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::HideToParent)
        m_sw_parent->resize(300, m_titlebar_height);


    return false;
}



template<typename T>
void ImageWindow<T>::ResizeDisplay() {

    if (isVerticalScrollBarOn() && isHorizontalScrollBarOff()) {
        if (m_winCols > m_dcols + sbv->width()) {
            for (int i = 0; i < sbv->width(); ++i)
                if (m_drows < int((m_source.Rows() - m_sourceOffY) * m_factor))
                    m_drows++;

            HideHorizontalScrollBar();
        }

        else {
            m_drows = m_winRows - sbh->height();
            m_dcols = m_winCols - sbv->width();
            ShowHorizontalScrollBar();
        }

    }

    else if (isHorizontalScrollBarOn() && isVerticalScrollBarOff()) {

        if (m_winRows > m_drows + sbh->height()) {
            for (int i = 0; i < sbh->height(); ++i)
                if (m_dcols < int((m_source.Cols() - m_sourceOffX) * m_factor))
                    m_dcols++;
            HideVerticalScrollBar();
        }

        else {
            m_dcols = m_winCols - sbv->width();
            m_drows = m_winRows - sbh->height();
            ShowVerticalScrollBar();
        }

    }

    else if (isVerticalScrollBarOn() && isHorizontalScrollBarOn()) {

        m_dcols = m_winCols - sbv->width();
        m_drows = m_winRows - sbh->height();

        ShowVerticalScrollBar();
        ShowHorizontalScrollBar();
    }

    else {
        HideHorizontalScrollBar();
        HideVerticalScrollBar();
    }

    int dr = (isHorizontalScrollBarOn()) ? sbh->height() : 0;
    int dc = (isVerticalScrollBarOn()) ? sbv->width() : 0;

    m_label->setGeometry((m_winCols - (m_dcols + dc)) / 2, (m_winRows - (m_drows + dr)) / 2, m_dcols, m_drows);
    m_display = QImage(m_dcols, m_drows, m_display.format());
}

template<typename T>
int ImageWindow<T>::IdealZoomFactor(bool workspace) {

    QSize size = screen()->availableSize();

    if (workspace)
        size = m_workspace->size();

    int width = size.width();
    int height = size.height();

    int factor = 1;
    for (; factor < 10; ++factor) {

        int new_cols = m_source.Cols() / factor;
        int new_rows = m_source.Rows() / factor;

        if (new_cols < 0.75 * width && new_rows < 0.75 * height)
            break;
    }


    return factor;

    m_factor_poll = (factor == 1) ? factor : -factor;
    m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

    return factor;

}


template<typename T>
void ImageWindow<T>::DisplayImage() {

    if (m_factor > 1)
        UpsampleToWindow(m_sourceOffX, m_sourceOffY, m_factor);

    else
        BinToWindow(m_sourceOffX, m_sourceOffY, 1 / m_factor);

    output.convertFromImage(m_display);
    m_label->setPixmap(output);
}

template<typename T>
void ImageWindow<T>::ShowPreview() {

    if (m_preview == nullptr)
        m_preview = std::unique_ptr<QDialog>(new PreviewWindow<T>(this));

}

template<typename T>
void ImageWindow<T>::ClosePreview() {
    
    m_preview.reset(nullptr);
 }

template<typename T>
void ImageWindow<T>::InstantiateScrollBars() {
    int val = 25 * devicePixelRatio();

    sbh = new ScrollBar;
    sa->setHorizontalScrollBar(sbh);
    connect(sbh, &ScrollBar::sliderPressed, this, &ImageWindow::sliderPressed_X);
    connect(sbh, &ScrollBar::sliderMoved, this, &ImageWindow::sliderMoved_X);
    connect(sbh, &ScrollBar::actionTriggered, this, &ImageWindow::sliderArrowX);
    connect(sbh, &ScrollBar::wheelEvent, this, &ImageWindow::sliderWheelX);
    sbh->setFixedHeight(20);

    sbv = new ScrollBar;
    sa->setVerticalScrollBar(sbv);
    connect(sbv, &ScrollBar::sliderPressed, this, &ImageWindow::sliderPressed_Y);
    connect(sbv, &ScrollBar::sliderMoved, this, &ImageWindow::sliderMoved_Y);
    connect(sbv, &ScrollBar::actionTriggered, this, &ImageWindow::sliderArrowY);
    connect(sbv, &ScrollBar::wheelEvent, this, &ImageWindow::sliderWheelY);
    sbv->setFixedWidth(20);
}

template<typename T>
void ImageWindow<T>::ShowHorizontalScrollBar() {

    double page_step = (m_dcols / m_factor) * m_initial_factor;

    sbh->setRange(0, m_source.Cols() * m_initial_factor - page_step);
    sbh->setSliderPosition(m_sourceOffX * m_initial_factor);
    sbh->setPageStep(page_step);
    m_scrollbarX = sbh->value();

    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

template<typename T>
void ImageWindow<T>::HideHorizontalScrollBar() {
    //sbh->hide();
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::ShowVerticalScrollBar() {

    double page_step = (m_drows / m_factor) * m_initial_factor;

    sbv->setRange(0, m_source.Rows() * m_initial_factor - page_step);
    sbv->setSliderPosition(m_sourceOffY * m_initial_factor);
    sbv->setPageStep(page_step);
    m_scrollbarY = sbv->value();

    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

template<typename T>
void ImageWindow<T>::HideVerticalScrollBar() {
    //sbv->hide();
    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::ShowScrollBars() {

    if (m_winCols < m_dcols) {
        m_dcols = m_winCols;
        ShowHorizontalScrollBar();
    }
    else
        HideHorizontalScrollBar();

    if (m_winRows < m_drows) {
        m_drows = m_winRows;
        ShowVerticalScrollBar();
    }

    else
        HideVerticalScrollBar();

}

template<typename T>
bool ImageWindow<T>::isHorizontalScrollBarOn() {
    return (sa->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn);
}

template<typename T>
bool ImageWindow<T>::isVerticalScrollBarOn() {
    return (sa->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn);
}

template<typename T>
bool ImageWindow<T>::isHorizontalScrollBarOff() {
    return (sa->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
}

template<typename T>
bool ImageWindow<T>::isVerticalScrollBarOff() {
    return (sa->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff);
}



template<typename T>
void ImageWindow<T>::Zoom(int x, int y) {

    double x_old = x / m_old_factor;
    double x_new = x / m_factor;

    double y_old = y / m_old_factor;
    double y_new = y / m_factor;

    m_sourceOffX += (x_old - x_new);
    m_sourceOffY += (y_old - y_new);

    m_drows = m_source.Rows() * m_factor;
    m_dcols = m_source.Cols() * m_factor;

    ShowScrollBars();
    m_sourceOffX = Clip(m_sourceOffX, 0.0, double(m_source.Cols() - m_dcols / m_factor));
    m_sourceOffY = Clip(m_sourceOffY, 0.0, double(m_source.Rows() - m_drows / m_factor));
    ResizeDisplay();

    m_old_factor = m_factor;

    DisplayImage();

}

template<typename T>
void ImageWindow<T>::Pan(int x, int y) {

    if (sbh->isVisible() || sbv->isVisible()) {

        m_sourceOffX -= double(x - m_mouseX)/m_factor;
        m_sourceOffY -= double(y - m_mouseY)/m_factor;

        m_mouseX = x;
        m_mouseY = y;

        m_sourceOffX = Clip(m_sourceOffX, 0.0, m_source.Cols() - m_dcols / m_factor);
        m_sourceOffY = Clip(m_sourceOffY, 0.0, m_source.Rows() - m_drows / m_factor);
        
        sbh->setSliderPosition(m_sourceOffX * m_initial_factor);
        sbv->setSliderPosition(m_sourceOffY * m_initial_factor);

        DisplayImage();
    }

}

template<typename T>
void ImageWindow<T>::Pan_SliderX(int x) {

    if (x == sbh->minimum())
        m_sourceOffX = m_scrollbarX = sbh->minimum();

    else if (x == sbh->maximum()) {
        m_sourceOffX = m_source.Cols() - m_dcols / m_factor;
        m_scrollbarX = sbh->maximum();
    }

    else {
        m_sourceOffX -= (m_scrollbarX - x) / m_initial_factor;
        m_sourceOffX = Clip(m_sourceOffX, 0.0, m_source.Cols() - m_dcols / m_factor);
        m_scrollbarX = x;
    }

    DisplayImage();
}

template<typename T>
void ImageWindow<T>::Pan_SliderY(int y) {

    if (y == sbv->minimum())
        m_sourceOffY = m_scrollbarY = 0;

    else if (y == sbv->maximum()) {
        m_sourceOffY = m_source.Rows() - m_drows / m_factor;
        m_scrollbarY = sbv->maximum();
    }

    else {
        m_sourceOffY -= (m_scrollbarY - y) / m_initial_factor;
        m_sourceOffY = Clip(m_sourceOffY, 0.0, m_source.Rows() - m_drows / m_factor);
        m_scrollbarY = y;
    }

    DisplayImage();
}



template<typename T>
void ImageWindow<T>::BinToWindow_RGB(int x_start, int y_start, int factor) {

    int factor2 = factor * factor;

    for (int y = 0, y_s = y_start; y < m_drows; ++y, y_s += factor) {
        for (int x = 0, x_s = x_start; x < m_dcols; ++x, x_s += factor) {

            float r = 0, g = 0, b = 0;

            for (int j = 0; j < factor; ++j)
                for (int i = 0; i < factor; ++i) {
                    r += m_source(x_s + i, y_s + j, 0);
                    g += m_source(x_s + i, y_s + j, 1);
                    b += m_source(x_s + i, y_s + j, 2);
                }

            m_display.scanLine(y)[3 * x + 0] = Pixel<uint8_t>::toType(T(r / factor2));
            m_display.scanLine(y)[3 * x + 1] = Pixel<uint8_t>::toType(T(g / factor2));
            m_display.scanLine(y)[3 * x + 2] = Pixel<uint8_t>::toType(T(b / factor2));
            
        }
    }

}

template<typename T>
void ImageWindow<T>::BinToWindow(int x_start, int y_start, int factor) {

    if (m_dchannels == 3)
        return BinToWindow_RGB(x_start, y_start, factor);

    int factor2 = factor * factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        for (int y = 0, y_s = y_start; y < m_drows; ++y, y_s += factor) {
            for (int x = 0, x_s = x_start; x < m_dcols; ++x, x_s += factor) {

                double pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += m_source(x_s + i, y_s + j, ch);
                    
                m_display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(T(pix / factor2));
            }
        }
    }

}

template<typename T>
void ImageWindow<T>::BinToWindow_STF(int x_start, int y_start, int factor) {

    if (m_stf) {
        if (compute_stf) {
            //stf.ComputeSTFCurve(source);
            compute_stf = false;
        }
    }

    int factor2 = factor * factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        for (int y = 0, y_s = y_start; y < m_drows; ++y, y_s += factor) {
            for (int x = 0, x_s = x_start; x < m_dcols; ++x, x_s += factor) {

                double pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += m_source(x_s + i, y_s + j, ch);
                pix = pix / factor2;

                //if (m_stf)
                    //pix = Pixel<T>::toType(stf.RGB_K.TransformPixel(Pixel<float>::toType(T(pix))));

                m_display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(T(pix));
            }
        }
    }
    
}

template<typename T>
void ImageWindow<T>::UpsampleToWindow_RGB(double x_start, double y_start, int factor) {

    double dd = 1.0 / factor;
    double y_s = y_start;

    for (int y = 0; y < m_drows; ++y, y_s += dd) {
        double x_s = x_start;
        for (int x = 0; x < m_dcols; ++x, x_s += dd) {

            m_display.scanLine(y)[3 * x + 0] = Pixel<uint8_t>::toType(m_source(x_s, y_s, 0));
            m_display.scanLine(y)[3 * x + 1] = Pixel<uint8_t>::toType(m_source(x_s, y_s, 1));
            m_display.scanLine(y)[3 * x + 2] = Pixel<uint8_t>::toType(m_source(x_s, y_s, 2));

        }
    }
}

template<typename T>
void ImageWindow<T>::UpsampleToWindow(double x_start, double y_start, int factor) {

    if (m_dchannels == 3)
        return UpsampleToWindow_RGB(x_start, y_start, factor);

    double dd = 1.0 / factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        double y_s = y_start;
        for (int y = 0; y < m_drows; ++y, y_s += dd) {
            double x_s = x_start;
            for (int x = 0; x < m_dcols; ++x, x_s += dd) {

                m_display.scanLine(y)[m_dchannels * (x)+ch] = Pixel<uint8_t>::toType(m_source(x_s, y_s, ch));

            }
        }
    }
}


template class ImageWindow<uint8_t>;
template class ImageWindow<uint16_t>;
template class ImageWindow<float>;








template<typename T>
PreviewWindow<T>::PreviewWindow(QWidget* parent) : QDialog(parent) {
    installEventFilter(this);

    auto iw_parent = reinterpret_cast<ImageWindow<T>*>(parent);
    m_bin_factor = iw_parent->IdealZoomFactor();

    m_drows = iw_parent->Source().Rows() / m_bin_factor;
    m_dcols = iw_parent->Source().Cols() / m_bin_factor;
    m_dchannels = iw_parent->Source().Channels();


    m_source = Image<T>(m_drows, m_dcols, m_dchannels);

    label = new QLabel(this);
    label->setGeometry(0, 0, m_dcols, m_drows);

    iws = new IWSS();

    if (m_dchannels == 1)
        display = QImage(m_dcols, m_drows, QImage::Format::Format_Grayscale8);

    else if (m_dchannels == 3)
        display = QImage(m_dcols, m_drows, QImage::Format::Format_RGB888);

    connect(iws, &IWSS::sendWindowClose, iw_parent, &ImageWindow<T>::ClosePreview);

    UpdatefromParent();

    ImagetoQImage(m_source, display);

    output.convertFromImage(display);
    label->setPixmap(output);

    this->setWindowTitle(iw_parent->ImageName() + "Preview: ");

    setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    setAttribute(Qt::WA_DeleteOnClose);
    show();
}

template<typename T>
void PreviewWindow<T>::UpdatefromParent() {

    auto parent = reinterpret_cast<ImageWindow<T>*>(this->parentWidget());

    int factor = m_bin_factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        for (int y = factor / 2, y_s = 0; y < m_drows; ++y, y_s += factor) {
            for (int x = factor / 2, x_s = 0; x < m_dcols; ++x, x_s += factor) {

                m_source(x, y, ch) = parent->Source()(x_s, y_s, ch);
            }
        }
    }
}

template<typename T>
void PreviewWindow<T>::UpdatefromParent_QualityRGB() {

    ImageWindow<T>* parent = reinterpret_cast<ImageWindow<T>*>(this->parentWidget());

    int factor = m_bin_factor;
    int factor2 = factor * factor;

    for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
        for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

            double R = 0, G = 0, B = 0;

            for (int j = 0; j < factor; ++j) {
                for (int i = 0; i < factor; ++i) {
                    R += parent->Source()(x_s + i, y_s + j, 0);
                    G += parent->Source()(x_s + i, y_s + j, 1);
                    B += parent->Source()(x_s + i, y_s + j, 2);
                }
            }
            m_source(x, y, 0) = R / factor2;
            m_source(x, y, 1) = G / factor2;
            m_source(x, y, 2) = B / factor2;

        }
    }
    
}

template<typename T>
void PreviewWindow<T>::UpdatefromParent_Quality() {

    if (m_dchannels == 3)
        return UpdatefromParent_QualityRGB();

    auto parent = reinterpret_cast<ImageWindow<T>*>(this->parentWidget());

    int factor = m_bin_factor;
    int factor2 = factor * factor;

    for (int ch = 0; ch < m_dchannels; ++ch) {
        for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
            for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

                double pix = 0;

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        pix += parent->Source()(x_s + i, y_s + j, ch);

                m_source(x, y, ch) = pix / factor2;

            }
        }
    }
}

template<typename T>
void PreviewWindow<T>::UpdatefromSource_Quality(Image<T>& src) {

    int factor = m_bin_factor;
    int factor2 = factor * factor;

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
void PreviewWindow<T>::UpdatePreview(Image<T>& src) {

    m_drows = src.Rows() / m_bin_factor;
    m_dcols = src.Cols() / m_bin_factor;
    m_dchannels = src.Channels();

    if (m_drows != m_source.Rows() || m_dcols != m_source.Cols() || m_dchannels != m_source.Channels()) {
        m_source = Image<T>(m_drows, m_dcols, m_dchannels);

        auto format = QImage::Format::Format_RGB888;

        if (m_dchannels == 1)
            format = QImage::Format::Format_Grayscale8;

        display = QImage(m_dcols, m_drows, format);
    }

    UpdatefromSource_Quality(src);
    DisplayImage();
}

template<typename T>
void PreviewWindow<T>::DisplayImage() {

    ImagetoQImage(m_source, display);

    output.convertFromImage(display);
    label->setPixmap(output);
}

template class PreviewWindow<uint8_t>;
template class PreviewWindow<uint16_t>;
template class PreviewWindow<float>;
