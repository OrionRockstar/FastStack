#pragma once
#include "Image.h"
#include "ProcessDialog.h"


class RangeMask {

	float m_low = 0.0;
	float m_high = 1.0;

	float m_fuzziness = 0.0;
	float m_smoothness = 0.0;

	bool m_invert = false;
	bool m_screening = false;
	bool m_lightness = true;

public:
	float Low()const { return m_low; }

	void setLow(float low) { m_low = low; }

	float High()const { return m_high; }

	void setHigh(float high) { m_high = high; }

	float Fuzziness()const { return m_fuzziness; }

	void setFuzziness(float fuzziness) { m_fuzziness = fuzziness; }

	float Smoothness()const { return m_smoothness; }

	void setSmoothness(float smoothness) {
		if (0 <= smoothness && smoothness <= 100)
			m_smoothness = smoothness;
	}

	bool Lightness()const { return m_lightness; }

	void setLightness(bool lightness) { m_lightness = lightness; }

	bool Screening()const { return m_screening; }

	void setScreening(bool screening) { m_screening = screening; }

	bool Invert()const { return m_invert; }

	void setInvert(bool invert) { m_invert = invert; }

	template <typename T>
	Image<T> generateMask(const Image<T>& img);

	template <typename T>
	void generateMask_overwrite(Image<T>& img);

	template<typename T>
	Image<T> generateMask_reduced(const Image<T>& img, int factor);

	template<typename T>
	void generateMask_reducedTo(const Image<T>& src, Image<T>& dst, int factor);
};


class RangeSlider : public QSlider {
	Q_OBJECT

private:
	const int m_handle_width = style()->pixelMetric(QStyle::PM_SliderLength) - 1;

	QStyleOptionSlider m_low;
	bool m_low_act = false;

	QStyleOptionSlider m_high;
	bool m_high_act = false;

public:
	RangeSlider(int min, int max, QWidget* parent);

signals:

	void sliderMoved_low(int pos);

	void sliderMoved_high(int pos);

public:
	int value_low()const { return m_low.sliderValue; }

	void setValue_low(int value) {
		m_low.sliderValue = m_low.sliderPosition = math::clip(value, minimum(), maximum());

		if (value >= m_high.sliderValue && value < maximum())
			emit sliderMoved_high(m_high.sliderValue = m_high.sliderPosition = value + 1);

		update();
	}

	int value_high()const { return m_high.sliderValue; }

	void setValue_high(int value) {
		m_high.sliderValue = m_high.sliderPosition = math::clip(value, minimum(), maximum());

		if (value <= m_low.sliderPosition && value > minimum())
			emit sliderMoved_low(m_low.sliderValue = m_low.sliderPosition = m_high.sliderPosition - 1);

		update();
	}

	void resetSlider();


private:
	void wheelEvent(QWheelEvent* e) {}

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};






class RangeMaskDialog : public ProcessDialog {

	RangeMask m_rm;

	const int m_le_width = 85;

	RangeSlider* m_range_slider;
	DoubleLineEdit* m_low_le;
	DoubleLineEdit* m_high_le;

	DoubleLineEdit* m_fuzzy_le;
	Slider* m_fuzzy_slider;

	DoubleLineEdit* m_smooth_le;
	Slider* m_smooth_slider;

	CheckBox* m_lightness_cb;
	CheckBox* m_screening_cb;
	CheckBox* m_invert_cb;

public:
	RangeMaskDialog(QWidget* parent = nullptr);

private:
	void addRangeSlider();

	void addFuzzinessInputs();

	void addSmoothnessInputs();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};
