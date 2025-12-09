#pragma once
#include "Image.h"

class DigitalClock : public QLCDNumber {
	Q_OBJECT

public:
	DigitalClock(QWidget* parent = nullptr);

	void displayTime();
};

enum class RectBorder {
	None = 0,
	TopEdge,
	LeftEdge,
	RightEdge,
	BottomEdge,
	TopLeftCorner,
	TopRightCorner,
	BottomLeftCorner,
	BottomRightCorner
};





class Frame {

	int m_frame_width = 6;
	QRect m_inner_region;
	QRect m_left;
	QRect m_right;
	QRect m_top;
	QRect m_bottom;

public:
	Frame(const QRect& rect, int frame_width = 6);

	Frame() = default;

	int frameWidth()const { return m_frame_width; }

	void setFrameWidth(int frame_width) { m_frame_width = frame_width; }

	bool isOnFrame(const QPoint& p)const;

	RectBorder rectBorder(const QPoint& p)const;

	void resize(const QRect& rect);

	QCursor cursorAt(const QPoint& p, QCursor default_return_cursor = Qt::ArrowCursor);
};




class OpaqueStyle : public QProxyStyle {

	qreal m_opacity = 1.0;
public:
	OpaqueStyle(QStyle* style = nullptr) : QProxyStyle(style) {}

	OpaqueStyle(qreal opacity, QStyle* style = nullptr) : m_opacity(opacity), QProxyStyle(style) {}

	void setOpacity(qreal opacity) { m_opacity = opacity; }

	qreal opacity()const { return m_opacity; }
private:
	void drawComplexControl(QStyle::ComplexControl element, const QStyleOptionComplex* opt, QPainter* p, const QWidget* widget) const override {
		p->setOpacity(m_opacity);
		QProxyStyle::drawComplexControl(element, opt, p, widget);
	}

	void drawControl(QStyle::ControlElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget = nullptr) const override {
		p->setOpacity(m_opacity);
		QProxyStyle::drawControl(element, opt, p, widget);
	}

	void drawItemPixmap(QPainter* p, const QRect& rect, int alignment, const QPixmap& pixmap) const override {
		p->setOpacity(m_opacity);
		QProxyStyle::drawItemPixmap(p, rect, alignment, pixmap);
	}

	void drawItemText(QPainter* p, const QRect& rect, int flags, const QPalette& pal, bool enabled, const QString& text, QPalette::ColorRole textRole = QPalette::ColorRole::Text) {
		p->setOpacity(m_opacity);
		QProxyStyle::drawItemText(p, rect, flags, pal, enabled, text, textRole);
	}

	void drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* opt, QPainter* p, const QWidget* widget) const override {
		p->setOpacity(m_opacity);
		QProxyStyle::drawPrimitive(element, opt, p, widget);
	}
};





class PushButton : public QPushButton {

protected:
	float m_opacity = 1.0f;
	QColor m_hover_color = Qt::blue;

public:
	PushButton(const QString& text, QWidget* parent = nullptr);

	PushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

	void setOpacity(double opacity) { m_opacity = opacity; update(); }

	void setMouseHoverColor(const QColor& color = Qt::blue) { m_hover_color = color; }

private:
	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

protected:
	virtual void paintEvent(QPaintEvent* e) override;
};

class FlatPushButton : public PushButton {

public:
	FlatPushButton(const QString& text, QWidget* parent = nullptr);

	FlatPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

private:
	void enterEvent(QEnterEvent* e);

	void leaveEvent(QEvent* e);
};




class DialogTitleBar : public QWidget {
	Q_OBJECT

private:
	double m_opacity = 1.0;

	static const int m_titlebar_height = 28;
	const int m_button_dim = 20;

	const QPixmap pm = QApplication::windowIcon().pixmap(22, 22);

	const QIcon m_shade_icon = QIcon("./Icons//shade-button-32.png");
	const QIcon m_unshade_icon = QIcon("./Icons//unshade-button-32.png");

	QButtonGroup* m_bg;
	bool m_shaded = false;

	QString m_title;
	const int m_label_offset = 30;

public:
	DialogTitleBar(QWidget* parent);

signals:
	void shadeWindow();

	void unshadeWindow();

	void iconPressed(Qt::MouseButtons button);

public:
	const QPixmap& pixmap()const { return pm; }

	QString title()const { return m_title; }

	void setTitle(const QString& text) { m_title = text; update(); }

	void resize(int width);

