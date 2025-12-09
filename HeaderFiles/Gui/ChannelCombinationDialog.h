#pragma once
#include "ChannelCombination.h"
#include "ProcessDialog.h"


class ChannelCombinationDialog : public ProcessDialog {

	ChannelCombination m_cc;

	QButtonGroup* m_colorspace_bg;

	CheckBox* m_red_cb;
	ImageComboBox* m_red_combo;

	CheckBox* m_green_cb;
	ImageComboBox* m_green_combo;

	CheckBox* m_blue_cb;
	ImageComboBox* m_blue_combo;

public:
	ChannelCombinationDialog(QWidget* parent);

private:
	void onImageWindowCreated()override;

	void onImageWindowClosed()override;

	void addColorSpaceBG();

	void addRedInputs();

	void addGreenInputs();

	void addBlueInputs();

	void resetDialog();

	void showPreview() {}

	void apply();

	void applytoPreview() {}
};