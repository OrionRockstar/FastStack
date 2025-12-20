#pragma once
#include "Histogram.h"
#include "CustomWidgets.h"
#include "qgraphicsscene.h"
#include "qgraphicsview.h"

class HistogramScene : QGraphicsScene {

	QPen m_cursor_pen;
public:
	HistogramScene() = default;

//private:
	//void paintEvent(QPaintEvent* e) {}
};





class HistogramView : public QGraphicsView {
	Q_OBJECT

public:
	enum class GraphStyle : uint8_t {
		line,
		area,
		bars,
		dots
	};
private:
	QGraphicsScene* m_gs = nullptr;

	ColorComponent m_comp = ColorComponent::rgb_k;

	HistogramVector m_histograms;

	QPoint m_cursor_pos;
	std::array<QGraphicsLineItem*, 2> m_cursor_lines = { nullptr,nullptr };

	int m_scale = 1;
	int m_old_scale = 1;
	bool m_clip = true;

	ScrollBar* m_vsb;
	ScrollBar* m_hsb;
	QWidget* m_corner;

	QRect m_hist_rect;
	int m_offsetx = 0;
	int m_offsety = 0;

	GraphStyle m_graph_style = GraphStyle::line;

protected:
	const std::array<QPen, 4> m_pens = { QPen(Qt::red), {Qt::green}, {Qt::blue}, {Qt::white} };

public:
	HistogramView(const QSize& size, QWidget* parent = nullptr);

signals:
	void histogramValue(uint16_t pixelValue_low, uint16_t pixelValue_high, uint32_t pixel_count);

	void cursorLeave();

public:
	ColorComponent colorComponent()const { return m_comp; }

	void setColorComponent(ColorComponent comp) { m_comp = comp; }

	void clipHistogram(bool clip = true) { m_clip = clip; drawHistogram(); }

	bool isClippedHistogram()const { return m_clip; }

	GraphStyle graphStyle()const { return m_graph_style; }

	void setGraphStyle(GraphStyle style) { m_graph_style = style; drawHistogram(); }

	template<typename T>
	void drawHistogramView(const Image<T>& img, Histogram::Resolution resolution);

	const std::vector<Histogram>& histogram()const { return m_histograms; }

	void setHistogram(const std::vector<Histogram>& histogram) {
		m_histograms = histogram;
		drawHistogram();
	}

private:
	void adjustHistogramRect();

	void emitHistogramValue(const QPoint& p);

	void removeCursor();

	void drawCursor(const QPoint& p);

	void moveCursor(const QPoint& dst);

	void drawGrid();

public:
	virtual void clearHistogram();

protected:
	virtual void drawHistogram();

	virtual void clearScene(bool draw_grid = true);

private:
	int scale()const { return m_scale; }

	int oldScale()const { return m_old_scale; }

	void addScrollBars();

	void showHorizontalScrollBar();

	void showVerticalScrollBar();

	void showScrollBars();

	bool eventFilter(QObject* obj, QEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void resizeEvent(QResizeEvent* e);

	void scrollContentsBy(int dx, int dy) {}

protected:
	virtual void wheelEvent(QWheelEvent* e);
};