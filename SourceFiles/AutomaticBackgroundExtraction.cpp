#include "pch.h"
#include "FastStack.h"
#include "AutomaticBackgroundExtraction.h"
#include "ImageWindow.h"
#include "ImageGeometry.h"

using ABE = AutomaticBackgroundExtraction;

std::vector<double> ABE::polynomial(int x) {

    std::vector<double> poly(polynomialDegree() + 1);
    poly[0] = 1;

    for (int i = 1, j = 0; i <= polynomialDegree(); ++i, ++j)
        poly[i] = poly[j] * x;
    
    return std::move(poly);
}

int ABE::computePolynomialLength() {

    int poly_length = 0;
    for (int i = 1; i <= polynomialDegree() + 1; ++i)
        poly_length += i;

    return poly_length;
}

float ABE::sampleMedian(const Image32& img, const ImagePoint& p) {

    std::vector<float> sample(sampleCount());

    for (int j = -sampleRadius(), el = 0; j <= sampleRadius(); ++j)
        for (int i = -sampleRadius(); i <= sampleRadius(); ++i)
            sample[el++] = img(p.x() + i, p.y() + j, p.channel());

    std::nth_element(sample.begin(), sample.begin() + sampleCount() / 2, sample.end());

    return sample[sampleCount() / 2];
}

float ABE::sampleMean(const Image32& img, const ImagePoint& p) {

    float sum = 0;

    for (int j = -sampleRadius(), el = 0; j <= sampleRadius(); ++j)
        for (int i = -sampleRadius(); i <= sampleRadius(); ++i)
            sum += img(p.x() + i, p.y() + j, p.channel());

    return sum / sampleCount();
}



void ABE::insertMatrixRow(Matrix& matrix, int row, const Point& p) {

    std::vector<double> xv = polynomial(p.x());
    std::vector<double> yv = polynomial(p.y());

    for (int j = 0, col = 0; j <= polynomialDegree(); ++j)
        for (int i = 0; i <= polynomialDegree() - j; ++i)
            matrix(row, col++) = xv[i] * yv[j];
}

double ABE::computePolynomial(const Matrix& coefficients, const Point& p) {

    std::vector<double> xv = polynomial(p.x());
    std::vector<double> yv = polynomial(p.y());

    double sum = 0.0;

    for (int j = 0, el = 0; j <= polynomialDegree(); ++j)
        for (int i = 0; i <= polynomialDegree() - j; ++i)
            sum += xv[i] * yv[j] * coefficients[el++];
       
    return sum;
}

void ABE::DrawSample(Image32& img, int x, int y, int ch) {

    //for (int j = -m_rad; j <= m_rad; ++j)
        //for (int i = -m_rad; i <= m_rad; ++i)
            //if (abs(j) == m_rad || abs(i) == m_rad)
                //img(x + i, y + j, ch) = 1;
}

