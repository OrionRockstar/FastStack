#pragma once
//#include "ui_ImageWindow.h"
#include "Image.h"
#include <QOpenGLWidget>
#include "HistogramTransformation.h"
#include "ProcessDialog.h"
#include "Statistics.h"

class ImageLabelGL : public QOpenGLWidget {
public:
	ImageLabelGL(QWidget* parent = nullptr) : QOpenGLWidget(parent) {}

	void paintGL() override {
		QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
		//f->glTexImage2D()
	}
};



class ImageLabel : public QLabel {
	Q_OBJECT

	const QImage* m_image = nullptr;
	const QImage* m_mask = nullptr;
	bool m_mask_visible = true;

public:
	ImageLabel(const QImage* img, QWidget* parent);

	void showMask(bool show = true) { m_mask_visible = show; }

	void setMask(const QImage* mask) { m_mask = mask; }

	void removeImage() { m_image = nullptr; }

	void removeMask() { m_mask = nullptr; }

	void draw() { update(); }

signals:
	void mousePos(const QPoint& p);

	void mouseLeave();

private:
	void paintEvent(QPaintEvent* event);

	void enterEvent(QEnterEvent* event);

	void leaveEvent(QEvent* event);

	bool eventFilter(QObject* object, QEvent* event);
};





class WindowSignals : public QWidget {
	Q_OBJECT

public:
	WindowSignals() {}

	~WindowSignals() {}

signals:
	void windowClosed();

	void windowOpened();

	void imageInfo(QMdiSubWindow* window);

	void pixelValue(QMdiSubWindow* window, const QPointF p);

};





class MaskSelectionDialog : public ProcessDialog {

	QWidget* m_iw;

	QLineEdit* m_current_mask;
	ComboBox* m_new_mask;
public:
	MaskSelectionDialog(QWidget* image_window, QMdiArea* workspace_parent);

private:
	void resetDialog() {}

	void showPreview() {}

	void apply();
};





class ImageWindowMenu : public QMenu {
	QWidget* m_parent;
	QMdiArea* m_workspace;
	QMenu* m_mask;

	QButtonGroup* bg;
	std::array<QColor, 8> m_mask_colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow,
										Qt::magenta, Qt::cyan, {255,165,0}, {127,0,255} };
	std::array<QString, 8> m_color_names = { "Red","Green","Blue", "Yellow", 
									    "Magenta", "Cyan", "Orange", "Violet" };

	MaskSelectionDialog* m_msd = nullptr;
public:

	ImageWindowMenu(QMdiArea* workspace, QWidget* parent);

private:
	void removeMask();

	void showMask(bool show = true);

	void invertMask(bool invert = false);

	void enableMask(bool enable = true);

	void setMaskColor(const QColor& color);

	void addMaskMenu();

	void addSelectMaskSelection();

	void addMaskColorSelection();
};





class ImageWindowToolbar : public QWidget {
	Q_OBJECT

private:
	PushButton* m_img3D_pb = nullptr;
	PushButton* m_stats_pb = nullptr;
	PushButton* m_hist_pb = nullptr;
	CheckablePushButton* m_stf_pb = nullptr;
	CheckablePushButton* m_zoom_win_pb = nullptr;

public:
	ImageWindowToolbar(int height, QWidget* parent = nullptr);

	bool isSTFChecked()const { return m_stf_pb->isChecked(); }

signals:
	void zoomWindowClicked(bool checked);

	void stfClicked(bool checked);

	void image3DReleased();

	void statisticsReleased();

	void histogramReleased();

private:
	void resizeEvent(QResizeEvent* e);
};







class ZoomWindow;

template<typename T>
class ImageWindow:public QWidget {

	class IWScrollArea : public QScrollArea {

	public:
		IWScrollArea(QWidget* parent = nullptr) : QScrollArea(parent) {
			this->setBackgroundRole(QPalette::ColorRole::Dark);
			QString corner = "QAbstractScrollArea::corner{background-color: dark gray; }";
			this->setStyleSheet(corner);
		}

	private:
		void wheelEvent(QWheelEvent* e) { e->ignore(); }
	};

	QMdiArea* m_workspace;
	QMdiSubWindow* m_sw_parent;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name;
	Image<T> m_source;

	ImageLabel* m_image_label = nullptr;
	QImage m_display;
	QImage m_mask_display;

	ImageWindowToolbar* m_toolbar = nullptr;

	std::unique_ptr<QDialog> m_preview = nullptr;

	HistogramTransformation m_ht;
	bool m_compute_stf = true;

	const ImageWindow<uint8_t>* m_mask = nullptr;
	QColor m_mask_color = { 127,0,255 };
	bool m_mask_enabled = true;
	bool m_invert_mask = false;
	bool m_show_mask = true;

