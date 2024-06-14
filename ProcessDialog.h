#pragma once
#include <qwidget.h>
#include "ImageWindow.h"

class PushButton : public QPushButton {
public:
	PushButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {

		this->setAutoDefault(false);
		this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");
	}
};

class Toolbar : public QWidget {
	Q_OBJECT

	int dx = 24;
	int dy = 24;

	QPushButton* m_apply_to;
	QIcon m_apply_to_icon;

	QPushButton* m_apply_current;
	QIcon m_apply_icon;

	QPushButton* m_preview;
	QIcon m_preview_icon;

	QPushButton* m_reset;
	QIcon m_reset_icon;

public:
	Toolbar(QWidget* parent, bool preview = true);

	template <typename Func1, typename Func2>
	void Connect(const QObject* receiver,Func1 apply_to_dd, Func2 apply, Func2 preview, Func2 reset) {

		connect(m_apply_to, &QPushButton::pressed, receiver, apply_to_dd);
		connect(m_apply_current, &QPushButton::released, receiver, apply);
		connect(m_preview, &QPushButton::pressed, receiver, preview);
		connect(m_reset, &QPushButton::pressed, receiver, reset);
	}

	void setDownFalse() { m_apply_to->setDown(false); }

	void enableApply(bool s) { m_apply_current->setEnabled(s); m_apply_current->setDown(s); }
};

class Timer : public QTimer {
	Q_OBJECT

public:
	Timer(int interval, QWidget* parent) : QTimer(parent) {
		this->setSingleShot(true);
		this->setInterval(interval);
	}
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


class ProgressDialog : public QProgressDialog {
	Q_OBJECT

public:
	ProgressDialog(ProgressSignal* signal_object) {

		//this->setAutoReset(true);

		//this->setAutoClose(true);
		this->setRange(0, 100);
		this->setModal(true);

		this->setMinimumDuration(0);
		this->open();
		this->setValue(0);

		connect(signal_object, &ProgressSignal::emitProgress, this, &QProgressDialog::setValue);
		connect(signal_object, &ProgressSignal::emitText, this, &ProgressDialog::setLabelText);
	}

public slots:
	void setLabelText(const QString& text) {
		QProgressDialog::setLabelText(text);
		qApp->processEvents();
	}
};


class ProcessDialog : public QDialog {
	Q_OBJECT

//protected:
private:
	Toolbar* m_tb;
	Timer* m_timer;

protected:
	QString m_name;
	QMdiArea* m_workspace;

public:

	ProcessDialog(QString name, const QSize& size, QWidget* parent, bool preview = true);

	ProcessDialog(QString name, const QSize& size, QMdiArea& workspace, QWidget* parent, bool preview = true);

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

		m_tb->Connect(receiver, apply_to_dd, apply, preview, reset);
	}

	virtual void showPreview();

	QString Name()const { return m_name; }

	bool PreviewProcessNameMatches(const QString& preview_name)const;

	bool isPreviewValid()const;

	void setEnabledAll(bool val) {
		for (auto child : parentWidget()->children())
			if (child->inherits("ProcessDialog"))
				((QWidget*)child)->setEnabled(val);
		QApplication::processEvents();
	}
};


class DoubleValidator : public QDoubleValidator {
public:
	DoubleValidator(double bottom, double top, int precision, QWidget* parent = nullptr) :QDoubleValidator(bottom, top, precision, parent) {}
	
	void fixup(QString& input) const;
};

class DoubleLineEdit : public QLineEdit {
	Q_OBJECT

public:

	DoubleLineEdit(DoubleValidator* validator, QWidget* parent = nullptr);

	DoubleLineEdit(const QString& contents, DoubleValidator* validator, QWidget* parent = nullptr);

	DoubleLineEdit(const QString& contents, DoubleValidator* validator, int max_length, QWidget* parent = nullptr);

	const DoubleValidator* Validator()const { return reinterpret_cast<const DoubleValidator*>(this->validator()); }

	float Valuef()const { return this->text().toFloat(); }

	double Value()const { return this->text().toDouble(); }

	int Valuei()const { return this->text().toInt(); }

	void setValue(float value);

	void setValue(double value);

	void setValue(int value);

	void setText_Validated(const QString& text);

	void removeEndDecimal();

	void addTrailingZeros();

	void addLabel(QLabel* label)const;

	void addSlider(QSlider* slider)const;
};

class ComponentPushButton : public QPushButton {
	Q_OBJECT
public:

	ComponentPushButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {

		this->setAutoFillBackground(true);
		this->setBackgroundRole(QPalette::ColorRole::Dark);
		this->setFlat(true);

		this->setAutoDefault(false);
		this->setCheckable(true);
		this->setAutoExclusive(true);
		QFont font = this->font();
		font.setPointSize(8);
		this->setFont(font);
		this->resize(this->fontMetrics().horizontalAdvance(text) + 10, this->size().height());
	}

	ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr) : QPushButton(icon, text, parent) {

		this->setAutoFillBackground(true);
		this->setBackgroundRole(QPalette::ColorRole::Dark);
		this->setFlat(true);

		this->setAutoDefault(false);
		this->setCheckable(true);
		this->setAutoExclusive(true);
		QFont font = this->font();
		font.setPointSize(8);
		this->setFont(font);
	}
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
