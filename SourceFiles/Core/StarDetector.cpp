#include "pch.h"
#include "StarDetector.h"
#include "Maths.h"
#include "RGBColorSpace.h"
#include "MorphologicalTransformation.h"
#include "FITS.h"
#include "Histogram.h"


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
                //bug!! stars at larger scales not added dur to "radaii" overlapping other stars
                //same star on different scales!?!?
                if (math::distance(svv[el][i].xc, svv[el][i].yc, s.xc, s.yc) <= s.avgRadius()) {
                    //s.radius = math::min<int>(s.radius,svv[el][i].radius * mr[el] + 0.5);
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
T localBackground(const Image<T>& img, const Star& star, int im = 3, int om = 5) {

    int xc = star.xc;
    int yc = star.yc;
    int rad = star.avgRadius();//(star.radius_x + star.radius_y) / 2.0;
    int r_inner = im * rad;//orig 1.5 or 2
    int r_outer = om * rad;//orig 3 or 4

    std::vector<T> local_background;
    local_background.reserve((std::_Pi * r_outer * r_outer) - (std::_Pi * r_inner * r_inner));

    for (int y = yc - r_outer; y <= yc + r_outer; ++y) {
        for (int x = xc - r_outer; x <= xc + r_outer; ++x) {
            if (img.isInBounds(x, y)) {
                float dist = math::distancef(x, y, star.xc, star.yc);
                if (r_inner < dist && dist < r_outer)
                    local_background.emplace_back(img(x, y));
            }
        }
    }

    local_background.shrink_to_fit();
   return math::median(local_background);
}

template<typename T>
bool StarDetector::centerGravityFit(const Image<T>& img, const Star& star, PSF& psf, T b, T t) {

    int pix_count = 0;

    double intensity_x = 0,
        intensity_y = 0,
        intensity2 = 0,
        flux = 0;

    double sx = 0, sy = 0;

    for (double y = star.yc - star.radius_y; y <= star.yc + star.radius_y; ++y) {
        for (double x = star.xc - star.radius_x; x <= star.xc + star.radius_x; ++x) {

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


    for (double y = star.yc - star.radius_y; y <= star.yc + star.radius_y; ++y) {
        for (double x = star.xc - star.radius_x; x <= star.xc + star.radius_x; ++x) {

            if (img(x, y) > t) {
                float pixel = Pixel<float>::toType(T(img(x, y) - b));
                sx += (y - yc) * (y - yc) * pixel;
                sy += (x - xc) * (x - xc) * pixel;
            }
        }
    }

    psf = PSF(Pixel<float>::toType(img(xc, yc)), xc, yc, sqrt(sx), sqrt(sy), flux, b);

    if (psf.roundness <= m_roundness)
        return false;

    return true;
}

template<typename T>
bool StarDetector::gaussianFit(const Image<T>& img, const Star& star, PSF& psf, T b) {

    float logA = logf(Pixel<float>::toType(img(star.xc, star.yc)));
    int size = (2 * star.radius_x + 1) * (2 * star.radius_y + 1);

    Matrix points(size, 6);
    Matrix d_lum(size);

    int pix_count = 0;

    for (double y = star.yc - star.radius_y, row = 0; y <= star.yc + star.radius_y; ++y) {
        double y2 = y * y;
        for (double x = star.xc - star.radius_x; x <= star.xc + star.radius_x; ++x) {
            if (img.isInBounds(x, y)) {
                if (img(x, y) > b) {
                    points.setRow(row, { 1.0, x, y, x * y ,x * x, y2 });
                    d_lum[row++] = logA - logf(Pixel<float>::toType(T(img(x, y) - b)));
                    pix_count++;
                }
            }
            else return false;
        }
    }

    if (pix_count < 6)
        return false;

    points.resize(pix_count, 6);
    d_lum.resize(pix_count);

    Matrix c = Matrix::leastSquares(points, d_lum);

    if (std::isnan(c[0]))
        return false;

    float e = std::numeric_limits<float>::epsilon();
    if (c[4] < e || c[5] < e)
        return false;

    psf = PSF(std::array<double, 6>({ c[0],c[1],c[2],c[3],c[4],c[5] }), img, b);

    if (std::isnan(psf.sx) || std::isnan(psf.sy))
        return false;

    if (std::isinf(psf.A) || !img.isInBounds(psf.xc, psf.yc))
        return false;

    if (Pixel<float>::toType(img(psf.xc, psf.yc)) < 1.65 * psf.B)
        return false;

    if (psf.roundness <= m_roundness)
        return false;

    return true;
}

template<typename T>
bool StarDetector::moffatFit(const Image<T>& img, const Star& star, PSF& psf, T b) {

    const float i_beta = 1 / -m_beta;

    float A = Pixel<float>::toType(img(star.xc, star.yc));
    int size = (2 * star.radius_x + 1) * (2 * star.radius_y + 1);

    Matrix points(size, 6);
    Matrix d_lum(size);

    int pix_count = 0;

    for (double y = star.yc - star.radius_y, row = 0; y <= star.yc + star.radius_y; ++y) {
        for (double x = star.xc - star.radius_x; x <= star.xc + star.radius_x; ++x) {
            if (img.isInBounds(x, y)) {
                if (img(x, y) > b) {
                    points.setRow(row, { 1.0, x, y, x * y, x * x, y * y });
                    d_lum[row++] = pow(Pixel<float>::toType(T(img(x, y) - b)) / A, i_beta) - 1;
                    pix_count++;
                }
            }
            else return false;
        }
    }

    if (pix_count < 6)
        return false;

    points.resize(pix_count, 6);
    d_lum.resize(pix_count);

    Matrix c = Matrix::leastSquares(points, d_lum);

    if (std::isnan(c[0]))
        return false;

    float e = std::numeric_limits<float>::epsilon();
    if (c[4] < e || c[5] < e)
        return false;

    psf = PSF(std::array<double, 6>({ c[0],c[1],c[2],c[3],c[4],c[5] }), img, b, PSF::Type::moffat, m_beta);

    if (std::isnan(psf.sx) || std::isnan(psf.sy))
        return false;

    if (std::isinf(psf.A) || !img.isInBounds(psf.xc, psf.yc))
        return false;

    if (Pixel<float>::toType(img(psf.xc, psf.yc)) < 1.65 * psf.B)
        return false;

    if (psf.roundness <= m_roundness)
        return false;

    return true;
}

template<typename T>
bool StarDetector::psfFit(const Image<T>& img, Star& star, PSF& psf) {

    T b = 0;
    Star oldstar;
    double oldRMSE = 0;

    for (int i = 0; i < 5; ++i) {
        //needs to be outside star so first star needs to have sufficent radius
        if (i == 0 || (abs(int(star.xc) - int(oldstar.xc)) > 1 || abs(int(star.yc) - int(oldstar.yc)) > 1 || abs(star.avgRadius() - oldstar.avgRadius()) > 1))
            b = localBackground(img, star);

        if (i == 0)
            star.radius_x = star.radius_y = 2;

        switch (m_psf_type) {
        case PSF::Type::gaussian:
            if (!gaussianFit(img, star, psf, b))
                return false;
            break;
        case PSF::Type::moffat:
            if (!moffatFit(img, star, psf, b))
                return false;
            break;
        default:
            return false;
        }

        float RMSE = psf.rmse(img);

        if (std::isnan(RMSE) || std::isinf(RMSE))
            return false;

        if (i < 2 || RMSE < oldRMSE) {
            star = { psf.xc,psf.yc, int(psf.fwhmx), int(psf.fwhmy) };
            star.luminance = psf.luminance();
        }

        oldstar = star;
        oldRMSE = RMSE;
    }

    return true;
}

void toStructureEdgeMap(Image8& src) {

    for (int y = 1; y < src.rows() - 1; ++y)
        for (int x = 1; x < src.cols() - 1; ++x)
            if (src(x, y) == 1)
                if (src(x - 1, y) != 0 && src(x + 1, y) != 0 && src(x, y - 1) != 0 && src(x, y + 1) != 0) src(x, y) = 2;
}

PSF computeMeanPSF(const PSFVector& stars) {

    PSF mean;
    for (const PSF& psf : stars) {
        mean.B += psf.B;
        mean.A += psf.A;
        mean.sx += psf.sx;
        mean.sy += psf.sy;
        mean.fwhmx += psf.fwhmx;
        mean.fwhmy += psf.fwhmy;
        mean.roundness += psf.roundness;
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
T localMean(const Image<T>& img, const Star& star, float im = 3, float om = 5) {

    int xc = star.xc;
    int yc = star.yc;
    const int r = math::max(star.radius_x, star.radius_y);
    const int r_inner = im * r;
    const int r_outer = om * r;

    double mean = 0;
    int c = 0;

    QRect rect(xc - r_inner, yc - r_inner, 2 * r_inner, 2 * r_inner);

    for (int j = yc - r_outer; j <= yc + r_outer; ++j) {
        for (int i = xc - r_outer; i <= xc + r_outer; ++i) {
            if (!rect.contains({ i,j }) && img.isInBounds(i, j)) {
                mean += img(i, j);
                c++;
            }
        }
    }

    return mean / c;
}

template<typename T>
StarVector StarDetector::DAOFIND(const Image<T>& img) {

    StarVector star_vector;
    PSFVector psf_vector;

    star_vector.reserve(2'000);
    psf_vector.reserve(2'000);

    Image<T> gray = Image<T>(img);
    gray.RGBtoGray();

    //to remove hot pixels
    MorphologicalTransformation mt(MorphologicalTransformation::Type::median);
    mt.apply(gray);

    GaussianFilter gf;
    gf.setSigma(m_sigma);
    gf.apply(gray);

    T median = gray.computeMedian(0);
    T t = median + m_K * gray.computeStdDev(0);

    int sr = 4;//search_radius//9px diameter
    int star_rad = 3;//may need to increase to 4/orig 3

    QPoint old_max_pos;

    for (int y = 0; y < gray.rows(); ++y) {

        for (int x = 0; x < gray.cols(); ++x) {

            if (gray(x, y) > t) {

                T max = t;
                QPoint max_pos(x, y);
                Star star(x, y, star_rad);

                for (int j = y - sr; j <= y + sr; ++j) {
                    for (int i = x - sr; i <= x + sr; ++i) {
                        if (gray.isInBounds(i, j)) {
                            if (gray(i, j) > max) {
                                max = gray(i, j);
                                max_pos = { i, j };
                                star = Star(i, j, star_rad);
                            }
                        }
                        else
                            goto newstar;
                    }
                }

                if (max_pos == old_max_pos)
                    goto newstar;

                old_max_pos = max_pos;

                for (const auto& s : star_vector)
                    if (math::distancef(s.xc, s.yc, star.xc, star.yc) < s.avgRadius())
                        goto newstar;

                //ensures max is suffeciently above background, //no apparent benifit to radius being 4 when computing mean             
                //majority of time spent here
                if (Pixel<float>::toType(max) < 1.65 * Pixel<float>::toType(localMean(gray, star)))
                    goto newstar;

                PSF psf;
                if (!psfFit(img, star, psf))
                    goto newstar;

                int ar = star.avgRadius();
                for (const auto& s : star_vector)
                    if (math::distancef(s.xc, s.yc, star.xc, star.yc) < math::max(s.avgRadius(), ar))
                        goto newstar;

                //needs to be atomic if multithreaded
                //no adding while searching through vec
                star_vector.emplace_back(star);
                psf_vector.emplace_back(psf);
            }
            newstar : 0;
        }
    }

    m_average_psf = computeMeanPSF(psf_vector);
    std::sort(star_vector.begin(), star_vector.end(), Star());
    
    return star_vector;
}
template StarVector StarDetector::DAOFIND(const Image8&);
template StarVector StarDetector::DAOFIND(const Image16&);
template StarVector StarDetector::DAOFIND(const Image32&);
