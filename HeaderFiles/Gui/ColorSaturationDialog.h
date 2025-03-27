#pragma once
#include "ColorSaturation.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"


class ColorSaturationScene : public CurveScene {
	ColorSaturation* m_cs;

public:
	ColorSaturationScene(ColorSaturation* cs, QRect rect, QWidget* parent);

	void normalizePoint(QPointF& p);

	void resetScene() override;

	void onCurveTypeChange(int id);

private:
	void renderCurve(CurveItem* curve);

	void mousePressEvent(QGraphicsSceneMouseEvent* event)override;

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event)override;

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override;

};




class RGBBar : public QLabel {

	Matrix m_coef;
	QImage m_src;
	QImage m_dst;
	uint8_t m_alpha = 0.65 * 255;

public:
	RGBBar(const QSize& size, QWidget* parent);

	void fillSource();

	void updateBar_shift(int shift = 0);

private:
	Matrix computeCoeficients(uint32_t width_size);

	void paintEvent(QPaintEvent* e);
};


class ColorGraphicsView : public QGraphicsView {

	ColorSaturationScene* m_css;

	QImage m_sat;
	RGBBar* m_rgb;

public:
	ColorGraphicsView(QGraphicsScene* scene, QWidget* parent);

	void shiftRGB(int x) {
		m_rgb->updateBar_shift(x);
	}

	void drawBackground(QPainter* painter, const QRectF& rect);
};



//add point selection arrows


class ColorSaturationDialog : public ProcessDialog {
	Q_OBJECT

private:
	ColorSaturation m_cs;

	ColorSaturationScene* m_css;
	ColorGraphicsView* m_gv;

	DoubleLineEdit* m_hue_le; //input
	DoubleLineEdit* m_saturation_le; //output

	QLabel* m_current_point;

	RGBBar* m_rgb_bar;

	DoubleLineEdit* m_hue_shift_le;
	Slider* m_hue_shift_slider;

	SpinBox* m_scale_sb;
	int m_old_value = 1;

	CurveTypeButtonGroup* m_curve_type_bg;
public:
	ColorSaturationDialog(QWidget* parent);

	void onItemARC();

	void onCurrentPos(QPointF point);

	void onValueChanged_scale(int value);

private:
	void addHueSaturation();

	void addHueShift();

	void addPointSelection();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};