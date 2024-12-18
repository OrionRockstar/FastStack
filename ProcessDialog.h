#pragma once
#include <qwidget.h>
#include "Star.h"
#include "Matrix.h"

//#include "ImageWindow.h"

class PushButton : public QPushButton {

	void mousePressEvent(QMouseEvent* e) {
		setDown(true);
		emit pressed();
	}

	void mouseReleaseEvent(QMouseEvent* e) {
		setDown(false);
		if (hitButton(e->pos()))
			emit released();
	}

public:
	PushButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {

		this->setAutoDefault(false);
		this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");
	}

	PushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr) : QPushButton(icon, text, parent) {

		this->setAutoDefault(false);
		this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");
	}
};

class Toolbar : public QWidget {
	Q_OBJECT

	QSize m_button_size = { 24,24 };

	bool m_apply_dd = true;
	PushButton* m_apply_dd_pb = nullptr;
	QIcon m_apply_to_icon;

	bool m_apply = true;
	PushButton* m_apply_pb = nullptr;
	QIcon m_apply_icon;

	bool m_preview = true;
	PushButton* m_preview_pb = nullptr;
	QIcon m_preview_icon;

	PushButton* m_reset_pb = nullptr;
	QIcon m_reset_icon;

public:
	Toolbar(QWidget* parent, bool preview = true, bool apply_dd = true, bool apply = true);

	template <typename Func1, typename Func2>
	void Connect(const QObject* receiver,Func1 apply_to_dd, Func2 apply, Func2 preview, Func2 reset) {

		connect(m_apply_dd_pb, &QPushButton::pressed, receiver, apply_to_dd);
		connect(m_apply_pb, &QPushButton::released, receiver, apply);
		connect(m_preview_pb, &QPushButton::released, receiver, preview);
		connect(m_reset_pb, &QPushButton::released, receiver, reset);
	}

private:
	void addApplyDDButton();

	void addApplyButton();

	void addPreviewButton();
	//void setToolTip_Apply(const QString& txt) { m_apply->setToolTip(txt); }
};

class Timer : public QTimer {
	Q_OBJECT

public:
	Timer(int interval, QWidget* parent) : QTimer(parent) {
		this->setSingleShot(true);
		this->setInterval(interval);
	}

	Timer() = default;
};



class ProgressSignal: public QObject {
	Q_OBJECT
public:
	ProgressSignal(QObject* parent = nullptr) {}
	~ProgressSignal() {}
public slots:

signals:
	void emitProgress(int val);

	void emitText(const QString& text);
};


/*class CollapsableSection : public QWidget {
	Q_OBJECT

	QWidget* m_widget;
	QPushButton* m_pb;
public:
	CollapsableSection(const QString& title, int width, QWidget* parent, bool checkable = false) : QWidget(parent) {

		this->resize(width, 25);

		QLabel* label = new QLabel(title, this);
		label->move(30, 0);

		m_pb = new QPushButton(this);
		m_pb->setCheckable(true);
		m_pb->setChecked(true);
		m_pb->move(this->width() - 30, 0);
		connect(m_pb, &QPushButton::clicked, this, [this](bool v) { emit clicked(v); });
	}
	signals:
	void clicked(bool v);

public:
	void addWidget(QWidget* widget) { m_widget = widget; }
};*/



class ProgressDialog : public QProgressDialog {
	Q_OBJECT

public:

	ProgressDialog(QWidget* parent = nullptr);

	ProgressDialog(const ProgressSignal& signal_object, QWidget* parent = nullptr);

};



class TextDisplay : public QDialog {

	Q_OBJECT

		QPlainTextEdit* m_pte;
public:
	TextDisplay(const QString& title, QWidget* parent = nullptr);

	void displayMatrix(const Matrix& m);

	void displayText(const QString& txt);

	void displayProgress(int progress);

	void setTextLine(const QString& txt);

	void displayPSFData(uint16_t size, const PSF& psf);

signals:
	void onClose();

private:
	void resizeEvent(QResizeEvent* e) {
		QDialog::resizeEvent(e);
		m_pte->resize(size());
	}

