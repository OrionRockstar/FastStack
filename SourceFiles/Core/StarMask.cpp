#include "pch.h"
#include "FastStack.h"
#include "StarMask.h"
#include "GaussianFilter.h"
#include "MorphologicalTransformation.h"

template<typename T>
void drawCircle(Image<T>& img, int xc, int yc, int radius) {

    int r2 = radius * radius;
    for (int x = 0, y = radius; x < y; ++x) {

        int offset = (x * x) - r2 + (y * y);
        //int offset = (x * x) - r2 + ((x == 0) ? (y - 1) * (y - 1) : (y * y));

        if (offset >= 0)
            y--;
        
        img.setPixel(Pixel<T>::max(), xc + x, yc - y);
        img.setPixel(Pixel<T>::max(), xc + y, yc - x);

        img.setPixel(Pixel<T>::max(), xc + y, yc + x);
        img.setPixel(Pixel<T>::max(), xc + x, yc + y);

        img.setPixel(Pixel<T>::max(), xc - x, yc + y);
        img.setPixel(Pixel<T>::max(), xc - y, yc + x);

        img.setPixel(Pixel<T>::max(), xc - y, yc - x);
        img.setPixel(Pixel<T>::max(), xc - x, yc - y);
    }
}

template<typename T>
void drawFilledCircle(Image<T>& img, int xc, int yc, int radius) {

    int r2 = radius * radius;
    for (int y = 0, x = radius, oldy = 0; y < x; ++y) {

        //int offset = (x * x) + (y * y) - r2;
        int offset = (y * y) - r2 + ((y == 0) ? (x - 1) * (x - 1) : x * x);

        if (offset >= 0)
            x--;

        img.setPixel(Pixel<T>::max() / 2, xc + x, yc - y);
        img.setPixel(Pixel<T>::max() / 2, xc + y, yc - x);

        img.setPixel(Pixel<T>::max() / 2, xc + y, yc + x);
        img.setPixel(Pixel<T>::max() / 2, xc + x, yc + y);

        img.setPixel(Pixel<T>::max() / 2, xc - x, yc + y);
        img.setPixel(Pixel<T>::max() / 2, xc - y, yc + x);

        img.setPixel(Pixel<T>::max() / 2, xc - y, yc - x);
        img.setPixel(Pixel<T>::max() / 2, xc - x, yc - y);


        for (int i = xc - x + 1; i < xc + x; ++i) {
            img.setPixel(Pixel<T>::max(), i, yc + y);
            img.setPixel(Pixel<T>::max(), i, yc - y);
        }

        if (oldy != yc + x) {
            for (int i = xc - y + 1; i < xc + y; ++i) {
                img.setPixel(Pixel<T>::max(), i, yc + x);
                img.setPixel(Pixel<T>::max(), i, yc - x);
            }
        }
        oldy = yc + x;
    }
}

template<typename T>
Image<T> StarMask::generateStarMask(const Image<T>& src) {

	StarVector sv = m_sd.DAOFIND(src);
    Image<T> mask(src.rows(), src.cols());

    for (const Star& star : sv) {
        float ct = cos(star.theta);
        float st = sin(star.theta);
        int rx2 = star.radius_x * star.radius_x;
        int ry2 = star.radius_y * star.radius_y;

        //mask(star.xc, star.yc) = Pixel<T>::max();
        for (int y = star.yc - star.radius_y; y <= int(star.yc + star.radius_y); ++y) {
            for (int x = star.xc - star.radius_x; x <= int(star.xc + star.radius_x); ++x) {
                if (mask.isInBounds(x, y)) {
                    double X = (x - star.xc) * ct + (y - star.yc) * st;
                    double Y = -(x - star.xc) * st + (y - star.yc) * ct;
                    if (((X * X) / rx2) + ((Y * Y) / ry2) <= 1.0)
                        mask(x, y) = (m_real_value) ? src(x, y) : Pixel<T>::max();
                }
            }
        }
    }

    GaussianFilter(m_stddev).apply(mask);

	return mask;
}
template Image8 StarMask::generateStarMask(const Image8&);
template Image16 StarMask::generateStarMask(const Image16&);
template Image32 StarMask::generateStarMask(const Image32&);
