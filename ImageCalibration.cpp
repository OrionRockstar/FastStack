#include "pch.h"
#include "ImageCalibration.h"
#include "FITS.h"
#include "TIFF.h"
#include "FastStack.h"
//#include "ImageStackingDialog.h"

void ImageCalibration::setMasterDark(std::filesystem::path path) {

	if (!std::filesystem::exists(path))
		return;

	std::string ext = path.extension().string();

	if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
		FITS fits;
		fits.Open(path);
		fits.ReadAny(m_master_dark);
	}

	else if (ext == ".tif" || ext == ".tiff") {
		TIFF tiff;
		tiff.Open(path);
		tiff.ReadAny(m_master_dark);
	}

	else
		return;
}

void ImageCalibration::setMasterFlat(std::filesystem::path path) {

	if (!std::filesystem::exists(path))
		return;

	std::string ext = path.extension().string();


	if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
		FITS fits;
		fits.Open(path);
		fits.ReadAny(m_master_flat);
	}

	else if (ext == ".tif" || ext == ".tiff") {
		TIFF tiff;
		tiff.Open(path);
		tiff.ReadAny(m_master_flat);
	}

	else
		return;

	for (int ch = 0; ch < m_master_flat.Channels(); ++ch)
		m_flat_mean[ch] = m_master_flat.ComputeMean(ch);
}

void ImageCalibration::CalibrateImage(Image32& src) {

	bool dark = false, flat = false;

	if (m_master_dark.Exists() && src.IsSameDim(m_master_dark))
		dark = true;

	if (m_master_flat.Exists() && src.IsSameDim(m_master_flat))
		flat = true;

	if (dark && flat) {
		for (int ch = 0; ch < src.Channels(); ++ch)
			for (auto s = src.begin(ch), d = m_master_dark.begin(ch), f = m_master_flat.begin(ch); s != src.end(ch); ++s, ++d, ++f)
				*s = Clip(((*s - *d) * m_flat_mean[ch]) / *f);
	}

	else if (dark && !flat) {
		for (int ch = 0; ch < src.Channels(); ++ch)
			for (auto s = src.begin(ch), d = m_master_dark.begin(ch); s != src.end(ch); ++s, ++d)
				*s = Clip(*s - *d);
	}


	else if (dark && !flat) {
		for (int ch = 0; ch < src.Channels(); ++ch)
			for (auto s = src.begin(ch), f = m_master_flat.begin(ch); s != src.end(ch); ++s, ++f)
				*s = Clip((*s * m_flat_mean[ch]) / *f);
	}
}






DarkTab::DarkTab(const QSize& size, QWidget* parent) : QWidget(parent) {
	this->resize(size);
	this->setAutoFillBackground(true);

	m_dfs = new FileSelection(this);

	QLabel* label = new QLabel("Integration Method: ", this);
	label->move(10, 212);

	m_integration_combo = new QComboBox(this);
	m_integration_combo->addItems({ "Average", "Median", "Minimum", "Maximum" });
	m_integration_combo->move(150, 210);
	connect(m_integration_combo, &QComboBox::activated, [&](int index) { m_is.setIntegrationMethod(ImageStacking::Integration(index)); });
}

void DarkTab::Reset() {
	m_is = ImageStacking();
	m_dfs->onClearList();
	m_integration_combo->setCurrentIndex(int(ImageStacking::Integration::average));
}


DarkFlatTab::DarkFlatTab(const QSize& size, QWidget* parent) : QWidget(parent) {
	this->resize(size);
	this->setAutoFillBackground(true);

	m_dffs = new FileSelection(this);

	QLabel* text = new QLabel("*Having one Dark Flat file implies that it is a master frame.", this);
	text->move(10, 205);
	QFont font;
	font.setPointSize(8);
	text->setFont(font);
	

	m_integration_combo = new QComboBox(this);
	m_integration_combo->addItems({ "Average", "Median", "Minimum", "Maximum" });
	m_integration_combo->move(10, 230);
	connect(m_integration_combo, &QComboBox::activated, [&](int index) { m_is.setIntegrationMethod(ImageStacking::Integration(index)); });
}

void DarkFlatTab::Reset() {
	m_is = ImageStacking();
	m_dffs->onClearList();
	m_integration_combo->setCurrentIndex(int(ImageStacking::Integration::average));
}


