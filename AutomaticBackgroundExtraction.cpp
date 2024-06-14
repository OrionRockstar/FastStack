#include "pch.h"
#include "FastStack.h"
#include "AutomaticBackgroundExtraction.h"
#include "ImageWindow.h"

using ABE = AutomaticBackgroundExtraction;

ABE::AutomaticBackgroundExtraction(int sample_radius, int sample_distance, float n_sigma) : m_rad(sample_radius), m_seperation(sample_distance) {

}

int ABE::ComputePolyLength() {

    int poly_length = 0;
    for (int i = 1; i <= m_poly_degree + 1; ++i)
        poly_length += i;

    return poly_length;
}

float ABE::SampleMean(const Image32& img, int x, int y, int ch) {

    float sum = 0;

    for (int j = y - m_rad; j <= y + m_rad; ++j)
        for (int i = x - m_rad; i <= x + m_rad; ++i)
            sum += img(i, j, ch);

    return sum / m_size;
}

float ABE::SampleMedian(const Image32& img, int x, int y, int ch) {
    std::vector<float> sample(m_size);
    for (int j = y - m_rad, el = 0; j <= y + m_rad; ++j)
        for (int i = x - m_rad; i <= x + m_rad; ++i)
            sample[el++] = img(i, j, ch);

    std::nth_element(sample.begin(), sample.begin() + m_size / 2, sample.end());

    return sample[m_size / 2];
}

void ABE::InsertPoint(Matrix& matrix, int row, int x, int y) {

    std::vector<double> xv(m_pd_1);
    xv[0] = 1;
    std::vector<double> yv(m_pd_1);
    yv[0] = 1;

    for (int i = 1, j = 0; i <= m_poly_degree; ++i, ++j) {
        xv[i] = xv[j] * x;
        yv[i] = yv[j] * y;
    }

    int limit = m_poly_degree;

    for (int j = 0, col = 0; j <= limit; ++j) {
        for (int i = 0; i <= limit - j; ++i) {
            matrix(row, col++) = xv[i] * yv[j];
        }
    }
}

float ABE::ComputePolynomial(const Matrix& coefficients, int x, int y) {

    std::vector<double> xv(m_pd_1);
    xv[0] = 1;
    std::vector<double> yv(m_pd_1);
    yv[0] = 1;

    for (int i = 1, j = 0; i <= m_poly_degree; ++i, ++j) {
        xv[i] = xv[j] * x;
        yv[i] = yv[j] * y;
    }

    int limit = m_poly_degree;
    double sum = 0;

    for (int j = 0, el = 0; j <= limit; ++j) {
        for (int i = 0; i <= limit - j; ++i) {
            sum += xv[i] * yv[j] * coefficients[el++];
        }
    }

    return sum;
}

void ABE::DrawSample(Image32& img, int x, int y, int ch) {

    for (int j = -m_rad; j <= m_rad; ++j)
        for (int i = -m_rad; i <= m_rad; ++i)
            if (abs(j) == m_rad || abs(i) == m_rad)
                img(x + i, y + j, ch) = 1;
}


Image32 ABE::CreateBackgroundModel(const Image32& src) {
    Image32 background(src.Rows(), src.Cols(), src.Channels());
    int sample_count = ((src.Rows() / m_dist) - 1) * ((src.Cols() / m_dist) - 1);

    for (int ch = 0; ch < src.Channels(); ++ch) {

        float median = src.ComputeMedian(ch);

        float sigma = src.ComputeAvgDev(ch, median);//src.ComputeStdDev(ch);

        float upper = median + m_uK * sigma;
        float lower = median - m_lK * sigma;

        Matrix variables(sample_count, m_poly_length);
        Matrix bgv(sample_count);

        int row = 0;
        int buffer = m_rad + 1;

        for (int y = buffer; y < src.Rows() - buffer; y += m_dist) {
            for (int x = buffer; x < src.Cols() - buffer; x += m_dist) {

                //correct
                float val = SampleMedian(src, x, y, ch);//SampleMean(src, x, y, ch);

                if (val > upper || val < lower) continue;

                InsertPoint(variables, row, x, y);
                bgv[row++] = val;
            }
        }

        row -= 1;

        variables.MatrixResize(row, m_poly_length);
        bgv.MatrixResize(row);

        Matrix coef(m_poly_length);
        coef = Matrix::LeastSquares(variables, bgv);

#pragma omp parallel for
        for (int y = 0; y < src.Rows(); ++y) {
            for (int x = 0; x < src.Cols(); ++x) {
                background(x, y, ch) = ComputePolynomial(coef, x, y);
            }
        }
    }

    return background;
}

