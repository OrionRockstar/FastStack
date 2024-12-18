#include "pch.h"
#include "FastStack.h"
#include "LRGBCombination.h"
#include "HistogramTransformation.h"

Status LRGBCombination::isImagesSameSize()const {
    Status status(false, "Incompatible Image Sizes!");

    if (!m_cc.isImagesSameSize())
        return status;
    
    QSize s = m_cc.outputSize();

    if (m_enable_lum && m_L) 
        if (m_L->rows() != s.height() || m_L->cols() != s.width())
            return status;
    
    return Status();
}

template<typename T>
void LRGBCombination::combineLuminance(Image32& rgb, const Image<T>& lum) {
    using HT = HistogramTransformation;

    double Lt_w = 1 - m_L_weight;
    //max value of lum midtone is 0.5
#pragma omp parallel for num_threads(2)
    for (int y = 0; y < rgb.rows(); ++y) {
        for (int x = 0; x < rgb.cols(); ++x) {
            auto color = rgb.color<double>(x, y);

            color.rRed() *= m_R_weight;
            color.rGreen() *= m_G_weight;
            color.rBlue() *= m_B_weight;

            double L, c, h;

            ColorSpace::RGBtoCIELch(color, L, c, h);
            L = HT::MTF(L * Lt_w + m_L_weight * lum(x,y), m_lightness_mtf);
            c = HT::MTF(c, m_saturation_mtf) * L + c * (1 - L);
            rgb.setColor(x, y, ColorSpace::CIELchtoRGB(L, c, h));
        }
    }
}

Image32 LRGBCombination::generateLRGBImage() {

    Image32 rgb = m_cc.generateRGBImage();

    double Lt_w = 1 - m_L_weight;

    if (m_L && m_enable_lum) {
        switch (m_L->type()) {
        case ImageType::UBYTE: {
            combineLuminance(rgb, *m_L);
            break;
        }
        case ImageType::USHORT: {
            combineLuminance(rgb, *reinterpret_cast<const Image16*>(m_L));
            break;
        }
        case ImageType::FLOAT: {
            combineLuminance(rgb, *reinterpret_cast<const Image32*>(m_L));
            break;
        }
        }
    }
    //max value of lum midtone is 0.5

    if (m_reduce_chrominance) {
        rgb.RGBtoCIELab();
        Image32 chrominance(rgb.rows(), rgb.cols(), 2);
        memcpy(chrominance.data.get(), &(*rgb.begin(1)), rgb.pxCount() * 2 * 4);
        //Wavelet().ChrominanceNoiseReduction(chrominance, m_layers_to_remove, m_layers_to_keep);
        memcpy(&rgb[rgb.pxCount()], chrominance.data.get(), rgb.pxCount() * 2 * 4);
        rgb.CIELabtoRGB();
    }

    return rgb;
}








LRGBCombinationDialog::LRGBCombinationDialog(QWidget* parent) : ProcessDialog("LRGBCombination", {400,500}, FastStack::recast(parent)->workspace(),false) {
    
    using LRGBCD = LRGBCombinationDialog;

    ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &LRGBCD::Apply, &LRGBCD::showPreview, &LRGBCD::resetDialog);

    m_toolbox = new QToolBox(this);
    m_toolbox->setBackgroundRole(QPalette::Window);
 
    m_pal.setColor(QPalette::ButtonText, Qt::white);
    m_toolbox->setPalette(m_pal);
    m_pal.setColor(QPalette::ButtonText, Qt::black);

    m_toolbox->move(10, 10);
    m_toolbox->setFixedWidth(380);


    //CollapsableSection* cs = new CollapsableSection("Image Selection", width() - 10 ,this);
    addImageSelection();
    //connect(cs, &CollapsableSection::clicked, this, [this](bool v) {m_image_selection_gb->setVisible(v); this->resize(QSize(400, height() + ((v) ? 200 : -200))); });
    m_toolbox->addItem(m_image_selection_gb, "Image Selection");

    addImageWeights();
    m_toolbox->addItem(m_image_weight_gb, "Image Weights");

    connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendClose, this, &LRGBCD::onWindowClose);
    connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendOpen, this, &LRGBCD::onWindowOpen);

    this->show();
}

void LRGBCombinationDialog::onWindowOpen() {

    auto iw = reinterpret_cast<ImageWindow8*>(m_workspace->subWindowList().last()->widget());

    if (iw->channels() == 1) {
        m_lum_combo->addItem(iw->name());
        m_red_combo->addItem(iw->name());
        m_green_combo->addItem(iw->name());
        m_blue_combo->addItem(iw->name());
    }
}

