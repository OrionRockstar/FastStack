#pragma once
#include "pch.h"
#include "Image.h"
#include "CurveInterpolation.h"
#include"Toolbar.h"
#include "Maths.h"
#include <QGraphicsScene>
#include <QGraphicsView>

enum class CurveComponent {
	red = 0,
	green,
	blue,
	rgb_k,
	Lightness,
	a,
	b,
	c,
	hue,
	saturation
};

class CurveTransform {
private:
	std::array<Curve, 10> m_comp_curves;

	Curve& rCurve(CurveComponent comp) { return m_comp_curves[int(comp)]; }

public:

	CurveTransform() = default;

	~CurveTransform() {};

	void SetInterpolationMethod(CurveComponent comp, CurveType type);

	void SetDataPoints(CurveComponent comp, std::vector<QPointF> points);

	void ComputeCoefficients(CurveComponent comp);

	void InterpolateValues(CurveComponent comp, std::vector<double>& values);

	template<typename T>
	void Apply(Image<T>& img);
};

class CurveItem {

	QGraphicsScene* m_scene = nullptr;

	CurveType m_type = CurveType::akima_spline;

	QPen m_pen;
	QBrush m_brush;

	QGraphicsItem* m_current = nullptr;

	QGraphicsItem* m_left = nullptr;
	QGraphicsItem* m_right = nullptr;

	QGraphicsPathItem* m_curve = nullptr;

	QPolygonF m_input_pts;
	QPolygonF m_curve_pts;

	QList<QGraphicsItem*> m_item_list;

public:

	CurveItem() = default;

	CurveItem(QPen pen, QBrush brush, QGraphicsScene* scene);

	void SetCurveType(CurveType type) { m_type = type; }

	CurveType CurveType() { return m_type; }

	QGraphicsItem* Left()const;

	QGraphicsItem* Right()const;

	QGraphicsItem* Current()const;

	QPointF ItemCenter(QGraphicsItem* item);

	QGraphicsItem* AddEllipse(qreal x, qreal y);

	bool CollidesWithOtherItem(QGraphicsItem* current);

	std::vector<QPointF> GetNormalizedInputs();

	QGraphicsItem* RemoveItem(QPointF point);

	void SetCurvePoints(std::vector<double>& values);

	void HideCurve();

	void HidePoints();

	void ShowPoints();

	void ShowCurve();

	void UpdateItemPos(QGraphicsItem* item, QPointF delta);

};


class CurveScene : public QGraphicsScene {
	Q_OBJECT
private:
	CurveTransform* m_ctp;

	int click_x = 0;
	int click_y = 0;

	std::array<QColor, 10> m_color = { QColor{255,000,000}, {000,255,000}, {000,000,255}, {255,255,255}, {255,255,255},
											 {255,255,000}, {000,255,255}, {255,000,127}, {000,128,255}, {178,102,255} };

	CurveComponent m_comp = CurveComponent::rgb_k;
	std::array<CurveItem*, 10> m_curve_items;

	std::vector<double> m_values = std::vector<double>(380);


	QGraphicsItem* m_current = nullptr;

public:

	CurveScene(CurveTransform* ct, QRect rect, QWidget* parent = nullptr);

	void AddGrid();

	void resetScene();

public slots:
	void ChannelChanged(int id);

	void CurveTypeChanged(int id);

	void receiveLinePos(QPointF point);

signals:
	void sendCurveType(int id);

	void sendCurrentPos(QPointF point);

	void sendEndCurrent(bool val);

private:

	QPointF GetCurrentItemCenter()const;

	bool isInScene(const QRectF rect)const;

	bool isInScene_ends(const QRectF rect)const;

	void RenderCurve(CurveItem* curve);

	void mousePressEvent(QGraphicsSceneMouseEvent* event);

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

	//check for vertical points
	// points with same x value
	//void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
};

class CurveTransformDialog : public ProcessDialog {
	Q_OBJECT

private:
	CurveTransform m_ct;

	CurveScene* cs;
	QGraphicsView* gv;

	QButtonGroup* m_component_bg;
	QButtonGroup* m_curvetype_bg;

	DoubleLineEdit* m_input_le;
	DoubleLineEdit* m_output_le;

public:
	CurveTransformDialog(QWidget* parent = nullptr);

public slots:

	void onClick(int id);

	void onChannelChange(int id);

	void onEditingFinished_io();

	void endCurrent(bool val);

	void currentPos(QPointF point);

signals:
	void sendLinePos(QPointF point);

private:

	void AddComponentSelection();

	void AddCuveTypeSelection();

	void AddPointLineEdits();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};


