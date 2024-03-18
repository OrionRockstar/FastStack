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

private:
	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name;


public:
	Image<T> source;
	IWSS* iws;
	QDialog* rtp = nullptr;

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

public:
	void onRTPClose() {
		rtp = nullptr;
	}

	bool rtpExists() {
		return (rtp != nullptr) ? true : false;
	}

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

public:
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
private:
	void ResizeWindowtoNormal();

	void ResizeDisplay();

public:
	void DisplayImage();

	void ShowRTP();
private:
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

//have parent be current subwindow and current process???
template <typename T>
class RTP_ImageWindow: public QDialog {

	ImageWindow<T>* m_parent;

private:
	int m_bin_factor = 1;
	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name = "Real-Time Preview: ";

public:
	Image<T> source;
	Image<T> modified;
	IWSS* iws;

private:
	QLabel* label;
	QImage display;
	QPixmap output;

public:
	RTP_ImageWindow(QWidget* parent = nullptr) : QDialog(parent) {

		m_parent = reinterpret_cast<ImageWindow<T>*>(parent);
		m_bin_factor = m_parent->IdealFactor();

		m_drows = m_parent->source.Rows() / m_bin_factor;
		m_dcols = m_parent->source.Cols() / m_bin_factor;
		m_dchannels = m_parent->source.Channels();


		source = Image<T>(m_drows, m_dcols, m_dchannels);
		//modified = Image<T>(m_drows, m_dcols, m_dchannels);

		label = new QLabel(this);
		label->setGeometry(0, 0, m_dcols, m_drows);

		iws = new IWSS();

		if (m_dchannels == 1)
			display = QImage(m_dcols, m_drows, QImage::Format::Format_Grayscale8);

		else if (m_dchannels == 3)
			display = QImage(m_dcols, m_drows, QImage::Format::Format_RGB888);

		connect(iws, &IWSS::sendWindowClose, m_parent, &ImageWindow<T>::onRTPClose);

		UpdatefromParent();

		CopyFromTo(source, display);

		output.convertFromImage(display);
		label->setPixmap(output);

		this->setWindowTitle(m_name);
		setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
		setAttribute(Qt::WA_DeleteOnClose);
		show();
	}

	/*RTP_ImageWindow(bool bin, QWidget* parent = nullptr) : QDialog(parent) {

		m_parent = reinterpret_cast<ImageWindow<T>*>(parent);
		m_bin_factor = m_parent->IdealFactor();

		m_drows = m_parent->source.Rows() / m_bin_factor;
		m_dcols = m_parent->source.Cols() / m_bin_factor;
		m_dchannels = m_parent->source.Channels();

		if (bin)
			source = Image<T>(m_drows, m_dcols, m_dchannels);
		else
			source = Image<T>(m_parent->source.Rows(), m_parent->source.Cols(), m_dchannels);
		UpdatefromPar();

		label = new QLabel(this);
		label->setGeometry(0, 0, m_dcols, m_drows);

		iws = new IWSS();

		if (m_dchannels == 1)
			display = QImage(m_dcols, m_drows, QImage::Format::Format_Grayscale8);

		else if (m_dchannels == 3)
			display = QImage(m_dcols, m_drows, QImage::Format::Format_RGB888);

		connect(iws, &IWSS::sendWindowClose, m_parent, &ImageWindow<T>::onRTPClose);

		//BintoPreview();
		//CopyFromTo(source, display);

		output.convertFromImage(display);
		label->setPixmap(output);

		this->setWindowTitle(m_name);
		setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
		setAttribute(Qt::WA_DeleteOnClose);
		show();
	}*/

	RTP_ImageWindow() = default;

	void sendClose();

	void closeEvent(QCloseEvent* close) {
		iws->sendWindowClose();
		close->accept();
	}

	QString Name()const { return m_name; }

	int BinFactor()const { return m_bin_factor; }

	void UpdatefromParent_RGB() {

		int factor = m_bin_factor;
		int factor2 = factor * factor;

		for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
			for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

				double r = 0, g = 0, b = 0;

				for (int j = 0; j < factor; ++j)
					for (int i = 0; i < factor; ++i) {
						r += m_parent->source(x_s + i, y_s + j, 0);
						g += m_parent->source(x_s + i, y_s + j, 1);
						b += m_parent->source(x_s + i, y_s + j, 2);
					}

				source(x, y, 0) = r / factor2;
				source(x, y, 1) = g / factor2;
				source(x, y, 2) = b / factor2;
			}
		}
		
	}

	//use binning instead?
	void UpdatefromParent() {

		int factor = m_bin_factor;
		int factor2 = factor * factor;

		for (int ch = 0; ch < m_dchannels; ++ch) {
			for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
				for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

					/*double pix = 0;

					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							pix += m_parent->source(x_s + i, y_s + j, ch);

					source(x, y, ch) = pix / factor2;*/

					source(x,y,ch) = m_parent->source(x_s, y_s, ch);
				}
			}
		}
	}

	void CopyFromTo(Image<T>& src, QImage& dst) {
		for (int ch = 0; ch < m_dchannels; ++ch) 
			for (int y = 0; y < m_drows; ++y) 
				for (int x = 0; x < m_dcols; ++x) 
					display.scanLine(y)[m_dchannels * x + ch] = Pixel<uint8_t>::toType(src(x,y, ch));
	}

	void DisplayImage() {
		CopyFromTo(source, display);
		output.convertFromImage(display);
		label->setPixmap(output);
		//source.CopyTo(modified);
	}

};

typedef RTP_ImageWindow<uint8_t> RTP_ImageWindow8;
typedef RTP_ImageWindow<uint16_t> RTP_ImageWindow16;
typedef RTP_ImageWindow<float> RTP_ImageWindow32;

