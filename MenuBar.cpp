#include "pch.h"
#include "MenuBar.h"
//#include "ImageWindow.h"
#include "FITS.h"
#include "TIFF.h"
#include "Bitmap.h"

#include"FastStack.h"
#include "SaveFileOptionsWindows.h"
#include "ImageFileReader.h"


ColorMenu::ColorMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Color");

	this->addAction(tr("Channel Combination"), this, &ColorMenu::channelCombinationSelection);
	this->addAction(tr("LRGB Combination"), this, &ColorMenu::lrgbCombinationSelection);
	this->addAction(tr("RGB->Grayscale"), this, &ColorMenu::rgbGrayscaleConverstionSelection);
}

void ColorMenu::channelCombinationSelection() {
	if (m_ccd == nullptr) {
		m_ccd = std::make_unique<ChannelCombinationDialog>(parentWidget());
		connect(m_ccd.get(), &ProcessDialog::onClose, this, [this]() {m_ccd.reset(); });
	}
}

void ColorMenu::lrgbCombinationSelection() {
	if (m_lrgbd == nullptr) {
		m_lrgbd = std::make_unique<LRGBCombinationDialog>(parentWidget());
		connect(m_lrgbd.get(), &ProcessDialog::onClose, this, [this]() {m_lrgbd.reset(); });
	}
}

void ColorMenu::rgbGrayscaleConverstionSelection() {

	auto iw = reinterpret_cast<ImageWindow8*>(FastStack::recast(parentWidget())->workspace()->currentSubWindow()->widget());

	switch (iw->type()) {
	case ImageType::UBYTE:
		return iw->convertToGrayscale();
	case ImageType::USHORT:
		return reinterpret_cast<ImageWindow16*>(iw)->convertToGrayscale();
	case ImageType::FLOAT:
		return reinterpret_cast<ImageWindow32*>(iw)->convertToGrayscale();
	}
}






ImageGeometryMenu::ImageGeometryMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Image Geometry");
	//this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("Crop"), this, &ImageGeometryMenu::CropSelection);
	this->addAction(tr("Fast Rotation"), this, &ImageGeometryMenu::FastRotationSelection);
	this->addAction(tr("Integer Resample"), this, &ImageGeometryMenu::IntegerResampleSelection);
	this->addAction(tr("Rotation"), this, &ImageGeometryMenu::RotationSelection);
}

void ImageGeometryMenu::RotationSelection() {
	if (m_rd == nullptr) {
		m_rd = new RotationDialog(parentWidget());
		connect(m_rd, &ProcessDialog::onClose, this, [this]() {m_rd = nullptr; });
	}
}

void ImageGeometryMenu::FastRotationSelection() {
	if (m_frd == nullptr) {
		m_frd = new FastRotationDialog(parentWidget());
		connect(m_frd, &ProcessDialog::onClose, this, [this]() {m_frd = nullptr; });
	}
}

void ImageGeometryMenu::IntegerResampleSelection() {
	if (m_irs == nullptr) {
		m_irs = std::make_unique<IntegerResampleDialog>(parentWidget());
		connect(m_irs.get(), &ProcessDialog::onClose, this, [this]() { m_irs.reset(); });
	}
}

void ImageGeometryMenu::CropSelection() {
	if (m_cd == nullptr) {
		m_cd = std::make_unique<CropDialog>(parentWidget());
		connect(m_cd.get(), &ProcessDialog::onClose, this, [this]() {m_cd.reset(); });
	}
}

ImageStackingMenu::ImageStackingMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Image Stacking");
	this->addAction(tr("Calibration Combination"), this, &ImageStackingMenu::calibrationCombinationSelection);
	this->addAction(tr("Drizzle Integration"), this, &ImageStackingMenu::drizzleIntegrationDialogSelection);
	this->addAction(tr("Image Stacking"), this, &ImageStackingMenu::imageStackingDialogSelection);
	this->addAction(tr("Star Alignment"), this, &ImageStackingMenu::starAlignmentDialogSelection);
}

void ImageStackingMenu::calibrationCombinationSelection() {
	if (m_ccd == nullptr) {
		m_ccd = std::make_unique<CalibrationCombinationDialog>(parentWidget());
		connect(m_ccd.get(), &ProcessDialog::onClose, this, [this]() { m_ccd.reset(); });
	}
}

