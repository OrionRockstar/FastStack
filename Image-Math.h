#pragma once
#define _USE_MATH_DEFINES
#include <opencv2/opencv.hpp>
#include <cmath>

inline double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

template<typename PtrType>
inline PtrType Median(PtrType * ptr, int size){
    std::vector<PtrType> imgbuf(size);
    std::copy(&ptr[0], &ptr[size], &imgbuf[0]);

    std::nth_element(&imgbuf[0], &imgbuf[size / 2], &imgbuf[imgbuf.size() - 1]);
    return (PtrType)imgbuf[size / 2];
}

inline std::array<double,2> Median_nMAD (float* ptr,int size){
    std::vector<double> imgbuf(size);
    std::copy(&ptr[0], &ptr[size], &imgbuf[0]);

    std::nth_element(&imgbuf[0], &imgbuf[size / 2], &imgbuf[size - 1]);
    double median=imgbuf[size / 2];

    for (int el = 0; el < size; ++el)
        imgbuf[el] = fabs(imgbuf[el] - median);

    std::nth_element(&imgbuf[0], &imgbuf[size / 2], &imgbuf[size - 1]);
    
    return { median,1.4826*imgbuf[size / 2] };
}

template<typename PtrType,typename SType>
inline double StandardDeviation(PtrType* ptr, int size) {
    SType mean = 0;
    for (int el = 0; el < size; el++)
        mean += ptr[el];
    mean /= size;
    SType var = 0;
    SType d;
    for (int el = 0; el < size; ++el) {
        d = ptr[el] - mean;
        var += d * d;
    }
    return sqrt(var / size);
};

inline void MedianBlur(cv::Mat &img) {
    //3x3 kernel median blur 
    float* iptr = (float*)img.data;
    std::array<float, 9>kernel = { 0 };
    std::vector<float> imgbuf(img.rows * img.cols);

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
}

inline void RotateTranslate(cv::Mat &img, double* aff_mat) {
    float* fptr = (float*)img.data;
    int x_n, y_n, temp, yx, yy, size = img.rows * img.cols, disp = (img.cols * (int)round(aff_mat[5])) + (int)round(aff_mat[2]);

    double theta = atan2(aff_mat[3], aff_mat[0]);

    std::vector<float> pixels(size);

    if (fabs(theta) <= M_PI_2) {
#pragma omp parallel for private(x_n,y_n,yx,yy,temp)
        for (int y = 0; y < img.rows; ++y) {
            yx = int(y * aff_mat[1]);
            yy = int(y * aff_mat[4]);
            for (int x = 0; x < img.cols; ++x)
            {
                x_n = int(x * aff_mat[0] + yx);
                y_n = int(x * aff_mat[3] + yy);
                temp = (y_n * img.cols + x_n) - disp;

                if (temp >= size) {
                    temp -= size;
                    //if (temp >= size)
                        //continue;
                }
                else if (temp < 0) {
                    temp += size;
                    //if (temp < 0)
                        //continue;
                }
                pixels[temp] = fptr[y * img.cols + x];
            }
        }
    }

    else {
#pragma omp parallel for private(x_n,y_n,yx,yy,temp)
        for (int y = 0; y < img.rows; ++y) {
            yx = int(y * aff_mat[1]);
            yy = int(y * aff_mat[4]);
            for (int x = 0; x < img.cols; ++x)
            {
                x_n = int(x * aff_mat[0] + yx);
                y_n = int(x * aff_mat[3] + yy);
                temp = (y_n * img.cols + x_n) + disp;
                if (temp >= size) {
                    temp -= size;
                    //if (temp >= size)
                        //continue;
                }
                else if (temp < 0) {
                    temp += size;
                    //if (temp < 0)
                        //continue;
                }
                pixels[temp] = fptr[y * img.cols + x];
            }
        }
    }

    std::copy(pixels.begin(), pixels.end(), &fptr[0]);
}

inline void MTF(cv::Mat &img) {
    //Midtone Transfer Function
    float* ptr = (float*)img.data;
    
    std::array<double,2> median_nmad = Median_nMAD(ptr,img.rows*img.cols);

    double mid = median_nmad[0], nMAD = median_nmad[1];

    double shadow = mid - 2.8 * nMAD, midtone = 4 * 1.4826 * (mid - shadow); //.75*mid

    for (int el = 0; el < img.rows * img.cols; ++el) {
        ptr[el] = (ptr[el] - shadow) / (1 - shadow);

        if (ptr[el] == 0 || ptr[el] == 1)  continue;

        else if (ptr[el] == midtone)  ptr[el] = .5;

        else ptr[el] = ((midtone - 1) * ptr[el]) / (((2 * midtone - 1) * ptr[el]) - midtone);

    }
}
