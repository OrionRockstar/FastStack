#include "pch.h"
#include "StarDetector.h"
#include "Maths.h"
#include "RGBColorSpace.h"
#include "GaussianFilter.h"

template<typename T>
T localMean(const Image<T>& img, const Star& star, bool circular, int im, int om) {

    float xc = star.xc;
    float yc = star.yc;

    double mean = 0;
    int c = 0;

    if (circular) {

        int r = star.avgRadius();
        int r_inner = im * r;
        int r_outer = om * r;

        for (int y = yc - r_outer; y <= int(yc + r_outer); ++y) {
            for (int x = xc - r_outer; x <= int(xc + r_outer); ++x) {
                if (img.isInBounds(x, y)) {
                    float dist = math::distancef(x, y, star.xc, star.yc);
                    if (r_inner <= dist && dist <= r_outer) {
                        mean += img(x, y);
                        c++;
                    }
                }
            }
        }
    }

    else {

        int rix = im * star.radius_x;
        int riy = im * star.radius_y;
        int rox = om * star.radius_x;
        int roy = om * star.radius_y;

        int ilx = 2 * rix + 1;
        int ily = 2 * riy + 1;

        QRect rect(xc - rix, yc - riy, ilx, ily);

        for (int y = yc - roy; y <= int(yc + roy); ++y) {
            for (int x = xc - rox; x <= int(xc + rox); ++x) {
                if (!rect.contains({ x,y }) && img.isInBounds(x, y)) {
                    mean += img(x, y);
                    c++;
                }
            }
        }
    }

    return mean / c;
}

template<typename T>
std::vector<T> localBackground(const Image<T>& img, const Star& star, bool circular, int im, int om = 3) {

    float xc = star.xc;
    float yc = star.yc;

    std::vector<T> local_background;

    if (circular) {

        int rad = math::max(star.radius_x, star.radius_y);//star.avgRadius();
        int r_inner = im * rad;
        int r_outer = om * rad;

        local_background.reserve((std::_Pi * r_outer * r_outer) - (std::_Pi * r_inner * r_inner));
        for (int y = yc - r_outer; y <= int(yc + r_outer); ++y) {
            for (int x = xc - r_outer; x <= int(xc + r_outer); ++x) {
                if (img.isInBounds(x, y)) {
                    float dist = math::distancef(x, y, star.xc, star.yc);
                    if (r_inner <= dist && dist <= r_outer)
                        local_background.emplace_back(img(x, y));
                }
            }
        }
    }

    else {

        int rad = star.avgRadius(); //math::max(star.radius_x, star.radius_y);//star.avgRadius();

        int rix = im * rad;
        int riy = im * rad;
        int rox = om * rad;
        int roy = om * rad;

        int ilx = 2 * rix + 1;
        int ily = 2 * riy + 1;

        local_background.reserve(((2 * rox + 1) * (2 * roy + 1)) - (ilx * ily));
        QRect rect(xc - rix, yc - riy, ilx, ily);

        for (int y = yc - roy; y <= int(yc + roy); ++y) {
            for (int x = xc - rox; x <= int(xc + rox); ++x) {
                if (!rect.contains({ x,y }) && img.isInBounds(x, y)) {
                    local_background.emplace_back(img(x, y));
                }
            }
        }
    }
    
    local_background.shrink_to_fit();
    return local_background;
}

template<typename T>
T starMean(const Image<T>& img, const Star& star) {

    int xc = star.xc;
    int yc = star.yc;
    int rx = star.radius_x;
    int ry = star.radius_y;

    double sum = 0;
    int c = 0;

    for (int y = yc - ry; y <= int(yc + ry); ++y) {
        for (int x = xc - rx; x <= int(xc + rx); ++x) {
            if (img.isInBounds(x, y)) {
                sum += img(x, y);
                c++;
            }
        }
    }

    return sum / c;
}

