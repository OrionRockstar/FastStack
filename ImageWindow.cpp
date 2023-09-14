#include "pch.h"
#include "ImageWindow.h"
#include "HistogramTransformation.h"
#include "Interpolator.h"
#include "Maths.h"

ImageWindow::ImageWindow(Image32& img, QWidget *parent)
	: QMainWindow(parent)
{
	//ui.setupUi(this);
	ImageWindow::setWindowTitle("Image");
    int factor = abs(m_min_factor_poll);
    m_rows = img.Rows() / factor;
    m_cols = img.Cols() / factor;
    m_channels = img.Channels();

    resize(m_cols, m_rows);
    sa = new QScrollArea(this);
    sa->setGeometry(0, 0, m_cols, m_rows);
    
    label = new QLabel(sa);
    label->setGeometry(0, 0, m_cols, m_rows);
    label->setScaledContents(true);

    InstantiateScrollBars();
    
    if (m_channels == 1)
        display = QImage(m_cols, m_rows, QImage::Format::Format_Grayscale8);

    else if (m_channels == 3)
        display = QImage(m_cols, m_rows, QImage::Format::Format_RGB888);
    
    output = QPixmap(m_cols, m_rows);

    //only methods depending on image type
    img.MoveTo(source);
    HistogramTransformation().STFStretch(source);
    BinToWindowBilinear(0, 0, factor);
    
    output.convertFromImage(display);
    label->setPixmap(output);
    
	this->show();

}

ImageWindow::~ImageWindow()
{}

void ImageWindow::wheelEvent(QWheelEvent* event) {

    int dy = event->angleDelta().y() / 120;

    if (abs(dy) > 1)
        return;

    m_factor_poll += dy;

    //limits image zooming range
    if (m_factor_poll > m_max_factor_poll)
        m_factor_poll = m_max_factor_poll;

    else if (m_factor_poll < m_min_factor_poll)
        m_factor_poll = m_min_factor_poll;

    else {
        if (dy > 0)
            m_page_step *= 2;
        else if (dy < 0)
            m_page_step /= 2;
    }

    //allows smooth transition between change in zoom_factors
    if (m_factor_poll < -1)
        m_factor = 1.0 / -m_factor_poll;

    else if ((m_factor_poll == -1 || m_factor_poll == 0) && dy > 0) {
        m_factor = m_factor_poll = 1;
    }

    else if (m_factor_poll >= 1)
        m_factor = m_factor_poll;

    else if ((m_factor_poll == 1 || m_factor_poll == 0) && dy < 0) {
        m_factor = 0.5; m_factor_poll = -2;
    }

    //event->accept();

    Zoom(event->scenePosition().x(), event->scenePosition().y());

}

void ImageWindow::mousePressEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {
        mouse_offx = event->x();
        mouse_offy = event->y();
    }
}

void ImageWindow::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton) {
        Pan(event->x(), event->y());
    }
}

void ImageWindow::InstantiateScrollBars() {

    sbh = new QScrollBar(sa);
    sa->setHorizontalScrollBar(sbh);
    connect(sbh, &QScrollBar::sliderPressed, this, &ImageWindow::setSliderX);
    connect(sbh, &QScrollBar::sliderMoved, this, &ImageWindow::sliderPanX);

    sbv = new QScrollBar(sa);
    sa->setVerticalScrollBar(sbv);
    connect(sbv, &QScrollBar::sliderPressed, this, &ImageWindow::setSliderY);
    connect(sbv, &QScrollBar::sliderMoved, this, &ImageWindow::sliderPanY);
}

void ImageWindow::ShowScrollBars() {

    if (!sbh->isVisible())
        sbh->setVisible(true);

    sbh->setRange(0, (source.Cols() - int(m_cols / m_factor)) * m_initial_factor);
    sbh->setSliderPosition(offsetx * m_initial_factor);
    sbh->setPageStep(m_cols / m_page_step);
    sbh->setFixedHeight(20);

    if (!sbv->isVisible())
        sbv->setVisible(true);

    sbv->setRange(0, (source.Rows() - int(m_rows / m_factor)) * m_initial_factor);
    sbv->setSliderPosition(offsety * m_initial_factor);
    sbv->setPageStep(m_rows / m_page_step);
    sbv->setFixedWidth(20);

    if (label->width() != m_cols - 20 || label->height() != m_rows - 20)
        label->setGeometry(0, 0, m_cols - 20, m_rows - 20);
}

void ImageWindow::HideScrollBars() {

    sbh->setVisible(false);
    sbv->setVisible(false);

    label->setGeometry(0, 0, m_cols, m_rows);

}

void ImageWindow::BinToWindow(int x_start, int y_start, int factor) {

    int factor2 = factor * factor;

    for (int ch = 0; ch < m_channels; ++ch) {
        for (int y = 0, y_s = y_start; y < m_rows; ++y, y_s += factor) {
            for (int x = 0, x_s = x_start; x < m_cols; ++x, x_s += factor) {

                //for (int ch = 0; ch < m_channels; ++ch) {
                    float pix = 0;

                    for (int j = 0; j < factor; ++j)
                        for (int i = 0; i < factor; ++i)
                            pix += source(x_s + i, y_s + j, ch);

                    //display.setPixel(QPoint(x, y), qRgb(pix*255/factor2,0,0));
                 
                    display.scanLine(y)[m_channels * x + ch] = (pix * 255.0f) / factor2;
                //}
            }
        }
    }
}

