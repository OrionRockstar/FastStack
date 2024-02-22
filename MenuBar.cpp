#include "pch.h"
#include "MenuBar.h"
//#include "ImageWindow.h"
#include "FITS.h"
#include "TIFF.h"
#include "QString"
#include"FastStack.h"
#include "SaveFileOptionsWindows.h"

void MenuBar::onWindowClose() {
	if (reinterpret_cast<FastStack*>(parentWidget())->workspace->subWindowList().size() == 1)
		save_as->setEnabled(false);
}

void MenuBar::onWindowOpen() {
	save_as->setEnabled(true);
}

MenuBar::MenuBar(QWidget *parent): QMenuBar(parent) {

	m_parent = reinterpret_cast<FastStack*>(parent);

	this->setStyleSheet(" QMenuBar::item:selected{background: #696969}; background-color: #D3D3D3; color:black;");

	AddFileMenu();
	AddProcessMenu();
}


void MenuBar::Open() {

		Image8 img8;
		Image16 img16;
		Image32 img32;

		std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist).toStdString();

		if (f_path == "") 
			return;

		std::string ext = f_path.extension().string();
		std::string filename = f_path.filename().string();

		if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
			FITS fits;
			fits.Open(f_path);
			switch (fits.GetFITSBitDepth()) {
				case 8: {
					fits.Read(img8);
					break;
				}
				case 16: {
					fits.Read(img16);
					break;
				}
				case -32: {
					fits.Read(img32);
					break;
				}
			}
		}

		else if (ext == ".tif" || ext == ".tiff") {
			TIFF tiff;
			tiff.Open(f_path);
			switch (tiff.GetTiffValue(TIFF::TIFFTAG::BitsPerSample)) {
				case 8: {
					tiff.Read(img8);
					break;
				}
				case 16: {
					tiff.Read(img16);
					break;
				}
				case 32: {
					tiff.Read(img32);
					break;
				}
			}
		}


		auto wptr = reinterpret_cast<FastStack*>(parentWidget())->workspace;
		if (img8.Exists()) {
			ImageWindow8* iw8 = new ImageWindow8(img8, filename.c_str(), wptr);
			//connect(iw8->iws, &IWSS::sendWindowClose, this, &MenuBar::onWindowClose);	
			//wptr->addSubWindow(iw8);// ->resize(iw8->Cols() + 12, iw8->Rows() + 36);
		}

		if (img16.Exists()) {
			ImageWindow16* iw16 = new ImageWindow16(img16, filename.c_str(), wptr);
			//connect(iw16->iws, &IWSS::sendWindowClose, this, &MenuBar::onWindowClose);
			//wptr->addSubWindow(iw16);// ->resize(iw16->Cols() + 12, iw16->Rows() + 36);
		}

		if (img32.Exists()) {
			ImageWindow32* iw32 = new ImageWindow32(img32, filename.c_str(), wptr);
			//connect(iw32->iws, &IWSS::sendWindowClose, this, &MenuBar::onWindowClose);
			//wptr->addSubWindow(iw32);// ->resize(iw32->Cols() + 12, iw32->Rows() + 36);
		}
		//wptr->currentSubWindow()->show();		
		//save_as->setEnabled(true);
}

void MenuBar::Save() {

}

void MenuBar::SaveAs() {

	auto fsp = reinterpret_cast<FastStack*>(m_parent);

	ImageWindow8* obj = reinterpret_cast<ImageWindow8*>(fsp->workspace->currentSubWindow()->widget());
	int bitdepth = obj->Bitdepth();

	std::filesystem::path file_path = QFileDialog::getSaveFileName(this, tr("Save Image As"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0].append("/" + obj->ImageName()), m_typelist, nullptr).toStdString();
	std::string ext = file_path.extension().string();

	ImageWindow8* iw8;
	ImageWindow16* iw16;
	ImageWindow32* iw32;

	if (bitdepth == 8)
		iw8 = obj;
	else if (bitdepth == 16)
		iw16 = reinterpret_cast<ImageWindow16*>(obj);
	else if (bitdepth == -32)
		iw32 = reinterpret_cast<ImageWindow32*>(obj);

	if (ext == ".fits" || ext == ".fit") {
		FITSWindow* fw = new FITSWindow(m_parent, bitdepth);
		int bd = -8;
		if (fw->exec() == QDialog::Accepted)
			bd = fw->getNewBitdepth();
		else
			return;

		FITS fits;
		fits.Create(file_path);
		if (bitdepth == 8)
			fits.Write(iw8->source, bd);
		else if (bitdepth == 16)
			fits.Write(iw16->source, bd);
		else if (bitdepth == -32)
			fits.Write(iw32->source, bd);

	}

	if (ext == ".tiff") {
		TIFFWindow* tw = new TIFFWindow(bitdepth);

		int bd = -8;
		bool planar_contiguous = true;

		if (tw->exec() == QDialog::Accepted) {
			bd = tw->getNewBitdepth();
			planar_contiguous = tw->getPlanarContig();
		}
		else
			return;

		TIFF tiff;
		tiff.Create(file_path);
		if (bitdepth == 8)
			tiff.Write(iw8->source, bd, planar_contiguous);
		else if (bitdepth == 16)
			tiff.Write(iw16->source, bd, planar_contiguous);
		else if (bitdepth == -32)
			tiff.Write(iw32->source, bd, planar_contiguous);

	}



}

void MenuBar::AddFileMenu() {

	filemenu = this->addMenu(tr("&File"));
	filemenu->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");


	open = filemenu->addAction(tr("&Open"), this, &MenuBar::Open);
	save = filemenu->addAction(tr("&Save"), this, &MenuBar::Save);
	save_as = filemenu->addAction(tr("&Save As..."), this, &MenuBar::SaveAs);

	save->setEnabled(false);
	save_as->setEnabled(false);

}

void MenuBar::AddProcessMenu() {

	m_process = new ProcessMenu(parentWidget());
	m_process->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");

	this->addMenu(m_process);
	//m_process->CreateProcessMenu();
	//processmenu = this->addMenu(tr("&Process"));
	//stretch_image = processmenu->addMenu(tr("Stretch Image"));
	//processmenu->CreateProcessMenu();
}

void MenuBar::AddAction() {

	AddFileMenu();

	//QPalette pal = palette();
	//pal.setColor(QPalette::Base, QColor("#696969"));
	//this->setPalette(pal);
	//this->setAutoFillBackground(true);
	//order can matter when it comes to style sheets
	//QString mbss;
	//this->actions().at(0)->setDisabled(true);
	//this->setStyleSheet(" QMenuBar::item:selected{background: #696969}; background-color: #D3D3D3; color:black;");// selection - background - color: #696969");
	//filemenu->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	//this->setStyleSheet("QMenuBar::item:selected{background: red} ");
}