void ImageStackingMenu::drizzleIntegrationDialogSelection() {

	if (m_did == nullptr) {
		m_did = std::make_unique<DrizzleIntegrationDialog>(parentWidget());
		connect(m_did.get(), &ProcessDialog::onClose, this, [this]() { m_did.reset(); });
	}
}

void ImageStackingMenu::imageStackingDialogSelection() {
	
	if (m_isd == nullptr) {
		m_isd = std::make_unique<ImageStackingDialog>(parentWidget());
		connect(m_isd.get(), &ProcessDialog::onClose, this, [this]() { m_isd.reset(); });
	}
}

void ImageStackingMenu::starAlignmentDialogSelection() {
	
	if (m_sad == nullptr) {
		m_sad = std::make_unique<StarAlignmentDialog>(parentWidget());
		connect(m_sad.get(), &ProcessDialog::onClose, this, [this]() { m_sad.reset(); });
	}
}

ImageTransformationsMenu::ImageTransformationsMenu(QWidget* parent) : QMenu(parent) {
	this->setTitle("Image Transformations");
	//this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("&Adaptive Stretch"), this, &ImageTransformationsMenu::AdaptiveStretchSelection);
	this->addAction(tr("&ArcSinh Stretch"), this, &ImageTransformationsMenu::ArcSinhStretchSelection);
	this->addAction(tr("&AutoHistogram"), this, &ImageTransformationsMenu::AutoHistogramSelection);
	this->addAction(tr("&Binerize"), this, &ImageTransformationsMenu::binerizeSelection);
	this->addAction(tr("&Color Saturation"), this, &ImageTransformationsMenu::colorSaturationSelection);
	this->addAction(tr("&Curve Transformation"), this, &ImageTransformationsMenu::CurvesTransformationSelection);
	this->addAction(tr("&Historgram Transformation"), this, &ImageTransformationsMenu::HistogramTransformationSelection);
	this->addAction(tr("&Local Histogram Equalization"), this, &ImageTransformationsMenu::LocalHistogramEqualizationSelection);
}

void ImageTransformationsMenu::AdaptiveStretchSelection() {
	if (m_asd == nullptr) {
		m_asd = new AdaptiveStretchDialog(parentWidget());
		connect(m_asd, &ProcessDialog::onClose, this, [this]() {m_asd = nullptr; });
	}
}

void ImageTransformationsMenu::ArcSinhStretchSelection() {
	if (m_ashd == nullptr) {
		m_ashd = new ASinhStretchDialog(parentWidget());
		connect(m_ashd, &ProcessDialog::onClose, this, [this]() {m_ashd = nullptr; });
	}
}

void ImageTransformationsMenu::AutoHistogramSelection() {
	if (m_ahd == nullptr) {
		m_ahd = new AutoHistogramDialog(parentWidget());
		connect(m_ahd, &ProcessDialog::onClose, this, [this]() {m_ahd = nullptr; });
	}
}

void ImageTransformationsMenu::binerizeSelection() {

	if (m_bd == nullptr) {
		m_bd = std::make_unique<BinerizeDialog>(parentWidget());
		connect(m_bd.get(), &ProcessDialog::onClose, this, [this]() { m_bd.reset(); });
	}
}

void ImageTransformationsMenu::colorSaturationSelection() {

	if (m_csd == nullptr) {
		m_csd = std::make_unique<ColorSaturationDialog>(parentWidget());
		connect(m_csd.get(), &ProcessDialog::onClose, this, [this]() { m_csd.reset(); });
	}
}

void ImageTransformationsMenu::CurvesTransformationSelection() {
	if (m_ctd == nullptr) {
		m_ctd = new CurveTransformDialog(parentWidget());
		connect(m_ctd, &ProcessDialog::onClose, this, [this]() {m_ctd = nullptr; });
	}
}

void ImageTransformationsMenu::HistogramTransformationSelection() {
	if (m_ht == nullptr) {
		m_ht = new HistogramTransformationDialog(parentWidget());
		connect(m_ht, &ProcessDialog::onClose, this, [this]() {m_ht = nullptr; });
	}
}

