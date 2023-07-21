#include "Drizzle.h"

float Drizzle::AddPixel(float inp, float out, float area, int pix_weight) {

	if (out == 0) {

		float dw = area * pix_weight;

		return (dw != 0) ? ((inp * area * pix_weight * s2)) / dw : out;
	}

	float dw = area * pix_weight + m_out_weight;

	return (dw != 0) ? ((inp * area * pix_weight * s2) + (out * m_out_weight)) / dw : out;
}

void Drizzle::DrizzlePixel(Image32& input, Point<int> source, Image32& output, Point<double> dest) {

	dest.x *= m_scale_factor;
	dest.y *= m_scale_factor;

	int x_f = (int)floor(dest.x);
	int y_f = (int)floor(dest.y);

	if (x_f < 0 || x_f >= output.Cols() - m_scale_factor || y_f < 0 || y_f >= output.Rows() - m_scale_factor) return;

	float vx = (1 - (dest.x - x_f));
	float vy = (1 - (dest.y - y_f));

	int limity = ceil(m_new_drop);
	if (vy > m_new_drop) {
		limity = 0;
		vy = m_new_drop;
	}

	int limitx = ceil(m_new_drop);
	if (vx > m_new_drop) {
		limitx = 0;
		vx = m_new_drop;
	}

	float lx, ly;

	for (int j = 0, el = 0; j <= limity; ++j) {
		if (j == 0)
			ly = vy;
		else if (j == limity)
			ly = (m_new_drop - vy - (j - 1));
		else ly = 1;

		for (int i = 0; i <= limitx; ++i) {
			if (i == 0)
				lx = vx;

			else if (i == limitx)
				lx = (m_new_drop - vx - (i - 1));

			else lx = 1;

			output(x_f + i, y_f + j) = AddPixel(input(source.x, source.y), output(x_f + i, y_f + j), lx * ly / m_new_drop_area, input.Weight_At(source.x, source.y));
		}
	}
}

void Drizzle::DrizzleFrame(Image32& input, Image32& output) {
	Matrix homography = input.homography.Inverse();

#pragma omp parallel for
	for (int y = 0; y < input.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < input.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2) + m_offset;
			double y_s = x * homography(1, 0) + yy + homography(1, 2) + m_offset;

			//DrizzlePixel(input(x, y), x_s, y_s, output);
			DrizzlePixel(input, Point(x, y), output, Point(x_s, y_s));

		}
	}

}