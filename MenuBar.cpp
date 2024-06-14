#include "pch.h"
#include "MenuBar.h"
//#include "ImageWindow.h"
#include "FITS.h"
#include "TIFF.h"
#include "Bitmap.h"

#include"FastStack.h"
#include "SaveFileOptionsWindows.h"
#include "ImageFileReader.h"


ImageGeometryMenu::ImageGeometryMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Image Geometry");
	this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("Rotation"), this, &ImageGeometryMenu::RotationSelection);
	this->addAction(tr("Fast Rotation"), this, &ImageGeometryMenu::FastRotationSelection);
	this->addAction(tr("Integer Resample"), this, &ImageGeometryMenu::IntegerResampleSelection);
}

void ImageGeometryMenu::RotationSelection() {
	if (m_rd == nullptr) {
		m_rd = new RotationDialog(parentWidget());
		connect(m_rd, &ProcessDialog::onClose, [this]() {m_rd = nullptr; });
	}
}

void ImageGeometryMenu::FastRotationSelection() {
	if (m_frd == nullptr) {
		m_frd = new FastRotationDialog(parentWidget());
		connect(m_frd, &ProcessDialog::onClose, [this]() {m_frd = nullptr; });
	}
}

void ImageGeometryMenu::IntegerResampleSelection() {
	if (m_irs == nullptr) {
		m_irs = std::make_unique<IntegerResampleDialog>(parentWidget());
		//m_irs = std::unique_ptr<IntegerResampleDialog>(new IntegerResampleDialog(parentWidget()));
		connect(m_irs.get(), &ProcessDialog::onClose, [this]() { m_irs.reset(); });
	}
}

ImageTransformationsMenu::ImageTransformationsMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Image Transformations");
	this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("&Adaptive Stretch"), this, &ImageTransformationsMenu::AdaptiveStretchSelection);
	this->addAction(tr("&AutoHistogram"), this, &ImageTransformationsMenu::AutoHistogramSelection);
	this->addAction(tr("&Historgram Transformation"), this, &ImageTransformationsMenu::HistogramTransformationSelection);
	this->addAction(tr("&ArcSinh Stretch"), this, &ImageTransformationsMenu::ArcSinhStretchSelection);
	this->addAction(tr("&Curves Transformation"), this, &ImageTransformationsMenu::CurvesTransformationSelection);
	this->addAction(tr("&Local Histogram Equalization"), this, &ImageTransformationsMenu::LocalHistogramEqualizationSelection);
}

void ImageTransformationsMenu::AdaptiveStretchSelection() {
	if (m_asd == nullptr) {
		m_asd = new AdaptiveStretchDialog(parentWidget());
		connect(m_asd, &ProcessDialog::onClose, [this]() {m_asd = nullptr; });
	}
}

void ImageTransformationsMenu::AutoHistogramSelection() {
	if (m_ahd == nullptr) {
		m_ahd = new AutoHistogramDialog(parentWidget());
		connect(m_ahd, &ProcessDialog::onClose, [this]() {m_ahd = nullptr; });
	}
}

void ImageTransformationsMenu::HistogramTransformationSelection() {
	if (m_ht == nullptr) {
		m_ht = new HistogramTransformationDialog(parentWidget());
		connect(m_ht, &ProcessDialog::onClose, [this]() {m_ht = nullptr; });
	}
}

void ImageTransformationsMenu::ArcSinhStretchSelection() {
	if (m_ashd == nullptr) {
		m_ashd = new ASinhStretchDialog(parentWidget());
		connect(m_ashd, &ProcessDialog::onClose, [this]() {m_ashd = nullptr; });
	}
}

void ImageTransformationsMenu::CurvesTransformationSelection() {
	if (m_ctd == nullptr) {
		m_ctd = new CurveTransformDialog(parentWidget());
		connect(m_ctd, &ProcessDialog::onClose, [this]() {m_ctd = nullptr; });
	}
}

void ImageTransformationsMenu::LocalHistogramEqualizationSelection() {
	if (m_lhed == nullptr) {
		m_lhed = new LocalHistogramEqualizationDialog(parentWidget());
		connect(m_lhed, &LocalHistogramEqualizationDialog::onClose, [this]() {m_lhed = nullptr; });
	}
}





MaskMenu::MaskMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle(tr("&Masks"));
	this->addAction(tr("Range Mask"), this, &MaskMenu::RangeMaskSelection);
}

void MaskMenu::RangeMaskSelection() {
	if (m_rmd == nullptr) {
		m_rmd = new RangeMaskDialog(parentWidget());
		connect(m_rmd, &ProcessDialog::onClose, [this]() { m_rmd = nullptr; });
	}
}





