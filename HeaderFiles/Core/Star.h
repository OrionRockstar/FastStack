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
    float magnitude = 0.0f;
    float theta = 0.0f; //radians
    float rmse = 0.0f; //root mean square error
    bool saturated = false;
    int rx = 0;
    int ry = 0;
    float peak = 0.0f;

private:
    float beta = 10.0f;
    int count = 0;


public:
    template<typename T>
    PSF(const std::array<double, 6>& c, const Image<T>& img, T bg, Type psf_type = Type::gaussian, float beta = 10.0f) : B(Pixel<float>::toType(bg)), type(psf_type), beta(beta) {

        auto d = 4 * c[4] * c[5] - c[3] * c[3];

        xc = (c[2] * c[3] - 2 * c[1] * c[5]) / d;
        yc = (c[1] * c[3] - 2 * c[2] * c[4]) / d;

        double _a = c[4];
        double _2b = c[3];
        double _c = c[5];

        theta = 0.5 * atan(_2b / (_a - _c));

        double ct = cos(theta);
        double st = sin(theta);

        //need to double check moffat
        switch (type) {
        case Type::gaussian: {
            sx = sqrt(1 / (2 * (_a * ct * ct + _2b * ct * st + _c * st * st)));
            sy = sqrt(1 / (2 * (_a * st * st - _2b * ct * st + _c * ct * ct)));
            fwhmx = 2.35482 * sx;
            fwhmy = 2.35482 * sy;
            rx = fwtmx() / 2;
            ry = fwtmy() / 2;
            A = exp((_a * (xc * xc) + _2b * (xc * yc) + _c * (yc * yc)) - c[0]);
            break;
        }

        case Type::moffat: {
            sx = sqrt(1 / (_a * ct * ct + _2b * ct * st + _c * st * st));
            sy = sqrt(1 / (_a * st * st - _2b * ct * st + _c * ct * ct));
            float b = 2 * sqrt(pow(2, 1 / beta) - 1);
            fwhmx = sx * b;
            fwhmy = sy * b;
            rx = fwhmx;
            ry = fwhmy;
            A = pow(1 + (_a * (xc * xc) + _2b * (xc * yc) + _c * (yc * yc)) - c[0], beta);
            break;
        }
        }

        if (!img.isInBounds(xc, yc)) {
            xc = yc = std::nanf("");
            return;
        }

        saturated = isSaturated(img);
        flux = 0.0;
        count = 0;

        //inline rmse
        /*for (int y = yc - ry; y <= int(yc + ry); ++y) {
            for (int x = xc - rx; x <= int(xc + rx); ++x) {
                if (img.isInBounds(x, y)) {
                    if (img(x, y) > bg) {
                        flux += Pixel<float>::toType(T(img(x, y) - bg));
                        count++;
                    }
                }
            }
        }*/

        double sum = 0;
        //int count = 0;
        auto v = [=, this](double dx, double dy) {
            switch (type) {
            case Type::gaussian:
                return A * expf(-((_a * dx * dx) + (_2b * dx * dy) + (_c * dy * dy))) + B;
            case Type::moffat:
                return  A * powf(1 + (_a * (dx * dx) + _2b * (dx * dy) + _c * (dy * dy)), -beta) + B;
            default:
                return 0.0f;
            }
        };

        for (int y = yc - ry; y <= int(yc + ry); ++y) {
            double dy = y - yc;
            for (int x = xc - rx; x <= int(xc + rx); ++x) {
                double dx = x - xc;
                if (img.isInBounds(x, y)) {
                    if (img(x, y) > bg) {
                        float pixel = Pixel<float>::toType(img(x, y));
                        flux += pixel - B;
                        float d = pixel - v(dx, dy);
                        sum += d * d;
                        count++;
                    }
                }
            }
        }

        magnitude = - 2.5 * log10(flux);
        peak = Pixel<float>::toType(img.at(xc, yc));
        roundness = math::min(fwhmx, fwhmy) / math::max(fwhmx, fwhmy);
        rmse = sqrt(sum / count);
        //rmse = RMSE(img);
    }

    PSF() = default;

    bool operator()(const PSF& a, const PSF& b) { return (a.A > b.A); }
