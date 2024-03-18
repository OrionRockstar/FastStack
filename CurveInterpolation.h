#pragma once
#include "pch.h"

#include "Maths.h"

enum class CurveType {
	cubic_spline,
	akima_spline,
	linear
};

typedef std::vector<Pointf> Pointsf;

struct Curve {
private:
	typedef std::vector < std::array<double, 4>> SplineCoefs;

	std::vector<QPointF> m_points;// = std::vector<Pointf>(0);//{ Pointf(0,0), Pointf(1, 1) };
	SplineCoefs splc;
	CurveType m_type = CurveType::akima_spline;
	bool is_identity = true;

	static bool ComaparePoints(QPointF a, QPointF b) { return (a.x() < b.x()); }

public:
	Curve() = default;

	Curve(bool color_saturation);

	void SetInterpolationCurve(CurveType type) { m_type = type; }

	void SetCoeffecients();

	template<typename T, int Offset>
	struct MyVector {
		std::unique_ptr<T[]> data;
		int m_offset = Offset;
		int m_size = 0;

		MyVector(int size) : m_size(size) {
			data = std::make_unique<T[]>(size);
		}
		MyVector() = default;
		~MyVector() {};

		T& operator[](int index) {
			return data[index + m_offset];
		}

		int size()const { return m_size; }
	};

	bool IsIdentity();

private:
	bool OutRange(double val, double low = 0.0, double high = 1.0);

	void CubicSplineCurve();

	void AkimaSplineCurve();

	void LinearCurve();

public:

	double Interpolate(double pixel);

private:
	double AkimaInterpolator(double pixel);

	double CubicInterpolator(double pixel);

	double LinearInterpolator(double pixel);

public:
	void InsertDataPoints(std::vector<QPointF> pts) {

		m_points.resize(pts.size());
		memcpy(&m_points[0], &pts[0], m_points.size() * sizeof(QPointF));

	}
};

