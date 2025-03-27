#pragma once
//#include "ui_ImageWindow.h"
#include "Image.h"
#include "HistogramTransformation.h"
#include "CustomWidgets.h"
#include "Statistics.h"
#include "Interpolator.h"
//#include "ProcessDialog.h"

class CrossCursor : public QCursor {

	static QPixmap crossCursor(const QSize& size = { 32,32 }) {

		QPixmap pm(size);
		pm.fill(Qt::transparent);

		QPainter p(&pm);
		QPen pen({ 150,69,255 });
		pen.setWidth(2);
		p.setPen(pen);

		int ym = size.height() / 2;
		int xm = size.width() / 2;

		p.drawLine(0, ym, size.width(), ym);
		p.drawLine(xm, 0, xm, size.height());

		pen.setWidth(1);
		p.setPen(pen);
		p.drawRect(3 * xm / 4, 3 * ym / 4, (xm - 1) / 2, (ym - 1) / 2);

		return pm;
	}

public:
	CrossCursor() : QCursor(crossCursor()) {}
};





class PanCursor : public QCursor {

	static QPixmap panCursor(const QSize& size = { 24,24 }) {

		QPixmap pix("./Icons//sizeall-cursor.png");
		return pix.scaled(size,Qt::KeepAspectRatio,Qt::TransformationMode::SmoothTransformation);
	}

public:
	PanCursor() : QCursor(panCursor()) {}
};





class PencilCursor : public QCursor {

	static QPixmap pencilCursor(const QSize& size = { 24,24 }) {

		QPixmap pix("./Icons//pencil-cursor.png");
		return pix.scaled(size, Qt::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
	}

public:
	PencilCursor() : QCursor(pencilCursor(),0, 24) {}
};





class ImageLabel : public QLabel {
	Q_OBJECT

	const QImage* m_image = nullptr;
	const QImage* m_mask = nullptr;
	bool m_mask_visible = true;

	const CrossCursor m_cross_cursor;
	const PanCursor m_pan_cursor;
	qreal m_opacity = 1.0;

public:
	ImageLabel(const QImage& img, QWidget* parent);

	void setMaskVisible(bool show = true) { m_mask_visible = show; }

	void setMask(const QImage* mask) { m_mask = mask; }

	void removeImage() { m_image = nullptr; }

	void removeMask() { m_mask = nullptr; }

	void draw() { update(); }

	void resetCursor() { this->setCursor(m_cross_cursor); }

	void setOpacity(qreal opacity) { m_opacity = opacity; }

signals:
	void mousePos(const QPoint& p);

	void mouseLeave();

private:

	void paintEvent(QPaintEvent* event);

	bool eventFilter(QObject* object, QEvent* event);
};





class ImageInfoSignals : public QWidget {
	Q_OBJECT

public:
	ImageInfoSignals() {}

	~ImageInfoSignals() {}

signals:
	void imageInfo(QMdiSubWindow* window);

	void pixelValue(const Image8* window, const QPointF& p);

	void previewPixelValue(const Image8* img, const QPointF& p, float factor, const QPointF offset = QPointF(0,0));
};






class ImageWindowToolbar : public QWidget {
	Q_OBJECT

private:
	const int m_width = 25;

	QButtonGroup* m_bg = nullptr;

	PushButton* m_img3D_pb = nullptr;
	PushButton* m_stats_pb = nullptr;
	PushButton* m_hist_pb = nullptr;
	PushButton* m_reset_pb = nullptr;
	CheckablePushButton* m_stf_pb = nullptr;
	CheckablePushButton* m_zoom_win_pb = nullptr;

public:
	ImageWindowToolbar(int height, QWidget* parent = nullptr);

	bool isSTFChecked()const { return m_stf_pb->isChecked(); }

	bool isZoomChecked()const { return m_zoom_win_pb->isChecked(); }

signals:
	void zoomWindowClicked(bool checked);

	void stfClicked(bool checked);

	void image3DReleased();

	void statisticsReleased();

	void histogramReleased();

	void resetReleased();

private:
	void resizeEvent(QResizeEvent* e);
};





class IWScrollArea : public QScrollArea {

	const int m_scrollbar_thickness = style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent);

	QPalette m_pal;
public:
	IWScrollArea(QWidget* parent = nullptr) : QScrollArea(parent) {

		m_pal.setBrush(QPalette::Window, QColor(128, 128, 128));
		this->setPalette(m_pal);
		//this->setBackgroundRole(QPalette::ColorRole::Dark);
		
		//QString corner = "QAbstractScrollArea::corner{background-color: dark gray; }";
		//this->setStyleSheet(corner);
	}

