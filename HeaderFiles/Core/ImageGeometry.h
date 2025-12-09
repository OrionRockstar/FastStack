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

	void reset() {
		m_theta = 0.0;
		m_interpolate = Interpolator::Type::bicubic_spline;
	}

	template <typename T>
	void apply(Image<T>& src);
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
	FastRotation() = default;

	FastRotation(Type type) : m_frt(type) {}

	void setFastRotationType(Type type) { m_frt = type; }

private:
	template<typename T>
	void rotate90CW(Image<T>& src);

	template<typename T>
	void rotate90CCW(Image<T>& src);

	template<typename T>
	void rotate180(Image<T>& src);

	template<typename T>
	void horizontalMirror(Image<T>& src);

	template<typename T>
	void verticalMirror(Image<T>& src);

public:
	template<typename T>
	void apply(Image<T>& src);
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
	void downsample_average(Image<T>& src);

	template<typename T>
	void downsample_median(Image<T>& src);

	template<typename T>
	void downsample_max(Image<T>& src);

	template<typename T>
	void downsample_min(Image<T>& src);

	template<typename T>
	void downsample(Image<T>& src);

	template<typename T>
	void upsample(Image<T>& src);

public:
	uint8_t factor()const { return m_factor; }

	void setFactor(uint8_t factor) { m_factor = factor; }

	Type type()const { return m_type; }

	void setType(Type type) { m_type = type; }

	Method method()const { return m_method; }

	void setMethod(Method method) { m_method = method; }

	template<typename T>
	void apply(Image<T>& src);
};




class Resize {

	Interpolator::Type m_type = Interpolator::Type::bicubic_spline;
	int m_new_rows = 1;
	int m_new_cols = 1;

public:
	int newRows()const { return m_new_rows; }

	int newCols()const { return m_new_cols; }

	void setNewRows(int n_rows) { m_new_rows = n_rows; }

	void setNewCols(int n_cols) { m_new_cols = n_cols; }

	void setNewSize(int n_rows, int n_cols) {
		m_new_rows = n_rows;
		m_new_cols = n_cols;
	}

	void setInterpolation(Interpolator::Type type) { m_type = type; }

	template<typename T>
	void apply(Image<T>& src);
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
	void apply(Image<T>& src);
};





class HomographyTransformation {

	Matrix m_homography = Matrix(3,3).identity();
	Interpolator::Type m_type = Interpolator::Type::bicubic_spline;
public:
	void setHomography(const Matrix& homography) {
		if (homography.isSize(3, 3))
			m_homography = homography;
	}

	void setInterpolationType(Interpolator::Type type) {
		m_type = type;
	}

	template<typename T>
	void apply(Image<T>& src);
};
