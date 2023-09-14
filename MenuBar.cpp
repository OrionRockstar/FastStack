#include "pch.h"
#include "MenuBar.h"


MenuBar::MenuBar(QWidget *parent): QMenuBar(parent) {
	this->autoFillBackground();
	//this->paintEvent();
	//filemenu = this->addMenu(tr("&File"));
	//open = filemenu->addAction(tr("&Open"), filemenu, &FileMenu::Open);
	//parent->addMenu(tr("&File"));
	//QPalette pal = QPalette();
	//pal.setColor(QPalette::Window, Qt::gray);
	//this->setPalette(pal);
	AddAction();
	//parent = this->addMenu(tr("&File"));
	//QAction* open = this->addAction(tr("&Open"), this, &FileMenu::Open);
}

MenuBar::~MenuBar()
{}

void MenuBar::Open() {
	    QFileDialog dialog(this, tr("Open File"));
		OpenImageDialog(dialog, QFileDialog::AcceptOpen);

		Image32 img;
		while (dialog.exec() == QDialog::Accepted && !FileOP::FitsRead(std::filesystem::path(dialog.selectedFiles().constFirst().toStdString()), img)) {}
}

void MenuBar::Save() {

}

void MenuBar::AddFileMenu() {
	filemenu = addMenu(tr("&File"));

	open = filemenu->addAction(tr("&Open"), this, &MenuBar::Open);
	save = filemenu->addAction(tr("&Save"), this, &MenuBar::Save);
	save_as = filemenu->addAction(tr("&Save As..."), this, &MenuBar::Save);
}

void MenuBar::AddAction() {

	AddFileMenu();

	QPalette pal = this->palette();
	//this->setAutoFillBackground(true);
	//order can matter when it comes to style sheets
	QString mbss;
	this->setStyleSheet(" QMenuBar::item:selected{background: #696969}; background-color: #D3D3D3; color:black; selection-background-color: #696969");
	//this->setStyleSheet("QMenuBar::item:selected{background: red} ");
}