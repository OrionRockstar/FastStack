#pragma once

//#include <QtWidgets/QMainWindow>
#include "pch.h"
#include "ui_FastStack.h"
#include "Image.h"
#include "MenuBar.h"
#include "ImageWindow.h"
#include <QMdiArea>
#include <qdockwidget.h>
#include "Workspace.h"

class FastStack : public QMainWindow {
    Q_OBJECT

public:
    FastStack(QWidget* parent = Q_NULLPTR);

    Workspace* m_workspace;

    MenuBar* m_menubar;

    Workspace* workspace() { return m_workspace; }

private:
    Ui::FastStackClass ui;

};
