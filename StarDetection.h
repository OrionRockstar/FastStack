#pragma once
#include"Image-Math.h"

class StarDetection
{
public:
    struct Star {
        double xc; //x-center coordinate
        double yc; //y-center coordinate
        double radius; //mean radius
        double luminance; //approximated absolte magnitude of star

        bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }
    };

    struct TrigAngles {
        double costheta;
        double sintheta;
    };

    typedef std::vector<Star>StarVector;

    void TrinerizeImage(cv::Mat img, int threshold, bool blur);

    void AperturePhotometry(cv::Mat img, StarVector &starvector);

    StarVector DetectStars(cv::Mat img, const double star_thresh, const int vote_thresh, const int total_votes, const int min_radius, const int max_radius,const bool medianblur);
};

