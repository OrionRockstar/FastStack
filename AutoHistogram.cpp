#include "pch.h"
#include "AutoHistogram.h"
#include "HistogramTransformation.h"
#include "FastStack.h"



float AutoHistogram::ShadowClipping(ColorComponent comp) {
    if (comp == ColorComponent::rgb_k)
        return m_shadow_clipping[0];
    else
        return m_shadow_clipping[int(comp)];
}

void AutoHistogram::setShadowClipping(ColorComponent comp, float percentage) {

    if (comp == ColorComponent::rgb_k)
        m_shadow_clipping = { percentage, percentage, percentage };
    else
        m_shadow_clipping[int(comp)] = percentage;
}

float AutoHistogram::HighlightClipping(ColorComponent comp) {
    if (comp == ColorComponent::rgb_k)
        return m_highlight_clipping[0];
    else
        return m_highlight_clipping[int(comp)];
}

void AutoHistogram::setHighlightClipping(ColorComponent comp, float percentage) {
    
    if (comp == ColorComponent::rgb_k)
        m_highlight_clipping = { percentage, percentage, percentage };
    else
        m_highlight_clipping[int(comp)] = percentage;
}

float AutoHistogram::TargetMedian(ColorComponent comp) {
    if (comp == ColorComponent::rgb_k)
        return m_target_median[0];
    else
        return m_target_median[int(comp)];
}

void AutoHistogram::setTargetMedian(ColorComponent comp, float tgt_median) {

    if (comp == ColorComponent::rgb_k)
        m_target_median = { tgt_median, tgt_median, tgt_median };

    else 
        m_target_median[int(comp)] = tgt_median;
}

float AutoHistogram::LTF(float pixel, float b) {
    return log(pixel * b + 1);
}

double AutoHistogram::ComputeLogMultiplier(float median, float tgt_median) {

    double xn = median;
    double xnm1 = tgt_median;

    auto ltf = [](double val, double b) {return log(1 + val * b); };

    for (int i = 0; i < 100; ++i) {

        double num = (ltf(median, xn) / ltf(1, xn)) - tgt_median;
        double denom = ((ltf(median, xn) / ltf(1, xn)) - (ltf(median, xnm1) / ltf(1, xnm1))) / (xn - xnm1);

        float xnp1 = xn - num / denom;

        if (isnan(xnp1) || isnan(xn))
            return -1;

        if (abs(xn - xnp1) < 0.00001)
            return xnp1;

        xnm1 = xn;
        xn = xnp1;
    }

    return 1.0;
}

template <typename T>
void AutoHistogram::Apply(Image<T>& img) {

    Histogram histogram;
    for (int ch = 0; ch < img.Channels(); ++ch) {

        histogram.ConstructHistogram(img.cbegin(ch), img.cend(ch));
        float median = Pixel<float>::toType(histogram.Median<T>());

        if (m_histogram_clipping) {

            int i = 0, count = 0;

            while (count < m_shadow_clipping[ch] * 0.01 * histogram.Count())
                count += histogram[i++];

            float shadow = float(i) / (histogram.Size() - 1);

            i = histogram.Size() - 1;
            count = 0;

            while (count < m_highlight_clipping[ch] * 0.01 * histogram.Count())
                count += histogram[i--];

            float highlight = float(i) / (histogram.Size() - 1);

            if (shadow >= median)
                median = 0;
            else if (median >= highlight)
                median = 1;
            else
                median = (median - shadow) / (highlight - shadow);

            T low = Pixel<T>::toType(shadow);
            T high = Pixel<T>::toType(highlight);

            img.Truncate(low, high, ch);
            img.Rescale(low, high, ch);
        }

        if (m_stretch) {
            switch (m_stretch_method) {

            case StretchMethod::log: {

                float b = ComputeLogMultiplier(median, m_target_median[ch]);

                if (b != -1.0f) {

                    float c = 1 / LTF(1, b);

                    if (!img.is_float()) {

                        std::vector<uint16_t> lut(Pixel<T>::max() + 1);

                        for (int i = 0; i < lut.size(); ++i)
                            lut[i] = c * LTF(float(i) / Pixel<T>::max(), b) * Pixel<T>::max();


                        for (T& pixel : image_channel(img, ch))
                            pixel = lut[pixel];
                    }

                    else {
                        for (T& pixel : image_channel(img, ch))
                            pixel = c * LTF(pixel, b);
                    }
                }

                break;

            }

            case StretchMethod::gamma: {

                float gamma = log10(m_target_median[ch]) / log10(median);

                if (!img.is_float()) {

                    std::vector<uint16_t> lut(Pixel<T>::max() + 1);

                    for (int i = 0; i < lut.size(); ++i)
                        lut[i] = pow(float(i) / Pixel<T>::max(), gamma) * Pixel<T>::max();

                    for (T& pixel : image_channel(img, ch))
                        pixel = lut[pixel];
                }

                else
                    for (T& pixel : image_channel(img, ch))
                        pixel = pow(pixel, gamma);

                break;
            }

            case StretchMethod::mtf: {

                HistogramTransformation ht;
                ColorComponent comp = ColorComponent::rgb_k;

                if (img.Channels() == 3)
                    comp = ColorComponent(ch);

                ht.setMidtone(comp, HistogramTransformation::MTF(median, m_target_median[ch]));
                ht.Apply(img);

                break;
            }
            }
        }
    }

}
template void AutoHistogram::Apply(Image8&);
template void AutoHistogram::Apply(Image16&);
template void AutoHistogram::Apply(Image32&);








