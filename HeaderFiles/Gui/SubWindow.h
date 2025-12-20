#pragma once
#include "CustomWidgets.h"

class SubWindow : public QMdiSubWindow {
	Q_OBJECT

private:
	QTimer* m_timer = nullptr;

public:
	SubWindow(QWidget* widget);

	void resizeToFit(int w, int h);

signals:
	void actionTriggered();

	void actionFinished();

private:
	void action();

	OpaqueStyle* opaqueStyle()const { return dynamic_cast<OpaqueStyle*>(style()); }

	bool event(QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void paintEvent(QPaintEvent* e);
};

