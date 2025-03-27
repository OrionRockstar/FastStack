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
