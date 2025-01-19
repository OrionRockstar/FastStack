#pragma once
#include "Image.h"
#include "ImageWindow.h"
#include "ProcessDialog.h"

class ASinhStretch {
	float m_stretch_factor = 1.0;
	float m_blackpoint = 0.0;
	bool m_srbg = true;

	float computeBeta();

	template<typename T>
	void applyMono(Image<T>& img);

	template<typename T>
	void applyRGB(Image<T>& img);

public:
	ASinhStretch() = default;

	float stretchFactor()const { return m_stretch_factor; }

	void setStretchFactor(float stretch_factor) { m_stretch_factor = stretch_factor; }

	float blackpoint()const { return m_blackpoint; }

	void setBlackpoint(float blackpoint) { m_blackpoint = blackpoint; }

	void setsRGB(bool srgb) { m_srbg = srgb; }

	template<typename T>
	void computeBlackpoint(const Image<T>& img);

	template<typename Image>
	void apply(Image& img);
};










class ASinhStretchDialog : public ProcessDialog {
	Q_OBJECT

	ASinhStretch m_ash;

	Slider* m_sf_slider;
	DoubleLineEdit* m_sf_le;

	Slider* m_bp_slider;
	DoubleLineEdit* m_bp_le;

	float m_current_bp = 0;
	Slider* m_fine_tune;

	CheckBox* m_rgb_cb;

	PushButton* m_bp_comp = nullptr;

public:
	ASinhStretchDialog(QWidget* parent = nullptr);

private:
	void addStretchFactorInputs();

	void addBlackpointInputs();

	void addFinetuneInputs();

	void computeBlackpoint();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();

};