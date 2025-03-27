#include "pch.h"
#include "FastStack.h"
#include "ImageGeometry.h"




template <typename T>
void Rotation::apply(Image<T>& src) {

	float theta = m_theta * (M_PI / 180.0);
	float s = sin(theta);
	float c = cos(theta);

	Image<T> temp(fabs(src.rows() * c) + fabs(src.cols() * s), fabs(src.cols() * c) + fabs(src.rows() * s), src.channels());

	float hc = temp.cols() / 2;
	float hr = temp.rows() / 2;

	float offsetx = hc - (temp.cols() - src.cols()) / 2;
	float offsety = hr - (temp.rows() - src.rows()) / 2;

	for (uint32_t ch = 0; ch < src.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.rows(); ++y) {

			double yx = (y - hr) * s;
			double yy = (y - hr) * c;

			for (int x = 0; x < temp.cols(); ++x) {

				double x_s = ((x - hc) * c - yx) + offsetx;
				double y_s = ((x - hc) * s + yy) + offsety;

				temp(x, y, ch) = Interpolator(m_interpolate).interpolatePixel(src, { x_s, y_s, ch });

			}
		}
	}
	temp.moveTo(src);
}
template void Rotation::apply(Image8&);
template void Rotation::apply(Image16&);
template void Rotation::apply(Image32&);








template<typename T>
void FastRotation::rotate90CW(Image<T>& src) {

	Image<T> temp(src.cols(), src.rows(), src.channels());

	int offsety = temp.cols() - 1;

	for (int ch = 0; ch < temp.channels(); ++ch) {
		for (int y = 0, x_s = y; y < temp.rows(); ++y, ++x_s) {
			for (int x = 0; x < temp.cols(); ++x) {
				int y_s = offsety - x;
				temp(x, y, ch) = src(x_s, y_s, ch);
			}
		}
	}
	
	temp.moveTo(src);
}