	static int titleBarHeight() { return m_titlebar_height; }

	void setOpacity(float opacity);

private:
	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};





class Dialog : public QDialog {
	Q_OBJECT

private:
	QMenu* m_menu = nullptr;
	DialogTitleBar* m_titlebar = nullptr;

	QSize m_min_size;
	QSizeGrip* m_sg = nullptr;

protected:
	QTimer* m_timer = nullptr;

	QSize m_default_size = QSize(100, 100);

	const double m_default_opacity = 0.975;
	const double m_event_opacity = 0.55;
	double m_opacity = m_default_opacity;

	const int m_shaded_width = 300;
	QSize m_pre_shade_size;
	const int m_border_width = 3;

	QPoint m_start_pos;
	bool m_moving = false;

	bool m_resizable = false;
	bool m_resizing = false;

private:
	QWidget* m_draw_area = nullptr;

public:
	Dialog(QWidget* parent, bool resizeable = false);

	QString title()const { return m_titlebar->title(); }

	void setTitle(const QString& text) { m_titlebar->setTitle(text); }

	void setOpacity(double opacity) { m_titlebar->setOpacity(m_opacity = opacity); setWindowOpacity(opacity); update(); }

signals:
	void windowClosed();

	void windowCreated();

protected:
	void setDefaultSize(const QSize& size);

	virtual void resizeDialog(int w, int h);

	DialogTitleBar* titlebar()const { return m_titlebar; }

	QWidget* drawArea()const { return m_draw_area; }

	virtual void resizeEvent(QResizeEvent* e);

	virtual void closeEvent(QCloseEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);

	virtual bool eventFilter(QObject* obj, QEvent* e);
};







class WindowSignals : public QWidget {
	Q_OBJECT

public:
	WindowSignals() {}

	~WindowSignals() {}

signals:
	void windowUpdated();

	void windowClosed();

	void windowCreated();
};

static void addLabel(const QWidget* widget, QLabel* label, int spaces = 3) {

	label->setParent(widget->parentWidget());
	auto g = widget->geometry();

	int x = g.x();
	int y = g.y();
	int h = g.height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text() + QString(spaces, QChar(' ')));

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}


class CheckablePushButton : public PushButton {

protected:
	QColor m_hover_color = QColor(89,89,89);//Qt::blue;

public:
	CheckablePushButton(const QString& text, QWidget* parent = nullptr);

	CheckablePushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

	void setMouseHoverColor(const QColor& color = Qt::blue) { m_hover_color = color; }

	void paintEvent(QPaintEvent* event) override;
};



class FlatCheckablePushButton : public CheckablePushButton {

public:
	FlatCheckablePushButton(const QString& text, QWidget* parent = nullptr);

	FlatCheckablePushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

private:
	void enterEvent(QEnterEvent* e);

	void leaveEvent(QEvent* e);
};



class IntValidator : public QIntValidator {
public:
	IntValidator(int bottom, int top, QWidget* parent = nullptr) :QIntValidator(bottom, top, parent) {}

	void fixup(QString& input) const;

	int validate(int value)const;
};


class DoubleValidator : public QDoubleValidator {
public:
	DoubleValidator(double bottom, double top, int precision, QWidget* parent = nullptr) : QDoubleValidator(bottom, top, precision, parent) {}

	void fixup(QString& input) const;
};



class LineEdit : public QLineEdit {

public:
	LineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

	void addLabel(QLabel* label);

	void addSlider(QSlider* slider);
};


class IntLineEdit : public LineEdit {
	Q_OBJECT

private:
	int m_default = 0;

public:
	IntLineEdit(int default_value, IntValidator* validator, QWidget* parent = nullptr);

	const IntValidator* intValidator()const { return reinterpret_cast<const IntValidator*>(this->validator()); }

	int value()const { return this->text().toInt(); }

	void setValue(int value);

	void reset();
};


class DoubleLineEdit : public LineEdit {
	Q_OBJECT

private:
	double m_default = 0.0;

public:
	DoubleLineEdit(DoubleValidator* validator, QWidget* parent = nullptr);

	DoubleLineEdit(double default_value, DoubleValidator* validator, QWidget* parent = nullptr);

	DoubleLineEdit(double default_value, DoubleValidator* validator, int max_length, QWidget* parent = nullptr);

	const DoubleValidator* doubleValidator()const { return reinterpret_cast<const DoubleValidator*>(this->validator()); }

