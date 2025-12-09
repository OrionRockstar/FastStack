#include "pch.h"
#include "MenuBar.h"
//#include "ImageWindow.h"
#include "FITS.h"
#include "TIFF.h"
#include "Bitmap.h"

#include"FastStack.h"
#include "SaveFileOptionsWindows.h"
#include "ImageFileReader.h"



BackgroundExtractionMenu::BackgroundExtractionMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {
	this->setTitle("Background Extraction");
	this->addAction(tr("&Automatic Background Extraction"), this, &BackgroundExtractionMenu::autoBackgroundExtractionSelection);
}

void BackgroundExtractionMenu::autoBackgroundExtractionSelection() {

	if (m_abed == nullptr) {
		m_abed = std::make_unique<AutomaticBackgroundExtractionDialog>(m_workspace);
		connect(m_abed.get(), &ProcessDialog::windowClosed, this, [this]() { m_abed.reset(); });
	}
}





ColorMenu::ColorMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {

	this->setTitle("Color");
	this->addAction(tr("Channel Combination"), this, &ColorMenu::channelCombinationSelection);
	this->addAction(tr("LRGB Combination"), this, &ColorMenu::lrgbCombinationSelection);
	this->addAction(tr("RGB->Grayscale"), this, &ColorMenu::rgbGrayscaleConverstionSelection);
}

void ColorMenu::channelCombinationSelection() {
	if (m_ccd == nullptr) {
		m_ccd = std::make_unique<ChannelCombinationDialog>(parentWidget());
		connect(m_ccd.get(), &ProcessDialog::windowClosed, this, [this]() {m_ccd.reset(); });
	}
}

void ColorMenu::lrgbCombinationSelection() {

	if (m_lrgbd == nullptr) {
		m_lrgbd = std::make_unique<LRGBCombinationDialog>(parentWidget());
		connect(m_lrgbd.get(), &ProcessDialog::windowClosed, this, [this]() { m_lrgbd.reset(); });
	}
}

void ColorMenu::rgbGrayscaleConverstionSelection() {

	auto ptr = FastStack::recast(parentWidget())->workspace();

	if (ptr->subWindowList().isEmpty())
		return;

	auto iw = reinterpret_cast<ImageWindow8*>(ptr->currentSubWindow()->widget());

	switch (iw->type()) {
	case ImageType::UBYTE:
		return iw->convertToGrayscale();
	case ImageType::USHORT:
		return reinterpret_cast<ImageWindow16*>(iw)->convertToGrayscale();
	case ImageType::FLOAT:
		return reinterpret_cast<ImageWindow32*>(iw)->convertToGrayscale();
	}
}





ImageGeometryMenu::ImageGeometryMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {
	this->setTitle("Image Geometry");
	//this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("Crop"), this, &ImageGeometryMenu::cropSelection);
	this->addAction(tr("Fast Rotation"), this, &ImageGeometryMenu::fastRotationSelection);
	this->addAction(tr("Integer Resample"), this, &ImageGeometryMenu::integerResampleSelection);
	this->addAction(tr("Resize"), this, &ImageGeometryMenu::resizeSelection);
	this->addAction(tr("Rotation"), this, &ImageGeometryMenu::rotationSelection);
}

void ImageGeometryMenu::cropSelection() {

	if (m_cd == nullptr) {
		m_cd = std::make_unique<CropDialog>(m_workspace);
		connect(m_cd.get(), &ProcessDialog::windowClosed, this, [this]() { m_cd.reset(); });
	}
}

void ImageGeometryMenu::fastRotationSelection() {

	if (m_frd == nullptr) {
		m_frd = std::make_unique<FastRotationDialog>(m_workspace);
		connect(m_frd.get(), &ProcessDialog::windowClosed, this, [this]() { m_frd.reset(); });
	}
}

void ImageGeometryMenu::integerResampleSelection() {

	if (m_irs == nullptr) {
		m_irs = std::make_unique<IntegerResampleDialog>(m_workspace);
		connect(m_irs.get(), &ProcessDialog::windowClosed, this, [this]() { m_irs.reset(); });
	}
}

void ImageGeometryMenu::resizeSelection() {

	if (m_rsd == nullptr) {
		m_rsd = std::make_unique<ResizeDialog>(m_workspace);
		connect(m_rsd.get(), &ProcessDialog::windowClosed, this, [this]() { m_rsd.reset(); });
	}
}

