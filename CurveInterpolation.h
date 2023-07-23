#pragma once
#include <array>
#include <vector>
#include <memory>
#include "Maths.h"

enum class CurveType {
	cubic_spline,
	akima_spline,
	linear
};

typedef std::vector<Pointf> Pointsf;

//typedef std::vector < std::array<double, 4>> SplineCoefs;
struct Curve {
	typedef std::vector < std::array<double, 4>> SplineCoefs;

	Pointsf points = { {0, 0}, {1, 1} };
	SplineCoefs splc;
	CurveType type = CurveType::akima_spline;
	bool do_interpolation = false;

	Curve() = default;

	Curve(bool color_saturation);

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

	bool OutRange(double val, double low = 0.0, double high = 1.0);

	void CubicSplineCurve();

	void AkimaSplineCurve();

	void LinearCurve();

public:
	void SetCoeffecients();

	double Interpolate(double pixel);

private:
	double AkimaInterpolator(double pixel);

	double CubicInterpolator(double pixel);

	double LinearInterpolator(double pixel);

public:
	bool InsertPoints(Pointsf& other);

	bool InsertPoints_CS(Pointsf& other);

	bool InesrtPoint(Pointf other);

	bool InesrtPoint_CS(Pointf other);

	bool RemovePoint(int el);
};

