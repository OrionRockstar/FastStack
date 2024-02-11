#pragma once
#include "pch.h"
#include "Image.h"
#include "ui_MenuBar.h"
#include "ImageWindow.h"

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
public:
    ImageTransformations(QWidget* parent) : QMenu(parent) {
        this->setTitle("Image Transformations");
        this->addAction(tr("&Historgram TransFormation"), this, &ImageTransformations::HistogramTransformation);
        this->addAction(tr("&ArcSinH Stretch"), this, &ImageTransformations::ArcSinhStretch);
        this->addAction(tr("&Curves Transformation"), this, &ImageTransformations::CurvesTransformation);
        this->addAction(tr("&Local Histogram Equalization"), this, &ImageTransformations::LocalHistogramEqualization);
    }

private:
    void HistogramTransformation() {
        HistogramTransformationWidget* htw = new HistogramTransformationWidget(this);
    }

    void ArcSinhStretch() {
    }

    void CurvesTransformation() {}

    void LocalHistogramEqualization() {}

};

class ProcessMenu : public QMenu {
    //QWidget* m_parent;

    QMenu* m_image_trans;
    QMenu* morphology;
    QMenu* m_abg_extraction;

    void CreateProcessMenu() {

        m_abg_extraction = new BackgroundExtraction(this);
        this->addMenu(m_abg_extraction);

        m_image_trans = new ImageTransformations(this);
        this->addMenu(m_image_trans);

        morphology = new Morphology(this);
        this->addMenu(morphology);
    }

public:

    ProcessMenu(QWidget* parent) : QMenu(parent) {
        this->setTitle("Process");
        CreateProcessMenu();
    }
};

class MenuBar: public QMenuBar
{
    Q_OBJECT

    //Ui::MenuBarClass ui;

public:
	MenuBar(QWidget*parent);
	~MenuBar() {}

    //FastStack* fsp;
    QWidget* m_parent;

	QMenu* filemenu;// = addMenu(tr("&File"));
	QAction* open;
    QAction* save;
    QAction* save_as;

    QMenu* m_process;
    //QMenu* processmenu;
    //QMenu* stretch_image;
    //QAction* histogram_transformation;

    uint32_t image_counter = 0;

    QString m_typelist =
        "FITS file(*.fits *.fts *.fit);;"
        "XISF file(*.xisf);;"
        "TIFF file(*.tiff *tif)";

    void HT();

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