void ImageGeometryMenu::rotationSelection() {

	if (m_rd == nullptr) {
		m_rd = std::make_unique<RotationDialog>(m_workspace);
		connect(m_rd.get(), &ProcessDialog::windowClosed, this, [this]() { m_rd.reset(); });
	}
}




ImageStackingMenu::ImageStackingMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {
	
	this->setTitle("Image Stacking");
	this->addAction(tr("Calibration Combination"), this, &ImageStackingMenu::calibrationCombinationSelection);
	this->addAction(tr("Drizzle Integration"), this, &ImageStackingMenu::drizzleIntegrationDialogSelection);
	this->addAction(tr("Image Stacking"), this, &ImageStackingMenu::imageStackingDialogSelection);
	this->addAction(tr("Star Alignment"), this, &ImageStackingMenu::starAlignmentDialogSelection);
}

void ImageStackingMenu::calibrationCombinationSelection() {

	if (m_ccd == nullptr) {
		m_ccd = std::make_unique<CalibrationCombinationDialog>(m_workspace);
		connect(m_ccd.get(), &ProcessDialog::windowClosed, this, [this]() { m_ccd.reset(); });
	}
}

void ImageStackingMenu::drizzleIntegrationDialogSelection() {

	if (m_did == nullptr) {
		m_did = std::make_unique<DrizzleIntegrationDialog>(m_workspace);
		connect(m_did.get(), &ProcessDialog::windowClosed, this, [this]() { m_did.reset(); });
	}
}

void ImageStackingMenu::imageStackingDialogSelection() {
	
	if (m_isd == nullptr) {
		m_isd = std::make_unique<ImageStackingDialog>(m_workspace);
		connect(m_isd.get(), &ProcessDialog::windowClosed, this, [this]() { m_isd.reset(); });
	}
}

void ImageStackingMenu::starAlignmentDialogSelection() {
	
	if (m_sad == nullptr) {
		m_sad = std::make_unique<StarAlignmentDialog>(m_workspace);
		connect(m_sad.get(), &ProcessDialog::windowClosed, this, [this]() { m_sad.reset(); });
	}
}





ImageTransformationsMenu::ImageTransformationsMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {

	this->setTitle("Image Transformations");
	//this->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");
	this->addAction(tr("&Adaptive Stretch"), this, &ImageTransformationsMenu::adaptiveStretchSelection);
	this->addAction(tr("&ArcSinh Stretch"), this, &ImageTransformationsMenu::arcSinhStretchSelection);
	this->addAction(tr("&AutoHistogram"), this, &ImageTransformationsMenu::autoHistogramSelection);
	this->addAction(tr("&Binerize"), this, &ImageTransformationsMenu::binerizeSelection);
	this->addAction(tr("&Color Saturation"), this, &ImageTransformationsMenu::colorSaturationSelection);
	this->addAction(tr("&Curve Transformation"), this, &ImageTransformationsMenu::curvesTransformationSelection);
	this->addAction(tr("&Historgram Transformation"), this, &ImageTransformationsMenu::histogramTransformationSelection);
	this->addAction(tr("&Exponential Transformation"), this, &ImageTransformationsMenu::exponentialTransformationSelection);
	this->addAction(tr("&Local Histogram Equalization"), this, &ImageTransformationsMenu::localHistogramEqualizationSelection);
}

void ImageTransformationsMenu::adaptiveStretchSelection() {

	if (nullptr == m_asd) {
		m_asd = std::make_unique<AdaptiveStretchDialog>(m_workspace);
		connect(m_asd.get(), &ProcessDialog::windowClosed, this, [this]() { m_asd.reset(); });
	}
}

void ImageTransformationsMenu::arcSinhStretchSelection() {

	if (m_ashd == nullptr) {
		m_ashd = std::make_unique<ASinhStretchDialog>(m_workspace);
		connect(m_ashd.get(), &ProcessDialog::windowClosed, this, [this]() { m_ashd.reset(); });
	}
}

void ImageTransformationsMenu::autoHistogramSelection() {

	if (m_ahd == nullptr) {
		m_ahd = std::make_unique<AutoHistogramDialog>(m_workspace);
		connect(m_ahd.get(), &ProcessDialog::windowClosed, this, [this]() { m_ahd.reset(); });
	}
}

void ImageTransformationsMenu::binerizeSelection() {

	if (m_bd == nullptr) {
		m_bd = std::make_unique<BinerizeDialog>(m_workspace);
		connect(m_bd.get(), &ProcessDialog::windowClosed, this, [this]() { m_bd.reset(); });
	}
}

