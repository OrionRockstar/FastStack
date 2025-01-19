#pragma once
#include <qwidget.h>
#include "Star.h"
#include "Matrix.h"
#include "Workspace.h"
#include "CustomWidgets.h"
//#include "ImageWindow.h"


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
	void connectFunctions(const QObject* receiver,Func1 apply_to_dd, Func2 apply, Func2 preview, Func2 reset) {

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
	Timer(int interval_ms, QWidget* parent) : QTimer(parent) {
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



class ProcessDialog : public QDialog {
	Q_OBJECT

	Toolbar* m_toolbar = nullptr;
	Timer* m_timer;

	const QObject* m_obj = nullptr;
protected:
	typedef void (ProcessDialog::* Function)();
private:
	Function m_func = nullptr;

protected:
	QString m_name;
	QMdiArea* m_workspace;

	QDialog* m_preview = nullptr;

	inline static QString m_crop_image_str = "Crop Image";
	inline static QString m_abe_str = "Automatic Background Extraction";

public:
	ProcessDialog(QString name, const QSize& size, QMdiArea* parent_workspace, bool preview = true, bool apply_dd = true, bool apply = true);

signals:
	void windowClosed();

	void processDropped();

private:
	void createDragInstance();

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

	template <typename Func>
	void connectToolbar(const QObject* receiver, Func apply, Func preview, Func reset) {
		connect(this, &ProcessDialog::processDropped, receiver, apply);
		m_toolbar->connectFunctions(receiver, &ProcessDialog::createDragInstance, apply, preview, reset);
	}

	void previewClosed() { m_preview = nullptr; }

	virtual void showPreview();

	void showPreview_zoomWindowIgnored();
private:
	void transferPreview(QWidget* image_window);

protected:
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


private:
	template<typename>
	void czw()const {
		auto ptr = imageRecast<>(m_workspace->currentSubWindow()->widget());
		ptr->zoomWindow()->connectZoomWindow(m_obj, m_func);
	}

public:
	template<typename Function>
	void connectZoomWindow(const QObject* obj, Function func) {

		m_obj = obj;
		m_func = reinterpret_cast<void(ProcessDialog::*)()>(func);

		auto ws = dynamic_cast<Workspace*>(m_workspace);

		if (ws->subWindowList().isEmpty())
			return;

		auto ptr = imageRecast<>(m_workspace->currentSubWindow()->widget());
		if (ptr->zoomWindow()) 
			ptr->zoomWindow()->connectZoomWindow(m_obj, m_func);

		connect( ws, &Workspace::zoomWindowCreated, this, &ProcessDialog::czw<Function>);
		connect( ws, &Workspace::zoomWindowClosed, m_obj, m_func);
	}

};
