#pragma once
//#include "ui_ImageWindow.h"
#include "Image.h"
#include "HistogramTransformation.h"
#include "AutomaticBackgroundExtraction.h"
#include "BilateralFilter.h"
#include "Maths.h"
#include "CustomWidgets.h"

#include "Statistics.h"

#include "SubWindow.h"
#include "Workspace.h"


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

	double m_opacity = 1.0;
	const CrossCursor m_cross_cursor;

	const QImage* m_image = nullptr;

public:
	ImageLabel(const QImage& img, QWidget* parent = nullptr);

	void draw() { update(); }

	double opacity()const { return m_opacity; }

	void setOpacity(double opacity) { m_opacity = opacity; }

	void setCrossCursor() { this->setCursor(m_cross_cursor); }

protected:
	virtual void paintEvent(QPaintEvent* e) override;
};




class ImageWindowBase;
class ImageWindowLabel : public ImageLabel {

	const ImageWindowBase* m_iw = nullptr;
	const QImage* m_mask = nullptr;
	bool m_mask_visible = true;
	const PanCursor m_pan_cursor;
public:
	ImageWindowLabel(const QImage& img, const ImageWindowBase* iw, QWidget* parent);

	void setMaskVisible(bool show = true) { m_mask_visible = show; }

	void setMask(const QImage* mask) { m_mask = mask; }

	void removeMask() { m_mask = nullptr; }

private:
	void paintEvent(QPaintEvent* e) override;

	bool eventFilter(QObject* object, QEvent* event);
};





class ImageWindowToolbar : QWidget {
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
	FlatPushButton* m_psf_pb = nullptr;

	qreal m_opacity = 1.0;

	using IWB = ImageWindowBase;

public:
	using QWidget::width;

	using QWidget::height;

	ImageWindowToolbar(QWidget* parent = nullptr);

	void setOpacity(qreal opacity) { 

		for (auto b : m_bg->buttons())
			dynamic_cast<PushButton*>(b)->setOpacity(opacity);
		update();
	}

	void connectToolbar(const QObject* receiver, void (IWB::*reset)(), void (IWB::*histogram)(), 
		void (IWB::* stats)(), void (IWB::* _3dimg)(), void (IWB::* stf)(bool), 
		void (IWB::*zoom_win)(bool), void (IWB::*psf)());

	void setHeight(int h) { resize(width(), h); }

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




class ZoomWindow;

class ImageWindowBase : public QWidget {
	Q_OBJECT
protected:
	Workspace* m_workspace = nullptr;
	SubWindow* m_sw_parent = nullptr;

	ImageWindowToolbar* m_toolbar = nullptr;
	IWScrollArea* m_sa = nullptr;
	ScrollBar* m_sbX = nullptr;
	ScrollBar* m_sbY = nullptr;
	ImageWindowLabel* m_image_label = nullptr;

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

	PointD m_offset;
	Point m_mousePos;

	HistogramTransformation m_ht;
	bool m_compute_stf = true;
	bool m_enable_stf = false;

	std::unique_ptr<HistogramDialog> m_hist_dialog;

	std::unique_ptr<StatisticsDialog> m_stats_dialog;
	Statistics::StatsVector m_sv;
	Statistics::StatsVector m_sv_clipped;

	std::unique_ptr<Image3DDialog> m_img3D_dialog;

	const PencilCursor m_pencil_cursor;
	bool m_enable_zoom_win = false;
	bool m_draw_zoom_win = false;
	QColor m_zwc = { 255,165,0 };
	std::unique_ptr<ZoomWindow> m_zoom_window;

	std::unique_ptr<PSFUtilityDialog> m_psf_dialog;
	const PSFVector* m_psf_vector = nullptr;
	const PSF* m_selected_psf = nullptr;

	int factorPoll()const { return m_factor_poll; }

	double oldFactor()const { return m_old_factor; }

	const QPixmap m_cros_cur = CrossCursor().pixmap();

	ImageWindowBase(const QString& name, Workspace* parent);

public:
	Workspace* workspace()const { return m_workspace; }

	SubWindow* subwindow()const { return m_sw_parent; }

	const HistogramTransformation& histogramTransformation()const { return m_ht; }

	ZoomWindow* zoomWindow()const { return m_zoom_window.get(); }

	double factor()const { return m_factor; }

	QString name()const { return m_name; }

