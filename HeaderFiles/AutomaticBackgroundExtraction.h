#pragma once
#include "Matrix.h"
#include "Image.h"
#include "ProcessDialog.h"
#include "ImageWindow.h"

class AutomaticBackgroundExtraction {
public:
    enum class Correction {
        subtraction,
        division
    };

private:
    int m_radius = 5;
    int m_seperation = 5;

    float m_uK = 0.8;
    float m_lK = 1.8;

    int m_dim = 2 * m_radius + 1;
    int m_count = m_dim * m_dim;
    int m_dist = m_dim + m_seperation;

    int m_poly_degree = 4;
    int m_poly_length = computePolynomialLength();


    Correction m_correction = Correction::subtraction;

    std::vector<double> polynomial(int x);

public:
    AutomaticBackgroundExtraction() = default;

    int sampleRadius()const { return m_radius; }

    void setSampleRadius(int radius) {
        m_radius = radius;
        m_dim = 2 * m_radius + 1;
        m_count = m_dim * m_dim;
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
        m_poly_length = computePolynomialLength();
    }

    void setCorrectionMethod(Correction method) { m_correction = method; }
private:
    int sampleDistance()const { return m_dist; }

    int sampleCount()const { return m_count; }

    int polynomialLength()const { return m_poly_length; }

    int computePolynomialLength();

    float sampleMedian(const Image32& img, const ImagePoint& p);

    float sampleMean(const Image32& img, const ImagePoint& p);

    void insertMatrixRow(Matrix& matrix, int row, const Point& p);

    double computePolynomial(const Matrix& coefficients, const Point& p);

    void DrawSample(Image32& img, int x, int y, int ch);

    Image32 createBackgroundModel(const Image32& src);

public:
    template<typename T>
    void apply(Image<T>&img);

    template<typename T>
    void applyTo(const Image<T>& src, Image<T>& dst, int factor);
};







class AutomaticBackgroundExtractionDialog : public ProcessDialog {
	Q_OBJECT

	AutomaticBackgroundExtraction m_abe;

    IntLineEdit* m_sample_radius_le = nullptr;
    Slider* m_sample_radius_slider = nullptr;
    IntLineEdit* m_sample_seperation_le = nullptr;
    Slider* m_sample_seperation_slider = nullptr;

    DoubleLineEdit* m_sigma_low_le = nullptr;
    Slider* m_sigma_low_slider = nullptr;
    DoubleLineEdit* m_sigma_high_le = nullptr;
    Slider* m_sigma_high_slider = nullptr;

    SpinBox* m_poly_degree_sb = nullptr;

    ComboBox* m_correction_combo = nullptr;

    PushButton* m_apply_to_preview_pb = nullptr;
    const QString m_apply_to_preview = "Apply to Preview";

public:

	AutomaticBackgroundExtractionDialog(QWidget* parent = nullptr);

private:

    void addSampleGeneration();

    void addSampleRejection();

    void addOther();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();

};