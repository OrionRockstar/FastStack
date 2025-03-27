#pragma once
#include "Image.h"
#include "Histogram.h"
#include "CustomWidgets.h"
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QSurfaceDataProxy>



struct Statistics {

	enum class BitDepth {
		_float = 1,
		_8bit = 255,
		_10bit = 1023,
		_12bit = 4095,
		_14bit = 16383,
		_16bit = 65535
	};

	ImageType type;

	uint32_t pixel_count;
	float mean = 0.0;
	float median = 0;
	float stdDev = 0.0;
	float avgDev = 0.0;
	float MAD = 0;
	//float sqrtBWMV = 0.0;
	float min = 0;
	float max = 1;

	typedef std::vector<Statistics> StatsVector;

private:
	Statistics& operator*=(BitDepth bitdepth) {
		mean *= int(bitdepth);
		median *= int(bitdepth);
		stdDev *= int(bitdepth);
		avgDev *= int(bitdepth);
		MAD *= int(bitdepth);
		min *= int(bitdepth);
		max *= int(bitdepth);

		return *this;
	}

public:
	template<typename T>
	static StatsVector computeStatistics(const Image<T>& img, bool clip = false);

private:
	template<typename T>
	void normalizedFromType(Statistics& s)const {

		s.mean = mean / Pixel<T>::max();
		s.median = Pixel<float>::toType(T(median));
		s.stdDev = stdDev / Pixel<T>::max();
		s.avgDev = avgDev / Pixel<T>::max();
		s.MAD = Pixel<float>::toType(T(MAD));
		s.min = Pixel<float>::toType(T(min));
		s.max = Pixel<float>::toType(T(max));
	}

public:
	Statistics toBitDepth(BitDepth bitdepth)const;
};





class StatisticsDialog : public Dialog {
	Q_OBJECT

	class Label : public QLabel {

	public:
		Label(const QString& txt, QWidget* parent = nullptr) : QLabel(txt, parent) {
			this->setAlignment(Qt::AlignCenter);
		}
	};

	const Statistics::StatsVector* m_stats_vector = nullptr;

	ComboBox* m_bit_depth_combo = nullptr;

	CheckBox* m_clip_cb;

	QTableWidget* m_stats_table;
	
	const QStringList m_stat_labels = { "Px Count","Mean","Median","StdDev","AvgDev","MAD","Minimum","Maximum","","" };

public:
	StatisticsDialog(const QString& img_name, const Statistics::StatsVector& statsvector, int precision, QWidget* parent);

	void updateStats(const Statistics::StatsVector& statsvector);

	bool isChecked()const { return m_clip_cb->isChecked(); }
signals:
	void clipped(bool clip);

private:
	void addStatsTable();

	void addBitDepthCombo();
};






class HistogramView : public QGraphicsView {
	Q_OBJECT


private:
	QGraphicsScene* m_gs = nullptr;

	ColorComponent m_comp = ColorComponent::rgb_k;

	std::vector<Histogram> m_histograms;

	std::array<QGraphicsLineItem*, 2> m_cursor_lines = {nullptr,nullptr};

	int m_scale = 1;
	int m_old_scale = 1;
	bool m_clip = true;

	ScrollBar* m_vsb;
	ScrollBar* m_hsb;

	QRect m_rect;
	QPoint m_start = QPoint(0,0);
	double m_offsetx = 0;
	double m_offsety = 0;


protected:
	const std::array<QPen, 4> m_pens = { QColor(255,0,0),QColor(0,255,0),QColor(0,0,255) ,QColor(255,255,255) };
	
public:
	HistogramView(const QSize& size, QWidget* parent = nullptr);

signals:
	void histogramValue(uint16_t pixelValue_low, uint16_t pixelValue_high, uint32_t pixel_count);

public:
	ColorComponent colorComponent()const { return m_comp; }

	void setColorComponent(ColorComponent comp) { m_comp = comp; }

	void drawGrid();
	
	template<typename T>
	void drawHistogramScene(const Image<T>& img);

	template<typename T>
	void constructHistogram(const Image<T>& img);

	template<typename T>
	void constructHistogram(const Image<T>& img, Histogram::Resolution resolution);

	void clearHistogram();

	void drawHistogram();

	void clipHistogram(bool clip = true) { m_clip = clip; drawHistogram(); }

	bool isClippedHistogram()const { return m_clip; }

private:
	void addScrollBars();

	void showHorizontalScrollBar();

	void hideHorizontalScrollBar();

	void showVerticalScrollBar();

	void hideVerticalScrollBar();

	void showScrollBars();

	void adjustHistogramRect();

	void removeCursor();

	void drawCursor(const QPoint& p);

protected:
	virtual void clearScene(bool draw_grid = true);

	virtual void resizeEvent(QResizeEvent* e);

	virtual void wheelEvent(QWheelEvent* e);

	bool eventFilter(QObject* obj, QEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void scrollContentsBy(int dx, int dy) {}
};




class HistogramDialog : public Dialog {
	Q_OBJECT

	HistogramView* m_hist_view = nullptr;
	CheckBox* m_clip_cb = nullptr;

	QLabel* m_hist_data = nullptr;

public:
	template<typename T>
	HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

	HistogramView* histogramView()const { return m_hist_view; }

private:
	void resizeEvent(QResizeEvent* e);
};






class SideWidget : public QWidget {
	Q_OBJECT

	enum class State : uint8_t {
		expanded,
		collapsed
	};
	
	//Dialog* m_dialog;
	State m_state = State::collapsed;
	const int m_collapsed_width = 25;
	const int m_expanded_width = 150;

	PushButton* m_resize_pb = nullptr;

public:
	SideWidget(QWidget* parent);

signals:
	void sizeChanged();

private:
	void onPress();
};


class Image3DDialog : public Dialog {
	Q_OBJECT


	Q3DSurface* m_graph = nullptr;
	QWidget* m_container = nullptr;
	
	QLinearGradient m_gradient = defaultGradient();
	QSurfaceDataProxy* m_img_proxy = nullptr;
	QSurface3DSeries* m_img_series = nullptr;
	int m_width = 0;

	SideWidget* m_side_widget = nullptr;

	const float darkRedPos = 1.0f;
	const float redPos = 0.8f;
	const float yellowPos = 0.6f;
	const float greenPos = 0.4f;
	const float darkGreenPos = 0.2f;

public:
	template<typename T>
	Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

private:
	QLinearGradient defaultGradient();

	template<typename T>
	int computeBinFactor(const Image<T>& img);

	template<typename T>
	Image<T> binImage(const Image<T>& img);

	template<typename T>
	float lumValue(const Image<T>& img, const Point& p);

	template<typename T>
	void fillImageProxy(const Image<T>& img);

};