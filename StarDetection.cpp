#include "StarDetection.h"

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

static void TrinerizeImage(Image32 &input, Image8 &output, float threshold, bool blur) {
    
    Image32 temp(input.Rows(), input.Cols());
    input.CopyTo(temp);

    if (blur)
        ImageOP::MedianBlur3x3(temp);

    for (int el = 0; el < temp.Total(); ++el)
        output[el] = (input[el] >= threshold) ? 1 : 0;
    
    for (int y = 1; y < output.Rows() - 1; ++y) {
        for (int x = 1; x < output.Cols() - 1; ++x) {
            if (output(x, y) == 1)
                if (output(x - 1, y) != 0 && output(x + 1, y) != 0 && output(x, y - 1) != 0 && output(x, y + 1) != 0) output(x, y) = 2;
        }
    }
}

static void AperturePhotometry(const Image32 &img, StarVector &starvector) {

    std::vector<float> local_background;

    for (auto& star : starvector) {
        int xc = int(round(star.xc)),
            yc = int(round(star.yc)),
            r = int(ceil(star.radius)),
            r6 = r * 6,
            r4 = r * 4,
            num = 0;
        double intensity_x = 0,
               intensity_y = 0,
               intensity2 = 0,
               intensity_sum = 0;
        local_background.clear();

        for (int y = yc - r6; y <= yc + r6; y++) {
            for (int x = xc - r6; x <= xc + r6; x++) {
                if ((0 <= y && y < img.Rows()) && (0 <= x && x < img.Cols()) && (r4 <= Distance(x, y, xc, yc) && Distance(x, y, xc, yc) <= r6))
                    local_background.push_back(img(x, y));
            }
        }
        auto background = local_background.begin() + local_background.size()/2;
        std::nth_element(local_background.begin(), background, local_background.end());

        for (int y = yc - r; y <= yc + r; y++) {
            for (int x = xc - r; x <= xc + r; x++) {
                if ((0 <= y && y < img.Rows()) && (0 <= x && x < img.Cols()) && Distance(x, y, xc, yc) <= r) {
                    double i2 = (double)img(x, y) * img(x, y);
                    intensity_x += i2 * x;
                    intensity_y += i2 * y;
                    intensity2 += i2;
                    intensity_sum += img(x, y);
                    num++;
                }
            }
        }
        star.xc = intensity_x / intensity2;
        star.yc = intensity_y / intensity2;
        star.luminance = -2.5 * log10(intensity_sum - (*background) * num);
    }
}

StarVector stardetection::DetectStars(Image32 &img,const float thresh_mult1 ,const float thresh_mult2, const int max_radius, const bool blur) {

    double threshold;// = (thresh_mult1 * img.median) + (thresh_mult2 * img.stdev);

    const int vote_thresh = 6;

    TrigVector trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / 12)
        trigang.push_back({ cos(theta),sin(theta) });

    Image8 tri(img.Rows(),img.Cols());
    TrinerizeImage(img, tri, threshold, blur);

    StarVector starvector;
    starvector.reserve(2000);

    for (int y = 0; y < img.Rows(); ++y) {
        for (int x = 0; x < img.Cols(); ++x) {
            newstar:
            if (tri(x, y) == 2) {

                for (int r = 2; r <= max_radius; r++) {

                    int vote = 0, spacev = 0, istarv = 0;
                    for (auto &tf:trigang) {
                        if (istarv >= vote_thresh)  break;

                        int a = int(round(x + r * tf.costheta)); //x
                        int b = int(round(y + r * tf.sintheta)); //y

                        if (0 <= a && a < img.Cols() && 0 <= b && b < img.Rows()) {
                            if (tri(a,b) == 2)  istarv++;
                            else if ((tri(a,b) == 1) && (img(x,y) > 1.5 * img(a,b)))  vote++;
                            else  spacev++;
                        }
                        else {
                            x++;
                            if (x >= img.Cols()) { y++; x = 0; }
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
                                if (x >= img.Cols()) { y++; x = 0; }
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

    AperturePhotometry(img, starvector);

    std::sort(starvector.begin(), starvector.end(), Star());

    if (starvector.size() > 200)
        starvector.resize(200);

    return starvector;
}

StarVector stardetection::DetectStars_WaveletBased(Image32& img, const float thresh_mult, const int max_radius, const int scale) {

    const int vote_thresh = 6, total_votes = 12;

    TrigVector trigang;
    for (double theta = 0; theta < 2 * M_PI; theta += 2 * M_PI / total_votes)
        trigang.push_back({ cos(theta),sin(theta) });

    Image8Vector wavelet_vector;
    ImageOP::B3WaveletTransformTrinerized(img, wavelet_vector, thresh_mult, scale);

    StarVector starvector;
    starvector.reserve(2000);

    int min_radius = 1;
    for (auto& wavelet : wavelet_vector) {

        double ix2min_radius = 1.0 / (2 * min_radius);

        for (int y = 0; y < img.Rows(); ++y) {
            for (int x = 0; x < img.Cols(); ++x) {
            newstar:
                if (wavelet(x, y) == 2) {
                    for (int r = min_radius; r <= max_radius; ++r) {

                        int vote = 0, spacev = 0, istarv = 0;

                        for (auto& tf : trigang) {

                            if (istarv == vote_thresh)  break;

                            int a = int(round(x + r * tf.costheta)); //x
                            int b = int(round(y + r * tf.sintheta)); //y 

                            if (img.IsInBounds(a, b)) {

                                if (wavelet(a, b) == 2)  istarv++;

                                else if ((wavelet(a, b) == 1) && (img(x, y) > 1.85f * img(a, b))) vote++;

                                else  spacev++;
                            }
                            else {
                                x++;
                                if (x >= img.Cols()) { y++; x = 0; }
                                goto newstar;
                            }
                        }

                        if (vote >= vote_thresh) {
                            for (struct Star& star : starvector) {
                                if (Distance(double(x), double(y), star.xc, star.yc) <= star.radius + min_radius) { //adjust star.rad + r whch gives best radius
                                    star.xc = .5 * (star.xc + x);
                                    star.yc = .5 * (star.yc + y);
                                    star.radius = (star.radius + (r * ix2min_radius));//adjust
                                    x++;
                                    if (x >= img.Cols()) { y++; x = 0; }
                                    goto newstar;
                                }
                            }
                            starvector.emplace_back(x, y, r);
                        }
                        else if (spacev >= vote_thresh)  break;
                    }
                }
            }
        }
        min_radius *= 2;
    }

    AperturePhotometry(img, starvector);

    std::sort(starvector.begin(), starvector.end(), Star());

    if (starvector.size() > 200)
        starvector.resize(200);

    return starvector;
}
