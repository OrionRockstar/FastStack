#include "pch.h"
#include "ImageWindow.h"
//#include "HistogramTransformation.h"
#include "FastStack.h"
#include "FITS.h"

//change parent to faststack???
template<typename T>
ImageWindow<T>::ImageWindow(Image<T>& img, QString name, QWidget* parent) : QWidget(parent) {

    m_parent = parent;

    iws = new IWSS();

    m_name = name;

    img.MoveTo(source);

    IdealFactor();//= abs(m_factor_poll = -4);

    this->setWindowTitle("1:" + QString(std::to_string(abs(m_factor_poll)).c_str()) + " " + m_name);

    m_drows = m_winRows = source.Rows() * m_factor;
    m_dcols = m_winCols = source.Cols() * m_factor;
    m_dchannels = source.Channels();

    sa = new QScrollArea(this);
    sa->setGeometry(0, 0, m_winCols, m_winRows);
    sa->setBackgroundRole(QPalette::ColorRole::Dark);
    sa->setAutoFillBackground(true);

    QString corner = "QAbstractScrollArea::corner{background-color: light gray; }";    
    sa->setStyleSheet(corner);


    label = new QLabel(this);
    label->setGeometry(0, 0, m_winCols, m_winRows);

    InstantiateScrollBars();

    if (m_dchannels == 1)
        display = QImage(m_dcols, m_drows, QImage::Format::Format_Grayscale8);

    else if (m_dchannels == 3)
        display = QImage(m_dcols, m_drows, QImage::Format::Format_RGB888);

    //HistogramTransformation().STFStretch(source);
    //RTP_ImageWindow<T>* r;// = new RTP_ImageWindow<T>(this);
    //rtp = new RTP_ImageWindow<T>(this);

    BinToWindow(0, 0, 1/m_factor);
    
    output.convertFromImage(display);
    label->setPixmap(output);

    reinterpret_cast<Workspace*>(m_parent)->addSubWindow(this);
    reinterpret_cast<Workspace*>(m_parent)->currentSubWindow()->show();

    //this->setFixedSize(m_winCols, m_winRows);
}


template<typename T>
void ImageWindow<T>::sliderPressedX() {
    m_scrollbarX = sbh->value();
}

template<typename T>
void ImageWindow<T>::sliderPressedY() {
    m_scrollbarY = sbv->value();
}

template<typename T>
void ImageWindow<T>::sliderPanX(int value) {
    
    if (value == m_scrollbarX)
        return;

    Pan_SliderX(value);
}

template<typename T>
void ImageWindow<T>::sliderPanY(int value) {

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
void ImageWindow<T>::mousePressEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {
        m_mouseX = event->x();
        m_mouseY = event->y();
    }
}

