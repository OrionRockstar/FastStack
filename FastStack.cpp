#include "pch.h"
#include "FastStack.h"
#include "ImageWindow.h"
#include "Image.h"
#include "Maths.h"

FastStack::FastStack(QWidget *parent)
    : QMainWindow(parent)
{ 
    ui.setupUi(this);
    //this->menuBar()->addMenu()
    // 
    //QMenu* fileMenu = menuBar()->addMenu(tr("& File"));
    
    //will need to be apart of mainwindow/faststack as it needs access to image
    //or create temp image in menubar
    m_menubar = new MenuBar(this);
    this->setMenuBar(m_menubar);
    this->setWindowTitle("FastStack");
    this->resize(this->screen()->geometry().width()/2, this->screen()->geometry().height() / 2);
    this->resize(1386, 921);
    this->resize(1108, 736);

    //this->setWindowState(Qt::WindowMaximized);
    QString mss = "background: #708090";
    this->setStyleSheet(mss);
    //this->insertToolBar();

    QPushButton* button = new QPushButton("Stretch",this);
    QRect bs = { 0,22,50,50 };
    button->setGeometry(bs);
    QString pbss = "QPushButton {background-color: rgb(220,0,0);"
        "border-radius: 6px;}"
        "QPushButton:hover {background-color: rgb(255,0,0);}"
        "QPushButton:pressed {background-color:rgb(200,0,0)}";

    Image32 img;
    std::filesystem::path orion = "C:\\Users\\Zack\\Desktop\\Fits Files\\Vixen80\\orion.fts";
    std::filesystem::path test = "C:\\Users\\Zack\\Desktop\\test.fits";
    FileOP::FitsRead(test, img);
    ImageWindow* iw = new ImageWindow(img);

    /*QImage qim(img.Cols(), img.Rows(), QImage::Format::Format_Grayscale8);
    for (int el = 0; el < img.Total(); ++el)
        qim.bits()[el] = img[el] * 255;

    QLabel* lab = new QLabel(this);
    QPixmap pixmap;
    pixmap.convertFromImage(qim);
    //pixmap.fromImage(qim);
    lab->setGeometry(QRect(50, 50, 550, 550));
    lab->setPixmap(pixmap);*/
    //QString  path = "C:\\Users\\Zack\\Desktop\\Gina\\IMG_20230119_153938_HDR.jpg";
    //QPixmap pixmap(path);
    
    //QSize s = { 50,50 };
    //pixmap.scaled(s);
    //button->setIcon(QIcon(pixmap));
    //button->setIconSize(pixmap.rect().size());
    button->setStyleSheet(pbss);
    QMenu* processMenu = menuBar()->addMenu(tr(" & Process"));

    //QMenu* pm = menuBar()->addMenu();
    //QAction* open = fileMenu->addAction(tr("&Open"), this, &FastStack::open);
    //open->setShortcut(QKeySequence::Open);

}
