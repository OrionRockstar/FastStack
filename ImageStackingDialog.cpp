#include "pch.h"
#include "FastStack.h"
#include "ImageStackingDialog.h"
#include "ImageCalibration.h"
#include "FITS.h"
#include "TIFF.h"
#include "StarMatching.h"
#include "ImageOperations.h"
#include "ImageStacking.h"
#include "ImageWindow.h"
#include "ImageGeometry.h"


void TempFolder::WriteTempFits(Image32& src, std::filesystem::path file_path) {
	FITS fits;
	fits.Create(Path().append(file_path.stem().concat("_temp.fits").string()));
	fits.Write(src, -32);
}

FileTab::FileTab(const QSize& size, QWidget* parent) {

	this->resize(size);
	this->setAutoFillBackground(true);

	AddFileSelection();
	AddMasterDarkSelection();
	AddMasterFlatSelection();

}

void FileTab::onAddLightFiles() {

	QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

	for (auto file : file_paths)
		m_file_list_view->addItem(QFileInfo(file).fileName());

	for (int i = 0; i < file_paths.size(); ++i)
		m_paths.push_back(file_paths[i].toStdString());
}

void FileTab::onRemoveFile() {

	if (m_file_list_view->count() == 0)
		return;

	int index = m_file_list_view->currentIndex().row();

	if (index == -1)
		index += m_paths.size();

	m_file_list_view->takeItem(index);
	m_paths.erase(m_paths.begin() + index);

}

void FileTab::onClearList() {

	m_file_list_view->clear();
	m_paths.clear();
}

void FileTab::onClick_dark(bool checked) {

	m_dark_file->setEnabled(checked);
	m_add_dark_pb->setEnabled(checked);
}

void FileTab::onAddDarkFrame() {

	QString file = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

	m_dark_file->setText(file);
}


void FileTab::onClick_flat(bool checked) {

	m_flat_file->setEnabled(checked);
	m_add_flat_pb->setEnabled(checked);
}

