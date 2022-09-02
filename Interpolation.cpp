#include "Interpolation.h"

float Interpolation::Bilinear(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 1 || y_s < 1 || y_s >= img.Rows() - 1) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);
		if (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows())
			return img(ys, xs);
	}

	int x_f = (int)floor(x_s);
	int y_f = (int)floor(y_s);

	double r1 = img(y_f, x_f) * (x_f + 1 - x_s) + img(y_f, x_f + 1) * (x_s - x_f);
	double r2 = img(y_f + 1, x_f) * (x_f + 1 - x_s) + img(y_f + 1, x_f + 1) * (x_s - x_f);

	return float(r1 * (y_f + 1 - y_s) + r2 * (y_s - y_f));
}

float Interpolation::Catmull_Rom_V1(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);
		if (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows())
			return img(ys, xs);
		else return 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

	double a, b, c, d;
	double px[4];

	for (int i = -1; i < 3; ++i) {
		a = -.5 * img(y_f + i, x_f - 1) + 1.5 * img(y_f + i, x_f) - 1.5 * img(y_f + i, x_f + 1) + .5 * img(y_f + i, x_f + 2);
		b = img(y_f + i, x_f - 1) - 2.5 * img(y_f + i, x_f) + 2 * img(y_f + i, x_f + 1) - .5 * img(y_f + i, x_f + 2);
		c = -.5 * img(y_f + i, x_f - 1) + .5 * img(y_f + i, x_f + 1);
		d = img(y_f + i, x_f);
		px[i + 1] = (a * dx * dx * dx) + (b * dx * dx) + (c * dx) + d;
	}

	a = -.5 * px[0] + 1.5 * px[1] - 1.5 * px[2] + .5 * px[3];
	b = px[0] - 2.5 * px[1] + 2 * px[2] - .5 * px[3];
	c = -.5 * px[0] + .5 * px[2];
	d = px[1];

	return float((a * dy * dy * dy) + (b * dy * dy) + (c * dy) + d);
}

float Interpolation::Catmull_Rom_V2(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);
		if (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows())
			return img(ys, xs);
		else
			return 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

	float vecx[4];
	float vecy[4];

	for (int i = -1; i < 3; ++i) {
		double xd = fabs(dx - i);
		double yd = fabs(dy - i);

		if (xd < 1)
			vecx[i + 1] = float(9 * (xd * xd * xd) - 15 * (xd * xd) + 6) / 6;
		else if (1 <= xd && xd < 2)
			vecx[i + 1] = float(-3 * (xd * xd * xd) + 15 * (xd * xd) - 24 * xd + 12) / 6;
		else
			vecx[i + 1] = 0;

		if (yd < 1)
			vecy[i + 1] = float(9 * (yd * yd * yd) - 15 * (yd * yd) + 6) / 6;
		else if (1 <= yd && yd <= 2)
			vecy[i + 1] = float(-3 * (yd * yd * yd) + 15 * (yd * yd) - 24 * yd + 12) / 6;
		else
			vecy[i + 1] = 0;
	}

	float a = vecy[0] * (img(y_f - 1, x_f - 1) * vecx[0] + img(y_f - 1, x_f) * vecx[1] + img(y_f - 1, x_f + 1) * vecx[2] + img(y_f - 1, x_f + 2) * vecx[3]);
	float b = vecy[1] * (img(y_f, x_f - 1) * vecx[0] + img(y_f, x_f) * vecx[1] + img(y_f, x_f + 1) * vecx[2] + img(y_f, x_f + 2) * vecx[3]);
	float c = vecy[2] * (img(y_f + 1, x_f - 1) * vecx[0] + img(y_f + 1, x_f) * vecx[1] + img(y_f + 1, x_f + 1) * vecx[2] + img(y_f + 1, x_f + 2) * vecx[3]);
	float d = vecy[3] * (img(y_f + 2, x_f - 1) * vecx[0] + img(y_f + 2, x_f) * vecx[1] + img(y_f + 2, x_f + 1) * vecx[2] + img(y_f + 2, x_f + 2) * vecx[3]);

	return a + b + c + d;
}

