#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "Histogram.h"
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QSurfaceDataProxy>

struct Statistics {

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

	template<typename T>
	static StatsVector computeStatistics(const Image<T>& img, bool clip = false);
};





class StatisticsDialog : public QDialog {
	Q_OBJECT

	class Label : public QLabel {

	public:
		Label(const QString& txt, QWidget* parent = nullptr) : QLabel(txt, parent) {
			this->setAlignment(Qt::AlignCenter);
		}
	};

	CheckBox* m_clip_cb;

	QTableWidget* m_stats_table;
	
	const QStringList m_stat_labels = { "Px Count","Mean","Median","StdDev","AvgDev","MAD","Minimum","Maximum","","" };

public:
	StatisticsDialog(const QString& img_name, const Statistics::StatsVector& statsvector, int precision, QWidget* parent);

	void populateStats(const Statistics::StatsVector& statsvector, int precision);

	bool isChecked()const { return m_clip_cb->isChecked(); }

	signals:
	void onClose();

	void clipped(bool clip);

private:
	void closeEvent(QCloseEvent* e) {
		QDialog::closeEvent(e);
		emit onClose();
		e->accept();
	}
};






class HistogramView : public QGraphicsView {
	Q_OBJECT

public:
	enum class Resolution : uint32_t {
		_8bit = 256,
		_10bit = 1024,
		_12bit = 4096,
		_14bit = 16383,
		_16bit = 65536
	};

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
	void constructHistogram(const Image<T>& img, Resolution resolution);

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



class Titlebar : public QWidget {

	QLabel* m_icon_label;
	QLabel* m_title_label;

public:
	Titlebar(const QIcon& icon, const QString& text, QWidget& parent) : QWidget(&parent) {
		this->setFixedHeight(30);
		this->resize(parent.width(), height());

		m_icon_label = new QLabel(this);
		m_icon_label->setPixmap(icon.pixmap(20, 20));
		m_icon_label->move(5, 5);

		m_title_label = new QLabel(text, this);
		m_title_label->move(40, 7);

		std::cout << this->backgroundRole() << "\n";

		QPalette p;
		p.setColor(QPalette::ColorRole::Window, Qt::red);
		this->setPalette(p);
		this->setAutoFillBackground(true);
	}
};



class HistogramDialog : public QDialog {
	Q_OBJECT

	HistogramView* m_hist_view = nullptr;
	CheckBox* m_clip_cb = nullptr;

	QLabel* m_hist_data = nullptr;

public:
	template<typename T>
	HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

	HistogramView* histogramView()const { return m_hist_view; }

signals:
	void onClose();

private:
	void resizeEvent(QResizeEvent* e);

	void closeEvent(QCloseEvent* e);
};




class Image3DDialog : public QDialog {
	Q_OBJECT

	Q3DSurface* m_graph = nullptr;
	QWidget* m_container = nullptr;
	
	QSurfaceDataProxy* m_img_proxy = nullptr;
	QSurface3DSeries* m_img_series = nullptr;

	const float darkRedPos = 1.0f;
	const float redPos = 0.8f;
	const float yellowPos = 0.6f;
	const float greenPos = 0.4f;
	const float darkGreenPos = 0.2f;

public:
	template<typename T>
	Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

signals:
	void onClose();

private:
	template<typename T>
	int computeBinFactor(const Image<T>& img);

	template<typename T>
	Image<T> binImage(const Image<T>& img);

	template<typename T>
	float lumValue(const Image<T>& img, const Point& p);

	template<typename T>
	void fillImageProxy(const Image<T>& img);

	void closeEvent(QCloseEvent* e) {

		QDialog::closeEvent(e);
		emit onClose();
		e->accept();
	}
};