void ImageWindow::BinToWindowBilinear(int x_start, int y_start, int factor) {

    if (m_channels == 3)
        return BinToWindowBilinearRGB(x_start, y_start, factor);

    Interpolator<Image8> interpolator8;
    Interpolator<Image32> interpolator;

    //for (int ch = 0; ch < m_channels; ++ch) {
        for (int y = 0, y_s = y_start; y < m_rows; ++y, y_s += factor) {
            for (int x = 0, x_s = x_start; x < m_cols; ++x, x_s += factor) {

                //display.setPixel(x, y, pix);
                if (source.Exists())
                    display.scanLine(y)[x] = interpolator.InterpolatePixel(source, x_s, y_s, 0, Interpolate::bilinear) * 255;

                else if(source8.Exists())
                    display.scanLine(y)[x] = interpolator8.InterpolatePixel(source8, x_s, y_s, 0, Interpolate::bilinear);
                //display.bits()[y * m_cols + x] = (pix * 255.0f) / factor2;
            }
        }
    //}
}

void ImageWindow::BinToWindowBilinearRGB(int x_start, int y_start, int factor) {

    Interpolator<Image32> interpolator;

    for (int y = 0, y_s = y_start; y < m_rows; ++y, y_s += factor) {
        for (int x = 0, x_s = x_start; x < m_cols; ++x, x_s += factor) {


            display.scanLine(y)[3 * x] = interpolator.InterpolatePixel(source, x_s, y_s, 0, Interpolate::bilinear) * 255;
            display.scanLine(y)[3 * x + 1] = interpolator.InterpolatePixel(source, x_s, y_s, 1, Interpolate::bilinear) * 255;
            display.scanLine(y)[3 * x + 2] = interpolator.InterpolatePixel(source, x_s, y_s, 2, Interpolate::bilinear) * 255;
        }
    }
    
}

void ImageWindow::UpsampleToWindow(int x_start, int y_start, int factor) {

    for (int ch = 0; ch < m_channels; ++ch) {
        for (int y = 0, y_s = y_start; y < m_rows; y += factor, ++y_s) {
            for (int x = 0, x_s = x_start; x < m_cols; x += factor, ++x_s) {

                for (int j = 0; j < factor; ++j)
                    for (int i = 0; i < factor; ++i)
                        if (IsInWindow(x + i, y + j))
                            display.scanLine(y + j)[m_channels * (x + i) + ch] = source(x_s, y_s, ch) * 255.0f;

            }
        }
    }

}

void ImageWindow::Zoom(int x, int y) {

    if (m_factor_poll >= m_min_factor_poll) {

        if (m_factor_poll == m_min_factor_poll) {
            offsetx = offsety = 0;
        }

        int x_old = x / m_old_factor + offsetx;
        int x_new = x / m_factor + offsetx;

        int y_old = y / m_old_factor + offsety;
        int y_new = y / m_factor + offsety;

        offsetx += (x_old - x_new);
        offsety += (y_old - y_new);

        offsetx = Clip(offsetx, 0, source.Cols() - int(m_cols / m_factor));
        offsety = Clip(offsety, 0, source.Rows() - int(m_rows / m_factor));


        if (m_factor > 1)
            UpsampleToWindow(offsetx, offsety, m_factor);

        else
            BinToWindowBilinear(offsetx, offsety, 1 / m_factor);


        m_old_factor = m_factor;

        if (m_factor_poll != m_min_factor_poll)
            ShowScrollBars();

        else 
            HideScrollBars();

        output.convertFromImage(display);
        label->setPixmap(output);
    }
}

int ImageWindow::DeltaMouseAdjustment(float dm) {

    dm /= m_factor;

    if (0 < dm && dm < 1)
        return 1;
    else if (0 > dm && dm > -1)
        return -1;
    else
        return dm;
}

void ImageWindow::Pan(int x, int y) {

    if (m_factor_poll != m_min_factor_poll) {

        offsetx -= (x - mouse_offx);//DeltaMouseAdjustment(x - mouse_offx);
        offsety -= (y - mouse_offy);// DeltaMouseAdjustment(y - mouse_offy);

        mouse_offx = x;
        mouse_offy = y;


        offsetx = Clip(offsetx, 0, source.Cols() - int(m_cols / m_factor) - 1);
        offsety = Clip(offsety, 0, source.Rows() - int(m_rows / m_factor) - 1);

        if (m_factor > 1)
            UpsampleToWindow(offsetx, offsety, m_factor);

        else
            BinToWindowBilinear(offsetx, offsety, 1 / m_factor);

        sbh->setSliderPosition(offsetx * m_initial_factor);
        sbv->setSliderPosition(offsety * m_initial_factor);

        output.convertFromImage(display);
        label->setPixmap(output);
    }

}

void ImageWindow::Pan_SliderX(int x) {

    if (m_factor_poll != m_min_factor_poll) {

        if (x == 0)
            offsetx = scrollbar_offx = 0;

        else {
            offsetx -= (x - scrollbar_offx);
            scrollbar_offx = x;
        }

        offsetx = Clip(offsetx, 0, source.Cols() - int(m_cols / m_factor) - 1);

        if (m_factor > 1)
            UpsampleToWindow(offsetx, offsety, m_factor);

        else
            BinToWindowBilinear(offsetx, offsety, 1 / m_factor);


        output.convertFromImage(display);
        label->setPixmap(output);
    }
}

void ImageWindow::Pan_SliderY(int y) {

    if (m_factor_poll != m_min_factor_poll) {

        if (y == 0)
            offsety = scrollbar_offy = 0;

        else {
            offsety -= (y - scrollbar_offy);
            scrollbar_offy = y;
        }

        offsety = Clip(offsety, 0, source.Rows() - int(m_rows / m_factor) - 1);

        if (m_factor > 1)
            UpsampleToWindow(offsetx, offsety, m_factor);

        else
            BinToWindowBilinear(offsetx, offsety, 1 / m_factor);


        output.convertFromImage(display);
        label->setPixmap(output);
    }
}