#pragma once
//#include "ui_ImageWindow.h"
#include "Image.h"
#include "HistogramTransformation.h"
//#include "ProcessDialog.h"
#include "CustomWidgets.h"
#include "Statistics.h"
#include "Interpolator.h"


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

public:
	ImageLabel(const QImage& img, QWidget* parent);

	void setMaskVisible(bool show = true) { m_mask_visible = show; }

	void setMask(const QImage* mask) { m_mask = mask; }

	void removeImage() { m_image = nullptr; }

	void removeMask() { m_mask = nullptr; }

	void draw() { update(); }

	void resetCursor() { this->setCursor(m_cross_cursor); }
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

	void windowCreated();
};


class ImageInfoSignals : public QWidget {
	Q_OBJECT

public:
	ImageInfoSignals() {}

	~ImageInfoSignals() {}

signals:
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

	bool isZoomChecked()const { return m_zoom_win_pb->isChecked(); }

signals:
	void zoomWindowClicked(bool checked);

	void stfClicked(bool checked);

	void image3DReleased();

	void statisticsReleased();

	void histogramReleased();

private:
	void resizeEvent(QResizeEvent* e);
};





class IWScrollArea : public QScrollArea {

public:
	IWScrollArea(QWidget* parent = nullptr) : QScrollArea(parent) {
		this->setBackgroundRole(QPalette::ColorRole::Dark);
		QString corner = "QAbstractScrollArea::corner{background-color: dark gray; }";
		this->setStyleSheet(corner);
	}

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

public:
	SubWindow(QWidget* widget);

};


class DialogTitleBar : public QWidget {
	Q_OBJECT

	const int m_titlebar_height = 28;
	const int m_button_dim = m_titlebar_height;

	const QPixmap pm = QApplication::windowIcon().pixmap(22, 22);

	const QIcon m_shade_icon = QIcon("./Icons//shade-button-32.png");
	const QIcon m_unshade_icon = QIcon("./Icons//unshade-button-32.png");

	QButtonGroup* m_bg;
	bool m_shaded = false;

	QLabel* m_label = nullptr;
	const int m_label_offset = 30;

public:
	DialogTitleBar(QWidget* parent) : QWidget(parent) {

		this->setFixedHeight(m_titlebar_height);
		this->setGeometry(0, 0, parent->width(), m_titlebar_height);

		QPalette p;
		p.setColor(QPalette::Window, QColor(69, 0, 128));
		p.setColor(QPalette::Button, QColor(39, 39, 39));
		p.setColor(QPalette::ButtonText, Qt::white);

		this->setPalette(p);
		this->setAutoFillBackground(true);

		m_bg = new QButtonGroup(this);
		m_bg->setExclusive(true);

		m_bg->addButton(new PushButton(QIcon("./Icons//close-button-32.png"), "", this), 1);
		connect(m_bg->button(1), &QPushButton::released, parent, &QWidget::close);

		m_bg->addButton(new PushButton(m_shade_icon, "", this), 2);		
		auto onShade = [this]() {

			if (m_shaded == false) {
				m_shaded = true;
				m_bg->button(2)->setIcon(m_unshade_icon);
				emit shadeWindow();
			}

			else {
				m_shaded = false;
				m_bg->button(2)->setIcon(m_shade_icon);
				emit unshadeWindow();
			}

		};
		connect(m_bg->button(2), &QPushButton::released, this, onShade);


		for (auto button : m_bg->buttons()) {
			button->setPalette(p);
			button->resize(m_button_dim, m_button_dim);
			button->move(width() - (m_bg->id(button) * m_button_dim), 0);
		}

		m_label = new QLabel("", this);
		m_label->setFixedHeight(m_titlebar_height);
		m_label->setGeometry(m_label_offset, 0, width() - (m_label_offset + m_button_dim), m_titlebar_height);
		m_label->setAttribute(Qt::WA_TransparentForMouseEvents);

		this->show();
	}

signals:
	void shadeWindow();

	void unshadeWindow();

public:
	QString title()const { return m_label->text(); }

	void setTitle(const QString& text) {
		m_label->setText(text);
	}

	void resize(int width) {
		QWidget::resize(width, m_titlebar_height);

		for (auto button : m_bg->buttons())
			button->move(width - (m_bg->id(button) * m_button_dim), 0);

		m_label->setGeometry(m_label_offset,0, width - (m_label_offset + m_bg->buttons().size() * m_button_dim), m_titlebar_height);
	}

private:
	void paintEvent(QPaintEvent* e) {
		QPainter p(this);
		p.drawPixmap(3, 3, pm);
	}
};


class Prev : public QDialog {