using AHD = AutoHistogramDialog;
AHD::TargetMedian::TargetMedian(AutoHistogram& ah, QWidget* parent) : QWidget(parent), m_ah(&ah), m_parent(reinterpret_cast<AutoHistogramDialog*>(parent)) {
    this->resize(680, 160);
    this->setAutoFillBackground(true);
    this->setBackgroundRole(QPalette::ColorRole::Midlight);

    m_joined_rgb_cb = new QCheckBox("Joined RGB", this);
    m_joined_rgb_cb->setChecked(true);
    m_joined_rgb_cb->move(200, 10);
    connect(m_joined_rgb_cb, &QCheckBox::clicked, this, &TargetMedian::onClicked_joined);


    m_stretch_combo = new QComboBox(this);
    m_stretch_combo->move(350, 10);
    m_stretch_combo->addItems({ "Gamma Exponent", "Logarithmic Transformation", "MidtoneTransferFunction" });
    connect(m_stretch_combo, &QComboBox::activated, this, [this](int index) { m_ah->setStretchMethod(AutoHistogram::StretchMethod(index)); m_parent->ApplytoPreview(); });

    AddRedInputs();
    AddGreenInputs();
    AddBlueInputs();
    
    setDisabled_GreenBlue(true);
}

void AHD::TargetMedian::onActionTriggered_red(int action) {
    float med = float(m_red_slider->sliderPosition()) / m_red_slider->maximum();
    m_red_le->setValue(med);

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels();
        m_ah->setTargetMedian(ColorComponent::rgb_k, med);
    }

    else
        m_ah->setTargetMedian(ColorComponent::red, med);

    m_parent->startTimer();
}

void AHD::TargetMedian::onEditingFinished_red() {
    float med = m_red_le->Valuef();
    m_red_slider->setSliderPosition(med * m_red_slider->maximum());

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels();
        m_ah->setTargetMedian(ColorComponent::rgb_k, med);
    }
    else
        m_ah->setTargetMedian(ColorComponent::red, med);

    m_parent->ApplytoPreview();
}

void AHD::TargetMedian::onActionTriggered_green(int action) {
    float med = float(m_green_slider->sliderPosition()) / m_green_slider->maximum();
    m_green_le->setValue(med);
    m_ah->setTargetMedian(ColorComponent::green, med);
    m_parent->startTimer();
}

void AHD::TargetMedian::onEditingFinished_green() {
    float med = m_green_le->Valuef();
    m_green_slider->setSliderPosition(med * m_green_slider->maximum());
    m_ah->setTargetMedian(ColorComponent::green, med);
    m_parent->ApplytoPreview();
}

void AHD::TargetMedian::onActionTriggered_blue(int action) {
    float med = float(m_blue_slider->sliderPosition()) / m_blue_slider->maximum();
    m_blue_le->setValue(med);
    m_ah->setTargetMedian(ColorComponent::blue, med);
    m_parent->startTimer();
}

