#include "StarDetection.h"

using SD = StarDetection;

void SD::TrinerizeImage(cv::Mat input, cv::Mat &output, int threshold, bool blur) {
    output = input.clone();
    ushort* iptr = (ushort*)output.data;

    if (blur)
        MedianBlur(output);

    for (int i = 0; i < output.rows * output.cols; ++i) {
        if (iptr[i] >= threshold)  iptr[i] = 1;
        else  iptr[i] = 0;
    }

    for (int y = 1; y < output.rows - 1; ++y) {
        for (int x = 1; x < output.cols - 1; ++x) {
            if (iptr[y * output.cols + x] == 1) {
                //if ((x - 1 >= 0 && mptr[y * col + (x - 1)] == 0) || (x + 1 < col && mptr[y * col + (x + 1)] == 0) || (y - 1 >= 0 && mptr[(y - 1) * col + x] == 0) || (y + 1 < row && mptr[(y + 1) * col + x] == 0))  continue;
                if (iptr[y * output.cols + (x - 1)] == 0 || iptr[y * output.cols + (x + 1)] == 0 || iptr[(y - 1) * output.cols + x] == 0 || iptr[(y + 1) * output.cols + x] == 0)  continue;
                else  iptr[y * output.cols + x] = 2;
            }
        }
    }
}

void SD::AperturePhotometry(cv::Mat img, SD::StarVector &starvector) {
    unsigned short* iptr = (unsigned short*)img.data;
   
    int x, y, r, num,col = img.cols;
    double intensity_x, intensity_y, intensity2, intensity_sum;
    double mult = 1.0 / 65535.0;
    std::vector<double> skyvec;

    for (auto &star : starvector) {
        x = int(round(star.xc));
        y = int(round(star.yc));
        r = int(ceil(star.radius))+1;
        intensity_x = 0;
        intensity_y = 0;
        intensity2 = 0;
        intensity_sum = 0;
        num = 0;
        skyvec.clear();

        for (int row = y - 6 * r; row <= y + 6 * r; row++) {
            for (int col = x - 6 * r; col <= x + 6 * r; col++) {
                if ((0 <= row && row < img.rows) && (0 <= col && col < img.cols) && ((4 * r) <= Distance(col, row, x, y) && Distance(col, row, x, y) <= (6 * r)))
                    skyvec.push_back(mult * (double)iptr[row * img.cols + col]);
            }
        }
        auto sky = skyvec.begin() + int(.5 * skyvec.size());
        std::nth_element(skyvec.begin(), sky, skyvec.end());

        for (int row = y - r; row <= y + r; row++) {
            for (int col = x - r; col <= x + r; col++) {
                if ((0 <= row && row < img.rows) && (0 <= col && col < img.cols) && Distance(col, row, x, y) <= r) {
                    intensity_x += (mult * iptr[row * img.cols + col]) * (mult * iptr[row * img.cols + col]) * col;
                    intensity_y += (mult * iptr[row * img.cols + col]) * (mult * iptr[row * img.cols + col]) * row;
                    intensity2 += (mult * iptr[row * img.cols + col]) * (mult * iptr[row * img.cols + col]);
                    intensity_sum += mult * iptr[row * img.cols + col];
                    num++;
                }
            }
        }
        star.xc = intensity_x / intensity2;
        star.yc = intensity_y / intensity2;
        star.luminance = -2.5 * log10(intensity_sum - (*sky) * num);
    }

}

SD::StarVector SD::DetectStars(cv::Mat img,const double star_thresh,const int vote_thresh,const int total_votes, const int min_radius, const int max_radius,bool blur) {
    ushort* iptr = (ushort*)img.data;

    unsigned short median = Median<unsigned short>(iptr, img.rows * img.cols);
    unsigned short stddev = (unsigned short)StandardDeviation<unsigned short,int64_t>(iptr, img.rows * img.cols);
    int threshold = median + star_thresh * stddev;

    std::vector <StarDetection::TrigAngles> trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / total_votes)
        trigang.push_back({ cos(theta),sin(theta) });

    cv::Mat tri;
    SD().TrinerizeImage(img, tri, threshold, blur);
    uchar* tptr = (uchar*)tri.data;

    bool newp = true;
    int vote, spacev, istarv, a, b;

    SD::StarVector starvector;

    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            if (tptr[y * img.cols + x] == 2) {
                newp = true;
                for (int r = min_radius; r <= max_radius; r++) {
                    vote = 0, spacev = 0, istarv = 0;
                    for (auto trigf = trigang.begin(); trigf != trigang.end(); trigf++) {
                        if (istarv >= vote_thresh)  break;
                        a = int(round(x + r * (*trigf).costheta)); //x
                        b = int(round(y + r * (*trigf).sintheta)); //y 
                        if (0 <= a && a < img.cols && 0 <= b && b < img.rows) {
                            if (tptr[b * img.cols + a] == 2)  istarv++;
                            else if ((tptr[b * img.cols + a] == 1) && (iptr[y * img.cols + x] > 1.5 * iptr[b * img.cols + a]))  vote++;
                            else  spacev++;
                        }
                        else {
                            spacev += vote_thresh;
                            vote = 0;
                            break;
                        }
                    }

                    if (vote >= vote_thresh) {
                        for (auto &star:starvector){
                            if (Distance(double(x), double(y), star.xc, star.yc) <= r) {
                                star.xc = .5 * (star.xc + x);
                                star.yc = .5 * (star.yc + y);
                                star.radius = .5 * (star.radius + r);
                                newp = false;
                                break;
                            }
                        }
                        if (newp)
                            starvector.push_back({ double(x),double(y),double(r) });

                        else  break;
                    }
                    else if (spacev >= vote_thresh)  break;
                }
            }
        }
    }

    tri.release();

    SD().AperturePhotometry(img, starvector);

    std::sort(starvector.begin(), starvector.end(), StarDetection::Star());

    if (starvector.size() > 200)
        starvector.resize(200);

    return starvector;
}