	uint32_t rows()const { return m_drows; }

	uint32_t cols()const { return m_dcols; }

	uint32_t channels()const { return m_dchannels; }

	PointD sourceOffset()const { return m_offset; }

	bool stfEnabled()const { return m_enable_stf; }

	QColor zoomWindowColor()const { return m_zwc; }

	void setZoomWindowColor(const QColor& color);

	const PSFVector* psfVector()const { return m_psf_vector; }

	const PSF* selectedPSF()const { return m_selected_psf; };

signals:
	void windowCreated();

	void windowClosed();

	void windowUpdated();

	void zoomWindowCreated();

	void zoomWindowClosed();

	void imageInfo(const Image8* img);

	void pixelValue(const Image8* img, const QPointF& p);

	void previewPixelValue(const Image8* img, const QPointF& p, float factor, const QPointF offset = QPointF(0, 0));

	void psfSelected(const PSF* psf);
protected:
	int computeBinFactor(const QSize& img_size, bool to_workspace = false)const;

private:
	void closeEvent(QCloseEvent* e);

	void dragEnterEvent(QDragEnterEvent* e);

	void dropEvent(QDropEvent* e);

protected:
	virtual void resiseEvent(QResizeEvent* e);

	virtual void wheelEvent(QWheelEvent* e);

	virtual void resetWindowSize() = 0;

	virtual void openHistogramDialog() = 0;

	virtual void openStatisticsDialog() = 0;

	virtual void openImage3DDialog() = 0;

	virtual void enableSTF(bool enable) = 0;

	void enableZoomWindowMode(bool enable);

	void onZoomWindowClose();

	virtual void openPSFDialog() = 0;
};





template<typename T>
class PreviewWindow;

template<typename T>
class ImageWindow : public ImageWindowBase {

	Image<T> m_source;

	const ImageWindow<uint8_t>* m_mask = nullptr;
	QImage m_mask_display;
	QColor m_mask_color = Qt::cyan;// { 127, 0, 255 };
	bool m_mask_enabled = true;
	bool m_invert_mask = false;
	bool m_show_mask = true;

	std::unique_ptr<PreviewWindow<T>> m_preview;
	QMetaObject::Connection m_zoomwindow_connection;

public:
	ImageWindow() = default;

	ImageWindow(Image<T>&& img, QString name, Workspace* parent = nullptr);

	ImageType type()const { return m_source.type(); }

private:
	double clipOffsetX() {
		return m_offset.x = math::clip(m_offset.x, 0.0, math::max(0.0, m_source.cols() - cols() / factor()));
	}

	double clipOffsetY() {
		return m_offset.y = math::clip(m_offset.y, 0.0, math::max(0.0, m_source.rows() - rows() / factor()));
	}

public:
	Image<T>& source() { return m_source; }

	const Image<T>& source()const { return m_source; }

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

	bool previewExists()const { return (m_preview.get() != nullptr); }

	PreviewWindow<T>* preview()const { return m_preview.get(); }

	void showPreview(PreviewWindow<T>* preview = nullptr);

	void convertToGrayscale();

private:
	bool eventFilter(QObject* object, QEvent* event);

	void mousePressEvent(QMouseEvent* event);

	void mouseMoveEvent(QMouseEvent* event);

	void mouseReleaseEvent(QMouseEvent* event);

	void resizeEvent(QResizeEvent* event)override;

	void wheelEvent(QWheelEvent* event)override;

	void showScrollBars();

	template<typename P>
	void applyMask(const Image<T>& modified) {

		const Image<P>* mask = reinterpret_cast<const Image<P>*>(&m_mask->source());

		for (int ch = 0; ch < source().channels(); ++ch) {
			int mask_ch = (ch < mask->channels()) ? ch : 0;
			for (int y = 0; y < source().rows(); ++y) {
				for (int x = 0; x < source().cols(); ++x) {
					float m = Pixel<float>::toType((*mask)(x, y, mask_ch));
					m = (maskInverted()) ? 1.0f - m : m;
					m_source(x, y, ch) = m_source(x, y, ch) * (1 - m) + modified(x, y, ch) * m;
				}
			}
		}
	}

public:
	template<class P>
	void applyToSource(P& obj, void (P::* apply)(Image<T>&)) {

		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_workspace->enableChildren(false);

		auto mask_apply = [&, this]() {
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
		};

		QEventThreads::runThread([&, this]() { (maskEnabled() && this->maskExists()) ? mask_apply() : (obj.*apply)(m_source); });

		//(maskEnabled() && this->maskExists()) ? mask_apply() : (obj.*apply)(m_source);

		QApplication::restoreOverrideCursor();
		m_workspace->enableChildren(true);
		m_compute_stf = true;

		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();

		emit windowUpdated();
	}

