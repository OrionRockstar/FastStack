#include "pch.h"
#include "FastStack.h"
#include "ImageStackingDialog.h"
#include "ImageCalibration.h"
#include "FITS.h"
#include "TIFF.h"
#include "Homography.h"
#include "StarMatching.h"
#include "ImageStacking.h"
#include "ImageWindow.h"


StarDetectionGroupBox::StarDetectionGroupBox(StarDetector& star_detector, QWidget* parent, bool title) : m_sd(&star_detector), GroupBox(parent) {

	this->setFixedSize(520, 230);

	if (title)
		setTitle("Star Detection");

	addWaveletInputs();
	addThresholdInputs();
	addPeakEdgeRatioInputs();
	addRoundnessInputs();
	addMaxStarsInputs();
}

void StarDetectionGroupBox::addWaveletInputs() {

	m_wavelet_layers_sb = new SpinBox(this);
	m_wavelet_layers_sb->move(185, 25);
	m_wavelet_layers_sb->setRange(1, 6);
	m_wavelet_layers_sb->setValue(5);
	m_wavelet_layers_sb->setFixedWidth(75);
	m_wavelet_layers_sb->addLabel(new QLabel("Wavelet Layers:   ", this));

	connect(m_wavelet_layers_sb, &QSpinBox::valueChanged, this, [this](int value) { m_sd->setWaveletLayers(value); });

	m_median_blur_cb = new CheckBox("Median Blur", this);
	m_median_blur_cb->move(335, 25);
	m_median_blur_cb->setChecked(true);
	connect(m_median_blur_cb, &::QCheckBox::clicked, this, [this](bool v) {m_sd->applyMedianBlur(v); });
}

void StarDetectionGroupBox::addThresholdInputs() {
	QString txt = "Sets K value of star threshold defined as median + K * standard deviation.";

	m_sigmaK_le = new DoubleLineEdit(m_sd->sigmaK(), new DoubleValidator(0.0, 10.0, 2), this);
	m_sigmaK_le->move(185, 65);
	m_sigmaK_le->addLabel(new QLabel("Star Signal Threshold:   ", this));
	m_sigmaK_le->setToolTip(txt);

	m_sigmaK_slider = new Slider(Qt::Horizontal, this);
	m_sigmaK_slider->setFixedWidth(205);
	m_sigmaK_slider->setRange(0, 200);
	m_sigmaK_slider->setValue(20);
	m_sigmaK_le->addSlider(m_sigmaK_slider);
	m_sigmaK_slider->setToolTip(txt);

	auto action = [this](int) {
		double value = m_sigmaK_slider->sliderPosition() / 20.0;
		m_sigmaK_le->setValue(value);
		m_sd->setSigmaK(value);
	};

	auto edited = [this]() {
		double value = m_sigmaK_le->value();
		m_sigmaK_slider->setValue(value * 20);
		m_sd->setSigmaK(value);
	};

	connect(m_sigmaK_slider, &QSlider::actionTriggered, this, action);
	connect(m_sigmaK_le, &QLineEdit::editingFinished, this, edited);
}

void StarDetectionGroupBox::addPeakEdgeRatioInputs() {

	m_peak_edge_le = new DoubleLineEdit(m_sd->peakEdge(), new DoubleValidator(0.0, 1.0, 2), this);
	m_peak_edge_le->move(185, 105);
	m_peak_edge_le->addLabel(new QLabel("Peak-Edge Ratio:   ", this));

	m_peak_edge_slider = new Slider(Qt::Horizontal, this);
	m_peak_edge_slider->setFixedWidth(205);
	m_peak_edge_slider->setRange(0, 100);
	m_peak_edge_slider->setValue(m_sd->peakEdge() * 100);
	m_peak_edge_le->addSlider(m_peak_edge_slider);

	auto action = [this](int) {
		double value = m_peak_edge_slider->sliderPosition() / 100.0;
		m_peak_edge_le->setValue(value);
		m_sd->setPeakEdge(value);
	};

	auto edited = [this]() {
		double value = m_peak_edge_le->value();
		m_peak_edge_slider->setValue(value * 100);
		m_sd->setPeakEdge(value);
	};

	connect(m_peak_edge_slider, &QSlider::actionTriggered, this, action);
	connect(m_peak_edge_le, &QLineEdit::editingFinished, this, edited);
}

