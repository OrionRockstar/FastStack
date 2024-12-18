#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "ImageFileReader.h"
#include "RGBColorSpace.h"


class ChannelCombination {

private:
	ColorSpace::Type m_color_space = ColorSpace::rgb;

	bool m_enable_red = true;
	const Image8* m_R = nullptr;

	bool m_enable_green = true;
	const Image8* m_G = nullptr;

	bool m_enable_blue = true;
	const Image8* m_B = nullptr;

	double redPixel(int el)const;

	double greenPixel(int el)const;

	double bluePixel(int el)const;

	Color<double> outputColor(const Color<double>& inp)const;
public:

	void enableRed(bool enable) { m_enable_red = enable; }

	void enableGreen(bool enable) { m_enable_green = enable; }

	void enableBlue(bool enable) { m_enable_blue = enable; }

	void setRed(const Image8* red) { m_R = red; }

	void setGreen(const Image8* green) { m_G = green; }

	void setBlue(const Image8* blue) { m_B = blue; }

	QSize outputSize()const;

	Status isImagesSameSize()const;

	ColorSpace::Type colorspace()const { return m_color_space; }

	void setColorspace(ColorSpace::Type cs) { m_color_space = cs; }

	Image32 generateRGBImage();
};






class ChannelCombinationDialog : public ProcessDialog {

	ChannelCombination m_cc;

	QButtonGroup* m_colorspace_bg;

	CheckBox* m_red_cb;
	ComboBox* m_red_combo;

	CheckBox* m_green_cb;
	ComboBox* m_green_combo;

	CheckBox* m_blue_cb;
	ComboBox* m_blue_combo;

public:
	ChannelCombinationDialog(QWidget* parent);

private:
	void onWindowOpen();

	void onWindowClose();

	void addColorSpaceBG();

	void addRedInputs();

	void addGreenInputs();

	void addBlueInputs();

	void resetDialog();

	void showPreview() {}

	void Apply();

	void ApplytoPreview(){}
};