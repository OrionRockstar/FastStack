#pragma once
#include "Image.h"
#include "SubWindow.h"


class Workspace:public QMdiArea {
    Q_OBJECT
private:
    QUndoStack* m_process_stack;

    int m_offsetx = 0;
    int m_offsety = 0;

    using QMdiArea::addSubWindow;
    using QMdiArea::currentSubWindow;
    using QMdiArea::removeSubWindow;

public:
    Workspace(QWidget* parent = nullptr);

    SubWindow* addSubWindow(SubWindow* isw, Qt::WindowFlags windowFlags = Qt::WindowFlags());

    void removeSubWindow(SubWindow* subwindow);

    void enableChildren(bool enable);

    bool hasSubWindows()const { return !this->subWindowList().isEmpty(); }

    SubWindow* currentSubWindow()const { return dynamic_cast<SubWindow*>(QMdiArea::currentSubWindow()); }

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