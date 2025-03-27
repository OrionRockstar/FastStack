#include "pch.h"
#include "Workspace.h"
#include "ImageFileReader.h"
#include "FastStackToolBar.h"


Workspace::Workspace(QWidget* parent) : QMdiArea(parent) {
    this->setAcceptDrops(true);
    m_process_stack = new QUndoStack(this);

    this->setActivationOrder(QMdiArea::StackingOrder);
    this->setBackground(QColor(96,96,96));
}

void Workspace::UpdateOffsets() {
    m_offsetx += 10;
    m_offsety += 10;

    if (m_offsetx > size().width() * 0.5)
        m_offsetx = 0;

    if (m_offsety > size().height() * 0.5)
        m_offsety = 0;
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
