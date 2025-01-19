#pragma once
#include "CurveInterpolation.h"
#include "Image.h"
#include "ProcessDialog.h"

class ColorSaturation {
	Curve m_saturation_curve = Curve({ 0.0,0.0 }, { 1.0,0.0 });
    double m_hue_shift = 0;
	int m_scale = 1;

public:
	void setHueShift(double amount = 0) {
		m_hue_shift = amount;
	}

	Curve& saturationCurve() { return m_saturation_curve; }

    void setInterpolation(Curve::Type type) {
		m_saturation_curve.setInterpolation(type);
        m_saturation_curve.computeCoeffecients();
    }

	void setScale(int scale) { m_scale = scale; }

public:
	template<typename T>
	void apply(Image<T>& img);
};



class ColorSaturationScene : public CurveScene {
	ColorSaturation* m_cs;

public:
	ColorSaturationScene(ColorSaturation* cs, QRect rect, QWidget* parent);

	void normalizePoint(QPointF& p);

	void resetScene() override;

	void onCurveTypeChange(int id);

private:
	void renderCurve(CurveItem * curve);

	void mousePressEvent(QGraphicsSceneMouseEvent * event)override;

	void mouseMoveEvent(QGraphicsSceneMouseEvent * event)override;

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override;

};




class RGBBar : public QLabel {

	Matrix m_coef;
	QImage m_src;
	QImage m_dst;
	QPixmap m_pix;
	uint8_t m_alpha = 0.65 * 255;

	Matrix computeCoeficients(uint32_t width_size);

public:
	RGBBar(const QSize& size, QWidget* parent);

	void fillSource();

	void updateBar_shift(int shift = 0);
};


class ColorGraphicsView : public QGraphicsView {

	ColorSaturationScene* m_css;

	GradientImage m_sat;
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