#include "pch.h"
#include "CurveInterpolation.h"
#include "Matrix.h"


Curve::Curve(bool color_saturation) {
	if (color_saturation) {
		m_points = { {0, 0}, {1, 0} };
		is_identity = false;
	}
	else
		m_points = { {0, 0}, {1, 1} };
}

bool Curve::IsIdentity() {
	return is_identity = (m_points.size() == 2 && m_points[0].x() == 0 && m_points[0].y() == 0 && m_points[1].x() == 1 && m_points[1].y() == 1);
}

bool Curve::OutRange(double val, double low, double high) {
	return (val < low || val > high) ? true : false;
}

void Curve::CubicSplineCurve() {

	std::sort(m_points.begin(), m_points.end(), ComaparePoints);

	int dim = 4 * (m_points.size() - 1);

	Matrix x(dim, dim);
	Matrix constants(dim);
	Matrix y(dim);

	int start = 0;
	for (int row = start, offset = 0, el = 0; row < 2 * m_points.size() - 2; ++row) {

		x(row, 3 + offset) = 1;
		x(row, 2 + offset) = m_points[el].x();
		x(row, 1 + offset) = m_points[el].x() * m_points[el].x();
		x(row, offset) = m_points[el].x() * x(row, 1 + offset);

		y[row] = m_points[el].y();

		if (row % 2 != 0 && row != start)
			offset += 4;
		if (row % 2 == 0)
			el++;
	}

	start += (2 * m_points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (m_points.size() - 2); ++row, offset += 4, ++el) {

		x(row, offset) = 3 * (m_points[el].x() * m_points[el].x());
		x(row, 1 + offset) = 2 * m_points[el].x();
		x(row, 2 + offset) = 1;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -x(row, 1 + offset);
		x(row, 6 + offset) = -1;

		y[row] = 0;
	}

	start += (m_points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (m_points.size() - 2); ++row, offset += 4, ++el) {
		x(row, offset) = 6 * m_points[el].x();
		x(row, 1 + offset) = 2;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -2;

		y[row] = 0;
	}

	start += (m_points.size() - 2);
	x(start, 1) = 2;
	start++;
	x(start, dim - 4) = 6 * m_points[m_points.size() - 1].x();
	x(start, dim - 3) = 2;


	constants = (x.Transpose() * x).Inverse() * x.Transpose() * y;

	splc.resize(m_points.size() - 1);


	for (int el = 0, offset = 0; el < splc.size(); ++el, offset += 4)
		splc[el] = { constants[offset],constants[1 + offset],constants[2 + offset],constants[3 + offset] };

}

void Curve::AkimaSplineCurve() {

	std::sort(m_points.begin(), m_points.end(), ComaparePoints);

	int spline_count = m_points.size() - 1;

	MyVector<double, 2> m_vec(spline_count + 4);
	for (int el = 0; el < spline_count; ++el)
		m_vec[el] = (m_points[el + 1].y() - m_points[el].y()) / (m_points[el + 1].x() - m_points[el].x());

	std::vector<double> s_vec(m_points.size());

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
		
		splc[el][3] = m_points[el].y();
		splc[el][2] = s_vec[el];
		double dx = m_points[el + 1].x() - m_points[el].x();
		splc[el][1] = (3 * m_vec[el] - 2 * s_vec[el] - s_vec[el + 1]) / dx;
		splc[el][0] = (s_vec[el] + s_vec[el + 1] - 2 * m_vec[el]) / (dx * dx);

	}
}

void Curve::LinearCurve() {

	std::sort(m_points.begin(), m_points.end(), ComaparePoints);

	splc.resize(m_points.size() - 1);

	for (int i = 0; i < m_points.size() - 1; ++i)
		splc[i][0] = (m_points[i + 1].y() - m_points[i].y()) / (m_points[i + 1].x() - m_points[i].x());

}

void Curve::SetCoeffecients() {
	using enum CurveType;

	//if (points.size() == 2)
		//return LinearCurve();

	switch (m_type) {
	case akima_spline:
		if (m_points.size() > 4)
			return AkimaSplineCurve();

	case cubic_spline:
		return CubicSplineCurve();

	case linear:
		return LinearCurve();

	}
}

double Curve::Interpolate(double pixel) {

	if (is_identity)
		return pixel;

	using enum CurveType;
	switch (m_type) {
	case akima_spline:
		if (m_points.size() > 4)
			return AkimaInterpolator(pixel);
	case cubic_spline:
		//if (points.size() > 2)
			return CubicInterpolator(pixel);
	case linear:
		return LinearInterpolator(pixel);

	default:
		return CubicInterpolator(pixel);
	}
}

double Curve::AkimaInterpolator(double pixel) {
	for (int el = 0; el < m_points.size() - 1; ++el) {
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x()) {
			pixel -= m_points[el].x();
			return Clip((splc[el][0] * pixel * pixel * pixel + splc[el][1] * pixel * pixel + splc[el][2] * pixel + splc[el][3]));
		}
	}
	return pixel;
}

double Curve::CubicInterpolator(double pixel) {
	if (m_points.size() == 0) return pixel;
	for (int el = 0; el < m_points.size() - 1; ++el)
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x()) 
			return (splc[el][0] * pixel * pixel * pixel + splc[el][1] * pixel * pixel + splc[el][2] * pixel + splc[el][3]);
		
	return pixel;
}

double Curve::LinearInterpolator(double pixel) {
	if (m_points.size() == 0) return pixel;

	for (int el = 0; el < m_points.size() - 1; ++el)
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x())
			return (splc[el][0] * (pixel - m_points[el].x()) + m_points[el].y());

	return pixel;
}

