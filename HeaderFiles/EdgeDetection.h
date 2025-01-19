#pragma once
#include "Image.h"
#include "ProcessDialog.h"


class EdgeDetection {

	const std::array<int8_t, 9> m_sobelx = { 1, 0,-1,
											 2, 0,-2,
											 1, 0,-1 };
	const std::array<int8_t, 9> m_sobely = { 1, 2, 1,
											 0, 0, 0,
											-1,-2,-1 };

	const std::array<int8_t, 9> m_scharrx = { 3, 0, -3,
											 10, 0,-10,
											  3, 0, -3 };
	const std::array<int8_t, 9> m_scharry = { 3, 10, 3,
											  0,  0, 0,
											 -3,-10,-3 };

	const std::array<int8_t, 4> m_robertsx = { 1, 0,
											   0,-1 };
	const std::array<int8_t, 4> m_robertsy = { 0, 1,
										      -1, 0 };

	const std::array<int8_t, 9> m_prewittx = { 1, 1, 1,
											   0, 0, 0,
											  -1,-1,-1 };
	const std::array<int8_t, 9> m_prewitty = { 1, 0,-1,
											   1, 0,-1,
											   1, 0,-1 };

public:
	enum class Operator : uint8_t {
		sobel,
		scharr,
		roberts_cross,
		prewitt
	};

private:
	struct Gradient{
		float magnitude = 0.0f;
		float theta = 0.0;
	};

	Operator m_operator = Operator::sobel;

	template<typename T>
	Gradient Sobel(const Image<T>& img, const ImagePoint& p);

	template<typename T>
	Gradient Scharr(const Image<T>& img, const ImagePoint& p);

	template<typename T>
	Gradient Roberts_Cross(const Image<T>& img, const ImagePoint& p);

	template<typename T>
	Gradient Prewitt(const Image<T>& img, const ImagePoint& p);

public:
	void setOperator(Operator op) { m_operator = op; }

	QString getOperator_qstring()const;

public:
	template<typename T>
	Image32 apply(const Image<T>& img);
};





class EdgeDetectionDialog : public ProcessDialog {

	EdgeDetection m_ed;
	QButtonGroup* m_operator_bg;

public:
	EdgeDetectionDialog(QWidget* paerent);

	void resetDialog() {
		m_ed.setOperator(EdgeDetection::Operator::sobel);
		m_operator_bg->button(int(EdgeDetection::Operator::sobel))->setChecked(true);
	}

	void showPreview() {}

	void apply();
};