FlatTab::FlatTab(const QSize& size, QWidget* parent) : QWidget(parent) {
	this->resize(size);
	this->setAutoFillBackground(true);

	m_ffs = new FileSelection(this);

	m_integration_combo = new QComboBox(this);
	m_integration_combo->addItems({ "Average", "Median", "Minimum", "Maximum" });
	m_integration_combo->move(10, 210);
	connect(m_integration_combo, &QComboBox::activated, [&](int index) { m_is.setIntegrationMethod(ImageStacking::Integration(index)); });
}

void FlatTab::Reset() {
	m_is = ImageStacking();
	m_ffs->onClearList();
	m_integration_combo->setCurrentIndex(int(ImageStacking::Integration::average));
}





using CCD = CalibrationCombinationDialog;
CCD::CalibrationCombinationDialog(QWidget* parent) :ProcessDialog("Calibration Combination", QSize(500, 400), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {
	QPalette pal;
	pal.setColor(QPalette::Window, Qt::lightGray);
	//m_file_tab->setPalette(pal);
	m_tabs = new QTabWidget(this);

	m_tabs->resize(size().width(), size().height() - 24);

	m_dark_tab = new DarkTab();
	m_tabs->addTab(m_dark_tab, "Dark");
	
	m_dflat_tab = new DarkFlatTab();
	m_tabs->addTab(m_dflat_tab, "Dark Flat");

	m_flat_tab = new FlatTab();
	m_tabs->addTab(m_flat_tab, "Flat");

	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &CCD::Apply, &CCD::showPreview, &CCD::resetDialog);

	this->show();
}

static void ReadImageFile(std::filesystem::path path, Image32& dst) {

	std::string ext = path.extension().string();
	std::string filename = path.filename().string();

	if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
		FITS fits;
		fits.Open(path);
		fits.ReadAny(dst);
		fits.Close();
	}

	else if (ext == ".tif" || ext == ".tiff") {
		TIFF tiff;
		tiff.Open(path);
		tiff.ReadAny(dst);
		tiff.Close();
	}
}

static void showMessageBox(const QString& text, const QString& informative_text) {
	QMessageBox mb;
	mb.setText(text);
	mb.setInformativeText(informative_text);
	mb.exec();
}

void CCD::resetDialog() {
	m_dark_tab->Reset();
	m_dflat_tab->Reset();
	m_flat_tab->Reset();
}

void CCD::Apply() {

	if (m_dark_tab->DarkPaths().size() == 0 && m_dflat_tab->DarkFlatPaths().size() == 0 && m_flat_tab->FlatPaths().size() == 0)
		return;

	Image32 master_frame;
	Image32 master_dflat;

	ImageWindow32* iw32;

	if (m_dark_tab->DarkPaths().size() >= 1) {
		this->setEnabledAll(false);
		Status status = m_dark_tab->DarkImageStacker().IntegrateImages(m_dark_tab->DarkPaths(), master_frame);
		if (status)
			iw32 = new ImageWindow32(master_frame, "MasterDark", reinterpret_cast<Workspace*>(m_workspace));
		else
			showMessageBox("Unable to stack Dark frames", status.m_message);
	}

	if (m_dflat_tab->DarkFlatPaths().size() > 1) {
		this->setEnabledAll(false);

		Status status = m_dflat_tab->DarkFlatImageStacker().IntegrateImages(m_dflat_tab->DarkFlatPaths(), master_frame);
		if (status)
			iw32 = new ImageWindow32(master_frame, "MasterDarkFlat", reinterpret_cast<Workspace*>(m_workspace));
		else
			showMessageBox("Unable to stack Dark Flat frames", status.m_message);
	}

	else if (m_dflat_tab->DarkFlatPaths().size() == 1)
		ReadImageFile(m_dflat_tab->DarkFlatPaths()[0], master_dflat);
	

	if (m_flat_tab->FlatPaths().size() >= 1) {
		this->setEnabledAll(false);

		FileVector paths = m_flat_tab->FlatPaths();

		if (master_dflat.Exists()) {

			TempFolder temp;

			for (auto file_it = m_flat_tab->FlatPaths().begin(); file_it != m_flat_tab->FlatPaths().end(); ++file_it) {

				ReadImageFile(*file_it, master_frame);

				if (master_frame.Matches(master_dflat)) {
					master_frame -= master_dflat;
					temp.WriteTempFits(master_frame, *file_it);
				}
			}

			paths = temp.Files();
		}

		Status status = m_flat_tab->FlatImageStacker().IntegrateImages(paths, master_frame);

		if (status.m_success)
			iw32 = new ImageWindow32(master_frame, "MasterFlat", reinterpret_cast<Workspace*>(m_workspace));
		else
			showMessageBox("Unable to stack Flat frames", status.m_message);
	}

	this->setEnabledAll(true);
}
