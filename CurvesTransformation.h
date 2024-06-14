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

	void SetInterpolationMethod(ColorComponent comp, CurveType type);

	void SetDataPoints(ColorComponent comp, std::vector<QPointF> points);

	void ComputeCoefficients(ColorComponent comp);

	void InterpolateValues(ColorComponent comp, std::vector<double>& values);

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

public:
	const QList<QGraphicsItem*>& ItemList()const { return m_item_list; }

	int ItemIndex(const QGraphicsItem* item)const { return m_item_list.indexOf(item); }

	int ItemListSize()const { return m_item_list.size(); }

	QPointF InputPoint(int index) { return m_input_pts[index]; }

	CurveType CurveType() { return m_type; }

	QGraphicsItem* Left()const { return m_left; }

	QGraphicsItem* Right()const { return m_right; }

	QGraphicsItem* Current()const { return m_current; }

	static QPointF ItemCenter(const QGraphicsItem* item);

	QGraphicsItem* AddEllipse(qreal x, qreal y);

	bool CollidesWithOtherItem(QGraphicsItem* current);

	std::vector<QPointF> GetNormalizedInputs();

	void RemoveItem(QPointF point);

	void SetCurvePoints(const std::vector<double>& y_values);

	void setCurveVisibility(bool visible);

	void UpdateItemPos(QGraphicsItem* item, QPointF delta);

	const QGraphicsItem* nextItem();

	const QGraphicsItem* previousItem();

};


class CurveScene : public QGraphicsScene {
	Q_OBJECT
private:
	CurveTransform* m_ctp;

	int click_x = 0;
	int click_y = 0;

	std::array<QColor, 10> m_color = { QColor{255,000,000}, {000,255,000}, {000,000,255}, {255,255,255}, {255,255,255},
											 {255,255,000}, {000,255,255}, {255,000,127}, {000,128,255}, {178,102,255} };

	ColorComponent m_comp = ColorComponent::rgb_k;
	std::array<CurveItem*, 10> m_curve_items;

	std::vector<double> m_values = std::vector<double>(380);


	QGraphicsItem* m_current = nullptr;

public:

	CurveScene(CurveTransform* ct, QRect rect, QWidget* parent = nullptr);

	CurveItem* curveItem(ColorComponent comp)const {
		return m_curve_items[int(comp)];
	}

	void drawGrid();

	void resetScene();

public slots:
	void ChannelChanged(int id);

	void CurveTypeChanged(int id);

	void onUpdatePoint(QPointF point);

	void onPressed_previous_next();

signals:
	//item added, removed, changed
	void itemARC();

	void sendCurveType(int id);

	void sendCurrentPos(QPointF point);

	void sendIsEndCurrent(bool val);

	void end(bool);

private:
	bool xValueExists()const;

	bool isInScene(const QRectF rect)const;

	bool isInScene_ends(const QRectF rect)const;

	void RenderCurve(CurveItem* curve);

	void mousePressEvent(QGraphicsSceneMouseEvent* event);

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
};

class CurveTransformDialog : public ProcessDialog {
	Q_OBJECT

private:
	CurveTransform m_ct;

	CurveScene* m_cs;
	QGraphicsView* m_gv;

	QButtonGroup* m_component_bg;
	QButtonGroup* m_curvetype_bg;

	DoubleLineEdit* m_input_le;
	DoubleLineEdit* m_output_le;

	QLabel* m_current_point;

public:
	CurveTransformDialog(QWidget* parent = nullptr);

public slots:
	void onItemARC();

	void onEditingFinished_io();

	void onCurrentPos(QPointF point);

	void onPressed_previous();

	void onPressed_next();

signals:
	void updatePoint(QPointF point);

private:

	void AddComponentSelection();

	void AddCuveTypeSelection();

	void AddPointLineEdits();

	void AddPointSelection();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};


