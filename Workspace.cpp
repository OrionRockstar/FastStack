#include "pch.h"
#include "Workspace.h"
#include "ImageFileReader.h"

Workspace::Workspace(QWidget* parent) : QMdiArea(parent) {
    this->setAcceptDrops(true);
}

void Workspace::UpdateOffsets() {
    m_offsetx += 20;
    m_offsety += 20;

    if (m_offsetx > size().width() * 0.75)
        m_offsetx = 0;

    if (m_offsety > size().height() * 0.75)
        m_offsety = 0;
}

void Workspace::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void Workspace::dropEvent(QDropEvent* event) {
    for (auto url : event->mimeData()->urls()) {
        std::filesystem::path path = url.toLocalFile().toStdString();

        Status status = ImageFileReader(this).Read(path);
        if (!status)
            QMessageBox().information(this, "", status.m_message);

    }

}

void Workspace::mouseDoubleClickEvent(QMouseEvent* event) {
    std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist).toStdString();

    if (f_path == "")
        return;

    ImageFileReader(this).Read(f_path);
}