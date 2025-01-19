#include "pch.h"
#include "FastStack.h"
#include "StarMask.h"
#include "GaussianFilter.h"

template<typename T>
Image<T> StarMask::generateStarMask(const Image<T>& src) {

	StarVector sv = m_sd.applyStarDetection(src);

    Image<T> mask(src.rows(), src.cols());

    for (auto star : sv) {
        int xc = star.xc;
        int yc = star.yc;
        int r = star.radius;
        for (int j = yc - r; j <= yc + r; ++j)
            for (int i = xc - r; i <= xc + r; ++i)
                if (mask.isInBounds(i, j) && math::distance(star.xc, star.yc, i, j) <= star.radius)
                    mask(i, j) = (m_real_value) ? src(i, j) : Pixel<T>::max();
    }

    GaussianFilter(m_stddev).apply(mask);
	return mask;
}
template Image8 StarMask::generateStarMask(const Image8&);
template Image16 StarMask::generateStarMask(const Image16&);
template Image32 StarMask::generateStarMask(const Image32&);









StarMaskDialog::StarMaskDialog(QWidget* parent) : ProcessDialog("StarMask", QSize(500,280), FastStack::recast(parent)->workspace(), false, true, false) {

	connectToolbar(this, &StarMaskDialog::apply, &StarMaskDialog::showPreview, &StarMaskDialog::resetDialog);

    addWaveletInputs();
    addThresholdInputs();
    addPeakEdgeRatioInputs();
    addRoundnessInputs();
    addGaussianBlurInputs();
    addPSFInputs();

    m_real_value_cb = new CheckBox("Real Pixel Value", this);
    m_real_value_cb->move(335, 215);
    connect(m_real_value_cb, &QCheckBox::clicked, this, [this](bool v) { m_sm.setRealValue(v); });

	this->show();
}

void StarMaskDialog::addWaveletInputs() {

    m_wavelet_layers_sb = new SpinBox(this);
    m_wavelet_layers_sb->move(175, 15);
    m_wavelet_layers_sb->setRange(1, 6);
    m_wavelet_layers_sb->setValue(5);
    m_wavelet_layers_sb->setFixedWidth(75);
    m_wavelet_layers_sb->addLabel(new QLabel("Wavelet Layers:   ", this));

    connect(m_wavelet_layers_sb, &QSpinBox::valueChanged, this, [this](int value) { m_sm.starDetector()->setWaveletLayers(value); });

    m_median_blur_cb = new CheckBox("Median Blur", this);
    m_median_blur_cb->move(325, 15);
    m_median_blur_cb->setChecked(true);
    connect(m_median_blur_cb, &::QCheckBox::clicked, this, [this](bool v) {m_sm.starDetector()->applyMedianBlur(v); });
}

void StarMaskDialog::addThresholdInputs() {
    QString txt = "Sets K value of star threshold defined as median + K * standard deviation.";

    m_sigmaK_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 10.0, 2), this);
    m_sigmaK_le->move(175, 55);
    m_sigmaK_le->addLabel(new QLabel("Star Signal Threshold:   ", this));
    m_sigmaK_le->setToolTip(txt);

    m_sigmaK_slider = new Slider(Qt::Horizontal, this);
    m_sigmaK_slider->setFixedWidth(200);
    m_sigmaK_slider->setRange(0, 200);
    m_sigmaK_slider->setValue(20);
    m_sigmaK_le->addSlider(m_sigmaK_slider);
    m_sigmaK_slider->setToolTip(txt);

    auto action = [this](int) {
        double value = m_sigmaK_slider->sliderPosition() / 20.0;
        m_sigmaK_le->setValue(value);
        m_sm.starDetector()->setSigmaK(value);
    };

    auto edited = [this]() {
        double value = m_sigmaK_le->value();
        m_sigmaK_slider->setValue(value * 20);
        m_sm.starDetector()->setSigmaK(value);
    };

    connect(m_sigmaK_slider, &QSlider::actionTriggered, this, action);
    connect(m_sigmaK_le, &QLineEdit::editingFinished, this, edited);
}

void StarMaskDialog::addPeakEdgeRatioInputs() {

    m_peak_edge_le = new DoubleLineEdit(m_sm.starDetector()->peakEdge(), new DoubleValidator(0.0, 1.0, 2), this);
    m_peak_edge_le->move(175, 95);
    m_peak_edge_le->addLabel(new QLabel("Peak-Edge Ratio:   ", this));

    m_peak_edge_slider = new Slider(Qt::Horizontal, this);
    m_peak_edge_slider->setFixedWidth(200);
    m_peak_edge_slider->setRange(0, 100);
    m_peak_edge_slider->setValue(m_sm.starDetector()->peakEdge() * 100);
    m_peak_edge_le->addSlider(m_peak_edge_slider);

    auto action = [this](int) {
        double value = m_peak_edge_slider->sliderPosition() / 100.0;
        m_peak_edge_le->setValue(value);
        m_sm.starDetector()->setPeakEdge(value);
    };

    auto edited = [this]() {
        double value = m_peak_edge_le->value();
        m_peak_edge_slider->setValue(value * 100);
        m_sm.starDetector()->setPeakEdge(value);
    };

    connect(m_peak_edge_slider, &QSlider::actionTriggered, this, action);
    connect(m_peak_edge_le, &QLineEdit::editingFinished, this, edited);
}

