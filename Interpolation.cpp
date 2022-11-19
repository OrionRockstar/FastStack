#include "Interpolation.h"
#include <array>

std::array<float,4>& getKernelRow(Image32& img, int x, int y) {
	std::array<float, 4> pixrow;
	pixrow[0] = img(y, x - 1);
	pixrow[1] = img(y, x);
	pixrow[2] = img(y, x + 1);
	pixrow[3] = img(y, x + 2);
	return pixrow;
}

float Interpolate(std::array<float,4>& pixrow, std::array<float,4>& vec) {
	float f03 = pixrow[0] * vec[0] + pixrow[3] * vec[3];
	float f12 = pixrow[1] * vec[1] + pixrow[2] * vec[2];

	return (-f03 < .3 * f12) ? f03 + f12 : f12 / (vec[1] + vec[2]);
}

float Interpolation::Bilinear(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 1 || y_s < 1 || y_s >= img.Rows() - 1) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);

		return (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows()) ? img(ys, xs) : 0;
	}

	int x_f = (int)floor(x_s);
	int y_f = (int)floor(y_s);

	double r1 = img(y_f, x_f) * (x_f + 1 - x_s) + img(y_f, x_f + 1) * (x_s - x_f);
	double r2 = img(y_f + 1, x_f) * (x_f + 1 - x_s) + img(y_f + 1, x_f + 1) * (x_s - x_f);

	return float(r1 * (y_f + 1 - y_s) + r2 * (y_s - y_f));
}

float Interpolation::Catmull_Rom(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);

		return (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows()) ? img(ys, xs) : 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

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

	resxv[0] = Interpolate(getKernelRow(img, x_f, y_f - 1), vecx);
	resxv[1] = Interpolate(getKernelRow(img, x_f, y_f), vecx);
	resxv[2] = Interpolate(getKernelRow(img, x_f, y_f + 1), vecx);
	resxv[3] = Interpolate(getKernelRow(img, x_f, y_f + 2), vecx);

	return Interpolate(resxv, vecy);
}

float Interpolation::Bicubic_Spline(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);

		return (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows()) ? img(ys, xs) : 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

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

	resxv[0] = Interpolate(getKernelRow(img, x_f, y_f - 1), vecx);
	resxv[1] = Interpolate(getKernelRow(img, x_f, y_f), vecx);
	resxv[2] = Interpolate(getKernelRow(img, x_f, y_f + 1), vecx);
	resxv[3] = Interpolate(getKernelRow(img, x_f, y_f + 2), vecx);

	return Interpolate(resxv, vecy);
}

float Interpolation::Bicubic_B_Spline(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);

		return (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows()) ? img(ys, xs) : 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

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

	float a = vecy[0] * (img(y_f - 1, x_f - 1) * vecx[0] + img(y_f - 1, x_f) * vecx[1] + img(y_f - 1, x_f + 1) * vecx[2] + img(y_f - 1, x_f + 2) * vecx[3]);
	float b = vecy[1] * (img(y_f, x_f - 1) * vecx[0] + img(y_f, x_f) * vecx[1] + img(y_f, x_f + 1) * vecx[2] + img(y_f, x_f + 2) * vecx[3]);
	float c = vecy[2] * (img(y_f + 1, x_f - 1) * vecx[0] + img(y_f + 1, x_f) * vecx[1] + img(y_f + 1, x_f + 1) * vecx[2] + img(y_f + 1, x_f + 2) * vecx[3]);
	float d = vecy[3] * (img(y_f + 2, x_f - 1) * vecx[0] + img(y_f + 2, x_f) * vecx[1] + img(y_f + 2, x_f + 1) * vecx[2] + img(y_f + 2, x_f + 2) * vecx[3]);

	return a + b + c + d;
}