#include "pch.h"
#include "FastStack.h"
#include "StarMaskDialog.h"
#include "ImageWindow.h"


//use same preview style as auto bg extract where i manually update preview
StarMaskDialog::StarMaskDialog(Workspace* parent) : ProcessDialog("StarMask", QSize(500, 185), parent, false, true, false) {

    addStarThresholdInputs();
    addRoundnessInputs();
    addGaussianBlurInputs();
    addPSFInputs();

    m_real_value_cb = new CheckBox("RPV", drawArea());
    m_real_value_cb->move(365, 142);
    m_real_value_cb->setToolTip("Real pixel value");
    connect(m_real_value_cb, &QCheckBox::clicked, this, [this](bool v) { m_sm.setRealValue(v); });

    this->show();
}

void StarMaskDialog::addStarThresholdInputs() {

    QString txt = "Sets K value of star threshold, defined as median + K * avg deviation.";

    m_sigmaK_input = new DoubleInput("Star signal threshold:   ", m_sm.starDetector().K(), new DoubleValidator(0.1, 5.0, 2), drawArea(), 20);
    m_sigmaK_input->move(175, 15);
    m_sigmaK_input->setSliderWidth(200);
    m_sigmaK_input->setToolTip(txt);

    auto func = [this]() { m_sm.starDetector().setK(m_sigmaK_input->valuef()); };
    connect(m_sigmaK_input, &InputBase::actionTriggered, this, func);
    connect(m_sigmaK_input, &InputBase::editingFinished, this, func);
}

void StarMaskDialog::addRoundnessInputs() {

    m_roundness_input = new DoubleInput("Roundness threshold:   ", m_sm.starDetector().roundness(), new DoubleValidator(0.1, 1.0, 2), drawArea(), 100);
    m_roundness_input->move(175, 55);
    m_roundness_input->setSliderWidth(200);

    auto func = [this]() { m_sm.starDetector().setRoundness(m_roundness_input->valuef()); };
    connect(m_roundness_input, &InputBase::actionTriggered, this, func);
    connect(m_roundness_input, &InputBase::editingFinished, this, func);
}

void StarMaskDialog::addGaussianBlurInputs() {

    m_gblur_sigma_input = new DoubleInput("Gaussian blur std-dev:   ", m_sm.gaussianBlurStdDev(), new DoubleValidator(0.1, 10.0, 2), drawArea(), 10);
    m_gblur_sigma_input->move(175, 95);
    m_gblur_sigma_input->setSliderWidth(200);

    auto func = [this]() { m_sm.setGaussianBlurStdDev(m_gblur_sigma_input->valuef()); };
    connect(m_gblur_sigma_input, &InputBase::actionTriggered, this, func);
    connect(m_gblur_sigma_input, &InputBase::editingFinished, this, func);
}

void StarMaskDialog::addPSFInputs() {

    m_psf_combo = new ComboBox(drawArea());
    m_psf_combo->move(135, 140);
    addLabel(m_psf_combo, new QLabel("PSF:"));
    m_psf_combo->addItem("Gaussian", QVariant::fromValue(PSF::Type::gaussian));
    m_psf_combo->addItem("Moffat", QVariant::fromValue(PSF::Type::moffat));

    m_beta_sb = new DoubleSpinBox(m_sm.starDetector().beta(), 0.0, 10.0, 1, drawArea());
    m_beta_sb->move(285, 140);
    addLabel(m_beta_sb, new QLabel("Beta:"));
    m_beta_sb->setSingleStep(0.1);

    auto activate = [this]() {

        auto psf_t = m_psf_combo->currentData().value<PSF::Type>();
        m_sm.starDetector().setPSF(psf_t);

        if (psf_t == PSF::Type::moffat)
            m_beta_sb->setEnabled(true);
        else
            m_beta_sb->setDisabled(true);
    };

    connect(m_psf_combo, &QComboBox::activated, this, activate);
    activate();
    connect(m_beta_sb, &QDoubleSpinBox::valueChanged, this, [this](double val) { m_sm.starDetector().setBeta(val); });
}

void StarMaskDialog::resetDialog() {

    m_sm = StarMask();

    auto&  sd = m_sm.starDetector();

    m_sigmaK_input->reset();

    m_roundness_input->reset();

    m_gblur_sigma_input->reset();

    m_psf_combo->setCurrentIndex(int(sd.psf()));
    m_beta_sb->setValue(sd.beta());
    m_beta_sb->setEnabled(false);
    m_real_value_cb->setChecked(m_sm.realValue());
}

void StarMaskDialog::apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

    QString name = "StarMask";

    enableSiblings_Subwindows(false);

    int count = 0;
    for (auto sw : m_workspace->subWindowList()) {
        auto ptr = imageRecast<>(sw->widget());
        if (ptr->name().contains(name))
            count++;
    }

    if (count != 0)
        name += QString::number(count);

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        Image8 sm = m_sm.generateStarMask(iwptr->source());
        ImageWindow8* iw = new ImageWindow8(std::move(sm), name, m_workspace);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = imageRecast<uint16_t>(iwptr);
        Image16 sm = m_sm.generateStarMask(iw16->source());
        ImageWindow16* iw = new ImageWindow16(std::move(sm), name, m_workspace);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = imageRecast<float>(iwptr);
        Image32 sm = m_sm.generateStarMask(iw32->source());
        ImageWindow32* iw = new ImageWindow32(std::move(sm), name, m_workspace);
        break;
    }
    }

    enableSiblings_Subwindows(true);
}