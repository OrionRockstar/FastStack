#pragma once
#include "pch.h"
#include "Image.h"
#include "ui_MenuBar.h"
#include "ImageWindow.h"

#include"ASinhStretch.h"
#include"CurvesTransformation.h"
#include"LocalHistogramEqualization.h"

class BackgroundExtraction : public QMenu {
public:
    BackgroundExtraction(QWidget* parent) : QMenu(parent) {
        this->setTitle("Background Extraction");
        this->addAction(tr("&Automatic Background Extraction"), this, &BackgroundExtraction::AutoBackgroundExtraction);
    }
private:
    void AutoBackgroundExtraction() {}
};

class Morphology : public QMenu {
public:
    Morphology(QWidget* parent) : QMenu(parent) {
        this->setTitle(tr("&Morphology"));
        this->addAction(tr("Morphological Transformations"), this, &Morphology::MorphologicalTransformations);
    }
private:
    void MorphologicalTransformations() {}

    void GaussianBlur() {}
};

class ImageTransformations : public QMenu {
    Q_OBJECT

public:
    ImageTransformations(QWidget* parent) : QMenu(parent) {
        this->setTitle("Image Transformations");
        this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
        this->addAction(tr("&Historgram TransFormation"), this, &ImageTransformations::HistogramTransformation);
        this->addAction(tr("&ArcSinH Stretch"), this, &ImageTransformations::ArcSinhStretch);
        this->addAction(tr("&Curves Transformation"), this, &ImageTransformations::CurvesTransformation);
        this->addAction(tr("&Local Histogram Equalization"), this, &ImageTransformations::LocalHistogramEqualization);
    }

private slots:
    void onHistogramTransformationClose() {
        htw = nullptr;
    }

    void onASinhStretchDialogClose() {
        m_ashd = nullptr;
    }

    void onLocalHistogramEqualizationDialogClose() {
        m_lhed = nullptr;
    }

    void onCurveTransformationDialogClose() {
        m_ctd = nullptr;
    }

private:
    HistogramTransformationDialog* htw = nullptr;
    ASinhStretchDialog* m_ashd = nullptr;
    LocalHistogramEqualizationDialog* m_lhed = nullptr;
    CurveTransformDialog* m_ctd = nullptr;

    void HistogramTransformation() {
        if (htw == nullptr) {
            htw = new HistogramTransformationDialog(parentWidget());
            connect(htw, &ProcessDialog::onClose, this, &ImageTransformations::onHistogramTransformationClose);
        }
    }

    void ArcSinhStretch() {
        if (m_ashd == nullptr) {
            m_ashd = new ASinhStretchDialog(parentWidget());
            connect(m_ashd, &ProcessDialog::onClose, this, &ImageTransformations::onASinhStretchDialogClose);
        }
    }

    void CurvesTransformation() {
        if (m_ctd == nullptr) {
            m_ctd = new CurveTransformDialog(parentWidget());
            connect(m_ctd, &ProcessDialog::onClose, this, &ImageTransformations::onCurveTransformationDialogClose);
        }
    }

    void LocalHistogramEqualization() {
        if (m_lhed == nullptr) {
            m_lhed = new LocalHistogramEqualizationDialog(parentWidget());
            connect(m_lhed, &LocalHistogramEqualizationDialog::onClose, this, &ImageTransformations::onLocalHistogramEqualizationDialogClose);
        }
    }

};

class ProcessMenu : public QMenu {

    QMenu* m_image_trans;
    QMenu* morphology;
    QMenu* m_abg_extraction;

public:

    ProcessMenu(QWidget* parent) : QMenu(parent) {
        this->setTitle("Process");
        CreateProcessMenu();
    }

    void CreateProcessMenu() {

        m_abg_extraction = new BackgroundExtraction(this);
        this->addMenu(m_abg_extraction);

        m_image_trans = new ImageTransformations(parentWidget());
        this->addMenu(m_image_trans);

        morphology = new Morphology(this);
        this->addMenu(morphology);
    }
};

class MenuBar: public QMenuBar {
    Q_OBJECT
    //Ui::MenuBarClass ui;
public:
    MenuBar(QWidget* parent);
	~MenuBar() {}

    QMainWindow* m_parent;

	QMenu* filemenu;// = addMenu(tr("&File"));
	QAction* open;
    QAction* save;
    QAction* save_as;

    QMenu* m_process;

    uint32_t image_counter = 0;

    QString m_typelist =
        "FITS file(*.fits *.fts *.fit);;"
        "XISF file(*.xisf);;"
        "TIFF file(*.tiff *tif)";

    void Open();

    void Save();

    void SaveAs();

    void AddFileMenu();

    void AddProcessMenu();

	void AddAction();

public slots:

    void onWindowClose();

    void onWindowOpen();

signals:
    void initiateSave();
};
