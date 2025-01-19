#pragma once
#include "Image.h"
#include <array>
#include "Maths.h"


class Interpolator {
public:
	enum class Type {
		nearest_neighbor,
		bilinear,
		bicubic_spline,
		bicubic_b_spline,
		cubic_b_spline,
		catmull_rom,
		lanczos3
	};

	Interpolator::Type m_type = Interpolator::Type::bicubic_spline;

	Interpolator(Interpolator::Type type = Interpolator::Type::bicubic_spline) : m_type(type) {}

	//void setType() {}
private:
	template <typename T>
	std::array<float, 4> getKernelRow(const Image<T>& img, const ImagePoint& p) {
		std::array<float, 4> pixrow;
		const T* im = &img(p);

		pixrow[0] = *(--im);
		pixrow[1] = *(++im);
		pixrow[2] = *(++im);
		pixrow[3] = *(++im);

		return pixrow;
	}

	template <typename T>
	std::array<float, 6> getKernelRow6(const Image<T>& img, const ImagePoint& p) {
		std::array<float, 6> pixrow;
		const T* im = &img(p);

		pixrow[0] = *(im -= 2);
		pixrow[1] = *(++im);
		pixrow[2] = *(++im);
		pixrow[3] = *(++im);
		pixrow[4] = *(++im);
		pixrow[5] = *(++im);

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

	template <typename T>
	float NearestNeighbor(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		float dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		float dy = p.y() - y_f;

		x_f += (dx > .5) ? 1 : 0;
		y_f += (dy > .5) ? 1 : 0;

		return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
	}

	template <typename T>
	float Bilinear(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		float dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		float dy = p.y() - y_f;

		if (IsOutRange(x_f, 0, img.cols() - 1) || IsOutRange(y_f, 0, img.rows() - 1)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
		}

		float r1 = img(x_f, y_f, p.channel()) * (1 - dx) + img(x_f + 1, y_f, p.channel()) * dx;
		float r2 = img(x_f, y_f + 1, p.channel()) * (1 - dx) + img(x_f + 1, y_f + 1, p.channel()) * dx;

		return r1 * (1 - dy) + r2 * dy;
	}

	template <typename T>
	float Bicubic_Spline(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		double dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		double dy = p.y() - y_f;

		if (IsOutRange(x_f, 1, img.cols() - 2) || IsOutRange(y_f, 1, img.rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
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

		ImagePoint s = { x_f, y_f, p.channel() };
		s.ry()--;
		resxv[0] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[1] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[2] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[3] = InterpolatePix(getKernelRow(img, s), vecx);

		return InterpolatePix(resxv, vecy);
	}

	template <typename T>
	float Bicubic_B_Spline(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		double dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		double dy = p.y() - y_f;

		if (IsOutRange(x_f, 1, img.cols() - 2) || IsOutRange(y_f, 1, img.rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
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

	template <typename T>
	float Cubic_B_Spline(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		double dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		double dy = p.y() - y_f;

		if (IsOutRange(x_f, 1, img.cols() - 2) || IsOutRange(y_f, 1, img.rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
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

		ImagePoint s = { x_f, y_f, p.channel() };
		s.ry()--;
		resxv[0] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[1] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[2] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[3] = InterpolatePix(getKernelRow(img, s), vecx);

		return InterpolatePix(resxv, vecy);
	}

	template <typename T>
	float Catmull_Rom(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		double dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		double dy = p.y() - y_f;

		if (IsOutRange(x_f, 1, img.cols() - 2) || IsOutRange(y_f, 1, img.rows() - 2)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
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

		ImagePoint s = { x_f, y_f, p.channel() };
		s.ry()--;
		resxv[0] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[1] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[2] = InterpolatePix(getKernelRow(img, s), vecx);
		s.ry()++;
		resxv[3] = InterpolatePix(getKernelRow(img, s), vecx);

		return InterpolatePix(resxv, vecy);
	}

	template <typename T>
	float Lanczos3(const Image<T>& img, const DoubleImagePoint& p) {

		int x_f = (int)floor(p.x());
		double dx = p.x() - x_f;
		int y_f = (int)floor(p.y());
		double dy = p.y() - y_f;

		if (IsOutRange(x_f, 2, img.cols() - 3) || IsOutRange(y_f, 2, img.rows() - 3)) {

			x_f += (dx > .5) ? 1 : 0;
			y_f += (dy > .5) ? 1 : 0;

			return img.isInBounds(x_f, y_f) ? img(x_f, y_f, p.channel()) : 0;
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

		ImagePoint s = { x_f,y_f,p.channel() };
		s.ry() -= 2;
		resxv[0] = InterpolatePix(getKernelRow6(img, s), vecx);
		s.ry()++;
		resxv[1] = InterpolatePix(getKernelRow6(img, s), vecx);
		s.ry()++;
		resxv[2] = InterpolatePix(getKernelRow6(img, s), vecx);
		s.ry()++;
		resxv[3] = InterpolatePix(getKernelRow6(img, s), vecx);
		s.ry()++;
		resxv[4] = InterpolatePix(getKernelRow6(img, s), vecx);	
		s.ry()++;
		resxv[5] = InterpolatePix(getKernelRow6(img, s), vecx);

		return InterpolatePix(resxv, vecy) / GetWeight(vecx, vecy);
	}

public:
	template <typename T>
	T interpolatePixel(const Image<T>& img, const DoubleImagePoint& p) {
		using enum Type;
		
		switch (m_type) {
		case nearest_neighbor:
			return NearestNeighbor(img, p);

		case bilinear:
			return Bilinear(img, p);

		case bicubic_spline:
			return math::clip(Bicubic_Spline(img, p));

		case bicubic_b_spline:
			return math::clip(Bicubic_B_Spline(img, p));

		case cubic_b_spline:
			return math::clip(Cubic_B_Spline(img, p));

		case catmull_rom:
			return math::clip(Catmull_Rom(img, p));

		case lanczos3:
			return math::clip(Lanczos3(img, p));

		default:
			return math::clip(Bicubic_Spline(img, p));
		}
	}
};
