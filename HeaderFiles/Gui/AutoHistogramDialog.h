#pragma once
#include "AutoHistogram.h"
#include "ProcessDialog.h"

class AutoHistogramDialog : public ProcessDialog {

	AutoHistogram m_ah;

	GroupBox* m_tgt_med_gb;
	CheckBox* m_target_enable_cb;
	CheckBox* m_target_rgb_cb;
	ComboBox* m_stretch_combo;

	DoubleInput* m_target_red_inp = nullptr;
	DoubleInput* m_target_green_inp = nullptr;
	DoubleInput* m_target_blue_inp = nullptr;


	GroupBox* m_hist_clip_gb;
	CheckBox* m_hist_enable_cb;
	CheckBox* m_hist_rgb_cb;

	GroupBox* m_shadow_gb;
	DoubleInput* m_shadow_red_inp = nullptr;
	DoubleInput* m_shadow_green_inp = nullptr;
	DoubleInput* m_shadow_blue_inp = nullptr;

	GroupBox* m_highlight_gb;
	DoubleInput* m_highlight_red_inp = nullptr;
	DoubleInput* m_highlight_green_inp = nullptr;
	DoubleInput* m_highlight_blue_inp = nullptr;

public:
	AutoHistogramDialog(Workspace* parent);

private:

	void addTargetMedian();

	void addTargetMedianInputs();

	void addHistogramClipping();

	void addHistogramClippingInputs_Shadow();

	void addHistogramClippingInputs_Highlight();

	void resetDialog();

	void apply();

	void applyPreview();
};