template<typename T>
void FastRotation::rotate90CCW(Image<T>& src) {

	Image<T> temp(src.cols(), src.rows(), src.channels());

	int offsetx = temp.rows() - 1;

	for (int ch = 0; ch < temp.channels(); ++ch) {
		for (int y = 0; y < temp.rows(); ++y) {
			int x_s = offsetx - y;
			for (int x = 0, y_s = 0; x < temp.cols(); ++x, ++y_s) {
				temp(x, y, ch) = src(x_s, y_s, ch);
			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void FastRotation::rotate180(Image<T>& src) {

	int r = src.rows() - 1;
	int c = src.cols() - 1;

	for (int ch = 0; ch < src.channels(); ++ch) {
		for (int y = 0; y < src.rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0, x_s = c; x < src.cols(); ++x, --x_s) {
				std::swap(src(x, y, ch), src(x_s, y_s, ch));
			}
		}
	}
}

template<typename T>
void FastRotation::horizontalMirror(Image<T>& src) {

	int c = src.cols() - 1;
	int hc = src.cols() / 2;

	for (int ch = 0; ch < src.channels(); ++ch) {
		for (int y = 0; y < src.rows(); ++y) {
			for (int x = 0, x_s = c; x < hc; ++x, --x_s) {
				std::swap(src(x, y, ch), src(x_s, y, ch));
			}
		}
	}
}

template<typename T>
void FastRotation::verticalMirror(Image<T>& src) {

	int r = src.rows() - 1;

	for (int ch = 0; ch < src.channels(); ++ch) {
		for (int y = 0; y < src.rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0; x < src.cols(); ++x) {
				std::swap(src(x, y, ch), src(x, y_s, ch));
			}
		}
	}
}

template<typename T>
void FastRotation::apply(Image<T>& src) {

	switch (m_frt) {
	case Type::rotate90CW:
		return rotate90CW(src);

	case Type::rotate90CCW:
		return rotate90CCW(src);

	case Type::rotate180:
		return rotate180(src);

	case Type::horizontalmirror:
		return horizontalMirror(src);

	case Type::verticalmirror:
		return verticalMirror(src);

	default:
		return;
	}
}
template void FastRotation::apply(Image8& src);
template void FastRotation::apply(Image16& src);
template void FastRotation::apply(Image32& src);






template<typename T>
void IntegerResample::downsample_average(Image<T>& src) {

	Image<T> temp(src.rows() / m_factor, src.cols() / m_factor, src.channels());
	int factor2 = m_factor * m_factor;

	for (int ch = 0; ch < temp.channels(); ++ch) {

		for (int y = 0; y < temp.rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.cols(); ++x) {
				int x_s = m_factor * x;

				float pix = 0;
				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						pix += src(x_s + i, y_s + j, ch);

				temp(x, y, ch) = pix / factor2;

			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void IntegerResample::downsample_median(Image<T>& src) {

	Image<T> temp(src.rows() / m_factor, src.cols() / m_factor, src.channels());
	std::vector<float> kernel(m_factor * m_factor);
	auto mp = kernel.begin() + m_factor / 2;

	for (int ch = 0; ch < temp.channels(); ++ch) {

		for (int y = 0; y < temp.rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.cols(); ++x) {
				int x_s = m_factor * x;


				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						kernel[j * m_factor + i] = src(x_s + i, y_s + j, ch);

				std::nth_element(kernel.begin(), mp, kernel.end());
				temp(x, y, ch) = *mp;

			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void IntegerResample::downsample_max(Image<T>& src) {

	Image<T> temp(src.rows() / m_factor, src.cols() / m_factor, src.channels());

	for (int ch = 0; ch < temp.channels(); ++ch) {

		for (int y = 0; y < temp.rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.cols(); ++x) {
				int x_s = m_factor * x;

				T max = 0;

				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						if (src(x_s + i, y_s + j, ch) > max)
							max = src(x_s + i, y_s + j, ch);

				temp(x, y, ch) = max;

			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void IntegerResample::downsample_min(Image<T>& src) {

	Image<T> temp(src.rows() / m_factor, src.cols() / m_factor, src.channels());

	for (int ch = 0; ch < temp.channels(); ++ch) {

		for (int y = 0; y < temp.rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.cols(); ++x) {
				int x_s = m_factor * x;

				T min = Pixel<T>::max();
				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						if (src(x_s + i, y_s + j, ch) < min)
							min = src(x_s + i, y_s + j, ch);

				temp(x, y, ch) = min;

			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void IntegerResample::downsample(Image<T>& src) {
	using enum IntegerResample::Method;

	if (m_factor >= src.rows() || m_factor >= src.cols())
		return;

	switch (m_method) {
	case average:
		return downsample_average(src);

	case median:
		return downsample_median(src);

	case max:
		return downsample_max(src);

	case min:
		return downsample_min(src);

	default:
		return;
	}
}

template<typename T>
void IntegerResample::upsample(Image<T>& src) {

	Image<T> temp(src.rows() * m_factor, src.cols() * m_factor, src.channels());

	for (int ch = 0; ch < temp.channels(); ++ch) {

		for (int y = 0; y < src.rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < src.cols(); ++x) {
				int x_s = m_factor * x;

				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						temp(x_s + i, y_s + j, ch) = src(x, y, ch);

			}
		}
	}

	temp.moveTo(src);
}

template<typename T>
void IntegerResample::apply(Image<T>& src) {

	if (m_factor > 100) return;

	switch (m_type) {
	case Type::downsample:
		return downsample(src);

	case Type::upsample:
		return upsample(src);

	default:
		return;
	}
}
template void IntegerResample::apply(Image8&);
template void IntegerResample::apply(Image16&);
template void IntegerResample::apply(Image32&);







template<typename T>
void Resize::apply(Image<T>& src) {

	Image<T> temp(m_new_rows, m_new_cols, src.channels());

	double ry = double(src.rows()) / temp.rows();
	double rx = double(src.cols()) / temp.cols();

	for (uint32_t ch = 0; ch < src.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = Interpolator(m_type).interpolatePixel(src, { x_s, y_s, ch });
			}
		}
	}
	
	temp.moveTo(src);
}
template void Resize::apply(Image8&);
template void Resize::apply(Image16&);
template void Resize::apply(Image32&);





template<typename T>
void Crop::apply(Image<T>& src) {

	Image<T> temp(m_y2 - m_y1, m_x2 - m_x1, src.channels());

	for (int ch = 0; ch < src.channels(); ++ch)
		for (int y = 0; y < temp.rows(); ++y)
			for (int x = 0; x < temp.cols(); ++x)
				temp(x, y, ch) = src(x + m_x1, y + m_y1, ch);

	src = std::move(temp);
}
template void Crop::apply(Image8&);
template void Crop::apply(Image16&);
template void Crop::apply(Image32&);







template<typename T>
void HomographyTransformation::apply(Image<T>& src) {

	Image<T> temp(src.rows(), src.cols(), src.channels());

	Matrix inverse = m_homography.inverse();

	for (uint32_t ch = 0; ch < src.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < src.rows(); ++y) {

			double yx = y * m_homography(0, 1);
			double yy = y * m_homography(1, 1);
			double yz = y * m_homography(2, 1);

			for (int x = 0; x < src.cols(); ++x) {
				double x_s = x * m_homography(0, 0) + yx + m_homography(0, 2);
				double y_s = x * m_homography(1, 0) + yy + m_homography(1, 2);
				double zed = x * m_homography(2, 0) + yz + m_homography(2, 2);

				temp(x, y, ch) = Interpolator(m_type).interpolatePixel(src, { x_s / zed, y_s / zed, ch });
			}
		}
	}

	temp.moveTo(src);
}
template void HomographyTransformation::apply(Image8&);
template void HomographyTransformation::apply(Image16&);
template void HomographyTransformation::apply(Image32&);