template<typename T>
T starStdDev(const Image<T>& img, const Star& star) {

    int xc = star.xc;
    int yc = star.yc;
    int rx = star.radius_x;
    int ry = star.radius_y;

    double sum = 0;
    int c = 0;

    for (int y = yc - ry; y <= int(yc + ry); ++y) {
        for (int x = xc - rx; x <= int(xc + rx); ++x) {
            if (img.isInBounds(x, y)) {
                sum += img(x, y);
                c++;
            }
        }
    }

    float mean = Pixel<float>::toType(T(sum / c));

    double d, var = 0;
    for (int y = yc - ry; y <= int(yc + ry); ++y) {
        for (int x = xc - rx; x <= int(xc + rx); ++x) {
            if (img.isInBounds(x, y)) {
                d = img.pixel<float>(x, y) - mean;
                var += d * d;
            }
        }
    }

    return Pixel<T>::toType(sqrt(var / c));
}



template<typename T>
bool StarDetector::gaussianFit(const Image<T>& orig, const Image<T>& conv, const Star& star, PSF& psf, T b, T t) {

    float logA = logf(Pixel<float>::toType(conv(star.xc, star.yc)));
    int rx = star.radius_x;
    int ry = star.radius_y;
    int lx = (2 * rx + 1);
    int ly = (2 * ry + 1);
    int size = lx * ly;

    Matrix points(size, 6);
    Matrix d_lum(size);

    int pix_count = 0;

    for (int j = 0, row = 0; j < ly; ++j) {
        double y = star.yc - ry + j;
        double y2 = y * y;
        for (int i = 0; i < lx; ++i) {
            double x = star.xc - rx + i;
            if (conv.isInBounds(x, y)) {
                if (conv(x, y) > t) {
                    points.setRow(row, { 1.0, x, y, x * y, x * x, y2 });
                    d_lum[row++] = logA - logf(Pixel<float>::toType(T(conv(x, y) - b)));
                    pix_count++;
                }
            }
            else return false;
        }
    }

    /*for (double y = star.yc - ry, row = 0; y <= star.yc + ry; ++y) {
        double y2 = y * y;
        for (double x = star.xc - rx; x <= star.xc + rx; ++x) {
            if (conv.isInBounds(x, y)) {
                if (conv(x, y) > t) {
                    points.setRow(row, { 1.0, x, y, x * y, x * x, y2 });
                    d_lum[row++] = logA - logf(Pixel<float>::toType(T(conv(x, y) - b)));
                    pix_count++;
                }
            }
            else return false;
        }
    }*/

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

    psf = PSF({ c[0],c[1],c[2],c[3],c[4],c[5] }, orig, b);

    return psf.isValid();
}

template<typename T>
bool StarDetector::moffatFit(const Image<T>& orig, const Image<T>& conv, const Star& star, PSF& psf, T b, T t) {

    const float i_beta = 1 / m_beta;

    float A = Pixel<float>::toType(conv(star.xc, star.yc));
    int rx = star.radius_x;
    int ry = star.radius_y;
    int lx = (2 * rx + 1);
    int ly = (2 * ry + 1);
    int size = lx * ly;

    Matrix points(size, 6);
    Matrix d_lum(size);

    int pix_count = 0;

    for (int j = 0, row = 0; j < ly; ++j) {
        double y = star.yc - ry + j;
        double y2 = y * y;
        for (int i = 0; i < lx; ++i) {
            double x = star.xc - rx + i;
            if (conv.isInBounds(x, y)) {
                if (conv(x, y) > t) {
                    points.setRow(row, { 1.0, x, y, x * y, x * x, y2 });
                    d_lum[row++] = pow(A / Pixel<float>::toType(T(conv(x, y) - b)), i_beta) - 1;
                    pix_count++;
                }
            }
            else return false;
        }
    }

    /*for (double y = star.yc - ry, row = 0; y <= star.yc + ry; ++y) {
        double y2 = y * y;
        for (double x = star.xc - rx; x <= star.xc + rx; ++x) {
            if (conv.isInBounds(x, y)) {
                if (conv(x, y) > t) {
                    points.setRow(row, { 1.0, x, y, x * y, x * x, y2 });
                    d_lum[row++] = pow(A / Pixel<float>::toType(T(conv(x, y) - b)), i_beta) - 1;
                    pix_count++;
                }
            }
            else return false;
        }
    }*/

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

    psf = PSF(std::array<double, 6>({ c[0],c[1],c[2],c[3],c[4],c[5] }), orig, b, PSF::Type::moffat, m_beta);

    return psf.isValid();
}