	void setOpaquePal() { 
		m_pal.setBrush(QPalette::Window, QColor(128, 128, 128, 140));
		m_pal.setBrush(QPalette::Button, QColor(128, 128, 128, 140));

		this->setPalette(m_pal);
	}

	void unsetOpaquePal() { 
		m_pal.setBrush(QPalette::Window, QColor(128, 128, 128));
		this->setPalette(m_pal);
	}

	int scrollbarThickness()const { return m_scrollbar_thickness; }

	QSize viewportSize()const {

		int dr = (isHorizontalScrollBarOn()) ? horizontalScrollBar()->height() : 0;
		int dc = (isVerticalScrollBarOn()) ? verticalScrollBar()->width() : 0;

		return QSize(width() - dc, height() - dr);
	}

	bool isHorizontalScrollBarOn()const { return (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn); }

	bool isVerticalScrollBarOn()const { return (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn); }

	bool isHorizontalScrollBarOff()const { return (horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOff); }

	bool isVerticalScrollBarOff()const { return (verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOff); }

private:
	void wheelEvent(QWheelEvent* e) { e->ignore(); }
};





class SubWindow : public QMdiSubWindow {


	QPalette m_pal;
	QPalette m_opaque_pal;
	QTimer* m_timer = nullptr;

public:
	SubWindow(QWidget* widget);
	
private:
	void makeOpaque();

	//void mousePressEvent(QMouseEvent* e);

	//void mouseMoveEvent(QMouseEvent* e);

	//void mouseReleaseEvent(QMouseEvent* e);
};





class ZoomWindow;

template<typename T> 
class PreviewWindow;

template<typename T>
class ImageWindow : public QWidget {

	QMdiArea* m_workspace = nullptr;
	SubWindow* m_sw_parent = nullptr;
	ImageWindowToolbar* m_toolbar = nullptr;
	//QTimer* m_timer;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QSize m_default_size;

	QString m_name;
	Image<T> m_source;

	ImageLabel* m_image_label = nullptr;
	QImage m_display;
	QImage m_mask_display;

	std::unique_ptr<PreviewWindow<T>> m_preview;

	HistogramTransformation m_ht;
	bool m_compute_stf = true;

	const ImageWindow<uint8_t>* m_mask = nullptr;
	QColor m_mask_color = Qt::cyan;// { 127, 0, 255 };
	bool m_mask_enabled = true;
	bool m_invert_mask = false;
	bool m_show_mask = true;

	IWScrollArea* m_sa = nullptr;
	ScrollBar* m_sbX = nullptr;
	ScrollBar* m_sbY = nullptr;

	const int m_min_factor_poll = -10;
	const int m_max_factor_poll = 30;
	int m_factor_poll = -4;

	double m_factor = 1.0 / abs(m_factor_poll);//0.25;
	double m_old_factor = m_factor;
	double m_initial_factor = m_factor;

	double m_sourceOffX = 0;
	double m_sourceOffY = 0;

	int m_mouseX = 0;
	int m_mouseY = 0;

	const int m_titlebar_height = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
	const int m_border_width = QApplication::style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);

	const std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();
	const std::unique_ptr<ImageInfoSignals> m_iis = std::make_unique<ImageInfoSignals>();

	std::unique_ptr<StatisticsDialog> m_stats_dialog;
	Statistics::StatsVector m_sv;
	Statistics::StatsVector m_sv_clipped;

	std::unique_ptr<HistogramDialog> m_hist_dialog;

	std::unique_ptr<Image3DDialog> m_img3D_dialog;
	std::unique_ptr<QWidget> m_swp;


	const PencilCursor m_pencil_cursor;
	bool m_draw_zoom_win = false;
	QColor m_zwc = QColor(69, 0, 128);
	std::unique_ptr<ZoomWindow> m_zoom_window;

	QMetaObject::Connection m_connection;

public:
	WindowSignals* windowSignals()const { return m_ws.get(); }

	ImageInfoSignals* imageInfoSignals()const { return m_iis.get(); }

	ImageWindow() = default;

	ImageWindow(Image<T>&& img, QString name, QWidget* parent = nullptr);

	int computeBinFactor(bool workspace = false)const;

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

	QMdiArea* workspace()const { return m_workspace; }

	QMdiSubWindow* subwindow()const { return m_sw_parent; }

	ZoomWindow* zoomWindow()const { return m_zoom_window.get(); }

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

	QColor zoomWindowColor()const { return m_zwc; }

	void setZoomWindowColor(const QColor& color);

	bool previewExists()const { return (m_preview.get() != nullptr); }

	PreviewWindow<T>* preview()const { return m_preview.get(); }

	void showPreview(PreviewWindow<T>* preview = nullptr);

	void closePreview();

	void convertToGrayscale();

	void setOpaque();

	void unsetOpaque();

