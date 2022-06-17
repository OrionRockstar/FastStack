#pragma once
#include "Image.h"

class StarDetection
{
public:
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


    typedef std::vector<Star>StarVector;

    void TrinerizeImage(Image &input, Image &output, int threshold, bool blur);

    void AperturePhotometry(const Image &img, StarVector &starvector);

    StarVector DetectStars(Image &img, const double star_thresh, const int vote_thresh, const int total_votes, const int min_radius, const int max_radius,const bool medianblur);
};

