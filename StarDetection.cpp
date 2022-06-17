#include "StarDetection.h"

using SD = StarDetection;

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

void SD::TrinerizeImage(Image &input, Image &output, int threshold, bool blur) {
    output = input.DeepCopy();
    float* iptr = (float*)output.data;

    if (blur);
        ImageOP::MedianBlur3x3(output);

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
    output.Convert<float, uint8_t>(output);
}

void SD::AperturePhotometry(const Image &img, SD::StarVector &starvector) {
   
    int x, y, r, num;
    double intensity_x, intensity_y, intensity2, intensity_sum;

    std::vector<float> skyvec;

    for (auto &star : starvector) {
        x = int(round(star.xc));
        y = int(round(star.yc));
        r = int(round(star.radius))+1;
        intensity_x = 0;
        intensity_y = 0;
        intensity2 = 0;
        intensity_sum = 0;
        num = 0;
        skyvec.clear();

        for (int row = y - 6 * r; row <= y + 6 * r; row++) {
            for (int col = x - 6 * r; col <= x + 6 * r; col++) {
                if ((0 <= row && row < img.rows) && (0 <= col && col < img.cols) && ((4 * r) <= Distance(col, row, x, y) && Distance(col, row, x, y) <= (6 * r)))
                    skyvec.push_back(img.at<float>(row,col));
            }
        }
        auto sky = skyvec.begin() + int(.5 * skyvec.size());
        std::nth_element(skyvec.begin(), sky, skyvec.end());

        for (int row = y - r; row <= y + r; row++) {
            for (int col = x - r; col <= x + r; col++) {
                if ((0 <= row && row < img.rows) && (0 <= col && col < img.cols) && Distance(col, row, x, y) <= r) {
                    double i2 = (double)img.at<float>(row, col) * img.at<float>(row, col);
                    intensity_x += i2 * col;
                    intensity_y += i2 * row;
                    intensity2 += i2;
                    intensity_sum += img.at<float>(row,col);
                    num++;
                }
            }
        }
        star.xc = intensity_x / intensity2;
        star.yc = intensity_y / intensity2;
        star.luminance = -2.5 * log10(intensity_sum - (*sky) * num);
    }

}

SD::StarVector SD::DetectStars(Image &img,const double star_thresh,const int vote_thresh,const int total_votes, const int min_radius, const int max_radius,bool blur) {

    double median = ImageOP::Median(img);
    double stddev = ImageOP::StandardDeviation(img);
    double threshold = median + star_thresh * stddev;

    std::vector <StarDetection::TrigAngles> trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / total_votes)
        trigang.push_back({ cos(theta),sin(theta) });

    Image tri;
    SD::TrinerizeImage(img, tri, threshold, blur);
    uint8_t* tptr = (uint8_t*)tri.data;

    bool newp = true;
    int vote, spacev, istarv, a, b;

    SD::StarVector starvector;
    starvector.reserve(2000);

    for (int y = 0; y < img.rows; y++) {
        for (int x = 0; x < img.cols; x++) {
            if (tri.at<uint8_t>(y,x) == 2) {
                newp = true;
                for (int r = min_radius; r <= max_radius; r++) {

                    vote = 0, spacev = 0, istarv = 0;
                    for (auto &tf:trigang) {
                        if (istarv >= vote_thresh)  break;

                        a = int(round(x + r * tf.costheta)); //x
                        b = int(round(y + r * tf.sintheta)); //y

                        if (0 <= a && a < img.cols && 0 <= b && b < img.rows) {
                            if (tri.at<uint8_t>(b,a) == 2)  istarv++;
                            else if ((tri.at<uint8_t>(b,a) == 1) && (img.at<float>(y,x) > 1.5 * img.at<float>(b,a)))  vote++;
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
                            starvector.emplace_back(Star(x,y,r));

                        else  break;
                    }
                    else if (spacev >= vote_thresh)  break;
                }
            }
        }
    }

    tri.Release();

    SD::AperturePhotometry(img, starvector);

    std::sort(starvector.begin(), starvector.end(), StarDetection::Star());

    if (starvector.size() > 200)
        starvector.resize(200);

    return starvector;
}