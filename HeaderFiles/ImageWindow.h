#pragma once
//#include "ui_ImageWindow.h"
#include "Image.h"
#include "HistogramTransformation.h"
#include "CustomWidgets.h"
#include "Statistics.h"
#include "Interpolator.h"
#include "ImageSubWindow.h"
#include "Workspace.h"
#include <QOpenGLWidget>
#include <QOpenGLTexture>

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




class GLImageLabel : public QOpenGLWidget {
	Q_OBJECT

	QOpenGLTexture* m_texture = nullptr;
	const QImage* m_image = nullptr;
	const QImage* m_mask = nullptr;
	bool m_mask_visible = true;

	const QPixmap m_cross_cursor = CrossCursor().pixmap();

public:
	GLImageLabel(const QImage& img, QWidget* parent = nullptr) : m_image(&img), QOpenGLWidget(parent) {

		m_texture = new QOpenGLTexture(img);
		this->setCursor(CrossCursor());

		//this->setCursor(Qt::BlankCursor);
		//this->setAttribute(Qt::WA_Hover);
		//this->setMouseTracking(true);
	}

	void draw() { update(); }
private:

	void initializeGL() override;

	void paintGL() override;
};

class ImageLabel : public QLabel {
	Q_OBJECT

	const QImage* m_image = nullptr;
	const QImage* m_mask = nullptr;
	bool m_mask_visible = true;

	//CursorArea* m_ca;
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





class ImageInfoSignals : public QObject {
	Q_OBJECT

public:
	ImageInfoSignals(QObject* parent = nullptr) : QObject(parent) {}

signals:
	void imageInfo(const Image8* img);

	void pixelValue(const Image8* window, const QPointF& p);

	void previewPixelValue(const Image8* img, const QPointF& p, float factor, const QPointF offset = QPointF(0,0));
};







class ImageWindowToolbar : public QWidget {
	Q_OBJECT

private:
	const int m_width = 25;

	QButtonGroup* m_bg = nullptr;

	FlatPushButton* m_reset_pb = nullptr;
	FlatPushButton* m_hist_pb = nullptr;
	FlatPushButton* m_stats_pb = nullptr;
	FlatPushButton* m_img3D_pb = nullptr;
	FlatCheckablePushButton* m_stf_pb = nullptr;
	FlatCheckablePushButton* m_zoom_win_pb = nullptr;

	qreal m_opacity = 1.0;

public:
	ImageWindowToolbar(int height, QWidget* parent = nullptr);

	void setOpacity(qreal opacity) { 

		for (auto b : m_bg->buttons())
			dynamic_cast<PushButton*>(b)->setOpacity(opacity);
		update();
	}

	template <typename Func, typename BoolFunc>
	void connectToolbar(const QObject* receiver, Func&& reset, Func&& histogram, Func&& stats, Func&& _3dimg, BoolFunc&& stf, BoolFunc&& zoom_win) {

		connect(m_reset_pb, &QPushButton::released, receiver, reset);
		connect(m_hist_pb, &QPushButton::released, receiver, histogram);
		connect(m_stats_pb, &QPushButton::released, receiver, stats);
		connect(m_img3D_pb, &QPushButton::released, receiver, _3dimg);
		connect(m_stf_pb, &QPushButton::toggled, receiver, stf);
		connect(m_zoom_win_pb, &QPushButton::toggled, receiver, zoom_win);
	}

private:
	void resizeEvent(QResizeEvent* e);
};





class IWScrollArea : public QScrollArea {

	const int m_scrollbar_thickness = style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent);
public:
	IWScrollArea(QWidget* parent = nullptr) : QScrollArea(parent) {

		QPalette pal;
		pal.setBrush(QPalette::Window, QColor(128, 128, 128));
		this->setPalette(pal);
	}