	void closeEvent(QCloseEvent* e) {
		QDialog::closeEvent(e);
		emit onClose();
		e->accept();
	}
};




static void MessageBox(const QString& message, QWidget* parent) {
	QMessageBox::information(parent, "FastStack", message);
}




class ProcessDialog : public QDialog {
	Q_OBJECT

	Toolbar* m_toolbar = nullptr;
	Timer* m_timer;

protected:
	QString m_name;
	QMdiArea* m_workspace;

	QDialog* m_preview = nullptr;

public:
	ProcessDialog(QString name, const QSize& size, QMdiArea* parent_workspace, bool preview = true, bool apply_dd = true, bool apply = true);
signals:
	void onClose();

	void processDropped();

public:
	void CreateDragInstance();

protected:
	template<typename Func>
	void setTimer(int interval, QObject* object, Func method) {
		m_timer = new Timer(interval, this);
		connect(m_timer, &QTimer::timeout, object, method);
	}

	void startTimer();

	void closeEvent(QCloseEvent* close);

	bool eventFilter(QObject* o, QEvent* e) {

		if (o == this && e->type() == QEvent::Move)
			this->setWindowOpacity(0.55);

		else if( this->windowOpacity() != 0.95)
			this->setWindowOpacity(0.95);

		return false;
	}

	template <typename Func1, typename Func2>
	void ConnectToolbar(const QObject* receiver, Func1 apply_to_dd, Func2 apply, Func2 preview, Func2 reset) {

		m_toolbar->Connect(receiver, apply_to_dd, apply, preview, reset);
	}

	virtual void showPreview();

	QString name()const { return m_name; }

	bool isPreviewValid()const;

	void setEnabledAll(bool val) {
		for (auto child : parentWidget()->children())
			if (child->inherits("ProcessDialog"))
				((QWidget*)child)->setEnabled(val);
		QApplication::processEvents();
	}

	void resize(const QSize& size) {
		QDialog::resize(size);
		if (m_toolbar != nullptr)
			m_toolbar->move(0, height() - m_toolbar->height());
	}

	void showDialog() {
		QDialog::show();
		this->activateWindow();
	}

	void updateImageLabel(const QMdiSubWindow* window);
};





class DoubleValidator : public QDoubleValidator {
public:
	DoubleValidator(double bottom, double top, int precision, QWidget* parent = nullptr) :QDoubleValidator(bottom, top, precision, parent) {}
	
	void fixup(QString& input) const;
};

class IntValidator : public QIntValidator {
public:
	IntValidator(int bottom, int top, QWidget* parent = nullptr) :QIntValidator(bottom, top, parent) {}

	void fixup(QString& input) const;

	int validate(int value)const;
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

	//const IntValidator* intValidator()const { return reinterpret_cast<const IntValidator*>(this->validator()); }

	float valuef()const { return this->text().toFloat(); }

	double value()const { return this->text().toDouble(); }

	void setValue(float value);

	void setValue(double value);

	void removeEndDecimal();

	void addTrailingZeros();
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

	int m_default = 0;

public:
	Slider(Qt::Orientation orientation, QWidget* parent = nullptr) : QSlider(orientation, parent) {}

	void setDefualtValue(int value) { m_default = value; setValue(value); }

	void reset() { setValue(m_default); }

private:
	void paintEvent(QPaintEvent* event);
};


class RadioButton : public QRadioButton {

	QColor m_indicator_color = QColor(123, 0, 216);
	QColor m_base_color = QColor(196, 196, 196);
	//QColor m_text_color = Qt::white;

public:
	RadioButton(const QString& text, QWidget* parent = nullptr) : QRadioButton(text, parent) {}

	void setIndicatorColor(const QColor& color) { m_indicator_color = color; }

	void setBaseColor(const QColor& color) { m_indicator_color = color; }

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


class ComponentPushButton : public QPushButton {
public:

	ComponentPushButton(const QString& text, QWidget* parent = nullptr);

	ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr);
private:
	void paintEvent(QPaintEvent* event);
};


class CheckablePushButton : public QPushButton {
public:
	CheckablePushButton(const QString& text, QWidget* parent = nullptr);

private:
	void paintEvent(QPaintEvent* event);
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
