#pragma once

//#include <QtWidgets/QMainWindow>
#include "pch.h"
#include "ui_FastStack.h"
#include "Image.h"
#include "MenuBar.h"
#include "ImageWindow.h"
#include <QMdiArea>
#include <qdockwidget.h>

class Workspace : public QMdiArea {
    Q_OBJECT

public:
    //QDialog* rtp = nullptr;

    int m_offsetx = 0;
    int m_offsety = 0;

    Workspace(QWidget* parent = nullptr) : QMdiArea(parent) {}

    void UpdateOffsets() {
        m_offsetx += 20;
        m_offsety += 20;

        if (m_offsetx > size().width() * 0.75)
            m_offsetx = 0;

        if (m_offsety > size().height() * 0.75)
            m_offsety = 0;
    }
public slots:
    void receiveClose() {
        sendClose();
    }

    void receiveOpen() { 
        sendOpen(); 
    }

signals:
    void sendClose();

    void sendOpen();
};

class FastStack : public QMainWindow
{
    Q_OBJECT

public:
    FastStack(QWidget* parent = Q_NULLPTR);

    Workspace* workspace;
    //QMdiSubWindow* sw;
    //Image32 img;
    MenuBar* m_menubar;

private:
    Ui::FastStackClass ui;

};
