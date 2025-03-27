#pragma once
#include "AutoHistogram.h"
#include "ProcessDialog.h"

class AutoHistogramDialog : public ProcessDialog {

	AutoHistogram m_ah;

	GroupBox* m_tgt_med_gb;
	CheckBox* m_target_enable_cb;
	CheckBox* m_target_rgb_cb;
	ComboBox* m_stretch_combo;
	DoubleLineEdit* m_target_red_le;
	Slider* m_target_red_slider;
	DoubleLineEdit* m_target_green_le;
	Slider* m_target_green_slider;
	DoubleLineEdit* m_target_blue_le;
	Slider* m_target_blue_slider;


	GroupBox* m_hist_clip_gb;
	CheckBox* m_hist_enable_cb;
	CheckBox* m_hist_rgb_cb;
	GroupBox* m_shadow_gb;
	DoubleLineEdit* m_shadow_red_le;
	Slider* m_shadow_red_slider;
	DoubleLineEdit* m_shadow_green_le;
	Slider* m_shadow_green_slider;
	DoubleLineEdit* m_shadow_blue_le;
	Slider* m_shadow_blue_slider;

	GroupBox* m_highlight_gb;
	DoubleLineEdit* m_highlight_red_le;
	Slider* m_highlight_red_slider;
	DoubleLineEdit* m_highlight_green_le;
	Slider* m_highlight_green_slider;
	DoubleLineEdit* m_highlight_blue_le;
	Slider* m_highlight_blue_slider;

public:
	AutoHistogramDialog(QWidget* parent);

private:

	void addTargetMedianInputs();

	void addTargetMedianInputs_Red();

	void addTargetMedianInputs_Green();

	void addTargetMedianInputs_Blue();

	void joinTargetMedian(bool v);

	void updateJoinedTarget();

	void addHistogramClippingInputs();

	void addHistogramClippingInputs_ShadowRed();

	void addHistogramClippingInputs_ShadowGreen();

	void addHistogramClippingInputs_ShadowBlue();

	void addHistogramClippingInputs_HighlightRed();

	void addHistogramClippingInputs_HighlightGreen();

	void addHistogramClippingInputs_HighlightBlue();

	void joinHistogramClipping(bool v);

	void updateJoinedHistogramClipping();

	void showPreview();

	void resetDialog();

	void apply();

	void applytoPreview();
};