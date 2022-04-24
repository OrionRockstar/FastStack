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

//inline double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

/*inline int Median(unsigned short* iptr, int size) {
    std::vector<unsigned short> imgbuf(size);
    std::copy(&iptr[0], &iptr[size], &imgbuf[0]);

    std::nth_element(&imgbuf[0], &imgbuf[size / 2], &imgbuf[imgbuf.size()]);
    return imgbuf[size / 2];
};*/

/*inline int StandardDeviation(unsigned short* iptr, int size) {
    int64_t mean = 0;
    for (int el = 0; el < size; el++)
        mean += iptr[el];
    mean /= size;
    int64_t var = 0;
    int64_t d;
    for (int el = 0; el < size; ++el) {
        d = iptr[el] - mean;
        var += d * d;
    }
    return (int)sqrt(var / size);
};*/

/*inline void MedianBlur(cv::Mat img) {
    unsigned short* iptr = (unsigned short*)img.data;
    std::array<unsigned short, 9>kernel = { 0 };
    std::vector<unsigned short> imgbuf(img.rows * img.cols);

#pragma omp parallel for firstprivate(kernel)
    for (int y = 1; y < img.rows - 1; ++y) {
        for (int x = 1; x < img.cols - 1; ++x) {
            for (int ky = -1; ky < 2; ++ky) {
                for (int kx = -1; kx < 2; ++kx)
                    kernel[ky * 3 + kx + 4] = iptr[(y * img.cols + x) + (ky * img.cols + kx)];
            }

            for (int r = 0; r < 3; ++r) {
                for (int i = 0; i < 4; ++i) {
                    if (kernel[i] > kernel[4])
                        std::swap(kernel[i], kernel[4]);
                    if (kernel[i + 5] < kernel[4])
                        std::swap(kernel[i + 5], kernel[4]);
                }
            }
            imgbuf[y * img.cols + x] = kernel[4];
        }
    }
    std::copy(&imgbuf[0], &imgbuf[imgbuf.size()], &iptr[0]);
}*/
