#pragma once
#include <vector>
#include "Matrix.h"
#include "Maths.h"
#include "Image.h"

struct PSF {
    enum class Type : uint8_t {
        gaussian,
        moffat
    };

    Type type = Type::gaussian;
    float B = 0.0f;
    float A = 0.0f;
    float xc = 0.0f;
    float yc = 0.0f;
    float sx = 0.0f;
    float sy = 0.0f;
    float fwhmx = 0.0f;
    float fwhmy = 0.0f;
    float roundness = 1.0f;
    float flux = 0.0f;
    float theta = 0.0;

private:
    float beta = 10.0;

public:
    template<typename T>
    PSF(const std::array<double, 6>& c, const Image<T>& img, T bg, Type psf_type = Type::gaussian, float beta = 10.0f) : B(Pixel<float>::toType(bg)), type(psf_type), beta(beta) {

        auto d = 4 * c[4] * c[5] - c[3] * c[3];

        xc = (c[2] * c[3] - 2 * c[1] * c[5]) / d;//-c[1] / (2 * c[4]);
        yc = (c[1] * c[3] - 2 * c[2] * c[4]) / d;//-c[2] / (2 * c[5]);

        double _a = c[4];
        double _2b = c[3];
        double _c = c[5];

        theta = 0.5 * atan(_2b / (_a - _c));

        double ct = cos(theta);
        double st = sin(theta);

        switch (type) {
        case Type::gaussian: {
            sx = sqrt(1 / (2 * (_a * ct * ct + _2b * ct * st + _c * st * st)));
            sy = sqrt(1 / (2 * (_a * st * st - _2b * ct * st + _c * ct * ct)));
            fwhmx = 2.35482 * sx;
            fwhmy = 2.35482 * sy;
            A = exp((_a * (xc * xc) + _2b * (xc * yc) + _c * (yc * yc)) - c[0]);
            break;
        }

        case Type::moffat: {
            sx = sqrt(1 / (_a * ct * ct + _2b * ct * st + _c * st * st));
            sy = sqrt(1 / (_a * st * st - _2b * ct * st + _c * ct * ct));
            float b = 2 * sqrt(pow(2, 1 / beta) - 1);
            fwhmx = sx * b;
            fwhmy = sy * b;
            A = pow(1 + (_a * (xc * xc) + _2b * (xc * yc) + _c * (yc * yc)) - c[0], -1 / beta);
            break;
        }
        }

        int rx = fwhmx;
        int ry = fwhmy;

        if (!img.isInBounds(xc, yc))
            return;

        flux = 0.0;
        for (double y = yc - ry; y <= yc + ry; ++y) {
            for (double x = xc - rx; x <= xc + rx; ++x) {
                if (img.isInBounds(x, y))
                    if (img(x, y) > bg)
                        flux += Pixel<float>::toType(T(img(x, y) - bg));
            }
        }
        
        roundness = math::min(fwhmx, fwhmy) / math::max(fwhmx, fwhmy);
    }

    PSF() = default;

    float luminance()const { return -2.5 * log10(flux); }

    template<typename T>
    float rmse(const Image<T>& img)const {

        double ct = cos(theta);
        double st = sin(theta);
        double _sx = 2 * sx * sx;
        double _sy = 2 * sy * sy;

        double _a = ((ct * ct) / _sx) + ((st * st) / _sy);
        double _2b = 2 * ((-st * ct) / _sx) + ((st * ct) / _sy);
        double _c = ((st * st) / _sx) + ((ct * ct) / _sy);

        int ry = fwhmy;
        int rx = fwhmx;

        double sum = 0;
        int count = 0;
        T b = Pixel<T>::toType(B);

        auto v = [&, this](double dx, double dy) {
            switch (type) {
            case Type::gaussian:
                return A * expf(-((_a * dx * dx) + (_2b * dx * dy) + (_c * dy * dy)));
            case Type::moffat:
                return  A * powf(1 + (_a * (dx * dx) + _2b * (dx * dy) + _c * (dy * dy)), -1 / beta);
            default:
                return 0.0f;
            }
        };

        for (double y = yc - ry; y <= yc + ry; ++y) {
            double dy = y - yc;
            double dy2 = dy * dy;
            for (double x = xc - rx; x <= xc + rx; ++x) {
                double dx = x - xc;
                if (img.isInBounds(x, y)) {
                    if (img(x, y) > b) {
                        float pix = Pixel<float>::toType(T(img(x, y) - b)) - v(dx,dy);
                        sum += pix * pix;
                        count++;
                    }
                }
            }
        }

        return sqrt(sum / count);
    }

    float fwtmx()const {

        switch (type) {
        case Type::gaussian:
            return 4.29193 * sx;

        case Type::moffat:
            return sx * sqrtf(powf(10, 1 / beta) - 1);
        }
    }

    float fwtmy()const {

        switch (type) {
        case Type::gaussian:
            return 4.29193 * sy;

        case Type::moffat:
            return sy * sqrtf(powf(10, 1 / beta) - 1);
        }
    }
};
typedef std::vector<PSF> PSFVector;


struct Star {
    // star descriptor
    float xc = 0; // x-coord of center
    float yc = 0; // y-coord of center
    int radius_x = 0;
    int radius_y = 0;

    float luminance = 0; //appoximated absolute magnitude of star(luminance)

    Star(float x, float y, int r) :xc(x), yc(y), radius_x(r), radius_y(r) {};

    Star(float x, float y, int rx, int ry) :xc(x), yc(y), radius_x(rx), radius_y(ry) {};

    Star() = default;

    bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }

    int avgRadius()const { return (radius_x + radius_y) / 2; }
};

struct StarVector : public std::vector<Star> {
    void shrink_to_size(size_t newsize) {
        if (size() > newsize) {
            resize(newsize);
            shrink_to_fit();
        }
    }
};

struct StarPair {
    float rxc = 0; // reference
    float ryc = 0;
    float txc = 0; //target
    float tyc = 0;

    StarPair(float rxc, float ryc, float txc, float tyc) :rxc(rxc), ryc(ryc), txc(txc), tyc(tyc) {};
    StarPair() = default;
};

typedef std::vector<StarPair> StarPairVector;