template<typename T>
bool StarDetector::psfFit(const Image<T>& orig, const Image<T>& conv, Star& star, PSF& psf) {

    float old_rmse = 1.0f;
    float sigma = 0;

    for (int i = 0; i < 5; ++i) {

        //orig 1 & 3
        std::vector<T> bg = localBackground(orig, star, false, 1, 3);
        if (bg.size() < 13) // int((3^2 * pi - 1^2 * pi) / 2 + 0.5)
            return false;
        T b = math::median(bg);
        T s = 1.4826 * math::mad(bg, b); 
        T t = Pixel<T>::toType(math::clip(Pixel<float>::toType(b) + Pixel<float>::toType(s)));

        //if (i < 2)
            //star.radius_x = star.radius_y = 2 + i;

        PSF _psf = psf;
        switch (m_psf_type) {
        case PSF::Type::gaussian:
            if (!gaussianFit(orig, conv, star, _psf, b, t))
                return false;
            break;
        case PSF::Type::moffat:
            if (!moffatFit(orig, conv, star, _psf, b, t))
                return false;
            break;
        default:
            return false;
        }

        //lower threshold can lead to by inital centroid guess
        //unstable fitting filtering
        float d = (i > 2) ? 1.0 : 2.0;
        if (i > 1 && (abs(star.xc - _psf.xc) > d || abs(star.yc - _psf.yc) > d))
            return false;

        if (std::isnan(_psf.rmse) || std::isinf(_psf.rmse))
            return false;

        if (_psf.rmse < old_rmse) {
            star = Star(psf = _psf);
            sigma = Pixel<float>::toType(s);
        }

        if (i > 2 && abs(old_rmse - _psf.rmse) < 0.005)
            break;

        old_rmse = _psf.rmse;
    }

    if (psf.roundness <= roundness())
        return false;

    if (math::max(star.radius_x, star.radius_y) > 40)
        return false;
    
    if (psf.rmse > 0.35)
        return false;

    //filters large, dim regions, such as nebula, that give a false "good fit"
    int v = (m_psf_type == PSF::Type::gaussian) ? 4 : 6;
    if (psf.mean() < psf.B + 3 * sigma && star.avgRadius() > v && psf.peak < 0.75 * Pixel<float>::max())
        return false;

    if (psf.peak < psf.B + sigma || psf.A < psf.B + sigma)
        return false;

    float s = psf.sharpness();
    if (!(0.0 <= s && s <= 1.0))
        return false;
    
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
void StarDetector::daofind(const Image<T>& gray, StarVector& star_vector, PSFVector& psf_vector) {

    star_vector.reserve(2'000);
    psf_vector.reserve(2'000);

    Image<T> conv = Image<T>(gray);

    T median = gray.computeMedian(0);
    T sigma = gray.computeStdDev(0);
    T t = median + m_K * sigma;

    GaussianFilter gf;
    gf.setSigma(m_sigma);
    gf.apply(conv);

    //auto tp = getTimePoint();
    Threads threads;
    threads.run([&, t, sigma](int start, int end) {

        float ks = m_K * Pixel<float>::toType(sigma);
        int sr = 3;//search_radius
        int star_rad = 3;//7px diameter
        StarVector sv;
        sv.reserve(math::max(int(star_vector.capacity() / threads.threadCount()), 200));
        Point old_max_pos;

        for (int y = start; y < end; ++y) {
            for (int x = 0; x < conv.cols(); ++x) {

                if (gray(x, y) > t) {

                    T max = t;
                    Point max_pos(x, y);
                    Star star(x, y, star_rad);

                    for (int j = y - sr; j <= y + sr; ++j) {
                        for (int i = x - sr; i <= x + sr; ++i) {
                            if (gray.isInBounds(i, j)) {
                                if (gray(i, j) > max) {
                                    max = conv(i, j);
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

                    for (const auto& s : sv)
                        if (math::distancef(s.xc, s.yc, star.xc, star.yc) < s.avgRadius())
                            goto newstar;

                    //ensures max is suffeciently above local background(incase local background is above threshold)
                    //may not need //orig smult was 3 & 5
                    if (Pixel<float>::toType(max) < math::clipf(Pixel<float>::toType(localMean(gray, star,false,2,4)) + ks))
                        goto newstar;

                    //filters out large, flat regions above t, can rule out sat/large stars 
                    if (Pixel<float>::toType(starStdDev(gray, star)) < 0.005)
                        goto newstar;

                    PSF psf;
                    if (!psfFit(gray, conv, star, psf))
                        goto newstar;

                    float ar = star.avgRadius();
                    for (const auto& s : sv)
                        if (math::distancef(s.xc, s.yc, star.xc, star.yc) < math::max(s.avgRadius(), ar))
                            goto newstar;

                    sv.emplace_back(star);

                    threads.mutex.lock();
                    star_vector.emplace_back(star);
                    psf_vector.emplace_back(psf);
                    threads.mutex.unlock();
                }
                newstar : 0;
            }
        }
        }, conv.rows());
    //displayTimeDuration(tp);

    auto erase_element = []<typename T>(std::vector<T>&v, size_t pos) {
        std::swap(v[pos], v[v.size() - 1]);
        v.pop_back();
    };

    //removes duplicate stars
    for (int i = 0; i < star_vector.size(); ++i) {
        auto& cs = star_vector[i];
        float csr = cs.avgRadius();
        for (int j = i + 1; j < star_vector.size(); ++j) {
            auto& s = star_vector[j];
            if (math::distancef(cs.xc, cs.yc, s.xc, s.yc) < math::max(s.avgRadius(), csr)) {
                erase_element(star_vector, j);
                erase_element(psf_vector, j);
                break;
            }
        }
    }

    star_vector.shrink_to_fit();
    psf_vector.shrink_to_fit();

    m_average_psf = computeMeanPSF(psf_vector);
}

template<typename T>
StarVector StarDetector::DAOFIND(const Image<T>& img) {

    StarVector star_vector;
    PSFVector psf_vector;

    if (img.channels() == 3)
        daofind(img.createGrayscaleImage(), star_vector, psf_vector);
    else
        daofind(img, star_vector, psf_vector);

    std::sort(star_vector.begin(), star_vector.end(), Star());
    return star_vector;
}
template StarVector StarDetector::DAOFIND(const Image8&);
template StarVector StarDetector::DAOFIND(const Image16&);
template StarVector StarDetector::DAOFIND(const Image32&);

template<typename T>
PSFVector StarDetector::DAOFIND_PSF(const Image<T>& img) {

    StarVector star_vector;
    PSFVector psf_vector;

    if (img.channels() == 3) 
        daofind(img.createGrayscaleImage(), star_vector, psf_vector);
    else
        daofind(img, star_vector, psf_vector);

    std::sort(psf_vector.begin(), psf_vector.end(), PSF());
    return psf_vector;
}
template PSFVector StarDetector::DAOFIND_PSF(const Image8&);
template PSFVector StarDetector::DAOFIND_PSF(const Image16&);
template PSFVector StarDetector::DAOFIND_PSF(const Image32&);