private:
	void resetWindowSize();

	void mouseMove_ImageLabel(const QPoint& p)const;

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
	void applyToSource(P& obj, void (P::* apply)(Image<T>&)) {

		if (maskEnabled() && this->maskExists())
			this->applyWithMask<P>(obj, apply);
		else
			(obj.*apply)(m_source);
		//std::thread t(apply, &obj, std::ref(m_source));
		//t.join();

		m_compute_stf = true;

		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();

		emit m_ws->windowUpdated();
	}

	template<class P>
	void applyToSource_Geometry(P& t, void (P::* apply)(Image<T>&)) {

		removeMask();

		(t.*apply)(m_source);

		int factor = computeBinFactor(true);
		m_factor_poll = (factor == 1) ? factor : -factor;
		m_initial_factor = m_old_factor = m_factor = 1.0 / abs(m_factor_poll);

		QString str = QString::number(abs(m_factor_poll));
		if (m_factor > 1)
			this->setWindowTitle(str + ":1" + " " + m_name);
		else
			this->setWindowTitle("1:" + str + " " + m_name);

		m_zoom_window.reset();
		enableZoomWindowMode();
		zoom(0, 0);
		updateStatisticsDialog();
		updateHistogramDialog();

		emit m_iis->imageInfo(m_sw_parent);
		emit m_ws->windowUpdated();
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
	}

	void instantiateScrollBars();

	void showHorizontalScrollBar();

	void hideHorizontalScrollBar();

	void showVerticalScrollBar();

	void hideVerticalScrollBar();

	void showScrollBars();


	void resizeImageLabel();

	void displayImage();

	void displayMask();


	void zoom(int x, int y);

	void pan(int x, int y);


	void binToWindow(int factor);

	void binToWindow_stf(int factor);

	void binToWindow_RGB(int factor);

	void binToWindow_Colorspace(int factor);

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

	void showSTFImage();

	void enableZoomWindowMode();

	void onZoomWindowClose();

public:
	template<typename Func>
	void connectPreview(const QObject* receiver, Func func) {

		disconnect(m_connection);
		m_connection = connect(m_ws.get(), &WindowSignals::windowUpdated, receiver, func);
	}
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
	Q_OBJECT

	const ImageWindow8* m_iw = nullptr;
	const int m_pen_width = 2;
	QColor m_pen_color = QColor(75, 0, 130);

	QRectF m_img_rect;

	QPoint m_pos;
	QMetaObject::Connection m_connection;

	int penWidth()const { return m_pen_width; }

public:
	template<typename T>
	ZoomWindow(const QPoint& start_pos, const QColor& pen_color, const ImageWindow<T>& iw, QWidget* parent = nullptr) :m_pos(start_pos), m_pen_color(pen_color), m_iw(imageRecast<uint8_t>(&iw)), QWidget(parent) {
		
		this->setGeometry(QRect(m_pos, QSize(0, 0)));
		update();
		
		this->setAttribute(Qt::WA_DeleteOnClose);
		this->show();
	}

	QColor penColor()const { return m_pen_color; }

	void setPenColor(const QColor& color) { 
		m_pen_color = color; 
		update();
	}

	QRect imageRect()const { return m_img_rect.toRect(); }

	QRectF imageRectF()const { return m_img_rect; }

	void keepFrameInImage_draw(QRect& rect)const;

	void keepFrameInImage_move(QRect& rect)const;

	void endCorner(const QPoint& p);

	void scaleWindow();

	void moveBy(int dx, int dy);

signals:
	void windowMoved();

	void windowClosed();

public:
	template<typename Func>
	void connectZoomWindow(const QObject* receiver, Func func) {

		disconnect(m_connection);
		m_connection = connect(this, &ZoomWindow::windowMoved, receiver, func);
	}

private:
	void closeEvent(QCloseEvent* e);

	void wheelEvent(QWheelEvent* e) { e->ignore(); }

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};





class PreviewImageLabel : public QLabel {
	Q_OBJECT

	const QImage* m_image = nullptr;
	const CrossCursor m_cross_cursor;

	double m_opacity = 1.0;
	
public:
	PreviewImageLabel(const QImage& img, QWidget* parent);

	void removeImage() { m_image = nullptr; }

	void draw() { update(); }

	void setOpacity(double opacity) { m_opacity = opacity; }

signals:
	void mousePos(const QPoint& p);

	void mouseLeave();
private:
	void paintEvent(QPaintEvent* event);
};