void FileTab::onAddFlatFrame() {

	QString file = QFileDialog::getOpenFileName(this, tr("Open File"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

	m_flat_file->setText(file);
}


void FileTab::AddFileSelection() {

	m_file_list_view = new QListWidget(this);
	m_file_list_view->move(10, 10);
	m_file_list_view->size().setWidth(300);

	m_file_list_view->resize(365, m_file_list_view->sizeHint().height());

	QPalette p;
	p.setBrush(QPalette::ColorRole::AlternateBase, QColor(127, 127, 255));


	//p.setBrush(QPalette::ColorRole::Base, QColor(0, 255, 0));
	m_file_list_view->setPalette(p);
	m_file_list_view->setAlternatingRowColors(true);

	m_add_files_pb = new QPushButton("Add Light Files", this);
	m_add_files_pb->move(380, 10);

	m_remove_file_pb = new QPushButton("Remove Item", this);
	m_remove_file_pb->move(380, 45);

	connect(m_add_files_pb, &QPushButton::pressed, this, &FileTab::onAddLightFiles);
	connect(m_remove_file_pb, &QPushButton::pressed, this, &FileTab::onRemoveFile);


	//QStringList f_path = QFileDialog::getOpenFileNames(this, tr("Open Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], "*.fit");
}

void FileTab::AddMasterDarkSelection() {

	m_dark_cb = new QCheckBox(this);
	m_dark_cb->setChecked(true);
	m_dark_cb->move(10, 217);

	m_dark_file = new QLineEdit(this);
	m_dark_file->resize(345, 30);
	m_dark_file->move(30, 210);

	m_add_dark_pb = new QPushButton("Master Dark", this);
	m_add_dark_pb->move(380, 210);

	connect(m_add_dark_pb, &QPushButton::pressed, this, &FileTab::onAddDarkFrame);
	connect(m_dark_cb, &QCheckBox::clicked, this, &FileTab::onClick_dark);
}

void FileTab::AddMasterFlatSelection() {

	m_flat_cb = new QCheckBox(this);
	m_flat_cb->setChecked(true);
	m_flat_cb->move(10, 257);

	m_flat_file = new QLineEdit(this);
	m_flat_file->resize(345, 30);
	m_flat_file->move(30, 250);

	m_add_flat_pb = new QPushButton("Master Flat", this);
	m_add_flat_pb->move(380, 250);

	connect(m_add_flat_pb, &QPushButton::pressed, this, &FileTab::onAddFlatFrame);
	connect(m_flat_cb, &QCheckBox::clicked, this, &FileTab::onClick_flat);
}











/*StarsTab::StarsTab(const QSize& size, QWidget* parent) {

	this->resize(size);
	this->setAutoFillBackground(true);

	AddThresholdInputs();
	AddMaxStarRadiusInputs();

	QLabel* label = new QLabel("Wavelet Layers: ", this);
	m_wavelet_layers_sb = new QSpinBox(this);
	m_wavelet_layers_sb->move(125, 10);
	m_wavelet_layers_sb->setRange(1, 6);
	m_wavelet_layers_sb->setValue(5);
	connect(m_wavelet_layers_sb, &QSpinBox::valueChanged, this, &StarsTab::onChange_waveletLayers);


	/*m_interpolation_combo = new QComboBox(this);
	m_interpolation_combo->addItem("Nearest Neighbor");
	m_interpolation_combo->addItem("Bilinear");
	m_interpolation_combo->addItem("Bicubic Spline");
	m_interpolation_combo->addItem("Bicubic B Spline");
	m_interpolation_combo->addItem("Cubic B Spline");
	m_interpolation_combo->addItem("Catmull Rom");
	m_interpolation_combo->addItem("Lanczos3");

	m_interpolation_combo->setCurrentIndex(2);

	//m_median_blur_cb = new QCheckBox("Median Blur", this);
	//m_median_blur_cb->setChecked(true);

	//m_photometry_type_combo = new QComboBox(this);
	//m_photometry_type_combo->addItems({ "Gaussian","WCG" });
}*/

StarsTab::StarsTab(StarDetector& star_detector, const QSize& size, QWidget* parent) {
	this->resize(size);
	this->setAutoFillBackground(true);

	m_sd = &star_detector;

	AddThresholdInputs();
	AddMaxStarRadiusInputs();
	AddPeakEdgeResponseInputs();
	AddNumberofStarsInputs();

	QLabel* label = new QLabel("Wavelet Layers: ", this);
	m_wavelet_layers_sb = new QSpinBox(this);
	m_wavelet_layers_sb->move(125, 10);
	m_wavelet_layers_sb->setRange(1, 6);
	m_wavelet_layers_sb->setValue(5);
	connect(m_wavelet_layers_sb, &QSpinBox::valueChanged, this, &StarsTab::onChange_waveletLayers);


	/*m_interpolation_combo = new QComboBox(this);
	m_interpolation_combo->addItem("Nearest Neighbor");
	m_interpolation_combo->addItem("Bilinear");
	m_interpolation_combo->addItem("Bicubic Spline");
	m_interpolation_combo->addItem("Bicubic B Spline");
	m_interpolation_combo->addItem("Cubic B Spline");
	m_interpolation_combo->addItem("Catmull Rom");
	m_interpolation_combo->addItem("Lanczos3");

	m_interpolation_combo->setCurrentIndex(2);*/

	//m_median_blur_cb = new QCheckBox("Median Blur", this);
	//m_median_blur_cb->setChecked(true);

	//m_photometry_type_combo = new QComboBox(this);
	//m_photometry_type_combo->addItems({ "Gaussian","WCG" });
}

void StarsTab::onSliderMoved_K(int value) {
	m_K_le->setText(QString::number(value / 100.0, 'd', (value == 1'000) ? 1 : 2));
}

void StarsTab::editingFinished_K() {

	double K = m_K_le->text().toDouble();

	if (K == 10.0)
		m_K_le->setText(QString::number(K, 'd', 1));

	m_K_slider->setValue(K * 100);
}

void StarsTab::onSliderMoved_starRadius(int value) {
	m_max_starRadius_le->setText(QString::number(value / 10));
}

void StarsTab::editingFinished_starRadius() {
	m_max_starRadius_slider->setValue(m_max_starRadius_le->text().toInt());
}

void StarsTab::onSliderMoved_peakEdge(int value) {
	m_peak_edge_response_le->setText(QString::number(value / 100.0, 'f', 2));
}


void StarsTab::AddThresholdInputs() {

	int y = 45;

	QLabel* threshold = new QLabel("Star Threhsold: ", this);
	threshold->move(10, y);

	m_K_le = new DoubleLineEdit("3.00", new DoubleValidator(0.0, 10.0, 2), this);
	m_K_le->move(125, y);
	m_K_le->resize(50, 30);

	m_K_slider = new QSlider(Qt::Horizontal, this);
	m_K_slider->move(185, y);
	m_K_slider->setRange(100, 1000);
	m_K_slider->setValue(300);
	m_K_slider->setFixedWidth(200);

	connect(m_K_slider, &QSlider::sliderMoved, this, &StarsTab::onSliderMoved_K);
	connect(m_K_le, &QLineEdit::editingFinished, this, &StarsTab::editingFinished_K);
}

void StarsTab::AddMaxStarRadiusInputs() {

	int y = 80;

	QLabel* label = new QLabel("Max Star Radius: ", this);
	label->move(10, y);

	m_max_starRadius_le = new DoubleLineEdit("32", new DoubleValidator(17, 50, 0), this);
	m_max_starRadius_le->move(125, y);
	m_max_starRadius_le->resize(50, 30);


	m_max_starRadius_slider = new QSlider(Qt::Horizontal, this);
	m_max_starRadius_slider->move(185, y);
	m_max_starRadius_slider->setRange(170, 500);
	m_max_starRadius_slider->setValue(320);
	m_max_starRadius_slider->setFixedWidth(200);

	connect(m_max_starRadius_slider, &QSlider::sliderMoved, this, &StarsTab::onSliderMoved_starRadius);
	connect(m_max_starRadius_le, &QLineEdit::editingFinished, this, &StarsTab::editingFinished_starRadius);
}

void StarsTab::AddPeakEdgeResponseInputs() {
	int y = 115;

	m_peak_edge_response_le = new DoubleLineEdit("0.65", new DoubleValidator(0.0, 1.0, 1), this);
	m_peak_edge_response_le->resize(50, 30);
	m_peak_edge_response_le->move(125, y);

	m_peak_edge_response_slider = new QSlider(Qt::Horizontal, this);
	m_peak_edge_response_slider->move(185, y);
	m_peak_edge_response_slider->setRange(0, 100);
	m_peak_edge_response_slider->setValue(65);
	m_peak_edge_response_slider->setFixedWidth(200);

	connect(m_peak_edge_response_slider, &QSlider::sliderMoved, this, &StarsTab::onSliderMoved_peakEdge);
}

void StarsTab::AddNumberofStarsInputs() {
	int y = 150;

	m_num_stars_le = new DoubleLineEdit("200", new DoubleValidator(0, 250, 0), this);
	m_num_stars_le->resize(50, 30);
	m_num_stars_le->move(125, y);


	m_num_stars_slider = new QSlider(Qt::Horizontal, this);
	m_num_stars_slider->move(185, y);
	m_num_stars_slider->setRange(500, 2500);
	m_num_stars_slider->setValue(2000);
	m_num_stars_slider->setFixedWidth(200);

	connect(m_num_stars_slider, &QSlider::sliderMoved, this, &StarsTab::onSliderMoved_starsNum);
}




ImageStackingDialog::ImageStackingDialog(QWidget* parent): ProcessDialog("ImageStacking", QSize(500,600), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent, false) {
	//this->resize(300, 600);

	QPalette pal;
	pal.setColor(QPalette::Window, Qt::lightGray);
	//m_file_tab->setPalette(pal);

	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ImageStackingDialog::Apply, &ImageStackingDialog::showPreview, &ImageStackingDialog::resetDialog);

	m_tabs = new QTabWidget(this);
	m_tabs->resize(500, 400);
	m_tabs->move(0, 30);

	m_file_tab = new FileTab();
	m_tabs->addTab(m_file_tab, "File Selection");

	m_star_tab = new StarsTab(m_sd);
	m_tabs->addTab(m_star_tab, "Detection && Alignment");

	m_tabs->setCurrentIndex(1);
	//m_tabs->addTab(new QWidget, "Detection && Matching");
	m_tabs->addTab(new QWidget, "Image Integration");


	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}



void ImageStackingDialog::Apply() {
	
	if (m_file_tab->LightPaths().size() == 0)
		return;

	this->setEnabledAll(false);

	TempFolder temp;

	Image32 img;
	FITS fits;
	StarVector ref_sv;
	ImageCalibration ic;
	StarMatching sm;
	HomographyTransformation ht;

	int ref_rows;
	int ref_cols;
	int ref_channels;

	ic.setMasterDark(m_file_tab->MasterDarkPath());
	ic.setMasterFlat(m_file_tab->MasterFlatPath());

	for (auto file_it = m_file_tab->LightPaths().begin(); file_it != m_file_tab->LightPaths().end(); ++file_it) {
		
		std::string ext = (*file_it).extension().string();

		if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
			FITS fits;
			fits.Open(*file_it);
			fits.ReadAny(img);	
		}

		else if (ext == ".tif" || ext == ".tiff") {
			TIFF tiff;
			tiff.Open(*file_it);
			tiff.ReadAny(img);
		}

		//ic.CalibrateImage(img);
		//calibrate
		if (file_it == m_file_tab->LightPaths().begin())
			ref_sv = m_sd.ApplyStarDetection(img);
		
		else {

			StarVector tgt_sv = m_sd.ApplyStarDetection(img);

			StarPairVector spv = sm.MatchStars(ref_sv, tgt_sv);


			Matrix h = Homography().ComputeHomography(spv);
\
			if (isnan(h(0, 0)))
				continue; //use to pass over bad frame

			//if (drizzle)
				//ImageOP::AlignedStats(img, img.homography, type);

			//else
			ht.setHomography(h);
			ht.Apply(img);
			//ImageOP::AlignFrame(img, img.homography, Interpolator::Type::bicubic_spline);
		}

		temp.WriteTempFits(img, *file_it);

	}

	m_is.IntegrateImages(temp.Files(), img);

	ImageWindow32* iw32 = new ImageWindow32(img, "StackedImage", reinterpret_cast<Workspace*>(m_workspace));

	this->setEnabledAll(true);

}