template<typename T>
void ImageWindow<T>::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {
        Pan(event->x(), event->y());
    }
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
        auto ptr = reinterpret_cast<Workspace*>(m_parent);

        //ptr->currentSubWindow()->setGeometry(ptr->m_offsetx, ptr->m_offsety, m_winCols + 15 / devicePixelRatio(), m_winRows + 45 / devicePixelRatio());//15, 45 for native window
        //ptr->currentSubWindow()
        ptr->currentSubWindow()->setGeometry(ptr->m_offsetx, ptr->m_offsety, m_winCols+10, m_winRows+35);
        //ptr->currentSubWindow()->setStyleSheet("QMdiSubWindow:Title {background: green;}");
        //ptr->currentSubWindow()->setStyleSheet("QMdiSubWindow::")
        ptr->UpdateOffsets();

        connect(iws, &IWSS::sendWindowOpen, ptr, &Workspace::receiveOpen);
        connect(iws, &IWSS::sendWindowClose, ptr, &Workspace::receiveClose);
        iws->sendWindowOpen();

        return;
    }

    int new_cols = size().width();
    int new_rows = size().height();

    if (new_cols <= sbv->width() || new_rows <= sbh->height())
        return;

    sa->resize(new_cols, new_rows);
    //if window width greater than display image
    if (new_cols >= int((source.Cols() - m_sourceOffX) * m_factor) - 1) {
        double dx = (new_cols - m_winCols) / m_factor;

        if (dx > 0)
            m_sourceOffX -= dx;
        m_sourceOffX = Clip(m_sourceOffX, 0.0, double(source.Cols()));
        m_dcols = (source.Cols() - m_sourceOffX) * m_factor;

        HideHorizontalScrollBar();
    }

    else {
        m_dcols = new_cols;
        ShowHorizontalScrollBar();
    }

    if (new_rows >= int((source.Rows() - m_sourceOffY) * m_factor) - 1) {
        double dy = (new_rows - m_winRows) / m_factor;

        if (dy > 0)
            m_sourceOffY -= dy;

        m_sourceOffY = Clip(m_sourceOffY, 0.0, double(source.Rows()));
        m_drows = (source.Rows() - m_sourceOffY) * m_factor;

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
void ImageWindow<T>::ResizeWindowtoNormal() {

    m_winCols = source.Cols() * m_initial_factor;
    m_winRows = source.Rows() * m_initial_factor;

    auto win = reinterpret_cast<QMdiArea*>(m_parent)->currentSubWindow();
    win->hide();
    win->resize(m_winCols + 12, m_winRows + 36);
    win->show();

}


template<typename T>
void ImageWindow<T>::ResizeDisplay() {

    //size().width() changes depending on scaling factor
    //std::cout << size().width() << " " << m_winCols << "\n";
    //std::cout << sbv->width();

    if (sbv->isVisible() && !sbh->isVisible()) {
        if (m_winCols > m_dcols + sbv->width()) {
            for (int i = 0; i < sbv->width(); ++i)
                if (m_drows < int((source.Rows() - m_sourceOffY) * m_factor))
                    m_drows++;

            HideHorizontalScrollBar();
        }

        else {
            m_drows = m_winRows - sbh->height();
            m_dcols = m_winCols - sbv->width();
            ShowHorizontalScrollBar();
        }

    }

    else if (sbh->isVisible() && !sbv->isVisible()) {

        if (m_winRows > m_drows + sbh->height()) {
            for (int i = 0; i < sbh->height(); ++i)
                if (m_dcols < int((source.Cols() - m_sourceOffX) * m_factor))
                    m_dcols++;
            HideVerticalScrollBar();
        }

        else {
            m_dcols = m_winCols - sbv->width();
            m_drows = m_winRows - sbh->height();
            ShowVerticalScrollBar();
        }

    }

    else if (sbv->isVisible() && sbh->isVisible()) {

        m_dcols = m_winCols - sbv->width();
        m_drows = m_winRows - sbh->height();

        ShowVerticalScrollBar();
        ShowHorizontalScrollBar();
    }

    else {
        HideHorizontalScrollBar();
        HideVerticalScrollBar();
    }

    int dr = (sbh->isVisible()) ? sbh->height() : 0;
    int dc = (sbv->isVisible()) ? sbv->width() : 0;

    label->setGeometry((m_winCols - (m_dcols + dc)) / 2, (m_winRows - (m_drows + dr)) / 2, m_dcols, m_drows);
    display = QImage(m_dcols, m_drows, display.format());
}

template<typename T>
void ImageWindow<T>::DisplayImage() {

    if (m_factor > 1)
        UpsampleToWindow(m_sourceOffX, m_sourceOffY, m_factor);

    else
        BinToWindow(m_sourceOffX, m_sourceOffY, 1 / m_factor);

    output.convertFromImage(display);
    label->setPixmap(output);
}

template<typename T>
void ImageWindow<T>::ShowRTP() {
    if (rtp == nullptr) {
        rtp = new RTP_ImageWindow<T>(this);
    }
    else {
        reinterpret_cast<RTP_ImageWindow<T>*>(rtp)->UpdatefromParent();
        reinterpret_cast<RTP_ImageWindow<T>*>(rtp)->DisplayImage();
    }
}

template<typename T>
void ImageWindow<T>::InstantiateScrollBars() {
    int val = 25 * devicePixelRatio();

    sbh = new ScrollBar;
    sa->setHorizontalScrollBar(sbh);
    connect(sbh, &ScrollBar::sliderPressed, this, &ImageWindow::sliderPressedX);
    connect(sbh, &ScrollBar::sliderMoved, this, &ImageWindow::sliderPanX);
    connect(sbh, &ScrollBar::actionTriggered, this, &ImageWindow::sliderArrowX);
    connect(sbh, &ScrollBar::wheelEvent, this, &ImageWindow::sliderWheelX);
    sbh->setFixedHeight(20);

    sbv = new ScrollBar;
    sa->setVerticalScrollBar(sbv);
    connect(sbv, &ScrollBar::sliderPressed, this, &ImageWindow::sliderPressedY);
    connect(sbv, &ScrollBar::sliderMoved, this, &ImageWindow::sliderPanY);
    connect(sbv, &ScrollBar::actionTriggered, this, &ImageWindow::sliderArrowY);
    connect(sbv, &ScrollBar::wheelEvent, this, &ImageWindow::sliderWheelY);
    sbv->setFixedWidth(20);
}

template<typename T>
void ImageWindow<T>::ShowHorizontalScrollBar() {

    int page_step = (m_dcols / m_factor) * m_initial_factor;

    sbh->setRange(0, source.Cols() * m_initial_factor - page_step + 0.5);
    sbh->setSliderPosition(m_sourceOffX * m_initial_factor);
    sbh->setPageStep(page_step);
    //sbh->setFixedHeight(20);//multiply height by scale factor
    m_scrollbarX = sbh->value();

    //sbh->show();
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
}

template<typename T>
void ImageWindow<T>::HideHorizontalScrollBar() {
    //sbh->hide();
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::ShowVerticalScrollBar() {

    int page_step = (m_drows / m_factor) * m_initial_factor;

    sbv->setRange(0, source.Rows() * m_initial_factor - page_step + 0.5);
    sbv->setSliderPosition(m_sourceOffY * m_initial_factor);
    sbv->setPageStep(page_step);
    //sbv->setFixedWidth(20);
    //sbv->setFixedHeight(m_drows);
    m_scrollbarY = sbv->value();

    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //sbv->show();
}

template<typename T>
void ImageWindow<T>::HideVerticalScrollBar() {
    //sbv->hide();
    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

template<typename T>
void ImageWindow<T>::ShowScrollBars() {

    if (m_winCols > source.Cols() * m_factor)
        HideHorizontalScrollBar();
    else
        ShowHorizontalScrollBar();

    if (m_winRows > source.Rows() * m_factor)
        HideVerticalScrollBar();
    else
        ShowVerticalScrollBar();  

}

template<typename T>
void ImageWindow<T>::Zoom(int x, int y) {

    double x_old = x / m_old_factor;
    double x_new = x / m_factor;

    double y_old = y / m_old_factor;
    double y_new = y / m_factor;

    m_sourceOffX += (x_old - x_new);
    m_sourceOffY += (y_old - y_new);

    m_drows = source.Rows() * m_factor;
    m_dcols = source.Cols() * m_factor;

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

    m_sourceOffX = Clip(m_sourceOffX, 0.0, double(source.Cols() - m_dcols / m_factor));
    m_sourceOffY = Clip(m_sourceOffY, 0.0, double(source.Rows() - m_drows / m_factor));

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

        m_sourceOffX = Clip(m_sourceOffX, 0.0, source.Cols() - m_dcols / m_factor);
        m_sourceOffY = Clip(m_sourceOffY, 0.0, source.Rows() - m_drows / m_factor);
        
        sbh->setSliderPosition(m_sourceOffX * m_initial_factor);
        sbv->setSliderPosition(m_sourceOffY * m_initial_factor);

        DisplayImage();
    }

}

template<typename T>
void ImageWindow<T>::Pan_SliderX(int x) {

    if (x == 0)
        m_sourceOffX = m_scrollbarX = 0;

    else {
        m_sourceOffX -= (m_scrollbarX - x) / m_initial_factor;
        m_scrollbarX = x;
    }

    m_sourceOffX = Clip(m_sourceOffX, 0.0, source.Cols() - m_dcols / m_factor);

    DisplayImage();
}

template<typename T>
void ImageWindow<T>::Pan_SliderY(int y) {

    if (y == 0)
        m_sourceOffY = m_scrollbarY = 0;

    else {
        m_sourceOffY -= (m_scrollbarY - y) / m_initial_factor;
        m_scrollbarY = y;
    }

    m_sourceOffY = Clip(m_sourceOffY, 0.0, source.Rows() - m_drows / m_factor);

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
                    r += source(x_s + i, y_s + j, 0);
                    g += source(x_s + i, y_s + j, 1);
                    b += source(x_s + i, y_s + j, 2);
                }

            display.scanLine(y)[3 * x + 0] = Pixel<uint8_t>::toType(T(r / factor2));
            display.scanLine(y)[3 * x + 1] = Pixel<uint8_t>::toType(T(g / factor2));
            display.scanLine(y)[3 * x + 2] = Pixel<uint8_t>::toType(T(b / factor2));
            
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
                        pix += source(x_s + i, y_s + j, ch);
                    
               display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(T(pix / factor2));
            }
        }
    }
}

template<typename T>
void ImageWindow<T>::BinToWindow2(int x_start, int y_start, int factor) {

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
                        pix += source(x_s + i, y_s + j, ch);
                pix = pix / factor2;

                //if (m_stf)
                    //pix = Pixel<T>::toType(stf.RGB_K.TransformPixel(Pixel<float>::toType(T(pix))));

                display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(T(pix));
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

            display.scanLine(y)[3 * x + 0] = Pixel<uint8_t>::toType(source(x_s, y_s, 0));
            display.scanLine(y)[3 * x + 1] = Pixel<uint8_t>::toType(source(x_s, y_s, 1));
            display.scanLine(y)[3 * x + 2] = Pixel<uint8_t>::toType(source(x_s, y_s, 2));

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

                display.scanLine(y)[m_dchannels * (x)+ch] = Pixel<uint8_t>::toType(source(x_s, y_s, ch));

            }
        }
    }
}


template class ImageWindow<uint8_t>;
template class ImageWindow<uint16_t>;
template class ImageWindow<float>;