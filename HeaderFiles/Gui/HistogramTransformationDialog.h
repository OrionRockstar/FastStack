#pragma once
#include "ProcessDialog.h"
#include "HistogramTransformation.h"
#include "Statistics.h"



class HistogramSlider : public QSlider {
	Q_OBJECT

private:
	const int m_handle_width = 6;// style()->pixelMetric(QStyle::PM_SliderLength) - 12;
	const int m_handle_height = 14;

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

	void apply();

	void applytoPreview();
};