	IWScrollArea* m_sa = nullptr;
	ScrollBar* m_sbX = nullptr;
	ScrollBar* m_sbY = nullptr;

	int m_min_factor_poll = -10;
	int m_max_factor_poll = 30;
	int m_factor_poll = -4;

	double m_factor = 1.0 / abs(m_factor_poll);//0.25;
	double m_old_factor = m_factor;
	double m_initial_factor = m_factor;

	double m_sourceOffX = 0;
	double m_sourceOffY = 0;

	int m_mouseX = 0;
	int m_mouseY = 0;

	bool m_open = false;

	int m_titlebar_height = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
	int m_border_width = QApplication::style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);

	std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();
	QCursor m_cursor = QCursor(Qt::CursorShape::CrossCursor);

	std::unique_ptr<StatisticsDialog> m_stats_dialog;
	Statistics::StatsVector m_sv;
	Statistics::StatsVector m_sv_clipped;

	std::unique_ptr<HistogramDialog> m_hist_dialog;

	std::unique_ptr<Image3DDialog> m_img3D_dialog;

	bool m_draw_zoom_win = false;
	std::unique_ptr<ZoomWindow> m_zoom_window;

public:
	WindowSignals* windowSignals()const { return m_ws.get(); }

	ImageWindow() = default;

	ImageWindow(Image<T>& img, QString name, QWidget* parent = nullptr);

	int computeZoomFactor(bool workspace = false)const;

	int rows()const { return m_drows; }

	int cols()const { return m_dcols; }

	int channels()const { return m_dchannels; }

	ImageType type()const { return m_source.type(); }

	QString name()const { return m_name; }

	double factor()const { return m_factor; }

	double sourceOffsetX()const { return m_sourceOffX; }

	double sourceOffsetY()const { return m_sourceOffY; }

	const HistogramTransformation& histogramTransformation()const { return m_ht; }

	const ImageWindowToolbar* toolbar()const { return m_toolbar; }

	Image<T>& source() { return m_source; }

	const Image<T>& source()const { return m_source; }

	QMdiSubWindow* subwindow()const { return m_sw_parent; }

	const ZoomWindow* zoomWindow()const { return m_zoom_window.get(); }

	const ImageWindow<uint8_t>* mask()const { return m_mask; }

	bool maskExists()const { return (m_mask != nullptr) ? true : false; }


	void setMask(const ImageWindow<uint8_t>* mask);

	void removeMask();

	bool maskEnabled()const { return m_mask_enabled; }

	void enableMask(bool enable = false);

	bool maskInverted()const { return m_invert_mask; }

	void invertMask(bool invert = false);

	bool maskVisible()const { return m_show_mask; }

	void showMask(bool show = true);

	QColor maskColor()const { return m_mask_color; }

	void setMaskColor(const QColor& color) { m_mask_color = color; displayImage(); }


	bool previewExists()const { return (m_preview.get() != nullptr) ? true : false; }

	QDialog* preview()const { return m_preview.get(); }

	void showPreview(QDialog* preview = nullptr);

	void closePreview();

	void showZoomWindow(bool show = false);

	bool isZoomWindowActive()const { return (m_zoom_window != nullptr); }

	void convertToGrayscale();

private:
	void mouseEnterMove_ImageLabel(const QPoint& p)const;

	void mouseLeave_ImageLabel()const;

	void wheelEvent(QWheelEvent* event);

	void dragEnterEvent(QDragEnterEvent* event);

	void dropEvent(QDropEvent* event);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void mouseReleaseEvent(QMouseEvent* event);

	void closeEvent(QCloseEvent* close);

	void resizeEvent(QResizeEvent* event);

	bool eventFilter(QObject* object, QEvent* event);


public:
	template<class P>
	void applyToSource(P& t, void (P::* apply)(Image<T>&)) {

		if (maskEnabled() && this->maskExists())
			return this->applyWithMask<P>(t, apply);

		(t.*apply)(m_source);

		m_compute_stf = true;

		resizeDisplay();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();
	}

	//take into account differing number of channels? or create new method
	template<class P>
	void applyToSource_Geometry(P& t, void (P::* apply)(Image<T>&)) {

		removeMask();

		(t.*apply)(m_source);

		int factor = computeZoomFactor(true);
		m_factor_poll = (factor == 1) ? factor : -factor;
		m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

		QString str = QString::number(abs(m_factor_poll));
		if (m_factor > 1)
			this->setWindowTitle(str + ":1" + " " + m_name);
		else
			this->setWindowTitle("1:" + str + " " + m_name);

		m_drows = m_source.rows() * m_factor;
		m_dcols = m_source.cols() * m_factor;

		//showScrollBars();
		m_sourceOffX = Clip(m_sourceOffX, 0.0, double(m_source.cols() - cols() / m_factor));
		m_sourceOffY = Clip(m_sourceOffY, 0.0, double(m_source.rows() - rows() / m_factor));
		resizeDisplay();

		m_old_factor = m_factor;

		displayImage();
		updateStatisticsDialog();
	}

