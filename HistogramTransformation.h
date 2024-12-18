#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "RGBColorSpace.h"
#include "Statistics.h"

class HistogramTransformation {

	class MTFCurve {

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

		float shadow() const { return m_shadow; }

		void setShadow(float shadow) {
			m_shadow = shadow;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float midtone() const { return m_midtone; }

		void setMidtone(float midtone) {
			m_midtone = midtone;
			m1 = m_midtone - 1;
			m2 = 2 * m_midtone - 1;
		}

		float highlight() const { return m_highlights; }

		void setHighlight(float hightlight) {
			m_highlights = hightlight;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float MTF(float pixel)const;

		float transformPixel(float pixel)const;

		void Generate16Bit_LUT();

		void Generate8Bit_LUT();

		template <typename T>
		void ApplyChannel(Image<T>& img, int ch);
	};

private:
	std::array<MTFCurve, 4> m_hist_curves;

public:
	MTFCurve& operator[](ColorComponent comp) {
		return m_hist_curves[int(comp)];
	}

	const MTFCurve& operator[](ColorComponent comp) const {
		return m_hist_curves[int(comp)];
	}

	HistogramTransformation() = default;

	float shadow(ColorComponent component)const {
		return (*this)[component].shadow();
	}

	void setShadow(ColorComponent component, float shadow) {
		(*this)[component].setShadow(shadow);
	}

	float midtone(ColorComponent component)const {
		return (*this)[component].midtone();
	}

	void setMidtone(ColorComponent component, float midtone) {
		(*this)[component].setMidtone(midtone);
	}

	float highlight(ColorComponent component)const {
		return (*this)[component].highlight();
	}

	void setHighlight(ColorComponent component, float highlight) {
		(*this)[component].setHighlight(highlight);
	}

	float transformPixel(ColorComponent component, float pixel)const;

	static float MTF(float pixel, float midtone);

	template<typename T>
	void computeSTFCurve(const Image<T>& img);

	template<typename T>
	void Apply(Image<T>& img);
};






class HistogramSlider : public QSlider {
	Q_OBJECT

private:
	int click_x = 0;

	QStyleOptionSlider m_trackbar;

	QStyleOptionSlider m_shadow;
	bool m_shadow_act = false;

	QStyleOptionSlider m_midtone;
	bool m_midtone_act = false;

	QStyleOptionSlider m_highlight;
	bool m_highlight_act = false;

	int max_val = 400;
	double m_med = .5;

	QColor m_brush = Qt::white;

public:
	HistogramSlider(Qt::Orientation orientation, QWidget* parent);

signals:

	void sliderMoved_shadow(int pos);

	void sliderMoved_midtone(int pos);

	void sliderMoved_highlight(int pos);

public:

	void setMedian(float median);

	int sliderPosition_shadow()const;

	void setSliderPosition_Shadow(int pos);

	int sliderPosition_midtone()const;

	void setSliderPosition_Midtone(int pos);

	int sliderPosition_highlight()const;

	void setSliderPosition_Highlight(int pos);

	void resetSliderPositions();

	void setHandleColor(QColor brush) { m_brush = brush; }
private:
	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);
	
	void paintEvent(QPaintEvent* event);

	void drawHandle(const QRect& rect, const QStyleOptionSlider& opt, QPainter& p);

	void AddStyleSheet();
};





class HistogramTransformationView : public HistogramView {

	HistogramTransformation* m_ht = nullptr;
	QGraphicsPathItem* m_mtf_curve = nullptr;

public:
	HistogramTransformationView(HistogramTransformation& ht, const QSize& size, QWidget* parent = nullptr);

	QGraphicsPathItem* mtfCurve()const { return m_mtf_curve; }

	void drawScene();

	void clearScene(bool draw_grid = true)override;

	void resetScene();

	void drawMTFCurve();
private:
	void wheelEvent(QWheelEvent* e) {}
};






class HistogramTransformationDialog : public ProcessDialog {
	Q_OBJECT

	HistogramTransformation m_ht;
	HistogramTransformationView* m_htv = nullptr;

	ColorComponent m_current_comp = ColorComponent::rgb_k;
	const std::array<QColor, 4> m_colors = { Qt::red,Qt::green,Qt::blue,Qt::white };

	HistogramSlider* m_histogram_slider = nullptr;

	CheckablePushButton* m_mtf_curve_pb = nullptr;

	DoubleLineEdit* m_shadow_le = nullptr;
	DoubleLineEdit* m_midtone_le = nullptr;
	DoubleLineEdit* m_highlight_le = nullptr;


	ComboBox* m_image_sel = nullptr;
	QButtonGroup* m_component_bg = nullptr; 
	ComboBox* m_hist_res_combo = nullptr;

public:
	HistogramTransformationDialog(QWidget* parent = nullptr);

private:
	void onWindowOpen();

	void onWindowClose();

private:
	void onClick(int id);
	
	void onActivation_imageSelection(int index);

	void onActivation_resolution(int index);

	void sliderMoved_shadow(int pos);

	void sliderMoved_midtone(int pos);

	void sliderMoved_highlight(int pos);

	void editingFinished_smh();

	void addHistogramSlider();

	void addChannelSelection();

	void addLineEdits();

	void addImageSelectionCombo();

	void addMTFPushButton();

	void addResolutionCombo();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};