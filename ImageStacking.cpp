#include "ImageStacking.h"

using IS = ImageStacking;

static float Mean(std::vector<float>& pixelstack) {
    float mean = 0;
    for (size_t el = 0; el < pixelstack.size(); ++el)
        mean += pixelstack[el];
    return mean /pixelstack.size();
}

static float Median(std::vector<float>& pixelstack);

static float StandardDeviation(std::vector<float>& pixelstack) {
    float mean = 0;
    for (size_t el = 0; el < pixelstack.size(); ++el)
        mean += pixelstack[el];
    mean /= pixelstack.size();
    float d;
    double var = 0;
    for (size_t el = 0; el < pixelstack.size(); ++el){
        d = pixelstack[el] - mean;
        var += d * d;
    }
    return (float)sqrt(var / pixelstack.size());
}

static void MeanStandardDeviation(std::vector<float>& pixelstack,float& mean,float& stddev) {
    mean = 0;
    for (size_t el = 0; el < pixelstack.size(); ++el)
        mean += pixelstack[el];
    mean /= pixelstack.size();
    float d;
    double var = 0;
    for (size_t el = 0; el < pixelstack.size(); ++el) {
        d = pixelstack[el] - mean;
        var += d * d;
    }
    stddev=(float)sqrt(var / pixelstack.size());
}

void IS::Average(std::vector<Image>& imgvec,Image& final_image){

    std::vector<float> pixelstack(imgvec.size());
    float median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.total; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].at<float>(el);
        final_image.at<float>(el) = Mean(pixelstack);
    }
        
}
    
void IS::Median(std::vector<Image>& imgvec, Image& final_image) {

    std::vector<float> pixelstack(imgvec.size());

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.total; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].at<float>(el);

        std::nth_element(pixelstack.begin(), pixelstack.begin() + pixelstack.size() / 2, pixelstack.end());
        final_image.at<float>(el) = pixelstack[pixelstack.size() / 2];
    }
}

void IS::SigmaClipping(std::vector<Image>& imgvec, Image& final_image,double l_sigma,double u_sigma) {

    std::vector<float> pixelstack(imgvec.size());
    float median = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,median,stddev,old_stddev)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        pixelstack.resize(imgvec.size());
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].at<float>(el);

        std::sort(pixelstack.begin(), pixelstack.end());
        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            median = pixelstack[pixelstack.size() / 2];
            stddev = StandardDeviation(pixelstack);

            if (stddev == 0 || (old_stddev - stddev) == 0)  break;

            std::experimental::erase_if(pixelstack,[median, stddev, l_sigma, u_sigma](auto x) {return (x < median - l_sigma * stddev || x > median + u_sigma * stddev); });
            old_stddev = stddev;
        }
        final_image.at<float>(el) = Mean(pixelstack);
    }
}

void IS::KappaSigmaClipping(std::vector<Image>& imgvec, Image& final_image, double l_sigma, double u_sigma) {

    std::vector<float> pixelstack(imgvec.size());
    float mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,mean,stddev,old_stddev)
    for (int el = 0; el < final_image.total; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].at<float>(el);

        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            MeanStandardDeviation(pixelstack, mean, stddev);
            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            std::experimental::erase_if(pixelstack, [mean, stddev, l_sigma, u_sigma](auto x) {return (x < mean - l_sigma * stddev || x > mean + u_sigma * stddev); });

            old_stddev = stddev;
        }
        final_image.at<float>(el) = Mean(pixelstack);
    }
   
}

void IS::WinsorizedSigmaClipping(std::vector<Image>& imgvec, Image& final_image, double l_sigma, double u_sigma) {

    std::vector<float> pixelstack(imgvec.size());
    float median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,median,stddev,old_stddev)
    for (int el = 0; el < final_image.total; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = ((float*)imgvec[i].data)[el];

        std::sort(pixelstack.begin(), pixelstack.end());
        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            median = pixelstack[pixelstack.size() / 2];
            stddev = StandardDeviation(pixelstack);

            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            float u_thresh = median + u_sigma * stddev;
            for (int upper = pixelstack.size() / 2; upper < pixelstack.size(); ++upper) 
                if (pixelstack[upper] > u_thresh) pixelstack[upper] = pixelstack[upper - 1];
                
            
            float l_thresh = median - l_sigma * stddev;
            for (int lower = pixelstack.size() / 2; lower >= 0; lower--) 
                if (pixelstack[lower] < l_thresh) pixelstack[lower] = pixelstack[lower + 1];
            
            old_stddev = stddev;
        }
        final_image.at<float>(el) = Mean(pixelstack);
    }
}

