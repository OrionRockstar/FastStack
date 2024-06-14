#include "pch.h"
#include "FastStack.h"
#include "ImageGeometry.h"


//HAVE SEPERATE FILES FOR DIALOGS, ImageGeometryDialogs

template <typename T>
void Rotation::Apply(Image<T>& src) {

	float theta = m_theta * (M_PI / 180.0);
	float s = sin(theta);
	float c = cos(theta);

	Image<T> temp(fabs(src.Rows() * c) + fabs(src.Cols() * s), fabs(src.Cols() * c) + fabs(src.Rows() * s), src.Channels());

	float hc = temp.Cols() / 2;
	float hr = temp.Rows() / 2;

	float offsetx = hc - (temp.Cols() - src.Cols()) / 2;
	float offsety = hr - (temp.Rows() - src.Rows()) / 2;

	for (int ch = 0; ch < src.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {

			double yx = (y - hr) * s;
			double yy = (y - hr) * c;

			for (int x = 0; x < temp.Cols(); ++x) {

				double x_s = ((x - hc) * c - yx) + offsetx;
				double y_s = ((x - hc) * s + yy) + offsety;

				temp(x, y, ch) = Interpolator().InterpolatePixel(src, x_s, y_s, ch, m_interpolate);

			}
		}
	}

	temp.MoveTo(src);
}
template void Rotation::Apply(Image8&);
template void Rotation::Apply(Image16&);
template void Rotation::Apply(Image32&);








template<typename T>
void FastRotation::Rotate90CW(Image<T>& src) {

	Image<T> temp(src.Cols(), src.Rows(), src.Channels());

	int offsety = temp.Cols() - 1;

	for (int ch = 0; ch < temp.Channels(); ++ch) {
		for (int y = 0, x_s = y; y < temp.Rows(); ++y, ++x_s) {
			for (int x = 0; x < temp.Cols(); ++x) {
				int y_s = offsety - x;
				temp(x, y, ch) = src(x_s, y_s, ch);
			}
		}
	}
	
	temp.MoveTo(src);
}
//void FastRotation::Rotate90CW(Image<T>& src)

template<typename T>
void FastRotation::Rotate90CCW(Image<T>& src) {

	Image<T> temp(src.Cols(), src.Rows());

	int offsetx = temp.Rows() - 1;

	for (int ch = 0; ch < temp.Channels(); ++ch) {
		for (int y = 0; y < temp.Rows(); ++y) {
			int x_s = offsetx - y;
			for (int x = 0, y_s = 0; x < temp.Cols(); ++x, ++y_s) {
				temp(x, y, ch) = src(x_s, y_s, ch);
			}
		}
	}

	temp.MoveTo(src);
}

template<typename T>
void FastRotation::Rotate180(Image<T>& src) {

	int r = src.Rows() - 1;
	int c = src.Cols() - 1;

	for (int ch = 0; ch < src.Channels(); ++ch) {
		for (int y = 0; y < src.Rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0, x_s = c; x < src.Cols(); ++x, --x_s) {
				std::swap(src(x, y, ch), src(x_s, y_s, ch));
			}
		}
	}
}

template<typename T>
void FastRotation::HorizontalMirror(Image<T>& src) {

	int c = src.Cols() - 1;
	int hc = src.Cols() / 2;

	for (int ch = 0; ch < src.Channels(); ++ch) {
		for (int y = 0; y < src.Rows(); ++y) {
			for (int x = 0, x_s = c; x < hc; ++x, --x_s) {
				std::swap(src(x, y, ch), src(x_s, y, ch));
			}
		}
	}
}

template<typename T>
void FastRotation::VerticalMirror(Image<T>& src) {

	int r = src.Rows() - 1;

	for (int ch = 0; ch < src.Channels(); ++ch) {
		for (int y = 0; y < src.Rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0; x < src.Cols(); ++x) {
				std::swap(src(x, y), src(x, y_s));
			}
		}
	}
}

template<typename T>
void FastRotation::Apply(Image<T>& src) {
	using enum FastRotation::Type;

	switch (m_frt) {
	case rotate90CW:
		return Rotate90CW(src);

	case rotate90CCW:
		return Rotate90CCW(src);

	case rotate180:
		return Rotate180(src);

	case horizontalmirror:
		return HorizontalMirror(src);

	case verticalmirror:
		return VerticalMirror(src);

	default:
		return;
	}
}
template void FastRotation::Apply(Image8& src);
template void FastRotation::Apply(Image16& src);
template void FastRotation::Apply(Image32& src);






template<typename T>
void IntegerResample::Downsample_average(Image<T>& src) {

	Image<T> temp(src.Rows() / m_factor, src.Cols() / m_factor, src.Channels());
	int factor2 = m_factor * m_factor;

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < temp.Rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.Cols(); ++x) {
				int x_s = m_factor * x;

				float pix = 0;
				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						pix += src(x_s + i, y_s + j, ch);

				temp(x, y, ch) = pix / factor2;

			}
		}
	}

	temp.MoveTo(src);
}

