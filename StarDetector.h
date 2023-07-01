#pragma once
#include"Image.h"
#include "Star.h"

class StarDetector {

    double m_K = 3.0;
    bool m_median_blur = true;
    int m_num_of_layers = 5;

    struct TrigAngle {
        double _cos = 0;
        double _sin = 0;
    };

    std::vector<TrigAngle> BuildTrigVector() {
        std::vector<TrigAngle> trig_vector;

        for (double theta = 0; theta < 2.0 * M_PI; theta += 2.0 * M_PI / 12)
            trig_vector.push_back({ cos(theta),sin(theta) });

        return trig_vector;
    }

    std::vector<TrigAngle> m_trigvector = BuildTrigVector();

    void BuildTrigVector(std::vector<TrigAngle>& trig_vector);

    bool IsNewStar(int x, int y, int r, StarVector& star_vector);

    StarVector CombineStarVectors(std::vector<StarVector>& svv);

    void AperturePhotometry_WCG(const Image32& img, StarVector& star_vector);

    void AperturePhotometry_Gaussian(const Image32& img, StarVector& star_vector);

public:
    StarDetector() = default;

    StarDetector(double K, bool median_blur, int scale) : m_K(K), m_median_blur(median_blur), m_num_of_layers(scale) {}

    StarVector StarDetection(const Image32 & img);
};

