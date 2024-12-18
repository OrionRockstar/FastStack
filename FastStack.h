#pragma once
//#include <QtWidgets/QMainWindow>
#include "pch.h"
#include "ui_FastStack.h"
#include "MenuBar.h"
#include <QMdiArea>
#include "Workspace.h"
#include "FastStackToolBar.h"


//#define FASTSTACK(ptr) *reinterpret_cast<FastStack*>(ptr)
//have 2 workspaces, one for image & one for 
class FastStack : public QMainWindow {
    Q_OBJECT

    Workspace* m_workspace;
    MenuBar* m_menubar;

    FastStackToolBar* m_toolbar;

public:
    FastStack(QWidget* parent = Q_NULLPTR);

    Workspace* workspace()const { return m_workspace; }

    FastStackToolBar* toolbar()const { return m_toolbar; }

    inline static FastStack* recast(QWidget* ptr) {
        return reinterpret_cast<FastStack*>(ptr);
    }

private:
    Ui::FastStackClass ui;

};
