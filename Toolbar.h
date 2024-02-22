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
	Toolbar(QWidget* parent) : QWidget(parent) {
		this->setGeometry(0, parent->size().height() - dy, parent->size().width(), dx);

		m_apply = new QPushButton(this);
		m_apply->setAutoDefault(false);
		m_apply->setToolTip("Apply");
		m_apply->move(0, 0);
		m_apply->resize(dx, dy);
		m_apply->setText("A");

		m_preview = new QPushButton(this);
		m_preview->setAutoDefault(false);
		m_preview->setToolTip("Show Preview");
		m_preview->move(dx, 0);
		m_preview->resize(dx, dy);
		m_preview->setText("P");

		m_reset = new QPushButton(this);
		m_reset->setAutoDefault(false);
		m_reset->setToolTip("Reset");
		m_reset->move(dx * 2, 0);
		m_reset->resize(dx, dy);
		m_reset->setText("R");

		this->setStyleSheet("QToolTip {border : 0px solid dark gray; background: solid dark gray; color: white}");

		this->setAutoFillBackground(true);
		QPalette pal = QPalette();
		pal.setColor(QPalette::Window, Qt::lightGray);
		this->setPalette(pal);

		connect(m_apply, &QPushButton::pressed, this, &Toolbar::sendApply);
		connect(m_preview, &QPushButton::pressed, this, &Toolbar::sendPreview);
		connect(m_reset, &QPushButton::pressed, this, &Toolbar::sendReset);
	}

signals:
	void sendApply();

	void sendPreview();

	void sendReset();
};