int ABE::SampleSize(int radius) {
    return m_rad = radius;
}

int ABE::SampleSeperation(int seperation) {
    m_seperation = seperation;
    m_dist = m_dim + m_seperation;
    return m_seperation;
}

float ABE::UpperThreshold(float multiplier) {
    return m_uK = multiplier;
}

float ABE::LowerThreshold(float multiplier) {
    return m_lK = multiplier;
}

int ABE::PolynomialDegree(int degree) {
    m_poly_degree = degree;
    m_poly_length = ComputePolyLength();
    m_pd_1 = m_poly_degree + 1;
    return m_poly_degree;
}

ABE::Correction ABE::CorrectionMethod(Correction correction) {
    return m_correction = correction;
}

template<typename T>
void ABE::Apply(Image<T>& img) {

    Image32 n_copy(img.Rows(), img.Cols(), img.Channels());

    for (int i = 0; i < img.TotalPxCount(); ++i)
        n_copy[i] = Pixel<float>::toType(img[i]);

    Image32 background = CreateBackgroundModel(n_copy);

    using enum Correction;
    switch (m_correction) {

    case none:
        return;

    case subtraction:
        n_copy -= background;
        break;

    case division:
        n_copy /= background;
        break;
    }

    n_copy.Normalize();

    for (int i = 0; i < img.TotalPxCount(); ++i)
        img[i] = Pixel<T>::toType(n_copy[i]);

}
template void ABE::Apply(Image8&);
template void ABE::Apply(Image16&);
template void ABE::Apply(Image32&);







using ABED = AutomaticBackgroundExtractionDialog;

ABED::AutomaticBackgroundExtractionDialog(QWidget* parent) : ProcessDialog("AutomaticBackgroundExtraction",QSize(400,800), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent, false) {


    connect(this, &ProcessDialog::processDropped, this, &ABED::Apply);
    ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ABED::Apply, &ABED::showPreview, &ABED::resetDialog);

    AddSampleGeneration();

    this->show();
}

void ABED::AddSampleGeneration() {
    m_box_size_le = new QLineEdit("5" , this);
    m_box_size_le->setValidator(new QIntValidator(1, 100, m_box_size_le));
    m_box_size_le->resize(60, 25);
    m_box_size_le->move(75, 0);

    m_box_size_slider = new QSlider(Qt::Horizontal, this);
    m_box_size_slider->setRange(1, 100);
    m_box_size_slider->setFixedWidth(200);
    m_box_size_slider->setValue(5);
    m_box_size_slider->move(150, 0);

    connect(m_box_size_slider, &QSlider::valueChanged, this, &ABED::setLineEdit_boxsize);

    m_box_dist_le = new QLineEdit("5", this);
    m_box_dist_le->move(0, 50);

    m_box_dist_slider = new QSlider(Qt::Horizontal, this);
    m_box_dist_slider->setRange(0, 100);
    m_box_dist_slider->setFixedWidth(200);
    m_box_dist_slider->setValue(5);
    m_box_dist_slider->move(150, 50);

    connect(m_box_dist_slider, &QSlider::valueChanged, this, &ABED::setLineEdit_boxdist);

}


void ABED::resetDialog() {}

void ABED::showPreview() {}

void ABED::Apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->Source().Bitdepth()) {
    case 8: {
        m_abe.Apply(iwptr->Source());
        iwptr->DisplayImage();
        break;
    }
    case 16: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        m_abe.Apply(iw16->Source());
        iw16->DisplayImage();
        break;
    }
    case -32: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        m_abe.Apply(iw32->Source());
        iw32->DisplayImage();
        break;
    }
    }

}

void ABED::ApplytoPreview() {}