#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "RGBColorSpace.h"

class AutoHistogram {

public:
	enum class StretchMethod {
		gamma,
		log,
		mtf
	};

private:
	std::array<float, 3> m_shadow_clipping = { 0.05,0.05,0.05 };

	std::array<float, 3> m_highlight_clipping = { 0.00,0.00,0.00 };

	std::array<float, 3> m_target_median = { 0.15,0.15,0.15 };

	StretchMethod m_stretch_method = StretchMethod::gamma;

	bool m_histogram_clipping = true;

	bool m_stretch = true;

public:
	AutoHistogram() = default;

	void enableStretching(bool stretch) { m_stretch = stretch; }

	void enableHistogramClipping(bool histogram_clipping) { m_histogram_clipping = histogram_clipping; }

	float shadowClipping(ColorComponent comp)const;

	void setShadowClipping(ColorComponent comp, float percentage);

	float highlightClipping(ColorComponent comp)const;

	void setHighlightClipping(ColorComponent comp, float percentage);

	float targetMedian(ColorComponent comp)const;

	void setTargetMedian(ColorComponent comp,float tgt_median);

	void setStretchMethod(StretchMethod method) { m_stretch_method = method; }

private:
	float LTF(float pixel, float b);

	double ComputeLogMultiplier(float median, float tgt_median);

public:
	template <typename P>
	void Apply(Image<P>& img);
};








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

	void Apply();

	void ApplytoPreview();
};