#include "StarDetection.h"

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

void stardetection::TrinerizeImage(Image32 &input, Image8 &output, int threshold, bool blur) {
    
    Image32 temp(input.Rows(), input.Cols());
    input.CopyTo(temp);

    if (blur)
        ImageOP::MedianBlur3x3(temp);

    for (int el = 0; el < temp.Total(); ++el) 
        (input[el] >= threshold) ? output[el] = 1 : output[el] = 0;
    

    for (int y = 1; y < output.Rows() - 1; ++y) {
        for (int x = 1; x < output.Cols() - 1; ++x) {
            if (output(y, x) == 1)
                if (output(y, x - 1) != 0 && output(y, x + 1) != 0 && output(y - 1, x) != 0 && output(y + 1, x) != 0) output(y, x) = 2;
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
        r = int(ceil(star.radius + .5));
        intensity_x = 0;
        intensity_y = 0;
        intensity2 = 0;
        intensity_sum = 0;
        num = 0;
        skyvec.clear();
        int r6 = r * 6;
        int r4 = r * 4;

        for (int y = yc - r6; y <= yc + r6; y++) {
            for (int x = xc - r6; x <= xc + r6; x++) {
                if ((0 <= y && y < img.Rows()) && (0 <= x && x < img.Cols()) && (r4 <= Distance(x, y, xc, yc) && Distance(x, y, xc, yc) <= r6))
                    skyvec.push_back(img(y, x));
            }
        }
        auto sky = skyvec.begin() + skyvec.size()/2;
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

StarVector stardetection::DetectStars(Image32 &img,const double thresh_mult1 ,const double thresh_mult2, const int max_radius, const bool blur) {

    double threshold = (thresh_mult1 * img.median) + (thresh_mult2 * img.stdev);

    const int vote_thresh = 6;

    TrigVector trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / 12)
        trigang.push_back({ cos(theta),sin(theta) });

    Image8 tri(img.Rows(),img.Cols());
    stardetection::TrinerizeImage(img, tri, threshold, blur);

    int vote, spacev, istarv, a, b;

    StarVector starvector;
    starvector.reserve(2000);

    for (int y = 0; y < img.Rows(); ++y) {
        for (int x = 0; x < img.Cols(); ++x) {
            newstar:
            if (tri(y,x) == 2) {

                for (int r = 2; r <= max_radius; r++) {

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
                            x++;
                            if (x == img.Cols()) { y++; x = 0; }
                            goto newstar;
                        }
                    }

                    if (vote >= vote_thresh) {
                        for (auto &star : starvector){
                            if (Distance(double(x), double(y), star.xc, star.yc) <= r) {
                                star.xc = .5 * (star.xc + x);
                                star.yc = .5 * (star.yc + y);
                                star.radius = .5 * (star.radius + r);
                                x++;
                                if (x == img.Cols()) { y++; x = 0; }
                                goto newstar;
                            }
                        }
                        starvector.emplace_back(Star(x,y,r));
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