	void setOpacity(qreal opacity) { 
		QPalette pal = palette();

		QColor c = pal.color(QPalette::Window);
		c.setAlpha(opacity * 255);
		pal.setBrush(QPalette::Window, c);

		c = pal.color(QPalette::Button);
		c.setAlpha(opacity * 255);
		pal.setBrush(QPalette::Button, c);

		this->setPalette(pal);
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

	void showVerticalScrollBar() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); }

	void hideVerticalScrollBar() { setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); }

	void showHorizontalScrollBar() { setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn); }

	void hideHorizontalScrollBar() { setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); }

private:
	void wheelEvent(QWheelEvent* e) { e->ignore(); }
};






class ImageWindowBase : public QWidget {
	Q_OBJECT
protected:
	Workspace* m_workspace = nullptr;
	ImageSubWindow* m_sw_parent = nullptr;

	QString m_name;

	QImage m_display;
	uint32_t m_drows = 0;
	uint32_t m_dcols = 0;
	uint32_t m_dchannels = 0;

	const int m_min_factor_poll = -10;
	const int m_max_factor_poll = 30;
	int m_factor_poll = -4;

	double m_factor = 1.0 / abs(m_factor_poll);//0.25;
	double m_old_factor = m_factor;

	int factorPoll()const { return m_factor_poll; }

	double oldFactor()const { return m_old_factor; }

	const QPixmap m_cros_cur = CrossCursor().pixmap();
public:
	ImageWindowBase(const QString& name, Workspace* parent);

	Workspace* workspace()const { return m_workspace; }

	ImageSubWindow* subwindow()const { return m_sw_parent; }

	double factor()const { return m_factor; }

	QString name()const { return m_name; }

	uint32_t rows()const { return m_drows; }

	uint32_t cols()const { return m_dcols; }

	uint32_t channels()const { return m_dchannels; }

signals:
	void windowCreated();

	void windowClosed();

	void windowUpdated();

	void zoomWindowCreated();

	void zoomWindowClosed();

	void imageInfo(const Image8* img);

	void pixelValue(const Image8* img, const QPointF& p);

	void previewPixelValue(const Image8* img, const QPointF& p, float factor, const QPointF offset = QPointF(0, 0));

protected:
	int computeBinFactor(const QSize& img_size, bool to_workspace = false)const;

private:
	void closeEvent(QCloseEvent* e);

	void dragEnterEvent(QDragEnterEvent* e);

	void dropEvent(QDropEvent* e);

protected:
	void wheelEvent(QWheelEvent* e);
};




class ZoomWindow;

template<typename T>
class PreviewWindow;

template<typename T>
class ImageWindow : public ImageWindowBase {

	ImageWindowToolbar* m_toolbar = nullptr;

	IWScrollArea* m_sa = nullptr;
	ScrollBar* m_sbX = nullptr;
	ScrollBar* m_sbY = nullptr;

	ImageLabel* m_image_label = nullptr;

	Image<T> m_source;

	const ImageWindow<uint8_t>* m_mask = nullptr;
	QImage m_mask_display;
	QColor m_mask_color = Qt::cyan;// { 127, 0, 255 };
	bool m_mask_enabled = true;
	bool m_invert_mask = false;
	bool m_show_mask = true;

	PointD m_offset;
	Point m_mousePos;

	std::unique_ptr<PreviewWindow<T>> m_preview;

	HistogramTransformation m_ht;
	bool m_compute_stf = true;
	bool m_enable_stf = false;

	std::unique_ptr<StatisticsDialog> m_stats_dialog;
	Statistics::StatsVector m_sv;
	Statistics::StatsVector m_sv_clipped;

	std::unique_ptr<HistogramDialog> m_hist_dialog;

	std::unique_ptr<Image3DDialog> m_img3D_dialog;

	const PencilCursor m_pencil_cursor;
	bool m_enable_zoom_win = false;
	bool m_draw_zoom_win = false;
	QColor m_zwc = QColor(69, 0, 128);
	std::unique_ptr<ZoomWindow> m_zoom_window;

public:
	ImageWindow() = default;

	ImageWindow(Image<T>&& img, QString name, Workspace* parent = nullptr);

