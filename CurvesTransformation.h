#pragma once
#include "pch.h"
#include "Image.h"
#include "CurveInterpolation.h"
#include "ProcessDialog.h"
#include "Maths.h"
#include "RGBColorSpace.h"
#include <QGraphicsScene>
#include <QGraphicsView>



class CurveTransform {
private:
	std::array<Curve, 10> m_comp_curves;

	Curve& rCurve(ColorComponent comp) { return m_comp_curves[int(comp)]; }

public:

	CurveTransform() = default;

	~CurveTransform() {};

	void setInterpolation(ColorComponent comp, Curve::Type type);

	void setDataPoints(ColorComponent comp, std::vector<QPointF> points);

	void computeCoefficients(ColorComponent comp);

	void interpolateValues(ColorComponent comp, std::vector<double>& values);

	template<typename T>
	void Apply(Image<T>& img);
};






class CurveTransformScene : public CurveScene {
	CurveTransform* m_ctp;

public:
	CurveTransformScene(CurveTransform* ctp, QRect rect, QWidget* parent);

	CurveTransformScene() = default;

	void onCurveTypeChange(int id);

	void onUpdatePoint(QPointF point)override;

private:
	void renderCurve(CurveItem* curve);

	void mousePressEvent(QGraphicsSceneMouseEvent* event)override;

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event)override;

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event)override;

};



class CurveGraphicsView : public QGraphicsView {

	CurveTransformScene* m_cts;

	struct Axis {
		QImage x_axis = QImage(0, 0, QImage::Format::Format_ARGB32);
		QImage y_axis = QImage(0, 0, QImage::Format::Format_ARGB32);
	};

	std::array<Axis, 10> m_color_axis;

public:
	CurveGraphicsView(QGraphicsScene* scene, QWidget* parent);

	void loadAxis();

	void drawBackground(QPainter* painter, const QRectF& rect)override;
};





class CurveTransformDialog : public ProcessDialog {
	Q_OBJECT

private:
	CurveTransform m_ct;

	CurveTransformScene* m_cs;
	CurveGraphicsView* m_gv;

	QButtonGroup* m_component_bg;
	CurveTypeButtonGroup* m_curve_type_bg;


	DoubleLineEdit* m_input_le;
	DoubleLineEdit* m_output_le;

	QLabel* m_current_point;

public:
	CurveTransformDialog(QWidget* parent = nullptr);

public slots:
	void onItemARC();

	void onEditingFinished_io();

	void onCurrentPos(QPointF point);

signals:
	void updatePoint(QPointF point);

private:

	void addComponentSelection();

	void addPointLineEdits();

	void addPointSelection();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};