Image32 ABE::createBackgroundModel(const Image32& src) {

    Image32 background(src.rows(), src.cols(), src.channels());
    int number_of_samples = ((src.rows() / sampleDistance()) - 1) * ((src.cols() / sampleDistance()) - 1);

    for (uint32_t ch = 0; ch < src.channels(); ++ch) {

        float median = src.computeMedian(ch);

        float sigma = src.computeAvgDev(ch, median);//src.ComputeStdDev(ch);

        float upper = median + sigmaKUpper() * sigma;
        float lower = median - sigmaKLower() * sigma;

        Matrix variables(number_of_samples, polynomialLength());
        Matrix bgv(number_of_samples);

        int row = 0;
        int buffer = sampleRadius() + 1;

        for (int y = buffer; y < src.rows() - buffer; y += sampleDistance()) {
            for (int x = buffer; x < src.cols() - buffer; x += sampleDistance()) {

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

    Image32 n_copy(img.rows(), img.cols(), img.channels());

    for (int i = 0; i < img.totalPxCount(); ++i)
        n_copy[i] = Pixel<float>::toType(img[i]);

    Image32 background = createBackgroundModel(n_copy);

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
void ABE::applyTo(const Image<T>& src, Image<T>& dst, int factor) {

    Image32 n_copy(src.rows(), src.cols(), src.channels());

    for (int i = 0; i < src.totalPxCount(); ++i)
        n_copy[i] = Pixel<float>::toType(src[i]);

    Image32 background = createBackgroundModel(n_copy);

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

    IntegerResample ir;
    ir.setFactor(factor);
    ir.apply(n_copy);

    if (!dst.isSameSize(n_copy))
        dst = Image<T>(n_copy.rows(), n_copy.cols(), n_copy.channels());

    for (int i = 0; i < dst.totalPxCount(); ++i)
        dst[i] = Pixel<T>::toType(n_copy[i]);
}
template void ABE::applyTo(const Image8&, Image8&, int);
template void ABE::applyTo(const Image16&, Image16&, int);
template void ABE::applyTo(const Image32&, Image32&, int);







using ABED = AutomaticBackgroundExtractionDialog;

ABED::AutomaticBackgroundExtractionDialog(QWidget* parent) : ProcessDialog(m_abe_str, QSize(530,400), FastStack::recast(parent)->workspace(), true) {

    connectToolbar(this, &ABED::apply, &ABED::showPreview, &ABED::resetDialog);

    addSampleGeneration();
    addSampleRejection();
    addOther();

    this->show();
}

void ABED::addSampleGeneration() {

    GroupBox* gb = new GroupBox("Sample Generation", this);
    gb->move(15, 15);
    gb->resize(500, 125);

    m_sample_radius_le = new IntLineEdit(5, new IntValidator(1, 100), gb);
    m_sample_radius_le->resize(60, 30);
    m_sample_radius_le->move(160, 30);
    m_sample_radius_le->addLabel(new QLabel("Sample Radius:   ", gb));

    m_sample_radius_slider = new Slider(Qt::Horizontal, gb);
    m_sample_radius_slider->setRange(1, 100);
    m_sample_radius_slider->setFixedWidth(250);
    m_sample_radius_slider->setValue(5);
    m_sample_radius_le->addSlider(m_sample_radius_slider);

    auto action_radius = [this](int) {
        int pos = m_sample_radius_slider->sliderPosition();
        m_sample_radius_le->setValue(pos);
        m_abe.setSampleRadius(pos);
    };

    auto edited_radius = [this]() {
        int val = m_sample_radius_le->value();
        m_sample_seperation_slider->setValue(val);
        m_abe.setSampleRadius(val);
    };

    connect(m_sample_radius_slider, &QSlider::actionTriggered, this, action_radius);
    connect(m_sample_radius_le, &QLineEdit::editingFinished, this, edited_radius);



    m_sample_seperation_le = new IntLineEdit(5, new IntValidator(0, 100), gb);
    m_sample_seperation_le->resize(60, 30);
    m_sample_seperation_le->move(160, 75);
    m_sample_seperation_le->addLabel(new QLabel("Sample Seperation:   ", gb));

    m_sample_seperation_slider = new Slider(Qt::Horizontal, gb);
    m_sample_seperation_slider->setRange(0, 100);
    m_sample_seperation_slider->setFixedWidth(250);
    m_sample_seperation_slider->setValue(5);
    m_sample_seperation_le->addSlider(m_sample_seperation_slider);


    auto action_seperation = [this](int) {
        int pos = m_sample_seperation_slider->sliderPosition();
        m_sample_seperation_le->setValue(pos);
        m_abe.setSampleSeperation(pos);
    };

    auto edited_seperation = [this]() {
        int val = m_sample_seperation_le->value();
        m_sample_seperation_slider->setValue(val);
        m_abe.setSampleSeperation(val);
    };

    connect(m_sample_seperation_slider, &QSlider::actionTriggered, this, action_seperation);
    connect(m_sample_seperation_le, &QLineEdit::editingFinished, this, edited_seperation);
}

void ABED::addSampleRejection() {

    GroupBox* gb = new GroupBox("Sample Rejection", this);
    gb->move(15, 150);
    gb->resize(500, 125);

    m_sigma_low_le = new DoubleLineEdit(2.0, new DoubleValidator(0.0, 10.0, 2), gb);
    m_sigma_low_le->resize(60, 30);
    m_sigma_low_le->move(160, 30);
    m_sigma_low_le->addLabel(new QLabel("Sigma Low:   ", gb));

    m_sigma_low_slider = new Slider(Qt::Horizontal, gb);
    m_sigma_low_slider->setRange(0, 100);
    m_sigma_low_slider->setFixedWidth(250);
    m_sigma_low_slider->setValue(20);
    m_sigma_low_le->addSlider(m_sigma_low_slider);

    auto action_low = [this](int) {
        double low = m_sigma_low_slider->sliderPosition() / 10.0;
        m_sigma_low_le->setValue(low);
        m_abe.setSigmaKLower(low);
    };

    auto edited_low = [this]() {
        double val = m_sigma_low_le->value();
        m_sigma_low_slider->setValue(val * 10);
        m_abe.setSigmaKLower(val);
    };

    connect(m_sigma_low_slider, &QSlider::actionTriggered, this, action_low);
    connect(m_sigma_low_le, &QLineEdit::editingFinished, this, edited_low);



    m_sigma_high_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 10.0, 2), gb);
    m_sigma_high_le->resize(60, 30);
    m_sigma_high_le->move(160, 75);
    m_sigma_high_le->addLabel(new QLabel("Sigma High:   ", gb));

    m_sigma_high_slider = new Slider(Qt::Horizontal, gb);
    m_sigma_high_slider->setRange(0, 100);
    m_sigma_high_slider->setFixedWidth(250);
    m_sigma_high_slider->setValue(10);
    m_sigma_high_le->addSlider(m_sigma_high_slider);

    auto action_high = [this](int) {
        double high = m_sigma_high_slider->sliderPosition() / 10.0;
        m_sigma_high_le->setValue(high);
        m_abe.setSigmaKUpper(high);
    };

    auto edited_high = [this]() {
        double val = m_sigma_high_le->value();
        m_sigma_high_slider->setValue(val * 10);
        m_abe.setSigmaKUpper(val);
    };

    connect(m_sigma_high_slider, &QSlider::actionTriggered, this, action_high);
    connect(m_sigma_high_le, &QLineEdit::editingFinished, this, edited_high);
}

void ABED::addOther() {

    m_poly_degree_sb = new SpinBox(4, 0, 9, this);
    m_poly_degree_sb->move(175, 290);
    m_poly_degree_sb->addLabel(new QLabel("Polynomial Degree:   ", this));
    connect(m_poly_degree_sb, &QSpinBox::valueChanged, this, [this](int val) { m_abe.setPolynomialDegree(val); });

    m_correction_combo = new ComboBox(this);
    m_correction_combo->addItems({ "Subtraction","Division" });
    m_correction_combo->move(390, 290);
    m_correction_combo->addLabel(new QLabel("Correction Method:   ", this));
    connect(m_correction_combo, &QComboBox::activated, this, [this](int index) { m_abe.setCorrectionMethod(ABE::Correction(index)); });


    m_apply_to_preview_pb = new PushButton(m_apply_to_preview, this);
    m_apply_to_preview_pb->move(15, 335);
    m_apply_to_preview_pb->resize(500, m_apply_to_preview_pb->height());
    m_apply_to_preview_pb->setDisabled(true);

    connect(m_apply_to_preview_pb, &QPushButton::released, this, [this]() { std::thread t(&ABED::applytoPreview, this); t.detach();});
}

void ABED::resetDialog() {}

void ABED::showPreview() {

    ProcessDialog::showPreview_zoomWindowIgnored();

    if (m_preview != nullptr) {
        m_apply_to_preview_pb->setEnabled(true);
        connect(previewRecast(m_preview)->windowSignals(), &WindowSignals::windowClosed, this, [this]() { m_apply_to_preview_pb->setDisabled(true); });
    }

    std::thread t(&ABED::applytoPreview, this); 
    t.detach();
}

void ABED::apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        return iwptr->applyToSource(m_abe, &ABE::apply);
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        return iw16->applyToSource(m_abe, &ABE::apply);
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        return iw32->applyToSource(m_abe, &ABE::apply);
    }
    }

}

void ABED::applytoPreview() {

    if (!isPreviewValid())
        return;

    PreviewWindow8* iwptr = previewRecast(m_preview);

    m_apply_to_preview_pb->setDisabled(true);
    m_apply_to_preview_pb->setText("Generating Preview");

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        auto iw8 = iwptr;
        iw8->updatePreview(m_abe, &ABE::applyTo);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = previewRecast<uint16_t>(iwptr);
        iw16->updatePreview(m_abe, &ABE::applyTo);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = previewRecast<float>(iwptr);
        iw32->updatePreview(m_abe, &ABE::applyTo);
    }
    }

    m_apply_to_preview_pb->setEnabled(true);
    m_apply_to_preview_pb->setText(m_apply_to_preview);
}