template<typename T>
void IntegerResample::Downsample_median(Image<T>& src) {

	Image<T> temp(src.Rows() / m_factor, src.Cols() / m_factor, src.Channels());
	std::vector<float> kernel(m_factor * m_factor);
	auto mp = kernel.begin() + m_factor / 2;

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < temp.Rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.Cols(); ++x) {
				int x_s = m_factor * x;


				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						kernel[j * m_factor + i] = src(x_s + i, y_s + j, ch);

				std::nth_element(kernel.begin(), mp, kernel.end());
				temp(x, y, ch) = *mp;

			}
		}
	}

	temp.MoveTo(src);
}

template<typename T>
void IntegerResample::Downsample_max(Image<T>& src) {

	Image<T> temp(src.Rows() / m_factor, src.Cols() / m_factor, src.Channels());

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < temp.Rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.Cols(); ++x) {
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

	temp.MoveTo(src);
}

template<typename T>
void IntegerResample::Downsample_min(Image<T>& src) {

	Image<T> temp(src.Rows() / m_factor, src.Cols() / m_factor, src.Channels());

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < temp.Rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < temp.Cols(); ++x) {
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

	temp.MoveTo(src);
}

template<typename T>
void IntegerResample::Downsample(Image<T>& src) {
	using enum IntegerResample::Method;

	if (m_factor >= src.Rows() || m_factor >= src.Cols())
		return;

	switch (m_method) {
	case average:
		return Downsample_average(src);

	case median:
		return Downsample_median(src);

	case max:
		return Downsample_max(src);

	case min:
		return Downsample_min(src);

	default:
		return;
	}
}

template<typename T>
void IntegerResample::Upsample(Image<T>& src) {

	Image<T> temp(src.Rows() * m_factor, src.Cols() * m_factor, src.Channels());

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < src.Rows(); ++y) {
			int y_s = m_factor * y;

			for (int x = 0; x < src.Cols(); ++x) {
				int x_s = m_factor * x;

				for (int j = 0; j < m_factor; ++j)
					for (int i = 0; i < m_factor; ++i)
						temp(x_s + i, y_s + j, ch) = src(x, y, ch);

			}
		}
	}

	temp.MoveTo(src);
}

template<typename T>
void IntegerResample::Apply(Image<T>& src) {
	using enum IntegerResample::Type;

	if (m_factor > 100) return;

	switch (m_type) {
	case downsample:
		return Downsample(src);

	case upsample:
		return Upsample(src);

	default:
		return;
	}

}
template void IntegerResample::Apply(Image8&);
template void IntegerResample::Apply(Image16&);
template void IntegerResample::Apply(Image32&);







template<typename T>
void Resize::Apply(Image<T>& src) {

	Image<T> temp(m_new_rows, m_new_cols, src.Channels());

	double ry = double(src.Rows()) / temp.Rows();
	double rx = double(src.Cols()) / temp.Cols();

	for (int ch = 0; ch < src.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.Cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = Interpolator().InterpolatePixel(src, x_s, y_s, ch, m_type);

			}
		}
	}
	
	temp.MoveTo(src);
}
template void Resize::Apply(Image8&);
template void Resize::Apply(Image16&);
template void Resize::Apply(Image32&);





template<typename T>
void Crop::Apply(Image<T>& src) {

	Image<T> temp(m_y2 - m_y1, m_x2 - m_x1, src.Channels());

	for (int ch = 0; ch < src.Channels(); ++ch)
		for (int y = 0; y < temp.Rows(); ++y)
			for (int x = 0; x < temp.Cols(); ++x)
				temp(x, y, ch) = src(x + m_x1, y + m_y1, ch);

	src = std::move(temp);
}
template void Crop::Apply(Image8&);
template void Crop::Apply(Image16&);
template void Crop::Apply(Image32&);







template<typename T>
void HomographyTransformation::Apply(Image<T>& src) {

	Image<T> temp(src);

	for (int ch = 0; ch < src.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < src.Rows(); ++y) {

			double yx = y * m_homography(0, 1);
			double yy = y * m_homography(1, 1);
			double yz = y * m_homography(2, 1);

			for (int x = 0; x < src.Cols(); ++x) {
				double x_s = x * m_homography(0, 0) + yx + m_homography(0, 2);
				double y_s = x * m_homography(1, 0) + yy + m_homography(1, 2);
				double zed = x * m_homography(2, 0) + yz + m_homography(2, 2);

				temp(x, y, ch) = Interpolator().InterpolatePixel(src, x_s / zed, y_s / zed, ch, m_type);

			}
		}
	}

	temp.MoveTo(src);
}
template void HomographyTransformation::Apply(Image8&);
template void HomographyTransformation::Apply(Image16&);
template void HomographyTransformation::Apply(Image32&);


