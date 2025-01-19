#pragma once
#include "Image.h"
#include "ProcessDialog.h"

class Binerize {

	std::array<float, 3> m_threshold{ 0.5f,0.5f,0.5f };

public:
	Binerize() = default;

	float rgbk_RedThreshold()const { return m_threshold[0]; }

	void setRGBK_RedThreshold(float t) { m_threshold[0] = t; }

	float greenThreshold()const { return m_threshold[1]; }

	void setGreenThreshold(float t) { m_threshold[1] = t; }

	float blueThreshold()const { return m_threshold[2]; }

	void setBlueThreshold(float t) { m_threshold[2] = t; }

	template<typename T>
	void apply(Image<T>& img);
};






class BinerizeDialog : public ProcessDialog {

	Binerize m_binerize;

	const QString m_rgbk = "RGB/K:   ";
	const QString m_rk = "R/K:   ";

	QButtonGroup* m_rgb_bg = nullptr;

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