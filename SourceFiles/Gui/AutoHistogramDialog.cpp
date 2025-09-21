#include "pch.h"
#include "AutoHistogramDialog.h"
#include "FastStack.h"

using AHD = AutoHistogramDialog;
AHD::AutoHistogramDialog(QWidget* parent) : ProcessDialog("AutoHistogram", QSize(710, 435), FastStack::recast(parent)->workspace()) {

    setDefaultTimerInterval(250);

    addTargetMedian();
    addHistogramClipping();

    this->show();
}

void AHD::addTargetMedian() {

    using CC = ColorComponent;

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

    addTargetMedianInputs();

    auto joined = [this](bool v) {
        v = !v;
        float t = m_target_red_inp->valuef();
        m_target_green_inp->setValue(t);
        m_target_green_inp->setEnabled(v);
        m_ah.setTargetMedian(CC::green, t);
        m_target_blue_inp->setValue(t);
        m_target_blue_inp->setEnabled(v);
        m_ah.setTargetMedian(CC::blue, t);
        applytoPreview();
    };
    connect(m_target_rgb_cb, &QCheckBox::toggled, this, joined);

    auto enabled = [this](bool v) {
        m_target_rgb_cb->setEnabled(v);
        m_target_red_inp->setEnabled(v);
        if (!m_target_rgb_cb->isChecked()) {
            m_target_green_inp->setEnabled(v);
            m_target_blue_inp->setEnabled(v);
        }
        m_ah.enableStretching(v);
        applytoPreview();
    };

    connect(m_target_enable_cb, &QCheckBox::toggled, this, enabled);
    m_target_rgb_cb->click();
}

void AHD::addTargetMedianInputs() {

    using CC = ColorComponent;
    auto validator = new DoubleValidator(0.0, 1.0, 3);
    m_target_red_inp = new DoubleInput("Red:   ", m_ah.targetMedian(CC::red), validator, m_tgt_med_gb, 400);
    m_target_red_inp->move(115, 50);

    m_target_green_inp = new DoubleInput("Green:   ", m_ah.targetMedian(CC::green), validator, m_tgt_med_gb, 400);
    m_target_green_inp->move(115, 90);

    m_target_blue_inp = new DoubleInput("Blue:   ", m_ah.targetMedian(CC::blue), validator, m_tgt_med_gb, 400);
    m_target_blue_inp->move(115, 130);

    for (auto child : m_tgt_med_gb->children()) {
        auto ptr = dynamic_cast<DoubleInput*>(child);
        if (ptr) {
            ptr->setSliderWidth(400);
            ptr->setLineEditWidth(85);
        }
    }

    auto red = [this]() {    
        float v = m_target_red_inp->valuef();
        if (m_target_rgb_cb->isChecked()) {
            m_target_green_inp->setValue(v);
            m_target_blue_inp->setValue(v);
            m_ah.setTargetMedian(ColorComponent::rgb_k, v);
        }
        else
            m_ah.setTargetMedian(ColorComponent::red, v);
    };

    connect(m_target_red_inp, &InputBase::actionTriggered, this, [this, red](int) { red(); startTimer(); });
    connect(m_target_red_inp, &InputBase::editingFinished, this, [this, red]() { red(); applytoPreview(); });

    connect(m_target_green_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setTargetMedian(CC::green, m_target_green_inp->valuef());
        startTimer(); });
    connect(m_target_green_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setTargetMedian(CC::green, m_target_green_inp->valuef());
        applytoPreview(); });

    connect(m_target_blue_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setTargetMedian(CC::blue, m_target_blue_inp->valuef());
        startTimer(); });
    connect(m_target_blue_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setTargetMedian(CC::blue, m_target_blue_inp->valuef());
        applytoPreview(); });
}

void AHD::addHistogramClipping() {

    using CC = ColorComponent;
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

    addHistogramClippingInputs_Shadow();
    addHistogramClippingInputs_Highlight();

    auto joined = [this](bool v) {
        v = !v;
        float s = m_shadow_red_inp->valuef();
        m_shadow_green_inp->setValue(s);
        m_shadow_green_inp->setEnabled(v);
        m_ah.setShadowClipping(CC::green, s);
        m_shadow_blue_inp->setValue(s);
        m_shadow_blue_inp->setEnabled(v);
        m_ah.setShadowClipping(CC::blue, s);

        float h = m_highlight_red_inp->valuef();
        m_highlight_green_inp->setValue(h);
        m_highlight_green_inp->setEnabled(v);
        m_ah.setHighlightClipping(CC::green, h);
        m_highlight_blue_inp->setValue(h);
        m_highlight_blue_inp->setEnabled(v);
        m_ah.setHighlightClipping(CC::blue, h);

        applytoPreview();
    };

    connect(m_hist_rgb_cb, &QCheckBox::toggled, this, joined);
    m_hist_rgb_cb->click();

    auto enabled = [this](bool v) {
        m_hist_rgb_cb->setEnabled(v);
        m_shadow_red_inp->setEnabled(v);
        m_highlight_red_inp->setEnabled(v);
        if (!m_hist_rgb_cb->isChecked()) {
            m_shadow_green_inp->setEnabled(v);
            m_shadow_blue_inp->setEnabled(v);
            m_highlight_green_inp->setEnabled(v);
            m_highlight_blue_inp->setEnabled(v);
        }
        m_ah.enableHistogramClipping(v);
        applytoPreview();
    };

    connect(m_hist_enable_cb, &QCheckBox::toggled, this, enabled);
}

