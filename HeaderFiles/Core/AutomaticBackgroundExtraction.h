#pragma once
#include "Matrix.h"
#include "Image.h"


class AutomaticBackgroundExtraction {
public:
    enum class Correction {
        subtraction,
        division
    };

private:

    template<typename T>
    class Sample : std::vector<T> {
        uint32_t m_dimension = 0;
        uint32_t m_radius = 0;
        
    public:
        using std::vector<T>::operator[];

        using std::vector<T>::begin;

        using std::vector<T>::end;

        using std::vector<T>::size;

        Sample(uint32_t radius) : m_radius(radius), m_dimension(2*radius+1) {
            (*this).resize(m_dimension * m_dimension);
        }

        uint32_t dimension()const { return m_dimension; }

        uint32_t radius()const { return m_radius; }

        void populate(const Image<T>& img, const ImagePoint& p) {

            size_t memsize = dimension() * sizeof(T);
            int x = p.x() - radius();
            int y = p.y() - radius();

            for (int i = 0; i < dimension(); ++i)
                memcpy(&(*this)[i * dimension()], &img(x, y + i, p.channel()), memsize);
        }

        float median() {
            auto mid = size() / 2;
            std::nth_element(begin(), begin() + mid, end());
            return Pixel<float>::toType((*this)[mid]);
        }
    };


    uint32_t m_radius = 5;
    uint32_t m_seperation = 5;

    float m_uK = 0.8;
    float m_lK = 1.8;

    int m_dim = 2 * m_radius + 1;
    int m_size = m_dim * m_dim;
    uint32_t m_dist = m_dim + m_seperation; //distance between sample bary-center

    int m_poly_degree = 4;
    int m_poly_length = computePolynomialLength(m_poly_degree);


    Correction m_correction = Correction::subtraction;

    static std::vector<double> polynomial(int x, uint32_t degrees = 4);

    static uint32_t computePolynomialLength(uint32_t degrees = 4);


    uint32_t radius()const noexcept { return m_radius; }

    uint32_t dimension()const noexcept { return m_dim; }

    uint32_t size()const noexcept { return m_size; }

    uint32_t distance()const noexcept { return m_dist; }

public:
    AutomaticBackgroundExtraction() = default;

    int sampleRadius()const { return m_radius; }

    void setSampleRadius(int radius) {
        m_radius = radius;
        m_dim = 2 * m_radius + 1;
        m_size = m_dim * m_dim;
        m_dist = m_dim + m_seperation;
    }

    int sampleSeperation()const { return m_seperation; }

    void setSampleSeperation(int seperation) {
        m_seperation = seperation;
        m_dist = m_dim + m_seperation;
    }

    float sigmaKLower()const { return m_lK; }

    void setSigmaKLower(float K) { m_lK = K; }

    float sigmaKUpper()const { return m_uK; }

    void setSigmaKUpper(float K) { m_uK = K; }

    int polynomialDegree()const { return m_poly_degree; }

    void setPolynomialDegree(int degree) { 
        m_poly_degree = degree;
        m_poly_length = computePolynomialLength(degree);
    }

    void setCorrectionMethod(Correction method) { m_correction = method; }

private:
    int polynomialLength()const { return m_poly_length; }

    template<typename T>
    float sampleMedian(const Image<T>& img, const ImagePoint& p)const;

    void insertMatrixRow(Matrix& matrix, int row, const Point& p)const;

    double computePolynomial(const Matrix& coefficients, const Point& p)const;

    template<typename T>
    void drawSamples(Image<T>& img)const;

    template<typename T>
    Image32 createBackgroundModel(const Image<T>& src)const;

public:
    template<typename T>
    void apply(Image<T>&img);

    template<typename T>
    void applyTo(const Image<T>& src, Image<T>& dst, float scale_factor);
};
