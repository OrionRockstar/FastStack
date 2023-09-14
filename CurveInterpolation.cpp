#include "pch.h"
#include "CurveInterpolation.h"
#include "Matrix.h"


Curve::Curve(bool color_saturation) {
	if (color_saturation) {
		points = { {0, 0}, {1, 0} };
		do_interpolation = true;
	}
	else
		points = { {0, 0}, {1, 1} };
}

bool Curve::IsIdentity() {
	return (points.size() == 2 && points[0].x == 0 && points[0].y == 0 && points[1].x == 1 && points[1].y == 1);
}

bool Curve::OutRange(double val, double low, double high) {
	return (val < low || val > high) ? true : false;
}

void Curve::CubicSplineCurve() {

	std::sort(points.begin(), points.end(), Pointf());
	int dim = 4 * (points.size() - 1);

	Matrix x(dim, dim);
	Matrix constants(dim);
	Matrix y(dim);

	int start = 0;
	for (int row = start, offset = 0, el = 0; row < 2 * points.size() - 2; ++row) {

		x(row, 3 + offset) = 1;
		x(row, 2 + offset) = points[el].x;
		x(row, 1 + offset) = points[el].x * points[el].x;
		x(row, offset) = points[el].x * x(row, 1 + offset);

		y[row] = points[el].y;

		if (row % 2 != 0 && row != start)
			offset += 4;
		if (row % 2 == 0)
			el++;
	}

	start += (2 * points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (points.size() - 2); ++row, offset += 4, ++el) {

		x(row, offset) = 3 * (points[el].x * points[el].x);
		x(row, 1 + offset) = 2 * points[el].x;
		x(row, 2 + offset) = 1;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -x(row, 1 + offset);
		x(row, 6 + offset) = -1;

		y[row] = 0;
	}

	start += (points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (points.size() - 2); ++row, offset += 4, ++el) {
		x(row, offset) = 6 * points[el].x;
		x(row, 1 + offset) = 2;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -2;

		y[row] = 0;
	}

	start += (points.size() - 2);
	x(start, 1) = 2;
	start++;
	x(start, dim - 4) = 6 * points[points.size() - 1].x;
	x(start, dim - 3) = 2;


	constants = (x.Transpose() * x).Inverse() * x.Transpose() * y;

	splc.resize(points.size() - 1);

	for (int el = 0, offset = 0; el < splc.size(); ++el, offset += 4)
		splc[el] = { constants[offset],constants[1 + offset],constants[2 + offset],constants[3 + offset] };

}

void Curve::AkimaSplineCurve() {

	std::sort(points.begin(), points.end(), Pointf());

	int spline_count = points.size() - 1;

	MyVector<double, 2> m_vec(spline_count + 4);
	for (int el = 0; el < spline_count; ++el)
		m_vec[el] = (points[el + 1].y - points[el].y) / (points[el + 1].x - points[el].x);

	std::vector<double> s_vec(points.size());

	m_vec[-2] = 3 * m_vec[0] - 2 * m_vec[1];
	m_vec[-1] = 2 * m_vec[0] - m_vec[1];
	m_vec[spline_count] = 2 * m_vec[spline_count - 1] - m_vec[spline_count - 2];
	m_vec[spline_count + 1] = 3 * m_vec[spline_count - 1] - 2 * m_vec[spline_count - 2];


	for (int el = 0; el <= spline_count; ++el) {

		double a = abs(m_vec[el + 1] - m_vec[el]);
		double b = abs(m_vec[el - 1] - m_vec[el - 2]);

		s_vec[el] = (a + b != 0) ? ((a * m_vec[el - 1]) + (b * m_vec[el])) / (a + b) : 0.5 * (m_vec[el - 1] + m_vec[el]);

	}

	splc.resize(spline_count);

	for (int el = 0; el < spline_count; ++el) {
		
		splc[el][3] = points[el].y;
		splc[el][2] = s_vec[el];
		double dx = points[el + 1].x - points[el].x;
		splc[el][1] = (3 * m_vec[el] - 2 * s_vec[el] - s_vec[el + 1]) / dx;
		splc[el][0] = (s_vec[el] + s_vec[el + 1] - 2 * m_vec[el]) / (dx * dx);

	}
}