	ImageType type()const { return m_source.type(); }

	PointD sourceOffset()const { return m_offset; }

	bool stfEnabled()const { return m_enable_stf; }

private:
	double clipOffsetX() {
		return m_offset.x = math::clip(m_offset.x, 0.0, math::max(0.0, m_source.cols() - cols() / factor()));
	}

	double clipOffsetY() {
		return m_offset.y = math::clip(m_offset.y, 0.0, math::max(0.0, m_source.rows() - rows() / factor()));
	}

public:
	const HistogramTransformation& histogramTransformation()const { return m_ht; }

	Image<T>& source() { return m_source; }

	const Image<T>& source()const { return m_source; }

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

	void convertToGrayscale();

private:
	void resetWindowSize();

	bool eventFilter(QObject* object, QEvent* event);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void mouseReleaseEvent(QMouseEvent* event);

	void resizeEvent(QResizeEvent* event);

	void wheelEvent(QWheelEvent* event)override;

	void instantiateScrollBars();

	void showHorizontalScrollBar();

	void showVerticalScrollBar();

	void showScrollBars();

public:
	template<class P>
	void applyToSource(P& obj, void (P::* apply)(Image<T>&)) {

		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_workspace->enableChildren(false);

		if (maskEnabled() && this->maskExists())
			this->applyWithMask<P>(obj, apply);
		else
			(obj.*apply)(m_source);

		QApplication::restoreOverrideCursor();
		m_workspace->enableChildren(true);
		m_compute_stf = true;

		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();

		emit windowUpdated();
	}

	/*template<class P, class... Args >
	void applyToSource(P& obj, void (P::* apply)(Image<T>&, Args&&...), Args&&... args) {

		QApplication::setOverrideCursor(Qt::WaitCursor);

		//if (maskEnabled() && this->maskExists())
			//this->applyWithMask<P>(obj, apply);
		//else

		std::cout << "QQQ\n";
		(obj.*apply)(m_source, args...);

		//std::thread t(apply, &obj, std::ref(m_source));
		//t.join();

		QApplication::restoreOverrideCursor();

		m_compute_stf = true;

		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();

		emit m_ws->windowUpdated();
	}*/