void StarMaskDialog::addRoundnessInputs() {

    m_roundness_le = new DoubleLineEdit(m_sm.starDetector()->roundness(), new DoubleValidator(0.0, 1.0, 2), this);
    m_roundness_le->move(175, 135);
    m_roundness_le->addLabel(new QLabel("Roundness threshold:   ", this));

    m_roundness_slider = new Slider(Qt::Horizontal, this);
    m_roundness_slider->setFixedWidth(200);
    m_roundness_slider->setRange(0, 100);
    m_roundness_slider->setValue(m_sm.starDetector()->roundness() * 100);
    m_roundness_le->addSlider(m_roundness_slider);

    auto action = [this](int) {
        double value = m_roundness_slider->sliderPosition() / 100.0;
        m_roundness_le->setValue(value);
        m_sm.starDetector()->setRoundness(value);
    };

    auto edited = [this]() {
        double value = m_roundness_le->value();
        m_roundness_slider->setValue(value * 100);
        m_sm.starDetector()->setRoundness(value);
    };

    connect(m_roundness_slider, &QSlider::actionTriggered, this, action);
    connect(m_roundness_le, &QLineEdit::editingFinished, this, edited);
}

void StarMaskDialog::addGaussianBlurInputs() {

    m_gblur_sigma_le = new DoubleLineEdit(m_sm.gaussianBlurStdDev(), new DoubleValidator(0.0, 10.0, 2), this);
    m_gblur_sigma_le->move(175, 175);
    m_gblur_sigma_le->addLabel(new QLabel("Gaussian Blur Std Dev:   ", this));

    m_gblur_sigma_slider = new Slider(Qt::Horizontal, this);
    m_gblur_sigma_slider->setFixedWidth(200);
    m_gblur_sigma_slider->setRange(0, 100);
    m_gblur_sigma_slider->setValue(m_sm.gaussianBlurStdDev() * 10.0);
    m_gblur_sigma_le->addSlider(m_gblur_sigma_slider);

    auto action = [this](int) {
        double value = m_gblur_sigma_slider->sliderPosition() / 10.0;
        m_gblur_sigma_le->setValue(value);
        m_sm.setGaussianBlurStdDev(value);
    };

    auto edited = [this]() {
        double value = m_gblur_sigma_le->value();
        m_gblur_sigma_slider->setValue(value * 10);
        m_sm.setGaussianBlurStdDev(value);
    };

    connect(m_gblur_sigma_slider, &QSlider::actionTriggered, this, action);
    connect(m_gblur_sigma_le, &QLineEdit::editingFinished, this, edited);
}

void StarMaskDialog::addPSFInputs() {

    m_psf_combo = new ComboBox(this);
    m_psf_combo->move(100, 215);
    m_psf_combo->addLabel(new QLabel("PSF:   ", this));
    m_psf_combo->addItems({ "Gassian","Moffat"});
    m_psf_combo->setCurrentIndex(int(m_sm.starDetector()->psf()));

    m_beta_sb = new DoubleSpinBox(this);
    m_beta_sb->move(250, 215);
    m_beta_sb->addLabel(new QLabel("Beta:   ", this));
    m_beta_sb->setRange(0.0, 10.0);
    m_beta_sb->setSingleStep(0.1);
    m_beta_sb->setDecimals(1);
    m_beta_sb->setValue(m_sm.starDetector()->beta());

    auto activate = [this](int index) {

        auto psf_t = PSF::Type(index);
        m_sm.starDetector()->setPSF(psf_t);

        if (psf_t == PSF::Type::moffat)
            m_beta_sb->setEnabled(true);
        else
            m_beta_sb->setDisabled(true);
    };

    connect(m_psf_combo, &QComboBox::activated, this, activate);
    activate(m_psf_combo->currentIndex());
    connect(m_beta_sb, &QDoubleSpinBox::valueChanged, this, [this](double val) { m_sm.starDetector()->setBeta(val); });
}

void StarMaskDialog::resetDialog() {

    m_sm.reset();

    auto sd = m_sm.starDetector();

    m_wavelet_layers_sb->setValue(sd->waveletLayers());
    m_median_blur_cb->setChecked(sd->medianBlur());

    m_sigmaK_le->setValue(sd->sigmaK());
    m_sigmaK_le->editingFinished();

    m_peak_edge_le->setValue(sd->peakEdge());
    m_peak_edge_le->editingFinished();

    m_roundness_le->setValue(sd->roundness());
    m_roundness_le->editingFinished();

    m_psf_combo->setCurrentIndex(int(sd->psf()));
    m_beta_sb->setValue(sd->beta());
}

void StarMaskDialog::apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

    std::string name = "StarMask";

    setEnabledAll(false);

    int count = 0;
    for (auto sw : m_workspace->subWindowList()) {
        auto ptr = imageRecast<>(sw->widget());
        std::string img_name = ptr->name().toStdString();
        if (name == img_name)
            name += std::to_string(++count);
    }

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        Image8 sm = m_sm.generateStarMask(iwptr->source());
        ImageWindow8* iw = new ImageWindow8(std::move(sm), name.c_str(), m_workspace);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = imageRecast<uint16_t>(iwptr);
        Image16 sm = m_sm.generateStarMask(iw16->source());
        ImageWindow16* iw = new ImageWindow16(std::move(sm), name.c_str(), m_workspace);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = imageRecast<float>(iwptr);
        Image32 sm = m_sm.generateStarMask(iw32->source());
        ImageWindow32* iw = new ImageWindow32(std::move(sm), name.c_str(), m_workspace);
        break;
    }
    }

    setEnabledAll(true);
}