void ImageTransformationsMenu::LocalHistogramEqualizationSelection() {

	if (m_lhed == nullptr) {
		m_lhed = new LocalHistogramEqualizationDialog(parentWidget());
		connect(m_lhed, &LocalHistogramEqualizationDialog::onClose, this, [this]() {m_lhed = nullptr; });
	}
}



WaveletTransformationMenu::WaveletTransformationMenu(QWidget* parent) : QMenu(parent) {

	this->setTitle("Wavelet Transformation");
	this->addAction(tr("Wavelet Layers"), this, &WaveletTransformationMenu::waveletLayersSelection);
}

void WaveletTransformationMenu::waveletLayersSelection() {

	if (m_wld == nullptr) {
		m_wld = std::make_unique<WaveletLayersDialog>(parentWidget());
		connect(m_wld.get(), &ProcessDialog::onClose, this, [this]() { m_wld.reset(); });
	}
}



MaskMenu::MaskMenu(QWidget* parent) : QMenu(parent) {
	
	this->setTitle(tr("&Masks"));
	this->addAction(tr("Range Mask"), this, &MaskMenu::rangeMaskSelection);
	this->addAction(tr("Star Mask"), this, &MaskMenu::starMaskSelection);
}

void MaskMenu::rangeMaskSelection() {
	
	if (m_rmd == nullptr) {
		m_rmd = std::make_unique<RangeMaskDialog>(parentWidget());
		connect(m_rmd.get(), &ProcessDialog::onClose, [this]() { m_rmd.reset(); });
	}
}

void MaskMenu::starMaskSelection() {
	
	if (m_smd == nullptr) {
		m_smd = std::make_unique<StarMaskDialog>(parentWidget());
		connect(m_smd.get(), &ProcessDialog::onClose, this, [this]() {m_smd.reset(); });
	}
}




MorphologyMenu::MorphologyMenu(QWidget* parent) : QMenu(parent) {
	
	this->setTitle(tr("&Morphology"));
	this->addAction(tr("Bilateral Filter"), this, &MorphologyMenu::BilateralFilterSelection);
	this->addAction(tr("Gaussian Filter"), this, &MorphologyMenu::GaussianBlurSelection);
	this->addAction(tr("Morphological Transformations"), this, &MorphologyMenu::MorphologicalTransformationsSelection);
	this->addAction(tr("Canny Edge Detection"), this, &MorphologyMenu::EdgeDetectionSelection);
}

void MorphologyMenu::MorphologicalTransformationsSelection() {
	
	if (m_mtd == nullptr) {
		m_mtd = new MorphologicalTransformationDialog(parentWidget());
		connect(m_mtd, &ProcessDialog::onClose, this, [this]() { m_mtd = nullptr; });
	}
}

void MorphologyMenu::GaussianBlurSelection() {
	
	if (m_gfd == nullptr) {
		m_gfd = new GaussianFilterDialog(parentWidget());
		connect(m_gfd, &ProcessDialog::onClose, this, [this]() { m_gfd = nullptr; });
	}
}

void MorphologyMenu::BilateralFilterSelection() {
	
	if (m_bfd == nullptr) {
		m_bfd = new BilateralFilterDialog(parentWidget());
		connect(m_bfd, &ProcessDialog::onClose, this, [this]() { m_bfd = nullptr; });
	}
}

void MorphologyMenu::EdgeDetectionSelection() {
	
	if (m_edd == nullptr) {
		m_edd = std::make_unique<EdgeDetectionDialog>(parentWidget());
		connect(m_edd.get(), &ProcessDialog::onClose, this, [this]() { m_edd.reset(); });
	}
}





void ProcessMenu::CreateProcessMenu() {

	m_abg_extraction = new BackgroundExtraction(parentWidget());
	this->addMenu(m_abg_extraction);

	m_color = new ColorMenu(parentWidget());
	this->addMenu(m_color);

	m_image_geometry = new ImageGeometryMenu(parentWidget());
	this->addMenu(m_image_geometry);

	m_image_stacking = new ImageStackingMenu(parentWidget());
	this->addMenu(m_image_stacking);

	m_image_trans = new ImageTransformationsMenu(parentWidget());
	this->addMenu(m_image_trans);

	m_wavlet_trans = new WaveletTransformationMenu(parentWidget());
	this->addMenu(m_wavlet_trans);

	m_mask = new MaskMenu(parentWidget());
	this->addMenu(m_mask);

	m_morphology = new MorphologyMenu(parentWidget());
	this->addMenu(m_morphology);
}


