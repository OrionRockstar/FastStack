#include "pch.h"
#include "Workspace.h"
#include "ImageFileReader.h"
#include "FastStackToolBar.h"
#include "ImageWindow.h"

Workspace::Workspace(QWidget* parent) : QMdiArea(parent) {
    this->setAcceptDrops(true);
    m_process_stack = new QUndoStack(this);

    connect(this, &QMdiArea::subWindowActivated, this, [this](QMdiSubWindow* subwindow) { if (subwindow) emit imageActivated(&imageRecast(subwindow->widget())->source()); });

    this->installEventFilter(this);

    this->setActivationOrder(QMdiArea::StackingOrder);
    this->setBackground(QColor(96,96,96));
}

SubWindow* Workspace::addSubWindow(SubWindow* isw, Qt::WindowFlags windowFlags) {

    auto sw = QMdiArea::addSubWindow(isw, windowFlags);
    emit imageWindowCreated();
    sw->move(m_offsetx, m_offsety);
    m_offsetx += 10;
    m_offsety += 10;

    if (m_offsetx > size().width() * 0.5)
        m_offsetx = 0;

    if (m_offsety > size().height() * 0.5)
        m_offsety = 0;
    
    return dynamic_cast<SubWindow*>(sw);
}

void Workspace::removeSubWindow(SubWindow* subwindow) {

    emit imageWindowClosed();
    QMdiArea::removeSubWindow(subwindow);

    if (!this->currentSubWindow())
        emit imageActivated(nullptr);
}

void Workspace::enableChildren(bool enable) {

    for (auto c : children())
        if (dynamic_cast<QWidget*>(c))
            reinterpret_cast<QWidget*>(c)->setEnabled(enable);
}

void Workspace::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Workspace::dropEvent(QDropEvent* event) {
    for (auto url : event->mimeData()->urls()) {
        std::filesystem::path path = url.toLocalFile().toStdString();

        Status status = ImageFileReader(this).read(path);
        if (!status)
            QMessageBox().information(this, "", status.m_message);
    }
}

void Workspace::mouseDoubleClickEvent(QMouseEvent* event) {

    std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], ImageFileReader::typelist()).toStdString();

    if (f_path == "")
        return;

    ImageFileReader(this).read(f_path);
}