private:
	template<typename P>
	void applyMask(const Image<T>& modified) {

		const Image<P>* mask = reinterpret_cast<const Image<P>*>(&m_mask->source());

		for (int ch = 0; ch < source().channels(); ++ch) {
			int mask_ch = (ch < mask->channels()) ? ch : 0;
			for (int y = 0; y < source().rows(); ++y) {
				for (int x = 0; x < source().cols(); ++x) {
					float m = Pixel<float>::toType((*mask)(x, y, mask_ch));
					m = (maskInverted()) ? 1 - m : m;
					m_source(x, y, ch) = m_source(x, y, ch) * (1 - m) + modified(x, y, ch) * m;
				}
			}
		}
	}

	template<class P>
	void applyWithMask(P& t, void (P::* apply)(Image<T>&)) {

		Image<T> mod = Image<T>(m_source);
		(t.*apply)(mod);

		switch (m_mask->type()) {
		case ImageType::UBYTE:
			applyMask<uint8_t>(mod);
			break;
		case ImageType::USHORT:
			applyMask<uint16_t>(mod);
			break;
		case ImageType::FLOAT:
			applyMask<float>(mod);
			break;
		}

		//showScrollBars();
		resizeDisplay();
		displayImage();
		updateStatisticsDialog();
	}


	void instantiateScrollBars();

	void showHorizontalScrollBar();

	void hideHorizontalScrollBar();

	void showVerticalScrollBar();

	void hideVerticalScrollBar();

	bool isHorizontalScrollBarOn()const;

	bool isVerticalScrollBarOn()const;

	bool isHorizontalScrollBarOff()const;

	bool isVerticalScrollBarOff()const;

	void showScrollBars();


	void resizeDisplay();

	void displayImage();

	void displayMask();


	void zoom(int x, int y);

	void pan(int x, int y);


	void binToWindow(int factor);

	void binToWindow_stf(int factor);

	void binToWindow_RGB(int factor);

	template<typename P>
	void binToWindow_Mask(const Image<P>& src, int factor) {

		int factor2 = factor * factor;

		for (int ch = 0; ch < src.channels(); ++ch) {
			for (int y = 0, y_s = m_sourceOffY; y < rows(); ++y, y_s += factor) {
				for (int x = 0, x_s = m_sourceOffX; x < cols(); ++x, x_s += factor ) {

					uint8_t* mp = &m_mask_display.scanLine(y)[4 * x + (2 - ch)];
					double pix = 0;

					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							pix += src(x_s + i, y_s + j, ch);

					*mp = 255 - Pixel<uint8_t>::toType(P(pix / factor2));

					if (m_invert_mask)
						*mp = 255 - *mp;
				}
			}
		}
	}

	void upsampleToWindow(int factor);

	void upsampleToWindow_stf(int factor);

	void upsampleToWindow_RGB(int factor);

	template<typename P>
	void upsampleToWindow_Mask(const Image<P>& src, int factor) {

		int n = m_mask_display.depth() / 8;
		int factor2 = factor * factor;

		double dd = 1.0 / factor;

		for (int ch = 0; ch < src.channels(); ++ch) {
			double y_s = m_sourceOffY;
			for (int y = 0; y < rows(); ++y, y_s += dd) {
				double x_s = m_sourceOffX;
				for (int x = 0; x < cols(); ++x, x_s += dd) {
					uint8_t* mp = &m_mask_display.scanLine(y)[4 * x + (2 - ch)];

					*mp = 255 - Pixel<uint8_t>::toType(src(x_s, y_s, ch));

					if (m_invert_mask)
						*mp = 255 - *mp;

				}
			}
		}
	}	

	void openStatisticsDialog();

	void updateStatisticsDialog();

	void openHistogramDialog();

	void updateHistogramDialog();

	void openImage3DDialog();

	//expand stf to preview, stf applied to pixels after process is applied to preview
	void showSTFImage();
};

template<typename P = uint8_t>
static ImageWindow<P>* imageRecast(QWidget* window) { return static_cast<ImageWindow<P>*>(window); }

template<typename P, typename T>
static ImageWindow<P>* imageRecast(ImageWindow<T>* window) { return reinterpret_cast<ImageWindow<P>*>(window); }

template<typename P, typename T>
static const ImageWindow<P>* imageRecast(const ImageWindow<T>* window) { return reinterpret_cast<const ImageWindow<P>*>(window); }

typedef ImageWindow<uint8_t> ImageWindow8;
typedef ImageWindow<uint16_t> ImageWindow16;
typedef ImageWindow<float> ImageWindow32;





class ZoomWindow : public QWidget {
	WindowSignals m_ws;

	const ImageWindow8* m_iw = nullptr;
	int m_pen_width = 3;

