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

    //x2/a2 + y2/b2 = 1
    for (auto star : sv) {
        int xc = star.xc;
        int yc = star.yc;
        int r = math::max(star.radius_x, star.radius_y);//star.avgRadius();
        
        //drawCircle(mask, xc, yc, r);
        for (int j = yc - r; j <= yc + r; ++j)
            for (int i = xc - r; i <= xc + r; ++i)
                if (mask.isInBounds(i, j) && math::distancef( star.xc, star.yc, i, j) <= r)
                    mask(i, j) = (m_real_value) ? src(i, j) : Pixel<T>::max();
    }

    GaussianFilter(m_stddev).apply(mask);

	return mask;
}
template Image8 StarMask::generateStarMask(const Image8&);
template Image16 StarMask::generateStarMask(const Image16&);
template Image32 StarMask::generateStarMask(const Image32&);
