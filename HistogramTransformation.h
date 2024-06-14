#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "RGBColorSpace.h"

class HistogramTransformation {

	struct MTFCurve {
	private:
		float m_shadow = 0;
		float m_midtone = 0.5;
		float m_highlights = 1.0;

		float m1 = m_midtone - 1;
		float m2 = 2 * m_midtone - 1;
		float dv = 1.0 / (m_highlights - m_shadow);

	public:
		std::vector<uint16_t> m_lut;

		MTFCurve(float shadow, float midtone, float highlight) : m_shadow(shadow), m_midtone(midtone), m_highlights(highlight) {}

		MTFCurve() = default;

		bool IsIdentity() { return (m_shadow == 0.0 && m_midtone == 0.5 && m_highlights == 1.0); }

		float Shadow() const { return m_shadow; }

		float Midtone() const { return m_midtone; }

		float Highlight() const { return m_highlights; }

		void setShadow(float shadow);

		void setMidtone(float midtone);

		void setHighlight(float hightlight);
	
		float MTF(float pixel);

		float TransformPixel(float pixel);

		void Generate16Bit_LUT();

		void Generate8Bit_LUT();

		template <typename T>
		void ApplyChannel(Image<T>& img, int ch);
	};

private:
	std::array<MTFCurve, 4> m_hist_curves;

	MTFCurve& operator[](ColorComponent comp) {
		return m_hist_curves[int(comp)];
	}

	const MTFCurve& operator[](ColorComponent comp) const {
		return m_hist_curves[int(comp)];
	}

public:
	HistogramTransformation() = default;

	float Shadow(ColorComponent component) const;

	float Midtone(ColorComponent component) const;

	float Highlight(ColorComponent component) const;

	void setShadow(ColorComponent component, float shadow);

	void setMidtone(ColorComponent component, float midtone);

	void setHighlight(ColorComponent component, float hightlight);

	float TransformPixel(ColorComponent component, float pixel);

	static float MTF(float pixel, float midtone);

	template<typename T>
	void ComputeSTFCurve(Image<T>& img);

	template<typename T>
	void STFStretch(Image<T>& img);

	template<typename T>
	void Apply(Image<T>& img);
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

	int max_val = 400;
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

private:
	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);
	
	void paintEvent(QPaintEvent* event);

	void AddStyleSheet();
};



class HistogramTransformationDialog : public ProcessDialog {
	Q_OBJECT

	HistogramTransformation m_ht;
	ColorComponent m_current_comp = ColorComponent::rgb_k;

	HistogramSlider* m_histogram_slider;

	QGraphicsPathItem* m_mtf_curve = nullptr;
	QPushButton* m_mtf_curve_pb;

	QLabel* m_shadow_label;
	DoubleLineEdit* m_shadow_le;

	QLabel* m_midtone_label;
	DoubleLineEdit* m_midtone_le;

	QLabel* m_highlight_label;
	DoubleLineEdit* m_highlight_le;

	QGraphicsView* m_gv;
	QGraphicsScene* m_gs;

	std::array<QPen,4> m_pens = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) ,QColor(255,255,255) };

	QComboBox* m_image_sel;

	QButtonGroup* m_component_bg; 
	int m_current_hist_res = 16;
	

public:
	HistogramTransformationDialog(QWidget* parent = nullptr);

private:
	void onWindowOpen();

	void onWindowClose();

	void onButtonPress(bool val) {
		m_mtf_curve->setVisible(val);
	}

private:
	void onClick(int id);
	
	void onActivation_imageSelection(int index);

	void onActivation_resolution(int index);

	void sliderMoved_shadow(int value);

	void sliderMoved_midtone(int value);

	void sliderMoved_highlight(int value);

	void editingFinished_smh();

	void resetScene();

	void addGrid();

	void AddHistogramChart();

	template<typename T>
	void showHistogram(Image<T>& img);

	void showMTFCurve();

	void AddHistogramSlider();

	void AddChannelSelection();

	void AddLineEdits();

	void AddImageSelectionCombo();

	void AddMTFPushButton();

	void AddResolutionCombo();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};