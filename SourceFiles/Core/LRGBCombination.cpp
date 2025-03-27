#include "pch.h"
#include "LRGBCombination.h"
#include "HistogramTransformation.h"

Status LRGBCombination::isImagesSameSize()const {
    Status status(false, "Incompatible Image Sizes!");

    if (!m_cc.isImagesSameSize())
        return status;
    
    QSize s = m_cc.outputSize();

    if (m_enable_lum && m_L) 
        if (m_L->rows() != s.height() || m_L->cols() != s.width())
            return status;
    
    return Status();
}

template<typename T>
void LRGBCombination::combineLuminance(Image32& rgb, const Image<T>& lum) {
    using HT = HistogramTransformation;

    double Lt_w = 1 - m_L_weight;
    //max value of lum midtone is 0.5
#pragma omp parallel for num_threads(2)
    for (int y = 0; y < rgb.rows(); ++y) {
        for (int x = 0; x < rgb.cols(); ++x) {
            auto color = rgb.color<double>(x, y);

            color.rRed() *= m_R_weight;
            color.rGreen() *= m_G_weight;
            color.rBlue() *= m_B_weight;

            double L, c, h;

            ColorSpace::RGBtoCIELch(color, L, c, h);
            L = HT::MTF(L * Lt_w + m_L_weight * lum(x,y), m_lightness_mtf);
            c = HT::MTF(c, m_saturation_mtf) * L + c * (1 - L);
            rgb.setColor(x, y, ColorSpace::CIELchtoRGB(L, c, h));
        }
    }
}

Image32 LRGBCombination::generateLRGBImage() {

    Image32 rgb = m_cc.generateRGBImage();

    double Lt_w = 1 - m_L_weight;

    if (m_L && m_enable_lum) {
        switch (m_L->type()) {
        case ImageType::UBYTE: {
            combineLuminance(rgb, *m_L);
            break;
        }
        case ImageType::USHORT: {
            combineLuminance(rgb, *reinterpret_cast<const Image16*>(m_L));
            break;
        }
        case ImageType::FLOAT: {
            combineLuminance(rgb, *reinterpret_cast<const Image32*>(m_L));
            break;
        }
        }
    }
    //max value of lum midtone is 0.5

    if (m_reduce_chrominance) {
        rgb.RGBtoCIELab();
        Image32 chrominance(rgb.rows(), rgb.cols(), 2);
        memcpy(chrominance.data(), &(*rgb.begin(1)), rgb.pxCount() * 2 * 4);
        //Wavelet().ChrominanceNoiseReduction(chrominance, m_layers_to_remove, m_layers_to_keep);
        memcpy(&rgb[rgb.pxCount()], chrominance.data(), rgb.pxCount() * 2 * 4);
        rgb.CIELabtoRGB();
    }

    return rgb;
}



