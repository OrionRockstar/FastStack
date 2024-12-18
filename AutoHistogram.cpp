#include "pch.h"
#include "AutoHistogram.h"
#include "HistogramTransformation.h"
#include "Histogram.h"
#include "FastStack.h"



float AutoHistogram::shadowClipping(ColorComponent comp)const {
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

float AutoHistogram::highlightClipping(ColorComponent comp)const {
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

float AutoHistogram::targetMedian(ColorComponent comp)const {
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

template<typename T>
static void rescaleChannel(Image<T>& img, T a, T b, uint32_t channel) {

    if (b == a)
        return;

    float dba = 1 / float(b - a);

    for (T& pixel : image_channel(img, channel)) {

        if (pixel < a)
            pixel = 0;

        else if (pixel > b)
            pixel = b;

        else
            pixel = T((pixel - a) * dba);
    }
}

template<typename T>
static void truncateChannel(Image<T>& img, T a, T b, uint32_t channel) {

    for(T& pixel : image_channel(img, channel)) {
        if (pixel < a)
            pixel = a;
        else if (pixel > b)
            pixel = b;
    }
}

template <typename T>
void AutoHistogram::Apply(Image<T>& img) {

    Histogram histogram;
    for (int ch = 0; ch < img.channels(); ++ch) {

        histogram.constructHistogram(img, ch);
        float median = Pixel<float>::toType(histogram.Median<T>());

        if (m_histogram_clipping) {

            int i = 0, count = 0;

            while (count < m_shadow_clipping[ch] * 0.01 * histogram.count())
                count += histogram[i++];

            float shadow = float(i) / (histogram.resolution() - 1);

            i = histogram.resolution() - 1;
            count = 0;

            while (count < m_highlight_clipping[ch] * 0.01 * histogram.count())
                count += histogram[i--];

            float highlight = float(i) / (histogram.resolution() - 1);

            if (shadow >= median)
                median = 0;
            else if (median >= highlight)
                median = 1;
            else
                median = (median - shadow) / (highlight - shadow);

            T low = Pixel<T>::toType(shadow);
            T high = Pixel<T>::toType(highlight);

            truncateChannel(img, low, high, ch);
            rescaleChannel(img, low, high, ch);
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

                if (img.channels() == 3)
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









AutoHistogramDialog::AutoHistogramDialog(QWidget* parent) : ProcessDialog("AutoHistogram", QSize(710, 455), FastStack::recast(parent)->workspace()) {

    setTimer(250, this, &AutoHistogramDialog::ApplytoPreview);

    connect(this, &ProcessDialog::processDropped, this, &AutoHistogramDialog::Apply);
    ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &AutoHistogramDialog::Apply, &AutoHistogramDialog::showPreview, &AutoHistogramDialog::resetDialog);

    addTargetMedianInputs();
    addHistogramClippingInputs();

    this->show();
}

void AutoHistogramDialog::addTargetMedianInputs() {

    m_tgt_med_gb = new GroupBox("Target Median", this);
    m_tgt_med_gb->setGeometry(10, 10, 690, 180);

    m_target_enable_cb = new CheckBox("Enable", m_tgt_med_gb);
    m_target_enable_cb->setChecked(true);
    m_target_enable_cb->move(20, 20);
    
    m_target_rgb_cb = new CheckBox("Join RGB", m_tgt_med_gb);
    m_target_rgb_cb->move(120, 20);

    m_stretch_combo = new ComboBox(m_tgt_med_gb);
    m_stretch_combo->move(525, 20);
    m_stretch_combo->addLabel(new QLabel("Stretch Method:   ", m_tgt_med_gb));
    m_stretch_combo->addItems({ "Gamma", "Log", "MTF" });

    auto activation = [this](int index) {
        m_ah.setStretchMethod(AutoHistogram::StretchMethod(index));
        ApplytoPreview();
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
        ApplytoPreview();
    };

    connect(m_target_enable_cb, &QCheckBox::clicked, this, onClicked);

    connect(m_target_rgb_cb, &QCheckBox::clicked, this, &AutoHistogramDialog::joinTargetMedian);
    m_target_rgb_cb->click();
}

void AutoHistogramDialog::addTargetMedianInputs_Red() {

    m_target_red_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::red), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_red_le->resize(85, m_target_red_le->size().height());
    m_target_red_le->move(115, 50);
    m_target_red_le->addLabel(new QLabel("Red:   ", m_tgt_med_gb));

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

        ApplytoPreview();
    };

    connect(m_target_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_red_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addTargetMedianInputs_Green() {

    m_target_green_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::green), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_green_le->resize(85, m_target_green_le->size().height());
    m_target_green_le->move(115, 90);
    m_target_green_le->addLabel(new QLabel("Green:   ", m_tgt_med_gb));

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
        ApplytoPreview();
    };

    connect(m_target_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_green_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addTargetMedianInputs_Blue() {

    m_target_blue_le = new DoubleLineEdit(m_ah.targetMedian(ColorComponent::blue), new DoubleValidator(0.0, 1.0, 6), m_tgt_med_gb);
    m_target_blue_le->resize(85, m_target_blue_le->size().height());
    m_target_blue_le->move(115, 130);
    m_target_blue_le->addLabel(new QLabel("Blue:   ", m_tgt_med_gb));

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
        ApplytoPreview();
    };

    connect(m_target_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_target_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::joinTargetMedian(bool v) {
    if (v)
        updateJoinedTarget();
    
    m_target_green_le->setDisabled(v);
    m_target_green_slider->setDisabled(v);

    m_target_blue_le->setDisabled(v);
    m_target_blue_slider->setDisabled(v);
}

void AutoHistogramDialog::updateJoinedTarget() {

    m_target_green_le->setValue(m_target_red_le->value());
    m_target_green_slider->setValue(m_target_red_slider->sliderPosition());

    m_target_blue_le->setValue(m_target_red_le->value());
    m_target_blue_slider->setValue(m_target_red_slider->sliderPosition());
}



void AutoHistogramDialog::addHistogramClippingInputs() {

    m_hist_clip_gb = new GroupBox("Histogram Clipping", this);
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
        ApplytoPreview();
    };

    connect(m_hist_enable_cb, &QCheckBox::clicked, this, checked);
}

void AutoHistogramDialog::addHistogramClippingInputs_ShadowRed() {

    m_shadow_red_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_red_le->setFixedWidth(50);
    m_shadow_red_le->move(60,30);
    m_shadow_red_le->addLabel(new QLabel("Red:   ", m_shadow_gb));

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

        ApplytoPreview();
    };

    connect(m_shadow_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_red_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addHistogramClippingInputs_ShadowGreen() {

    m_shadow_green_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_green_le->setFixedWidth(50);
    m_shadow_green_le->move(60, 70);
    m_shadow_green_le->addLabel(new QLabel("Green:   ", m_shadow_gb));

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
        ApplytoPreview();
    };

    connect(m_shadow_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_green_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addHistogramClippingInputs_ShadowBlue() {

    m_shadow_blue_le = new DoubleLineEdit(0.05, new DoubleValidator(0.0, 1.0, 3), m_shadow_gb);
    m_shadow_blue_le->setFixedWidth(50);
    m_shadow_blue_le->move(60, 110);
    m_shadow_blue_le->addLabel(new QLabel("Green:   ", m_shadow_gb));

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
        ApplytoPreview();
    };

    connect(m_shadow_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_shadow_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addHistogramClippingInputs_HighlightRed() {

    m_highlight_red_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_red_le->setFixedWidth(50);
    m_highlight_red_le->move(60, 30);
    m_highlight_red_le->addLabel(new QLabel("Red:   ", m_highlight_gb));

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

        ApplytoPreview();
    };

    connect(m_highlight_red_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_red_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addHistogramClippingInputs_HighlightGreen() {

    m_highlight_green_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_green_le->setFixedWidth(50);
    m_highlight_green_le->move(60, 70);
    m_highlight_green_le->addLabel(new QLabel("Green:   ", m_highlight_gb));

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
        ApplytoPreview();
    };

    connect(m_highlight_green_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_green_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::addHistogramClippingInputs_HighlightBlue() {

    m_highlight_blue_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 3), m_highlight_gb);
    m_highlight_blue_le->setFixedWidth(50);
    m_highlight_blue_le->move(60, 110);
    m_highlight_blue_le->addLabel(new QLabel("Green:   ", m_highlight_gb));

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
        ApplytoPreview();
    };

    connect(m_highlight_blue_slider, &QSlider::actionTriggered, this, action);
    connect(m_highlight_blue_le, &QLineEdit::editingFinished, this, edited);
}

void AutoHistogramDialog::joinHistogramClipping(bool v) {
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

void AutoHistogramDialog::updateJoinedHistogramClipping() {
    m_shadow_green_le->setValue(m_shadow_red_le->value());
    m_shadow_green_slider->setValue(m_shadow_red_slider->sliderPosition());

    m_shadow_blue_le->setValue(m_shadow_red_le->value());
    m_shadow_blue_slider->setValue(m_shadow_red_slider->sliderPosition());

    m_highlight_green_le->setValue(m_highlight_red_le->value());
    m_highlight_green_slider->setValue(m_highlight_red_slider->sliderPosition());

    m_highlight_blue_le->setValue(m_highlight_red_le->value());
    m_highlight_blue_slider->setValue(m_highlight_red_slider->sliderPosition());
}




void AutoHistogramDialog::showPreview() {
    ProcessDialog::showPreview();
    ApplytoPreview();
}

void AutoHistogramDialog::resetDialog() {
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

    ApplytoPreview();
}

void AutoHistogramDialog::Apply() {

    if (m_workspace->subWindowList().size() == 0)
        return;

    auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        iwptr->applyToSource(m_ah, &AutoHistogram::Apply);
        break;
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
        iw16->applyToSource(m_ah, &AutoHistogram::Apply);
        break;
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
        iw32->applyToSource(m_ah, &AutoHistogram::Apply);
        break;
    }
    }

    ApplytoPreview();

}

void AutoHistogramDialog::ApplytoPreview() {

    if (!isPreviewValid())
        return;

    auto iwptr = reinterpret_cast<PreviewWindow8*>(m_preview);

    switch (iwptr->type()) {
    case ImageType::UBYTE: {
        auto iw8 = iwptr;
        return iw8->updatePreview(m_ah, &AutoHistogram::Apply);
    }
    case ImageType::USHORT: {
        auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr);
        return iw16->updatePreview(m_ah, &AutoHistogram::Apply);
    }
    case ImageType::FLOAT: {
        auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr);
        return iw32->updatePreview(m_ah, &AutoHistogram::Apply);
    }
    }
}