	template<class P>
	void applyToSource_Geometry(P& obj, void (P::* apply)(Image<T>&)) {

		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_workspace->enableChildren(false);

		removeMask();

		(obj.*apply)(m_source);

		QApplication::restoreOverrideCursor();
		m_workspace->enableChildren(true);
		m_compute_stf = true;

		m_zoom_window.reset();
		enableZoomWindowMode(m_enable_zoom_win);

		zoom(0, 0);
		updateStatisticsDialog();
		updateHistogramDialog();

		emit imageInfo(reinterpret_cast<Image8*>(&m_source));
		emit windowUpdated();
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
	void applyWithMask(P& obj, void (P::* apply)(Image<T>&)) {

		Image<T> mod = Image<T>(m_source);
		(obj.*apply)(mod);

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

	void resizeImageLabel();

	void displayImage();

	void displayMask();


	void zoom(int x, int y);

	void pan(int x, int y);


	void binToWindow(int factor);

	void binToWindow_RGB(int factor);

	void binToWindow_Colorspace(int factor);

	template<typename P>
	void binToWindow_Mask(const Image<P>& mask_src, int factor) {

		int factor2 = factor * factor;
		const uint8_t b = m_mask_color.blue();
		const uint8_t g = m_mask_color.green();
		const uint8_t r = m_mask_color.red();

		for (int y = 0, y_s = m_offset.y; y < rows(); ++y, y_s += factor) {
			for (int x = 0, x_s = m_offset.x; x < cols(); ++x, x_s += factor) {

				uint8_t max_pixel = 0;

				for (int ch = 0; ch < mask_src.channels(); ++ch) {
					float pixel = 0;
					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							pixel += mask_src(x_s + i, y_s + j, ch);
					max_pixel = math::max(Pixel<uint8_t>::toType(T(pixel / factor2)), max_pixel);
				}

				max_pixel = (m_invert_mask) ? max_pixel : 255 - max_pixel;

				float n = Pixel<float>::toType(max_pixel);

				uint8_t* p = &m_mask_display.scanLine(y)[4 * x];
				*p = n * b;
				*(p + 1) = n * g;
				*(p + 2) = n * r;
				*(p + 3) = max_pixel;
			}
		}
	}

	void upsampleToWindow(int factor);

	void upsampleToWindow_RGB(int factor);

	template<typename P>
	void upsampleToWindow_Mask(const Image<P>& mask_src, int factor) {

		int factor2 = factor * factor;
		double dd = 1.0 / factor;

		const uint8_t b = m_mask_color.blue();
		const uint8_t g = m_mask_color.green();
		const uint8_t r = m_mask_color.red();

		double y_s = m_offset.y;
		for (int y = 0; y < rows(); ++y, y_s += dd) {
			double x_s = m_offset.x;
			for (int x = 0; x < cols(); ++x, x_s += dd) {

				uint8_t max_pixel = 0;
				for (int ch = 0; ch < mask_src.channels(); ++ch)
					max_pixel = math::max(Pixel<uint8_t>::toType(mask_src(x_s, y_s, ch)), max_pixel);

				max_pixel = (m_invert_mask) ? max_pixel : 255 - max_pixel;

				float n = Pixel<float>::toType(max_pixel);

				uint8_t* p = &m_mask_display.scanLine(y)[4 * x];
				*p = n * b;
				*(p + 1) = n * g;
				*(p + 2) = n * r;
				*(p + 3) = max_pixel;
			}
		}
		
	}

	void openStatisticsDialog();

	void updateStatisticsDialog();

	void openHistogramDialog();

	void updateHistogramDialog();

	void openImage3DDialog();

	void enableSTF(bool enable);

	void enableZoomWindowMode(bool enable);

	void onZoomWindowClose();
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
	QColor m_pen_color = QColor(69, 0, 128);

	QRectF m_img_rect;
	bool m_resizing = false;
	bool m_moving = false;
	Frame m_frame;
	QRect m_start_rect;

	QPoint m_pos;
	RectBorder m_current_border = RectBorder::None;
	QMetaObject::Connection m_connection;

public:
	template<typename T>
	ZoomWindow(const QPoint& start_pos, const QColor& pen_color, const ImageWindow<T>& iw, QWidget* parent = nullptr) :m_pos(start_pos), m_pen_color(pen_color), m_iw(imageRecast<uint8_t>(&iw)), QWidget(parent) {
		
		m_connection = connect(this, &ZoomWindow::windowMoved, this, &ZoomWindow::defaultMethod);
		this->setMouseTracking(true);
		this->setGeometry(QRect(m_pos, QSize(0,0)));
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
	void windowAction();

	void windowMoved();

	void windowClosed();
public:
	template<typename Func>
	void connectZoomWindow(const QObject* receiver, Func&& func) {
		disconnect(m_connection);
		m_connection = connect(this, &ZoomWindow::windowMoved, receiver, func);
	}

	void disconnectZoomWindow() {
		disconnect(m_connection);
		m_connection = connect(this, &ZoomWindow::windowMoved, this, &ZoomWindow::defaultMethod);
	}

private:
	void computeImageRect();

	void defaultMethod();

	void closeEvent(QCloseEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);

	void resizeEvent(QResizeEvent* e);

	void wheelEvent(QWheelEvent* e) { e->ignore(); }
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





class PreviewWindowBase : public QWidget {
	Q_OBJECT

private:
	QSize m_pre_shade_size;
	QSize m_min_size;

	DialogTitleBar* m_titlebar = nullptr;
	QWidget* m_draw_area = nullptr;
	const int m_border_width = 3;
	QTimer* m_timer = nullptr;
	bool m_moving = false;
	QPoint m_start_pos;

	float m_opacity = 1.0;

protected:
	double m_scale_factor = 1.0f;
	QPointF m_zwtl = { 0,0 };

	uint32_t m_drows = 0;
	uint32_t m_dcols = 0;
	uint32_t m_dchannels = 0;

	PreviewImageLabel* m_image_label = nullptr;
	QImage m_display;

	QString m_process_type = "";

	bool m_ingore_zoom_window = false;

public:
	PreviewWindowBase(Workspace* workspace = nullptr, bool ignore_zoom_window = false);

	uint32_t rows()const { return m_drows; }

	uint32_t cols()const { return m_dcols; }

	uint32_t channels()const { return m_dchannels; }

	QString title()const { return m_titlebar->title(); }

	void setTitle(const QString& txt) { m_titlebar->setTitle(txt); }

	void setProcessType(QString process_type = "") { m_process_type = process_type; }

	const QString& processType()const { return m_process_type; }

	bool isZoomWidnowIgnored()const { return m_ingore_zoom_window; }

	double scaleFactor()const { return m_scale_factor; }

signals:
	void windowCreated();

	void windowClosed();

	void pixelValue(const Image8* img, const QPoint& preview_pos, const QPointF& img_pos);

protected:
	QWidget* drawArea()const { return m_draw_area; }

	DialogTitleBar* titlebar()const { return m_titlebar; }

	void resizeWindow(int w, int h);

	double computeScaleFactor(const QSize& n_size)const;

	void setOpacity(float opacity) {
		m_opacity = opacity;
		m_titlebar->setOpacity(opacity);
		this->m_image_label->setOpacity(opacity);
		update();
	}

private:
	void closeEvent(QCloseEvent* close);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);

	void resizeEvent(QResizeEvent* e);
};


template<typename T>
class PreviewWindow : public PreviewWindowBase {

	const ImageWindow<T>* m_image_window = nullptr;
	Image<T> m_source = Image<T>(1, 1, 1);

public:
	PreviewWindow(ImageWindow<T>* iw, bool ignore_zoomwindow = false);

	const ImageWindow<T>* imageWindow()const { return m_image_window; }

	ImageType type()const { return m_source.type(); }

protected:
	void resizeSource();

private:
	void updateSource();

	template<typename P>
	Image32 getMask() {

		auto mwp = m_image_window->mask();
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

	void updatePreview_Mask();

public:
	void updatePreview(bool from_parent = true);

	template<class P>
	void updatePreview(P& obj, void (P::* apply)(Image<T>&)) {

		updateSource();
		(obj.*apply)(m_source);

		if (m_image_window->maskEnabled() && m_image_window->maskExists())
			return updatePreview_Mask();

		displayImage();
	}

	//bilateral filter & abe
	template<class P>
	void updatePreview(P& obj, void (P::* apply)(const Image<T>&, Image<T>&, float, const QPointF&)) {

		updateSource();

		(obj.*apply)(m_image_window->source(), m_source, scaleFactor(), m_zwtl);
		m_dchannels = m_source.channels();

		displayImage();
	}

private:
	void displayImage();

	bool eventFilter(QObject* obj, QEvent* e);
};

template<typename P = uint8_t>
static PreviewWindow<P>* previewRecast(PreviewWindowBase* preview) { return static_cast<PreviewWindow<P>*>(preview); }

template<typename P = uint8_t>
static const PreviewWindow<P>* previewRecast(const PreviewWindowBase* preview) { return static_cast<const PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static PreviewWindow<P>* previewRecast(PreviewWindow<T>* preview) { return reinterpret_cast<PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static const PreviewWindow<P>* previewRecast(const PreviewWindow<T>* preview) { return reinterpret_cast<const PreviewWindow<P>*>(preview); }

typedef PreviewWindow<uint8_t> PreviewWindow8;
typedef PreviewWindow<uint16_t> PreviewWindow16;
typedef PreviewWindow<float> PreviewWindow32;

