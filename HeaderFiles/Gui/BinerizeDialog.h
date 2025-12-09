#pragma once
#include "ProcessDialog.h"
#include "Binerize.h"


class BinerizeDialog : public ProcessDialog {

	Binerize m_binerize;

	const QString m_rgbk = "RGB/K:   ";
	const QString m_rk = "Red/K:   ";

	QButtonGroup* m_rgb_bg = nullptr;

	DoubleInput* m_rgbk_red_inp = nullptr;
	DoubleInput* m_green_inp = nullptr;
	DoubleInput* m_blue_inp = nullptr;

public:
	BinerizeDialog(Workspace* parent);

private:
	void addRGBRadioInputs();

	void addInputs();

	void resetDialog();

	void apply();

	void applyPreview();
};
