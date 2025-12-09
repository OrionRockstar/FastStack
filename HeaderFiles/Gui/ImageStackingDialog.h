#pragma once

#include "ProcessDialog.h"

#include "StarDetector.h"

#include "ImageStacking.h"
#include "Drizzle.h"
#include "ImageCalibration.h"

#include "ImageIntegrationProcess.h"


class StarDetectionGroupBox : public GroupBox {

	StarDetector* m_sd = nullptr;

	uint16_t m_maxstars = 200;

	DoubleInput* m_sigmaK_input = nullptr;
	DoubleInput* m_roundness_input = nullptr;

	SpinBox* m_max_star_sb = nullptr;
	ComboBox* m_psf_combo = nullptr;
	DoubleSpinBox* m_beta_sb = nullptr;

public:
	StarDetectionGroupBox(StarDetector& star_detector, QWidget* parent = nullptr, bool title = false);

	uint16_t maxStars()const { return m_maxstars; }

private:
	void addThresholdInputs();

	void addRoundnessInputs();

public:
	void reset();
};







class ImageStackingDialog:public ProcessDialog {
	Q_OBJECT

	class FileSelectionGroupBox : public GroupBox {

		const int m_button_width = 115;

		std::vector<ImageStackingFiles> m_pats;

		std::vector<std::filesystem::path> m_paths;
		std::vector<std::filesystem::path> m_alignment_paths;

		ImageCalibrator* m_calibrator;

		const QPixmap m_pix = QPixmap("./Icons//Five_Pointed_Star_Solid11x11.png");

		ListWidget* m_file_list_view = nullptr;

		PushButton* m_add_files_pb = nullptr;
		PushButton* m_remove_file_pb = nullptr;
		PushButton* m_clear_list_pb = nullptr;
		PushButton* m_add_alignment_pb = nullptr;
		PushButton* m_clear_alignment_pb = nullptr;

		CheckBox* m_dark_cb = nullptr;
		LineEdit* m_dark_file_le = nullptr;
		PushButton* m_add_dark_pb = nullptr;

		CheckBox* m_flat_cb = nullptr;
		LineEdit* m_flat_file_le = nullptr;
		PushButton* m_add_flat_pb = nullptr;


		QString m_typelist =
			"FITS file(*.fits *.fts *.fit);;";
		//"TIFF file(*.tiff *.tif);;";

	public:
		FileSelectionGroupBox(ImageCalibrator& calibrator, QWidget* parent = nullptr);

		const PathVector& lightPaths()const { return m_paths; }

		const PathVector& alignmentPaths()const { return m_alignment_paths; }

	private:
		void addFileSelection();

		void addMasterDarkSelection();

		void addMasterFlatSelection();

	};

	class IntegrationGroupBox : public GroupBox {

		ImageStacking* m_is;

		InterpolationComboBox* m_interpolation_combo = nullptr;

		ComboBox* m_integration_combo = nullptr;

		ComboBox* m_normalization_combo = nullptr;

		ComboBox* m_rejection_combo = nullptr;


		DoubleLineEdit* m_sigma_low_le = nullptr;
		Slider* m_sigma_low_slider = nullptr;

		DoubleLineEdit* m_sigma_high_le = nullptr;
		Slider* m_sigma_high_slider = nullptr;

		CheckBox* m_weight_maps = nullptr;

	public:
		IntegrationGroupBox(ImageStacking& image_stacking, QWidget* parent = nullptr);

	private:
		void addCombos();

		void addSigmaInputs();

	public:
		CheckBox* weightsCheckbox()const { return m_weight_maps; }

		void reset();
	};

	ImageIntegrationProcess m_iip;

	QPalette m_pal;

	QToolBox* m_toolbox = nullptr;

	FileSelectionGroupBox* m_fileselection_gb = nullptr;
	StarDetectionGroupBox* m_stardetection_gb = nullptr;
	IntegrationGroupBox* m_integration_gb = nullptr;

	TextDisplay* m_text = nullptr;
	Image32 m_output;

public:
	ImageStackingDialog(Workspace* parent = nullptr);

private:
	signals:
	void processFinished(Status s);

private:
	void showTextDisplay();

	void resetDialog();

	void showPreview() {}

	void apply();
};





class DrizzleIntegrationDialog : public ProcessDialog {
	Q_OBJECT

	class FileSelectionGroupBox : public GroupBox {

		ImageCalibrator* m_calibrator;

		const int m_button_width = 115;

		FileVector m_paths;
		FileVector m_alignment_paths;
		FileVector m_weightmap_paths;

		const QPixmap m_pix = QPixmap("./Icons//Five_Pointed_Star_Solid11x11.png");

		ListWidget* m_file_list_view = nullptr;

		PushButton* m_add_files_pb = nullptr;
		PushButton* m_remove_file_pb = nullptr;
		PushButton* m_clear_list_pb = nullptr;
		PushButton* m_add_alignment_pb = nullptr;
		PushButton* m_clear_alignment_pb = nullptr;
		PushButton* m_add_weights_pb = nullptr;
		PushButton* m_clear_weights_pb = nullptr;


		CheckBox* m_dark_cb = nullptr;
		LineEdit* m_dark_file_le = nullptr;
		PushButton* m_add_dark_pb = nullptr;

		CheckBox* m_flat_cb = nullptr;
		LineEdit* m_flat_file_le = nullptr;
		PushButton* m_add_flat_pb = nullptr;

		QString m_typelist =
			"FITS file(*.fits *.fts *.fit);;";

	public:
		FileSelectionGroupBox(ImageCalibrator& calibrator, QWidget* parent = nullptr);

		const PathVector& lightPaths()const { return m_paths; }

		const PathVector& alignmentPaths()const { return m_alignment_paths; }

		const PathVector& weightmapPaths()const { return m_weightmap_paths; }

	private:
		void addFileSelection();

		void addAlignmentSelection();

		void addWeightMapSelection();

		void addMasterDarkSelection();

		void addMasterFlatSelection();
	};

	class DrizzleGroupBox : public GroupBox {
		
		Drizzle* m_drizzle = nullptr;

		DoubleLineEdit* m_drop_le = nullptr;
		Slider* m_drop_slider = nullptr;

		DoubleSpinBox* m_dropsize_sb = nullptr;
		SpinBox* m_scale_factor_sb = nullptr;

	public:
		DrizzleGroupBox(Drizzle& drizzle, QWidget* parent = nullptr);
	};

	DrizzleIntegrationProcesss m_dip;

	FileSelectionGroupBox* m_fileselection_gb = nullptr;
	DrizzleGroupBox* m_drizzle_gb = nullptr;

	QToolBox* m_toolbox = nullptr;

	TextDisplay* m_text = nullptr;
	Image32 m_output;
public:
	DrizzleIntegrationDialog(Workspace* parent = nullptr);

private:
signals:
	void processFinished(Status status);

private:
	void showTextDisplay();

	void resetDialog() {}

	void showPreview() {}

	void apply();
};