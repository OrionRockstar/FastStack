#include "pch.h"
#include "ImageCalibration.h"
#include "FITS.h"
#include "TIFF.h"
#include "FastStack.h"
//#include "ImageStackingDialog.h"

void ImageCalibrator::loadMasterDark() {

	if (m_master_dark.exists() || !m_apply_dark || !std::filesystem::exists(m_dark_path))
		return;

	if (FITS::isFITS(m_dark_path)) {
		FITS fits;
		fits.open(m_dark_path);
		fits.readAny(m_master_dark);
	}

	else if (TIFF::isTIFF(m_dark_path)) {
		TIFF tiff;
		tiff.open(m_dark_path);
		tiff.readAny(m_master_dark);
	}

	else
		return;
}

void ImageCalibrator::loadMasterFlat() {

	if (m_master_flat.exists() || !m_apply_flat || !std::filesystem::exists(m_flat_path))
		return;

	if (FITS::isFITS(m_flat_path)) {
		FITS fits;
		fits.open(m_flat_path);
		fits.readAny(m_master_flat);
	}

	else if (TIFF::isTIFF(m_flat_path)) {
		TIFF tiff;
		tiff.open(m_flat_path);
		tiff.readAny(m_master_flat);
	}

	else
		return;

	for (int ch = 0; ch < m_master_flat.channels(); ++ch)
		m_flat_mean[ch] = m_master_flat.computeMean(ch);
}


void ImageCalibrator::calibrateImage(Image32& src) {

	loadMasterDark();
	loadMasterFlat();

	bool dark = false, flat = false;

	if (m_apply_dark && m_master_dark.exists() && src.isSameShape(m_master_dark))
		dark = true;

	if (m_apply_flat && m_master_flat.exists() && src.isSameShape(m_master_flat))
		flat = true;

	for (int ch = 0; ch < src.channels(); ++ch)

	if (dark && flat) {
		for (int ch = 0; ch < src.channels(); ++ch)
			for (auto s = src.begin(ch), d = m_master_dark.begin(ch), f = m_master_flat.begin(ch); s != src.end(ch); ++s, ++d, ++f)
				*s = math::clip(((*s - *d) * m_flat_mean[ch]) / *f);
	}

	else if (dark && !flat) {
		for (int ch = 0; ch < src.channels(); ++ch)
			for (auto s = src.begin(ch), d = m_master_dark.begin(ch); s != src.end(ch); ++s, ++d)
				*s = math::clip(*s - *d);
	}

	else if (dark && !flat) {
		for (int ch = 0; ch < src.channels(); ++ch)
			for (auto s = src.begin(ch), f = m_master_flat.begin(ch); s != src.end(ch); ++s, ++f)
				*s = math::clip((*s * m_flat_mean[ch]) / *f);
	}
}






using CCD = CalibrationCombinationDialog;
CCD::FileSelectionGroupBox::FileSelectionGroupBox(QWidget * parent) : GroupBox(parent) {

	this->setMinimumHeight(235);
	this->setMinimumWidth(520);

	addFileSelection();
}

void CCD::FileSelectionGroupBox::addFileSelection() {

	m_file_list_view = new QListWidget(this);
	m_file_list_view->move(15, 25);
	m_file_list_view->resize(365, m_file_list_view->sizeHint().height());

	QPalette p;
	p.setBrush(QPalette::ColorRole::AlternateBase, QColor(69, 0, 169));

	m_file_list_view->setPalette(p);
	m_file_list_view->setAlternatingRowColors(true);

	m_add_files_pb = new PushButton("Add Files", this);
	m_add_files_pb->move(390, 25);
	m_add_files_pb->setFixedWidth(m_button_width);

	m_remove_file_pb = new PushButton("Remove Item", this);
	m_remove_file_pb->move(390, 65);
	m_remove_file_pb->setFixedWidth(m_button_width);

	m_clear_list_pb = new PushButton("Clear List", this);
	m_clear_list_pb->move(390, 105);
	m_clear_list_pb->setFixedWidth(m_button_width);


	auto addlightfiles = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

		for (auto file : file_paths)
			m_file_list_view->addItem(QFileInfo(file).fileName());

		for (int i = 0; i < file_paths.size(); ++i)
			m_paths.push_back(file_paths[i].toStdString());
	};

	auto removefile = [this]() {

		if (m_file_list_view->count() == 0)
			return;

		int index = m_file_list_view->currentIndex().row();

		if (index == -1)
			index += m_paths.size();

		m_file_list_view->takeItem(index);
		m_paths.erase(m_paths.begin() + index);
	};

	auto clearlist = [this]() {
		m_file_list_view->clear();
		m_paths.clear();
	};

	connect(m_add_files_pb, &QPushButton::pressed, this, addlightfiles);
	connect(m_remove_file_pb, &QPushButton::pressed, this, removefile);
	connect(m_clear_list_pb, &QPushButton::pressed, this, clearlist);
}

void CCD::FileSelectionGroupBox::reset() {

	m_clear_list_pb->pressed();
}


CCD::IntegrationGroupBox::IntegrationGroupBox(QWidget* parent) : GroupBox(parent) {

	this->setMinimumHeight(150);
	this->setMinimumWidth(520);

	addDarkCombo();
	addDarkFlatCombo();
	addFlatCombo();
}

void CCD::IntegrationGroupBox::addDarkCombo() {

	m_dark_combo = new ComboBox(this);
	m_dark_combo->addItems(m_methods);
	m_dark_combo->move(300, 25);
	m_dark_combo->addLabel(new QLabel("Dark Integration Method:   ", this));
	m_dark_combo->setCurrentIndex(int(m_dark_integration));

	connect(m_dark_combo, &QComboBox::activated, this, [this](int index) { m_dark_integration = ImageStacking::Integration(index); });
}

