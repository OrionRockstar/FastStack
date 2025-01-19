#pragma once
#include "pch.h"
#include "Image.h"
#include "ImageWindow.h"

#include "MorphologicalTransformation.h"

#include "ImageCalibration.h"
#include "ImageStackingDialog.h"

#include "ImageGeometryDialogs.h"
#include "GaussianFilter.h"
#include "BilateralFilter.h"
#include "EdgeDetection.h"
#include "RangeMask.h"
#include "StarMask.h"

#include "AutomaticBackgroundExtraction.h"

//color menu
#include "ChannelCombination.h"
#include "LRGBCombination.h"


//image transformation menu
#include "HistogramTransformation.h"
#include "AutoHistogram.h"
#include "ASinhStretch.h"
#include "ColorSaturation.h"
#include "CurvesTransformation.h"
#include "LocalHistogramEqualization.h"
#include "AdaptiveStretch.h"
#include "Binerize.h"

//
#include "Wavelet.h"
#include "StarAlignment.h"

class BackgroundExtraction : public QMenu {

    std::unique_ptr<AutomaticBackgroundExtractionDialog> m_abed;
public:
    BackgroundExtraction(QWidget* parent);
private:
    void autoBackgroundExtractionSelection();
};

class ColorMenu : public QMenu {
    std::unique_ptr<ChannelCombinationDialog> m_ccd;
    std::unique_ptr<LRGBCombinationDialog> m_lrgbd;

public:
    ColorMenu(QWidget* parent);

private:
    void channelCombinationSelection();

    void lrgbCombinationSelection();

    void rgbGrayscaleConverstionSelection();
};

class ImageGeometryMenu : public QMenu {

    RotationDialog* m_rd = nullptr;
    FastRotationDialog* m_frd = nullptr;
    HomographyTransformationDialog* m_htd = nullptr;
    std::unique_ptr<IntegerResampleDialog> m_irs;
    std::unique_ptr<CropDialog> m_cd;

public:
    ImageGeometryMenu(QWidget* parent);

private:
    void RotationSelection();

    void FastRotationSelection();

    void IntegerResampleSelection();

    void CropSelection();
};

class ImageStackingMenu : public QMenu {

    std::unique_ptr<CalibrationCombinationDialog> m_ccd;
    std::unique_ptr<DrizzleIntegrationDialog> m_did;
    std::unique_ptr<ImageStackingDialog> m_isd;
    std::unique_ptr<StarAlignmentDialog> m_sad;

public:
    ImageStackingMenu(QWidget* parent);

private:
    void calibrationCombinationSelection();

    void drizzleIntegrationDialogSelection();

    void imageStackingDialogSelection();

    void starAlignmentDialogSelection();
};

class MaskMenu : public QMenu {
    std::unique_ptr<RangeMaskDialog> m_rmd;
    std::unique_ptr<StarMaskDialog> m_smd;
public:
    MaskMenu(QWidget* parent);

private:
    void rangeMaskSelection();

    void starMaskSelection();
};

class MorphologyMenu : public QMenu {
    MorphologicalTransformationDialog* m_mtd = nullptr;
    BilateralFilterDialog* m_bfd = nullptr;
    GaussianFilterDialog* m_gfd = nullptr;
    std::unique_ptr<EdgeDetectionDialog> m_edd;

public:
    MorphologyMenu(QWidget* parent);

private:
    void MorphologicalTransformationsSelection();

    void GaussianBlurSelection();

    void BilateralFilterSelection();

    void EdgeDetectionSelection();
};

class ImageTransformationsMenu : public QMenu {
    Q_OBJECT

    AdaptiveStretchDialog* m_asd = nullptr;
    ASinhStretchDialog* m_ashd = nullptr;
    AutoHistogramDialog* m_ahd = nullptr;
    std::unique_ptr<BinerizeDialog> m_bd;
    std::unique_ptr<ColorSaturationDialog> m_csd;
    CurveTransformDialog* m_ctd = nullptr;
    HistogramTransformationDialog* m_ht = nullptr;
    LocalHistogramEqualizationDialog* m_lhed = nullptr;

public:
    ImageTransformationsMenu(QWidget* parent);

private:
    void AdaptiveStretchSelection();

    void ArcSinhStretchSelection();

    void AutoHistogramSelection();

    void binerizeSelection();

    void colorSaturationSelection();

    void CurvesTransformationSelection();

    void HistogramTransformationSelection();

    void LocalHistogramEqualizationSelection();
};

class WaveletTransformationMenu : public QMenu {
    Q_OBJECT

    std::unique_ptr<WaveletLayersDialog> m_wld = nullptr;

public:
    WaveletTransformationMenu(QWidget* parent);

private:
    void waveletLayersSelection();
};

class ProcessMenu : public QMenu {

    QMenu* m_color;
    QMenu* m_image_trans;
    QMenu* m_morphology;
    QMenu* m_abg_extraction;
    QMenu* m_image_geometry;
    QMenu* m_image_stacking;
    QMenu* m_wavlet_trans = nullptr;
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
