#include "pch.h"
#include "StarDetector.h"
#include "Maths.h"
#include "RGBColorSpace.h"
#include "MorphologicalTransformation.h"
#include "FITS.h"


void StarDetector::addNewStar(int x, int y, int r, StarVector& star_vector) {

    for (struct Star& star : star_vector) {
        if (math::distance(x, y, star.xc, star.yc) <= math::max<int>(r, star.radius)) {
            //star.xc = .5 * (star.xc + x);
            //star.yc = .5 * (star.yc + y);
            star.radius = math::max<int>(star.radius, r); //(star.rad + (r * ix2min_radius))
            return;
        }
    }

    star_vector.emplace_back(x, y, r);
}

StarVector StarDetector::combineStarVectors(std::vector<StarVector>& svv) {

    std::array<float, 6> mr = { 1,1,0.5,0.25,0.125,0.0625 };
    StarVector starvector;
    starvector.reserve(2500);

    for (auto& s : svv[0])
        starvector.emplace_back(s);

    for (int el = 1; el < svv.size(); ++el) {
        for (int i = 0; i < svv[el].size(); ++i) {
            bool new_star = true;
            for (auto& s : starvector) {
                if (math::distance(svv[el][i].xc, svv[el][i].yc, s.xc, s.yc) <= s.radius) {
                    s.radius = math::max<int>(s.radius,svv[el][i].radius * mr[el] + 0.5);
                    new_star = false;
                    break;
                }
            }
            if (new_star)
                starvector.emplace_back(svv[el][i]);
        }
    }

    svv.clear();
    svv.shrink_to_fit();

    return starvector;
}


template<typename T>
bool StarDetector::centerGravityFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t) {

    int pix_count = 0;

    double intensity_x = 0,
        intensity_y = 0,
        intensity2 = 0,
        flux = 0;

    double sx = 0, sy = 0;

    double _y = star.yc - star.radius;
    double _x = star.xc - star.radius;

    for (double y = star.yc - star.radius; y <= star.yc + star.radius; ++y) {
        for (double x = star.xc - star.radius; x <= star.xc + star.radius; ++x) {

            if (img.isInBounds(x, y)) {
                if (img(x, y) > t) {
                    float pixel = Pixel<float>::toType(T(img(x, y) - b));
                    double i2 = pixel * pixel;
                    intensity_x += i2 * x;
                    intensity_y += i2 * y;
                    intensity2 += i2;
                    flux += pixel;
                    pix_count++;
                }
            }
            else return false;
        }
    }

    if (pix_count < 5)
        return false;

    float xc = intensity_x / intensity2;
    float yc = intensity_y / intensity2;


    for (double y = star.yc - star.radius; y <= star.yc + star.radius; ++y) {
        for (double x = star.xc - star.radius; x <= star.xc + star.radius; ++x) {

            if (img(x, y) > t) {
                float pixel = Pixel<float>::toType(T(img(x, y) - b));
                sx += (y - yc) * (y - yc) * pixel;
                sy += (x - xc) * (x - xc) * pixel;
            }
        }
    }

    //std::cout << img(xc, yc) << "\n";
    psf = PSF(Pixel<float>::toType(img(xc, yc)), xc, yc, sqrt(sx), sqrt(sy), flux, b);

    if (psf.roundness <= m_roundness)
        return false;

    return true;
}

template<typename T>
bool StarDetector::gaussianFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t) {

    float logA = logf(Pixel<float>::toType(img(star.xc, star.yc)));
    double flux = 0;
    int size = (2 * star.radius + 1) * (2 * star.radius + 1);

    Matrix points(size, 5);
    Matrix d_lum(size);

    int pix_count = 0;

    for (double y = star.yc - star.radius, row = 0; y <= star.yc + star.radius; ++y) {
        for (double x = star.xc - star.radius; x <= star.xc + star.radius; ++x) {

            if (img.isInBounds(x, y)) {
                if (img(x, y) > t) {
                    float pixel = Pixel<float>::toType(T(img(x, y) - b));
                    points.setRow(row, { 1.0, x, y, x * x, y * y });
                    d_lum[row++] = logA - logf(pixel);
                    flux += pixel;
                    pix_count++;
                }
            }

            else return false;
        }
    }

    if (pix_count < 5)
        return false;

    points.resize(pix_count, 5);
    d_lum.resize(pix_count);

    Matrix c = Matrix::leastSquares(points, d_lum);

    if (std::isnan(c[0]))
        return false;

    if (c[3] <= 0.0 || c[4] <= 0.0)
        return false;

    psf = PSF(c, flux, b);

    if (std::isinf(psf.A) || !img.isInBounds(psf.xc, psf.yc))
        return false;

    if (psf.roundness <= m_roundness)
        return false;

    return true;
}