void AHD::TargetMedian::onEditingFinished_blue() {
    float med = m_blue_le->Valuef();
    m_blue_slider->setSliderPosition(med * m_blue_slider->maximum());
    m_ah->setTargetMedian(ColorComponent::blue, med);
    m_parent->ApplytoPreview();
}

void AHD::TargetMedian::onClicked_joined(bool v) {
    setDisabled_GreenBlue(v);

    m_ah->setTargetMedian(ColorComponent::rgb_k, m_red_le->Valuef());

    m_parent->ApplytoPreview();
}

void AHD::TargetMedian::updateJoinedChannels() {

    m_green_le->setValue(m_red_le->Valuef());
    m_green_slider->setSliderPosition(m_red_slider->sliderPosition());
    

    m_blue_le->setValue(m_red_le->Valuef());
    m_blue_slider->setSliderPosition(m_red_slider->sliderPosition());    
}

void AHD::TargetMedian::setDisabled_GreenBlue(bool val) {
    m_green_le->setDisabled(val);
    m_green_slider->setDisabled(val);

    m_blue_le->setDisabled(val);
    m_blue_slider->setDisabled(val);

    updateJoinedChannels();
}

void AHD::TargetMedian::AddRedInputs() {

    m_red_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
    m_red_le->setValue(0.15);
    m_red_le->resize(85, m_red_le->size().height());
    m_red_le->move(110, 50);
    m_red_le->addLabel(new QLabel("Red:   ", this));

    m_red_slider = new QSlider(Qt::Horizontal, this);
    m_red_slider->setSliderPosition(0.15 * 400);
    m_red_slider->setFixedWidth(400);
    m_red_slider->setRange(0, 400);
    m_red_le->addSlider(m_red_slider);

    connect(m_red_le, &DoubleLineEdit::editingFinished, this, &TargetMedian::onEditingFinished_red);
    connect(m_red_slider, &QSlider::actionTriggered, this, &TargetMedian::onActionTriggered_red);
}

void AHD::TargetMedian::AddGreenInputs() {

    m_green_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
    m_green_le->setValue(0.15);
    m_green_le->resize(85, m_green_le->size().height());
    m_green_le->move(110, 85);
    m_green_le->addLabel(new QLabel("Green:   ", this));

    m_green_slider = new QSlider(Qt::Horizontal, this);
    m_green_slider->setSliderPosition(0.15 * 400);
    m_green_slider->setFixedWidth(400);
    m_green_slider->setRange(0, 400);
    m_green_le->addSlider(m_green_slider);

    connect(m_green_le, &DoubleLineEdit::editingFinished, this, &TargetMedian::onEditingFinished_green);
    connect(m_green_slider, &QSlider::actionTriggered, this, &TargetMedian::onActionTriggered_green);
}

void AHD::TargetMedian::AddBlueInputs() {

    m_blue_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
    m_blue_le->setValue(0.15);
    m_blue_le->resize(85, m_blue_le->size().height());
    m_blue_le->move(110, 120);
    m_blue_le->addLabel(new QLabel("Blue:   ", this));

    m_blue_slider = new QSlider(Qt::Horizontal, this);
    m_blue_slider->setSliderPosition(0.15 * 400);
    m_blue_slider->setFixedWidth(400);
    m_blue_slider->setRange(0, 400);
    m_blue_le->addSlider(m_blue_slider);

    connect(m_blue_le, &DoubleLineEdit::editingFinished, this, &TargetMedian::onEditingFinished_blue);
    connect(m_blue_slider, &QSlider::actionTriggered, this, &TargetMedian::onActionTriggered_blue);
}

void AHD::TargetMedian::reset() {
    m_stretch_combo->setCurrentIndex(int(AutoHistogram::StretchMethod::gamma));

    float v = m_ah->TargetMedian(ColorComponent::red);
    m_red_le->setValue(v);
    m_red_slider->setSliderPosition(v * m_red_slider->maximum());

    v = m_ah->TargetMedian(ColorComponent::green);
    m_green_le->setValue(v);
    m_green_slider->setSliderPosition(v * m_green_slider->maximum());

    v = m_ah->TargetMedian(ColorComponent::blue);
    m_blue_le->setValue(v);
    m_blue_slider->setSliderPosition(v * m_blue_slider->maximum());
}




