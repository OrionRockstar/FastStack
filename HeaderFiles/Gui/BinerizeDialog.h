#pragma once
#include "ProcessDialog.h"
#include "Binerize.h"


class BinerizeDialog : public ProcessDialog {

	Binerize m_binerize;

	const QString m_rgbk = "RGB/K:";
	const QString m_rk = "Red/K:";

	QButtonGroup* m_rgb_bg = nullptr;

	QLabel* m_rgbk_label;
	QLabel* m_rk_label;

	DoubleLineEdit* m_rgbk_red_le = nullptr;
	Slider* m_rgbk_red_slider = nullptr;

	DoubleLineEdit* m_green_le = nullptr;
	Slider* m_green_slider = nullptr;

	DoubleLineEdit* m_blue_le = nullptr;
	Slider* m_blue_slider = nullptr;

public:
	BinerizeDialog(QWidget* parent);

private:
	void addRGBRadioInputs();

	void addRGBK_RedInputs();

	void addGreenInputs();

	void addBlueInputs();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};
