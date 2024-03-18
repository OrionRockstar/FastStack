#pragma once
#include "Image.h"
#include "Toolbar.h"
#include <QChart>
#include <qvalueaxis.h>
#include <QLineSeries>
#include <QChartView>

enum class Component {
	red = 0,
	green,
	blue,
	rgb_k
};

class HistogramTransformation {

	struct HistogramCurve {
	private:
		float m_shadow = 0;
		float m_midtone = 0.5;
		float m_highlights = 1.0;

		float m1 = m_midtone - 1;
		float m2 = 2 * m_midtone - 1;
		float dv = 1.0 / (m_highlights - m_shadow);

	public:
		std::vector<uint16_t> m_lut;

		HistogramCurve(float shadow, float midtone, float highlight) : m_shadow(shadow), m_midtone(midtone), m_highlights(highlight) {}

		HistogramCurve() = default;

		bool IsIdentity() { return (m_shadow == 0 && m_midtone == 0.5 && m_highlights == 1.0); }

		float Shadow() const { return m_shadow; }

		float Midtone() const { return m_midtone; }

		float Highlight() const { return m_highlights; }

		void setShadow(float shadow) {
			m_shadow = shadow;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		void setMidtone(float midtone) {
			m_midtone = midtone;
			m1 = m_midtone - 1;
			m2 = 2 * m_midtone - 1;
		}

		void setHighlight(float hightlight) {
			m_highlights = hightlight;
			dv = 1.0 / (m_highlights - m_shadow);
		}
	
		float MTF(float pixel) {

			if (pixel <= 0.0f) return 0;

			else if (pixel >= 1) return 1;

			else if (pixel == m_midtone)  return 0.5;

			return (m1 * pixel) / ((m2 * pixel) - m_midtone);

		}

		float TransformPixel(float pixel) {
			pixel = (pixel - m_shadow) * dv;

			return MTF(pixel);
		}


		void Generate16Bit_LUT() {
			m_lut.resize(65536);
			for (int el = 0; el < 65536; ++el)
				m_lut[el] = TransformPixel(el / 65535.0f) * 65535;
		}

		void Generate8Bit_LUT() {
			m_lut.resize(256);
			for (int el = 0; el < 256; ++el)
				m_lut[el] = TransformPixel(el / 255.0f) * 255;
		}

		template <typename Image>
		void ApplyChannel(Image& img, int ch);
	};

private:
	std::array<HistogramCurve, 4> m_hist_curves;

	HistogramCurve& operator[](Component comp) {
		return m_hist_curves[int(comp)];
	}

	const HistogramCurve& operator[](Component comp) const {
		return m_hist_curves[int(comp)];
	}

public:
	HistogramTransformation() = default;

	float Shadow(Component component) const;

	float Midtone(Component component) const;

	float Highlight(Component component) const;

	void setShadow(Component component, float shadow);

	void setMidtone(Component component, float midtone);

	void setHighlight(Component component, float hightlight);

	template<typename T>
	void ComputeSTFCurve(Image<T>& img);

	template<typename T>
	void STFStretch(Image<T>& img);

	template<typename Image>
	void Apply(Image& img);

};



class HistogramSlider : public QSlider {
	Q_OBJECT

private:
	int click_x = 0;

	QStyleOptionSlider m_shadow;
	bool m_shadow_act = false;

	QStyleOptionSlider m_midtone;
	bool m_midtone_act = false;

	QStyleOptionSlider m_highlight;
	bool m_highlight_act = false;

	int max_val = 380;
	double m_med = .5;

public:
	HistogramSlider(Qt::Orientation orientation, QWidget* parent);

signals:

	void sliderMoved_shadow(int value);

	void sliderMoved_midtone(int value);

	void sliderMoved_highlight(int value);

public:
	void setMedian(float median);

	int sliderPosition_shadow()const;

	void setSliderPosition_shadow(int pos);

	int sliderPosition_midtone()const;

	void setSliderPosition_midtone(int pos);

	int sliderPosition_highlight()const;

	void setSliderPosition_highlight(int pos);

	void resetSliderPositions();

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);
	
	void paintEvent(QPaintEvent* event);

};



class HistogramTransformationDialog : public ProcessDialog {
	Q_OBJECT

	HistogramTransformation m_ht;

	HistogramSlider* m_histogram_slider;

	QLabel* m_shadow_label;
	DoubleLineEdit* m_shadow_le;

	QLabel* m_midtone_label;
	DoubleLineEdit* m_midtone_le;

	QLabel* m_highlight_label;
	DoubleLineEdit* m_highlight_le;

	QChartView* cv;
	QChart* m_histogram;
	QValueAxis* m_axisX;
	QValueAxis* m_axisY;

	QComboBox* m_image_sel;

	QButtonGroup* m_component_bg;

public:
	HistogramTransformationDialog(QWidget* parent = nullptr);

	void onWindowOpen();

	void onWindowClose();

private:
	void onClick(int id);
	
	void onActivation(int index);

	void sliderMoved_shadow(int value);

	void sliderMoved_midtone(int value);

	void sliderMoved_highlight(int value);

	void editingFinished_shadow();

	void editingFinished_midtone();

	void editingFinished_highlight();

	void AddHistogramChart();

	template<typename T>
	void showHistogram(Image<T>& img);

	void AddChannelSelection();

	void AddLineEdits();

	void setHistogramTransformation();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};