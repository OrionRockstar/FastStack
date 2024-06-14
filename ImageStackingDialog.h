#pragma once

#include "ProcessDialog.h"
#include "Image.h"
#include "Matrix.h"
#include "StarDetector.h"
#include "StarMatching.h"
#include "Homography.h"
#include "Interpolator.h"
#include "ImageStacking.h"

class TempFolder {
	std::filesystem::path m_temp_path = std::filesystem::temp_directory_path().append("FastStackTemp");

public:
	TempFolder() {

		if (std::filesystem::exists(m_temp_path))
			std::filesystem::remove_all(m_temp_path);

		std::filesystem::create_directory(m_temp_path);
	}

	~TempFolder() {

		if (std::filesystem::exists(m_temp_path))
			std::filesystem::remove_all(m_temp_path);
		
	}

	std::filesystem::path Path() const { return m_temp_path; }

	FileVector Files()const {
		FileVector fv;
		for (auto file : std::filesystem::directory_iterator(m_temp_path))
			fv.push_back(file);
		return fv;
	}

	void WriteTempFits(Image32& src, std::filesystem::path file_path);
};

class FileTab : public QWidget {

	std::vector<std::filesystem::path> m_paths;

	QPushButton* m_add_files_pb;
	QPushButton* m_remove_file_pb;
	QPushButton* m_clear_list;

	QListWidget* m_file_list_view;

	QCheckBox* m_dark_cb;
	QLineEdit* m_dark_file;
	QPushButton* m_add_dark_pb;

	QCheckBox* m_flat_cb;
	QLineEdit* m_flat_file;
	QPushButton* m_add_flat_pb;

	QString m_typelist =
		"FITS file(*.fits *.fts *.fit);;"
		"XISF file(*.xisf);;"
		"TIFF file(*.tiff *.tif);;";

public:
	FileTab(const QSize& size = QSize(500,400), QWidget* parent = nullptr);

	const std::vector<std::filesystem::path>& LightPaths()const { return m_paths; }

	std::filesystem::path MasterDarkPath() { return m_dark_file->text().toStdString(); }

	std::filesystem::path MasterFlatPath() { return m_flat_file->text().toStdString(); }

private:
	void onAddLightFiles();

	void onRemoveFile();

	void onClearList();


	void onClick_dark(bool checked);

	void onAddDarkFrame();


	void onClick_flat(bool checked);

	void onAddFlatFrame();

	void AddFileSelection();

	void AddMasterDarkSelection();

	void AddMasterFlatSelection();

	void AddMasterBiasSelection();
};

class StarsTab : public QWidget {
	StarDetector* m_sd;

	DoubleLineEdit* m_K_le;
	QSlider* m_K_slider;

	DoubleLineEdit* m_max_starRadius_le;
	QSlider* m_max_starRadius_slider;

	QCheckBox* m_median_blur_cb;
	QSpinBox* m_wavelet_layers_sb;

	QComboBox* m_photometry_type_combo;

	QComboBox* m_interpolation_combo;

	DoubleLineEdit* m_peak_edge_response_le;
	QSlider* m_peak_edge_response_slider;

	DoubleLineEdit* m_num_stars_le;
	QSlider* m_num_stars_slider;


public:
	//StarsTab(const QSize& size = QSize(500, 400), QWidget* parent = nullptr);

	StarsTab(StarDetector& star_detector, const QSize& size = QSize(500, 400), QWidget* parent = nullptr);

	Interpolator::Type InterpolationMethod()const { return Interpolator::Type(m_interpolation_combo->currentIndex()); }

private:
	void onSliderMoved_K(int value);

	void editingFinished_K();

	void onSliderMoved_starRadius(int value);

	void editingFinished_starRadius();

	void onChange_waveletLayers(int value) {
		value = pow(2, value - 1);
		//m_sd->setWaveletLayers(val);
		m_max_starRadius_slider->setMinimum((value + 1) * 10);
		m_max_starRadius_le->setText(QString::number(m_max_starRadius_slider->value() / 10));
	}

	void onSliderMoved_peakEdge(int value);

	void onSliderMoved_starsNum(int value) {
		//std::cout << m_num_stars_slider->value() << " " << m_num_stars_slider->sliderPosition() << "\n";
		m_num_stars_le->setText(QString::number(value/10));
	}


	void AddThresholdInputs();

	void AddMaxStarRadiusInputs();

	void AddEdgeBrightnessInputs();

	void AddPeakEdgeResponseInputs();

	void AddNumberofStarsInputs();
};

class ImageIntegrationTab : public QWidget {

	ImageStacking* m_is;

	QComboBox* m_integration_combo;

	QComboBox* m_normalization_combo;

	QComboBox* m_rejection_combo;

	DoubleLineEdit* m_sigma_low_le;
	QSlider* m_sigma_low_slider;

	DoubleLineEdit* m_sigma_high_le;
	QSlider* m_sigma_high_slider;

};




class ImageStackingDialog:public ProcessDialog {

	StarDetector m_sd;
	ImageStacking m_is;

	QTabWidget* m_tabs;
	FileTab* m_file_tab;
	StarsTab* m_star_tab;

public:

	ImageStackingDialog(QWidget* parent = nullptr);

private:

	void resetDialog() {}

	void showPreview() {}

	void Apply();

};

