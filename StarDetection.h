#pragma once
#include"Image-Math.h"

class StarDetection
{
public:
    struct Star {
        double xc=0; //x-center coordinate
        double yc=0; //y-center coordinate
        double radius=0; //mean radius
        double luminance=0; //approximated absolte magnitude of star

        Star()=default;

        bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }


    };

    struct TrigAngles {
        double costheta;
        double sintheta;
    };


    typedef std::vector<Star>StarVector;

    void TrinerizeImage(cv::Mat &input, cv::Mat &output, int threshold, bool blur);

    void AperturePhotometry(cv::Mat &img, StarVector &starvector);

    StarVector DetectStars(cv::Mat &img, const double star_thresh, const int vote_thresh, const int total_votes, const int min_radius, const int max_radius,const bool medianblur);
};

