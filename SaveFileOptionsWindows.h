#pragma once
#include "pch.h"

class FITSWindow : public QDialog {
    Q_OBJECT

        QDialog* window;
    QVBoxLayout* layout;

    QRadioButton* bd8;
    QRadioButton* bd16;
    QRadioButton* bd32;
    QPushButton* save;

    int new_bitdepth = 8;

public:
    FITSWindow(int bitdepth) : new_bitdepth(bitdepth) {
        window = new QDialog;
        layout = new QVBoxLayout;

        //window->setWindowIcon

        window->resize(200, 100);
        window->setWindowTitle("FITS Save Options");

        bd8 = new QRadioButton;
        bd8->setText("8-bit unsigned int");
        layout->addWidget(bd8);

        bd16 = new QRadioButton;
        bd16->setText("16-bit unsigned int");
        layout->addWidget(bd16);

        bd32 = new QRadioButton;
        bd32->setText("32-bit floating point");
        layout->addWidget(bd32);

        switch (bitdepth) {
        case 8:
            bd8->setChecked(true);
            break;
        case 16:
            bd16->setChecked(true);
            break;
        case -32:
            bd32->setChecked(true);
            break;
        }

        save = new QPushButton;
        save->setText("Save");
        layout->addWidget(save);

        connect(bd8, &QRadioButton::toggled, this, &FITSWindow::setBitdepth8);
        connect(bd16, &QRadioButton::toggled, this, &FITSWindow::setBitdepth16);
        connect(bd32, &QRadioButton::toggled, this, &FITSWindow::setBitdepth32);

        connect(save, &QPushButton::pressed, this, &FITSWindow::saveImage);
        connect(window, &QDialog::finished, this, &FITSWindow::done);

        window->setLayout(layout);
    }

private slots:
    void setBitdepth8() { new_bitdepth = 8; }

    void setBitdepth16() { new_bitdepth = 16; }

    void setBitdepth32() { new_bitdepth = -32; }

    void saveImage() {
        window->accept();
    }

    void done(int r) {
        delete this;
    }

signals:

    void setBitdepth();

public:

    virtual int exec() { return window->exec(); }

    int getNewBitdepth() {
        return new_bitdepth;
    }

};


class TIFFWindow : public QDialog {
    Q_OBJECT

        QDialog* window;
    QVBoxLayout* layout;

    QRadioButton* bd8;
    QRadioButton* bd16;
    QRadioButton* bd32;
    QCheckBox* planar;
    QPushButton* save;

    int new_bitdepth = 8;
    bool planar_contig = true;

public:
    TIFFWindow(int bitdepth) : new_bitdepth(bitdepth) {
        window = new QDialog;
        layout = new QVBoxLayout;

        //window->setWindowIcon

        window->resize(200, 100);
        window->setWindowTitle("FITS Save Options");

        bd8 = new QRadioButton;
        bd8->setText("8-bit unsigned int");
        layout->addWidget(bd8);

        bd16 = new QRadioButton;
        bd16->setText("16-bit unsigned int");
        layout->addWidget(bd16);

        bd32 = new QRadioButton;
        bd32->setText("32-bit floating point");
        layout->addWidget(bd32);

        planar = new QCheckBox;
        planar->setText("Planar Contiguous");
        layout->addWidget(planar);
        planar->setCheckState(Qt::CheckState::Checked);

        switch (bitdepth) {
        case 8:
            bd8->setChecked(true);
            break;
        case 16:
            bd16->setChecked(true);
            break;
        case -32:
            bd32->setChecked(true);
            break;
        }

        save = new QPushButton;
        save->setText("Save");
        layout->addWidget(save);

        connect(bd8, &QRadioButton::toggled, this, &TIFFWindow::setBitdepth8);
        connect(bd16, &QRadioButton::toggled, this, &TIFFWindow::setBitdepth16);
        connect(bd32, &QRadioButton::toggled, this, &TIFFWindow::setBitdepth32);
        connect(planar, &QCheckBox::stateChanged, this, &TIFFWindow::setPlanar);

        connect(save, &QPushButton::pressed, this, &TIFFWindow::saveImage);
        connect(window, &QDialog::finished, this, &TIFFWindow::done);

        window->setLayout(layout);
    }

private slots:
    void setBitdepth8() { new_bitdepth = 8; }

    void setBitdepth16() { new_bitdepth = 16; }

    void setBitdepth32() { new_bitdepth = -32; }

    void setPlanar(int state) {
        if (state == Qt::CheckState::Checked)
            planar_contig = true;
        else
            planar_contig = false;
    }

    void saveImage() {
        window->accept();
    }

    void done(int r) {
        delete this;
    }

signals:

    void setBitdepth();

public:

    virtual int exec() { return window->exec(); }

    int getNewBitdepth() {
        return new_bitdepth;
    }

    bool getPlanarContig() { return planar_contig; }

};
