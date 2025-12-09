#pragma once
#include"Image.h"
#include "Star.h"


class StarDetector {

    float m_fwhm = 2;
    float m_sigma = 0.42466 * m_fwhm; // constant * fwhm

    float m_K = 1.0;
    float m_roundness = 0.5;

    float m_beta = 10.0f;
    PSF::Type m_psf_type = PSF::Type::gaussian;

    PSF m_average_psf;
public:
    StarDetector() = default;

    float K()const { return m_K; }

    void setK(float K) { m_K = K; }

    float roundness()const { return m_roundness; }

    void setRoundness(float roundness) { m_roundness = roundness; }

    PSF::Type psf()const { return m_psf_type; }

    void setPSF(PSF::Type psf_type) { m_psf_type = psf_type; }

    float beta()const { return m_beta; }

    void setBeta(float beta) { m_beta = beta; }

    PSF meanPSF()const { return m_average_psf; }

private:
    template<typename T>
    bool gaussianFit(const Image<T>& orig, const Image<T>& convolved, const Star& star, PSF& psf, T b, T t);

    template<typename T>
    bool moffatFit(const Image<T>& orig, const Image<T>& convolved, const Star& star, PSF& psf, T b, T t);

    template<typename T>
    bool psfFit(const Image<T>& orig, const Image<T>& convolved, Star& star, PSF& psf);

    template<typename T>
    void daofind(const Image<T>& gray, StarVector& star_vector, PSFVector& psf_vector);

public:
    template<typename T>
    StarVector DAOFIND(const Image<T>& img);

    template<typename T>
    PSFVector DAOFIND_PSF(const Image<T>& img);
};
