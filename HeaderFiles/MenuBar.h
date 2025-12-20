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
#include "AdaptiveStretchDialog.h"
#include "ASinhStretchDialog.h"
#include "AutoHistogramDialog.h"
#include "BinerizeDialog.h"
#include "ColorSaturationDialog.h"
#include "CurvesTransformationDialog.h"
#include "ExponentialTransformationDialog.h"
#include "HistogramTransformationDialog.h"
#include "LocalHistogramEqualizationDialog.h"


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


class FastStack;
class Workspace;

class Menu : public QMenu {
    Q_OBJECT

protected:
    Workspace* m_workspace = nullptr;

    Workspace* workspace() const { return m_workspace; }

public:
    Menu(Workspace* workspace, QWidget* parent = nullptr) : m_workspace(workspace), QMenu(parent) {}
};





class BackgroundExtractionMenu : public Menu {

    std::unique_ptr<AutomaticBackgroundExtractionDialog> m_abed;
public:
    BackgroundExtractionMenu(Workspace* workspace, QWidget* parent = nullptr);
private:
    void autoBackgroundExtractionSelection();
};





class ColorMenu : public Menu {
    std::unique_ptr<ChannelCombinationDialog> m_ccd;
    std::unique_ptr<LRGBCombinationDialog> m_lrgbd;

public:
    ColorMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void channelCombinationSelection();

    void lrgbCombinationSelection();

    void rgbGrayscaleConverstionSelection();
};





class ImageGeometryMenu : public Menu {

    std::unique_ptr<RotationDialog> m_rd;
    std::unique_ptr<FastRotationDialog> m_frd;
    //HomographyTransformationDialog* m_htd = nullptr;
    std::unique_ptr<IntegerResampleDialog> m_irs;
    std::unique_ptr<CropDialog> m_cd;
    std::unique_ptr<ResizeDialog> m_rsd;

public:
    ImageGeometryMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void cropSelection();

    void fastRotationSelection();

    void integerResampleSelection();

    void resizeSelection();

    void rotationSelection();
};





class ImageStackingMenu : public Menu {

    std::unique_ptr<CalibrationCombinationDialog> m_ccd;
    std::unique_ptr<DrizzleIntegrationDialog> m_did;
    std::unique_ptr<ImageStackingDialog> m_isd;
    std::unique_ptr<StarAlignmentDialog> m_sad;

public:
    ImageStackingMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void calibrationCombinationSelection();

    void drizzleIntegrationDialogSelection();

    void imageStackingDialogSelection();

    void starAlignmentDialogSelection();
};





class ImageTransformationsMenu : public Menu {

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
    ImageTransformationsMenu(Workspace* workspace, QWidget* parent = nullptr);

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





class MaskMenu : public Menu {
    std::unique_ptr<RangeMaskDialog> m_rmd;
    std::unique_ptr<StarMaskDialog> m_smd;
public:
    MaskMenu(Workspace* workspace, QWidget* parent = nullptr);

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



class MorphologyMenu : public Menu {
    std::unique_ptr<MorphologicalTransformationDialog> m_mtd;
    std::unique_ptr<BilateralFilterDialog> m_bfd;
    std::unique_ptr<GaussianFilterDialog> m_gfd;
    std::unique_ptr<EdgeDetectionDialog> m_edd;

public:
    MorphologyMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void MorphologicalTransformationsSelection();

    void GaussianBlurSelection();

    void bilateralFilterSelection();

    void EdgeDetectionSelection();
};





class NoiseReductionMenu : public Menu {

    std::unique_ptr<SCNRDialog> m_scnrd;

public:
    NoiseReductionMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void scnrSelection();
};





class WaveletTransformationMenu : public Menu {
    Q_OBJECT

    std::unique_ptr<WaveletLayersDialog> m_wld = nullptr;

public:
    WaveletTransformationMenu(Workspace* workspace, QWidget* parent = nullptr);

private:
    void waveletLayersSelection();
};

class ProcessMenu : public Menu {

public:

    ProcessMenu(Workspace* wokrspace, QWidget* parent);
};

class MenuBar: public QMenuBar {
    Q_OBJECT
    //Ui::MenuBarClass ui;
public:
    MenuBar(FastStack* parent);

private:
    Workspace* m_workspace = nullptr;
    //QMainWindow* m_parent;

	QMenu* filemenu;// = addMenu(tr("&File"));
	QAction* open;
    QAction* save;
    QAction* save_as;

    QMenu* m_process;

    uint32_t image_counter = 0;

public:
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
