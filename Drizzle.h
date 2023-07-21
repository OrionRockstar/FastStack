#pragma once
#include "Image.h"
#include "Interpolator.h"
#include "ImageOperations.h"

class Drizzle {
	float m_drop = 0.9;
	int m_scale_factor = 2;
	float m_offset = m_scale_factor * (1 - m_drop) / 2;
	float m_new_drop = m_scale_factor * m_drop;
	float m_new_drop_area = m_new_drop * m_new_drop;
	float s2 = m_drop * m_drop;
	int m_out_weight = 1;

public:
	Drizzle(float drop_size, int scale_factor = 2) : m_drop(drop_size), m_scale_factor(scale_factor) {
		assert(0.1 < drop_size && drop_size <= 1.0);
	}

	Drizzle() = default;

private:
	template<typename T>
	struct Point {
		T x = 0;
		T y = 0;

		Point() = default;
		Point(T x, T y) : x(x), y(y) {}
	};

	float AddPixel(float inp, float out, float area, int pix_weight);

	void DrizzlePixel(Image32& input, Point<int> source, Image32& output, Point<double> dest);


public:
	void DrizzleFrame(Image32& input, Image32& output);
};
