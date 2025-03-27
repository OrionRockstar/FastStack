#pragma once
#include "pch.h"
#include "RGBColorSpace.h"
#include "Maths.h"


class Curve {
public:
	enum class Type : uint8_t {
		akima_spline,
		cubic_spline,
		linear
	};

private:
	template<typename T, int Offset>
	struct MyVector {
		std::unique_ptr<T[]> data;
		int m_offset = Offset;
		int m_size = 0;

		MyVector(int size) : m_size(size) {
			data = std::make_unique<T[]>(size);
		}
		MyVector() = default;
		~MyVector() {};

		T& operator[](int index) {
			return data[index + m_offset];
		}

		int size()const { return m_size; }
	};

	typedef std::vector < std::array<double, 4>> SplineCoefs;

	std::vector<QPointF> m_points = { QPointF(0,0), QPointF(1, 1) };
	SplineCoefs m_splc;
	Type m_type = Type::akima_spline;

	static bool comaparePoints(const QPointF& a, const QPointF& b) { return (a.x() < b.x()); }

public:
	Curve() = default;

	Curve(const QPointF& a, const QPointF& b);

	void setInterpolation(Type type) { m_type = type; }

	void setDataPoints(const std::vector<QPointF>& pts);

	void computeCoeffecients();

	bool isIdentity()const;

private:
	void cubicSplineCurve();

	void akimaSplineCurve();

	void linearCurve();

public:

	double interpolate(double pixel)const;

private:
	double akimaInterpolator(double pixel)const;

	double cubicInterpolator(double pixel)const;

	double linearInterpolator(double pixel)const;
};





class CurveTypeButtonGroup : public QWidget {
	Q_OBJECT

		QButtonGroup* m_bg;
public:
	CurveTypeButtonGroup(QWidget* parent);

	QAbstractButton* button(int id) {
		return m_bg->button(id);
	}
signals:
	void idClicked(int id);
};






class CurveItem : public QObject{
	Q_OBJECT

protected:
	QGraphicsScene* m_scene = nullptr;

	Curve::Type m_type = Curve::Type::akima_spline;

	QPen m_pen;
	QBrush m_brush;

	QGraphicsItem* m_current = nullptr;

	QPointF m_left_default = {0.0,0.0};
	QGraphicsItem* m_left = nullptr;
	QPointF m_right_default = { 1.0,0.0 };
	QGraphicsItem* m_right = nullptr;

	bool m_linked_ends = false;

	QGraphicsPathItem* m_curve = nullptr;
	QPolygonF m_curve_pts;

	QList<QGraphicsItem*> m_item_list;

public:
	CurveItem(QPen pen, QBrush brush, QGraphicsScene* scene, float left_y = 0.0, float right_y = 1.0);

	void setCurveType(Curve::Type type) { m_type = type; }

signals:
	void itemChanged(QGraphicsItem* item);

public:
	QList<QGraphicsItem*>& itemList() { return m_item_list; }

	int itemIndex(const QGraphicsItem* item)const { return m_item_list.indexOf(item); }

	int itemListSize()const { return m_item_list.size(); }

	static QPointF itemCenter(const QGraphicsItem* item);

	QPointF itemCenter_norm(const QGraphicsItem* item);

	//Curve::Type CurveType()const { return m_type; }

	QGraphicsItem* endItem_Left()const { return m_left; }

	QGraphicsItem* endItem_Right()const { return m_right; }

	QGraphicsItem* currentItem()const { return m_current; }

	bool isEnd(const QGraphicsItem* item)const { 
		if (item == endItem_Left() || item == endItem_Right())
			return true; 
		return false;
	}

	void setLinkEnds(bool enable) { m_linked_ends = enable; }

	bool isEndsLinked()const { return m_linked_ends; }

	QGraphicsItem* addItem(qreal x, qreal y);

	bool curveCollision(const QGraphicsItem* item)const;

	std::vector<QPointF> curvePoints_norm();

	void removeItem(QPointF point);

	void setCurvePoints(const std::vector<double>& y_values);

	void setCurveVisibility(bool visible);

	void updateItemPos(QGraphicsItem* item, QPointF delta);

	const QGraphicsItem* nextItem();

	const QGraphicsItem* previousItem();

};






struct XRGB {
	int x = 0;
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;

	XRGB() = default;

	XRGB(int x, uint8_t r, uint8_t g, uint8_t b) : x(x), red(r), green(g), blue(b) {}

	XRGB(int x, const QColor& color) : x(x), red(color.red()), green(color.green()), blue(color.blue()) {}

	uint8_t& operator()(ColorComponent color) {
		using CC = ColorComponent;

		switch (color) {
		case CC::red:
			return red;

		case CC::green:
			return green;

		case CC::blue:
			return blue;
		}
		return red;
	}

	const uint8_t& operator()(ColorComponent color) const {
		using CC = ColorComponent;

		switch (color) {
		case CC::red:
			return red;

		case CC::green:
			return green;

		case CC::blue:
			return blue;
		}
		return red;

	}
};


class GradientImage {

	int m_poly = 1;

	Matrix m_coef;
	QImage m_src;

	Qt::Orientation m_direction = Qt::Horizontal;

public:
	enum Corner : uint8_t {
		none = 0x0000,
		top_left = 0x0001,
		top_right = 0x0002,
		bot_left = 0x0004,
		bot_right = 0x0008
	};

	typedef uint8_t Corner_t;

	const QImage& image()const { return m_src; }

	GradientImage(const QSize& size, Qt::Orientation orientation = Qt::Horizontal, int poly_degree = 1);

	GradientImage() = default;

private:
	double computePolynomial(const Matrix& coefficients, int x, int poly_degree);

	Matrix computeCoeficients(const std::vector<XRGB>& pts, ColorComponent cc, int poly_deg);

	Matrix computeCoeficients_RGB();

public:
	void setGradient(const std::vector<XRGB>& pts, float opacity = 1.0f);

	void setGradient(const std::vector<XRGB>& r, const std::vector<XRGB>& g, const std::vector<XRGB>& b, float opacity = 1.0f);

	void setGradientRGB(float opacity);

	void cutCorners(Corner_t corner_flags = Corner::none);
};





class CurveScene : public QGraphicsScene {
	Q_OBJECT
private:

	int click_x = 0;
	int click_y = 0;

protected:
	std::vector<double> m_values = std::vector<double>(width());
	QGraphicsItem* m_current = nullptr;

	std::array<QColor, 10> m_color = { QColor{255,000,000}, {000,255,000}, {000,000,255}, {255,255,255}, {255,255,255},
											 {255,127,000}, {000,255,127}, {255,000,128}, {255,255,0}, {178,102,255} };

	ColorComponent m_comp = ColorComponent::rgb_k;
	std::array<CurveItem*, 10> m_curve_items;

public:
	CurveScene(QRect rect, QWidget* parent);

	CurveScene() = default;

	CurveItem* curveItem(ColorComponent comp) {
		return m_curve_items[int(comp)];
	}

	QGraphicsItem* currentItem()const { return m_current; }

	void drawGrid();

	virtual void resetScene();

	QColor color(ColorComponent comp) { return m_color[int(comp)]; }

	ColorComponent currentComponent()const { return m_comp; }

public slots:
	void onChannelChange(int id);

protected:
	virtual void onUpdatePoint(QPointF point);

signals:
	void itemARC();

	void endItem(bool);

	void currentPos(QPointF point);

private:
	bool xValueExists()const;

	bool isInScene(const QPointF& p)const;

	bool isInScene_Edge(const QPointF& rp)const;

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);

	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event);

	virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
};
