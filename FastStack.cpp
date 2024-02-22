#include "pch.h"
#include "FastStack.h"
#include "ImageWindow.h"
#include "Image.h"
#include "Maths.h"
#include "FITS.h"
#include "LocalHistogramEqualization.h"

FastStack::FastStack(QWidget *parent)
    : QMainWindow(parent)
{ 
    ui.setupUi(this);

    
    //will need to be apart of mainwindow/faststack as it needs access to image
    //or create temp image in menubar

    this->resize(this->screen()->availableSize());
    this->setWindowState(Qt::WindowState::WindowMaximized);

    workspace = new Workspace(this);
    this->setCentralWidget(workspace);

    std::cout << this << "\n";
    m_menubar = new MenuBar(this);
    this->setMenuBar(m_menubar);
    this->setWindowTitle("FastStack");

    connect(workspace, &Workspace::sendOpen, m_menubar, &MenuBar::onWindowOpen);
    connect(workspace, &Workspace::sendClose, m_menubar, &MenuBar::onWindowClose);

    QPalette pal = palette();
    QColor("#708090");
    QColor("#696969");
    //pal.setColor(QPalette::Window, QColor(173,3,252));
    //this->setPalette(pal);
    //this->set
    //QString mss = "background: #708090";
    //this->setStyleSheet(mss);
    //this->insertToolBar();

    //QPushButton* button = new QPushButton("Stretch",this);
   // QRect bs = { 0,22,50,50 };
    //button->setGeometry(bs);
    QString pbss = "QPushButton {background-color: rgb(220,0,0);"
        "border-radius: 6px;}"
        "QPushButton:hover {background-color: rgb(255,0,0);}"
        "QPushButton:pressed {background-color:rgb(200,0,0)}";

    
    Image32 img;
    std::filesystem::path orion = "C:\\Users\\Zack\\Desktop\\Fits Files\\Vixen80\\orion.fts";
    std::filesystem::path test = "C:\\Users\\Zack\\Desktop\\test.fits";
    //FileOP::FitsRead(test, img);
    FITS fits;
    fits.Open(test);
    fits.Read(img);

    //work on window and scrollbar style
    //workspace->addSubWindow(iw32);
    //QString ss = "QMdiSubWindow { border-width: 4px; border-style: solid; border-color: purple;} QMdiSubWindow::title{color:purple; height:28px;}";

    //QString ss = "QMdiSubWindow {border-bottom-width: 6px; border-style: solid; border-color: white; }";
    //workspace->currentSubWindow()->setStyleSheet(ss);
    //workspace->currentSubWindow()->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    //workspace->currentSubWindow()->show();
    
    //fits.Open(orion);

    //button->setStyleSheet(pbss);
    //QMenu* processMenu = menuBar()->addMenu(tr(" & Process"));
    //HistogramTransformationWidget* widget = new HistogramTransformationWidget(this);
    //processMenu->addAction()
    //QMenu* pm = menuBar()->addMenu();
    //QAction* open = fileMenu->addAction(tr("&Open"), this, &FastStack::open);
    //open->setShortcut(QKeySequence::Open);

}