void CCD::IntegrationGroupBox::addDarkFlatCombo() {

	m_dflat_combo = new ComboBox(this);
	m_dflat_combo->addItems(m_methods);
	m_dflat_combo->move(300, 65);
	m_dflat_combo->addLabel(new QLabel("Dark Flat Integration Method:   ", this));
	m_dflat_combo->setCurrentIndex(int(m_dflat_integration));

	connect(m_dflat_combo, &QComboBox::activated, this, [this](int index) { m_dflat_integration = ImageStacking::Integration(index); });
}

void CCD::IntegrationGroupBox::addFlatCombo() {

	m_flat_combo = new ComboBox(this);
	m_flat_combo->addItems(m_methods);
	m_flat_combo->move(300, 105);
	m_flat_combo->addLabel(new QLabel("Flat Integration Method:   ", this));
	m_flat_combo->setCurrentIndex(int(m_flat_integration));

	connect(m_flat_combo, &QComboBox::activated, this, [this](int index) { m_flat_integration = ImageStacking::Integration(index); });
}

void CCD::IntegrationGroupBox::reset() {

	m_dark_integration = ImageStacking::Integration::median;
	m_dflat_integration = ImageStacking::Integration::median;
	m_flat_integration = ImageStacking::Integration::average;

	m_dark_combo->setCurrentIndex(int(m_dark_integration));
	m_dflat_combo->setCurrentIndex(int(m_dflat_integration));
	m_flat_combo->setCurrentIndex(int(m_flat_integration));
}


CCD::CalibrationCombinationDialog(Workspace* parent) :ProcessDialog("Calibration Combination", QSize(540, 400), parent, false) {

	m_toolbox = new QToolBox(this);
	m_toolbox->setFixedWidth(520);
	m_toolbox->move(10, 0);

	m_toolbox->setBackgroundRole(QPalette::Window);
	QPalette pal;
	pal.setColor(QPalette::ButtonText, Qt::white);
	m_toolbox->setPalette(pal);
	//m_toolbox->setPalette(QPalette(QPalette::ButtonText, Qt::white));

	m_dark_gb = new FileSelectionGroupBox();
	m_toolbox->addItem(m_dark_gb, "Dark Frame Selection");

	m_dflat_gb = new FileSelectionGroupBox();
	m_toolbox->addItem(m_dflat_gb, "Dark Flate Frame Selection");

	m_flat_gb = new FileSelectionGroupBox();
	m_toolbox->addItem(m_flat_gb, "Flat Frame Selection");

	m_integration_gb = new IntegrationGroupBox();
	m_toolbox->addItem(m_integration_gb, "Integration Selection");

	auto selected = [this](int index) {
		m_toolbox->resize(520, m_toolbox->currentWidget()->minimumHeight() + 35 * m_toolbox->count());
		this->resize(QSize(this->width(), m_toolbox->height() + 35));
	};

	connect(m_toolbox, &QToolBox::currentChanged, this, selected);
	m_toolbox->setCurrentIndex(0);
	m_toolbox->currentChanged(0);

	this->show();
}

static void ReadImageFile(std::filesystem::path path, Image32& dst) {

	auto filename = path.filename();

	if (FITS::isFITS(filename)) {
		FITS fits;
		fits.open(path);
		fits.readAny(dst);
		fits.close();
	}

	else if (TIFF::isTIFF(filename)) {
		TIFF tiff;
		tiff.open(path);
		tiff.readAny(dst);
		tiff.close();
	}
}

static void showMessageBox(const QString& text, const QString& informative_text) {

	QMessageBox mb;
	mb.setText(text);
	mb.setInformativeText(informative_text);
	mb.exec();
}

void CCD::resetDialog() {

	m_dark_gb->reset();
	m_dflat_gb->reset();
	m_flat_gb->reset();
	m_integration_gb->reset();
}

void CCD::apply() {

	Image32 master;
	ImageStacking is;

	if (m_dark_gb->filePaths().size() > 1) {

		is.setIntegrationMethod(m_integration_gb->darkIntegration());

		std::cout << m_dark_gb->filePaths()[0] << "\n";
		is.stackImages(m_dark_gb->filePaths(), master);

		ImageWindow32* iw32 = new ImageWindow32(std::move(master), "MasterDark", reinterpret_cast<Workspace*>(m_workspace));
	}

	Image32 dflat;
	if (m_dflat_gb->filePaths().size() > 1) {

		is.setIntegrationMethod(m_integration_gb->darkFlatIntegration());
		is.stackImages(m_dflat_gb->filePaths(), dflat);

		if (m_flat_gb->filePaths().size() == 0)
			ImageWindow32* iw32 = new ImageWindow32(std::move(dflat), "MasterDarkFlat", reinterpret_cast<Workspace*>(m_workspace));
	}

	else if (m_dflat_gb->filePaths().size() == 1 && m_flat_gb->filePaths().size() > 1) {
		ReadImageFile(m_dflat_gb->filePaths()[0], dflat);
	}

	if (m_flat_gb->filePaths().size() > 1) {

		is.setIntegrationMethod(m_integration_gb->flatIntegration());

		if (dflat.exists()) {
			TempFolder temp("CalibrationTemp");
			for (auto file : m_flat_gb->filePaths()) {
				ReadImageFile(file, master);
				master -= dflat;
				temp.writeTempFits(master, file);
			}

			is.stackImages(temp.filePaths(), master);
		}

		else 
			is.stackImages(m_flat_gb->filePaths(), master);

		ImageWindow32* iw32 = new ImageWindow32(std::move(master), "MasterFlat", reinterpret_cast<Workspace*>(m_workspace));
	}
}
