#pragma once
#include "Image.h"
#include "ImageOperations.h"

struct Star {
    double xc=0; //x-center coordinate
    double yc=0; //y-center coordinate
    double radius=0; //mean radius
    double luminance=0; //approximated absolute magnitude of star

    Star(double x, double y, double r) :xc(x), yc(y), radius(r) {};
    Star()=default;

    bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }


};

struct TrigAngles {
    double costheta;
    double sintheta;
};

typedef std::vector<TrigAngles>TrigVector;
typedef std::vector<Star>StarVector;


namespace stardetection {

//static void TrinerizeImage(Image32& input, Image8& output, float threshold, bool blur);

//static void AperturePhotometry(const Image32& img, StarVector& starvector);

StarVector DetectStars(Image32& img, const float thresh_mult1, const float thresh_mult2, const int max_radius, const bool medianblur);

StarVector DetectStars_WaveletBased(Image32& img, const float thresh_mult, const int max_radius, const int scale);
}