float Interpolation::Bicubic_Spline(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);

		if (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows())
			return img(ys, xs);
		else
			return 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

	float vecx[4];
	float vecy[4];

	for (int i = -1; i < 3; ++i) {
		double xd = fabs(dx - i);
		double yd = fabs(dy - i);

		if (0 <= xd && xd <= 1)
			vecx[i + 1] = float(1.5 * (xd * xd * xd) - 2.5 * (xd * xd) + 1);
		else if (1 < xd && xd <= 2)
			vecx[i + 1] = float(-.5 * (xd * xd * xd) + 2.5 * (xd * xd) - 4 * xd + 2);
		else
			vecx[i + 1] = 0;

		if (yd >= 0 && yd <= 1)
			vecy[i + 1] = float(1.5 * (yd * yd * yd) - 2.5 * (yd * yd) + 1);
		else if (1 < yd && yd <= 2)
			vecy[i + 1] = float(-.5 * (yd * yd * yd) + 2.5 * (yd * yd) - 4 * yd + 2);
		else
			vecy[i + 1] = 0;
	}

	float a = vecy[0] * (img(y_f - 1, x_f - 1) * vecx[0] + img(y_f - 1, x_f) * vecx[1] + img(y_f - 1, x_f + 1) * vecx[2] + img(y_f - 1, x_f + 2) * vecx[3]);
	float b = vecy[1] * (img(y_f, x_f - 1) * vecx[0] + img(y_f, x_f) * vecx[1] + img(y_f, x_f + 1) * vecx[2] + img(y_f, x_f + 2) * vecx[3]);
	float c = vecy[2] * (img(y_f + 1, x_f - 1) * vecx[0] + img(y_f + 1, x_f) * vecx[1] + img(y_f + 1, x_f + 1) * vecx[2] + img(y_f + 1, x_f + 2) * vecx[3]);
	float d = vecy[3] * (img(y_f + 2, x_f - 1) * vecx[0] + img(y_f + 2, x_f) * vecx[1] + img(y_f + 2, x_f + 1) * vecx[2] + img(y_f + 2, x_f + 2) * vecx[3]);

	return a + b + c + d;
}

float Interpolation::Bicubic_B_Spline(Image32& img, double& x_s, double& y_s) {

	if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) {
		int xs = (int)round(x_s);
		int ys = (int)round(y_s);
		if (xs >= 0 && xs < img.Cols() && ys >= 0 && ys < img.Rows())
			return img(ys, xs);
		else
			return 0;
	}

	int x_f = (int)floor(x_s);
	double dx = x_s - x_f;
	int y_f = (int)floor(y_s);
	double dy = y_s - y_f;

	float vecx[4];
	float vecy[4];

	for (int i = -1; i < 3; i++) {
		double xd = fabs(dx - i);
		double yd = fabs(dy - i);

		if (xd < 1)
			vecx[i + 1] = float(3 * (xd * xd * xd) - 6 * (xd * xd) + 4) / 6;
		else if (1 <= xd && xd < 2)
			vecx[i + 1] = float(-(xd * xd * xd) + 6 * (xd * xd) - 12 * xd + 8) / 6;
		else
			vecx[i + 1] = 0;

		if (yd < 1)
			vecy[i + 1] = float(3 * (yd * yd * yd) - 6 * (yd * yd) + 4) / 6;
		else if (1 <= yd && yd < 2)
			vecy[i + 1] = float(-(yd * yd * yd) + 6 * (yd * yd) - 12 * yd + 8) / 6;
		else
			vecy[i + 1] = 0;
	}

	float a = vecy[0] * (img(y_f - 1, x_f - 1) * vecx[0] + img(y_f - 1, x_f) * vecx[1] + img(y_f - 1, x_f + 1) * vecx[2] + img(y_f - 1, x_f + 2) * vecx[3]);
	float b = vecy[1] * (img(y_f, x_f - 1) * vecx[0] + img(y_f, x_f) * vecx[1] + img(y_f, x_f + 1) * vecx[2] + img(y_f, x_f + 2) * vecx[3]);
	float c = vecy[2] * (img(y_f + 1, x_f - 1) * vecx[0] + img(y_f + 1, x_f) * vecx[1] + img(y_f + 1, x_f + 1) * vecx[2] + img(y_f + 1, x_f + 2) * vecx[3]);
	float d = vecy[3] * (img(y_f + 2, x_f - 1) * vecx[0] + img(y_f + 2, x_f) * vecx[1] + img(y_f + 2, x_f + 1) * vecx[2] + img(y_f + 2, x_f + 2) * vecx[3]);

	return a + b + c + d;
}