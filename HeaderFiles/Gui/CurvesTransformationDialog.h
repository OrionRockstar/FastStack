#pragma once

#include "CurvesTransformation.h"
#include "ImageWindow.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"



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





class CurvesTransformationDialog : public ProcessDialog {
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
	CurvesTransformationDialog(QWidget* parent = nullptr);

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

	void apply();

	void applytoPreview();
};