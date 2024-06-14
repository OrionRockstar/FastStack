#pragma once
#include "Image.h"
#include "ImageStacking.h"
#include "ProcessDialog.h"

class ImageCalibration {
	Image32 m_master_dark;
	Image32 m_master_flat;
	std::array<float, 3> m_flat_mean = { 0.0,0.0,0.0 };

public:
	
	void setMasterDark(std::filesystem::path path);

	void setMasterFlat(std::filesystem::path path);

	void removeMasterDark() {
		m_master_dark.~Image();
	}

	
	void CalibrateImage(Image32& src);
};

class DarkTab : public QWidget {
	ImageStacking m_is;

	FileSelection* m_dfs;
	QComboBox* m_integration_combo;

public:
	DarkTab(const QSize& size = QSize(500, 400), QWidget* parent = nullptr);

	const std::vector<std::filesystem::path>& DarkPaths()const { return m_dfs->FilePaths(); }

	ImageStacking& DarkImageStacker() { return m_is; }

	void Reset();
};

class DarkFlatTab : public QWidget {
	ImageStacking m_is;

	FileSelection* m_dffs;
	QComboBox* m_integration_combo;

public:
	DarkFlatTab(const QSize& size = QSize(500, 400), QWidget* parent = nullptr);

	const std::vector<std::filesystem::path>& DarkFlatPaths()const { return m_dffs->FilePaths(); }

	ImageStacking& DarkFlatImageStacker() { return m_is; }

	void Reset();
};

class FlatTab : public QWidget {
	ImageStacking m_is;

	FileSelection* m_ffs;
	QComboBox* m_integration_combo;

public:
	FlatTab(const QSize& size = QSize(500, 400), QWidget* parent = nullptr);

	const std::vector<std::filesystem::path>& FlatPaths()const { return m_ffs->FilePaths(); }

	ImageStacking& FlatImageStacker() { return m_is; }

	void Reset();
};

class CalibrationCombinationDialog : public ProcessDialog {

	QTabWidget* m_tabs;
	DarkTab* m_dark_tab;
	DarkFlatTab* m_dflat_tab;
	FlatTab* m_flat_tab;

public:
	CalibrationCombinationDialog(QWidget* parent = nullptr);

private:

	void resetDialog();

	void showPreview() {}

	void Apply();
};