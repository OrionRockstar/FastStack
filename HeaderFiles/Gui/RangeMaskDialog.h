#pragma once
#include "RangeMask.h"
#include "ProcessDialog.h"
#include "CustomWidgets.h"



class RangeSlider : public Slider {
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
	bool event(QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);

	void wheelEvent(QWheelEvent* e) {}
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

	void apply();

	void applyPreview();
};