private:
    //template<typename T>
    /*float RMSE(const Image<T>& img)const {

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
        auto v = [=, this](double dx, double dy) {
            switch (type) {
            case Type::gaussian:
                return A * expf(-((_a * dx * dx) + (_2b * dx * dy) + (_c * dy * dy))) + B;
            case Type::moffat:
                return  A * powf(1 + (_a * (dx * dx) + _2b * (dx * dy) + _c * (dy * dy)), -beta) + B;
            default:
                return 0.0f;
            }
        };

        T b = Pixel<T>::toType(B);
        for (int y = yc - ry; y <= int(yc + ry); ++y) {
            double dy = y - yc;
            //double dy2 = dy * dy;
            for (int x = xc - rx; x <= int(xc + rx); ++x) {
                double dx = x - xc;
                if (img.isInBounds(x, y)) {
                    if (img(x, y) > b) {
                        float pix = Pixel<float>::toType(img(x, y)) - v(dx,dy);
                        sum += pix * pix;
                        count++;
                    }
                }
            }
        }

        return sqrt(sum / count);
    }*/

    template<typename T>
    bool isSaturated(const Image<T>& img)const {

        //int rx = fwhmx / 2;
        //int ry = fwhmy / 2;
        T min_peak = 0.9 * Pixel<T>::max();
        int pix_count = 0;

        for (int y = yc - ry; y < int(yc + ry); ++y)
            for (int x = xc - rx; x < int(xc + rx); ++x)
                if (img.at(x, y) > min_peak)
                    pix_count++;
        
        return (pix_count > 5);
    }

public:
    bool isValid()const {

        if (std::isnan(sx) || std::isnan(sy))
            return false;

        if (std::isinf(A))
            return false;

        if (std::isnan(xc) || std::isnan(yc))
            return false;

        if (sx < 1.0f || sy < 1.0f)
            return false;

        if (count < 5)
            return false;

        return true;
    }

    float sharpness()const {
        float mean = ((flux + B * count) - peak) / (count - 1);
        return (peak - mean) / A;
    }

    float mean()const {
        return (flux + B * count) / (count - 1);
    }

    float peakMean()const {
        float mean = ((flux + B * count) - peak) / (count - 1);
        return peak / mean;
    }

    float meanFlux()const {
        return flux / (std::_Pi * fwhmx * fwhmy);
    }

    float fwtmx()const {

        switch (type) {
        case Type::gaussian:
            return 4.29193 * sx;

        case Type::moffat:
            return sx * sqrtf(powf(10, 1 / beta) - 1);
        }

        return 0.0f;
    }

    float fwtmy()const {

        switch (type) {
        case Type::gaussian:
            return 4.29193 * sy;

        case Type::moffat:
            return sy * sqrtf(powf(10, 1 / beta) - 1);
        }

        return 0.0f;
    }
};

typedef std::vector<PSF> PSFVector;

struct Star {
    // star descriptor
    float xc = 0; // x-coord of center
    float yc = 0; // y-coord of center

    int radius_x = 0;
    int radius_y = 0;
    float theta = 0; //radians

    float magnitude = 0; //appoximated absolute magnitude

    Star(const PSF& psf) : xc(psf.xc), yc(psf.yc), radius_x(psf.rx), radius_y(psf.ry), theta(psf.theta), magnitude(psf.magnitude) {
        //moffat needs fwhm
        //radius_x = psf.fwhmx;
        //radius_y = psf.fwhmy;
    }

    Star(float x, float y, int r) :xc(x), yc(y), radius_x(r), radius_y(r) {};

    Star() = default;

    bool operator()(const Star& a, const Star& b) { return (a.magnitude < b.magnitude); }

    float avgRadius()const { return (radius_x + radius_y) / 2.0f; }

    int maxRadius()const { return math::max(radius_x, radius_y); }

    int minRadius()const { return math::min(radius_x, radius_y); }

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
