#include "pch.h"
#include "FastStack.h"

#include "AutomaticBackgroundExtraction.h"
#include "ImageStackingDialog.h"

#include "ImageCalibration.h"
#include "ImageGeometryDialogs.h"
#include "RangeMask.h"
#include "AutoHistogram.h"
#include "ColorSaturation.h"
#include "ChannelCombination.h"
#include "LRGBCombination.h"
#include "StarMask.h"
#include "Binerize.h"
#include "StarAlignment.h"
#include "Statistics.h"
//#include "FastStackToolBar.h"



FastStack::FastStack(QWidget *parent) : QMainWindow(parent) { 
    ui.setupUi(this);

    QApplication::setWindowIcon(QIcon("./Icons//fast_stack_icon_fs2.png"));

    QIcon icon;
    //icon.addFile("./Icons//fast_stack_icon_fs2.png");
    //this->setWindowIcon(icon);

    this->setWindowTitle("FastStack");
    this->resize(this->screen()->availableSize());
    this->setWindowState(Qt::WindowState::WindowMaximized);

    m_workspace = new Workspace(this);
    this->setCentralWidget(m_workspace);

    m_menubar = new MenuBar(this);
    this->setMenuBar(m_menubar);

    connect(m_workspace, &Workspace::imageWindowCreated, m_menubar, &MenuBar::onWindowOpen);
    connect(m_workspace, &Workspace::imageWindowClosed, m_menubar, &MenuBar::onWindowClose);

    m_toolbar = new FastStackToolBar(this);
    this->addToolBar(Qt::ToolBarArea::BottomToolBarArea, m_toolbar);
    connect(m_workspace, &QMdiArea::subWindowActivated, m_toolbar->imageInformationLabel(), &ImageInformationLabel::displayText);

    //Preview* pv = new Preview(this);
    //StatisticsDialog* sd = new StatisticsDialog(this);
    //DrizzleIntegrationDialog* did = new DrizzleIntegrationDialog(this);
    //StarAlignmentDialog* sad = new StarAlignmentDialog(this);
    //CurveTransformDialog* ctd = new CurveTransformDialog(this);
    //StarMaskDialog* smd = new StarMaskDialog(this);
    //LRGBCombinationDialog* lcd = new LRGBCombinationDialog(this);
    //ChannelCombinationDialog* ccd = new ChannelCombinationDialog(this);
    //AutoHistogramDialog* m_ahd = new AutoHistogramDialog(this);
    //RangeMaskDialog* rmd = new RangeMaskDialog(this);
    //ColorSaturationDialog* csd = new ColorSaturationDialog(this);
    //ImageStackingDialog* isd = new ImageStackingDialog(this);
    //CalibrationCombinationDialog* ccd = new CalibrationCombinationDialog(this);
    //MorphologicalTransformationDialog* mtd = new MorphologicalTransformationDialog(this);
    // 
    //ASinhStretchDialog* ashd = new ASinhStretchDialog(this);
    //HistogramTransformationDialog* ht = new HistogramTransformationDialog(this);
    //CurveTransformDialog* ct = new CurveTransformDialog(this);
    //AutomaticBackgroundExtractionDialog* abed = new AutomaticBackgroundExtractionDialog(this);
}
