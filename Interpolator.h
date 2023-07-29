#pragma once
#include "Image.h"
#include <array>

enum class Interpolate {
	nearest_neighbor,
	bilinear,
	catmull_rom,
	bicubic_spline,
	bicubic_b_spline,
	cubic_b_spline,
	lanczos3
};

template <typename Image>
class Interpolator {

private:

	std::array<float, 4> getKernelRow(Image& img, int x, int y, int channel) {
		std::array<float, 4> pixrow;
		pixrow[0] = img(x - 1, y, channel);
		pixrow[1] = img(x, y, channel);
		pixrow[2] = img(x + 1, y, channel);
		pixrow[3] = img(x + 2, y, channel);
		return pixrow;
	}

	std::array<float, 6> getKernelRow6(Image& img, int x, int y, int channel) {
		std::array<float, 6> pixrow;
		pixrow[0] = img(x - 2, y, channel);
		pixrow[1] = img(x - 1, y, channel);
		pixrow[2] = img(x, y, channel);
		pixrow[3] = img(x + 1, y, channel);
		pixrow[4] = img(x + 2, y, channel);
		pixrow[5] = img(x + 3, y, channel);
		return pixrow;
	}

	float InterpolatePix(std::array<float, 4> pixrow, std::array<float, 4> vec) {
		float f03 = pixrow[0] * vec[0] + pixrow[3] * vec[3];
		float f12 = pixrow[1] * vec[1] + pixrow[2] * vec[2];

		return (-f03 < .3 * f12) ? f03 + f12 : f12 / (vec[1] + vec[2]);
	}

	float InterpolatePix(std::array<float, 6> pixrow, std::array<float, 6> vec) {
		float f05 = pixrow[0] * vec[0] + pixrow[5] * vec[5];
		float f14 = pixrow[1] * vec[1] + pixrow[4] * vec[4];
		float f23 = pixrow[2] * vec[2] + pixrow[3] * vec[3];
		return (-f14 < .3 * f23) ? f05 + f14 + f23 : f23 / (vec[2] + vec[3]);
	}

	double R(double x) {
		double Px = (x > 0) ? x * x * x : 0;

		double xp1 = x + 1;
		xp1 = (xp1 > 0) ? xp1 * xp1 * xp1 : 0;

		double xp2 = x + 2;
		xp2 = (xp2 > 0) ? xp2 * xp2 * xp2 : 0;

		double xm1 = x - 1;
		xm1 = (xm1 > 0) ? xm1 * xm1 * xm1 : 0;

		return (xp2 - 4 * xp1 + 6 * Px - 4 * xm1) / 6;
	}

	bool IsOutRange(double value, int low, int high) {
		return (value < low || high <= value);
	}

	double sinc(double val) {
		val *= M_PI;
		return (val == 0) ? 1 : sin(val) / (val);
	}

	double L3(double val) {
		return (abs(val) < 3.0) ? sinc(val) * sinc(val * 0.333333) : 0;
	}

	float GetWeight(std::array<float, 6>& vecx, std::array<float, 6>& vecy) {
		float w = 0;
		w += vecx[0] * vecy[0];
		w += vecx[1] * vecy[0];
		w += vecx[2] * vecy[0];
		w += vecx[3] * vecy[0];
		w += vecx[4] * vecy[0];
		w += vecx[5] * vecy[0];

		w += vecx[0] * vecy[1];
		w += vecx[1] * vecy[1];
		w += vecx[2] * vecy[1];
		w += vecx[3] * vecy[1];
		w += vecx[4] * vecy[1];
		w += vecx[5] * vecy[1];

		w += vecx[0] * vecy[2];
		w += vecx[1] * vecy[2];
		w += vecx[2] * vecy[2];
		w += vecx[3] * vecy[2];
		w += vecx[4] * vecy[2];
		w += vecx[5] * vecy[2];

		w += vecx[0] * vecy[3];
		w += vecx[1] * vecy[3];
		w += vecx[2] * vecy[3];
		w += vecx[3] * vecy[3];
		w += vecx[4] * vecy[3];
		w += vecx[5] * vecy[3];

		w += vecx[0] * vecy[4];
		w += vecx[1] * vecy[4];
		w += vecx[2] * vecy[4];
		w += vecx[3] * vecy[4];
		w += vecx[4] * vecy[4];
		w += vecx[5] * vecy[4];

		w += vecx[0] * vecy[5];
		w += vecx[1] * vecy[5];
		w += vecx[2] * vecy[5];
		w += vecx[3] * vecy[5];
		w += vecx[4] * vecy[5];
		w += vecx[5] * vecy[5];

		return w;
	}

public:

