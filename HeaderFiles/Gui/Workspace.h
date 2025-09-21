#pragma once
#include "Image.h"
#include "ImageSubWindow.h"

class Workspace:public QMdiArea {
    Q_OBJECT
private:
    QUndoStack* m_process_stack;

    int m_offsetx = 0;
    int m_offsety = 0;

public:
    Workspace(QWidget* parent = nullptr);

    ImageSubWindow* addImageSubWindow(ImageSubWindow* isw, Qt::WindowFlags windowFlags = Qt::WindowFlags());

    void removeImageSubWindow(ImageSubWindow* subwindow);

    void enableChildren(bool enable);
signals:
    void imageWindowClosed();

    void imageWindowCreated();

    void imageActivated(const Image8* img);

public:
    void dragEnterEvent(QDragEnterEvent* event);

    void dropEvent(QDropEvent* event);

    void mouseDoubleClickEvent(QMouseEvent* event);
};

static Workspace* workspaceRecast(QMdiArea* workspace) { return dynamic_cast<Workspace*>(workspace); }