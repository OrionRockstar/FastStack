#pragma once
#include "Image.h"
#include "ImageWindow.h"
#include "ProcessDialog.h"

class ASinhStretch {
	float m_stretch_factor = 1.0;
	float m_blackpoint = 0.0;
	bool m_srbg = true;

	float ComputeBeta();

	template<typename T>
	void ApplyMono(Image<T>& img);

	template<typename T>
	void ApplyRGB(Image<T>& img);

public:
	ASinhStretch() = default;

	float StretchFactor()const;

	void setStretchFactor(float stretch_factor);

	float Blackpoint()const;

	void setBlackpoint(float blackpoint);

	void setsRGB(bool srgb) { m_srbg = srgb; }

	template<typename T>
	void ComputeBlackpoint(Image<T>& img);

	template<typename Image>
	void Apply(Image& img);
};










class ASinhStretchDialog : public ProcessDialog {
	Q_OBJECT

	ASinhStretch m_ash;

	QSlider* m_sf_slider;
	DoubleLineEdit* m_sf_le;

	QSlider* m_bp_slider;
	DoubleLineEdit* m_bp_le;

	float m_current_bp = 0;
	QSlider* m_fine_tune;

	QCheckBox* m_rgb_cb;
public:
	ASinhStretchDialog(QWidget* parent = nullptr);

private:
	void actionSlider_sf(int action);

	void editingFinished_sf();

	void actionSlider_bp(int action);

	void editingFinished_bp();

	void sliderPressed_ft();

	void sliderMoved_ft(int value);

	void sliderReleased_ft();

	void onClick_srgb(bool val);

	void onPressed_blackpoint();

	void AddStretchFactorInputs();

	void AddBlackpointInputs();

	void AddFinetuneInputs();

	void computeBlackpoint();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();

};