MorphologyMenu::MorphologyMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle(tr("&Morphology"));
	this->addAction(tr("Bilateral Filter"), this, &MorphologyMenu::BilateralFilterSelection);
	this->addAction(tr("Gaussian Filter"), this, &MorphologyMenu::GaussianBlurSelection);
	this->addAction(tr("Morphological Transformations"), this, &MorphologyMenu::MorphologicalTransformationsSelection);
	this->addAction(tr("Sobel Edge Detection"), this, &MorphologyMenu::SobelSelection);
}

void MorphologyMenu::MorphologicalTransformationsSelection() {
	if (m_mtd == nullptr) {
		m_mtd = new MorphologicalTransformationDialog(parentWidget());
		connect(m_mtd, &ProcessDialog::onClose, [this]() { m_mtd = nullptr; });
	}
}

void MorphologyMenu::GaussianBlurSelection() {
	if (m_gfd == nullptr) {
		m_gfd = new GaussianFilterDialog(parentWidget());
		connect(m_gfd, &ProcessDialog::onClose, [this]() { m_gfd = nullptr; });
	}
}

void MorphologyMenu::BilateralFilterSelection() {
	if (m_bfd == nullptr) {
		m_bfd = new BilateralFilterDialog(parentWidget());
		connect(m_bfd, &ProcessDialog::onClose, [this]() { m_bfd = nullptr; });
	}
}

void MorphologyMenu::SobelSelection() {
	if (m_sd == nullptr) {
		m_sd = std::make_unique<SobelDialog>(parentWidget());
		connect(m_sd.get(), &ProcessDialog::onClose, [this]() { m_sd.reset(); });
	}
}





void ProcessMenu::CreateProcessMenu() {

	m_abg_extraction = new BackgroundExtraction(parentWidget());
	this->addMenu(m_abg_extraction);

	m_image_geometry = new ImageGeometryMenu(parentWidget());
	this->addMenu(m_image_geometry);

	m_image_trans = new ImageTransformationsMenu(parentWidget());
	this->addMenu(m_image_trans);

	m_mask = new MaskMenu(parentWidget());
	this->addMenu(m_mask);

	m_morphology = new MorphologyMenu(parentWidget());
	this->addMenu(m_morphology);
}


void MenuBar::onWindowClose() {
	if (reinterpret_cast<FastStack*>(parentWidget())->m_workspace->subWindowList().size() == 1)
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

		std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist).toStdString();

		if (f_path == "") 
			return;

		ImageFileReader(reinterpret_cast<FastStack*>(parent())->m_workspace).Read(f_path);
		/// <summary>
		/// only need the following to read image and open window
		/// </summary>
		/*Image8 img8;
		Image16 img16;
		Image32 img32;

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
			fits.Close();
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
			tiff.Close();
		}

		else if (ext == ".bmp") {
			Bitmap bitmap;
			bitmap.Open(f_path);
			bitmap.Read(img8);
			bitmap.Close();
		}

		auto wptr = reinterpret_cast<FastStack*>(parent())->workspace;

		if (img8.Exists())
			ImageWindow8* iw8 = new ImageWindow8(img8, filename.c_str(), wptr);

		if (img16.Exists())
			ImageWindow16* iw16 = new ImageWindow16(img16, filename.c_str(), wptr);

		if (img32.Exists())
			ImageWindow32* iw32 = new ImageWindow32(img32, filename.c_str(), wptr);*/
}

void MenuBar::Save() {

}

void MenuBar::SaveAs() {

	auto fsp = reinterpret_cast<FastStack*>(m_parent);

	ImageWindow8* obj = reinterpret_cast<ImageWindow8*>(fsp->m_workspace->currentSubWindow()->widget());
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
			fits.Write(iw8->Source(), bd);
		else if (bitdepth == 16)
			fits.Write(iw16->Source(), bd);
		else if (bitdepth == -32) 
			fits.Write(iw32->Source(), bd);
		
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
			tiff.Write(iw8->Source(), bd, planar_contiguous);
		else if (bitdepth == 16)
			tiff.Write(iw16->Source(), bd, planar_contiguous);
		else if (bitdepth == -32)
			tiff.Write(iw32->Source(), bd, planar_contiguous);
	}

	if (ext == ".bmp") {
		Bitmap bitmap;
		bitmap.Create(file_path);
		if (bitdepth == 8)
			bitmap.Write(iw8->Source());
		else if (bitdepth == 16)
			bitmap.Write(iw16->Source());
		else if (bitdepth == -32)
			bitmap.Write(iw32->Source());

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