template <typename T>
class PreviewWindow: public Dialog {

	QPalette m_pal;
	QPalette m_opaque_pal;

	const ImageWindow<T>* m_image_window = nullptr;

protected:
	PreviewImageLabel* m_image_label = nullptr;

private:
	double m_scale_factor = 1.0f;

	uint32_t m_drows = 0;
	uint32_t m_dcols = 0;
	uint32_t m_dchannels = 0;

	Image<T> m_source;
	QImage m_display;

	QString m_process_type = "";

	bool m_ingore_zoom_window = false;

	const std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();
	const std::unique_ptr<ImageInfoSignals> m_iis = std::make_unique<ImageInfoSignals>();

	QPointF m_zwtl = {0,0};

	QMdiSubWindow* m_sw;

public:
	PreviewWindow(QWidget* image_window, bool ignore_zoom_window = false);

	uint32_t rows()const { return m_drows; }

	uint32_t cols()const { return m_dcols; }

	uint32_t channels()const { return m_dchannels; }

	ImageType type()const { return m_source.type(); }

	const ImageWindow<T>* imageWindow()const { return m_image_window; }

	void setProcessType(QString process_type = "") { m_process_type = process_type; }

	const QString& processType()const { return m_process_type; }

	WindowSignals* windowSignals()const { return m_ws.get(); }

	bool isZoomWidnowIgnored()const { return m_ingore_zoom_window; }

	double scaleFactor()const { return m_scale_factor; }

	double computeScaleFactor(const QSize& n_size, const QSize& draw_area_size)const;

private:
	void makeOpaque();

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void closeEvent(QCloseEvent* close);

	bool eventFilter(QObject* obj, QEvent* e);

	void resizeSource();

	void updateSource();

public:
	void updatePreview(bool from_parent = true);

	template<class P>
	void updatePreview(P& obj, void (P::* apply)(Image<T>&)) {

		updateSource();
		(obj.*apply)(m_source);

		if (imageWindow()->maskEnabled() && imageWindow()->maskExists())
			return updatePreview_Mask();

		displayImage();
	}

	//bilateral filter & abe
	template<class P>
	void updatePreview(P& obj, void (P::* apply)(const Image<T>&, Image<T>&, float, const QPointF&)) {

		updateSource();

		(obj.*apply)(imageWindow()->source(), m_source, scaleFactor(), m_zwtl);
		m_dchannels = m_source.channels();

		displayImage();
	}

private:
	void updatePreview_Mask();

	template<typename P>
	Image32 getMask() {

		auto mwp = imageWindow()->mask();
		Image32 mask(rows(), cols(), mwp->channels());

		const Image<P>* mask_source = reinterpret_cast<const Image<P>*>(&mwp->source());

		float _s = 1 / scaleFactor();

		if (_s >= 1.0f) {
			int f = _s;
			int f2 = f * f;

			for (int ch = 0; ch < channels(); ++ch) {
				for (int y = 0; y < rows(); ++y) {
					float y_s = y * _s + m_zwtl.y();

					for (int x = 0; x < cols(); ++x) {
						float x_s = x * _s + m_zwtl.x();

						double pix = 0;

						for (int j = 0; j < f; ++j)
							for (int i = 0; i < f; ++i)
								pix += (*mask_source)(x_s + i, y_s + j, ch);

						mask(x, y, ch) = Pixel<float>::toType(P(pix / f2));
					}
				}
			}
		}

		else {
			for (int ch = 0; ch < channels(); ++ch) {
				for (int y = 0; y < rows(); ++y) {
					int y_s = y * _s + m_zwtl.y();

					for (int x = 0; x < cols(); ++x) {
						int x_s = x * _s + m_zwtl.x();

						mask(x, y, ch) = Pixel<float>::toType((*mask_source)(x_s, y_s, ch));
					}
				}
			}
		}

		return std::move(mask);
	}

protected:
	void displayImage();
};

template<typename P = uint8_t>
static PreviewWindow<P>* previewRecast(QDialog* preview) { return static_cast<PreviewWindow<P>*>(preview); }

template<typename P = uint8_t>
static const PreviewWindow<P>* previewRecast(const QDialog* preview) { return static_cast<const PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static PreviewWindow<P>* previewRecast(PreviewWindow<T>* preview) { return reinterpret_cast<PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static const PreviewWindow<P>* previewRecast(const PreviewWindow<T>* preview) { return reinterpret_cast<const PreviewWindow<P>*>(preview); }

typedef PreviewWindow<uint8_t> PreviewWindow8;
typedef PreviewWindow<uint16_t> PreviewWindow16;
typedef PreviewWindow<float> PreviewWindow32;

