#pragma once
#include <qwidget.h>

class Toolbar : public QWidget {
	Q_OBJECT

	int dx = 24;
	int dy = 24;

	QPushButton* m_apply;
	QIcon m_apply_icon;

	QPushButton* m_preview;
	QIcon m_preview_icon;

	QPushButton* m_reset;
	QIcon m_reset_icon;

public:
	Toolbar(QWidget* parent);

signals:
	void sendApply();

	void sendPreview();

	void sendReset();
};

class Timer : public QTimer {
	Q_OBJECT

public:
	Timer(int interval, QWidget* parent) : QTimer(parent) {
		this->setSingleShot(true);
		this->setInterval(interval);
	}
};

class ProcessDialog : public QDialog {
	Q_OBJECT

protected:
	QString m_name;
	QMdiArea* m_workspace;
	Toolbar* m_tb;
	Timer* m_timer;

public:
	ProcessDialog(QString name, QWidget*parent): QDialog(parent), m_name(name) {}

signals:
	void onClose();

protected:

	void closeEvent(QCloseEvent* close);

	void setWorkspace(QMdiArea* workspace) { m_workspace = workspace; }

	void setToolbar(Toolbar* toolbar) { m_tb = toolbar; }

	virtual void showPreview();

	QString Name()const { return m_name; }
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

	DoubleLineEdit(const QString& contents,DoubleValidator* validator, QWidget* parent = nullptr);

	const DoubleValidator* Validator()const { return reinterpret_cast<const DoubleValidator*>(this->validator()); }

	void removeEndDecimal();

	void addTrailingZeros();
};

class ComponentPushButton : public QPushButton {
	Q_OBJECT
public:

	ComponentPushButton(const QString& text, QWidget* parent = nullptr) : QPushButton(text, parent) {

		this->setAutoDefault(false);
		this->setCheckable(true);
		this->setAutoExclusive(true);
		QFont font = this->font();
		font.setPointSize(8);
		this->setFont(font);
	}

	ComponentPushButton(const QIcon& icon, const QString& text, QWidget* parent = nullptr) : QPushButton(icon, text, parent) {

		this->setAutoDefault(false);
		this->setCheckable(true);
		this->setAutoExclusive(true);
		QFont font = this->font();
		font.setPointSize(8);
		this->setFont(font);
	}
};