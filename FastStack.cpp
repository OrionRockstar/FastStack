#include "pch.h"
#include "FastStack.h"
//#include "ImageWindow.h"
//#include "Image.h"
//#include "CurvesTransformation.h"
#include "AutomaticBackgroundExtraction.h"
//#include "MorphologicalTransformation.h"
#include "ImageStackingDialog.h"
//#include "AdaptiveStretch.h"
#include "ImageCalibration.h"
#include "ImageGeometryDialogs.h"
#include "Masks.h"
#include "AutoHistogram.h"

FastStack::FastStack(QWidget *parent)
    : QMainWindow(parent)
{ 
    ui.setupUi(this);
    this->setWindowTitle("FastStack");
    this->resize(this->screen()->availableSize());
    this->setWindowState(Qt::WindowState::WindowMaximized);

    m_workspace = new Workspace(this);
    this->setCentralWidget(m_workspace);

    m_menubar = new MenuBar(this);
    this->setMenuBar(m_menubar);

    connect(m_workspace, &Workspace::sendOpen, m_menubar, &MenuBar::onWindowOpen);
    connect(m_workspace, &Workspace::sendClose, m_menubar, &MenuBar::onWindowClose);

    QPalette pal = palette();
    QColor("#708090");
    QColor("#696969");

    //QPushButton* button = new QPushButton("Stretch",this);
   // QRect bs = { 0,22,50,50 };
    //button->setGeometry(bs);
    QString pbss = "QPushButton {background-color: rgb(220,0,0);"
        "border-radius: 6px;}"
        "QPushButton:hover {background-color: rgb(255,0,0);}"
        "QPushButton:pressed {background-color:rgb(200,0,0)}";

    //IntegerResampleDialog* ird = new IntegerResampleDialog(this);
    //CropDialog* cd = new CropDialog(this);
    //AutoHistogramDialog* m_ahd = new AutoHistogramDialog(this);
    //RangeMaskDialog* rmd = new RangeMaskDialog(this);
    //ImageStackingDialog* isd = new ImageStackingDialog(this);
    //CalibrationCombinationDialog* ccd = new CalibrationCombinationDialog(this);
    //MorphologicalTransformationDialog* mtd = new MorphologicalTransformationDialog(this);
    // 
    //AdaptiveStretchDialog* asd = new AdaptiveStretchDialog(this);
    //ASinhStretchDialog* ashd = new ASinhStretchDialog(this);
    //HistogramTransformationDialog* ht = new HistogramTransformationDialog(this);
    //CurveTransformDialog* ct = new CurveTransformDialog(this);
    //AutomaticBackgroundExtractionDialog* abed = new AutomaticBackgroundExtractionDialog(this);
 
}
