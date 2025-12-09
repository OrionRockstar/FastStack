#include "pch.h"
#include "ExponentialTransformation.h"
#include "RGBColorSpace.h"


using ET = ExponentialTransformation;

template<typename T>
void ET::apply(Image<T>& img) {

    Image<T> mask;
    if (sigma() != 0.0f) {
        img.copyTo(mask);
        m_gf.apply(mask);
    }

    Threads(2).run([&, this](uint32_t start, uint32_t end) {
        //auto id = std::this_thread::get_id();
        //std::hash<std::thread::id> hasher;

        for (int y = start; y < end; ++y) {
            for (int x = 0; x < img.cols(); ++x) {

                double L = 0;
                if (lightnessMask()) {
                    if (img.channels() == 1)
                        L = Pixel<float>::toType(img(x, y));
                    else
                        L = ColorSpace::CIEL(img.color<double>(x, y));
                }
                double L1 = 1 - L;

                for (int ch = 0; ch < img.channels(); ++ch) {

                    float pixel = Pixel<float>::toType(img(x, y, ch));

                    float pix = pixel;
                    if (mask.exists())
                        pix =  Pixel<float>::toType(mask(x, y, ch));

                    pix = pow(1 - pix, order());

                    switch (method()) {
                    case Method::power_inverted_pixels:
                        pix = pow(pixel, pix);
                        break;

                    case Method::screen_mask_invert:
                        pix = 1 - (1 - pixel) * pix;
                        break;
                    }

                    if (lightnessMask())
                        pix = pixel * L + pix * (1 - L);

                    img(x, y, ch) = Pixel<T>::toType(pix);
                }
            }
        }
        }, img.rows());
}
template void ET::apply(Image8&);
template void ET::apply(Image16&);
template void ET::apply(Image32&);