template<typename T>
bool StarDetector::moffatFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t) {

    const float i_beta = 1 / -m_beta;

    float A = img(star.xc, star.yc);
    double flux = 0;
    int size = (2 * star.radius + 1) * (2 * star.radius + 1);

    Matrix points(size, 5);
    Matrix d_lum(size);

    int pix_count = 0;

    for (double y = star.yc - star.radius, row = 0; y <= star.yc + star.radius; ++y) {
        for (double x = star.xc - star.radius; x <= star.xc + star.radius; ++x) {
            if (img.isInBounds(x, y)) {
                if (img(x, y) > t) {
                    T pixel = img(x, y) - b;
                    points.setRow(row, { 1.0, x, y, x * x, y * y });
                    d_lum[row++] = pow(pixel / A, i_beta) - 1;
                    flux += pixel;
                    pix_count++;
                }
            }
            else return false;
        }
    }

    if (pix_count < 5)
        return false;

    points.resize(pix_count, 5);
    d_lum.resize(pix_count);

    Matrix c = Matrix::leastSquares(points, d_lum);

    if (std::isnan(c[0]))
        return false;

    if (c[3] <= 0.0 || c[4] <= 0.0)
        return false;

    psf = PSF(c, flux, b,PSF::Type::moffat, m_beta);

    if (psf.roundness <= 0.5)
        return false;

    return true;
}

template<typename T>
bool StarDetector::psfFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t) {

    switch (m_psf_type) {
    case PSF::Type::gaussian:
        return gaussianFit(img, star, psf, b, t);
    case PSF::Type::moffat:
        return moffatFit(img, star, psf, b, t);

    //case PSF::Type::gaussian:
        //return centerGravityFit(img, star, psf, b, t);

    default:
        return false;
    }
}

/*template<typename T>
void StarDetector::aperturePhotometry(const Image<T>& img, StarVector& star_vector) {

    std::vector<T> local_background;

    PSF average_psf;

    for (auto star = star_vector.begin(); star != star_vector.end(); ) {
        float xc = star->xc;
        float yc = star->yc;
        int rad = star->radius;

        local_background.clear();

        int r_inner = 1.5 * rad;
        int r_outer = 3 * rad;

        for (int y = yc - r_outer; y <= yc + r_outer; ++y) {
            for (int x = xc - r_outer; x <= xc + r_outer; ++x) {
                double dist = Distance(x, y, star->xc, star->yc);
                if (img.isInBounds(x, y) && (r_inner <= dist && dist <= r_outer))
                    local_background.push_back(img(x, y));
            }
        }

        T b = Median_nocopy(local_background);
        T t = b + 2 * AvgDev(local_background, b);//m_K * StandardDeviation(local_background);

        PSF psf;

        if (!psfFit(img, *star, psf, b, t)) {
            star = star_vector.erase(star);
            if (star == star_vector.end())
                break;
            continue;
        }


        average_psf += psf;

        star->xc = psf.xc;
        star->yc = psf.yc;
        star->radius = (psf.fwhmx + psf.fwhmy) / 2;
        star->luminance = -2.5 * log10(psf.flux);
        star++;
    }

    //average_psf.A /= star_vector.size();
    //
}
template void StarDetector::aperturePhotometry(const Image8&, StarVector&);
template void StarDetector::aperturePhotometry(const Image16&, StarVector&);
template void StarDetector::aperturePhotometry(const Image32&, StarVector&);*/