void LRGBCombinationDialog::onWindowClose() {

    QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->name();

    int index = m_lum_combo->findText(str);
    if (index == m_lum_combo->currentIndex()) {
        m_lc.setLum(nullptr);
        m_lum_combo->setCurrentIndex(0);
    }
    m_lum_combo->removeItem(index);

    index = m_red_combo->findText(str);
    if (index == m_red_combo->currentIndex()) {
        m_lc.setRed(nullptr);
        m_red_combo->setCurrentIndex(0);
    }
    m_red_combo->removeItem(index);


    index = m_green_combo->findText(str);
    if (index == m_green_combo->currentIndex()) {
        m_lc.setGreen(nullptr);
        m_green_combo->setCurrentIndex(0);
    }
    m_green_combo->removeItem(index);


    index = m_blue_combo->findText(str);
    if (index == m_blue_combo->currentIndex()) {
        m_lc.setBlue(nullptr);
        m_blue_combo->setCurrentIndex(0);
    }
    m_blue_combo->removeItem(index);
}

void LRGBCombinationDialog::addImageSelection() {


    m_image_selection_gb = new GroupBox(this);
    m_image_selection_layout = new QGridLayout;

    addLumInputs();
    addRedInputs();
    addGreenInputs();
    addBlueInputs();

    m_image_selection_gb->setLayout(m_image_selection_layout);
}

void LRGBCombinationDialog::addLumInputs() {

    m_lum_cb = new QCheckBox("Lum", this);
    m_lum_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_lum_cb, 0, 0);
    connect(m_lum_cb, &QCheckBox::clicked, this, [this](bool v) { m_lc.enableLum(v); });

    m_lum_combo = new ComboBox(this);
    m_lum_combo->setPalette(m_pal);
    m_lum_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_lum_combo, 0, 1);
    m_lum_combo->addItem("No Selected Image", -1);

    for (auto sw : m_workspace->subWindowList())
        m_lum_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

    auto activation = [this]() {
        for (auto sw : m_workspace->subWindowList()) {
            auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
            if (m_lum_combo->currentText() == iw->name())
                return m_lc.setLum(&iw->source());
        }
        m_lc.setLum(nullptr);
    };
    connect(m_lum_combo, &QComboBox::activated, this, activation);
}

void LRGBCombinationDialog::addRedInputs() {

    m_red_cb = new QCheckBox("Red", this);
    m_red_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_red_cb, 1, 0);
    connect(m_red_cb, &QCheckBox::clicked, this, [this](bool v) { m_lc.enableRed(v); });

    m_red_combo = new ComboBox(this);
    m_red_combo->setPalette(m_pal);
    m_red_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_red_combo, 1, 1);
    m_red_combo->addItem("No Selected Image", -1);

    for (auto sw : m_workspace->subWindowList())
        m_red_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

    auto activation = [this]() {
        for (auto sw : m_workspace->subWindowList()) {
            auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
            if (m_red_combo->currentText() == iw->name())
                return m_lc.setRed(&iw->source());
        }
        m_lc.setRed(nullptr);
    };
    connect(m_red_combo, &QComboBox::activated, this, activation);
}

void LRGBCombinationDialog::addGreenInputs() {

    m_green_cb = new QCheckBox("Green", this);

    m_green_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_green_cb, 2, 0);
    connect(m_green_cb, &QCheckBox::clicked, this, [this](bool v) { m_lc.enableGreen(v); });

    m_green_combo = new ComboBox(this);
    m_green_combo->setPalette(m_pal);
    m_green_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_green_combo, 2, 1);
    m_green_combo->addItem("No Selected Image", 0);

    for (auto sw : m_workspace->subWindowList())
        m_green_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

    auto activation = [this]() {
        for (auto sw : m_workspace->subWindowList()) {
            auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
            if (m_green_combo->currentText() == iw->name())
                return m_lc.setGreen(&iw->source());
        }
        m_lc.setGreen(nullptr);
    };
    connect(m_green_combo, &QComboBox::activated, this, activation);
}

void LRGBCombinationDialog::addBlueInputs() {

    m_blue_cb = new QCheckBox("Blue", this);
    m_blue_cb->setChecked(true);
    m_image_selection_layout->addWidget(m_blue_cb, 3, 0);
    connect(m_blue_cb, &QCheckBox::clicked, this, [this](bool v) { m_lc.enableBlue(v); });

    m_blue_combo = new ComboBox(this);
    m_blue_combo->setPalette(m_pal);
    m_blue_combo->setFixedWidth(250);
    m_image_selection_layout->addWidget(m_blue_combo, 3, 1);
    m_blue_combo->addItem("No Selected Image", 0);

    for (auto sw : m_workspace->subWindowList())
        m_blue_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

    auto activation = [this]() {
        for (auto sw : m_workspace->subWindowList()) {
            auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
            if (m_blue_combo->currentText() == iw->name())
                return m_lc.setBlue(&iw->source());
        }
        m_lc.setBlue(nullptr);
    };
    connect(m_blue_combo, &QComboBox::activated, this, activation);
}

