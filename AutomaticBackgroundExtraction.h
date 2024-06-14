#pragma once
#include "Matrix.h"
#include "Image.h"
#include "ProcessDialog.h"

class AutomaticBackgroundExtraction {
public:
    enum class Correction {
        none,
        subtraction,
        division
    };

private:
    std::vector<float> sample;
    int m_rad = 5;
    int m_seperation = 5;

    float m_uK = 0.8;
    float m_lK = 1.8;

    int m_dim = 2 * m_rad + 1;
    int m_size = m_dim * m_dim;
    int m_dist = m_dim + m_seperation;

    int m_poly_degree = 4;
    int m_poly_length = ComputePolyLength();

    int m_pd_1 = m_poly_degree + 1;

    Correction m_correction = Correction::subtraction;

public:
    AutomaticBackgroundExtraction() = default;
    AutomaticBackgroundExtraction(int sample_radius, int sample_distance, float n_sigma);

private:
    int ComputePolyLength();

    float SampleMean(const Image32 & img, int x, int y, int ch);

    float SampleMedian(const Image32& img, int x, int y, int ch);

    void InsertPoint(Matrix & matrix, int row, int x, int y);

    float ComputePolynomial(const Matrix & coefficients, int x, int y);

    void DrawSample(Image32 & img, int x, int y, int ch);

    Image32 CreateBackgroundModel(const Image32 & src);

public:

    int SampleSize(int radius);

    int SampleSeperation(int seperation);

    float UpperThreshold(float multiplier);

    float LowerThreshold(float multiplier);

    int PolynomialDegree(int degree);

    Correction CorrectionMethod(Correction correction);

    template<typename T>
    void Apply(Image<T>&img);
};









class AutomaticBackgroundExtractionDialog : public ProcessDialog {
	Q_OBJECT

	AutomaticBackgroundExtraction m_abe;

    QLineEdit* m_box_size_le;
    QSlider* m_box_size_slider;
    QLineEdit* m_box_dist_le;
    QSlider* m_box_dist_slider;


    DoubleLineEdit* m_global_sigma_le;
    DoubleLineEdit* m_local_sigma_le;



public:

	AutomaticBackgroundExtractionDialog(QWidget* parent = nullptr);

public slots:
    void setLineEdit_boxsize(int value) {
        m_box_size_le->setText(QString::number(value));
    }

    void setLineEdit_boxdist(int value) {
        m_box_dist_le->setText(QString::number(value));
    }

private:

    void AddSampleGeneration();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();

};