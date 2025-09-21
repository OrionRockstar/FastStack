#pragma once
#include"Image.h"
#include "Star.h"
#include "Wavelet.h"
#include "Matrix.h"


class StarDetector {

    StructureMaps m_maps;
    inline static float m_sigma = 0.42466 * 3; // constant * fwhm
    float m_K = 1.0;
    float m_roundness = 0.5;

    float m_beta = 10.0f;
    PSF::Type m_psf_type = PSF::Type::gaussian;

    PSF m_average_psf;
public:
    StarDetector() = default;

    float sigmaK()const { return m_K; }

    void setSigmaK(float K) { m_K = K; }

    float roundness()const { return m_roundness; }

    void setRoundness(float roundness) { m_roundness = roundness; }

    PSF::Type psf()const { return m_psf_type; }

    void setPSF(PSF::Type psf_type) { m_psf_type = psf_type; }

    float beta()const { return m_beta; }

    void setBeta(float beta) { m_beta = beta; }

    PSF meanPSF()const { return m_average_psf; }

private:
    StarVector combineStarVectors(std::vector<StarVector>& svv);

    template<typename T>
    bool centerGravityFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t);

    template<typename T>
    bool gaussianFit(const Image<T>& img, const Star& star, PSF& psf, T b);

    template<typename T>
    bool moffatFit(const Image<T>& img, const Star& star, PSF& psf, T b);

    template<typename T>
    bool psfFit(const Image<T>& img, Star& star, PSF& psf);

public:
    template<typename T>
    StarVector DAOFIND(const Image<T>& img);
};
