#pragma once

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


class PushButton : public QPushButton {

	void mousePressEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

public:
	PushButton(const QString& text, QWidget* parent = nullptr);

	PushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
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


public:
	ScrollBar(QWidget* parent = nullptr) : QScrollBar(parent) {
		this->setAttribute(Qt::WA_NoMousePropagation);
	}

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