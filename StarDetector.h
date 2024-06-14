#pragma once
#include"Image.h"
#include "Star.h"

class StarDetector {

    double m_K = 3.0;
    double m_peak_edge = 0.65;
    bool m_median_blur = true;
    int m_wavelet_layers = 5;
    int m_max_radius = 32;
    int m_max_sv_size = 200;

    struct TrigAngle {
        double _cos = 0;
        double _sin = 0;
    };

    std::vector<TrigAngle> m_trigvector = BuildTrigVector();

public:
    StarDetector() = default;

    StarDetector(double K, bool median_blur, int scale) : m_K(K), m_median_blur(median_blur), m_wavelet_layers(scale) {}


    void setSigmaK(double K) { m_K = K; }

    void setApplyMedianBlur(bool val) { m_median_blur = val; }

    void setWaveletLayers(int layers) { m_wavelet_layers = layers; }

private:
    std::vector<TrigAngle> BuildTrigVector() {
        std::vector<TrigAngle> trig_vector;

        for (double theta = 0; theta < 2.0 * M_PI; theta += 2.0 * M_PI / 12)
            trig_vector.push_back({ cos(theta),sin(theta) });

        return trig_vector;
    }

    void AddNewStar(int x, int y, int r, StarVector& star_vector);

    StarVector CombineStarVectors(std::vector<StarVector>& svv);

    void TrimStarVector(StarVector& starvector);

    void AperturePhotometry_WCG(const Image32& img, StarVector& star_vector);

    void AperturePhotometry_Gaussian(const Image32& img, StarVector& star_vector);

    void toStructureEdgeMap(Image8& src);

public:
    StarVector ApplyStarDetection(const Image32& img);
};

