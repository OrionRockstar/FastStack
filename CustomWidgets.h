#pragma once

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





class PushButton : public QPushButton {

	double m_opacity = 1.0;
public:
	PushButton(const QString& text, QWidget* parent = nullptr);

	PushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);

	void setOpacity(double opacity) { m_opacity = opacity; }
private:
	void mousePressEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};





class DialogTitleBar : public QWidget {
	Q_OBJECT

private:
	QPalette m_default_pal;
	QPalette m_opaque_pal;
	double m_opacity = 1.0;

	static const int m_titlebar_height = 28;
	const int m_button_dim = 20;

	const QPixmap pm = QApplication::windowIcon().pixmap(22, 22);

	const QIcon m_shade_icon = QIcon("./Icons//shade-button-32.png");
	const QIcon m_unshade_icon = QIcon("./Icons//unshade-button-32.png");

	QButtonGroup* m_bg;
	bool m_shaded = false;

	QLabel* m_label = nullptr;
	const int m_label_offset = 30;

public:
	DialogTitleBar(QWidget* parent);

signals:
	void shadeWindow();

	void unshadeWindow();

	void iconPressed(Qt::MouseButtons button);

public:
	const QPixmap& pixmap()const { return pm; }

	QString title()const { return m_label->text(); }

	void setTitle(const QString& text) { m_label->setText(text); }

	void resize(int width);

	static int titleBarHeight() { return m_titlebar_height; }

	void setOpaque();

	void unsetOpaque();

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

	double m_default_opacity = 0.975;
	double m_event_opacity = 0.55;

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

	void setOpacity(double opacity) { m_default_opacity = opacity; }

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

	bool eventFilter(QObject* obj, QEvent* e);
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

	auto g = widget->geometry();

	int x = g.x();
	int y = g.y();
	int h = g.height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text() + QString(spaces, QChar(' ')));

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}


class CheckablePushButton : public QPushButton {
public:
	CheckablePushButton(const QString& text, QWidget* parent = nullptr);

private:
	void paintEvent(QPaintEvent* event);
};




class IntValidator : public QIntValidator {
public:
	IntValidator(int bottom, int top, QWidget* parent = nullptr) :QIntValidator(bottom, top, parent) {}

	void fixup(QString& input) const;

	int validate(int value)const;
};


class DoubleValidator : public QDoubleValidator {
public:
	DoubleValidator(double bottom, double top, int precision, QWidget* parent = nullptr) :QDoubleValidator(bottom, top, precision, parent) {}

	void fixup(QString& input) const;
};


class LineEdit : public QLineEdit {
	QLabel* m_label = nullptr;

public:
	LineEdit(QWidget* parent = nullptr) : QLineEdit(parent) {}

	void setLabelText(const QString& txt);

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

	double m_opacity = 1.0;

	const int m_thickness = style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent);
public:
	ScrollBar(QWidget* parent = nullptr) : QScrollBar(parent) {
		this->setAttribute(Qt::WA_NoMousePropagation);

		QPalette pal;
		pal.setColor(QPalette::ColorRole::Button, QColor(169, 169, 169));
		pal.setColor(QPalette::ColorRole::WindowText, QColor(0, 0, 0));
		this->setPalette(pal);
	}

	int thickness()const { return m_thickness; }

	void setOpacity(double opacity) { m_opacity = opacity; }

private:
	void paintEvent(QPaintEvent* event);
};


class Slider : public QSlider {

public:
	Slider(Qt::Orientation orientation, QWidget* parent = nullptr) : QSlider(orientation, parent) {}

private:
	void paintEvent(QPaintEvent* event);
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


class ComboBox : public QComboBox {
public:
	ComboBox(QWidget* parent = nullptr);

	void addLabel(QLabel* label)const;
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

public:
	DoubleSpinBox(QWidget* parent = nullptr);

	DoubleSpinBox(double value, double min, double max, int precision, QWidget* parent = nullptr);

	void addLabel(QLabel* label)const;
};




class ComponentPushButton : public QPushButton {
public:

	ComponentPushButton(const QString& text, QWidget* parent = nullptr);

	ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
private:
	void paintEvent(QPaintEvent* event);
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