#include "pch.h"
#include "AutomaticBackgroundExtractionDialog.h"
#include "FastStack.h"

using ABE = AutomaticBackgroundExtraction;
using ABED = AutomaticBackgroundExtractionDialog;

ABED::AutomaticBackgroundExtractionDialog(Workspace* parent) : ProcessDialog("Automatic Background Extraction", QSize(530, 380), parent, true) {

    addSampleGeneration();
    addSampleRejection();
    addOther();

    this->show();
}

void ABED::addSampleGeneration() {

    GroupBox* gb = new GroupBox("Sample Generation", drawArea());
    gb->move(15, 15);
    gb->resize(500, 125);

    m_sample_radius_le = new IntLineEdit(5, new IntValidator(1, 50), gb);
    m_sample_radius_le->resize(60, 30);
    m_sample_radius_le->move(160, 30);
    addLabel(m_sample_radius_le, new QLabel("Sample Radius:   ", gb));

    m_sample_radius_slider = new Slider(gb);
    m_sample_radius_slider->setRange(1, 50);
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



    m_sample_seperation_le = new IntLineEdit(5, new IntValidator(1, 50), gb);
    m_sample_seperation_le->resize(60, 30);
    m_sample_seperation_le->move(160, 75);
    addLabel(m_sample_seperation_le, new QLabel("Sample Seperation:   ", gb));

    m_sample_seperation_slider = new Slider(gb);
    m_sample_seperation_slider->setRange(0, 50);
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

    GroupBox* gb = new GroupBox("Sample Rejection", drawArea());
    gb->move(15, 150);
    gb->resize(500, 125);

    m_sigma_low_le = new DoubleLineEdit(2.0, new DoubleValidator(0.0, 10.0, 2), gb);
    m_sigma_low_le->resize(60, 30);
    m_sigma_low_le->move(160, 30);
    addLabel(m_sigma_low_le, new QLabel("Sigma Low:   ", gb));

    m_sigma_low_slider = new Slider(gb);
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
    addLabel(m_sigma_high_le, new QLabel("Sigma High:   ", gb));

    m_sigma_high_slider = new Slider(gb);
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

    m_poly_degree_sb = new SpinBox(4, 0, 9, drawArea());
    m_poly_degree_sb->move(175, 290);
    addLabel(m_poly_degree_sb, new QLabel("Polynomial Degree:   ", drawArea()));
    connect(m_poly_degree_sb, &QSpinBox::valueChanged, this, [this](int val) { m_abe.setPolynomialDegree(val); });

    m_correction_combo = new ComboBox(drawArea());
    m_correction_combo->addItems({ "Subtraction","Division" });
    m_correction_combo->move(390, 290);
    addLabel(m_correction_combo, new QLabel("Correction Method:   ", drawArea()));
    connect(m_correction_combo, &QComboBox::activated, this, [this](int index) { m_abe.setCorrectionMethod(ABE::Correction(index)); });


    m_apply_to_preview_pb = new PushButton(m_apply_to_preview, drawArea());
    m_apply_to_preview_pb->move(15, 335);
    m_apply_to_preview_pb->resize(500, m_apply_to_preview_pb->height());
    m_apply_to_preview_pb->setDisabled(true);

    connect(m_apply_to_preview_pb, &QPushButton::released, this, &ABED::applytoPreview);
    connect(this, &ProcessDialog::previewRemoved, this, std::bind(&PushButton::setEnabled, m_apply_to_preview_pb, true));
    connect(this, &ProcessDialog::previewAdded, this, std::bind(&PushButton::setEnabled, m_apply_to_preview_pb, false));
}

void ABED::closeEvent(QCloseEvent* e) {

    if (isPreviewValid())
        preview()->close();

    ProcessDialog::closeEvent(e);
}

void ABED::resetDialog() {}

void ABED::showPreview() {

    showPreviewWindow(true);
}

void ABED::apply() {

    if (!workspace()->hasSubWindows())
        return;

    switch (currentImageType()) {
    case ImageType::UBYTE: 
        return currentImageWindow()->applyToSource(m_abe, &ABE::apply);
    
    case ImageType::USHORT: 
        return currentImageWindow<uint16_t>()->applyToSource(m_abe, &ABE::apply);
    
    case ImageType::FLOAT: 
        return currentImageWindow<float>()->applyToSource(m_abe, &ABE::apply);
    }
}

void ABED::applyPreview() {

    if (!isPreviewValid())
        return;

    m_apply_to_preview_pb->setDisabled(true);
    m_apply_to_preview_pb->setText("Generating Preview");

    switch (preview()->type()) {
    case ImageType::UBYTE: 
        preview()->updatePreview(m_abe, &ABE::applyTo);
        break;
    
    case ImageType::USHORT: 
        preview<uint16_t>()->updatePreview(m_abe, &ABE::applyTo);
        break;
    
    case ImageType::FLOAT: 
        preview<float>()->updatePreview(m_abe, &ABE::applyTo);
    
    }

    m_apply_to_preview_pb->setEnabled(true);
    m_apply_to_preview_pb->setText(m_apply_to_preview);
}