#pragma once
#include <qwidget.h>
#include "Star.h"
#include "Workspace.h"
#include "CustomWidgets.h"


class DialogToolbar : public QWidget {
	Q_OBJECT

public:
	static const int m_toolbarHeight = 24;
	QSize m_button_size = { 24,24 };

	bool m_apply_dd = true;
	FlatPushButton* m_apply_dd_pb = nullptr;
	QIcon m_apply_to_icon;

	bool m_apply = true;
	FlatPushButton* m_apply_pb = nullptr;
	QIcon m_apply_icon;

	bool m_preview = true;
	FlatPushButton* m_preview_pb = nullptr;
	QIcon m_preview_icon;

	FlatPushButton* m_reset_pb = nullptr;
	QIcon m_reset_icon;

public:
	DialogToolbar(QWidget* parent, bool preview = true, bool apply_dd = true, bool apply = true);

	template <typename Func>
	void connectFunctions(const QObject* receiver, Func&& apply_to_dd, Func&& apply, Func&& preview, Func&& reset) {

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
private:
	uint32_t m_default_interval_msec = 100;

	void onTimout() { 
		if (this->interval() != m_default_interval_msec)
			this->setInterval(m_default_interval_msec); 
	}

public:
	Timer(int default_interval_msec, QWidget* parent = nullptr) : m_default_interval_msec(default_interval_msec), QTimer(parent) {
		this->setSingleShot(true);
		this->setInterval(default_interval_msec);
		connect(this, &QTimer::timeout, this, &Timer::onTimout);
	}

	Timer() = default;

	uint32_t defaultInterval()const { return m_default_interval_msec; }

	void setDefaultInterval(uint32_t msec) { this->setInterval(m_default_interval_msec = msec); }
};



class ProgressSignal: public QObject {
	Q_OBJECT

public:
signals:
	void emitProgress(int val);

	void emitText(const QString& text);

	void finished();
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

	ProgressDialog(ProgressSignal* signal_object, QWidget* parent = nullptr);
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




class PreviewWindowBase;
class ProcessDialog : public Dialog {
	Q_OBJECT

private:
	DialogToolbar* m_toolbar = nullptr;
	std::unique_ptr<Timer> m_timer = std::make_unique<Timer>(100, this);

	bool m_finished = true;

protected:
	QString m_name;
	Workspace* m_workspace;

	PreviewWindowBase* m_preview = nullptr;

public:
	ProcessDialog(const QString& name, const QSize& size, Workspace* parent_workspace, bool preview = true, bool apply_dd = true, bool apply = true);

	QString name()const { return m_name; }

	void enableSiblings_Subwindows(bool enable);

	void resizeDialog(const QSize& size);

	bool isPreviewValid()const;

signals:
	void processDropped();

	void previewAdded();

	void previewRemoved();

private:
	void removePreview();

	void createDragInstance();

	void connectZoomWindow();

	void onZoomWindowCreated();

protected:
	void applytoPreview();

	void startTimer()const { m_timer->start(); }

	void setDefaultTimerInterval(int msec) {
		m_timer->setDefaultInterval(msec);
	}

	void showPreviewWindow(bool ignore_zoomwindow = false);

	virtual void onImageWindowCreated() {}

	virtual void onImageWindowClosed() {}

	virtual void closeEvent(QCloseEvent* close);

	virtual void resetDialog() {}

	void showPreview() { showPreviewWindow(); }

	virtual void apply() {}

	virtual void applyPreview() {};
};