void AHD::addHistogramClippingInputs_Shadow() {

    using CC = ColorComponent;
    auto validator = new DoubleValidator(0.0, 1.0, 3);

    m_shadow_red_inp = new DoubleInput("Red:  ", m_ah.shadowClipping(CC::red), validator, m_shadow_gb, 200);
    m_shadow_red_inp->move(60, 30);

    m_shadow_green_inp = new DoubleInput("Green:  ", m_ah.shadowClipping(CC::green), validator, m_shadow_gb, 200);
    m_shadow_green_inp->move(60,70);

    m_shadow_blue_inp = new DoubleInput("Blue:  ", m_ah.shadowClipping(CC::blue), validator, m_shadow_gb, 200);
    m_shadow_blue_inp->move(60, 110);

    for (auto child : m_shadow_gb->children()) {
        auto ptr = dynamic_cast<DoubleInput*>(child);
        if (ptr) {
            ptr->setSliderWidth(200);
            ptr->setLineEditWidth(50);
        }
    }

    auto red = [this]() {
        float v = m_shadow_red_inp->valuef();
        if (m_hist_rgb_cb->isChecked()) {
            m_shadow_green_inp->setValue(v);
            m_shadow_blue_inp->setValue(v);
            m_ah.setShadowClipping(CC::rgb_k, v);
        }
        else
            m_ah.setShadowClipping(CC::red, v);
    };

    connect(m_shadow_red_inp, &InputBase::actionTriggered, this, [this, red](int) { red(); startTimer(); });
    connect(m_shadow_red_inp, &InputBase::editingFinished, this, [this, red]() { red(); applytoPreview(); });

    connect(m_shadow_green_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setShadowClipping(CC::green, m_shadow_green_inp->valuef());
        startTimer(); });
    connect(m_shadow_green_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setShadowClipping(CC::green, m_shadow_green_inp->valuef());
        applytoPreview(); });

    connect(m_shadow_blue_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setShadowClipping(CC::blue, m_shadow_blue_inp->valuef());
        startTimer(); });
    connect(m_shadow_blue_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setShadowClipping(CC::blue, m_shadow_blue_inp->valuef());
        applytoPreview(); });
}

void AHD::addHistogramClippingInputs_Highlight() {

    using CC = ColorComponent;
    auto validator = new DoubleValidator(0.0, 1.0, 3);

    m_highlight_red_inp = new DoubleInput("Red:  ", m_ah.highlightClipping(CC::red), validator, m_highlight_gb, 200);
    m_highlight_red_inp->move(60, 30);
    m_highlight_green_inp = new DoubleInput("Green:  ", m_ah.highlightClipping(CC::green), validator, m_highlight_gb, 200);
    m_highlight_green_inp->move(60, 70);
    m_highlight_blue_inp = new DoubleInput("Blue:  ", m_ah.highlightClipping(CC::blue), validator, m_highlight_gb, 200);
    m_highlight_blue_inp->move(60, 110);

    for (auto child : m_highlight_gb->children()) {
        auto ptr = dynamic_cast<DoubleInput*>(child);
        if (ptr) {
            ptr->setSliderWidth(200);
            ptr->setLineEditWidth(50);
        }
    }

    auto red = [this]() {
        float v = m_highlight_red_inp->valuef();
        if (m_hist_rgb_cb->isChecked()) {
            m_highlight_green_inp->setValue(v);
            m_highlight_blue_inp->setValue(v);
            m_ah.setHighlightClipping(CC::rgb_k, v);
        }
        else
            m_ah.setHighlightClipping(CC::red, v);
    };

    connect(m_highlight_red_inp, &InputBase::actionTriggered, this, [this, red](int) { red(); startTimer(); });
    connect(m_highlight_red_inp, &InputBase::editingFinished, this, [this, red]() { red(); applytoPreview(); });

    connect(m_highlight_green_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setHighlightClipping(CC::green, m_highlight_green_inp->valuef());
        startTimer(); });
    connect(m_highlight_green_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setHighlightClipping(CC::green, m_highlight_green_inp->valuef());
        applytoPreview(); });

    connect(m_highlight_blue_inp, &InputBase::actionTriggered, this, [this](int) {
        m_ah.setHighlightClipping(CC::blue, m_highlight_blue_inp->valuef());
        startTimer(); });
    connect(m_highlight_blue_inp, &InputBase::editingFinished, this, [this]() {
        m_ah.setHighlightClipping(CC::blue, m_highlight_blue_inp->valuef());
        applytoPreview(); });
}

void AHD::resetDialog() {
    m_ah = AutoHistogram();

    m_target_red_inp->reset();
    m_target_green_inp->reset();
    m_target_green_inp->setEnabled(false);
    m_target_blue_inp->reset();
    m_target_blue_inp->setEnabled(false);

    m_target_rgb_cb->setChecked(true);
    m_target_enable_cb->setChecked(true);

    m_shadow_red_inp->reset();
    m_shadow_green_inp->reset();
    m_shadow_green_inp->setEnabled(false);
    m_shadow_blue_inp->reset();
    m_shadow_green_inp->setEnabled(false);

    m_highlight_red_inp->reset();
    m_highlight_green_inp->reset();
    m_highlight_green_inp->setEnabled(false);
    m_highlight_blue_inp->reset();
    m_highlight_green_inp->setEnabled(false);

    m_hist_rgb_cb->setChecked(true);
    m_hist_enable_cb->setChecked(true);

    m_stretch_combo->setCurrentIndex(0);

    applytoPreview();
}

void AHD::apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        return iwptr->applyToSource(m_ah, &AutoHistogram::apply);
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        return iw16->applyToSource(m_ah, &AutoHistogram::apply);
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        return iw32->applyToSource(m_ah, &AutoHistogram::apply);
    }
    }
}

void AHD::applyPreview() {

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