void StarDetectionGroupBox::addRoundnessInputs() {

	m_roundness_le = new DoubleLineEdit(m_sd->roundness(), new DoubleValidator(0.0, 1.0, 2), this);
	m_roundness_le->move(185, 145);
	m_roundness_le->addLabel(new QLabel("Roundness threshold:   ", this));

	m_roundness_slider = new Slider(Qt::Horizontal, this);
	m_roundness_slider->setFixedWidth(205);
	m_roundness_slider->setRange(0, 100);
	m_roundness_slider->setValue(m_sd->roundness() * 100);
	m_roundness_le->addSlider(m_roundness_slider);

	auto action = [this](int) {
		double value = m_roundness_slider->sliderPosition() / 100.0;
		m_roundness_le->setValue(value);
		m_sd->setRoundness(value);
	};

	auto edited = [this]() {
		double value = m_roundness_le->value();
		m_roundness_slider->setValue(value * 100);
		m_sd->setRoundness(value);
	};

	connect(m_roundness_slider, &QSlider::actionTriggered, this, action);
	connect(m_roundness_le, &QLineEdit::editingFinished, this, edited);
}

void StarDetectionGroupBox::addMaxStarsInputs() {

	m_max_stars_le = new IntLineEdit(m_maxstars, new IntValidator(50, 250), this);
	m_max_stars_le->move(185, 185);
	m_max_stars_le->addLabel(new QLabel("Stars:   ", this));

	m_max_stars_slider = new Slider(Qt::Horizontal, this);
	m_max_stars_slider->setFixedWidth(205);
	m_max_stars_slider->setRange(50, 250);
	m_max_stars_slider->setValue(m_maxstars);
	m_max_stars_le->addSlider(m_max_stars_slider);

	auto action = [this](int) {
		double value = m_max_stars_slider->sliderPosition();
		m_max_stars_le->setValue(value);
		m_maxstars = value;
	};

	connect(m_max_stars_slider, &QSlider::actionTriggered, this, action);
}

void StarDetectionGroupBox::reset() {

	m_wavelet_layers_sb->setValue(m_sd->waveletLayers());
	m_median_blur_cb->setChecked(m_sd->medianBlur());

	m_sigmaK_le->setValue(m_sd->sigmaK());
	m_sigmaK_le->editingFinished();

	m_peak_edge_le->setValue(m_sd->peakEdge());
	m_peak_edge_le->editingFinished();

	m_roundness_le->setValue(m_sd->roundness());
	m_roundness_le->editingFinished();

	m_max_stars_le->setValue(200);
	m_max_stars_le->editingFinished();
}




ImageStackingDialog::FileSelectionGroupBox::FileSelectionGroupBox(ImageCalibrator& calibrator, QWidget* parent) : m_calibrator(&calibrator), GroupBox(parent) {

	this->setMinimumHeight(315);
	this->setMaximumWidth(520);


	addFileSelection();
	addMasterDarkSelection();
	addMasterFlatSelection();
}

