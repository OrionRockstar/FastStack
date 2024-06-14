#pragma once
#include "Image.h"
#include "Interpolator.h"
#include "Matrix.h"

class Rotation {

	float m_theta = 0.0;
	Interpolator::Type m_interpolate = Interpolator::Type::bicubic_spline;

public:

	void setTheta(double theta) { m_theta = theta; }

	void setInterpolate(Interpolator::Type interpolate) { m_interpolate = interpolate; }

	void Reset() {
		m_theta = 0.0;
		m_interpolate = Interpolator::Type::bicubic_spline;
	}

	template <typename T>
	void Apply(Image<T>& src);
};





class FastRotation {
public:
	enum class Type {
		rotate90CW,
		rotate90CCW,
		rotate180,
		horizontalmirror,
		verticalmirror
	};

private:
	Type m_frt = Type::rotate90CW;

public:
	void setFastRotationType(Type type) { m_frt = type; }

private:
	template<typename T>
	void Rotate90CW(Image<T>& src);

	template<typename T>
	void Rotate90CCW(Image<T>& src);

	template<typename T>
	void Rotate180(Image<T>& src);

	template<typename T>
	void HorizontalMirror(Image<T>& src);

	template<typename T>
	void VerticalMirror(Image<T>& src);

public:
	template<typename T>
	void Apply(Image<T>& src);
};





class IntegerResample {
public:
	enum class Type{
		downsample,
		upsample
	};

	enum class Method {
		average,
		median,
		max,
		min
	};

private:
	uint8_t m_factor = 1;
	Type m_type = Type::downsample;
	Method m_method = Method::average;

	template<typename T>
	void Downsample_average(Image<T>& src);

	template<typename T>
	void Downsample_median(Image<T>& src);

	template<typename T>
	void Downsample_max(Image<T>& src);

	template<typename T>
	void Downsample_min(Image<T>& src);

	template<typename T>
	void Downsample(Image<T>& src);

	template<typename T>
	void Upsample(Image<T>& src);

public:
	uint8_t factor()const { return m_factor; }

	void setFactor(uint8_t factor) { m_factor = factor; }

	Type type()const { return m_type; }

	void setType(Type type) { m_type = type; }

	Method method()const { return m_method; }

	void setMethod(Method method) { m_method = method; }

	template<typename T>
	void Apply(Image<T>& src);
};




class Resize {

	Interpolator::Type m_type = Interpolator::Type::bicubic_spline;
	int m_new_rows = 1;
	int m_new_cols = 1;

public:
	template<typename T>
	void Apply(Image<T>& src);
};





class Crop {

	int m_x1 = 0;
	int m_x2 = 0;
	int m_y1 = 0;
	int m_y2 = 0;
public:
	void setRegion(const QRect& rect) {
		m_x1 = rect.topLeft().x();
		m_x2 = rect.topRight().x();
		m_y1 = rect.topLeft().y();
		m_y2 = rect.bottomLeft().y();
	}

	template<typename T>
	void Apply(Image<T>& src);
};





class HomographyTransformation {
	Matrix m_homography = Matrix(3,3).Identity();
	Interpolator::Type m_type = Interpolator::Type::bicubic_spline;
public:
	void setHomography(const Matrix& homography) {
		if (homography.isSize(3, 3))
			m_homography = homography;
		m_homography.Print();
	}

	void setIinterpolationType(Interpolator::Type type) {
		m_type = type;
	}

	template<typename T>
	void Apply(Image<T>& src);
};
