#pragma once

class Workspace:public QMdiArea {
    Q_OBJECT
private:
    QString m_typelist =
        "All Accepted Formats(*.bmp *.fits *.fts *.fit *.tiff *.tif);;"
        "BMP file(*.bmp);;"
        "FITS file(*.fits *.fts *.fit);;"
        "XISF file(*.xisf);;"
        "TIFF file(*.tiff *.tif)";
public:
    //QDialog* rtp = nullptr;

    int m_offsetx = 0;
    int m_offsety = 0;

    Workspace(QWidget* parent = nullptr);

    void UpdateOffsets();

    //const int Offset_X()const { return m_offsetx; }

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

public:
    void dragEnterEvent(QDragEnterEvent* event);

    void dropEvent(QDropEvent* event);

    void mouseDoubleClickEvent(QMouseEvent* event);
};

