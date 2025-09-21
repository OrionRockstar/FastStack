#pragma once
#include "pch.h"
#include "Image.h"
#include "ImageWindow.h"

#include "ImageCalibration.h"
#include "ImageStackingDialog.h"

#include "ImageGeometryDialogs.h"

#include "AutomaticBackgroundExtractionDialog.h"

//color menu
#include "ChannelCombinationDialog.h"
#include "LRGBCombinationDialog.h"


//image transformation menu
#include "HistogramTransformationDialog.h"
#include "AdaptiveStretchDialog.h"

#include "ASinhStretchDialog.h"
#include "AutoHistogramDialog.h"

#include "ColorSaturationDialog.h"
#include "CurvesTransformationDialog.h"
#include "LocalHistogramEqualizationDialog.h"
#include "BinerizeDialog.h"
#include "ExponentialTransformationDialog.h"

//
#include "WaveletLayersDialog.h"
#include "StarAlignment.h"

//mask
#include "RangeMaskDialog.h"
#include "StarMaskDialog.h"

//morphology
#include "GaussianFilterDialog.h"
#include "BilateralFilterDialog.h"
#include "EdgeDetection.h"
#include "MorphologicalTransformationDialog.h"

//noise reduction
#include "SCNRDialog.h"

#include "MediaPlayerDialog.h"



class BackgroundExtractionMenu : public QMenu {

    std::unique_ptr<AutomaticBackgroundExtractionDialog> m_abed;
public:
    BackgroundExtractionMenu(QWidget* parent);
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

    std::unique_ptr<RotationDialog> m_rd;
    std::unique_ptr<FastRotationDialog> m_frd;
    //HomographyTransformationDialog* m_htd = nullptr;
    std::unique_ptr<IntegerResampleDialog> m_irs;
    std::unique_ptr<CropDialog> m_cd;

public:
    ImageGeometryMenu(QWidget* parent);

private:
    void rotationSelection();

    void fastRotationSelection();

    void integerResampleSelection();

    void cropSelection();
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





class ImageTransformationsMenu : public QMenu {

    std::unique_ptr<AdaptiveStretchDialog> m_asd;
    std::unique_ptr<ASinhStretchDialog> m_ashd;
    std::unique_ptr<AutoHistogramDialog> m_ahd;
    std::unique_ptr<BinerizeDialog> m_bd;
    std::unique_ptr<ColorSaturationDialog> m_csd;
    std::unique_ptr<CurvesTransformationDialog> m_ctd;
    std::unique_ptr<ExponentialTransformationDialog> m_etd;
    std::unique_ptr<HistogramTransformationDialog> m_ht;
    std::unique_ptr<LocalHistogramEqualizationDialog> m_lhed;

public:
    ImageTransformationsMenu(QWidget* parent);

private:
    void adaptiveStretchSelection();

    void arcSinhStretchSelection();

    void autoHistogramSelection();

    void binerizeSelection();

    void colorSaturationSelection();

    void curvesTransformationSelection();

    void histogramTransformationSelection();

    void exponentialTransformationSelection();

    void localHistogramEqualizationSelection();
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





class MediaMenu : public QMenu {
    std::unique_ptr<MediaPlayerDialog> m_mpd;

public:
    MediaMenu(QWidget* parent);

private:
    void mediaPlayerSelection();
};



class MorphologyMenu : public QMenu {
    std::unique_ptr<MorphologicalTransformationDialog> m_mtd;
    std::unique_ptr<BilateralFilterDialog> m_bfd;
    std::unique_ptr<GaussianFilterDialog> m_gfd;
    std::unique_ptr<EdgeDetectionDialog> m_edd;

public:
    MorphologyMenu(QWidget* parent);

private:
    void MorphologicalTransformationsSelection();

    void GaussianBlurSelection();

    void bilateralFilterSelection();

    void EdgeDetectionSelection();
};





class NoiseReductionMenu : public QMenu {

    std::unique_ptr<SCNRDialog> m_scnrd;

public:
    NoiseReductionMenu(QWidget* parent);

private:
    void scnrSelection();
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

    QMenu* m_color = nullptr;
    QMenu* m_image_trans = nullptr;
    BackgroundExtractionMenu* m_abg_extraction = nullptr;
    ImageGeometryMenu* m_image_geometry = nullptr;
    ImageStackingMenu* m_image_stacking = nullptr;
    WaveletTransformationMenu* m_wavlet_trans = nullptr;
    MaskMenu* m_mask = nullptr;
    MorphologyMenu* m_morphology = nullptr;
    NoiseReductionMenu* m_noise_reduction_menu = nullptr;
    MediaMenu* m_media_menu = nullptr;

public:

    ProcessMenu(QWidget* parent) : QMenu(parent) {
        this->setTitle("Process");
        createProcessMenu();
    }

private:
    void createProcessMenu();
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

    void addFileMenu();

    void addProcessMenu();

public slots:

    void onWindowClose();

    void onWindowOpen();

signals:
    void initiateSave();
};