	float NearestNeighbor(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		float dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		float dy = y_s - y_f;

		x_f += (dx > .5) ? 1 : 0;
		y_f += (dy > .5) ? 1 : 0;

		return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;

	}

	float Bilinear(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		float dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		float dy = y_s - y_f;

		if (IsOutRange(x_f, 0, img.Cols() - 1) || IsOutRange(y_f, 0, img.Rows() - 1)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		float r1 = img(x_f, y_f, ch) * (1 - dx) + img(x_f + 1, y_f, ch) * dx;
		float r2 = img(x_f, y_f + 1, ch) * (1 - dx) + img(x_f + 1, y_f + 1, ch) * dx;

		return r1 * (1 - dy) + r2 * dy;
	}

	float Catmull_Rom(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		double dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		double dy = y_s - y_f;

		if (IsOutRange(x_f, 1, img.Cols() - 2) || IsOutRange(y_f, 1, img.Rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		std::array<float, 4> vecx;
		std::array<float, 4> vecy;

		dx++;
		vecx[0] = float(-3 * (dx * dx * dx) + 15 * (dx * dx) - 24 * dx + 12) / 6;
		dx--;
		vecx[1] = float(9 * (dx * dx * dx) - 15 * (dx * dx) + 6) / 6;
		dx = 1 - dx;
		vecx[2] = float(9 * (dx * dx * dx) - 15 * (dx * dx) + 6) / 6;
		dx++;
		vecx[3] = float(-3 * (dx * dx * dx) + 15 * (dx * dx) - 24 * dx + 12) / 6;

		dy++;
		vecy[0] = float(-3 * (dy * dy * dy) + 15 * (dy * dy) - 24 * dy + 12) / 6;
		dy--;
		vecy[1] = float(9 * (dy * dy * dy) - 15 * (dy * dy) + 6) / 6;
		dy = 1 - dy;
		vecy[2] = float(9 * (dy * dy * dy) - 15 * (dy * dy) + 6) / 6;
		dy++;
		vecy[3] = float(-3 * (dy * dy * dy) + 15 * (dy * dy) - 24 * dy + 12) / 6;

		std::array<float, 4> resxv;

		resxv[0] = InterpolatePix(getKernelRow(img, x_f, y_f - 1, ch), vecx);
		resxv[1] = InterpolatePix(getKernelRow(img, x_f, y_f, ch), vecx);
		resxv[2] = InterpolatePix(getKernelRow(img, x_f, y_f + 1, ch), vecx);
		resxv[3] = InterpolatePix(getKernelRow(img, x_f, y_f + 2, ch), vecx);

		return InterpolatePix(resxv, vecy);
	}

	float Bicubic_Spline(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		double dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		double dy = y_s - y_f;

		if (IsOutRange(x_f, 1, img.Cols() - 2) || IsOutRange(y_f, 1, img.Rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		std::array<float, 4> vecx;
		std::array<float, 4> vecy;

		dx++;
		vecx[0] = float(-.5 * (dx * dx * dx) + 2.5 * (dx * dx) - 4 * dx + 2);
		dx--;
		vecx[1] = float(1.5 * (dx * dx * dx) - 2.5 * (dx * dx) + 1);
		dx = 1 - dx;
		vecx[2] = float(1.5 * (dx * dx * dx) - 2.5 * (dx * dx) + 1);
		dx++;
		vecx[3] = float(-.5 * (dx * dx * dx) + 2.5 * (dx * dx) - 4 * dx + 2);

		dy++;
		vecy[0] = float(-.5 * (dy * dy * dy) + 2.5 * (dy * dy) - 4 * dy + 2);
		dy--;
		vecy[1] = float(1.5 * (dy * dy * dy) - 2.5 * (dy * dy) + 1);
		dy = 1 - dy;
		vecy[2] = float(1.5 * (dy * dy * dy) - 2.5 * (dy * dy) + 1);
		dy++;
		vecy[3] = float(-.5 * (dy * dy * dy) + 2.5 * (dy * dy) - 4 * dy + 2);

		std::array<float, 4> resxv;

		resxv[0] = InterpolatePix(getKernelRow(img, x_f, y_f - 1, ch), vecx);
		resxv[1] = InterpolatePix(getKernelRow(img, x_f, y_f, ch), vecx);
		resxv[2] = InterpolatePix(getKernelRow(img, x_f, y_f + 1, ch), vecx);
		resxv[3] = InterpolatePix(getKernelRow(img, x_f, y_f + 2, ch), vecx);

		return InterpolatePix(resxv, vecy);
	}

	float Bicubic_B_Spline(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		double dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		double dy = y_s - y_f;

		if (IsOutRange(x_f, 1, img.Cols() - 2) || IsOutRange(y_f, 1, img.Rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		double rdxm1 = R(-1 - dx);
		double rdx = R(-dx);
		double rdxp1 = R(1 - dx);
		double rdxp2 = R(2 - dx);

		double rdym1 = R(-1 - dy);
		double rdy = R(-dy);
		double rdyp1 = R(1 - dy);
		double rdyp2 = R(2 - dy);

		return img(x_f - 1, y_f - 1) * rdxm1 * rdym1 +
			img(x_f, y_f - 1) * rdx * rdym1 +
			img(x_f + 1, y_f - 1) * rdxp1 * rdym1 +
			img(x_f + 2, y_f - 1) * rdxp2 * rdym1 +

			img(x_f - 1, y_f) * rdxm1 * rdy +
			img(x_f, y_f) * rdx * rdy +
			img(x_f + 1, y_f) * rdxp1 * rdy +
			img(x_f + 2, y_f) * rdxp2 * rdy +

			img(x_f - 1, y_f + 1) * rdxm1 * rdyp1 +
			img(x_f, y_f + 1) * rdx * rdyp1 +
			img(x_f + 1, y_f + 1) * rdxp1 * rdyp1 +
			img(x_f + 2, y_f + 1) * rdxp2 * rdyp1 +

			img(x_f - 1, y_f + 2) * rdxm1 * rdyp2 +
			img(x_f, y_f + 2) * rdx * rdyp2 +
			img(x_f + 1, y_f + 2) * rdxp1 * rdyp2 +
			img(x_f + 2, y_f + 2) * rdxp2 * rdyp2;

	}

	float Cubic_B_Spline(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		double dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		double dy = y_s - y_f;

		if (IsOutRange(x_f, 1, img.Cols() - 2) || IsOutRange(y_f, 1, img.Rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		std::array<float, 4> vecx;
		std::array<float, 4> vecy;

		dx++;
		vecx[0] = float(-(dx * dx * dx) + 6 * (dx * dx) - 12 * dx + 8) / 6;
		dx--;
		vecx[1] = float(3 * (dx * dx * dx) - 6 * (dx * dx) + 4) / 6;
		dx = 1 - dx;
		vecx[2] = float(3 * (dx * dx * dx) - 6 * (dx * dx) + 4) / 6;
		dx++;
		vecx[3] = float(-(dx * dx * dx) + 6 * (dx * dx) - 12 * dx + 8) / 6;

		dy++;
		vecy[0] = float(-(dy * dy * dy) + 6 * (dy * dy) - 12 * dy + 8) / 6;
		dy--;
		vecy[1] = float(3 * (dy * dy * dy) - 6 * (dy * dy) + 4) / 6;
		dy = 1 - dy;
		vecy[2] = float(3 * (dy * dy * dy) - 6 * (dy * dy) + 4) / 6;
		dy++;
		vecy[3] = float(-(dy * dy * dy) + 6 * (dy * dy) - 12 * dy + 8) / 6;

		std::array<float, 4> resxv;

		resxv[0] = InterpolatePix(getKernelRow(img, x_f, y_f - 1, ch), vecx);
		resxv[1] = InterpolatePix(getKernelRow(img, x_f, y_f, ch), vecx);
		resxv[2] = InterpolatePix(getKernelRow(img, x_f, y_f + 1, ch), vecx);
		resxv[3] = InterpolatePix(getKernelRow(img, x_f, y_f + 2, ch), vecx);

		return InterpolatePix(resxv, vecy);
	}

	float Lanczos3(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		double dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		double dy = y_s - y_f;

		if (IsOutRange(x_f, 2, img.Cols() - 3) || IsOutRange(y_f, 2, img.Rows() - 3)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;
		}

		std::array<float, 6> vecx;
		std::array<float, 6> vecy;

		dx += 2;
		vecx[0] = L3(dx);
		dx--;
		vecx[1] = L3(dx);
		dx--;
		vecx[2] = L3(dx);
		dx = 1 - dx;
		vecx[3] = L3(dx);
		dx++;
		vecx[4] = L3(dx);
		dx++;
		vecx[5] = L3(dx);


		dy += 2;
		vecy[0] = L3(dy);
		dy--;
		vecy[1] = L3(dy);
		dy--;
		vecy[2] = L3(dy);
		dy = 1 - dy;
		vecy[3] = L3(dy);
		dy++;
		vecy[4] = L3(dy);
		dy++;
		vecy[5] = L3(dy);

		std::array<float, 6> resxv;

		resxv[0] = InterpolatePix(getKernelRow6(img, x_f, y_f - 2, ch), vecx);
		resxv[1] = InterpolatePix(getKernelRow6(img, x_f, y_f - 1, ch), vecx);
		resxv[2] = InterpolatePix(getKernelRow6(img, x_f, y_f, ch), vecx);
		resxv[3] = InterpolatePix(getKernelRow6(img, x_f, y_f + 1, ch), vecx);
		resxv[4] = InterpolatePix(getKernelRow6(img, x_f, y_f + 2, ch), vecx);
		resxv[5] = InterpolatePix(getKernelRow6(img, x_f, y_f + 3, ch), vecx);

		return InterpolatePix(resxv, vecy) / GetWeight(vecx, vecy);
	}

	float InterpolatePixel(Image& img, double x_s, double y_s, int ch, Interpolate type = Interpolate::Bicubic_Spline) {
		using enum Interpolate;
		
		switch (type) {
		case nearest_neighbor:
			return NearestNeighbor(img, x_s, y_s, ch);
		case bilinear:
			return Bilinear(img, x_s, y_s, ch);
		case catmull_rom:
			return img.ClipPixel(Catmull_Rom(img, x_s, y_s, ch));
		case bicubic_spline:
			return img.ClipPixel(Bicubic_Spline(img, x_s, y_s, ch));
		case bicubic_b_spline:
			return img.ClipPixel(Bicubic_B_Spline(img, x_s, y_s, ch));
		case cubic_b_spline:
			return img.ClipPixel(Cubic_B_Spline(img, x_s, y_s, ch));
		case lanczos3:
			return img.ClipPixel(Lanczos3(img, x_s, y_s, ch));

		default:
			return img.ClipPixel(Bicubic_Spline(img, x_s, y_s, ch));
		}
	}
};