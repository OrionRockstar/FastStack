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
	Image<T> GenerateMask(Image<T>& img);

};





class RangeSlider : public QSlider {
	Q_OBJECT

private:
	int click_x = 0;

	QStyleOptionSlider m_low;
	bool m_low_act = false;

	QStyleOptionSlider m_high;
	bool m_high_act = false;

public:
	RangeSlider(Qt::Orientation orientation, QWidget* parent);

signals:

	void sliderMoved_low(int value);

	void sliderMoved_high(int value);

public:
	int sliderPosition_low()const;

	void setSliderPosition_low(int pos);

	int sliderPosition_high()const;

	void setSliderPosition_high(int pos);

	void resetSliderPositions();

private:
	void wheelEvent(QWheelEvent* e) {}

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void paintEvent(QPaintEvent* event);

};


class RangeMaskDialog : public ProcessDialog {

	RangeMask m_rm;

	const int m_le_width = 85;

	RangeSlider* m_range_slider;
	DoubleLineEdit* m_low_le;
	DoubleLineEdit* m_high_le;

	DoubleLineEdit* m_fuzzy_le;
	QSlider* m_fuzzy_slider;

	DoubleLineEdit* m_smooth_le;
	QSlider* m_smooth_slider;

	QCheckBox* m_lightness_cb;
	QCheckBox* m_screening_cb;
	QCheckBox* m_invert_cb;

public:
	RangeMaskDialog(QWidget* parent = nullptr);

private:
	void onEditingFinished_low();

	void onSliderMoved_low(int pos);

	void onEditingFinished_high();

	void onSliderMoved_high(int pos);

	void onEditingFinished_fuzzy();

	void onActionTriggered_fuzzy(int action);

	void onEditingFinished_smooth();

	void onActionTriggered_smooth(int action);

	void AddRangeSlider();

	void AddFuzzinessInputs();

	void AddSmoothnessInputs();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};