void Curve::LinearCurve() {

	std::sort(points.begin(), points.end(), Pointf());

	splc.resize(points.size() - 1);

	for (int i = 0; i < points.size() - 1; ++i)
		splc[i][0] = (points[i + 1].y - points[i].y) / (points[i + 1].x - points[i].x);

}

void Curve::SetCoeffecients() {
	using enum CurveType;

	switch (type) {
	case akima_spline:
		if (points.size() <= 4)
			return CubicSplineCurve();
		else
			return AkimaSplineCurve();

	case cubic_spline:
		return CubicSplineCurve();

	case linear:
		return LinearCurve();

	}
}

double Curve::Interpolate(double pixel) {
	if (!do_interpolation)
		return pixel;

	using enum CurveType;
	switch (type) {
	case akima_spline:
		if (points.size() > 4)
			return AkimaInterpolator(pixel);
	case cubic_spline:
		return CubicInterpolator(pixel);
	case linear:
		return LinearInterpolator(pixel);
	}
}

double Curve::AkimaInterpolator(double pixel) {
	for (int el = 0; el < points.size() - 1; ++el) {
		if (points[el].x <= pixel && pixel < points[el + 1].x) {
			pixel -= points[el].x;
			return Clip((splc[el][0] * pixel * pixel * pixel + splc[el][1] * pixel * pixel + splc[el][2] * pixel + splc[el][3]));
		}
	}
	return pixel;
}

double Curve::CubicInterpolator(double pixel) {
	if (points.size() == 0) return pixel;

	for (int el = 0; el < points.size() - 1; ++el)
		if (points[el].x <= pixel && pixel < points[el + 1].x)
			return (splc[el][0] * pixel * pixel * pixel + splc[el][1] * pixel * pixel + splc[el][2] * pixel + splc[el][3]);

	return pixel;
}

double Curve::LinearInterpolator(double pixel) {
	if (points.size() == 0) return pixel;

	for (int el = 0; el < points.size() - 1; ++el)
		if (points[el].x <= pixel && pixel < points[el + 1].x)
			return (splc[el][0] * (pixel - points[el].x) + points[el].y);

	return pixel;
}

bool Curve::InsertPoints(Pointsf& other) {

	for (int i = 0, a = 1; i < other.size(); ++i) {

		if (OutRange(other[i].x) || OutRange(other[i].y))
			continue;

		bool new_point = true;
		for (auto p : points)
			if (other[i].x == p.x)
				new_point = false;

		if (new_point)
			points.insert(points.begin() + a++, { other[i] });
	}
	return (!IsIdentity()) ? true : false;
}

bool Curve::InsertPoints_CS(Pointsf& other) {

	for (int i = 0, a = 1; i < other.size(); ++i) {

		if (OutRange(other[i].x) || OutRange(other[i].y, -10, 10))
			continue;

		bool new_point = true;
		for (auto p : points)
			if (other[i].x == p.x)
				new_point = false;

		if (new_point)
			points.insert(points.begin() + a++, { other[i] });
	}
	return (points.size() != 2) ? true : false;
}

bool Curve::InesrtPoint(Pointf other) {

	if (OutRange(other.x) || OutRange(other.y))
		return false;

	if (other.x == 0)
		return false;

	for (int el = 0; el < points.size() - 1; ++el) {

		if (points[el].x < other.x && other.x < points[el + 1].x) {
			points.insert(points.begin() + el, { other });
			return true;
		}

		if (other.x == points[el + 1].x)
			return false;
	}
}

bool Curve::InesrtPoint_CS(Pointf other) {

	if (OutRange(other.x) || OutRange(other.y, -10, 10))
		return false;

	if (other.x == 0)
		return false;

	for (int el = 0; el < points.size() - 1; ++el) {

		if (points[el].x < other.x && other.x < points[el + 1].x) {
			points.insert(points.begin() + el, { other });
			return true;
		}

		if (other.x == points[el + 1].x)
			return false;
	}
}

bool Curve::RemovePoint(int el) {

	if (el == 0 || el >= points.size() - 1)
		return false;

	points.erase(points.begin() + el);

	if (IsIdentity())
		do_interpolation = false;
}