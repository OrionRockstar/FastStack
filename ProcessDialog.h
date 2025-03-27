#pragma once
#include <qwidget.h>
#include "Star.h"
#include "Matrix.h"
#include "Workspace.h"
#include "CustomWidgets.h"
#include "Image.h"
//#include "ImageWindow.h"


class DialogToolbar : public QWidget {
	Q_OBJECT

	static const int m_toolbarHeight = 24;
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
	DialogToolbar(QWidget* parent, bool preview = true, bool apply_dd = true, bool apply = true);

	template <typename Func1, typename Func2>
	void connectFunctions(const QObject* receiver,Func1 apply_to_dd, Func2 apply, Func2 preview, Func2 reset) {

		connect(m_apply_dd_pb, &QPushButton::pressed, receiver, apply_to_dd);
		connect(m_apply_pb, &QPushButton::released, receiver, apply);
		connect(m_preview_pb, &QPushButton::released, receiver, preview);
		connect(m_reset_pb, &QPushButton::released, receiver, reset);
	}

	void setGeometry(int x, int y, int w);

	void resize(int width);

	static int toolbarHeight() { return m_toolbarHeight; }
private:
	void addApplyDDButton();

	void addApplyButton();

	void addPreviewButton();

	void resizeEvent(QResizeEvent* e);
};





class Timer : public QTimer {
	Q_OBJECT

public:
	Timer(int interval_ms, QWidget* parent = nullptr) : QTimer(parent) {
		this->setSingleShot(true);
		this->setInterval(interval_ms);
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






class ProcessDialog : public Dialog {
	Q_OBJECT

private:
	DialogToolbar* m_toolbar = nullptr;

	std::unique_ptr<Timer> m_timer;

	const ProcessDialog* m_obj = nullptr;
//protected:
	typedef void (ProcessDialog::* Function)();
//private:
	Function m_func = nullptr;

protected:
	QString m_name;
	QMdiArea* m_workspace;

	QDialog* m_preview = nullptr;

	inline static QString m_crop_image_str = "Crop Image";
	inline static QString m_abe_str = "Automatic Background Extraction";

public:
	ProcessDialog(const QString& name, const QSize& size, QMdiArea* parent_workspace, bool preview = true, bool apply_dd = true, bool apply = true);

	QString name()const { return m_name; }

	void enableSiblings(bool enable);

	void resizeDialog(const QSize& size);

	bool isPreviewValid()const;

signals:
	void processDropped();

private:
	void createDragInstance();

protected:
	void startTimer()const { m_timer->start(); }

	void setTimerInterval(int interval)const {
		m_timer->setInterval(interval);
	}

	template<typename Func>
	void setPreviewMethod(const ProcessDialog* obj, Func func) {

		m_obj = obj;
		m_func = reinterpret_cast<void(ProcessDialog::*)()>(func);
		connect(m_timer.get(), &QTimer::timeout, m_obj, m_func, Qt::UniqueConnection);
	}

	template <typename Func>
	void connectToolbar(const QObject* obj, Func apply, Func preview, Func reset) {

		connect(this, &ProcessDialog::processDropped, obj, apply);
		m_toolbar->connectFunctions(obj, &ProcessDialog::createDragInstance, apply, preview, reset);
	}

	virtual void showPreview(bool ignore_zoomwindow = false);

	virtual void closeEvent(QCloseEvent* close);

private:
	void czw()const;

	void connectZoomWindow();
public:
	const Image8* findImage(const QString& name);
};
