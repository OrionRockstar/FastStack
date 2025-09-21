#include "pch.h"
#include "LRGBCombinationDialog.h"
#include "FastStack.h"


using LRGBCD = LRGBCombinationDialog;

LRGBCD::LRGBCombinationDialog(QWidget* parent) : ProcessDialog("LRGBCombination", { 400,275 }, FastStack::recast(parent)->workspace(), false, false) {

    m_toolbox = new QToolBox(drawArea());
    m_toolbox->setBackgroundRole(QPalette::Window);

    m_pal.setColor(QPalette::ButtonText, Qt::white);
    m_toolbox->setPalette(m_pal);
    m_pal.setColor(QPalette::ButtonText, Qt::black);

    m_toolbox->move(10, 10);
    m_toolbox->setFixedWidth(380);


    addImageSelection();
    m_toolbox->addItem(m_image_selection_gb, "Image Selection");

    addImageWeights();
    m_toolbox->addItem(m_image_weight_gb, "Image Weights");

    this->show();
}

void LRGBCD::onImageWindowCreated() {

    auto iw = imageRecast<>(m_workspace->subWindowList().last()->widget());

    if (iw->channels() == 1) {
        m_lum_combo->addImage(iw);
        m_red_combo->addImage(iw);
        m_green_combo->addImage(iw);
        m_blue_combo->addImage(iw);
    }
}

void LRGBCD::onImageWindowClosed() {

    auto img = &imageRecast<>(m_workspace->currentSubWindow()->widget())->source();

    int index = m_lum_combo->findImage(img);
    if (index == m_lum_combo->currentIndex()) {
        m_lrgbc.setLum(nullptr);
        m_lum_combo->setCurrentIndex(0);
    }
    m_lum_combo->removeItem(index);

    index = m_red_combo->findImage(img);
    if (index == m_red_combo->currentIndex()) {
        m_lrgbc.channelCombination().setRed(nullptr);
        m_red_combo->setCurrentIndex(0);
    }
    m_red_combo->removeItem(index);


    index = m_green_combo->findImage(img);
    if (index == m_green_combo->currentIndex()) {
        m_lrgbc.channelCombination().setGreen(nullptr);
        m_green_combo->setCurrentIndex(0);
    }
    m_green_combo->removeItem(index);


    index = m_blue_combo->findImage(img);
    if (index == m_blue_combo->currentIndex()) {
        m_lrgbc.channelCombination().setBlue(nullptr);
        m_blue_combo->setCurrentIndex(0);
    }
    m_blue_combo->removeItem(index);
}

void LRGBCD::addImageSelection() {

    m_image_selection_gb = new GroupBox(this);
    m_image_selection_layout = new QGridLayout;

    addLumInputs();
    addRedInputs();
    addGreenInputs();
    addBlueInputs();

    for (auto sw : m_workspace->subWindowList()) {
        auto iw = imageRecast<>(sw->widget());
        if (iw->channels() == 1) {
            m_lum_combo->addImage(iw);
            m_red_combo->addImage(iw);
            m_green_combo->addImage(iw);
            m_blue_combo->addImage(iw);
        }
    }

    m_image_selection_gb->setLayout(m_image_selection_layout);
    m_image_selection_gb->layout()->setContentsMargins(10, 20, 10, 15);
}

void LRGBCD::addLumInputs() {

    m_lum_cb = new CheckBox("Lum", this);
    m_lum_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_lum_cb, 0, 0);
    connect(m_lum_cb, &QCheckBox::clicked, this, [this](bool v) { m_lrgbc.enableLum(v); });

    m_lum_combo = new ComboBox(this);
    m_lum_combo->setPalette(m_pal);
    m_lum_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_lum_combo, 0, 1);
    m_lum_combo->addItem("No Selected Image", 0);

    connect(m_lum_combo, &QComboBox::activated, this, [this]() { m_lrgbc.setLum(m_lum_combo->currentImage()); });
}

void LRGBCD::addRedInputs() {

    m_red_cb = new CheckBox("Red", this);
    m_red_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_red_cb, 1, 0);
    connect(m_red_cb, &QCheckBox::clicked, this, [this](bool v) { m_lrgbc.channelCombination().enableRed(v); });

    m_red_combo = new ComboBox(this);
    m_red_combo->setPalette(m_pal);
    m_red_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_red_combo, 1, 1);
    m_red_combo->addItem("No Selected Image", 0);

    connect(m_red_combo, &QComboBox::activated, this, [this]() { m_lrgbc.channelCombination().setRed(m_red_combo->currentImage()); });
}

void LRGBCD::addGreenInputs() {

    m_green_cb = new CheckBox("Green", this);

    m_green_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_green_cb, 2, 0);
    connect(m_green_cb, &QCheckBox::clicked, this, [this](bool v) { m_lrgbc.channelCombination().enableGreen(v); });

    m_green_combo = new ComboBox(this);
    m_green_combo->setPalette(m_pal);
    m_green_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_green_combo, 2, 1);
    m_green_combo->addItem("No Selected Image", 0);

    connect(m_green_combo, &QComboBox::activated, this, [this]() { m_lrgbc.channelCombination().setGreen(m_green_combo->currentImage()); });
}

void LRGBCD::addBlueInputs() {

    m_blue_cb = new CheckBox("Blue", this);
    m_blue_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_blue_cb, 3, 0);
    connect(m_blue_cb, &QCheckBox::clicked, this, [this](bool v) { m_lrgbc.channelCombination().enableBlue(v); });

    m_blue_combo = new ComboBox(this);
    m_blue_combo->setPalette(m_pal);
    m_blue_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_blue_combo, 3, 1);
    m_blue_combo->addItem("No Selected Image", 0);

    connect(m_blue_combo, &QComboBox::activated, this, [this]() { m_lrgbc.channelCombination().setBlue(m_blue_combo->currentImage()); });
}

