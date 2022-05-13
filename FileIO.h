#pragma once
#include<opencv2/core.hpp>
#include "cfitsio/fitsio.h"
class FileIO
{
    //Reads 8-bit and 16-bit fit files into 'normalized' 32-bit float
	cv::Mat Fit2Mat(char*file) {
        fitsfile* fptr;   /* FITS file pointer, defined in fitsio.h */
        int status = 0;   /* CFITSIO status value MUST be initialized to zero! */
        int bitpix, naxis;// , ii, anynul;
        long naxes[2] = { 1,1 }, fpixel[2] = { 1,1 };
        int count = 0;
        cv::Mat img;

        if (!fits_open_file(&fptr, file, READONLY, &status))
        {
            if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status))
            {
                if (naxis > 2 || naxis == 0)
                    printf("Error: only 1D or 2D images are supported\n");
                else
                {
                    if (bitpix == 16) {
                        img = cv::Mat(naxes[1], naxes[0], CV_16U);
                        unsigned short* ptr = (unsigned short*)img.data;
                        fits_read_pix(fptr, TUSHORT, fpixel, naxes[0] * naxes[1], NULL, ptr, NULL, &status);
                        ptr = NULL;
                        img.convertTo(img, CV_32F);
                        float* nptr = (float*)img.data;
                        for (int el = 0; el < img.rows * img.cols; ++el) nptr[el] /= 65535;
                    }
                    else if (bitpix == 8) {
                        img = cv::Mat(naxes[1], naxes[0], CV_8U);
                        unsigned char* mptr = (unsigned char*)img.data;
                        fits_read_pix(fptr, TUSHORT, fpixel, naxes[0] * naxes[1], NULL, mptr, NULL, &status);
                        img.convertTo(img, CV_32F);
                        float* nptr = (float*)img.data;
                        for (int el = 0; el < img.rows * img.cols; ++el) nptr[el] /= 255;
                    }
                }
            }
            fits_close_file(fptr, &status);
        }
        if (status) fits_report_error(stderr, status); /* print any error message */
        return img;
    }

    void Mat2Fit(cv::Mat img);

};

