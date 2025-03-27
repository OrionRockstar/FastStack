#include "pch.h"
#include "AutoHistogramDialog.h"
#include "FastStack.h"

using AHD = AutoHistogramDialog;
AHD::AutoHistogramDialog(QWidget* parent) : ProcessDialog("AutoHistogram", QSize(710, 435), FastStack::recast(parent)->workspace()) {

    setTimerInterval(250);
    setPreviewMethod(this, &AHD::applytoPreview);
    connectToolbar(this, &AHD::apply, &AHD::showPreview, &AHD::resetDialog);

    addTargetMedianInputs();
    addHistogramClippingInputs();

    this->show();
}

void AHD::addTargetMedianInputs() {

    m_tgt_med_gb = new GroupBox("Target Median", drawArea());
    m_tgt_med_gb->setGeometry(10, 10, 690, 180);

    m_target_enable_cb = new CheckBox("Enable", m_tgt_med_gb);
    m_target_enable_cb->setChecked(true);
    m_target_enable_cb->move(20, 20);

    m_target_rgb_cb = new CheckBox("Join RGB", m_tgt_med_gb);
    m_target_rgb_cb->move(120, 20);

    m_stretch_combo = new ComboBox(m_tgt_med_gb);
    m_stretch_combo->move(525, 20);
    addLabel(m_stretch_combo, new QLabel("Stretch Method:   ", m_tgt_med_gb));
    m_stretch_combo->addItems({ "Gamma", "Log", "MTF" });

    auto activation = [this](int index) {
        m_ah.setStretchMethod(AutoHistogram::StretchMethod(index));
        applytoPreview();
    };

    connect(m_stretch_combo, &QComboBox::activated, this, activation);

    addTargetMedianInputs_Red();
    addTargetMedianInputs_Green();
    addTargetMedianInputs_Blue();

    auto onClicked = [this](bool v) {
        for (auto child : m_tgt_med_gb->children())
            if (child != m_target_enable_cb)
                reinterpret_cast<QWidget*>(child)->setEnabled(v);

        m_ah.enableStretching(v);
        applytoPreview();
    };

    connect(m_target_enable_cb, &QCheckBox::clicked, this, onClicked);

    connect(m_target_rgb_cb, &QCheckBox::clicked, this, &AutoHistogramDialog::joinTargetMedian);
    m_target_rgb_cb->click();
}