void LRGBCD::addImageWeights() {

    m_image_weight_gb = new GroupBox(this);

    m_image_weight_layout = new QGridLayout;
    addLWeightInputs();
    addRWeightInputs();
    addGWeightInputs();
    addBWeightInputs();

    m_image_weight_gb->setLayout(m_image_weight_layout);
    m_image_weight_gb->layout()->setContentsMargins(10, 20, 10, 15);
}

void LRGBCD::addLWeightInputs() {

    m_lum_le = new DoubleLineEdit(m_lrgbc.lumWeight() , new DoubleValidator(0.0, 1.0, 6), this);
    //m_lum_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Lum", this), 0, 0);
    m_image_weight_layout->addWidget(m_lum_le, 0, 1);

    m_lum_slider = new Slider(this);
    m_lum_slider->setFixedWidth(200);
    m_lum_slider->setRange(0, 200);
    m_lum_slider->setValue(200);
    m_image_weight_layout->addWidget(m_lum_slider, 0, 2);

    auto ef = [this]() {
        double w = m_lum_le->value();
        m_lrgbc.setLumWeight(w);
        m_lum_slider->setValue(w * 200); };
    connect(m_lum_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_lum_slider->sliderPosition() / 200.0;
        m_lrgbc.setLumWeight(w);
        m_lum_le->setValue(w); };
    connect(m_lum_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCD::addRWeightInputs() {

    m_red_le = new DoubleLineEdit(1.0,new DoubleValidator(0.0, 1.0, 6), this);
    //m_red_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Red", this), 1, 0);
    m_image_weight_layout->addWidget(m_red_le, 1, 1);

    m_red_slider = new Slider(this);
    m_red_slider->setFixedWidth(200);
    m_red_slider->setRange(0, 200);
    m_red_slider->setValue(200);
    m_image_weight_layout->addWidget(m_red_slider, 1, 2);

    auto ef = [this]() {
        double w = m_red_le->value();
        m_lrgbc.setRedWeight(w);
        m_red_slider->setValue(w * 200); };
    connect(m_red_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_red_slider->sliderPosition() / 200.0;
        m_lrgbc.setRedWeight(w);
        m_red_le->setValue(w); };
    connect(m_red_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCD::addGWeightInputs() {

    m_green_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 6), this);
    //m_green_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Green", this), 2, 0);
    m_image_weight_layout->addWidget(m_green_le, 2, 1);

    m_green_slider = new Slider(this);
    m_green_slider->setFixedWidth(200);
    m_green_slider->setRange(0, 200);
    m_green_slider->setValue(200);
    m_image_weight_layout->addWidget(m_green_slider, 2, 2);

    auto ef = [this]() {
        double w = m_green_le->value();
        m_lrgbc.setGreenWeight(w);
        m_green_slider->setValue(w * 200); };
    connect(m_green_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_green_slider->sliderPosition() / 200.0;
        m_lrgbc.setGreenWeight(w);
        m_green_le->setValue(w); };
    connect(m_green_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCD::addBWeightInputs() {

    m_blue_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 6), this);
    //m_blue_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Blue", this), 3, 0);
    m_image_weight_layout->addWidget(m_blue_le, 3, 1);

    m_blue_slider = new Slider(this);
    m_blue_slider->setFixedWidth(200);
    m_blue_slider->setRange(0, 200);
    m_blue_slider->setValue(200);
    m_image_weight_layout->addWidget(m_blue_slider, 3, 2);

    auto ef = [this]() {
        double w = m_blue_le->value();
        m_lrgbc.setBlueWeight(w);
        m_blue_slider->setValue(w * 200); };
    connect(m_blue_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_blue_slider->sliderPosition() / 200.0;
        m_lrgbc.setBlueWeight(w);
        m_blue_le->setValue(w); };
    connect(m_blue_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCD::addChrominaceNRInputs() {

}

void LRGBCD::resetDialog() {

    m_lrgbc = LRGBCombination();

    auto cc = m_lrgbc.channelCombination();

    m_lum_cb->setChecked(true);
    m_lum_combo->setCurrentIndex(0);

    m_red_cb->setChecked(cc.isRedEnabled());
    m_red_combo->setCurrentIndex(0);

    m_green_cb->setChecked(cc.isGreenEnabled());
    m_green_combo->setCurrentIndex(0);

    m_blue_cb->setChecked(cc.isBlueEnabled());
    m_blue_combo->setCurrentIndex(0);

    m_lum_le->setValue(m_lrgbc.lumWeight());
    emit m_lum_le->editingFinished();

    m_red_le->setValue(m_lrgbc.redWeight());
    emit m_red_le->editingFinished();

    m_green_le->setValue(m_lrgbc.greenWeight());
    emit m_green_le->editingFinished();

    m_blue_le->setValue(m_lrgbc.blueWeight());
    emit m_blue_le->editingFinished();
}

void LRGBCD::apply() {

    Status status = m_lrgbc.isImagesSameSize();

    if (status) {
        Image32 lrgb = m_lrgbc.generateLRGBImage();
        ImageWindow32* iw = new ImageWindow32(std::move(lrgb), "LRGB Image", m_workspace);
    }

    else
        QMessageBox::information(this, "", status.m_message);
}