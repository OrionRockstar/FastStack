#pragma once
#include <vector>
#include "Matrix.h"
#include "Maths.h"

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

private:
    float beta = 10.0;

public:
    PSF(float A, float xc, float yc, float sx, float sy, float flux) : A(A), xc(xc), yc(yc), sx(sx), sy(sy), flux(flux) {
        fwhmx = 2.35482 * sx;
        fwhmy = 2.35482 * sy;
        roundness = math::min(fwhmx, fwhmy) / math::max(fwhmx, fwhmy);
    }

    PSF(Matrix c, float flux, float background, Type psf_type = Type::gaussian, float beta = 10.0f) : flux(flux), B(background), type(psf_type), beta(beta) {
        xc = -c[1] / (2 * c[3]);
        yc = -c[2] / (2 * c[4]);
        sx = 1 / sqrtf(2 * c[3]);
        sy = 1 / sqrtf(2 * c[4]);
        computeFWHM();
        roundness = math::min(fwhmx, fwhmy) / math::max(fwhmx, fwhmy);
        A = exp(((xc * xc) / (2.0 * sx * sx)) + ((yc * yc) / (2.0 * sy * sy)) - c[0]);
    }

    PSF() = default;

private:
    void computeFWHM() {

        switch (type) {

        case Type::gaussian: {
            fwhmx = 2.35482 * sx;
            fwhmy = 2.35482 * sy;
            return;
        }

        case Type::moffat: {
            float b = 2 * sqrtf(powf(2, 1 / beta) - 1);
            fwhmx = sx * b;
            fwhmy = sy * b;
            return;
        }
        }
    }
};

struct Star {
    // star descriptor
    float xc = 0; // x-coord of center
    float yc = 0; // y-coord of center
    int radius = 0;
    float luminance = 0; //appoximated absolute magnitude of star(luminance)
    Star(float x, float y, float r) :xc(x), yc(y), radius(r) {};
    Star() = default;
    ~Star() {};

    bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }
};

struct StarVector : public std::vector<Star> {
    void shrink_to_size(size_t newsize) {
        if (size() > newsize) {
            resize(newsize);
            shrink_to_fit();
        }
    }
};



struct StarPSF {
    // star descriptor
    PSF psf;

    int radius = 0;
    float luminance = 0; //appoximated absolute magnitude of star(luminance)

    StarPSF(const PSF& psf, const Star& star) : psf(psf), radius(star.radius), luminance(star.radius) {}
    StarPSF() = default;
    ~StarPSF() {}

    bool operator()(StarPSF& a, StarPSF& b) { return (a.luminance < b.luminance); }
};

struct StarPSFVector : public std::vector<StarPSF> {
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