void ImageStackingDialog::FileSelectionGroupBox::addFileSelection() {

	m_file_list_view = new ListWidget(this);
	m_file_list_view->move(15, 25);
	m_file_list_view->resize(365, m_file_list_view->sizeHint().height());

	m_add_files_pb = new PushButton("Add Light Files", this);
	m_add_files_pb->move(390, 25);
	m_add_files_pb->setFixedWidth(m_button_width);

	m_remove_file_pb = new PushButton("Remove Item", this);
	m_remove_file_pb->move(390, 65);
	m_remove_file_pb->setFixedWidth(m_button_width);

	m_clear_list_pb = new PushButton("Clear List", this);
	m_clear_list_pb->move(390, 105);
	m_clear_list_pb->setFixedWidth(m_button_width);

	m_add_alignment_pb = new PushButton("Add Alignment", this);
	m_add_alignment_pb->move(390, 145);
	m_add_alignment_pb->setFixedWidth(m_button_width);

	m_clear_alignment_pb = new PushButton("Clear Alignment", this);
	m_clear_alignment_pb->move(390, 185);
	m_add_alignment_pb->setFixedWidth(m_button_width);

	auto addlightfiles = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Light Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

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

	auto alignment = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Alignment Files"), QString(), "INFO file(*.info)");

		for (const auto file : file_paths) {
			auto path = alignmentLightPath(file.toStdString());
			for (int i = 0; i < m_paths.size(); ++i) {
				if (path == m_paths[i]) {
					m_file_list_view->item(i)->setIcon(m_pix);
					m_alignment_paths.push_back(file.toStdString());
				}
			}
		}
	};

	auto clearalignment = [this]() {

		for (int i = 0; i < m_file_list_view->count(); ++i)
			m_file_list_view->item(i)->setIcon(QPixmap());

		m_alignment_paths.clear();
	};

	connect(m_add_files_pb, &QPushButton::pressed, this, addlightfiles);
	connect(m_remove_file_pb, &QPushButton::pressed, this, removefile);
	connect(m_clear_list_pb, &QPushButton::pressed, this, clearlist);
	connect(m_add_alignment_pb, &QPushButton::pressed, this, alignment);
	connect(m_clear_alignment_pb, &QPushButton::pressed, this, clearalignment);
}

