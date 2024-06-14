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

	float ShadowClipping(ColorComponent comp);

	void setShadowClipping(ColorComponent comp, float percentage);

	float HighlightClipping(ColorComponent comp);

	void setHighlightClipping(ColorComponent comp, float percentage);

	float TargetMedian(ColorComponent comp);

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

	class TargetMedian : public QWidget {

		AutoHistogramDialog* m_parent;
		AutoHistogram* m_ah;

		QCheckBox* m_joined_rgb_cb;
		QComboBox* m_stretch_combo;

		DoubleLineEdit* m_red_le;
		QSlider* m_red_slider;

		DoubleLineEdit* m_green_le;
		QSlider* m_green_slider;

		DoubleLineEdit* m_blue_le;
		QSlider* m_blue_slider;
	public:
		TargetMedian(AutoHistogram& ah, QWidget* parent);
	private:
		void onActionTriggered_red(int action);

		void onEditingFinished_red();

		void onActionTriggered_green(int action);

		void onEditingFinished_green();

		void onActionTriggered_blue(int action);

		void onEditingFinished_blue();

		void onClicked_joined(bool v);

		void updateJoinedChannels();

		void setDisabled_GreenBlue(bool val);

		void AddRedInputs();

		void AddGreenInputs();

		void AddBlueInputs();

	public:
		void reset();
	};

	class HistogramClipping : public QWidget {
		AutoHistogramDialog* m_parent;
		AutoHistogram* m_ah;

		QCheckBox* m_joined_rgb_cb;

		DoubleLineEdit* m_redShadow_le;
		QSlider* m_redShadow_slider;

		DoubleLineEdit* m_greenShadow_le;
		QSlider* m_greenShadow_slider;

		DoubleLineEdit* m_blueShadow_le;
		QSlider* m_blueShadow_slider;


		DoubleLineEdit* m_redHighlight_le;
		QSlider* m_redHighlight_slider;

		DoubleLineEdit* m_greenHighlight_le;
		QSlider* m_greenHighlight_slider;

		DoubleLineEdit* m_blueHighlight_le;
		QSlider* m_blueHighlight_slider;

	public:
		HistogramClipping(AutoHistogram& ah, QWidget* parent);
	private:
		void onActionTriggered_redShadow(int action);

		void onEditingFinished_redShadow();

		void onActionTriggered_greenShadow(int action);

		void onEditingFinished_greenShadow();

		void onActionTriggered_blueShadow(int action);

		void onEditingFinished_blueShadow();

		void onClicked_joined(bool v);

		void updateJoinedChannels_shadow();

		void setDisabled_GreenBlueShadow(bool val);

		void AddRedShadowInputs();

		void AddGreenShadowInputs();

		void AddBlueShadowInputs();



		void onActionTriggered_redHighlight(int action);

		void onEditingFinished_redHighlight();

		void onActionTriggered_greenHighlight(int action);

		void onEditingFinished_greenHighlight();

		void onActionTriggered_blueHighlight(int action);

		void onEditingFinished_blueHighlight();

		void updateJoinedChannels_Highlight();

		void setDisabled_GreenBlueHighlight(bool val);

		void AddRedHighlightInputs();

		void AddGreenHighlightInputs();

		void AddBlueHighlightInputs();
	public:
		void reset();
	};

	AutoHistogram m_ah;

	TargetMedian* m_tgt_median;

	HistogramClipping* m_hist_clip;

	QCheckBox* m_target_median_cb;
	QCheckBox* m_histogram_clipping_cb;


public:
	AutoHistogramDialog(QWidget* parent);

private:
	void onClicked_tergetMedian(bool v);

	void onClicked_histogramClipping(bool v);

	void showPreview();

	void resetDialog();

	void Apply();

	void ApplytoPreview();
};