void AHD::addTargetMedianInputs_Red() {

    m_target_red_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::red), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_red_le->resize(85, m_target_red_le->size().height());
    m_target_red_le->move(115, 50);
    addLabel(m_target_red_le, new QLabel("Red:", m_tgt_med_gb));

    m_target_red_slider = new Slider(Qt::Horizontal, m_tgt_med_gb);
    m_target_red_slider->setSliderPosition(0.15 * 400);
    m_target_red_slider->setFixedWidth(400);
    m_target_red_slider->setRange(0, 400);
    m_target_red_le->addSlider(m_target_red_slider);

    auto action = [this](int) {
        float med = float(m_target_red_slider->sliderPosition()) / m_target_red_slider->maximum();
        m_target_red_le->setValue(med);

        if (m_target_rgb_cb->isChecked()) {
            updateJoinedTarget();
            m_ah.setTargetMedian(ColorComponent::rgb_k, med);
        }

        else
            m_ah.setTargetMedian(ColorComponent::red, med);

        startTimer();
    };

    auto edited = [this]() {
        float med = m_target_red_le->value();
        m_target_red_slider->setValue(med * m_target_red_slider->maximum());

        if (m_target_rgb_cb->isChecked()) {
            updateJoinedTarget();
            m_ah.setTargetMedian(ColorComponent::rgb_k, med);
        }

        else
            m_ah.setTargetMedian(ColorComponent::red, med);

        applytoPreview();
    };

    connect(m_target_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_red_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addTargetMedianInputs_Green() {

    m_target_green_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::green), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_green_le->resize(85, m_target_green_le->size().height());
    m_target_green_le->move(115, 90);
    addLabel(m_target_green_le, new QLabel("Green:", m_tgt_med_gb));

    m_target_green_slider = new Slider(Qt::Horizontal, m_tgt_med_gb);
    m_target_green_slider->setSliderPosition(0.15 * 400);
    m_target_green_slider->setFixedWidth(400);
    m_target_green_slider->setRange(0, 400);
    m_target_green_le->addSlider(m_target_green_slider);

    auto action = [this](int) {
        float med = float(m_target_green_slider->sliderPosition()) / m_target_green_slider->maximum();
        m_target_green_le->setValue(med);
        m_ah.setTargetMedian(ColorComponent::green, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_target_green_le->value();
        m_target_green_slider->setValue(med * m_target_green_slider->maximum());
        m_ah.setTargetMedian(ColorComponent::green, med);
        applytoPreview();
    };

    connect(m_target_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_green_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addTargetMedianInputs_Blue() {

    m_target_blue_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::blue), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_blue_le->resize(85, m_target_blue_le->size().height());
    m_target_blue_le->move(115, 130);
    addLabel(m_target_blue_le, new QLabel("Blue:", m_tgt_med_gb));

    m_target_blue_slider = new Slider(Qt::Horizontal, m_tgt_med_gb);
    m_target_blue_slider->setSliderPosition(0.15 * 400);
    m_target_blue_slider->setFixedWidth(400);
    m_target_blue_slider->setRange(0, 400);
    m_target_blue_le->addSlider(m_target_blue_slider);

    auto action = [this](int) {
        float med = float(m_target_blue_slider->sliderPosition()) / m_target_blue_slider->maximum();
        m_target_blue_le->setValue(med);
        m_ah.setTargetMedian(ColorComponent::blue, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_target_blue_le->value();
        m_target_blue_slider->setValue(med * m_target_blue_slider->maximum());
        m_ah.setTargetMedian(ColorComponent::blue, med);
        applytoPreview();
    };

    connect(m_target_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::joinTargetMedian(bool v) {

    if (v)
        updateJoinedTarget();

    m_target_green_le->setDisabled(v);
    m_target_green_slider->setDisabled(v);

    m_target_blue_le->setDisabled(v);
    m_target_blue_slider->setDisabled(v);
}

void AHD::updateJoinedTarget() {

    m_target_green_le->setValue(m_target_red_le->value());
    m_target_green_slider->setValue(m_target_red_slider->sliderPosition());

    m_target_blue_le->setValue(m_target_red_le->value());
    m_target_blue_slider->setValue(m_target_red_slider->sliderPosition());
}


void AHD::addHistogramClippingInputs() {

    m_hist_clip_gb = new GroupBox("Histogram Clipping", drawArea());
    m_hist_clip_gb->setGeometry(10, 200, 690, 220);

    m_hist_enable_cb = new CheckBox("Enable", m_hist_clip_gb);
    m_hist_enable_cb->setChecked(true);
    m_hist_enable_cb->move(20, 20);

    m_hist_rgb_cb = new CheckBox("Joing RGB", m_hist_clip_gb);
    m_hist_rgb_cb->move(120, 20);

    m_shadow_gb = new GroupBox("Shadow Clipping", m_hist_clip_gb);
    m_shadow_gb->setGeometry(10, 50, 330, 160);

    m_highlight_gb = new GroupBox("Highlight Clipping", m_hist_clip_gb);
    m_highlight_gb->setGeometry(350, 50, 330, 160);

    addHistogramClippingInputs_ShadowRed();
    addHistogramClippingInputs_ShadowGreen();
    addHistogramClippingInputs_ShadowBlue();
    addHistogramClippingInputs_HighlightRed();
    addHistogramClippingInputs_HighlightGreen();
    addHistogramClippingInputs_HighlightBlue();

    connect(m_hist_rgb_cb, &QCheckBox::clicked, this, &AutoHistogramDialog::joinHistogramClipping);
    m_hist_rgb_cb->click();

    auto checked = [this](bool v) {
        for (auto child : m_shadow_gb->children())
            reinterpret_cast<QWidget*>(child)->setEnabled(v);

        for (auto child : m_highlight_gb->children())
            reinterpret_cast<QWidget*>(child)->setEnabled(v);

        m_ah.enableHistogramClipping(v);
        applytoPreview();
    };

    connect(m_hist_enable_cb, &QCheckBox::clicked, this, checked);
}

void AHD::addHistogramClippingInputs_ShadowRed() {

    m_shadow_red_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_red_le->setFixedWidth(50);
    m_shadow_red_le->move(60, 30);
    addLabel(m_shadow_red_le, new QLabel("Red:", m_shadow_gb));

    m_shadow_red_slider = new Slider(Qt::Horizontal, m_shadow_gb);
    m_shadow_red_slider->setFixedWidth(200);
    m_shadow_red_slider->setRange(0, 200);
    m_shadow_red_slider->setValue(0.02 * m_shadow_red_slider->maximum());
    m_shadow_red_le->addSlider(m_shadow_red_slider);

    auto action = [this](int) {
        float med = float(m_shadow_red_slider->sliderPosition()) / m_shadow_red_slider->maximum();
        m_shadow_red_le->setValue(med);

        if (m_hist_rgb_cb->isChecked()) {
            updateJoinedHistogramClipping();
            m_ah.setShadowClipping(ColorComponent::rgb_k, med);
        }
        else
            m_ah.setShadowClipping(ColorComponent::red, med);

        startTimer();
    };

    auto edited = [this]() {
        float med = m_shadow_red_le->value();
        m_shadow_red_slider->setValue(med * m_shadow_red_slider->maximum());

        if (m_hist_rgb_cb->isChecked()) {
            updateJoinedHistogramClipping();
            m_ah.setShadowClipping(ColorComponent::rgb_k, med);
        }
        else
            m_ah.setShadowClipping(ColorComponent::red, med);

        applytoPreview();
    };

    connect(m_shadow_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_red_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addHistogramClippingInputs_ShadowGreen() {

    m_shadow_green_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_green_le->setFixedWidth(50);
    m_shadow_green_le->move(60, 70);
    addLabel(m_shadow_green_le, new QLabel("Green:", m_shadow_gb));

    m_shadow_green_slider = new Slider(Qt::Horizontal, m_shadow_gb);
    m_shadow_green_slider->setFixedWidth(200);
    m_shadow_green_slider->setRange(0, 200);
    m_shadow_green_slider->setValue(0.02 * m_shadow_green_slider->maximum());
    m_shadow_green_le->addSlider(m_shadow_green_slider);

    auto action = [this](int) {
        float med = float(m_shadow_green_slider->sliderPosition()) / m_shadow_green_slider->maximum();
        m_shadow_green_le->setValue(med);
        m_ah.setShadowClipping(ColorComponent::green, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_shadow_green_le->value();
        m_shadow_green_slider->setValue(med * m_shadow_green_slider->maximum());
        m_ah.setShadowClipping(ColorComponent::green, med);
        applytoPreview();
    };

    connect(m_shadow_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_green_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addHistogramClippingInputs_ShadowBlue() {

    m_shadow_blue_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_blue_le->setFixedWidth(50);
    m_shadow_blue_le->move(60, 110);
    addLabel(m_shadow_blue_le, new QLabel("Blue:", m_shadow_gb));

    m_shadow_blue_slider = new Slider(Qt::Horizontal, m_shadow_gb);
    m_shadow_blue_slider->setFixedWidth(200);
    m_shadow_blue_slider->setRange(0, 200);
    m_shadow_blue_slider->setValue(0.02 * m_shadow_blue_slider->maximum());
    m_shadow_blue_le->addSlider(m_shadow_blue_slider);

    auto action = [this](int) {
        float med = float(m_shadow_blue_slider->sliderPosition()) / m_shadow_blue_slider->maximum();
        m_shadow_blue_le->setValue(med);
        m_ah.setShadowClipping(ColorComponent::blue, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_shadow_blue_le->value();
        m_shadow_blue_slider->setValue(med * m_shadow_blue_slider->maximum());
        m_ah.setShadowClipping(ColorComponent::blue, med);
        applytoPreview();
    };

    connect(m_shadow_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addHistogramClippingInputs_HighlightRed() {

    m_highlight_red_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_red_le->setFixedWidth(50);
    m_highlight_red_le->move(60, 30);
    addLabel(m_highlight_red_le, new QLabel("Red:", m_highlight_gb));

    m_highlight_red_slider = new Slider(Qt::Horizontal, m_highlight_gb);
    m_highlight_red_slider->setFixedWidth(200);
    m_highlight_red_slider->setRange(0, 200);
    m_highlight_red_le->addSlider(m_highlight_red_slider);

    auto action = [this](int) {
        float med = float(m_highlight_red_slider->sliderPosition()) / m_highlight_red_slider->maximum();
        m_highlight_red_le->setValue(med);

        if (m_hist_rgb_cb->isChecked()) {
            updateJoinedHistogramClipping();
            m_ah.setHighlightClipping(ColorComponent::rgb_k, med);
        }
        else
            m_ah.setHighlightClipping(ColorComponent::red, med);

        startTimer();
    };

    auto edited = [this]() {
        float med = m_highlight_red_le->value();
        m_highlight_red_slider->setValue(med * m_highlight_red_slider->maximum());

        if (m_hist_rgb_cb->isChecked()) {
            updateJoinedHistogramClipping();
            m_ah.setHighlightClipping(ColorComponent::rgb_k, med);
        }
        else
            m_ah.setHighlightClipping(ColorComponent::red, med);

        applytoPreview();
    };

    connect(m_highlight_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_red_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addHistogramClippingInputs_HighlightGreen() {

    m_highlight_green_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_green_le->setFixedWidth(50);
    m_highlight_green_le->move(60, 70);
    addLabel(m_highlight_green_le, new QLabel("Green:", m_highlight_gb));

    m_highlight_green_slider = new Slider(Qt::Horizontal, m_highlight_gb);
    m_highlight_green_slider->setFixedWidth(200);
    m_highlight_green_slider->setRange(0, 200);
    m_highlight_green_le->addSlider(m_highlight_green_slider);

    auto action = [this](int) {
        float med = float(m_highlight_green_slider->sliderPosition()) / m_highlight_green_slider->maximum();
        m_highlight_green_le->setValue(med);
        m_ah.setHighlightClipping(ColorComponent::green, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_highlight_green_le->value();
        m_highlight_green_slider->setValue(med * m_highlight_green_slider->maximum());
        m_ah.setHighlightClipping(ColorComponent::green, med);
        applytoPreview();
    };

    connect(m_highlight_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_green_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::addHistogramClippingInputs_HighlightBlue() {

    m_highlight_blue_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_blue_le->setFixedWidth(50);
    m_highlight_blue_le->move(60, 110);
    addLabel(m_highlight_blue_le, new QLabel("Blue:", m_highlight_gb));

    m_highlight_blue_slider = new Slider(Qt::Horizontal, m_highlight_gb);
    m_highlight_blue_slider->setFixedWidth(200);
    m_highlight_blue_slider->setRange(0, 200);
    m_highlight_blue_le->addSlider(m_highlight_blue_slider);

    auto action = [this](int) {
        float med = float(m_highlight_blue_slider->sliderPosition()) / m_highlight_blue_slider->maximum();
        m_highlight_blue_le->setValue(med);
        m_ah.setHighlightClipping(ColorComponent::blue, med);
        startTimer();
    };

    auto edited = [this]() {
        float med = m_highlight_blue_le->value();
        m_highlight_blue_slider->setValue(med * m_highlight_blue_slider->maximum());
        m_ah.setHighlightClipping(ColorComponent::blue, med);
        applytoPreview();
    };

    connect(m_highlight_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AHD::joinHistogramClipping(bool v) {
    if (v)
        updateJoinedHistogramClipping();

    m_shadow_green_le->setDisabled(v);
    m_shadow_green_slider->setDisabled(v);

    m_shadow_blue_le->setDisabled(v);
    m_shadow_blue_slider->setDisabled(v);

    m_highlight_green_le->setDisabled(v);
    m_highlight_green_slider->setDisabled(v);

    m_highlight_blue_le->setDisabled(v);
    m_highlight_blue_slider->setDisabled(v);
}

void AHD::updateJoinedHistogramClipping() {
    m_shadow_green_le->setValue(m_shadow_red_le->value());
    m_shadow_green_slider->setValue(m_shadow_red_slider->sliderPosition());

    m_shadow_blue_le->setValue(m_shadow_red_le->value());
    m_shadow_blue_slider->setValue(m_shadow_red_slider->sliderPosition());

    m_highlight_green_le->setValue(m_highlight_red_le->value());
    m_highlight_green_slider->setValue(m_highlight_red_slider->sliderPosition());

    m_highlight_blue_le->setValue(m_highlight_red_le->value());
    m_highlight_blue_slider->setValue(m_highlight_red_slider->sliderPosition());
}


void AHD::showPreview() {
    ProcessDialog::showPreview();
    applytoPreview();
}

void AHD::resetDialog() {
    m_ah = AutoHistogram();

    m_target_enable_cb->setChecked(true);

    for (auto child : m_tgt_med_gb->children())
        reinterpret_cast<QWidget*>(child)->setEnabled(true);

    m_target_red_le->setValue(m_ah.targetMedian(ColorComponent::red));
    m_target_red_slider->setValue(m_ah.targetMedian(ColorComponent::red) * m_target_red_slider->maximum());

    m_target_rgb_cb->setChecked(true);
    joinTargetMedian(true);


    m_hist_enable_cb->setChecked(true);

    for (auto child : m_shadow_gb->children())
        reinterpret_cast<QWidget*>(child)->setEnabled(true);

    for (auto child : m_highlight_gb->children())
        reinterpret_cast<QWidget*>(child)->setEnabled(true);

    m_shadow_red_le->setValue(m_ah.shadowClipping(ColorComponent::red));
    m_shadow_red_slider->setValue(m_ah.shadowClipping(ColorComponent::red) * m_shadow_red_slider->maximum());

    m_highlight_red_le->setValue(m_ah.highlightClipping(ColorComponent::red));
    m_highlight_red_slider->setValue(m_ah.highlightClipping(ColorComponent::red) * m_highlight_red_slider->maximum());

    m_hist_rgb_cb->setChecked(true);
    joinHistogramClipping(true);

    m_stretch_combo->setCurrentIndex(0);

    applytoPreview();
}

void AHD::apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        iwptr->applyToSource(m_ah, &AutoHistogram::apply);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        iw16->applyToSource(m_ah, &AutoHistogram::apply);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        iw32->applyToSource(m_ah, &AutoHistogram::apply);
        break;
    }
    }

    applytoPreview();
}

void AHD::applytoPreview() {

    if (!isPreviewValid())
        return;

    auto iwptr = reinterpret_cast<PreviewWindow8*>(m_preview);

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        auto iw8 = iwptr;
        return iw8->updatePreview(m_ah, &AutoHistogram::apply);
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr);
        return iw16->updatePreview(m_ah, &AutoHistogram::apply);
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr);
        return iw32->updatePreview(m_ah, &AutoHistogram::apply);
    }
    }
}