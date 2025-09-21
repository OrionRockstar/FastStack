#pragma once
#include "ProcessDialog.h"
#include "HistogramTransformation.h"
#include "HistogramView.h"



class HistogramSlider : public QSlider {
	Q_OBJECT

public:
	enum class Tone: uint8_t {
		shadow,
		midtone,
		highlight
	};

private:
	struct SliderHandle {
		QStyleOptionSlider option;
		bool active = false;
	};

	struct Handles {
		std::array<SliderHandle, 3> data;

		SliderHandle& operator[](Tone tone) {
			return data[size_t(tone)];
		}
	};

	const int m_handle_width = 6;// style()->pixelMetric(QStyle::PM_SliderLength) - 12;
	const int m_handle_height = 14;

	const int m_view_width = 400;
	Handles m_handles;

	double m_med = .5;

	QColor m_brush = Qt::white;

	int computeMidtone();

public:
	HistogramSlider(QWidget* parent);

signals:
	void sliderMoved_shadow(int pos);

	void sliderMoved_midtone(int pos);

	void sliderMoved_highlight(int pos);

public:
	int sliderPosition(Tone tone) {

		return m_handles[tone].option.sliderValue;
	}

	void setSliderPosition(Tone tone, int value) {

		switch (tone) {
		case Tone::shadow:
			return setSliderPosition_shadow(value);
		case Tone::midtone:
			return setSliderPosition_midtone(value);
		case Tone::highlight:
			return setSliderPosition_highlight(value);
		}
	}

	void resetSlider();

	void setHandleColor(QColor brush) { m_brush = brush; }

private:
	void setSliderPosition_shadow(int value);

	void setSliderPosition_midtone(int value);

	void setSliderPosition_highlight(int value);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* event);

	void drawHandle(QStyleOptionSlider& opt, QPainter& p);
};





class HistogramTransformationView : public HistogramView {

	HistogramTransformation* m_ht = nullptr;

	bool m_mtf_visible = true;
	QGraphicsPathItem* m_mtf_curve = nullptr;
	QGraphicsLineItem* m_midtone_dash = nullptr;
	QGraphicsLineItem* m_highlight_dash = nullptr;

public:
	HistogramTransformationView(HistogramTransformation& ht, const QSize& size, QWidget* parent = nullptr);

private:
	void clearScene(bool draw_grid = true)override;

public:
	void clearHistogram();

	void drawHistogram();

	void setMTFVisible(bool visible);

	void drawMTFCurve();

private:
	void wheelEvent(QWheelEvent* e) {}
};





class HistogramTransformationDialog : public ProcessDialog {

	Q_OBJECT

private:
	HistogramTransformation m_ht;
	HistogramTransformationView* m_htv = nullptr;
	HistogramVector m_current_histogram;
	HistogramView* m_hv = nullptr;

	ColorComponent m_current_comp = ColorComponent::rgb_k;
	const std::array<QColor, 4> m_colors = { Qt::red,Qt::green,Qt::blue,Qt::white };

	HistogramSlider* m_histogram_slider = nullptr;
	using Tone = HistogramSlider::Tone;

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
	void onImageWindowCreated()override;

	void onImageWindowClosed()override;

private:
	int toPos(float val) {
		return val * m_histogram_slider->maximum();
	}

	float toValue(int pos) {
		return pos / float(m_histogram_slider->maximum());
	}

	void onClick(int id);

	void onImageSelection();

	void onResolutionSelection();

	void onChange();

	void addHistogramSlider();

	void addChannelSelection();

	void addLineEdits();

	void addImageSelectionCombo();

	void addMTFPushButton();

	void addResolutionCombo();

	void resetDialog();

	void apply() override;

	void applyPreview() override;

	//works
	//void atpTest()override { std::cout << "HistogramTransformation\n"; }
};

