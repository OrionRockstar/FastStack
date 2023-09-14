#pragma once

#include <QMainWindow>
#include "ui_ImageWindow.h"
#include "Image.h"
#include <QWheelEvent>

//template <typename T>
class ImageWindow : public QMainWindow
{
	Q_OBJECT

public:
	ImageWindow(Image8& img, QWidget* parent = nullptr);

	ImageWindow(Image16& img, QWidget* parent = nullptr);

	ImageWindow(Image32& img, QWidget* parent = nullptr);

	~ImageWindow();

private:
	Ui::ImageWindowClass ui;
	int m_rows = 0;
	int m_cols = 0;
	int m_channels = 0;

	Image32 source;
	Image8 source8;


	QLabel* label;
	QImage display;
	QPixmap output;

	QScrollArea* sa;
	QScrollBar* sbh;
	QScrollBar* sbv;
	
	int m_min_factor_poll = -4;
	int m_max_factor_poll = 15;

	float m_factor = 1.0 / abs(m_min_factor_poll);//0.25;
	float m_old_factor = m_factor;

	int m_factor_poll = m_min_factor_poll;
	
	int m_page_step = 1;
	float m_initial_factor = m_factor;

	int offsetx = 0;
	int offsety = 0;

	int mouse_offx = 0;
	int mouse_offy = 0;

	int scrollbar_offx = 0;
	int scrollbar_offy = 0;

public slots:
	void setSliderX() {
		scrollbar_offx = sbh->value() / -m_initial_factor;
	}

	void setSliderY() {
		scrollbar_offy = sbv->value() / -m_initial_factor;
	}

	void sliderPanX(int value) {
		value /= -m_initial_factor;
		if (value == scrollbar_offx)
			return;
		
		Pan_SliderX(value);// (m_initial_factor / m_factor), 0);
	}

	void sliderPanY(int value) {
		value /= -m_initial_factor;
		if (value == scrollbar_offy)
			return;

		Pan_SliderY(value);// (m_initial_factor / m_factor), 0);
	}


protected:
	virtual void wheelEvent(QWheelEvent* event);

	virtual void mousePressEvent(QMouseEvent* event);

	virtual void mouseMoveEvent(QMouseEvent* event);

private:
	void InstantiateScrollBars();

	void ShowScrollBars();

	void HideScrollBars();

	bool IsInWindow(int x, int y) { return 0 <= x && x < m_cols && 0 <= y && y < m_rows; }

	void BinToWindow(int x_start, int y_start, int factor);


	void BinToWindowBilinear(int x_start, int y_start, int factor);

	void BinToWindowBilinearRGB(int x_start, int y_start, int factor);

	void UpsampleToWindow(int x_start, int y_start, int factor);

	void Zoom(int x, int y);

	//decrease mouse sensitivity at higher zooms
	//withour increasing jitter
	//make offsetx and y float??
	int DeltaMouseAdjustment(float dm);

	void Pan(int x, int y);

	void Pan_SliderX(int x);

	void Pan_SliderY(int y);

};