	QRect m_img_rect;
	QCursor m_cursor;

	QPoint m_pos;

public:
	template<typename T>
	ZoomWindow(const ImageWindow<T>& iw, QWidget* parent = nullptr) : m_iw(reinterpret_cast<const ImageWindow8*>(&iw)), QWidget(parent) {
		//this->setWindowFlags(Qt::MouseP)
		this->show();
	}

	template<typename T>
	ZoomWindow(const ImageWindow<T>& iw, const QRect& geometry, QWidget* parent = nullptr) : m_iw(reinterpret_cast<const ImageWindow8*>(&iw)), QWidget(parent) {

		this->setMinimumSize(20, 20);
		this->setGeometry(geometry);

		double factor = iw.factor();
		float x = geometry.x() / factor + m_iw->sourceOffsetX();
		float y = geometry.y() / factor + m_iw->sourceOffsetY();
		float w = geometry.width() / factor;
		float h = geometry.height() / factor;

		m_img_rect = QRect(x, y, w, h);

		this->show();
	}

	const WindowSignals* windowSignals()const { return &m_ws; }

	QRect frameRect()const;

	QRect imageRect()const { return m_img_rect; }

	bool isInBounds(const QRect rect)const;

	void startCorner(const QPoint& p);

	void endCorner(const QPoint& p);

	void scaleWindow(int direction);

	void moveBy(int dx, int dy);

	void updatePos();

private:
	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};






template <typename T>
class PreviewWindow: public QDialog {

	int m_bin_factor = 1;
	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_process_type = "";

	WindowSignals* m_ws = new WindowSignals;

	Image<T> m_source;

protected:
	QImage m_display;
	ImageLabel* m_image_label;

public:
	PreviewWindow(QWidget* parent = nullptr);

	~PreviewWindow() { m_ws->windowClosed(); }

	void closeEvent(QCloseEvent* close);

	bool eventFilter(QObject* o, QEvent* e);

	int binFactor()const { return m_bin_factor; }

	int rows()const { return m_drows; }

	int cols()const { return m_dcols; }

	int channels()const { return m_dchannels; }

	ImageType type()const { return m_source.type(); }

	ImageWindow<T>* parentWindow() { return reinterpret_cast<ImageWindow<T>*>(parentWidget()); }

	void setProcessType(QString process_type = "") { m_process_type = process_type; }

	QString processType()const { return m_process_type; }

	WindowSignals* windowSignals()const { return m_ws; }

	void updateSourcefromZoomWindow();

protected:
	void updateSource();

public:
	void updatePreview(bool from_parent = true);

	template<class P>
	void updatePreview(P& obj, void (P::* apply)(Image<T>&)) {

		updateSource();
		(obj.*apply)(m_source);

		if (parentWindow()->maskEnabled() && parentWindow()->maskExists())
			return updatePreview_Mask();

		displayImage();
	}

	/*template<class P>
	void updatePreview(P& obj, Image<T> (P::* apply)(const Image<T>&,int)) {

		m_source = (obj.*apply)(parentWindow()->source(), m_bin_factor);
		m_dchannels = m_source.channels();

		displayImage();
	}*/

	//bilateral filter & range_mask
	template<class P>
	void updatePreview(P& obj, void (P::* apply)(const Image<T>&, Image<T>&, int)) {

		(obj.*apply)(parentWindow()->source(), m_source, m_bin_factor);
		m_dchannels = m_source.channels();

		displayImage();
	}

private:
	void updatePreview_Mask();

	template<typename P>
	Image32 getMask(int factor) {

		int factor2 = factor * factor;

		auto mwp = parentWindow()->mask();
		Image32 mask(rows(), cols(), mwp->channels());

		const Image<P>* mask_source = reinterpret_cast<const Image<P>*>(&mwp->source());
		for (int ch = 0; ch < mask.channels(); ++ch) {
			for (int y = 0, y_s = 0; y < m_drows; ++y, y_s += factor) {
				for (int x = 0, x_s = 0; x < m_dcols; ++x, x_s += factor) {

					double pix = 0;

					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							pix += (*mask_source)(x_s + i, y_s + j, ch);

					mask(x, y, ch) = Pixel<float>::toType(P(pix / factor2));

				}
			}
		}

		return mask;
	}

	void ImagetoQImage_stf();

protected:
	void displayImage();
};

template<typename P = uint8_t>
static PreviewWindow<P>* previewRecast(QDialog* preview) { return static_cast<PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static PreviewWindow<P>* previewRecast(PreviewWindow<T>* preview) { return reinterpret_cast<PreviewWindow<P>*>(preview); }

typedef PreviewWindow<uint8_t> PreviewWindow8;
typedef PreviewWindow<uint16_t> PreviewWindow16;
typedef PreviewWindow<float> PreviewWindow32;
