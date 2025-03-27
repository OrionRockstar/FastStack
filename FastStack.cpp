#include "pch.h"
#include "FastStack.h"

#include "AutomaticBackgroundExtraction.h"
#include "ImageStackingDialog.h"

#include "ImageCalibration.h"
#include "ImageGeometryDialogs.h"
#include "RangeMask.h"
#include "ColorSaturation.h"
#include "ChannelCombination.h"
#include "LRGBCombination.h"
#include "StarAlignment.h"
#include "CustomWidgets.h"
//#include "FastStackToolBar.h"



FastStack::FastStack(QWidget *parent) : QMainWindow(parent) { 
    ui.setupUi(this);

    QApplication::setWindowIcon(QIcon("./Icons//fast_stack_icon_fs2.png"));

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

    //StatisticsDialog* sd = new StatisticsDialog(this);
    //DrizzleIntegrationDialog* did = new DrizzleIntegrationDialog(this);
    //StarAlignmentDialog* sad = new StarAlignmentDialog(this);

    //ImageStackingDialog* isd = new ImageStackingDialog(this);
    //CalibrationCombinationDialog* ccd = new CalibrationCombinationDialog(this);

    //AutomaticBackgroundExtractionDialog* abed = new AutomaticBackgroundExtractionDialog(this);
}
