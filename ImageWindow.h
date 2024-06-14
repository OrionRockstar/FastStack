#pragma once

//#include "HistogramTransformation.h"
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
	QWidget* m_workspace;
	QMdiSubWindow* m_sw_parent;

	QString m_ss = "QMdiSubWindow:title {color:blue;}"
		"QMdiSubWindow[Focus=true]:window {border-width: 4px; border-style: solid; border-color: #1A81E8; "
		" border-top-left-radius: 12px;  border-top-right-radius: 12px; border-bottom-left-radius: 5px; border-bottom-right-radius: 5px;}";

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name;

	Image<T> m_source;

	std::unique_ptr<QDialog> m_preview = nullptr;

	//HistogramTransformation stf;
	bool m_stf = false;
	bool compute_stf = true;

	QLabel* m_label;
	QImage m_display;
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

	int m_titlebar_height = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
	int m_border_width = QApplication::style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);

public:
	IWSS* iws;

	ImageWindow() = default;

	ImageWindow(Image<T>& img, QString name, QWidget* parent = nullptr);

	~ImageWindow() {}

	int Rows()const { return m_drows; }

	int Cols()const { return m_dcols; }

	int Bitdepth()const { return m_source.Bitdepth(); }

	QString ImageName()const { return m_name; }

	Image<T>& Source() { return m_source; }

	bool previewExists() {
		return (m_preview.get() != nullptr) ? true : false;
	}

	QDialog* Preview()const { return m_preview.get(); }

private:
	//slots
	void sliderPressed_X();

	void sliderPressed_Y();

	void sliderMoved_X(int value);

	void sliderMoved_Y(int value);

	void sliderArrowX(int action);

	void sliderArrowY(int action);

	//events
	void sliderWheelX(QWheelEvent* event);

	void sliderWheelY(QWheelEvent* event);

	void dragEnterEvent(QDragEnterEvent* event);

	void dropEvent(QDropEvent* event);

	void wheelEvent(QWheelEvent* event);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void closeEvent(QCloseEvent* close);

	bool eventFilter(QObject* object, QEvent* event);

	//called when window opens
	//when window min/max
	//when window resizes
	void resizeEvent(QResizeEvent* event);

	void ResizeDisplay();

	//void Apply(Image<T>&) {}

public:
	int IdealZoomFactor(bool workspace = false);

	template<class P>
	void UpdateImage(P& t, void (P::*apply)(Image<T>&)) {

		(t.*apply)(m_source);

		m_drows = m_source.Rows() * m_factor;
		m_dcols = m_source.Cols() * m_factor;

		//m_sourceOffX = Clip(m_sourceOffX, 0.0, double(m_source.Cols()));
		//m_sourceOffY = Clip(m_sourceOffY, 0.0, double(m_source.Rows()));

		ShowScrollBars();
		ResizeDisplay();
		DisplayImage();
	}

	void DisplayImage();

	void ShowPreview();

	void ShowPreview(QDialog* p) {
		if (m_preview == nullptr)
			m_preview = std::unique_ptr<QDialog>(p);
	}

	void ClosePreview();

private:
	void InstantiateScrollBars();

	void ShowHorizontalScrollBar();

	void HideHorizontalScrollBar();

	void ShowVerticalScrollBar();

	void HideVerticalScrollBar();

	bool isHorizontalScrollBarOn();

	bool isVerticalScrollBarOn();

	bool isHorizontalScrollBarOff();

	bool isVerticalScrollBarOff();

	void ShowScrollBars();


	void Zoom(int x, int y);

	void Pan(int x, int y);

	void Pan_SliderX(int x);

	void Pan_SliderY(int y);


	void BinToWindow_RGB(int x_start, int y_start, int factor);

	void BinToWindow(int x_start, int y_start, int factor);

	void BinToWindow_STF(int x_start, int y_start, int factor);


	void UpsampleToWindow_RGB(double x_start, double y_start, int factor);

	void UpsampleToWindow(double x_start, double y_start, int factor);

};

typedef ImageWindow<uint8_t> ImageWindow8;
typedef ImageWindow<uint16_t> ImageWindow16;
typedef ImageWindow<float> ImageWindow32;




template<typename T>
static void ImagetoQImage(Image<T>& src, QImage& dst) {

	if (src.Rows() != dst.height() || src.Cols() != dst.width()) {

		QImage::Format format;

		if (src.Channels() == 1)
			format = QImage::Format::Format_Grayscale8;

		else if (src.Channels() == 3)
			format = QImage::Format::Format_RGB888;

		dst = QImage(src.Cols(), src.Rows(), format);
	}

	for (int ch = 0; ch < src.Channels(); ++ch)
		for (int y = 0; y < src.Rows(); ++y)
			for (int x = 0; x < src.Cols(); ++x)
				dst.scanLine(y)[src.Channels() * x + ch] = Pixel<uint8_t>::toType(src(x, y, ch));

}




template <typename T>
class PreviewWindow: public QDialog {

private:
	int m_bin_factor = 1;
	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name = "Real-Time Preview: ";

	IWSS* iws;

	Image<T> m_source;
	QLabel* label;
	QImage display;
	QPixmap output;

public:
	PreviewWindow(QWidget* parent = nullptr);

	void closeEvent(QCloseEvent* close) {
		iws->sendWindowClose();
		close->accept();
	}

	bool eventFilter(QObject* o, QEvent* e) {

		if (o == this && e->type() == QEvent::NonClientAreaMouseButtonPress)
			this->setWindowOpacity(.55);
		if (o == this && e->type() == QEvent::NonClientAreaMouseButtonRelease)
			this->setWindowOpacity(1.0);

		return false;
	}

	QString Name()const { return m_name; }

	//Image<T>& Source() { return m_source; }

	int BinFactor()const { return m_bin_factor; }

	/*void UpdatefromParent_RGB() {

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
		
	}*/

	//use binning instead?
private:
	void UpdatefromParent();

	void UpdatefromParent_QualityRGB();

	void UpdatefromParent_Quality();

	void UpdatefromSource_Quality(Image<T>& src);

	void DisplayImage();

	//void UpdatePreview()
public:
	template<class P>
	void UpdatePreview(P& obj, void (P::*apply)(Image<T>&)) {
		UpdatefromParent_Quality();
		(obj.*apply)(m_source);
		DisplayImage();
	}

	void UpdatePreview(Image<T>& src);
};

typedef PreviewWindow<uint8_t> PreviewWindow8;
typedef PreviewWindow<uint16_t> PreviewWindow16;
typedef PreviewWindow<float> PreviewWindow32;
