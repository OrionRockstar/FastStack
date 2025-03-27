#pragma once
#include "Image.h"
#include "ImageFileReader.h"
#include "RGBColorSpace.h"


class ChannelCombination {

	ColorSpace::Type m_color_space = ColorSpace::Type::rgb;

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
	bool isRedEnabled()const { return m_enable_red; }

	void enableRed(bool enable) { m_enable_red = enable; }

	bool isGreenEnabled()const { return m_enable_green; }

	void enableGreen(bool enable) { m_enable_green = enable; }

	bool isBlueEnabled()const { return m_enable_blue; }

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