	const int m_titlebar_height = 30;

	DialogTitleBar* m_titlebar;

	QPoint m_start_pos;
	bool m_moving = false;

public:
	Prev(QWidget* parent) : QDialog(parent) {
		this->resize(200, 100);

		m_titlebar = new DialogTitleBar(this);
		m_titlebar->setTitle("Prev");

		this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
		this->show();

	}

private:
	void mousePressEvent(QMouseEvent* e) {
		if (e->buttons() == Qt::LeftButton && childAt(e->pos()) == m_titlebar) {
			m_start_pos = e->pos();
			m_moving = true;
		}
	}

	void mouseMoveEvent(QMouseEvent* e) {
		if (e->buttons() == Qt::LeftButton && m_moving)
			this->move(geometry().topLeft() + (e->pos() - m_start_pos));
	}

	void mouseReleaseEvent(QMouseEvent* e) {
		if (e->button() == Qt::LeftButton)
			m_moving = false;
	}

};



template<typename T> 
class PreviewWindow;

class ZoomWindow;

template<typename T>
class ImageWindow : public QWidget {

	//Preview* pv;

	QMdiArea* m_workspace;
	SubWindow* m_sw_parent;
	ImageWindowToolbar* m_toolbar = nullptr;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	QString m_name;
	Image<T> m_source;

	ImageLabel* m_image_label = nullptr;
	QImage m_display;
	QImage m_mask_display;

	std::unique_ptr<PreviewWindow<T>> m_preview;

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

	const int m_titlebar_height = QApplication::style()->pixelMetric(QStyle::PM_TitleBarHeight);
	const int m_border_width = QApplication::style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);

	const std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();
	const std::unique_ptr<ImageInfoSignals> m_iis = std::make_unique<ImageInfoSignals>();

	std::unique_ptr<StatisticsDialog> m_stats_dialog;
	Statistics::StatsVector m_sv;
	Statistics::StatsVector m_sv_clipped;

	QGraphicsOpacityEffect m_opacity_effect;

	std::unique_ptr<HistogramDialog> m_hist_dialog;

	std::unique_ptr<Image3DDialog> m_img3D_dialog;

	const PencilCursor m_pencil_cursor;
	bool m_draw_zoom_win = false;
	std::unique_ptr<ZoomWindow> m_zoom_window;

public:
	WindowSignals* windowSignals()const { return m_ws.get(); }

	ImageInfoSignals* imageInfoSignals()const { return m_iis.get(); }

	ImageWindow() = default;

	ImageWindow(Image<T>&& img, QString name, QWidget* parent = nullptr);

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

	const Image<T>& source()const { return m_source; }

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

	bool previewExists()const { return (m_preview.get() != nullptr); }

	PreviewWindow<T>* preview()const { return m_preview.get(); }

	void showPreview(PreviewWindow<T>* preview = nullptr);

	void closePreview();

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
	void applyToSource(P& obj, void (P::* apply)(Image<T>&)) {

		if (maskEnabled() && this->maskExists())
			return this->applyWithMask<P>(obj, apply);

		(obj.*apply)(m_source);
		//std::thread t(apply, &obj, std::ref(m_source));
		//t.join();

		m_compute_stf = true;

		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
		updateHistogramDialog();
	}

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

		m_zoom_window.reset();
		drawZoomWindow();

		showScrollBars();
		resizeImageLabel();
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
		resizeImageLabel();
		displayImage();
		updateStatisticsDialog();
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

	void drawZoomWindow();

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

	QRect m_img_rect;

	QPoint m_pos;
	QMetaObject::Connection m_connection;

public:
	template<typename T>
	ZoomWindow(const ImageWindow<T>& iw, QWidget* parent = nullptr) : m_iw(imageRecast<uint8_t>(&iw)), QWidget(parent) {
		this->setAttribute(Qt::WA_DeleteOnClose);
		this->show();
	}

	template<typename T>
	ZoomWindow(const ImageWindow<T>& iw, const QRect& geometry, QWidget* parent = nullptr) : m_iw(imageRecast<uint8_t>(&iw)), QWidget(parent) {

		this->setGeometry(geometry);
		this->setAttribute(Qt::WA_DeleteOnClose);

		double factor = iw.factor();
		float x = geometry.x() / factor + m_iw->sourceOffsetX();
		float y = geometry.y() / factor + m_iw->sourceOffsetY();
		float w = geometry.width() / factor;
		float h = geometry.height() / factor;

		m_img_rect = QRect(x, y, w, h);

		this->show();
	}

	QRect frameRect()const;

	QRect imageRect()const { return m_img_rect; }

	bool isInBounds(const QRect& rect)const;

	void keepFrameInImage(QRect& rect)const;

	void startCorner(const QPoint& p);

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
	void wheelEvent(QWheelEvent* e) { e->ignore(); }

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};




