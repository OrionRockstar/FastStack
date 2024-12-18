#pragma once
#include "pch.h"
#include "Image.h"

class FITSWindow : public QDialog {
    Q_OBJECT

        //QDialog* window;
    QVBoxLayout* layout;

    QRadioButton* bd8;
    QRadioButton* bd16;
    QRadioButton* bd32;
    QPushButton* save;

    ImageType m_type =ImageType::UBYTE;

public:
    FITSWindow(ImageType type, QWidget* parent) : m_type(type), QDialog(parent) {
        //window = new QDialog(this);
        layout = new QVBoxLayout(this);
        //window->setWindowIcon

        this->resize(200, 100);
        this->setWindowTitle("FITS Save Options");

        bd8 = new QRadioButton(this);
        bd8->setText("8-bit unsigned int");
        layout->addWidget(bd8);

        bd16 = new QRadioButton(this);
        bd16->setText("16-bit unsigned int");
        layout->addWidget(bd16);

        bd32 = new QRadioButton(this);
        bd32->setText("32-bit floating point");
        layout->addWidget(bd32);

        switch (type) {
        case ImageType::UBYTE:
            bd8->setChecked(true);
            break;
        case ImageType::USHORT:
            bd16->setChecked(true);
            break;
        case ImageType::FLOAT:
            bd32->setChecked(true);
            break;
        }

        save = new QPushButton(this);
        save->setText("Save");
        layout->addWidget(save);

        connect(bd8, &QRadioButton::toggled, this, [this]() { m_type = ImageType::UBYTE; });
        connect(bd16, &QRadioButton::toggled, this, [this]() { m_type = ImageType::USHORT; });
        connect(bd32, &QRadioButton::toggled, this, [this]() { m_type = ImageType::FLOAT; });

        connect(save, &QPushButton::pressed, this, &FITSWindow::saveImage);

        this->setLayout(layout);
        this->setAttribute(Qt::WA_DeleteOnClose);
        this->show();
    }

    void saveImage() {
        this->accept();
    }

public:

    ImageType imageType()const { return m_type; }

};


class TIFFWindow : public QDialog {
    Q_OBJECT

    QVBoxLayout* layout;

    QRadioButton* bd8;
    QRadioButton* bd16;
    QRadioButton* bd32;
    QCheckBox* planar;
    QPushButton* save;

    ImageType m_type = ImageType::UBYTE;
    bool planar_contig = true;

public:
    TIFFWindow(ImageType type, QWidget* parent) : m_type(type), QDialog(this)  {
        layout = new QVBoxLayout;

        this->resize(200, 100);
        this->setWindowTitle("FITS Save Options");

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

        switch (m_type) {
        case ImageType::UBYTE:
            bd8->setChecked(true);
            break;
        case ImageType::USHORT:
            bd16->setChecked(true);
            break;
        case ImageType::FLOAT:
            bd32->setChecked(true);
            break;
        }

        save = new QPushButton;
        save->setText("Save");
        layout->addWidget(save);

        connect(bd8, &QRadioButton::toggled, this, [this]() {m_type = ImageType::UBYTE; });
        connect(bd16, &QRadioButton::toggled, this, [this]() {m_type = ImageType::UBYTE; });
        connect(bd32, &QRadioButton::toggled, this, [this]() {m_type = ImageType::UBYTE; });
        connect(planar, &QCheckBox::clicked, this, [this](bool v) { planar_contig = v; });
        connect(save, &QPushButton::pressed, this, [this]() { this->accept(); });

        this->setLayout(layout);
        this->setAttribute(Qt::WA_DeleteOnClose);
    }

    ImageType imageType() { return m_type; }

    bool planarContig() { return planar_contig; }

};
