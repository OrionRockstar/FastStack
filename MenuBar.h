#pragma once
#include "pch.h"
#include "Image.h"
#include "ui_MenuBar.h"
#include "ImageWindow.h"

#include"HistogramTransformation.h"
#include"AutoHistogram.h"
#include"ASinhStretch.h"
#include"CurvesTransformation.h"
#include"LocalHistogramEqualization.h"
#include "AdaptiveStretch.h"
#include "MorphologicalTransformation.h"
#include "ImageStackingDialog.h"
#include "ImageGeometryDialogs.h"
#include "GaussianFilter.h"
#include "BilateralFilter.h"
#include "Sobel.h"
#include "Masks.h"


class BackgroundExtraction : public QMenu {
public:
    BackgroundExtraction(QWidget* parent) : QMenu(parent) {
        this->setTitle("Background Extraction");
        this->addAction(tr("&Automatic Background Extraction"), this, &BackgroundExtraction::AutoBackgroundExtraction);
    }
private:
    void AutoBackgroundExtraction() {}
};

class ImageGeometryMenu : public QMenu {

    RotationDialog* m_rd = nullptr;
    FastRotationDialog* m_frd = nullptr;
    HomographyTransformationDialog* m_htd = nullptr;
    std::unique_ptr<IntegerResampleDialog> m_irs;

public:
    ImageGeometryMenu(QWidget* parent);

private:
    void RotationSelection();

    void FastRotationSelection();

    void IntegerResampleSelection();
};

class MaskMenu : public QMenu {
    RangeMaskDialog* m_rmd = nullptr;

public:
    MaskMenu(QWidget* parent);

private:
    void RangeMaskSelection();
};

class MorphologyMenu : public QMenu {
    MorphologicalTransformationDialog* m_mtd = nullptr;
    BilateralFilterDialog* m_bfd = nullptr;
    GaussianFilterDialog* m_gfd = nullptr;
    std::unique_ptr<SobelDialog> m_sd;

public:
    MorphologyMenu(QWidget* parent);

private:
    void MorphologicalTransformationsSelection();

    void GaussianBlurSelection();

    void BilateralFilterSelection();

    void SobelSelection();
};

class ImageTransformationsMenu : public QMenu {
    Q_OBJECT

    AdaptiveStretchDialog* m_asd = nullptr;
    AutoHistogramDialog* m_ahd = nullptr;
    HistogramTransformationDialog* m_ht = nullptr;
    ASinhStretchDialog* m_ashd = nullptr;
    LocalHistogramEqualizationDialog* m_lhed = nullptr;
    CurveTransformDialog* m_ctd = nullptr;

public:
    ImageTransformationsMenu(QWidget* parent);

private:
    void AdaptiveStretchSelection();

    void AutoHistogramSelection();

    void HistogramTransformationSelection();

    void ArcSinhStretchSelection();

    void CurvesTransformationSelection();

    void LocalHistogramEqualizationSelection();

};

class ProcessMenu : public QMenu {

    QMenu* m_image_trans;
    QMenu* m_morphology;
    QMenu* m_abg_extraction;
    QMenu* m_image_geometry;
    QMenu* m_mask;

public:

    ProcessMenu(QWidget* parent) : QMenu(parent) {
        this->setTitle("Process");
        CreateProcessMenu();
    }

private:
    void CreateProcessMenu();
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
        "All Accepted Formats(*.bmp *.fits *.fts *.fit *.tiff *.tif);;"
        "BMP file(*.bmp);;"
        "FITS file(*.fits *.fts *.fit);;"
        "XISF file(*.xisf);;"
        "TIFF file(*.tiff *.tif)";

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