class Preview : public SubWindow {

	int m_bin_factor = 1;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	Image8* m_source;
	ImageLabel* m_image_label;
	QImage m_display;

	QString m_process_type = "";

	bool m_ingore_zoom_window = false;

	const std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();

};



template <typename T>
class PreviewWindow: public QDialog {

	DialogTitleBar* m_titlebar;
	QPoint m_start_pos;
	bool m_moving = false;
	//const ImageWindow<T>* m_parent_window

	int m_bin_factor = 1;

	int m_drows = 0;
	int m_dcols = 0;
	int m_dchannels = 0;

	Image<T> m_source;
	ImageLabel* m_image_label;
	QImage m_display;

	QString m_process_type = "";

	bool m_ingore_zoom_window = false;

	const std::unique_ptr<WindowSignals> m_ws = std::make_unique<WindowSignals>();

public:
	PreviewWindow(QWidget* parent = nullptr, bool ignore_zoom_window = false);

	int binFactor()const { return m_bin_factor; }

	int rows()const { return m_drows; }

	int cols()const { return m_dcols; }

	int channels()const { return m_dchannels; }

	ImageType type()const { return m_source.type(); }

	ImageWindow<T>* parentWindow()const { return imageRecast<T>(parentWidget()); }

	void setProcessType(QString process_type = "") { m_process_type = process_type; }

	const QString& processType()const { return m_process_type; }

	WindowSignals* windowSignals()const { return m_ws.get(); }

	bool isZoomWidnowIgnored()const { return m_ingore_zoom_window; }

	QString title()const { return m_titlebar->title(); }

	void setTitle(const QString& text) { m_titlebar->setTitle(text); }

	double zoomWindowScaleFactor()const;

	void updateSourcefromZoomWindow();

private:
	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void closeEvent(QCloseEvent* close);

protected:
	void updateSource();

	void resizePreview(int w, int h);

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
	Image32 getMask() {

		if (!m_ingore_zoom_window && parentWindow()->zoomWindow())
			return getMaskZoomWindow<P>();

		int factor2 = binFactor() * binFactor();

		auto mwp = parentWindow()->mask();
		Image32 mask(rows(), cols(), mwp->channels());

		const Image<P>* mask_source = reinterpret_cast<const Image<P>*>(&mwp->source());
		for (int ch = 0; ch < mask.channels(); ++ch) {
			for (int y = 0, y_s = 0; y < rows(); ++y, y_s += binFactor()) {
				for (int x = 0, x_s = 0; x < cols(); ++x, x_s += binFactor()) {

					double pix = 0;

					for (int j = 0; j < binFactor(); ++j)
						for (int i = 0; i < binFactor(); ++i)
							pix += (*mask_source)(x_s + i, y_s + j, ch);

					mask(x, y, ch) = Pixel<float>::toType(P(pix / factor2));

				}
			}
		}

		return mask;
	}

	template<typename P>
	Image32 getMaskZoomWindow() {

		QRect r = parentWindow()->zoomWindow()->imageRect();

		double rx = double(width()) / r.width();
		double ry = double(height()) / r.height();
		double s = math::min(rx, ry);

		auto mwp = parentWindow()->mask();
		Image32 mask(rows(), cols(), mwp->channels());

		double _s = 1 / s;

		const Image<P>& mask_source = *reinterpret_cast<const Image<P>*>(&mwp->source());

		for (uint32_t ch = 0; ch < mask.channels(); ++ch) {
			for (int y = 0; y < rows(); ++y) {
				double y_s = y * _s + r.y();

				for (int x = 0; x < cols(); ++x) {
					double x_s = x * _s + r.x();

					mask(x, y, ch) = Interpolator(Interpolator::Type::bicubic_spline).interpolatePixel(mask_source, { x_s, y_s, ch });
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

template<typename P = uint8_t>
static const PreviewWindow<P>* previewRecast(const QDialog* preview) { return static_cast<const PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static PreviewWindow<P>* previewRecast(PreviewWindow<T>* preview) { return reinterpret_cast<PreviewWindow<P>*>(preview); }

template<typename P, typename T>
static const PreviewWindow<P>* previewRecast(const PreviewWindow<T>* preview) { return reinterpret_cast<const PreviewWindow<P>*>(preview); }

typedef PreviewWindow<uint8_t> PreviewWindow8;
typedef PreviewWindow<uint16_t> PreviewWindow16;
typedef PreviewWindow<float> PreviewWindow32;