void ImageTransformationsMenu::colorSaturationSelection() {

	if (m_csd == nullptr) {
		m_csd = std::make_unique<ColorSaturationDialog>(m_workspace);
		connect(m_csd.get(), &ProcessDialog::windowClosed, this, [this]() { m_csd.reset(); });
	}
}

void ImageTransformationsMenu::curvesTransformationSelection() {

	if (m_ctd == nullptr) {
		m_ctd = std::make_unique<CurvesTransformationDialog>(m_workspace);
		connect(m_ctd.get(), &ProcessDialog::windowClosed, this, [this]() { m_ctd.reset(); });
	}
}

void ImageTransformationsMenu::exponentialTransformationSelection() {

	if (m_etd == nullptr) {
		m_etd = std::make_unique<ExponentialTransformationDialog>(m_workspace);
		connect(m_etd.get(), &ProcessDialog::windowClosed, this, [this]() { m_etd.reset(); });
	}
}

void ImageTransformationsMenu::histogramTransformationSelection() {
	if (m_ht == nullptr) {
		m_ht = std::make_unique<HistogramTransformationDialog>(m_workspace);
		connect(m_ht.get(), &ProcessDialog::windowClosed, this, [this]() { m_ht.reset(); });
	}
}

void ImageTransformationsMenu::localHistogramEqualizationSelection() {

	if (m_lhed == nullptr) {
		m_lhed = std::make_unique<LocalHistogramEqualizationDialog>(m_workspace);
		connect(m_lhed.get(), &LocalHistogramEqualizationDialog::windowClosed, this, [this]() { m_lhed.reset(); });
	}
}





WaveletTransformationMenu::WaveletTransformationMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {

	this->setTitle("Wavelet Transformation");
	this->addAction(tr("Wavelet Layers"), this, &WaveletTransformationMenu::waveletLayersSelection);
}

void WaveletTransformationMenu::waveletLayersSelection() {

	if (m_wld == nullptr) {
		m_wld = std::make_unique<WaveletLayersDialog>(m_workspace);
		connect(m_wld.get(), &ProcessDialog::windowClosed, this, [this]() { m_wld.reset(); });
	}
}





MaskMenu::MaskMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {
	
	this->setTitle(tr("&Masks"));
	this->addAction(tr("Range Mask"), this, &MaskMenu::rangeMaskSelection);
	this->addAction(tr("Star Mask"), this, &MaskMenu::starMaskSelection);
}

void MaskMenu::rangeMaskSelection() {
	
	if (m_rmd == nullptr) {
		m_rmd = std::make_unique<RangeMaskDialog>(m_workspace);
		connect(m_rmd.get(), &ProcessDialog::windowClosed, [this]() { m_rmd.reset(); });
	}
}

void MaskMenu::starMaskSelection() {
	
	if (m_smd == nullptr) {
		m_smd = std::make_unique<StarMaskDialog>(m_workspace);
		connect(m_smd.get(), &ProcessDialog::windowClosed, this, [this]() {m_smd.reset(); });
	}
}



MediaMenu::MediaMenu(QWidget* parent) : QMenu(parent) {

	this->setTitle(tr("&Media"));
	this->addAction(tr("Playlist Player"), this, &MediaMenu::mediaPlayerSelection);
}

void MediaMenu::mediaPlayerSelection() {

	if (m_mpd == nullptr) {
		m_mpd = std::make_unique<MediaPlayerDialog>(parentWidget());
		connect(m_mpd.get(), &ProcessDialog::windowClosed, [this]() { m_mpd.reset(); });
	}
}



MorphologyMenu::MorphologyMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {
	
	this->setTitle(tr("&Morphology"));
	this->addAction(tr("Bilateral Filter"), this, &MorphologyMenu::bilateralFilterSelection);
	this->addAction(tr("Gaussian Filter"), this, &MorphologyMenu::GaussianBlurSelection);
	this->addAction(tr("Morphological Transformations"), this, &MorphologyMenu::MorphologicalTransformationsSelection);
	this->addAction(tr("Canny Edge Detection"), this, &MorphologyMenu::EdgeDetectionSelection);
}

void MorphologyMenu::MorphologicalTransformationsSelection() {
	
	if (m_mtd == nullptr) {
		m_mtd = std::make_unique<MorphologicalTransformationDialog>(m_workspace);
		connect(m_mtd.get(), &ProcessDialog::windowClosed, this, [this]() { m_mtd.reset(); });
	}
}

