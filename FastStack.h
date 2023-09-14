#pragma once

//#include <QtWidgets/QMainWindow>
#include "pch.h"
#include "ui_FastStack.h"
#include "Image.h"
#include "MenuBar.h"

class FastStack : public QMainWindow
{
    Q_OBJECT

public:
    FastStack(QWidget* parent = Q_NULLPTR);

    Image32 img;
    MenuBar* m_menubar;

private:
    Ui::FastStackClass ui;
};
