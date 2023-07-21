#pragma once

//#include <QtWidgets/QMainWindow>
#include<QtWidgets>
#include "ui_FastStack.h"

class FastStack : public QMainWindow
{
    Q_OBJECT

public:
    FastStack(QWidget *parent = Q_NULLPTR);

private:
    Ui::FastStackClass ui;
};
