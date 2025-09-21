#include "pch.h"
#include "ImageSubWindow.h"


ImageSubWindow::ImageSubWindow(QWidget* widget) : QMdiSubWindow() {

    this->setWidget(widget);
    //this->setMinimumSize(200, 200);
    this->setWindowFlags(Qt::SubWindow | Qt::WindowShadeButtonHint);

    this->setAutoFillBackground(false);

    QPalette pal;
    pal.setColor(QPalette::Inactive, QPalette::Text, Qt::white);
    pal.setColor(QPalette::Highlight, QColor(39, 39, 39));
    pal.setColor(QPalette::Inactive, QPalette::Window, QColor(139, 139, 139));
    pal.setColor(QPalette::Light, QColor(69, 0, 128, 169));
    this->setPalette(pal);
    this->setStyle(new OpaqueStyle(style()));

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &ImageSubWindow::action);

}

void ImageSubWindow::resizeToFit(int w, int h) {

    int fw = style()->pixelMetric(QStyle::PM_MdiSubWindowFrameWidth);
    int tbh = style()->pixelMetric(QStyle::PM_TitleBarHeight);

    this->resize(w + (2 * fw), h + (tbh + fw));
}

void ImageSubWindow::action() {
    emit actionTriggered();
    opaqueStyle()->setOpacity(0.55);
    update();
}

bool ImageSubWindow::event(QEvent* e) {

    if (e->type() == QEvent::HideToParent)
        this->resize(400, style()->pixelMetric(QStyle::PM_TitleBarHeight));

    return QMdiSubWindow::event(e);
}

void ImageSubWindow::mousePressEvent(QMouseEvent* e) {

    QMdiSubWindow::mousePressEvent(e);

    if (e->buttons() == Qt::LeftButton)
        m_timer->start(1'000);
}

void ImageSubWindow::mouseMoveEvent(QMouseEvent* e) {

    QMdiSubWindow::mouseMoveEvent(e);

    if (e->buttons() == Qt::LeftButton && m_timer->id() != Qt::TimerId::Invalid) {
        m_timer->stop();
        action();
    }
}

void ImageSubWindow::mouseReleaseEvent(QMouseEvent* e) {

    QMdiSubWindow::mouseReleaseEvent(e);

    if (e->button() == Qt::LeftButton) {
        if (m_timer->id() != Qt::TimerId::Invalid)
            m_timer->stop();
        else {
            emit actionFinished();
            opaqueStyle()->setOpacity(1.0);
            update();
        }
    }
}

void ImageSubWindow::paintEvent(QPaintEvent* e) {

    QPainter p(this);
    QColor c = palette().color(QPalette::Window);
    c.setAlpha(opaqueStyle()->opacity() * 255);
    p.fillRect(rect(), c);

    QPalette pal(palette());

    if (this == mdiArea()->currentSubWindow() && windowState() == Qt::WindowActive)
        pal.setColor(QPalette::Window, QColor(39, 39, 39));
    else
        pal.setColor(QPalette::Window, QColor(139, 139, 139));

    if (isEnabled())
        pal.setColor(QPalette::Highlight, QColor(69, 0, 128));

    else {
        pal.setColor(QPalette::Window, QColor(139, 139, 139));
        pal.setColor(QPalette::Active, QPalette::Highlight, QColor(0, 0, 69));
    }
    this->setPalette(pal);

    QMdiSubWindow::paintEvent(e);
}
