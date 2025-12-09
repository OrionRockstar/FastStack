#include "pch.h"
#include "FastStack.h"
#include "AutomaticBackgroundExtraction.h"
#include "ImageWindow.h"

using ABE = AutomaticBackgroundExtraction;

std::vector<double> ABE::polynomial(int x, uint32_t degrees) {

    std::vector<double> poly(degrees + 1);
    poly[0] = 1;

    for (int i = 1, j = 0; i <poly.size(); ++i, ++j)
        poly[i] = poly[j] * x;
    
    return std::move(poly);
}

uint32_t ABE::computePolynomialLength(uint32_t degrees) {

    uint32_t poly_length = 0;
    for (int i = 1; i <= degrees + 1; ++i)
        poly_length += i;

    return poly_length;
}

template<typename T>
float ABE::sampleMedian(const Image<T>& img, const ImagePoint& p)const {

    std::vector<T> sample(size());
    size_t memsize = dimension() * sizeof(T);
    int x = p.x() - radius();
    int y = p.y() - radius();

    for (int i = 0; i < dimension(); ++i)
        memcpy(&sample[i * dimension()], &img(x, y + i, p.channel()), memsize);

    std::nth_element(sample.begin(), sample.begin() + sample.size() / 2, sample.end());

    return Pixel<float>::toType(sample[sample.size() / 2]);
}

void ABE::insertMatrixRow(Matrix& matrix, int row, const Point& p)const {

    std::vector<double> xv = polynomial(p.x, polynomialDegree());
    std::vector<double> yv = polynomial(p.y, polynomialDegree());

    for (int j = 0, col = 0; j <= polynomialDegree(); ++j)
        for (int i = 0; i <= polynomialDegree() - j; ++i)
            matrix(row, col++) = xv[i] * yv[j];
}

double ABE::computePolynomial(const Matrix& coefficients, const Point& p)const {

    std::vector<double> xv = polynomial(p.x, polynomialDegree());
    std::vector<double> yv = polynomial(p.y, polynomialDegree());

    double sum = 0.0;

    for (int j = 0, el = 0; j <= polynomialDegree(); ++j)
        for (int i = 0; i <= polynomialDegree() - j; ++i)
            sum += xv[i] * yv[j] * coefficients[el++];
       
    return sum;
}

template<typename T>
void ABE::drawSamples(Image<T>& img)const {

    int number_of_samples = ((img.rows() / distance()) - 1) * ((img.cols() / distance()) - 1);

    for (uint32_t ch = 0; ch < img.channels(); ++ch) {

        int row = 0;
        int buffer = radius() + 1;

        for (int y = buffer; y < img.rows() - buffer; y += distance()) {
            for (int x = buffer; x < img.cols() - buffer; x += distance()) {

                for (int j = -radius(); j <= radius(); ++j)
                    for (int i = -radius(); i <= radius(); ++i)
                        if (abs(j) == radius() || abs(i) == radius())
                            img(x + i, y + j, ch) = Pixel<T>::max();

            }
        }
    }
}

template<typename T>
Image32 ABE::createBackgroundModel(const Image<T>& src)const {

    Image32 background(src.rows(), src.cols(), src.channels());
    int number_of_samples = (src.rows() / distance()) * (src.cols() / distance());

    for (uint32_t ch = 0; ch < src.channels(); ++ch) {

        T med = src.computeMedian(ch);
        float median = Pixel<float>::toType(med);

        float sigma = Pixel<float>::toType(src.computeAvgDev(ch, med));//src.ComputeStdDev(ch);

        float upper = median + sigmaKUpper() * sigma;
        float lower = median - sigmaKLower() * sigma;

        Matrix variables(number_of_samples, polynomialLength());
        Matrix bgv(number_of_samples);

        int row = 0;
        int buffer = radius() + 1;

        for (int y = buffer; y < src.rows() - buffer; y += distance()) {
            for (int x = buffer; x < src.cols() - buffer; x += distance()) {

                float val = sampleMedian(src, { x, y, ch });

                if (val > upper || val < lower) continue;

                insertMatrixRow(variables, row, { x, y });
                bgv[row++] = val;
            }
        }

        row -= 1;

        variables.resize(row, polynomialLength());
        bgv.resize(row);

        Matrix coef(polynomialLength());
        coef = Matrix::leastSquares(variables, bgv);

#pragma omp parallel for
        for (int y = 0; y < src.rows(); ++y)
            for (int x = 0; x < src.cols(); ++x)
                background(x, y, ch) = computePolynomial(coef, { x, y });

    }

    return background;
}

template<typename T>
void ABE::apply(Image<T>& img) {

    Image32 background = createBackgroundModel(img);

    Image32 n_copy(img.rows(), img.cols(), img.channels());

    for (int i = 0; i < img.totalPxCount(); ++i)
        n_copy[i] = Pixel<float>::toType(img[i]);

    using enum Correction;
    switch (m_correction) {

    case subtraction:
        n_copy -= background;
        break;

    case division:
        n_copy /= background;
        break;
    }

    n_copy.normalize();

    for (int i = 0; i < img.totalPxCount(); ++i)
        img[i] = Pixel<T>::toType(n_copy[i]);
}
template void ABE::apply(Image8&);
template void ABE::apply(Image16&);
template void ABE::apply(Image32&);

template<typename T>
void ABE::applyTo(const Image<T>& src, Image<T>& dst, float scale_factor) {

    Image32 background = createBackgroundModel(src);

    Image32 n_copy(src.rows(), src.cols(), src.channels());
    for (int i = 0; i < src.totalPxCount(); ++i)
        n_copy[i] = Pixel<float>::toType(src[i]);

    using enum Correction;
    switch (m_correction) {

    case subtraction:
        n_copy -= background;
        break;

    case division:
        n_copy /= background;
        break;
    }

    n_copy.normalize();

    auto _s = 1 / scale_factor;

    if (dst.cols() != uint32_t(src.cols() * scale_factor) || dst.rows() != uint32_t(src.rows() * scale_factor) || dst.channels() != src.channels())
        dst = Image<T>(src.rows() * scale_factor, src.cols() * scale_factor, src.channels());

    for (int ch = 0; ch < dst.channels(); ++ch) {
        for (int y = 0; y < dst.rows(); ++y) {
            int y_s = y * _s + 0.5;
            for (int x = 0; x < dst.cols(); ++x) {
                int x_s = x * _s + 0.5;

                dst(x, y, ch) = Pixel<T>::toType(n_copy.at(x_s, y_s, ch));
            }
        }
    }
}
template void ABE::applyTo(const Image8&, Image8&, float);
template void ABE::applyTo(const Image16&, Image16&, float);
template void ABE::applyTo(const Image32&, Image32&, float);