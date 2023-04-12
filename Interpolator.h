#pragma once
#include "Image.h"
#include <array>

enum class Interpolation_Type {
	Nearest_Neighbor,
	Bilinear,
	Catmull_Rom,
	Bicubic_Spline,
	Bicubic_B_Spline
};

template <typename Image>
class Interpolator {

private:

	static std::array<float, 4> getKernelRow(Image& img, int x, int y, int channel) {
		std::array<float, 4> pixrow;
		pixrow[0] = img(x - 1, y, channel);
		pixrow[1] = img(x, y, channel);
		pixrow[2] = img(x + 1, y, channel);
		pixrow[3] = img(x + 2, y, channel);
		return pixrow;
	}

	static float InterpolatePix(std::array<float, 4> pixrow, std::array<float, 4>& vec) {
		float f03 = pixrow[0] * vec[0] + pixrow[3] * vec[3];
		float f12 = pixrow[1] * vec[1] + pixrow[2] * vec[2];

		return (-f03 < .3 * f12) ? f03 + f12 : f12 / (vec[1] + vec[2]);
	}

	static bool IsOutRange(double value, int low, int high) {
		return (value < low || high <= value);
	}

public:

	static float NearestNeighbor(Image& img, double x_s, double y_s, int ch) {

		int x_f = (int)floor(x_s);
		float dx = x_s - x_f;
		int y_f = (int)floor(y_s);
		float dy = y_s - y_f;

		x_f += (dx > .5) ? 1 : 0;
		y_f += (dy > .5) ? 1 : 0;

		return img.IsInBounds(x_f, y_f) ? img(x_f, y_f, ch) : 0;

	}

	static float Bilinear(Image& img, double x_s, double y_s, int ch) {

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

	static float Catmull_Rom(Image& img, double x_s, double y_s, int ch) {

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

	static float Bicubic_Spline(Image& img, double x_s, double y_s, int ch) {

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

	static float Bicubic_B_Spline(Image& img, double x_s, double y_s, int ch) {

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

	static float InterpolatePixel(Image& img, double x_s, double y_s, int ch, Interpolation_Type type = Interpolation_Type::Bicubic_Spline) {
		switch (type) {
		case Interpolation_Type::Nearest_Neighbor:
			return NearestNeighbor(img, x_s, y_s, ch);
		case Interpolation_Type::Bilinear:
			return Bilinear(img, x_s, y_s, ch);
		case Interpolation_Type::Catmull_Rom:
			return img.ClipPixel(Catmull_Rom(img, x_s, y_s, ch));
		case Interpolation_Type::Bicubic_Spline:
			return img.ClipPixel(Bicubic_Spline(img, x_s, y_s, ch));
		case Interpolation_Type::Bicubic_B_Spline:
			return img.ClipPixel(Bicubic_B_Spline(img, x_s, y_s, ch));
		default:
			return img.ClipPixel(Bicubic_Spline(img, x_s, y_s, ch));
		}
	}
};