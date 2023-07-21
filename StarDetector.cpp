#include "StarDetector.h"
#include "Maths.h"
#include <array>
#include "Matrix.h"
#include "Wavelet.h"

bool StarDetector::IsNewStar(int x, int y, int r, std::vector<Star>& star_vector) {

    for (struct Star& star : star_vector) {
        if (Distance(double(x), double(y), star.xc, star.yc) <= Max<double>(r, star.radius)) {
            star.xc = .5 * (star.xc + x);
            star.yc = .5 * (star.yc + y);
            star.radius = Max<double>(star.radius, r); //(star.rad + (r * ix2min_radius))

            return false;
        }
    }

    return true;
}

StarVector StarDetector::CombineStarVectors(std::vector<StarVector>& svv) {

    std::array<float, 5> ix2min_radius = { 0.5f, 0.25f, 0.125f, 0.0625f, 0.03125f };

    StarVector starvector;
    starvector.reserve(2500);

    for (auto& s : svv[0])
        starvector.emplace_back(s);

    for (int el = 1; el < svv.size(); ++el) {
        for (int i = 0; i < svv[el].size(); ++i) {
            bool new_star = true;
            for (auto& s : starvector) {
                if (Distance(svv[el][i].xc, svv[el][i].yc, s.xc, s.yc) <= s.radius) {
                    s.radius = (s.radius + (svv[el][i].radius * ix2min_radius[el]));
                    new_star = false;
                    break;
                }
            }
            if (new_star)
                starvector.emplace_back(svv[el][i].xc, svv[el][i].yc, svv[el][i].radius);
        }

    }

    return starvector;
}

void StarDetector::AperturePhotometry_WCG(const Image32& img, StarVector& star_vector) {
    std::vector<float> local_background;

    for (auto star = star_vector.begin(); star != star_vector.end(); ++star) {

        int xc = int(star->xc),
            yc = int(star->yc),
            r = int(star->radius),
            r6 = r * 4.5,
            r4 = r * 2.5,
            num = 0;

        bool erase_star = false;

        double intensity_x = 0,
            intensity_y = 0,
            intensity2 = 0,
            intensity_sum = 0;

        local_background.clear();

        for (int y = yc - r6; y <= yc + r6; ++y) {
            for (int x = xc - r6; x <= xc + r6; ++x) {
                double dist = Distance(x, y, star->xc, star->yc);
                if (img.IsInBounds(x, y) && (r4 <= dist && dist <= r6))
                    local_background.push_back(img(x, y));
            }
        }

        float b = MedianNC(local_background);
        float t = b * MADNC(local_background, b);

        for (int y = yc - r; y <= yc + r; ++y) {
            for (int x = xc - r; x <= xc + r; ++x) {
                if (img.IsInBounds(x, y)) {
                    if (img(x, y) > t) {
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
            star_vector.erase(star);
            star--;
            continue;
        }

        star->xc = intensity_x / intensity2;
        star->yc = intensity_y / intensity2;

        star->luminance = -2.5 * log10(intensity_sum - b * num);
    }
}

void StarDetector::AperturePhotometry_Gaussian(const Image32& img, StarVector& star_vector) {

    std::vector<float> local_background;

    for (auto star = star_vector.begin(); star != star_vector.end(); ++star) {
        int xc = star->xc;
        int yc = star->yc;
        int r = star->radius;
        int num = 0;

        local_background.clear();

        int r_inner = 2.5 * r;
        int r_outer = r * 4.5;

        for (int y = yc - r_outer; y <= yc + r_outer + 1; ++y) {
            for (int x = xc - r_outer; x <= xc + r_outer + 1; ++x) {
                double dist = Distance(x, y, star->xc, star->yc);
                if (img.IsInBounds(x, y) && (r_inner <= dist && dist <= r_outer))
                    local_background.push_back(img(x, y));
            }
        }

        float b = MedianNC(local_background);
        float t = b + MADNC(local_background, b);

        bool erase_star = false;

        int size = (2 * r + 1) * (2 * r + 1);

        Matrix points(size, 5);
        Matrix d_lum(size);

        float logA = log(img(star->xc, star->yc));

        float intensity_sum = 0;
        for (int y = yc - r, el = 0; y <= yc + r; ++y) {
            for (int x = xc - r; x <= xc + r; ++x) {
                if (img.IsInBounds(x, y)) {
                    if (img(x, y) > t) {
                        points.ModifyRow(el, 0, { 1, x, y, x * x, y * y });
                        d_lum[el++] = logA - log(img(x, y) - b);
                        intensity_sum += img(x, y);
                        num++;
                    }
                }
                else erase_star = true;
            }
        }

        if (erase_star || num < 5) {
            star_vector.erase(star);
            star--;
            continue;
        }

        points.MatrixResize(num, 5);
        d_lum.MatrixResize(num);

        Matrix c(5);
        c = Matrix::LeastSquares(points, d_lum);

        if (c[3] == 0 || c[4] == 0) {
            star_vector.erase(star);
            star--;
            continue;
        }


        star->xc = -c[1] / (2 * c[3]);
        star->yc = -c[2] / (2 * c[4]);

        //float stdx = 1 / sqrtf(2 * c[3]);
        //float stdy = 1 / sqrtf(2 * c[4]);

        star->luminance = -2.5 * log10(intensity_sum - (b * num));

    }
}


StarVector StarDetector::StarDetection(const Image32& img) {

    Image8Vector wavelet_vector;
    Wavelet().B3WaveletTransform_Trinerized(img, wavelet_vector, m_K, m_median_blur, m_num_of_layers);

    std::vector<std::vector<Star>> svv(wavelet_vector.size());
    for (auto& s : svv)
        s.reserve(1000);

    std::array<int, 5> min_rad = { 1,2,4,8,16 };

#pragma omp parallel for schedule(static) 
    for (int el = 0; el < wavelet_vector.size(); ++el) {

        for (int y = 0; y < img.Rows(); ++y) {
            for (int x = 0; x < img.Cols(); ++x) {

                if (wavelet_vector[el](x, y) == 2) {

                    for (int r = min_rad[el]; r <= 32; ++r) {
                        //bool edge_star = false;

                        int vote = 0, spacev = 0, istarv = 0;

                        for (auto tf : m_trigvector) {

                            if (istarv == 6)  break;

                            int a = int(round(x + r * tf._cos)); //x
                            int b = int(round(y + r * tf._sin)); //y 

                            if (img.IsInBounds(a, b, 5)) {

                                if (wavelet_vector[el](a, b) == 2)  istarv++;

                                else if ((wavelet_vector[el](a, b) == 1) && (img(x, y) >= 1.5 * img(a, b))) vote++;

                                else  spacev++;
                            }

                            else
                                goto newstar;


                        }

                        if (vote >= 6) {
                            if (IsNewStar(x, y, r, svv[el]))
                                svv[el].emplace_back(x, y, r);
                        }

                        else if (spacev >= 6)  break;
                    }
                    newstar : 0;
                }
            }
        }
    }

    wavelet_vector.clear();
    StarVector star_vector = CombineStarVectors(svv);
    svv.clear();

    AperturePhotometry_Gaussian(img, star_vector);

    std::sort(star_vector.begin(), star_vector.end(), Star());

    return star_vector;
}