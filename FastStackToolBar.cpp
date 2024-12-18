#include "pch.h"
#include "FastStackToolBar.h"
#include "ImageWindow.h"

ImageInformationLabel::ImageInformationLabel(QWidget* parent) : QLabel(parent) {}

template<typename T>
static double memSize_MB(uint32_t count) {
    return count * sizeof(T) / 1048576.0;
}

void ImageInformationLabel::displayText(const QMdiSubWindow* window) {

    if (window == nullptr)
        return this->setText("");

    Image8& img = imageRecast(window->widget())->source();

    QString dot = tr(" \u00B7 ");
    QString txt="";
    txt += m_rowString + QString::number(img.rows()) + dot;
    txt += m_colString + QString::number(img.cols()) + dot;
    txt += m_channelString + QString::number(img.channels()) + dot;

    txt += m_bitdepthString;
    switch (img.type()) {
    case ImageType::UBYTE: {
        txt += "u8" + dot + QString::number(memSize_MB<uint8_t>(img.TotalPxCount()), 'f', 3);
        break;
    }
    case ImageType::USHORT: {
        txt += "u16" + dot + QString::number(memSize_MB<uint16_t>(img.TotalPxCount()), 'f', 3);
        break;
    }
    case ImageType::FLOAT: {
        txt += "32f" + dot + QString::number(memSize_MB<float>(img.TotalPxCount()), 'f', 3);
        break;
    }
    }

    txt += "MB   ";
    this->setText(txt);
}

PixelValueLabel::PixelValueLabel(QWidget* parent) : QLabel(parent) {}



void PixelValueLabel::displayText(QMdiSubWindow* window, const QPointF& p) {

    if (window == nullptr)
        return this->setText("");

    Image8* img = &imageRecast(window->widget())->source();

    if (!img->isInBounds(p.x(), p.y()))
        return this->setText("");

    QString txt = "";
    txt += m_xStr;
    txt += QString::number(p.x(),'f',2) + "   ";
    txt += m_yStr;
    txt += QString::number(p.y(), 'f', 2) + "      ";

    //Image8* img = &imageRecast(window->widget())->source();

    switch (img->type()) {
    case ImageType::UBYTE: {
        txt += rgb_k(img, p);
        break;
    }
    case ImageType::USHORT: {
        Image16* im16 = reinterpret_cast<Image16*>(img);
        txt += rgb_k(im16, p);
        break;
    }
    case ImageType::FLOAT: {
        Image32* im32 = reinterpret_cast<Image32*>(img);
        txt += rgb_k(im32, p);
        break;
    }
    }
    //if (img.channels() == 1)
        //txt += m_KStr + QString::number(img(p.x(), p.y()));
    
    this->setText(txt);
}

FastStackToolBar::FastStackToolBar(QWidget* parent) : QToolBar(parent) {

    QPalette p;
    p.setBrush(QPalette::Window, Qt::darkGray);
    setPalette(p);
    this->setFixedHeight(40);

    m_img_info = new ImageInformationLabel(this);
    this->addWidget(m_img_info);

    this->addSeparator();

    m_pix_val = new PixelValueLabel(this);
    this->addWidget(m_pix_val);
}