void MorphologyMenu::GaussianBlurSelection() {
	
	if (m_gfd == nullptr) {
		m_gfd = std::make_unique<GaussianFilterDialog>(m_workspace);
		connect(m_gfd.get(), &ProcessDialog::windowClosed, this, [this]() { m_gfd.reset(); });
	}
}

void MorphologyMenu::bilateralFilterSelection() {
	
	if (m_bfd == nullptr) {
		m_bfd = std::make_unique<BilateralFilterDialog>(m_workspace);
		connect(m_bfd.get(), &ProcessDialog::windowClosed, this, [this]() { m_bfd.reset(); });
	}
}

void MorphologyMenu::EdgeDetectionSelection() {
	
	if (m_edd == nullptr) {
		m_edd = std::make_unique<EdgeDetectionDialog>(m_workspace);
		connect(m_edd.get(), &ProcessDialog::windowClosed, this, [this]() { m_edd.reset(); });
	}
}





NoiseReductionMenu::NoiseReductionMenu(Workspace* workspace, QWidget* parent) : Menu(workspace, parent) {

	this->setTitle(tr("&Noise Reduction"));
	this->addAction(tr("SCNR"), this, &NoiseReductionMenu::scnrSelection);
}

void NoiseReductionMenu::scnrSelection() {

	if (m_scnrd == nullptr) {
		m_scnrd = std::make_unique<SCNRDialog>(parentWidget());
		connect(m_scnrd.get(), &ProcessDialog::windowClosed, this, [this]() { m_scnrd.reset(); });
	}
}




ProcessMenu::ProcessMenu(Workspace* wokrspace, QWidget* parent) : Menu(wokrspace, parent) {

	this->setTitle("Process");

	this->addMenu(new BackgroundExtractionMenu(m_workspace, this));
	this->addMenu(new ColorMenu(m_workspace, this));
	this->addMenu(new ImageGeometryMenu(m_workspace, this));
	this->addMenu(new ImageStackingMenu(m_workspace, this));
	this->addMenu(new ImageTransformationsMenu(m_workspace, this));
	this->addMenu(new WaveletTransformationMenu(m_workspace, this));
	this->addMenu(new MaskMenu(m_workspace, this));
	this->addMenu(new MediaMenu(parentWidget()));
	this->addMenu(new MorphologyMenu(m_workspace, this));
	this->addMenu(new NoiseReductionMenu(m_workspace, this));
}





void MenuBar::onWindowClose() {
	if (FastStack::recast(parentWidget())->workspace()->subWindowList().size() == 1)
		save_as->setEnabled(false);
}

void MenuBar::onWindowOpen() {
	save_as->setEnabled(true);
}

MenuBar::MenuBar(FastStack* parent): QMenuBar(parent) {

	m_workspace = parent->workspace();
	//m_parent = reinterpret_cast<FastStack*>(parent);

	QPalette pal;
	pal.setColor(QPalette::ButtonText, Qt::white);
	setPalette(pal);

	addFileMenu();
	addProcessMenu();
}


void MenuBar::Open() {

		std::filesystem::path f_path = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], ImageFileReader::typelist()).toStdString();

		if (f_path == "") 
			return;

		ImageFileReader(m_workspace).read(f_path);
}

void MenuBar::Save() {

}

void MenuBar::SaveAs() {

	//auto fsp = reinterpret_cast<FastStack*>(m_workspace);

	ImageWindow8* obj = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());
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

		FITSWindow* fw = new FITSWindow(type, m_workspace);
		
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
		TIFFWindow* tw = new TIFFWindow(type, m_workspace);

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

void MenuBar::addFileMenu() {

	filemenu = this->addMenu(tr("&File"));

	//filemenu->setStyleSheet("QMenu::item:disabled{color:grey}""QMenu::item:selected{background:#696969}");

	open = filemenu->addAction(tr("&Open"), this, &MenuBar::Open);
	save = filemenu->addAction(tr("&Save"), this, &MenuBar::Save);
	save_as = filemenu->addAction(tr("&Save As..."), this, &MenuBar::SaveAs);

	save->setEnabled(false);
	save_as->setEnabled(false);
}

void MenuBar::addProcessMenu() {

	m_process = new ProcessMenu(m_workspace, parentWidget());
	this->addMenu(m_process);
}