AHD::HistogramClipping::HistogramClipping(AutoHistogram& ah, QWidget* parent) : QWidget(parent), m_ah(&ah), m_parent(dynamic_cast<AutoHistogramDialog*>(parent)) {
    this->resize(680, 185);
    //this->setAutoFillBackground(true);
    //this->setBackgroundRole(QPalette::ColorRole::Dark);

    m_joined_rgb_cb = new QCheckBox("Joined RGB", this);
    m_joined_rgb_cb->setChecked(true);
    m_joined_rgb_cb->move(200, 10);
    connect(m_joined_rgb_cb, &QCheckBox::clicked, this, &HistogramClipping::onClicked_joined);

    QLabel* l = new QLabel("Shadow Clipping", this);
    l->move(60, 45);
    AddRedShadowInputs();
    AddGreenShadowInputs();
    AddBlueShadowInputs();

    l = new QLabel("Highlight Clipping", this);
    l->move(400, 45);
    AddRedHighlightInputs();
    AddGreenHighlightInputs();
    AddBlueHighlightInputs();

    setDisabled_GreenBlueShadow(m_joined_rgb_cb->isChecked());
    setDisabled_GreenBlueHighlight(m_joined_rgb_cb->isChecked());

}

void AHD::HistogramClipping::onActionTriggered_redShadow(int action) {
    float perc = float(m_redShadow_slider->sliderPosition()) / m_redShadow_slider->maximum();
    m_redShadow_le->setValue(perc);

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels_shadow();
        m_ah->setShadowClipping(ColorComponent::rgb_k, perc);
    }
    else
        m_ah->setShadowClipping(ColorComponent::red, perc);

    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_redShadow() {
    float perc = m_redShadow_le->Valuef();
    m_redShadow_slider->setSliderPosition(perc * m_redShadow_slider->maximum());

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels_shadow();
        m_ah->setShadowClipping(ColorComponent::rgb_k, perc);
    }
    else
        m_ah->setShadowClipping(ColorComponent::red, perc);


    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::onActionTriggered_greenShadow(int action) {
    float perc = float(m_greenShadow_slider->sliderPosition()) / m_greenShadow_slider->maximum();
    m_greenShadow_le->setValue(perc);
    m_ah->setShadowClipping(ColorComponent::green, perc);
    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_greenShadow() {
    float perc = m_greenShadow_le->Valuef();
    m_greenShadow_slider->setSliderPosition(perc * m_greenShadow_slider->maximum());
    m_ah->setShadowClipping(ColorComponent::green, perc);
    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::onActionTriggered_blueShadow(int action) {
    float perc = float(m_blueShadow_slider->sliderPosition()) / m_blueShadow_slider->maximum();
    m_blueShadow_le->setValue(perc);
    m_ah->setShadowClipping(ColorComponent::blue, perc);
    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_blueShadow() {
    float perc = m_blueShadow_le->Valuef();
    m_blueShadow_slider->setSliderPosition(perc * m_blueShadow_slider->maximum());
    m_ah->setShadowClipping(ColorComponent::blue, perc);
    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::onClicked_joined(bool v) {

    setDisabled_GreenBlueShadow(v);
    setDisabled_GreenBlueHighlight(v);

    //m_ah->set;
    m_ah->setShadowClipping(ColorComponent::rgb_k, m_redShadow_le->Valuef());
    m_ah->setHighlightClipping(ColorComponent::rgb_k, m_redHighlight_le->Valuef());

    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::setDisabled_GreenBlueShadow(bool val) {
    m_greenShadow_le->setDisabled(val);
    m_greenShadow_slider->setDisabled(val);

    m_blueShadow_le->setDisabled(val);
    m_blueShadow_slider->setDisabled(val);

    updateJoinedChannels_shadow();
}

void AHD::HistogramClipping::updateJoinedChannels_shadow() {

    m_greenShadow_le->setValue(m_redShadow_le->Valuef());
    m_greenShadow_slider->setSliderPosition(m_redShadow_slider->sliderPosition());


    m_blueShadow_le->setValue(m_redShadow_le->Valuef());
    m_blueShadow_slider->setSliderPosition(m_redShadow_slider->sliderPosition());
}

void AHD::HistogramClipping::AddRedShadowInputs() {

    float v = m_ah->ShadowClipping(ColorComponent::red);

    m_redShadow_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_redShadow_le->setValue(v);
    m_redShadow_le->resize(50, m_redShadow_le->size().height());
    m_redShadow_le->move(60, 75);
    m_redShadow_le->addLabel(new QLabel("Red:   ", this));

    m_redShadow_slider = new QSlider(Qt::Horizontal, this);
    m_redShadow_slider->setFixedWidth(200);
    m_redShadow_slider->setRange(0, 200);
    m_redShadow_slider->setSliderPosition(v * m_redShadow_slider->maximum());
    m_redShadow_le->addSlider(m_redShadow_slider);


    connect(m_redShadow_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_redShadow);
    connect(m_redShadow_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_redShadow);
    //connect(m_redShadow_slider, &QSlider::sliderMoved, this, &HistogramClipping::onSliderMoved_redShadow);
}

void AHD::HistogramClipping::AddGreenShadowInputs() {

    float v = m_ah->ShadowClipping(ColorComponent::green);

    m_greenShadow_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_greenShadow_le->setValue(v);
    m_greenShadow_le->resize(50, m_greenShadow_le->size().height());
    m_greenShadow_le->move(60, 110);
    m_greenShadow_le->addLabel(new QLabel("Green:   ", this));

    m_greenShadow_slider = new QSlider(Qt::Horizontal, this);
    m_greenShadow_slider->setFixedWidth(200);
    m_greenShadow_slider->setRange(0, 200);
    m_greenShadow_slider->setSliderPosition(v * m_greenShadow_slider->maximum());
    m_greenShadow_le->addSlider(m_greenShadow_slider);

    connect(m_greenShadow_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_greenShadow);
    connect(m_greenShadow_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_greenShadow);
    //connect(m_greenShadow_slider, &QSlider::sliderMoved, this, &HistogramClipping::onSliderMoved_greenShadow);
}

void AHD::HistogramClipping::AddBlueShadowInputs() {

    float v = m_ah->ShadowClipping(ColorComponent::blue);

    m_blueShadow_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_blueShadow_le->setValue(v);
    m_blueShadow_le->resize(50, m_blueShadow_le->size().height());
    m_blueShadow_le->move(60, 145);
    m_blueShadow_le->addLabel(new QLabel("Blue:   ", this));

    m_blueShadow_slider = new QSlider(Qt::Horizontal, this);
    m_blueShadow_slider->setFixedWidth(200);
    m_blueShadow_slider->setRange(0, 200);
    m_blueShadow_slider->setSliderPosition(v * m_blueShadow_slider->maximum());
    m_blueShadow_le->addSlider(m_blueShadow_slider);

    connect(m_blueShadow_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_blueShadow);
    connect(m_blueShadow_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_blueShadow);
    //connect(m_blueShadow_slider, &QSlider::sliderMoved, this, &HistogramClipping::onSliderMoved_blueShadow);
}





void AHD::HistogramClipping::onActionTriggered_redHighlight(int action) {
    float perc = float(m_redHighlight_slider->sliderPosition()) / m_redHighlight_slider->maximum();
    m_redHighlight_le->setValue(perc);

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels_Highlight();
        m_ah->setHighlightClipping(ColorComponent::rgb_k, perc);
    }
    else
        m_ah->setHighlightClipping(ColorComponent::red, perc);

    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_redHighlight() {
    float perc = m_redHighlight_le->Valuef();
    m_redHighlight_slider->setSliderPosition(perc * m_redHighlight_slider->maximum());

    if (m_joined_rgb_cb->isChecked()) {
        updateJoinedChannels_Highlight();
        m_ah->setHighlightClipping(ColorComponent::rgb_k, perc);
    }
    else
        m_ah->setHighlightClipping(ColorComponent::red, perc);


    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::onActionTriggered_greenHighlight(int action) {
    float perc = float(m_greenHighlight_slider->sliderPosition()) / m_greenHighlight_slider->maximum();
    m_greenHighlight_le->setValue(perc);
    m_ah->setHighlightClipping(ColorComponent::green, perc);
    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_greenHighlight() {
    float perc = m_greenHighlight_le->Valuef();
    m_greenHighlight_slider->setSliderPosition(perc * m_greenHighlight_slider->maximum());
    m_ah->setHighlightClipping(ColorComponent::green, perc);
    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::onActionTriggered_blueHighlight(int action) {
    float perc = float(m_blueHighlight_slider->sliderPosition()) / m_blueHighlight_slider->maximum();
    m_blueHighlight_le->setValue(perc);
    m_ah->setHighlightClipping(ColorComponent::blue, perc);
    m_parent->startTimer();
}

void AHD::HistogramClipping::onEditingFinished_blueHighlight() {
    float perc = m_blueHighlight_le->Valuef();
    m_blueHighlight_slider->setSliderPosition(perc * m_blueHighlight_slider->maximum());
    m_ah->setHighlightClipping(ColorComponent::blue, perc);
    m_parent->ApplytoPreview();
}

void AHD::HistogramClipping::updateJoinedChannels_Highlight() {
    m_greenHighlight_le->setValue(m_redHighlight_le->Valuef());
    m_greenHighlight_slider->setSliderPosition(m_redHighlight_slider->sliderPosition());


    m_blueHighlight_le->setValue(m_redHighlight_le->Valuef());
    m_blueHighlight_slider->setSliderPosition(m_redHighlight_slider->sliderPosition());
}

void AHD::HistogramClipping::setDisabled_GreenBlueHighlight(bool val) {
    m_greenHighlight_le->setDisabled(val);
    m_greenHighlight_slider->setDisabled(val);

    m_blueHighlight_le->setDisabled(val);
    m_blueHighlight_slider->setDisabled(val);

    updateJoinedChannels_Highlight();
}

void AHD::HistogramClipping::AddRedHighlightInputs() {

    float v = m_ah->HighlightClipping(ColorComponent::red);

    m_redHighlight_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_redHighlight_le->setValue(v);
    m_redHighlight_le->resize(50, m_redHighlight_le->size().height());
    m_redHighlight_le->move(400, 75);
    m_redHighlight_le->addLabel(new QLabel("Red:   ", this));

    m_redHighlight_slider = new QSlider(Qt::Horizontal, this);
    m_redHighlight_slider->setFixedWidth(200);
    m_redHighlight_slider->setRange(0, 200);
    m_redHighlight_slider->setSliderPosition(v * m_redHighlight_slider->maximum());
    m_redHighlight_le->addSlider(m_redHighlight_slider);

    connect(m_redHighlight_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_redHighlight);
    connect(m_redHighlight_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_redHighlight);
}

void AHD::HistogramClipping::AddGreenHighlightInputs() {

    float v = m_ah->HighlightClipping(ColorComponent::green);

    m_greenHighlight_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_greenHighlight_le->setValue(v);
    m_greenHighlight_le->resize(50, m_greenHighlight_le->size().height());
    m_greenHighlight_le->move(400, 110);
    m_greenHighlight_le->addLabel(new QLabel("Green:   ", this));

    m_greenHighlight_slider = new QSlider(Qt::Horizontal, this);
    m_greenHighlight_slider->setFixedWidth(200);
    m_greenHighlight_slider->setRange(0, 200);
    m_greenHighlight_slider->setSliderPosition(v * m_greenHighlight_slider->maximum());
    m_greenHighlight_le->addSlider(m_greenHighlight_slider);

    connect(m_greenHighlight_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_greenHighlight);
    connect(m_greenHighlight_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_greenHighlight);
}

void AHD::HistogramClipping::AddBlueHighlightInputs() {

    float v = m_ah->HighlightClipping(ColorComponent::blue);

    m_blueHighlight_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
    m_blueHighlight_le->setValue(v);
    m_blueHighlight_le->resize(50, m_blueHighlight_le->size().height());
    m_blueHighlight_le->move(400, 145);
    m_blueHighlight_le->addLabel(new QLabel("Blue:   ", this));

    m_blueHighlight_slider = new QSlider(Qt::Horizontal, this);
    m_blueHighlight_slider->setFixedWidth(200);
    m_blueHighlight_slider->setRange(0, 200);
    m_blueHighlight_slider->setSliderPosition(v * m_blueHighlight_slider->maximum());
    m_blueHighlight_le->addSlider(m_blueHighlight_slider);

    connect(m_blueHighlight_le, &DoubleLineEdit::editingFinished, this, &HistogramClipping::onEditingFinished_blueHighlight);
    connect(m_blueHighlight_slider, &QSlider::actionTriggered, this, &HistogramClipping::onActionTriggered_blueHighlight);
}

void AHD::HistogramClipping::reset() {
    float v = m_ah->ShadowClipping(ColorComponent::red);
    m_redShadow_le->setValue(v);
    m_redShadow_slider->setSliderPosition(v * m_redShadow_slider->maximum());

    v = m_ah->ShadowClipping(ColorComponent::green);
    m_greenShadow_le->setValue(v);
    m_greenShadow_slider->setSliderPosition(v * m_greenShadow_slider->maximum());

    v = m_ah->ShadowClipping(ColorComponent::blue);
    m_blueShadow_le->setValue(v);
    m_blueShadow_slider->setSliderPosition(v * m_blueShadow_slider->maximum());


    v = m_ah->HighlightClipping(ColorComponent::red);
    m_redHighlight_le->setValue(v);
    m_redHighlight_slider->setSliderPosition(v * m_redHighlight_slider->maximum());

    v = m_ah->HighlightClipping(ColorComponent::green);
    m_greenHighlight_le->setValue(v);
    m_greenHighlight_slider->setSliderPosition(v * m_greenHighlight_slider->maximum());

    v = m_ah->HighlightClipping(ColorComponent::blue);
    m_blueHighlight_le->setValue(v);
    m_blueHighlight_slider->setSliderPosition(v * m_blueHighlight_slider->maximum());
}




AutoHistogramDialog::AutoHistogramDialog(QWidget* parent) : ProcessDialog("AutoHistogram", QSize(680, 370), *reinterpret_cast<FastStack*>(parent)->workspace(), parent) {

    setTimer(250, this, &AutoHistogramDialog::ApplytoPreview);

    connect(this, &ProcessDialog::processDropped, this, &AutoHistogramDialog::Apply);
    ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &AutoHistogramDialog::Apply, &AutoHistogramDialog::showPreview, &AutoHistogramDialog::resetDialog);

    m_tgt_median = new TargetMedian(m_ah, this);

    m_hist_clip = new HistogramClipping(m_ah, this);
    m_hist_clip->move(0, 161);

    m_target_median_cb = new QCheckBox("Target Median", this);
    m_target_median_cb->setChecked(true);
    m_target_median_cb->move(10, 10);
    connect(m_target_median_cb, &QCheckBox::clicked, this, &AutoHistogramDialog::onClicked_tergetMedian);


    m_histogram_clipping_cb = new QCheckBox("Histogram Clipping", this);
    m_histogram_clipping_cb->setChecked(true);
    m_histogram_clipping_cb->move(10, 171);
    connect(m_histogram_clipping_cb, &QCheckBox::clicked, this, &AutoHistogramDialog::onClicked_histogramClipping);

    this->show();
}

void AutoHistogramDialog::onClicked_tergetMedian(bool v) {
    m_tgt_median->setEnabled(v);
    m_ah.enableStretching(v);
    ApplytoPreview();
}

void AutoHistogramDialog::onClicked_histogramClipping(bool v) {
    m_hist_clip->setEnabled(v);
    m_ah.enableHistogramClipping(v);
    ApplytoPreview();
}

void AutoHistogramDialog::showPreview() {
    ProcessDialog::showPreview();
    ApplytoPreview();
}

void AutoHistogramDialog::resetDialog() {
    m_ah = AutoHistogram();
    m_tgt_median->reset();
    m_hist_clip->reset();
}

void AutoHistogramDialog::Apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->Source().Bitdepth()) {
    case 8: {
        iwptr->UpdateImage(m_ah, &AutoHistogram::Apply);
        break;
    }
    case 16: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        iw16->UpdateImage(m_ah, &AutoHistogram::Apply);
        break;
    }
    case -32: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        iw32->UpdateImage(m_ah, &AutoHistogram::Apply);
        break;
    }
    }

    ApplytoPreview();

}

void AutoHistogramDialog::ApplytoPreview() {

    if (!isPreviewValid())
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->Source().Bitdepth()) {
    case 8: {
        auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
        return iw8->UpdatePreview(m_ah, &AutoHistogram::Apply);
    }
    case 16: {
        auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
        return iw16->UpdatePreview(m_ah, &AutoHistogram::Apply);
    }
    case -32: {
        auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
        return iw32->UpdatePreview(m_ah, &AutoHistogram::Apply);
    }
    }
}