void LRGBCombinationDialog::addImageWeights() {

    m_image_weight_gb = new GroupBox(this);

    m_image_weight_layout = new QGridLayout;
    addLWeightInputs();
    addRWeightInputs();
    addGWeightInputs();
    addBWeightInputs();

    m_image_weight_gb->setLayout(m_image_weight_layout);
}

void LRGBCombinationDialog::addLWeightInputs() {

    m_lum_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
    m_lum_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Lum", this), 0, 0);
    m_image_weight_layout->addWidget(m_lum_le, 0, 1);

    m_lum_slider = new Slider(Qt::Horizontal, this);
    m_lum_slider->setFixedWidth(200);
    m_lum_slider->setRange(0, 200);
    m_lum_slider->setValue(200);
    m_image_weight_layout->addWidget(m_lum_slider, 0, 2);

    auto ef = [this]() {
        double w = m_lum_le->value();
        m_lc.setLumWeight(w);
        m_lum_slider->setValue(w * 200); };
    connect(m_lum_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_lum_slider->sliderPosition() / 200.0;
        m_lc.setLumWeight(w);
        m_lum_le->setValue(w); };
    connect(m_lum_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCombinationDialog::addRWeightInputs() {

    m_red_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
    m_red_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Red", this), 1, 0);
    m_image_weight_layout->addWidget(m_red_le, 1, 1);

    m_red_slider = new Slider(Qt::Horizontal, this);
    m_red_slider->setFixedWidth(200);
    m_red_slider->setRange(0, 200);
    m_red_slider->setValue(200);
    m_image_weight_layout->addWidget(m_red_slider, 1, 2);

    auto ef = [this]() {
        double w = m_red_le->value(); 
        m_lc.setRedWeight(w); 
        m_red_slider->setValue(w * 200); };
    connect(m_red_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_red_slider->sliderPosition() / 200.0; 
        m_lc.setRedWeight(w);
        m_red_le->setValue(w); };
    connect(m_red_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCombinationDialog::addGWeightInputs() {

    m_green_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
    m_green_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Green", this), 2, 0);
    m_image_weight_layout->addWidget(m_green_le, 2, 1);

    m_green_slider = new Slider(Qt::Horizontal, this);
    m_green_slider->setFixedWidth(200);
    m_green_slider->setRange(0, 200);
    m_green_slider->setValue(200);
    m_image_weight_layout->addWidget(m_green_slider, 2, 2);

    auto ef = [this]() {
        double w = m_green_le->value();
        m_lc.setGreenWeight(w);
        m_green_slider->setValue(w * 200); };
    connect(m_green_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_green_slider->sliderPosition() / 200.0;
        m_lc.setGreenWeight(w);
        m_green_le->setValue(w); };
    connect(m_green_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCombinationDialog::addBWeightInputs() {

    m_blue_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
    m_blue_le->setValue(1.0);
    m_image_weight_layout->addWidget(new QLabel("Blue", this), 3, 0);
    m_image_weight_layout->addWidget(m_blue_le, 3, 1);

    m_blue_slider = new Slider(Qt::Horizontal, this);
    m_blue_slider->setFixedWidth(200);
    m_blue_slider->setRange(0, 200);
    m_blue_slider->setValue(200);
    m_image_weight_layout->addWidget(m_blue_slider, 3, 2);

    auto ef = [this]() {
        double w = m_blue_le->value();
        m_lc.setBlueWeight(w);
        m_blue_slider->setValue(w * 200); };
    connect(m_blue_le, &QLineEdit::editingFinished, this, ef);

    auto at = [this](int action) {
        double w = m_blue_slider->sliderPosition() / 200.0;
        m_lc.setBlueWeight(w);
        m_blue_le->setValue(w); };
    connect(m_blue_slider, &QSlider::actionTriggered, this, at);
}

void LRGBCombinationDialog::addChrominaceNRInputs() {

}

void LRGBCombinationDialog::resetDialog() {}

void LRGBCombinationDialog::Apply() {

    Status status = m_lc.isImagesSameSize();

    if (status) {
        Image32 lrgb = m_lc.generateLRGBImage();
        ImageWindow32* iw = new ImageWindow32(lrgb, "LRGB Image", m_workspace);
    }

    else
        QMessageBox::information(this, "", status.m_message);
}
