#include "StarDetection.h"

//using SD = StarDetection;

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

void stardetection::TrinerizeImage(Image32 &input, Image8 &output, int threshold, bool blur) {
    
    Image32 temp(input.Rows(), input.Cols());
    input.CopyTo(temp);

    if (blur)
        ImageOP::MedianBlur3x3(temp);

    for (int i = 0; i < temp.Total(); ++i) {
        if (temp[i] >= threshold)  output[i] = 1;
        else  output[i] = 0;
    }

    for (int y = 1; y < output.Rows() - 1; ++y) {
        for (int x = 1; x < output.Cols() - 1; ++x) {
            if (output(y, x) == 1) {
                if (output(y, x - 1) == 0 || output(y, x + 1) == 0 || output(y - 1, x) == 0 || output(y + 1, x) == 0)  continue;
                else  output(y, x) = 2;
            }
        }
    }
}

void stardetection::AperturePhotometry(const Image32 &img, StarVector &starvector) {

    int xc, yc, r, num;
    double intensity_x, intensity_y, intensity2, intensity_sum;
    std::vector<float> skyvec;

    for (auto& star : starvector) {
        xc = int(round(star.xc));
        yc = int(round(star.yc));
        r = int(round(star.radius)) + 1;
        intensity_x = 0;
        intensity_y = 0;
        intensity2 = 0;
        intensity_sum = 0;
        num = 0;
        skyvec.clear();

        for (int y = yc - 6 * r; y <= yc + 6 * r; y++) {
            for (int x = xc - 6 * r; x <= xc + 6 * r; x++) {
                if ((0 <= y && y < img.Rows()) && (0 <= x && x < img.Cols()) && ((4 * r) <= Distance(x, y, xc, yc) && Distance(x, y, xc, yc) <= (6 * r)))
                    skyvec.push_back(img(y, x));
            }
        }
        auto sky = skyvec.begin() + int(.5 * skyvec.size());
        std::nth_element(skyvec.begin(), sky, skyvec.end());

        for (int y = yc - r; y <= yc + r; y++) {
            for (int x = xc - r; x <= xc + r; x++) {
                if ((0 <= y && y < img.Rows()) && (0 <= x && x < img.Cols()) && Distance(x, y, xc, yc) <= r) {
                    double i2 = (double)img(y, x) * img(y, x);
                    intensity_x += i2 * x;
                    intensity_y += i2 * y;
                    intensity2 += i2;
                    intensity_sum += img(y, x);
                    num++;
                }
            }
        }
        star.xc = intensity_x / intensity2;
        star.yc = intensity_y / intensity2;
        star.luminance = -2.5 * log10(intensity_sum - (*sky) * num);
    }
}

StarVector stardetection::DetectStars(Image32 &img,const double star_thresh,const int vote_thresh,const int total_votes, const int min_radius, const int max_radius,bool blur) {

    double threshold = img.median + star_thresh * img.stdev;

    TrigVector trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / total_votes)
        trigang.push_back({ cos(theta),sin(theta) });

    Image8 tri(img.Rows(),img.Cols());
    stardetection::TrinerizeImage(img, tri, threshold, blur);

    bool newp = true;
    int vote, spacev, istarv, a, b;

    StarVector starvector;
    starvector.reserve(2000);

    for (int y = 0; y < img.Rows(); y++) {
        for (int x = 0; x < img.Cols(); x++) {
            if (tri(y,x) == 2) {
                newp = true;
                for (int r = min_radius; r <= max_radius; r++) {

                    vote = 0, spacev = 0, istarv = 0;
                    for (auto &tf:trigang) {
                        if (istarv >= vote_thresh)  break;

                        a = int(round(x + r * tf.costheta)); //x
                        b = int(round(y + r * tf.sintheta)); //y

                        if (0 <= a && a < img.Cols() && 0 <= b && b < img.Rows()) {
                            if (tri(b,a) == 2)  istarv++;
                            else if ((tri(b,a) == 1) && (img(y,x) > 1.5 * img(b,a)))  vote++;
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

    stardetection::AperturePhotometry(img, starvector);

    std::sort(starvector.begin(), starvector.end(), Star());

    if (starvector.size() > 200)
        starvector.resize(200);

    return starvector;
}