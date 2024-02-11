#pragma once

#include "HistogramTransformation.h"
#include <QMainWindow>
#include "ui_ImageWindow.h"
#include "Image.h"
#include <QWheelEvent>
#include <QCloseEvent>


class IWSS: public QMdiSubWindow {
	Q_OBJECT

public:
	IWSS() {}
	~IWSS() {}

public slots:

signals:
	void sendWindowClose();

	void sendWindowOpen();
};


class ScrollBar : public QScrollBar {
	Q_OBJECT

public:
	ScrollBar(QWidget* parent = nullptr) {}

	signals:
	void wheelEvent(QWheelEvent* event);

};


template<typename T>
class ImageWindow:public QWidget {
	//Q_OBJECT
	QWidget* m_parent;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name;

public:
	Image<T> source;
	IWSS* iws;

private:

	HistogramTransformation stf;
	bool m_stf = false;
	bool compute_stf = true;

	QLabel* label;
	QImage display;
	QPixmap output;

	QScrollArea* sa;
	ScrollBar* sbh;
	ScrollBar* sbv;

	int m_min_factor_poll = -10;
	int m_max_factor_poll = 15;
	int m_factor_poll = -4;

	double m_factor = 1.0 / abs(m_factor_poll);//0.25;
	double m_old_factor = m_factor;
	double m_initial_factor = m_factor;

	//Qt::WindowState m_state = Qt::WindowState::WindowNoState;

	int m_winRows = 0;
	int m_winCols = 0;

	double m_sourceOffX = 0;
	double m_sourceOffY = 0;

	int m_mouseX = 0;
	int m_mouseY = 0;

	int m_scrollbarX = 0;
	int m_scrollbarY = 0;

	bool m_open = false;

public:

	ImageWindow() = default;

	ImageWindow(Image<T>& img, QString name, QWidget* parent = nullptr);

	~ImageWindow() {}

	int Rows()const { return m_drows; }

	int Cols()const { return m_dcols; }

	int Bitdepth()const { return source.Bitdepth(); }

	QString ImageName()const { return m_name; }

private:

	void sliderPressedX();

	void sliderPressedY();

	void sliderPanX(int value);

	void sliderPanY(int value);

	void sliderArrowX(int action);

	void sliderArrowY(int action);

	void sliderWheelX(QWheelEvent* event);

	void sliderWheelY(QWheelEvent* event);


	void wheelEvent(QWheelEvent* event);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void closeEvent(QCloseEvent* close);

	//called when window opens
	//when window min/max
	//when window resizes
	void resizeEvent(QResizeEvent* event);

	//make it dependent on size of workspace
	int IdealFactor() {
		int width = screen()->availableSize().width();
		int height = screen()->availableSize().height();

		double ratio = double(width) / height;

		int factor = 1;
		for (; factor < 5; ++factor) {

			int new_cols = source.Cols() / factor;
			int new_rows = source.Rows() / factor;

			if (new_cols < 0.55 * width || new_rows < 0.7 * height)
				break;
		}

		m_factor_poll =	(factor == 1) ? factor : - factor;
		m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

		return factor;
	}

	void ResizeWindowtoNormal();

	void ResizeDisplay();

	void DisplayImage();


	void InstantiateScrollBars();

	void ShowHorizontalScrollBar();

	void HideHorizontalScrollBar();

	void ShowVerticalScrollBar();

	void HideVerticalScrollBar();

	void ShowScrollBars();


	void Zoom(int x, int y);

	void Pan(int x, int y);

	void Pan_SliderX(int x);

	void Pan_SliderY(int y);


	void BinToWindow_RGB(int x_start, int y_start, int factor);

	void BinToWindow(int x_start, int y_start, int factor);

	void BinToWindow2(int x_start, int y_start, int factor);


	void UpsampleToWindow_RGB(double x_start, double y_start, int factor);

	void UpsampleToWindow(double x_start, double y_start, int factor);

};

typedef ImageWindow<uint8_t> ImageWindow8;
typedef ImageWindow<uint16_t> ImageWindow16;
typedef ImageWindow<float> ImageWindow32;