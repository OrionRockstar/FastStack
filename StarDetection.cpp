#include "StarDetection.h"

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

static float Max(float a, float b) { return (a > b) ? a : b; }

static float MedianNC(std::vector<float>& vector) {
    size_t mid = vector.size() / 2;
    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}

static float MADNC(std::vector<float>& vector, float median) {
    size_t mid = vector.size() / 2;
    for (auto& val : vector)
        val = fabs(val - median);
    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}


static void TrinerizeImage(Image32& input, Image8 &output, float threshold, bool blur) {
    
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

static void AperturePhotometry(const Image32& img, StarVector& starvector) {

    std::vector<float> local_background;

    for (auto star = starvector.begin(); star != starvector.end(); ++star) {

        int xc = int(round(star->xc)),
            yc = int(round(star->yc)),
            r = int(star->radius + 1),
            r_outer = r * 3.5,
            r_inner = r * 1.5,
            num = 0;

        bool erase_star = false;

        double intensity_x = 0,
               intensity_y = 0,
               intensity2 = 0,
               intensity_sum = 0;

        local_background.clear();

        for (int y = yc - r_outer; y <= yc + r_outer; y++) {
            for (int x = xc - r_outer; x <= xc + r_outer; x++) {
                double dist = Distance(x, y, star->xc, star->yc);
                if (img.IsInBounds(x,y) && (r_inner <= dist && dist <= r_outer))
                    local_background.push_back(img(x, y));
            }
        }

        for (int y = yc - r; y <= yc + r; y++) {
            for (int x = xc - r; x <= xc + r; x++) {
                if (img.IsInBounds(x, y)) {
                    if (Distance(x, y, xc, yc) <= r) {

                        double i2 = (double)img(x, y) * img(x, y);
                        intensity_x += i2 * x;
                        intensity_y += i2 * y;
                        intensity2 += i2;
                        intensity_sum += img(x, y);
                        num++;
                    }
                }
                else erase_star = true;
            }
        }

        if (erase_star) {
            starvector.erase(star);
            star--;
            continue;
        }

        star->xc = intensity_x / intensity2;
        star->yc = intensity_y / intensity2;
        star->luminance = -2.5 * log10(intensity_sum - MedianNC(local_background) * num);
    }
}

static void AperturePhotometryGaussian(Image32& img, StarVector& starvector) {

    std::vector<float> local_background;

    for (auto star = starvector.begin(); star != starvector.end(); ++star) {

        int xc = star->xc;
        int yc = star->yc;
        int r = int(star->radius + 1);
        int num = 0;
        float intensity_sum = 0;

        local_background.clear();

        bool erase_star = false;

        int r_inner = 1.5 * r;
        int r_outer = r * 3.5;

        for (int y = yc - r_outer; y <= yc + r_outer; ++y) {
            for (int x = xc - r_outer; x <= xc + r_outer; ++x) {
                double dist = Distance(x, y, xc, yc);
                if (img.IsInBounds(x, y) && (r_inner <= dist && dist <= r_outer))
                    local_background.push_back(img(x, y));
            }
        }

        float b = MedianNC(local_background);
        float threshold = b + MADNC(local_background, b);

        int size = (2 * r + 1) * (2 * r + 1);

        Eigen::MatrixXd points(size, 5);
        Eigen::VectorXd d_lum(size);
        float A = img(xc, yc);
        float logA = log(A);

        for (int y = yc - r, el = 0; y <= yc + r; ++y) {
            for (int x = xc - r; x <= xc + r; ++x) {
                if (img.IsInBounds(x, y)) {
                    if (img(x, y) > threshold) {
                        points.block(el, 0, 1, 5) << 1, x, y, x* x, y* y;
                        d_lum[el++] = logA - log(img(x, y));
                        num++;
                    }
                }
                else erase_star = true;
            }
        }

        if (erase_star || num < 10) {
            starvector.erase(star);
            star--;
            continue;
        }

        points.conservativeResize(num, 5);
        d_lum.conservativeResize(num);

        Eigen::VectorXd c(5);

        //c = points.completeOrthogonalDecomposition().solve(d_lum);

        c = (points.transpose() * points).inverse() * (points.transpose() * d_lum);

        star->xc = -c[1] / (2 * c[3]);
        star->yc = -c[2] / (2 * c[4]);

        float stdx = 1 / sqrtf(2 * c[3]);
        float stdy = 1 / sqrtf(2 * c[4]);

        //2.355*stdx
        r = Max(3 * stdx, 3 * stdy);
        size = (2 * r + 1) * (2 * r + 1);

        stdx *= (2 * stdx);
        stdy *= (2 * stdy);

        float newA = A * expf(pow(xc - star->xc, 2) / stdx + pow(yc - star->yc, 2) / stdy);
        A = (newA > 1) ? 1 : newA;
        logA = log(A);

        points = Eigen::MatrixXd(size, 5);
        d_lum = Eigen::VectorXd(size);

        num = 0;

        for (int y = star->yc - r, el = 0; y <= star->yc + r; ++y) {
            float dy = y - star->yc;
            dy *= dy;
            for (int x = star->xc - r; x <= star->xc + r; ++x) {
                if (img.IsInBounds(x, y)) {

                    if (img(x, y) > threshold) {

                        float dx = x - star->xc;
                        float S = A * expf(-((dx * dx) / stdx) - dy / stdy);

                        if (abs(S / (img(x, y) - S)) > 3) {
                            points.block(el, 0, 1, 5) << 1, x, y, x* x, y* y;
                            d_lum[el++] = logA - log(img(x, y));
                            intensity_sum += img(x, y);
                            num++;
                        }
                    }
                }
                else erase_star = true;
            }
        }

        if (erase_star || num < 10) {
            starvector.erase(star);
            star--;
            continue;
        }

        points.conservativeResize(num, 5);
        d_lum.conservativeResize(num);

        //c = points.completeOrthogonalDecomposition().solve(d_lum);
        c = (points.transpose() * points).inverse() * (points.transpose() * d_lum);

        star->xc = -c[1] / (2 * c[3]);
        star->yc = -c[2] / (2 * c[4]);


        star->luminance = -2.5 * log10(intensity_sum - (b * num));

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