void MenuBar::onWindowClose() {
	if (FastStack::recast(parentWidget())->workspace()->subWindowList().size() == 1)
		save_as->setEnabled(false);
}

void MenuBar::onWindowOpen() {
	save_as->setEnabled(true);
}

MenuBar::MenuBar(QWidget *parent): QMenuBar(parent) {

	m_parent = reinterpret_cast<FastStack*>(parent);

	QPalette pal;
	pal.setColor(QPalette::Window, QColor(169, 169, 169));
	setPalette(pal);
	//this->
	//this->setStyleSheet(" QMenuBar::item:selected{background: #696969}; background-color: #D3D3D3; color:black;");

	AddFileMenu();
	AddProcessMenu();

}


void MenuBar::Open() {

		//QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

		std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], ImageFileReader::typelist()).toStdString();

		if (f_path == "") 
			return;

		ImageFileReader(FastStack::recast(parentWidget())->workspace()).read(f_path);
}

void MenuBar::Save() {

}

void MenuBar::SaveAs() {

	auto fsp = reinterpret_cast<FastStack*>(m_parent);

	ImageWindow8* obj = reinterpret_cast<ImageWindow8*>(fsp->workspace()->currentSubWindow()->widget());
	ImageType type = obj->type();

	std::filesystem::path file_path = QFileDialog::getSaveFileName(this, tr("Save Image As"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0].append("/" + obj->name()), ImageFileReader::typelist(), nullptr).toStdString();
	std::string ext = file_path.extension().string();
	file_path = file_path.replace_extension("");

	ImageWindow8* iw8;
	ImageWindow16* iw16;
	ImageWindow32* iw32;

	if (type == ImageType::UBYTE)
		iw8 = obj;
	else if (type == ImageType::USHORT)
		iw16 = reinterpret_cast<ImageWindow16*>(obj);
	else if (type == ImageType::FLOAT)
		iw32 = reinterpret_cast<ImageWindow32*>(obj);

	if (ext == ".fits" || ext == ".fit") {

		FITSWindow* fw = new FITSWindow(type, m_parent);
		
		if (fw->exec() != QDialog::Accepted)
			return;

		FITS fits;
		fits.create(file_path);

		switch (type) {
		case ImageType::UBYTE:
			return fits.write(iw8->source(), fw->imageType());
		case ImageType::USHORT:
			return fits.write(iw16->source(), fw->imageType());
		case ImageType::FLOAT:
			return fits.write(iw32->source(), fw->imageType());
		}
	}

	if (ext == ".tiff") {
		TIFFWindow* tw = new TIFFWindow(type, m_parent);

		if (tw->exec() != QDialog::Accepted)
			return;

		TIFF tiff;
		tiff.create(file_path);
		switch (type) {
		case ImageType::UBYTE:
			return tiff.write(iw8->source(), tw->imageType(), tw->planarContig());
		case ImageType::USHORT:
			return tiff.write(iw16->source(), tw->imageType(), tw->planarContig());
		case ImageType::FLOAT:
			return tiff.write(iw32->source(), tw->imageType(), tw->planarContig());
		}
	}

	if (ext == ".bmp") {
		Bitmap bitmap;
		bitmap.create(file_path);
		switch (type) {
		case ImageType::UBYTE:
			return bitmap.Write(iw8->source());
		case ImageType::USHORT:
			return bitmap.Write(iw16->source());
		case ImageType::FLOAT:
			return bitmap.Write(iw32->source());
		}
	}
}

void MenuBar::AddFileMenu() {

	filemenu = this->addMenu(tr("&File"));
	filemenu->setPalette(palette());
	//filemenu->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");


	open = filemenu->addAction(tr("&Open"), this, &MenuBar::Open);
	save = filemenu->addAction(tr("&Save"), this, &MenuBar::Save);
	save_as = filemenu->addAction(tr("&Save As..."), this, &MenuBar::SaveAs);

	save->setEnabled(false);
	save_as->setEnabled(false);

}

void MenuBar::AddProcessMenu() {

	m_process = new ProcessMenu(parentWidget());
	//m_process->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");

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