#pragma once
#include "Image.h"
#include "ImageStacking.h"
#include "ProcessDialog.h"

class ImageCalibrator {

	std::filesystem::path m_dark_path;
	std::filesystem::path m_flat_path;

	Image32 m_master_dark;
	Image32 m_master_flat;

	bool m_apply_dark = true;
	bool m_apply_flat = true;

	std::array<float, 3> m_flat_mean = { 0.0,0.0,0.0 };

	void loadMasterDark();

	void loadMasterFlat();
public:
	ImageCalibrator() = default;

	/*ImageCalibrator operator=(const ImageCalibrator& other) {
		m_dark_path = other.m_dark_path;
		m_flat_path = other.m_flat_path;

		m_apply_dark = other.m_apply_dark;
		m_apply_flat = other.m_apply_flat;

		return *this;
	}*/


	void setMasterDarkPath(const std::filesystem::path& dark_path) { m_dark_path = dark_path; }

	void setMasterFlatPath(const std::filesystem::path& flat_path) { m_flat_path = flat_path; }

	bool applyMasterDark()const { return m_apply_dark; }

	void setApplyMasterDark(bool apply) { m_apply_dark = apply; }

	bool applyMasterFlat()const { return m_apply_flat; }

	void setApplyMasterFlat(bool apply) { m_apply_flat = apply; }

	void calibrateImage(Image32& src);
};










class CalibrationCombinationDialog : public ProcessDialog {

	class FileSelectionGroupBox : public GroupBox {

		const int m_button_width = 115;

		std::vector<std::filesystem::path> m_paths;

		QListWidget* m_file_list_view;

		PushButton* m_add_files_pb;
		PushButton* m_remove_file_pb;
		PushButton* m_clear_list_pb;

		QString m_typelist =
			"FITS file(*.fits *.fts *.fit);;";
		//"TIFF file(*.tiff *.tif);;";

	public:
		FileSelectionGroupBox(QWidget* parent = nullptr);

		const std::vector<std::filesystem::path>& filePaths()const { return m_paths; }

	private:
		void addFileSelection();

	public:
		void reset();
	};

	class IntegrationGroupBox : public GroupBox {

		const QStringList m_methods = { "Average","Median","Min", "Max" };

		ComboBox* m_dark_combo = nullptr;
		ComboBox* m_dflat_combo = nullptr;
		ComboBox* m_flat_combo = nullptr;

		ImageStacking::Integration m_dark_integration = ImageStacking::Integration::median;
		ImageStacking::Integration m_dflat_integration = ImageStacking::Integration::median;
		ImageStacking::Integration m_flat_integration = ImageStacking::Integration::average;

	public:
		IntegrationGroupBox(QWidget* parent = nullptr);

		ImageStacking::Integration darkIntegration()const { return m_dark_integration; }

		ImageStacking::Integration darkFlatIntegration()const { return m_dflat_integration; }

		ImageStacking::Integration flatIntegration()const { return m_flat_integration; }

	private:
		void addDarkCombo();

		void addDarkFlatCombo();

		void addFlatCombo();

	public:
		void reset();
	};

	ImageStacking m_is;

	QToolBox* m_toolbox = nullptr;

	FileSelectionGroupBox* m_dark_gb = nullptr;
	FileSelectionGroupBox* m_dflat_gb = nullptr;
	FileSelectionGroupBox* m_flat_gb = nullptr;
	IntegrationGroupBox* m_integration_gb = nullptr;

public:
	CalibrationCombinationDialog(Workspace* parent = nullptr);

private:

	void resetDialog();

	void showPreview() {}

	void apply();
};