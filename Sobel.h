#pragma once
#include "Image.h"
#include "ProcessDialog.h"

class Sobel {
	const std::array<int8_t, 9> m_sx = { 1, 0,-1,
										 2, 0,-2,
										 1, 0,-1 };
	const std::array<int8_t, 9> m_sy = { 1, 2, 1,
										 0, 0, 0,
										-1,-2,-1 };

public:
	template<typename T>
	Image32 Apply(const Image<T>& img);
};




class SobelDialog : public ProcessDialog {

	Sobel m_sobel;

public:
	SobelDialog(QWidget* paerent);

	void resetDialog() {}

	void showPreview() {}

	void Apply();
};