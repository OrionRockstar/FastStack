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

double AutoHistogram::computeLogMultiplier(float median, float tgt_median) {

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
void AutoHistogram::apply(Image<T>& img) {

    Histogram histogram;
    for (int ch = 0; ch < img.channels(); ++ch) {

        histogram.constructHistogram(img, ch);
        float median = (img.type() == ImageType::FLOAT) ? histogram.medianf() : histogram.median();

        if (m_histogram_clipping) {

            int i = 0, count = 0;
            auto total = histogram.count();
            while (count < m_shadow_clipping[ch] * 0.01 * total)
                count += histogram[i++];

            float shadow = float(i) / (histogram.resolution() - 1);

            i = histogram.resolution() - 1;
            count = 0;

            while (count < m_highlight_clipping[ch] * 0.01 * total)
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

                float b = computeLogMultiplier(median, m_target_median[ch]);

                if (b != -1.0f) {

                    float c = 1 / LTF(1, b);

                    if (isUByteImage(img) || isUShortImage(img)) {

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

                if (isUByteImage(img) || isUShortImage(img)) {

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
                ht.apply(img);

                break;
            }
            }
        }
    }

}
template void AutoHistogram::apply(Image8&);
template void AutoHistogram::apply(Image16&);
template void AutoHistogram::apply(Image32&);