	float valuef()const { return this->text().toFloat(); }

	double value()const { return this->text().toDouble(); }

	void setValue(float value);

	void setValue(double value);

	void removeEndDecimal();

	void addTrailingZeros();

	void reset();
};



class ScrollBar : public QScrollBar {
	Q_OBJECT

	double m_opacity = 1.0;

	const int m_thickness = style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent);

	const QColor hover_color = { 89,20,148 };
	const QColor action_color = { 69,0,128 };

	QStyle::SubControl m_current_sc = QStyle::SC_None;

	inline static const QStyle::SubControl sc_sb_addline = QStyle::SC_ScrollBarAddLine;
	inline static const QStyle::SubControl sc_sb_subline = QStyle::SC_ScrollBarSubLine;
	inline static const QStyle::SubControl sc_sb_slider = QStyle::SC_ScrollBarSlider;

public:
	ScrollBar(QWidget* parent = nullptr);

	ScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

	int thickness()const { return m_thickness; }

	void setOpacity(double opacity) { m_opacity = opacity; }

private:
	QStyle::SubControl currentSC()const { return m_current_sc; }

	void mousePressEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* event);
};


class Slider : public QSlider {

	QPen m_pen = QPen(QColor(123, 0, 216), 1.5);
	QBrush m_brush = QBrush({ 39,39,39 });
	bool m_sunken = false;

public:
	Slider(QWidget* parent = nullptr, Qt::Orientation orientation = Qt::Horizontal);

	int value()const { return sliderPosition(); }

	bool isMouseOverHandle();

protected:
	void drawHandle(QPainter& p, QStyleOptionSlider& opt);

	bool event(QEvent* e);

	void paintEvent(QPaintEvent* event);
};






class InputBase : public QObject {
	Q_OBJECT

protected:
	float m_slider_mult = 1.0;
	std::function<void()> m_edited;// = std::bind(&DoubleInput::lineEdited, this);

	QMetaObject::Connection m_le_conn;
	QMetaObject::Connection m_slider_conn;

	QLabel* m_label = nullptr;
	LineEdit* m_line_edit = nullptr;
	Slider* m_slider = nullptr;

	InputBase(const QString& label_txt, QWidget* parent = nullptr, float slider_multiplier = 1.0);
public:
signals:
	void actionTriggered(int);

	void editingFinished();

public:
	virtual void onAction(std::function<void(int)> func = nullptr) = 0;

	virtual void onEdited(std::function<void()> func = nullptr) = 0;

	int sliderValue()const { return m_slider->value(); }

	int sliderPosition()const { return m_slider->sliderPosition(); }

	void setSliderValue(int value) { m_slider->setValue(value); }

	void setSliderAttributes(int min, int max, int single_step = 1, int page_step = 0);

	void setSliderWidth(int w) { m_slider->resize(w, m_slider->style()->pixelMetric(QStyle::PM_SliderThickness)); }

	void resizeLineEdit(int w, int h);

	void setLineEditWidth(int w);

	void setMaxLength(int l) { m_line_edit->setMaxLength(l); }

	void move(int ax, int ay);

	void setToolTip(const QString& txt) { m_line_edit->setToolTip(txt); }

	void setEnabled(bool enable) {
		m_line_edit->setEnabled(enable);
		m_slider->setEnabled(enable);
	}

	void setText(const QString& txt);

	virtual void reset() = 0;
};









class IntegerInput : public InputBase {

	IntLineEdit* m_ile = nullptr;

public:
	IntegerInput(const QString& label_txt, int default_value, IntValidator* validator, QWidget* parent = nullptr, float slider_multiplier = 1.0);

private:
	void lineEdited() {
		m_slider->setValue(m_ile->value() * m_slider_mult);
	}

	void sliderAction(int) {
		m_ile->setValue(m_slider->value() / m_slider_mult);
	}

public:
	void onAction(std::function<void(int)> func = nullptr);

	void onEdited(std::function<void()> func = nullptr);

	int value()const { return m_ile->value(); }

	void setLineEditValue(int value) { m_ile->setValue(value); }

	void setValue(double value) {
		m_ile->setValue(value);
		m_edited();
	}

	void reset() {
		m_ile->reset();
		m_edited();
	}
};






class DoubleInput : public InputBase {

	DoubleLineEdit* m_dle = nullptr;

public:
	DoubleInput(const QString& label_txt, double default_value, DoubleValidator* validator, QWidget* parent = nullptr, float slider_multiplier = 1.0);

private:
	void lineEdited() {
		m_slider->setValue(m_dle->value() * m_slider_mult);
	}

