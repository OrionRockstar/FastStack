#include "ImageStacking.h"

using IS = ImageStacking;
//void stack_frames(std::vector<Image_ptr> imgvec, cv::Mat final, int stack_t, int l_sigma, int u_sigma) 

void IS::Average(std::vector<IS::ImagePtr> imgvec,cv::Mat final_image){
    ushort* fptr = (ushort*)final_image.data;
    std::vector<ushort> pixelstack(imgvec.size());
    int median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].iptr[el];
        fptr[el] = ushort(std::accumulate(pixelstack.begin(), pixelstack.end(), 0) / pixelstack.size());
    }
        
}
    
void IS::Median(std::vector<IS::ImagePtr> imgvec, cv::Mat final_image) {
    ushort* fptr = (ushort*)final_image.data;
    std::vector<ushort> pixelstack(imgvec.size());
    int median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].iptr[el];

        std::nth_element(pixelstack.begin(), pixelstack.begin() + pixelstack.size() / 2, pixelstack.end());
        fptr[el] = pixelstack[pixelstack.size() / 2];
    }
}

void IS::SigmaClipping(std::vector<IS::ImagePtr> imgvec, cv::Mat final_image,double l_sigma,double u_sigma) {
    ushort* fptr = (ushort*)final_image.data;
    std::vector<ushort> pixelstack(imgvec.size());
    int median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,median,stddev,old_stddev,imgvec)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        pixelstack.resize(imgvec.size());
        for (int i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].iptr[el];

        std::sort(pixelstack.begin(), pixelstack.end());
        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            median = pixelstack[pixelstack.size() / 2];
            stddev = StandardDeviation(&pixelstack[0], (int)pixelstack.size());

            if (stddev == 0 || (old_stddev - stddev) == 0)  break;

            std::experimental::erase_if(pixelstack,[median, stddev, l_sigma, u_sigma](auto x) {return (x < median - l_sigma * stddev || x > median + u_sigma * stddev); });
            old_stddev = stddev;
        }
        fptr[el] = ushort(std::accumulate(pixelstack.begin(), pixelstack.end(), 0) / pixelstack.size());
    }
}

void IS::KappaSigmaClipping(std::vector<IS::ImagePtr> imgvec, cv::Mat final_image, double l_sigma, double u_sigma) {
    ushort* fptr = (ushort*)final_image.data;
    std::vector<ushort> pixelstack(imgvec.size());
    int median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,mean,stddev,old_stddev)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].iptr[el];

        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            mean = ushort(std::accumulate(pixelstack.begin(), pixelstack.end(), 0) / pixelstack.size());
            stddev = (ushort)sqrt(std::accumulate(pixelstack.begin(), pixelstack.end(), (double)0, [mean](auto a, auto b) {return a + pow(b - mean, 2); }) / pixelstack.size());

            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            std::experimental::erase_if(pixelstack, [mean, stddev, l_sigma, u_sigma](auto x) {return (x < mean - l_sigma * stddev || x > mean + u_sigma * stddev); });
            //for (auto it = pixelstack.begin(); it != pixelstack.end(); ++it)
                //if (*it < median - l_sigma * stddev || *it > median + u_sigma * stddev)
                    //*it = median;

            old_stddev = stddev;
        }
        fptr[el] = ushort(std::accumulate(pixelstack.begin(), pixelstack.end(), 0) / pixelstack.size());
    }
   
}

void IS::WinsorizedSigmaClipping(std::vector<IS::ImagePtr> imgvec, cv::Mat final_image, double l_sigma, double u_sigma) {
    ushort* fptr = (ushort*)final_image.data;
    std::vector<ushort> pixelstack(imgvec.size());
    int median = 0, mean = 0, stddev = 0, old_stddev = 0;

#pragma omp parallel for firstprivate(pixelstack,median,stddev,old_stddev)
    for (int el = 0; el < final_image.rows * final_image.cols; ++el) {
        for (size_t i = 0; i < pixelstack.size(); ++i)
            pixelstack[i] = imgvec[i].iptr[el];

        std::sort(pixelstack.begin(), pixelstack.end());
        old_stddev = 0;
        for (int iter = 0; iter < 5; iter++) {
            median = pixelstack[pixelstack.size() / 2];
            stddev = StandardDeviation(&pixelstack[0], (int)pixelstack.size());
            if (stddev == 0 || (old_stddev - stddev) == 0) break;

            int unn = 0;
            for (int upper = pixelstack.size() / 2; upper < pixelstack.size(); ++upper) {
                if (pixelstack[upper] <= median + u_sigma * stddev) unn = pixelstack[upper];
                else if (pixelstack[upper] > median + u_sigma * stddev) pixelstack[upper] = unn;
            }
            int lnn = 0;
            for (int lower = pixelstack.size() / 2; lower >= 0; lower--) {
                if (pixelstack[lower] >= median - l_sigma * stddev) lnn = pixelstack[lower];
                else if (pixelstack[lower] < median - l_sigma * stddev) pixelstack[lower] = lnn;
            }

            old_stddev = stddev;
        }
        fptr[el] = ushort(std::accumulate(pixelstack.begin(), pixelstack.end(), 0) / pixelstack.size());
    }
}

