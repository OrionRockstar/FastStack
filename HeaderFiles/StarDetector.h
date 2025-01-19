#pragma once
#include"Image.h"
#include "Star.h"
#include "Wavelet.h"
#include "Matrix.h"


class StarDetector {

    StructureMaps m_maps;
    float m_K = 1.0;
    float m_peak_edge = 0.65;
    float m_roundness = 0.5;
    const int m_max_radius = 69;
    //uint16_t m_max_sv_size = 65535;

    float m_beta = 10.0f;
    PSF::Type m_psf_type = PSF::Type::gaussian;

    struct TrigAngle {
        double m_cos = 0;
        double m_sin = 0;
    };

    const std::vector<TrigAngle> m_trigvector = buildTrigVector();

    PSF m_average_psf;
public:
    StarDetector() = default;

    //StarDetector(uint16_t num_stars) { m_max_sv_size = num_stars; }

    StarDetector& operator=(StarDetector&& other) {

        if (this != &other) {
            m_maps = std::move(other.m_maps);
            m_K = other.m_K;
            m_peak_edge = other.m_peak_edge;
            m_roundness = other.m_roundness;
            //m_max_sv_size = other.m_max_sv_size;
        }

        return *this;
    }

    float sigmaK()const { return m_K; }

    void setSigmaK(float K) { m_K = K; }

    float peakEdge()const { return m_peak_edge; }

    void setPeakEdge(float val) { m_peak_edge = val; }

    float roundness()const { return m_roundness; }

    void setRoundness(float roundness) { m_roundness = roundness; }

    bool medianBlur()const { return m_maps.medianBlur(); }

    void applyMedianBlur(bool val) { m_maps.applyMedianBlur(val); }

    PSF::Type psf()const { return m_psf_type; }

    void setPSF(PSF::Type psf_type) { m_psf_type = psf_type; }

    float beta()const { return m_beta; }

    void setBeta(float beta) { m_beta = beta; }

    uint8_t waveletLayers()const { return m_maps.layers(); }

    void setWaveletLayers(uint8_t layers) { m_maps.setLayers(layers); }

    //uint16_t maxNumberofStars()const { return m_max_sv_size; }

    //void setMaxNumberofStars(uint16_t num_stars) { m_max_sv_size = num_stars; }
    PSF meanPSF()const { return m_average_psf; }
private:
    constexpr std::vector<TrigAngle> buildTrigVector()const {
        std::vector<TrigAngle> trig_vector;

        for (double theta = 0; theta < 2.0 * M_PI; theta += 2.0 * M_PI / 12)
            trig_vector.push_back({ cos(theta),sin(theta) });

        return trig_vector;
    }

    void addNewStar(int x, int y, int r, StarVector& star_vector);

    StarVector combineStarVectors(std::vector<StarVector>& svv);

    template<typename T>
    void aperturePhotometry_WCG(const Image<T>& img, StarVector& star_vector);

    template<typename T>
    bool gaussianFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t);

    template<typename T>
    bool moffatFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t);

    template<typename T>
    bool psfFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t);

    //template<typename T>
    //void aperturePhotometry(const Image<T>& img, StarVector& star_vector);

    template<typename T>
    StarPSFVector aperturePhotometry(const Image<T>& img, StarVector& star_vector);

    void toStructureEdgeMap(Image8& src);

public:
    static PSF computeMeanPSF(const StarPSFVector& stars);

    template<typename T>
    StarVector applyStarDetection(const Image<T>& img);
};