void ImageStackingDialog::FileSelectionGroupBox::addMasterDarkSelection() {

	m_dark_cb = new CheckBox("", this);
	//m_dark_cb->setChecked(true);
	m_dark_cb->move(10, 237);

	m_dark_file_le = new LineEdit(this);
	m_dark_file_le->resize(345, 30);
	m_dark_file_le->move(35, 230);

	m_add_dark_pb = new PushButton("Master Dark", this);
	m_add_dark_pb->move(390, 230);
	m_add_dark_pb->setFixedWidth(m_button_width);

	auto dark = [this]() {
		QString file = QFileDialog::getOpenFileName(this, tr("Master Dark"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);
		m_dark_file_le->setText(file);
		m_calibrator->setMasterDarkPath(file.toStdString());
	};

	auto click = [this](bool v) {
		m_dark_file_le->setEnabled(v);
		m_add_dark_pb->setEnabled(v);
		m_calibrator->setApplyMasterDark(v);
	};

	connect(m_add_dark_pb, &QPushButton::pressed, this, dark);
	connect(m_dark_cb, &QCheckBox::clicked, this, click);
	m_dark_cb->clicked();
}

void ImageStackingDialog::FileSelectionGroupBox::addMasterFlatSelection() {

	m_flat_cb = new CheckBox("", this);
	m_flat_cb->move(10, 277);

	m_flat_file_le = new LineEdit(this);
	m_flat_file_le->resize(345, 30);
	m_flat_file_le->move(35, 270);

	m_add_flat_pb = new PushButton("Master Flat", this);
	m_add_flat_pb->move(390, 270);
	m_add_flat_pb->setFixedWidth(m_button_width);

	auto flat = [this]() {
		QString file = QFileDialog::getOpenFileName(this, tr("Master Flat"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);
		m_flat_file_le->setText(file);
		m_calibrator->setMasterFlatPath(file.toStdString());
	};

	auto click = [this](bool v) {
		m_flat_file_le->setEnabled(v);
		m_add_flat_pb->setEnabled(v);
		m_calibrator->setApplyMasterFlat(v);
	};

	connect(m_add_flat_pb, &QPushButton::pressed, this, flat);
	connect(m_flat_cb, &QCheckBox::clicked, this, click);
	m_flat_cb->clicked();
}


ImageStackingDialog::IntegrationGroupBox::IntegrationGroupBox(ImageStacking& image_stacking, QWidget* parent) : m_is(&image_stacking), GroupBox(parent) {
	
	this->setFixedSize(520, 300);

	addCombos();
	addSigmaInputs();

	m_weight_maps = new CheckBox("Generate Weight Maps", this);
	m_weight_maps->move(160, 265);
	//m_weight_maps->setChecked(false);
}

void ImageStackingDialog::IntegrationGroupBox::addCombos() {

	m_interpolation_combo = new InterpolationComboBox(this);
	m_interpolation_combo->move(195, 25);
	m_interpolation_combo->addLabel(new QLabel("Interpolation:   ", this));
	connect(m_interpolation_combo, &QComboBox::activated, this, [this](int index) {});

	m_integration_combo = new ComboBox(this);
	m_integration_combo->move(195, 65);
	m_integration_combo->addLabel(new QLabel("Integration Method:   ", this));
	m_integration_combo->addItems({ "Average", "Median", "Max", "Min" });
	connect(m_integration_combo, &QComboBox::activated, this, [this](int index) { m_is->setIntegrationMethod(ImageStacking::Integration(index)); });

	m_normalization_combo = new ComboBox(this);
	m_normalization_combo->move(195, 105);
	m_normalization_combo->addLabel(new QLabel("Normalization Method:   ", this));
	m_normalization_combo->addItems({ "No Normalization","Additive", "Multiplicative", "Additive Scaling", " Multiplicative Scaling" });
	connect(m_normalization_combo, &QComboBox::activated, this, [this](int index) { m_is->setNormalation(ImageStacking::Normalization(index)); });

	m_rejection_combo = new ComboBox(this);
	m_rejection_combo->move(195, 145);
	m_rejection_combo->addLabel(new QLabel("Pixel Rejection:   ", this));
	m_rejection_combo->addItems({ "No Rejection", "Sigma Clipping", "Winsorized Sigma Clipping" });
	connect(m_rejection_combo, &QComboBox::activated, this, [this](int index) { m_is->setRejectionMethod(ImageStacking::Rejection(index)); });
}

void ImageStackingDialog::IntegrationGroupBox::addSigmaInputs() {

	int width = 60;

	m_sigma_low_le = new DoubleLineEdit(m_is->sigmaLow(), new DoubleValidator(0.0, 10.0, 1), this);
	m_sigma_low_le->move(195, 185);
	m_sigma_low_le->setFixedWidth(width);
	m_sigma_low_le->addLabel(new QLabel("Sigma Low:   ", this));

	m_sigma_low_slider = new Slider(Qt::Horizontal, this);
	m_sigma_low_slider->setFixedWidth(205);
	m_sigma_low_slider->setRange(0, 100);
	m_sigma_low_slider->setValue(m_is->sigmaLow() * 10);
	m_sigma_low_le->addSlider(m_sigma_low_slider);

	auto action_low = [this](int) {
		float v = m_sigma_low_slider->sliderPosition() / 10.0;
		m_sigma_low_le->setValue(v);
		m_is->setSigmaLow(v);
	};

	auto edited_low = [this]() {
		float v = m_sigma_low_le->valuef();
		m_sigma_low_slider->setSliderPosition(v * 10);
		m_is->setSigmaLow(v);
	};

	connect(m_sigma_low_slider, &QSlider::actionTriggered, this, action_low);
	connect(m_sigma_low_le, &QLineEdit::editingFinished, this, edited_low);


	m_sigma_high_le = new DoubleLineEdit(m_is->sigmaHigh(), new DoubleValidator(0.0, 10.0, 1), this);
	m_sigma_high_le->move(195, 225);
	m_sigma_high_le->setFixedWidth(width);
	m_sigma_high_le->addLabel(new QLabel("Sigma High:   ", this));

	m_sigma_high_slider = new Slider(Qt::Horizontal, this);
	m_sigma_high_slider->setFixedWidth(205);
	m_sigma_high_slider->setRange(0, 100);
	m_sigma_high_slider->setValue(m_is->sigmaHigh() * 10);
	m_sigma_high_le->addSlider(m_sigma_high_slider);

	auto action_high = [this](int) {
		float v = m_sigma_high_slider->sliderPosition() / 10.0;
		m_sigma_high_le->setValue(v);
		m_is->setSigmaHigh(v);
	};

	auto edited_high = [this]() {
		float v = m_sigma_high_le->valuef();
		m_sigma_high_slider->setSliderPosition(v * 10);
		m_is->setSigmaHigh(v);
	};

	connect(m_sigma_high_slider, &QSlider::actionTriggered, this, action_high);
	connect(m_sigma_high_le, &QLineEdit::editingFinished, this, edited_high);
}

void ImageStackingDialog::IntegrationGroupBox::reset() {

}





ImageStackingDialog::ImageStackingDialog(QWidget* parent): ProcessDialog("ImageStacking", QSize(540,500), FastStack::recast(parent)->workspace(), false, false) {

	connectToolbar(this, &ImageStackingDialog::apply, &ImageStackingDialog::showPreview, &ImageStackingDialog::resetDialog);

	m_toolbox = new QToolBox(drawArea());
	m_toolbox->setFixedWidth(520);
	m_toolbox->move(10, 0);

	m_toolbox->setBackgroundRole(QPalette::Window);
	QPalette pal;
	pal.setColor(QPalette::ButtonText, Qt::white);
	m_toolbox->setPalette(pal);

	m_fileselection_gb = new FileSelectionGroupBox(m_iip.imageCalibrator());
	m_toolbox->addItem(m_fileselection_gb, "File Selection");


	m_stardetection_gb = new StarDetectionGroupBox(m_iip.starDetector());
	m_toolbox->addItem(m_stardetection_gb, "Star Detection");

	m_integration_gb = new IntegrationGroupBox(m_iip.imageStacker());
	m_toolbox->addItem(m_integration_gb, "Alignment && Integration");
	connect(m_integration_gb->weightsCheckbox(), &QCheckBox::clicked, this, [this](bool v) { m_iip.setGenerateWeightMaps(v); });

	auto selected = [this](int index) {
		m_toolbox->resize(520, m_toolbox->currentWidget()->minimumHeight() + 35 * m_toolbox->count());
		resizeDialog({ m_toolbox->width() + 20, m_toolbox->height() + 15});
	};

	connect(m_toolbox, &QToolBox::currentChanged, this, selected);
	m_toolbox->setCurrentIndex(0);
	m_toolbox->currentChanged(0);

	this->show();
}

void ImageStackingDialog::showTextDisplay() {

	if (m_text == nullptr) {
		m_text = new TextDisplay("Image Stacking Info", this);
		connect(m_text, &TextDisplay::onClose, this, [this]() { m_text = nullptr; });
		connect(m_iip.imageStackingSignal(), &ImageStackingSignal::emitText, m_text, &TextDisplay::displayText);
		connect(m_iip.imageStackingSignal(), &ImageStackingSignal::emitPSFData, m_text, &TextDisplay::displayPSFData);
		connect(m_iip.imageStackingSignal(), &ImageStackingSignal::emitMatrix, m_text, &TextDisplay::displayMatrix);
		connect(m_iip.imageStacker().imageStackingSignal(), &ImageStackingSignal::emitText, m_text, &TextDisplay::displayText);
		connect(m_iip.imageStacker().imageStackingSignal(), &ImageStackingSignal::emitProgress, m_text, &TextDisplay::displayProgress);
	}
}

void ImageStackingDialog::resetDialog() {
	m_iip.starDetector() = StarDetector();
	m_stardetection_gb->reset();
}

void ImageStackingDialog::apply() {

	this->setEnabled(false);

	showTextDisplay();

	//m_iip.setLightPaths(m_fileselection_gb->lightPaths());
	//m_iip.setAlignmentPaths(m_fileselection_gb->alignmentPaths());

	m_iip.setPaths(m_fileselection_gb->lightPaths(), m_fileselection_gb->alignmentPaths());
	m_iip.setMaxStars(m_stardetection_gb->maxStars());

	auto funca = [this]() {

		Status s = m_iip.integrateImages(m_output);
		emit processFinished(s);
	};

	auto funcb = [this](Status status) {

		if (!status)
			QMessageBox::about(this, "FastStack", status.m_message);
		
		else
			ImageWindow32* iw32 = new ImageWindow32(std::move(m_output), "StackedImage", reinterpret_cast<Workspace*>(m_workspace));

		if (m_output.exists())
			m_output.~Image();

		this->setEnabled(true);
	};

	if (!isSignalConnected(QMetaMethod::fromSignal(&ImageStackingDialog::processFinished)))
		connect(this, &ImageStackingDialog::processFinished, this, funcb);
	
	//QtConcurrent::run(funca);
	//QThread::create(funca)->start();
	std::thread(funca).detach();	
}





DrizzleIntegrationDialog::FileSelectionGroupBox::FileSelectionGroupBox(ImageCalibrator& calibrator, QWidget* parent) : m_calibrator(&calibrator), GroupBox(parent) {

	this->setFixedSize(520, 390);

	addFileSelection();
	addAlignmentSelection();
	addWeightMapSelection();
	addMasterDarkSelection();
	addMasterFlatSelection();
}

void DrizzleIntegrationDialog::FileSelectionGroupBox::addFileSelection() {

	m_file_list_view = new ListWidget(this);
	m_file_list_view->move(15, 25);
	m_file_list_view->resize(365, 270);


	m_add_files_pb = new PushButton("Add Light Files", this);
	m_add_files_pb->move(390, 25);
	m_add_files_pb->setFixedWidth(m_button_width);

	m_remove_file_pb = new PushButton("Remove Item", this);
	m_remove_file_pb->move(390, 65);
	m_remove_file_pb->setFixedWidth(m_button_width);

	m_clear_list_pb = new PushButton("Clear List", this);
	m_clear_list_pb->move(390, 105);
	m_clear_list_pb->setFixedWidth(m_button_width);

	auto addlightfiles = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Light Files"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);

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

	connect(m_add_files_pb, &QPushButton::released, this, addlightfiles);
	connect(m_remove_file_pb, &QPushButton::released, this, removefile);
	connect(m_clear_list_pb, &QPushButton::released, this, clearlist);
}

void DrizzleIntegrationDialog::FileSelectionGroupBox::addAlignmentSelection() {

	m_add_alignment_pb = new PushButton("Add Alignment", this);
	m_add_alignment_pb->move(390, 145);
	m_add_alignment_pb->setFixedWidth(m_button_width);

	m_clear_alignment_pb = new PushButton("Clear Alignment", this);
	m_clear_alignment_pb->move(390, 185);
	m_add_alignment_pb->setFixedWidth(m_button_width);

	auto alignment = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open Alignment Files"), QString(), "INFO file(*.info)");

		for (const auto file : file_paths) {
			auto path = alignmentLightPath(file.toStdString());
			for (int i = 0; i < m_paths.size(); ++i) {
				if (path == m_paths[i]) {
					m_file_list_view->item(i)->setIcon(m_pix);
					m_alignment_paths.push_back(file.toStdString());
					break;
				}
			}
		}
	};

	auto clearalignment = [this]() {

		for (int i = 0; i < m_file_list_view->count(); ++i)
			m_file_list_view->item(i)->setIcon(QPixmap());

		m_alignment_paths.clear();
	};

	connect(m_add_alignment_pb, &QPushButton::released, this, alignment);
	connect(m_clear_alignment_pb, &QPushButton::released, this, clearalignment);
}

void DrizzleIntegrationDialog::FileSelectionGroupBox::addWeightMapSelection() {

	m_add_weights_pb = new PushButton("Add Weights", this);
	m_add_weights_pb->move(390, 225);
	m_add_weights_pb->setFixedWidth(m_button_width);

	m_clear_weights_pb = new PushButton("Clear Weights", this);
	m_clear_weights_pb->move(390, 265);
	m_clear_weights_pb->setFixedWidth(m_button_width);

	auto weights = [this]() {

		QStringList file_paths = QFileDialog::getOpenFileNames(this, tr("Open WieghtMap Files"), QString(), "WMI file(*.wmi);;");

		for (const auto file : file_paths) {
			std::filesystem::path path = file.toStdString();
			auto name = path.stem();
			for (int i = 0; i < m_paths.size(); ++i) {
				if (name == m_paths[i].stem()) {
					m_file_list_view->item(i)->setText("<w>" + m_file_list_view->item(i)->text());
					m_weightmap_paths.emplace_back(path);
					break;
				}
			}
		}
	};

	auto clearwights = [this]() {

		for (int i = 0; i < m_file_list_view->count(); ++i)
			m_file_list_view->item(i)->setText(m_file_list_view->item(i)->text().sliced(3));

		m_weightmap_paths.clear();
	};

	connect(m_add_weights_pb, &QPushButton::released, this, weights);
	connect(m_clear_weights_pb, &QPushButton::released, this, clearwights);
}

void DrizzleIntegrationDialog::FileSelectionGroupBox::addMasterDarkSelection() {

	m_dark_cb = new CheckBox("", this);
	m_dark_cb->move(15, 312);

	m_dark_file_le = new LineEdit(this);
	m_dark_file_le->resize(340, 30);
	m_dark_file_le->move(40, 305);

	m_add_dark_pb = new PushButton("Master Dark", this);
	m_add_dark_pb->setFixedWidth(m_button_width);
	m_add_dark_pb->move(390, 305);

	auto dark = [this]() {
		QString file = QFileDialog::getOpenFileName(this, tr("Master Dark"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);
		m_dark_file_le->setText(file);
		m_calibrator->setMasterDarkPath(file.toStdString());
	};

	auto click = [this](bool v) {
		m_dark_file_le->setEnabled(v);
		m_add_dark_pb->setEnabled(v);
		m_calibrator->setApplyMasterDark(v);
	};

	connect(m_add_dark_pb, &QPushButton::pressed, this, dark);
	connect(m_dark_cb, &QCheckBox::clicked, this, click);
	m_dark_cb->clicked();
}

void DrizzleIntegrationDialog::FileSelectionGroupBox::addMasterFlatSelection() {

	m_flat_cb = new CheckBox("", this);
	m_flat_cb->move(15, 352);

	m_flat_file_le = new LineEdit(this);
	m_flat_file_le->resize(340, 30);
	m_flat_file_le->move(40, 345);

	m_add_flat_pb = new PushButton("Master Flat", this);
	m_add_flat_pb->move(390, 345);
	m_add_flat_pb->setFixedWidth(m_button_width);

	auto flat = [this]() {
		QString file = QFileDialog::getOpenFileName(this, tr("Master Flat"), QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)[0], m_typelist);
		m_flat_file_le->setText(file);
		m_calibrator->setMasterFlatPath(file.toStdString());
	};

	auto click = [this](bool v) {
		m_flat_file_le->setEnabled(v);
		m_add_flat_pb->setEnabled(v);
		m_calibrator->setApplyMasterFlat(v);
	};

	connect(m_add_flat_pb, &QPushButton::pressed, this, flat);
	connect(m_flat_cb, &QCheckBox::clicked, this, click);
	m_flat_cb->clicked();
}




DrizzleIntegrationDialog::DrizzleGroupBox::DrizzleGroupBox(Drizzle& drizzle, QWidget* parent) : m_drizzle(&drizzle), GroupBox(parent) {

	this->setFixedSize(520, 70);

	m_dropsize_sb = new DoubleSpinBox(m_drizzle->dropSize(), 0.10, 1.0, 2, this);
	m_dropsize_sb->setSingleStep(0.01);
	m_dropsize_sb->move(160, 25);
	m_dropsize_sb->addLabel(new QLabel("Drop Size:   ", this));

	connect(m_dropsize_sb, &QDoubleSpinBox::valueChanged, this, [this](double v) { m_drizzle->setDropSize(v); });

	m_scale_factor_sb = new SpinBox(m_drizzle->scaleFactor(), 2, 8, this);
	m_scale_factor_sb->move(360, 25);
	m_scale_factor_sb->addLabel(new QLabel("Scale Factor:   ", this));

	connect(m_scale_factor_sb, &QSpinBox::valueChanged, this, [this](int v) { m_drizzle->setScaleFactor(v); });
}




DrizzleIntegrationDialog::DrizzleIntegrationDialog(QWidget* parent) : ProcessDialog("Drizzle Integration", QSize(540,500), FastStack::recast(parent)->workspace(), false, false) {

	connectToolbar(this, &DrizzleIntegrationDialog::apply, &DrizzleIntegrationDialog::showPreview, &DrizzleIntegrationDialog::resetDialog);

	m_toolbox = new QToolBox(drawArea());
	m_toolbox->setFixedWidth(520);
	m_toolbox->move(10, 0);

	m_toolbox->setBackgroundRole(QPalette::Window);
	QPalette pal;
	pal.setColor(QPalette::ButtonText, Qt::white);
	m_toolbox->setPalette(pal);

	auto selected = [this](int index) {
		m_toolbox->resize(520, m_toolbox->currentWidget()->minimumHeight() + 35 * m_toolbox->count());
		resizeDialog({ m_toolbox->width() + 20, m_toolbox->height() + 15 });
	};

	connect(m_toolbox, &QToolBox::currentChanged, this, selected);

	m_fileselection_gb = new FileSelectionGroupBox(m_dip.imageCalibrator(), this);
	m_toolbox->addItem(m_fileselection_gb, "File Selection");

	m_drizzle_gb = new DrizzleGroupBox(m_dip.drizzle(), this);
	m_toolbox->addItem(m_drizzle_gb, "Drizzle Settings");

	selected(0);
	this->show();
}

void DrizzleIntegrationDialog::showTextDisplay() {

	if (m_text == nullptr) {
		m_text = new TextDisplay("Drizzle Info", this);
		connect(m_text, &TextDisplay::onClose, this, [this]() { m_text = nullptr; });
		connect(m_dip.imageStackingSignal(), &ImageStackingSignal::emitText, m_text, &TextDisplay::displayText);
		connect(m_dip.imageStackingSignal(), &ImageStackingSignal::emitProgress, m_text, &TextDisplay::displayProgress);
	}
}

void DrizzleIntegrationDialog::apply() {

	this->setEnabled(false);

	showTextDisplay();

	m_dip.setLightPaths(m_fileselection_gb->lightPaths());
	m_dip.setAlignmentPaths(m_fileselection_gb->alignmentPaths());
	m_dip.setWeightPaths(m_fileselection_gb->weightmapPaths());

	auto funca = [this]() {

		Status s = m_dip.drizzleImages(m_output);
		emit processFinished(s);
	};

	auto funcb = [this](Status status) {

		if (!status)
			QMessageBox::about(this, "FastStack", status.m_message);

		else
			//increment title
			ImageWindow32* iw32 = new ImageWindow32(std::move(m_output), "DrizzledImage", reinterpret_cast<Workspace*>(m_workspace));

		if (m_output.exists())
			m_output.~Image();

		this->setEnabled(true);
	};

	if (!isSignalConnected(QMetaMethod::fromSignal(&DrizzleIntegrationDialog::processFinished)))
		connect(this, &DrizzleIntegrationDialog::processFinished, this, funcb);

	std::thread(funca).detach();
}