template<typename T>
StarPSFVector StarDetector::aperturePhotometry(const Image<T>& img, StarVector& star_vector) {

    std::vector<T> local_background;

    StarPSFVector stars;
    stars.reserve(star_vector.size());

    for (auto star = star_vector.begin(); star != star_vector.end(); ++star) {
        float xc = star->xc;
        float yc = star->yc;
        int rad = star->radius;

        local_background.clear();

        int r_inner = 1.5 * rad;
        int r_outer = 3 * rad;

        for (int y = yc - r_outer; y <= yc + r_outer; ++y) {
            for (int x = xc - r_outer; x <= xc + r_outer; ++x) {
                double dist = math::distance(x, y, star->xc, star->yc);
                if (img.isInBounds(x, y) && (r_inner <= dist && dist <= r_outer))
                    local_background.push_back(img(x, y));
            }
        }

        T b = math::median(local_background);
        T t = b + math::avgDev(local_background, b);
        //T t = b + 2 * math::avgDev(local_background, b);//m_K * StandardDeviation(local_background);

        PSF psf;

        if (!psfFit(img, *star, psf, b, t)) {
            star = star_vector.erase(star);
            if (star == star_vector.end())
                break;
            continue;
        }

        star->xc = psf.xc;
        star->yc = psf.yc;
        star->radius = (psf.fwhmx + psf.fwhmy) / 2;
        star->luminance = -2.5 * log10(psf.flux);

        stars.push_back({ psf,*star });
    }

    stars.shrink_to_fit();
    return stars;
}
template StarPSFVector StarDetector::aperturePhotometry(const Image8&, StarVector&);
template StarPSFVector StarDetector::aperturePhotometry(const Image16&, StarVector&);
template StarPSFVector StarDetector::aperturePhotometry(const Image32&, StarVector&);

void StarDetector::toStructureEdgeMap(Image8& src) {

    for (int y = 1; y < src.rows() - 1; ++y)
        for (int x = 1; x < src.cols() - 1; ++x)
            if (src(x, y) == 1)
                if (src(x - 1, y) != 0 && src(x + 1, y) != 0 && src(x, y - 1) != 0 && src(x, y + 1) != 0) src(x, y) = 2;
}

PSF StarDetector::computeMeanPSF(const StarPSFVector& stars) {

    PSF mean;
    for (const StarPSF& star : stars) {
        mean.B += star.psf.B;
        mean.A += star.psf.A;
        mean.sx += star.psf.sx;
        mean.sy += star.psf.sy;
        mean.fwhmx += star.psf.fwhmx;
        mean.fwhmy += star.psf.fwhmy;
        mean.roundness += star.psf.roundness;
    }

    mean.B /= stars.size();
    mean.A /= stars.size();
    mean.sx /= stars.size();
    mean.sy /= stars.size();
    mean.fwhmx /= stars.size();
    mean.fwhmy /= stars.size();
    mean.roundness /= stars.size();

    return mean;
}

template<typename T>
StarVector StarDetector::applyStarDetection(const Image<T>& img) {

    Image8Vector wavelet_vector = m_maps.generateMaps(img);

    Image<T> gray = Image<T>(img);
    gray.RGBtoGray();

    T median = gray.computeMedian(0);
    T t = median + m_K * gray.computeAvgDev(0);//gray.computeStdDev(0);

    std::vector<StarVector> svv(wavelet_vector.size());

    const std::array<int, 6> min_rad = { 1,2,4,8,16,32 };

#pragma omp parallel for num_threads(wavelet_vector.size()) schedule(static) 
    for (int el = 0; el < wavelet_vector.size(); ++el) {

        Image8* map = &wavelet_vector[el];
        toStructureEdgeMap(*map);

        StarVector sv;
        sv.reserve(1'000);

        for (int y = 0; y < gray.rows(); ++y) {
            for (int x = 0; x < gray.cols(); ++x) {

                if ((*map)(x, y) == 2) {

                    for (int r = min_rad[el]; r <= m_max_radius; ++r) {

                        int vote = 0, spacev = 0, istarv = 0;

                        for (auto tf : m_trigvector) {

                            if (istarv > 6)  break;

                            int a = int(round(x + r * tf.m_cos)); //x
                            int b = int(round(y + r * tf.m_sin)); //y 

                            if (gray.isInBounds(a, b)) {

                                if ((*map)(a, b) == 2)  istarv++;

                                else if (((*map)(a, b) == 1) && m_peak_edge * gray(x, y) > gray(a, b) && gray(a,b) > t) vote++;

                                else  spacev++;
                            }

                            else
                                goto newstar;
                        }

                        if (vote >= 6)
                            addNewStar(x, y, r, sv);

                        else if (spacev > 6)  break;
                    }

                    newstar : 0;
                }
            }
        }

        svv[el] = std::move(sv);
    }

    wavelet_vector.clear();
    StarVector star_vector = combineStarVectors(svv);

    StarPSFVector stars = aperturePhotometry(gray, star_vector);
    m_average_psf = computeMeanPSF(stars);

    std::sort(star_vector.begin(), star_vector.end(), Star());
    
    return star_vector;
}
template StarVector StarDetector::applyStarDetection(const Image8&);
template StarVector StarDetector::applyStarDetection(const Image16&);
template StarVector StarDetector::applyStarDetection(const Image32&);