	template<class P>
	void applyToSource_Geometry(P& obj, void (P::* apply)(Image<T>&)) {

		QApplication::setOverrideCursor(Qt::WaitCursor);
		m_workspace->enableChildren(false);

		removeMask();

		QEventThreads::runThread([&, this]() { (obj.*apply)(m_source); });

		QApplication::restoreOverrideCursor();
		m_workspace->enableChildren(true);
		m_compute_stf = true;

		m_zoom_window.reset();
		enableZoomWindowMode(m_enable_zoom_win);

		zoom(0, 0);
		updateStatisticsDialog();
		updateHistogramDialog();

		emit m_workspace->imageActivated(reinterpret_cast<Image8*>(&m_source));
		emit windowUpdated();
	}

private:
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

	void resetWindowSize();

	void openHistogramDialog();

	void updateHistogramDialog();

	void openStatisticsDialog();

	void updateStatisticsDialog();

	void openImage3DDialog();

	void enableSTF(bool enable);

	void openPSFDialog();
};

template<typename P = uint8_t>
static ImageWindow<P>* imageRecast(QWidget* window) { return static_cast<ImageWindow<P>*>(window); }

template<typename P = uint8_t>
static ImageWindow<P>* imageRecast(ImageWindowBase* window) { return static_cast<ImageWindow<P>*>(window); }

template<typename P, typename T>
static ImageWindow<P>* imageRecast(ImageWindow<T>* window) { return reinterpret_cast<ImageWindow<P>*>(window); }

template<typename P, typename T>
static const ImageWindow<P>* imageRecast(const ImageWindow<T>* window) { return reinterpret_cast<const ImageWindow<P>*>(window); }

typedef ImageWindow<uint8_t> ImageWindow8;
typedef ImageWindow<uint16_t> ImageWindow16;
typedef ImageWindow<float> ImageWindow32;




class ProcessDialog;

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
	ZoomWindow(const QPoint& start_pos, const QColor& pen_color, const ImageWindow<T>& iw, ImageWindowLabel* parent = nullptr) :m_pos(start_pos), m_pen_color(pen_color), m_iw(imageRecast<uint8_t>(&iw)), QWidget(parent) {
		
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
	void windowMoved();

	void windowClosed();

public:
	//moving to image window would allow erasure of template
	template<typename Func>
	void connectZoomWindow(const QObject* receiver, Func&& func) {
		disconnect(m_connection);
		m_connection = connect(this, &ZoomWindow::windowMoved, receiver, func);
	}

	void connectZoomWindow2(const ProcessDialog* receiver, void(ProcessDialog::* func)());

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

	ImageLabel* m_image_label = nullptr;
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
	Image<T> m_source;

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

		//std::thread(apply, obj, std::ref(m_source)).join();
		QEventThreads::runThread(apply, obj, std::ref(m_source));

		if (m_image_window->maskEnabled() && m_image_window->maskExists())
			return updatePreview_Mask();

		displayImage();
	}

	void updatePreview(AutomaticBackgroundExtraction& obj, void (AutomaticBackgroundExtraction::* apply)(const Image<T>&, Image<T>&, float)) {

		resizeSource();
		QEventThreads::runThread(apply, obj, std::ref(m_image_window->source()), std::ref(m_source), scaleFactor());
		displayImage();
	}

	void updatePreview(BilateralFilter& obj, void (BilateralFilter::* apply)(const Image<T>&, Image<T>&, float, const QRectF&)) {

		resizeSource();
		auto& s = m_image_window->source();
		auto r = (!m_ingore_zoom_window && m_image_window->zoomWindow()) ? m_image_window->zoomWindow()->imageRectF() : QRectF(0, 0, s.cols(), s.rows());

		QEventThreads::runThread(apply, obj, std::ref(m_image_window->source()), std::ref(m_source), scaleFactor(), r);

		if (m_image_window->maskEnabled() && m_image_window->maskExists())
			return updatePreview_Mask();

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

