#include "ImageStacking.h"

static void RemoveOutliers(std::vector<float>& pixstack, float l_limit, float u_limit) {

    for (auto it = pixstack.begin(); it != pixstack.end(); ++it)
        if (*it<l_limit || *it>u_limit) {
            pixstack.erase(it);
            it--;
        }
}

static float Mean(std::vector<float>& pixelstack) {
    float mean = 0;

    for (float& pixel : pixelstack)
        mean += pixel;

    return mean / pixelstack.size();
}

//static float Median(std::vector<float>& pixelstack){}

static float StandardDeviation(std::vector<float>& pixelstack) {
    float mean = 0;
    for (float& pixel : pixelstack)
        mean += pixel;
    mean /= pixelstack.size();

    double d;
    double var = 0;
    for (float& pixel : pixelstack) {
        d = pixel - mean;
        var += d * d;
    }

    return (float)sqrt(var / pixelstack.size());
}

static void MeanStandardDeviation(std::vector<float>& pixelstack,float& mean,float& stddev) {
    mean = 0;
    for (float& pixel : pixelstack)
        mean += pixel;
    mean /= pixelstack.size();

    double d;
    double var = 0;
    for (float& pixel : pixelstack) {
        d = pixel - mean;
        var += d * d;
    }
    stddev = (float)sqrt(var / pixelstack.size());
}

void ImageStacking::Average(std::vector<Image32>& imgvec,Image32& final_image){

    std::vector<float> pixelstack(imgvec.size());

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.TotalImage(); ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i][el];
        final_image[el] = Mean(pixelstack);
    }
        
}
    
void ImageStacking::Median(std::vector<Image32>& imgvec, Image32& final_image) {

    std::vector<float> pixelstack(imgvec.size());
    int mid_point = pixelstack.size() / 2;
#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.TotalImage(); ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i][el];

        std::nth_element(pixelstack.begin(), pixelstack.begin() + mid_point, pixelstack.end());
        final_image[el] = pixelstack[mid_point];
    }
}

void ImageStacking::SigmaClipping(std::vector<Image32>& imgvec, Image32& final_image, float l_sigma, float u_sigma) {

    std::vector<float> pixelstack(imgvec.size());

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.TotalImage(); ++el) {
        pixelstack.resize(imgvec.size());
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i][el];

        std::sort(pixelstack.begin(), pixelstack.end());
        float old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            float median = pixelstack[pixelstack.size() / 2];
            float stddev = StandardDeviation(pixelstack);

            if (stddev == 0 || (old_stddev - stddev) == 0)  break;

            RemoveOutliers(pixelstack, median - l_sigma * stddev, median + l_sigma * stddev);

            old_stddev = stddev;
        }
        final_image[el] = Mean(pixelstack);
    }
}

void ImageStacking::KappaSigmaClipping(std::vector<Image32>& imgvec, Image32& final_image, float l_sigma, float u_sigma) {

    std::vector<float> pixelstack(imgvec.size());

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.TotalImage(); ++el) {
        pixelstack.resize(imgvec.size());
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i][el];

        float old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            float mean, stddev;
            MeanStandardDeviation(pixelstack, mean, stddev);
            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            RemoveOutliers(pixelstack, mean - l_sigma * stddev, mean + l_sigma * stddev);

            old_stddev = stddev;
        }
        final_image[el] = Mean(pixelstack);
    }
   
}

void ImageStacking::WinsorizedSigmaClipping(std::vector<Image32>& imgvec, Image32& final_image, float l_sigma, float u_sigma) {

    std::vector<float> pixelstack(imgvec.size());
    int mid_point = pixelstack.size() / 2;

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.TotalImage(); ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i][el];

        std::sort(pixelstack.begin(), pixelstack.end());
        float old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            float median = pixelstack[mid_point];
            float stddev = StandardDeviation(pixelstack);

            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            float u_thresh = median + u_sigma * stddev;
            for (int upper = mid_point; upper < (int)pixelstack.size(); ++upper) 
                if (pixelstack[upper] > u_thresh) pixelstack[upper] = pixelstack[upper - 1];
                
            
            float l_thresh = median - l_sigma * stddev;
            for (int lower = mid_point; lower >= 0; lower--) 
                if (pixelstack[lower] < l_thresh) pixelstack[lower] = pixelstack[lower + 1];
            
            old_stddev = stddev;
        }
        final_image[el] = Mean(pixelstack);
    }
}

void ImageStacking::StackImages(std::vector<Image32>& imgvec, Image32& final_image, StackType stack_type, float l_sigma, float u_sigma) {
    switch (stack_type) {
    case StackType::average:
        ImageStacking::Average(imgvec, final_image);
        break;
    case StackType::median:
        ImageStacking::Median(imgvec, final_image);
        break;
    case StackType::sigma_clip:
        ImageStacking::SigmaClipping(imgvec, final_image, l_sigma, u_sigma);
        break;
    case StackType::kappa_sigma_clip:
        ImageStacking::KappaSigmaClipping(imgvec, final_image, l_sigma, u_sigma);
        break;
    case StackType::winsorized_sigma_clip:
        ImageStacking::WinsorizedSigmaClipping(imgvec, final_image, l_sigma, u_sigma);
        break;
    }

    final_image.ComputeStatistics();
}