	void sliderAction(int) {
		m_dle->setValue(m_slider->sliderPosition() / m_slider_mult);
	}

public:
	void onAction(std::function<void(int)> func = nullptr);

	void onEdited(std::function<void()> func = nullptr);

	double value()const { return m_dle->value(); }

	float valuef()const { return m_dle->valuef(); }

	void setLineEditValue(double value) { m_dle->setValue(value); }

	void setLineEditValue(float value) { m_dle->setValue(value); }

	void setValue(double value) {
		m_dle->setValue(value);
		m_edited();
	}

	void setValue(float value) {
		m_dle->setValue(value);
		m_edited();
	}

	void reset() {
		m_dle->reset();
		m_edited();
	}
};




class RadioButton : public QRadioButton {

	QColor m_indicator_color = QColor(123, 0, 216);
	QColor m_base_color = QColor(196, 196, 196);
	//QColor m_text_color = Qt::white;

public:
	RadioButton(const QString& text, QWidget* parent = nullptr) : QRadioButton(text, parent) {}

private:
	void paintEvent(QPaintEvent* event);
};


class CheckBox : public QCheckBox {
public:
	CheckBox(const QString& text, QWidget* parent = nullptr) : QCheckBox(text, parent) {
		QPalette pal;
		pal.setBrush(QPalette::Text, QColor(123, 0, 216));
		this->setPalette(pal);
	}
};


template<typename T = uint8_t>
class ImageWindow;

class ComboBox : public QComboBox {
public:
	ComboBox(QWidget* parent = nullptr);

	void addLabel(QLabel* label)const;
};

class ImageComboBox : public ComboBox {

public:
	ImageComboBox(QWidget* parent = nullptr) : ComboBox(parent) {}

	void addImage(const QString& name, const Image8* img);

	void addImage(const ImageWindow<>* iw);

	const Image8* currentImage();

	const Image8* imageAt(int index);

	int findImage(const Image8* img);
};




class InterpolationComboBox : public ComboBox {
	int m_defualt_index = 2;
public:
	InterpolationComboBox(QWidget* parent = nullptr);

	void reset() { setCurrentIndex(m_defualt_index); }
};




class SpinBox : public QSpinBox {

public:
	SpinBox(QWidget* parent = nullptr);

	SpinBox(int value, int min, int max, QWidget* parent = nullptr);

	void addLabel(QLabel* label)const;
};


class DoubleSpinBox : public QDoubleSpinBox {
	Q_OBJECT

public:
	DoubleSpinBox(QWidget* parent = nullptr);

	DoubleSpinBox(double value, double min, double max, int precision, QWidget* parent = nullptr);

	void addLabel(QLabel* label)const;

signals:
	void step(int steps);

private:
	void stepBy(int steps) {
		QDoubleSpinBox::stepBy(steps);
		emit step(steps);
	}
};




class ComponentPushButton : public FlatCheckablePushButton {
public:

	ComponentPushButton(const QString& text, QWidget* parent = nullptr);

	ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
private:
	//void paintEvent(QPaintEvent* event);
};




class GroupBox : public QGroupBox {
	CheckBox* m_cb;

public:
	GroupBox(QWidget* parent) : QGroupBox(parent) {}

	GroupBox(const QString& title, QWidget* parent) : QGroupBox(title, parent) {}

private:
	void paintEvent(QPaintEvent* event);
};


class ListWidget : public QListWidget {

public:
	ListWidget(QWidget* parent = nullptr);
};


class FileSelection : public QWidget {

	std::vector<std::filesystem::path> m_paths;

	QPushButton* m_add_files_pb;
	QPushButton* m_remove_file_pb;
	QPushButton* m_clear_list_pb;

	QListWidget* m_file_list_view;

	QString m_typelist =
		"FITS file(*.fits *.fts *.fit);;"
		"XISF file(*.xisf);;"
		"TIFF file(*.tiff *.tif);;";

public:
	FileSelection(QWidget* parent = nullptr);

private:
	void dragEnterEvent(QDragEnterEvent* event);

	void dropEvent(QDropEvent* event);

	bool isValidFileType(const QString& path);

	void onAddFiles();

	void onRemoveFile();

public:
	void onClearList();

	const std::vector<std::filesystem::path>& FilePaths()const { return m_paths; }
};