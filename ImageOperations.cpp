#include "ImageOperations.h"

void ImageOP::AlignFrame(Image32& img, Matrix& homography, Interpolate interp_type) {
	Interpolator<Image32> interpolator;

	Image32 temp(img.Rows(), img.Cols(), img.Channels());
	temp.homography = img.homography;

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);
			for (int x = 0; x < img.Cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, interp_type);//img.ClipPixel(interp_type(img, x_s, y_s, ch));

			}
		}
	}

	img = std::move(temp);
	img.ComputeStatistics(true);
}

void ImageOP::AlignedStats(Image32& img, Matrix& homography, Interpolate interp_type) {

	Interpolator<Image32> interpolator;

	Image32 temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);

			for (int x = 0; x < img.Cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}
	temp.ComputeStatistics(true);
	img.MoveStatsFrom(temp);
	
}

void ImageOP::AlignImageStack(ImageVector& img_stack, Interpolate interp_type) {

	for (auto im = img_stack.begin(); im != img_stack.end(); im++) {
		if (im == img_stack.begin())
			im->ComputeStatistics();
		else
			ImageOP::AlignFrame(*im, im->homography, interp_type);
	}
}

template<typename Image>
void ImageOP::RotateImage(Image& img, float theta, Interpolate interp_type) {

	Interpolator<Image> interpolator;

	theta *= (M_PI / 180);
	float s = sin(theta);
	float c = cos(theta);
	Image temp(fabs(img.Rows() * c) + fabs(img.Cols() * s), fabs(img.Cols() * c) + fabs(img.Rows() * s), img.Channels());

	float hc = temp.Cols() / 2;
	float hr = temp.Rows() / 2;

	float offsetx = hc - (temp.Cols() - img.Cols()) / 2;
	float offsety = hr - (temp.Rows() - img.Rows()) / 2;
	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {

			double yx = (y - hr) * s;
			double yy = (y - hr) * c;

			for (int x = 0; x < temp.Cols(); ++x) {

				double x_s = ((x - hc) * c - yx) + offsetx;
				double y_s = ((x - hc) * s + yy) + offsety;

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}

	img = std::move(temp);

}
template void ImageOP::RotateImage(Image8&, float, Interpolate);
template void ImageOP::RotateImage(Image16&, float, Interpolate);
template void ImageOP::RotateImage(Image32&, float, Interpolate);


template<typename Image>
class FR {
public:
	static void FastRotate_90CW(Image& img) {
		Image temp(img.Cols(), img.Rows());
		temp.MoveStatsFrom(img);

		int hc = temp.Cols() / 2;
		int hr = temp.Rows() / 2;

		int offsetx = hc - (temp.Cols() - img.Cols()) / 2 - hr;
		int offsety = hr - (temp.Rows() - img.Rows()) / 2 + hc - 1;

		for (int y = 0; y < temp.Rows(); ++y) {
			int x_s = y - offsetx;
			for (int x = 0; x < temp.Cols(); ++x) {
				int y_s = offsety - x;

				temp(x, y) = img(x_s, y_s);

			}
		}
		img = std::move(temp);
	}

	static void FastRotate_90CCW(Image& img) {
		Image temp(img.Cols(), img.Rows());
		temp.MoveStatsFrom(img);

		int hr = temp.Rows() / 2;
		int hc = temp.Cols() / 2;

		int offsetx = hc - (temp.Cols() - img.Cols()) / 2 + hr;
		int offsety = hr - (temp.Rows() - img.Rows()) / 2 - hc;

		for (int y = 0; y < temp.Rows(); ++y) {
			int x_s = offsetx - y;
			for (int x = 0; x < temp.Cols(); ++x) {
				int y_s = x + offsety;

				temp(x, y) = img(x_s, y_s);

			}
		}

		img = std::move(temp);
	}

	static void FastRotate_180(Image& img) {
		int r = img.Rows() - 1;
		int c = img.Cols() - 1;
		for (int y = 0; y < img.Rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0, x_s = c; x < img.Cols(), x_s >= 0; ++x, --x_s) {
				//int x_s = c - x;
				std::swap(img(x, y), img(x_s, y_s));
			}
		}
	}

	static void FastRotate_HorizontalMirror(Image& img) {
		int c = img.Cols() - 1;
		int hc = img.Cols() / 2;
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0, x_s = c; x < hc, x_s >= hc; ++x, --x_s) {
				//int x_s = c - x;
				std::swap(img(x, y), img(x_s, y));
			}
		}
	}

	static void FastRotate_VerticalMirror(Image& img) {
		int r = img.Rows() - 1;
		for (int y = 0; y < img.Rows() / 2; ++y) {
			int y_s = r - y;
			for (int x = 0; x < img.Cols(); ++x)
				std::swap(img(x, y), img(x, y_s));
		}
	}
};

template<typename Image>
void ImageOP::FastRotation(Image& img, FastRotate type) {
	switch (type) {
	case FastRotate::rotate90CW:
		FR<Image>::FastRotate_90CW(img);
		break;
	case FastRotate::rotate90CCW:
		FR<Image>::FastRotate_90CCW(img);
		break;
	case FastRotate::rotate180:
		FR<Image>::FastRotate_180(img);
		break;
	case FastRotate::horizontalmirror:
		FR<Image>::FastRotate_HorizontalMirror(img);
		break;
	case FastRotate::verticalmirror:
		FR<Image>::FastRotate_VerticalMirror(img);
		break;
		//default:
			//break;
	}
}
template void ImageOP::FastRotation(Image8&, FastRotate);
template void ImageOP::FastRotation(Image16&, FastRotate);
template void ImageOP::FastRotation(Image32&, FastRotate);


void ImageOP::Crop(Image32& img, int top, int bottom, int left, int right) {

	Image32 temp(img.Rows() - (top + bottom), img.Cols() - (left + right), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch)
		for (int y = 0; y < temp.Rows(); ++y)
			for (int x = 0; x < temp.Cols(); ++x)
				temp(x, y, ch) = img(x + left, y + top, ch);

	img = std::move(temp);
}

template<typename Image>
void ImageOP::ImageResize_Bicubic(Image& img, int new_rows, int new_cols) {

	Interpolator<Image> interpolator;

	Image temp(new_rows, new_cols, img.Channels());

	double ry = double(img.Rows()) / temp.Rows();
	double rx = double(img.Cols()) / temp.Cols();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.Cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, Interpolate::bicubic_spline);

			}
		}
	}
	img = std::move(temp);
}
template void ImageOP::ImageResize_Bicubic(Image8&, int, int);
template void ImageOP::ImageResize_Bicubic(Image16&, int, int);
template void ImageOP::ImageResize_Bicubic(Image32&, int, int);

template<typename Image>
class Bin {
public:
	static void BinIntegerAverage(Image& img, int factor) {
		if (factor > 250) return;

		Image temp(img.Rows() / factor, img.Cols() / factor, img.Channels());
		int factor2 = factor * factor;

		for (int ch = 0; ch < temp.Channels(); ++ch) {

			for (int y = 0; y < temp.Rows(); ++y) {
				int y_s = factor * y;

				for (int x = 0; x < temp.Cols(); ++x) {
					int x_s = factor * x;

					float pix = 0;
					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							pix += img(x_s + i, y_s + j, ch);

					temp(x, y, ch) = pix / factor2;

				}
			}
		}

		img = std::move(temp);
		//img.ComputeStats();
	}

	static void BinIntegerMedian(Image& img, int factor) {
		if (factor > 250) return;

		Image temp(img.Rows() / factor, img.Cols() / factor, img.Channels());
		std::vector<float> kernel(factor * factor);
		auto mp = kernel.begin() + factor / 2;

		for (int ch = 0; ch < temp.Channels(); ++ch) {

			for (int y = 0; y < temp.Rows(); ++y) {
				int y_s = factor * y;

				for (int x = 0; x < temp.Cols(); ++x) {
					int x_s = factor * x;


					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)
							kernel[j * factor + i] = img(x_s + i, y_s + j, ch);

					std::nth_element(kernel.begin(), mp, kernel.end());
					temp(x, y, ch) = *mp;

				}
			}
		}

		img = std::move(temp);
		//img.ComputeStats();
	}

	static void BinIntegerMax(Image& img, int factor) {
		if (factor > 250) return;

		Image temp(img.Rows() / factor, img.Cols() / factor, img.Channels());

		for (int ch = 0; ch < temp.Channels(); ++ch) {

			for (int y = 0; y < temp.Rows(); ++y) {
				int y_s = factor * y;

				for (int x = 0; x < temp.Cols(); ++x) {
					int x_s = factor * x;

					float max = 0;
					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)

							max = (img(x_s + i, y_s + j, ch) > max) ? img(x_s + i, y_s + j, ch) : max;

					temp(x, y, ch) = max;

				}
			}
		}

		img = std::move(temp);
		//img.ComputeStats();
	}

	static void BinIntegerMin(Image& img, int factor) {
		if (factor > 250) return;

		Image temp(img.Rows() / factor, img.Cols() / factor, img.Channels());

		for (int ch = 0; ch < temp.Channels(); ++ch) {

			for (int y = 0; y < temp.Rows(); ++y) {
				int y_s = factor * y;

				for (int x = 0; x < temp.Cols(); ++x) {
					int x_s = factor * x;

					float min = 1;
					for (int j = 0; j < factor; ++j)
						for (int i = 0; i < factor; ++i)

							min = (img(x_s + i, y_s + j, ch) < min) ? img(x_s + i, y_s + j, ch) : min;

					temp(x, y, ch) = min;

				}
			}
		}

		img = std::move(temp);
		//img.ComputeStats();
	}
};

template<typename Image>
void ImageOP::BinImage(Image& img, int factor, int method) {
	switch (method) {
	case 0:
		Bin<Image>::BinIntegerAverage(img, factor);
		return;
	case 1:
		Bin<Image>::BinIntegerMedian(img, factor);
		return;
	case 2:
		Bin<Image>::BinIntegerMax(img, factor);
		return;
	case 3:
		Bin<Image>::BinIntegerMin(img, factor);
		return;
	}
}
template void ImageOP::BinImage(Image8&, int, int);
template void ImageOP::BinImage(Image16&, int, int);
template void ImageOP::BinImage(Image32&, int, int);

template<typename Image>
void ImageOP::UpsampleImage(Image& img, int factor) {

	if (factor > 4) return;

	Image temp(img.Rows() * factor, img.Cols() * factor, img.Channels());

	for (int ch = 0; ch < temp.Channels(); ++ch) {

		for (int y = 0; y < img.Rows() - factor; ++y) {
			int y_s = factor * y;

			for (int x = 0; x < img.Cols() - factor; ++x) {
				int x_s = factor * x;

				for (int j = 0; j < factor; ++j)
					for (int i = 0; i < factor; ++i)
						temp(x_s + i, y_s + j, ch) = img(x, y, ch);

			}
		}
	}

	img = std::move(temp);
}
template void ImageOP::UpsampleImage(Image8&, int);
template void ImageOP::UpsampleImage(Image16&, int);
template void ImageOP::UpsampleImage(Image32&, int);


template<typename T>
void ImageOP::Morphology::Erosion(Image<T>& img) {

	int k_r = m_kernel_radius;
	Image<T> temp(img);

	std::vector<int> locations = GetMaskedLocations();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			Kernel2D<T> kernel(m_kernel_radius);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				T min = std::numeric_limits<T>::max();
				for (int el = 0; el < locations.size(); ++el)
					if (kernel[locations[el]] < min)
						min = kernel[locations[el]];

				temp(x, y, ch) = min;
			}
		}
	}

	temp.MoveTo(img);
}
template void ImageOP::Morphology::Erosion(Image8&);
template void ImageOP::Morphology::Erosion(Image16&);
template void ImageOP::Morphology::Erosion(Image32&);

template<typename T>
void ImageOP::Morphology::Dialation(Image<T>& img) {

	int k_r = m_kernel_radius;
	Image<T> temp(img);

	std::vector<int> locations = GetMaskedLocations();


	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			Kernel2D<T> kernel(m_kernel_radius);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				T max = std::numeric_limits<T>::min();
				for (int el = 0; el < locations.size(); ++el)
					if (kernel[locations[el]] > max)
						max = kernel[locations[el]];

				temp(x, y, ch) = max;
			}
		}
	}

	temp.MoveTo(img);
}
template void ImageOP::Morphology::Dialation(Image8&);
template void ImageOP::Morphology::Dialation(Image16&);
template void ImageOP::Morphology::Dialation(Image32&);

template <typename T>
void ImageOP::Morphology::Opening(Image<T>& img) {

	Dialation(img);

	Erosion(img);

}
template void ImageOP::Morphology::Opening(Image8&);
template void ImageOP::Morphology::Opening(Image16&);
template void ImageOP::Morphology::Opening(Image32&);

template <typename T>
void ImageOP::Morphology::Closing(Image<T>& img) {

	Erosion(img);

	Dialation(img);

}
template void ImageOP::Morphology::Closing(Image8&);
template void ImageOP::Morphology::Closing(Image16&);
template void ImageOP::Morphology::Closing(Image32&);

template <typename T>
void ImageOP::Morphology::Selection(Image<T>& img) {

	int k_r = m_kernel_radius;
	Image<T> temp(img);

	std::vector<int> locations = GetMaskedLocations();
	int pivot = (m_selection == 1.0) ? locations.size() - 1 : locations.size() * m_selection;

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			Kernel2D<T> kernel(m_kernel_radius);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				std::vector<T> k(locations.size());

				if (locations.size() == kernel.Size())
					memcpy(&k[0], &kernel[0], sizeof(T) * kernel.Size());

				else
					for (int el = 0; el < k.size(); ++el)
						k[el] = kernel[locations[el]];

				std::nth_element(&k[0], &k[pivot], &k[locations.size()]);
				temp(x, y, ch) = k[pivot];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::Selection(Image8&);
template void ImageOP::Morphology::Selection(Image16&);
template void ImageOP::Morphology::Selection(Image32&);

template <typename T>
void ImageOP::Morphology::Median(Image<T>& img) {

	std::vector<int> locations = GetMaskedLocations();
	int half = locations.size() / 2;

	if (locations.size() == m_kmask.size() && m_kernel_dim <= 9)
		return FastMedian(img, m_kernel_dim);

	Image<T> temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			Kernel2D<T> kernel(m_kernel_radius);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				std::vector<T> k(locations.size());

				if (locations.size() == kernel.Size())
					memcpy(&k[0], &kernel[0], sizeof(T) * kernel.Size());

				else
					for (int el = 0; el < k.size(); ++el)
						k[el] = kernel[locations[el]];

				std::nth_element(&k[0], &k[half], &k[locations.size()]);
				temp(x, y, ch) = k[half];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::Median(Image8&);
template void ImageOP::Morphology::Median(Image16&);
template void ImageOP::Morphology::Median(Image32&);

template<typename T>
void ImageOP::Morphology::Midpoint(Image<T>& img) {

	int k_r = m_kernel_radius;
	Image<T> temp(img);

	std::vector<int> locations = GetMaskedLocations();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			Kernel2D<T> kernel(m_kernel_radius);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				T min = std::numeric_limits<T>::max();
				T max = std::numeric_limits<T>::min();
				for (int el = 0; el < locations.size(); ++el) {
					if (kernel[locations[el]] < min)
						min = kernel[locations[el]];

					if (kernel[locations[el]] > max)
						max = kernel[locations[el]];
				}

				temp(x, y, ch) = (0.5 * min + 0.5 * max);
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::Midpoint(Image8&);
template void ImageOP::Morphology::Midpoint(Image16&);
template void ImageOP::Morphology::Midpoint(Image32&);

template <typename T>
void ImageOP::Morphology::FastMedian3x3(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				std::array<T, 9>kernel{ 0 };

				kernel[0] = img.MirrorEdgePixel(x - 1, y - 1, ch);
				kernel[1] = img.MirrorEdgePixel(x, y - 1, ch);
				kernel[2] = img.MirrorEdgePixel(x + 1, y - 1, ch);

				kernel[3] = img.MirrorEdgePixel(x - 1, y, ch);
				kernel[4] = img(x, y, ch);
				kernel[5] = img.MirrorEdgePixel(x + 1, y, ch);

				kernel[6] = img.MirrorEdgePixel(x - 1, y + 1, ch);
				kernel[7] = img.MirrorEdgePixel(x, y + 1, ch);
				kernel[8] = img.MirrorEdgePixel(x + 1, y + 1, ch);

				for (int r = 0; r < 3; ++r) {
					for (int i = 0, j = 5; i < 4; ++i, ++j) {
						if (kernel[i] > kernel[4])
							std::swap(kernel[i], kernel[4]);
						if (kernel[j] < kernel[4])
							std::swap(kernel[j], kernel[4]);
					}
				}

				temp(x, y, ch) = kernel[4];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::FastMedian3x3(Image8&);
template void ImageOP::Morphology::FastMedian3x3(Image16&);
template void ImageOP::Morphology::FastMedian3x3(Image32&);

template <typename T>
void ImageOP::Morphology::FastMedian5x5(Image<T>& img) {
	Image<T> temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				std::array<T, 25>kernel{ 0 };

				for (int j = -2, el = 0; j <= 2; ++j)
					for (int i = -2; i <= 2; ++i)
						kernel[el++] = img.MirrorEdgePixel(x + i, y + j, ch);

				for (int r = 0; r < 5; ++r)
					for (int i = 0, j = 13; i < 12; ++i, ++j) {
						if (kernel[i] > kernel[12])
							std::swap(kernel[i], kernel[12]);
						if (kernel[j] < kernel[12])
							std::swap(kernel[j], kernel[12]);
					}

				temp(x, y, ch) = kernel[12];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::FastMedian5x5(Image8& img);
template void ImageOP::Morphology::FastMedian5x5(Image8& img);
template void ImageOP::Morphology::FastMedian5x5(Image8& img);

template <typename T>
void ImageOP::Morphology::FastMedian7x7(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				std::array<float, 49> kernel;

				for (int j = -3, el = 0; j <= 3; ++j)
					for (int i = -3; i <= 3; ++i)
						kernel[el++] = img.MirrorEdgePixel(x + i, y + j, ch);

				for (int r = 0; r < 7; ++r) {
					for (int i = 0, j = 25; i < 24; ++i, ++j) {
						if (kernel[i] > kernel[24])
							std::swap(kernel[i], kernel[24]);
						if (kernel[j] < kernel[24])
							std::swap(kernel[j], kernel[24]);
					}
				}

				temp(x, y, ch) = kernel[24];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::FastMedian7x7(Image8& img);
template void ImageOP::Morphology::FastMedian7x7(Image16& img);
template void ImageOP::Morphology::FastMedian7x7(Image32& img);

template <typename T>
void ImageOP::Morphology::FastMedian9x9(Image<T>& img) {
	Image<T> temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for //private(kernel)
		for (int y = 0; y < img.Rows(); ++y) {
			Kernel2D<T> kernel(4);
			kernel.Populate(img, y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(img, x, y, ch);

				std::vector<T> k(kernel.m_size);
				memcpy(&k[0], &kernel[0], k.size() * sizeof(T));

				for (int r = 0; r < 9; ++r) {
					for (int i = 0, j = 41; i < 40; ++i, ++j) {
						if (k[i] > k[40])
							std::swap(k[i], k[40]);
						if (k[j] < k[40])
							std::swap(k[j], k[40]);
					}
				}

				temp(x, y, ch) = k[40];
			}
		}
	}
	temp.MoveTo(img);
}
template void ImageOP::Morphology::FastMedian9x9(Image8& img);
template void ImageOP::Morphology::FastMedian9x9(Image16& img);
template void ImageOP::Morphology::FastMedian9x9(Image32& img);

template <typename T>
void ImageOP::Morphology::FastMedian(Image<T>& img, int kernel_dim) {
	if (kernel_dim > 9 || kernel_dim % 2 == 0)
		return;
	switch (kernel_dim) {
	case 3:
		return FastMedian3x3(img);
	case 5:
		return FastMedian5x5(img);
	case 7:
		return FastMedian7x7(img);
	case 9:
		return FastMedian9x9(img);
	}
}
template void ImageOP::Morphology::FastMedian(Image8&, int);
template void ImageOP::Morphology::FastMedian(Image16&, int);
template void ImageOP::Morphology::FastMedian(Image32&, int);


template<typename Image>
void ImageOP::SobelEdge(Image& img) {

	std::array<int, 9> sx = { -1,0,1,-2,0,2,-1,0,1 };
	std::array<int, 9> sy = { -1,-2,-1,0,0,0,1,2,1 };

	Image temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				float sumx = 0, sumy = 0;

				for (int j = -1, el = 0; j < 2; ++j)
					for (int i = -1; i < 2; ++i) {
						if (img.IsInBounds(x + i, y + j)) {
							sumx += img(x + i, y + j, ch) * sx[el];
							sumy += img(x + i, y + j, ch) * sy[el];
						}

						else {
							sumx += img(x, y, ch) * sx[el];
							sumy += img(x, y, ch) * sy[el];
						}
						el++;
					}
				temp(x, y, ch) = sqrtf(sumx * sumx + sumy * sumy);
			}
		}
	}

	temp.CopyTo(img);
	img.Normalize();
}
template void ImageOP::SobelEdge(Image8&);
template void ImageOP::SobelEdge(Image16&);
template void ImageOP::SobelEdge(Image32&);

template<typename T>
void ImageOP::BilateralFilter(Image<T>& img, float std_dev, float std_dev_range) {

	struct Kernel2D {
		std::unique_ptr<T[]> data;
		int m_dim = 0;
		int m_radius = 0;
		int m_size = 0;

		Kernel2D(int radius) : m_radius(radius) {
			m_dim = 2 * m_radius + 1;
			m_size = m_dim * m_dim;
			data = std::make_unique<T[]>(m_size);
		}

		~Kernel2D() {};

		T& operator[](int el) {
			return data[el];
		}

		int Size() const { return m_size; }
		int Radius() const { return m_radius; }
		int Dimension() const { return m_dim; }

		void PopulateKernelCC(Image<T>& img, int y) {

			for (int i = -Radius(), el = 0; i <= Radius(); ++i) {

				int xx = i;
				if (xx < 0)
					xx = -xx;
				else if (xx >= img.Cols())
					xx = 2 * img.Cols() - (xx + 1);

				for (int j = -Radius(); j <= Radius(); ++j) {

					int yy = y + j;
					if (yy < 0)
						yy = -yy;
					else if (yy >= img.Rows())
						yy = 2 * img.Rows() - (yy + 1);

					data[el++] = img(xx, yy);
				}
			}
		}

		void UpdateKernelCC(Image<T>& img, int x, int y) {

			int xx = x + Radius();
			if (xx >= img.Cols())
				xx = 2 * img.Cols() - (xx + 1);

			int n = Size() - Dimension();

			for (int el = 0; el < n; ++el)
				data[el] = data[el + Dimension()];

			for (int j = -Radius(), el = n; j <= Radius(); ++j, ++el) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				else if (yy >= img.Rows())
					yy = 2 * img.Rows() - (yy + 1);
				data[el] = img(xx, yy);

			}

		}

	};


	int kernel_radius = 3 * std_dev;
	int kernel_dim = kernel_radius + kernel_radius + 1;

	Image<T> temp(img.Rows(), img.Cols(), img.Channels());

	float k1 = 1 / (2 * M_PI * std_dev * std_dev), k2 = 1 / (2 * std_dev * std_dev);
	float k1_r = 1 / sqrtf(2 * M_PI * std_dev_range * std_dev_range), k2_r = 1 / (2 * std_dev_range * std_dev_range);

	std::vector<float> gk2d(kernel_dim * kernel_dim);

	for (int j = -kernel_radius, el = 0; j <= kernel_radius; ++j)
		for (int i = -kernel_radius; i <= kernel_radius; ++i)
			gk2d[el++] = k1 * expf(-k2 * ((i * i) + (j * j)));


	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			Kernel2D kernel(kernel_radius);
			kernel.PopulateKernelCC(img, y);
			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.UpdateKernelCC(img, x, y);

				float sum = 0.0f;
				float weight = 0.0f;

				for (int el = 0; el < kernel.Size(); ++el) {
					float d = img(x, y) - kernel[el];
					float w = k1_r * exp(-k2_r * d * d) * gk2d[el];
					weight += w;
					sum += w * kernel[el];
				}

				temp(x, y, ch) = sum / weight;

			}
		}
	}
	temp.CopyTo(img);
}
template void ImageOP::BilateralFilter(Image8&, float, float);
template void ImageOP::BilateralFilter(Image16&, float, float);
template void ImageOP::BilateralFilter(Image32&, float, float);

template<typename Image>
void ImageOP::PowerofInvertedPixels(Image& img, float order, bool lightness_mask) {

#pragma omp parallel for
	for (int el = 0; el < img.Total(); ++el)
		img[el] = (lightness_mask) ? (double(img[el]) * img[el]) + (1 - img[el]) * pow(img[el], pow(1 - img[el], order)) : pow(img[el], pow(1 - img[el], order));
}
template void ImageOP::PowerofInvertedPixels(Image8&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image16&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image32&, float, bool);

static int Min(int a, int b) {
	return (a < b) ? a : b;
}

template<typename Image>
void ImageOP::AdaptiveStretch(Image& img, float thresh_coef, int thresh_exp, float contrast_coef, int contrast_exp, int num_data_points);
template void ImageOP::AdaptiveStretch(Image8&, float, int, float, int, int);
template void ImageOP::AdaptiveStretch(Image16&, float, int, float, int, int);
template void ImageOP::AdaptiveStretch(Image32&, float, int, float, int, int);

static void ClipHistogram(std::vector<uint32_t>& histogram, int limit) {

	for (int iter = 0; iter < 5; ++iter) {

		int clip_count = 0;

		for (auto& val : histogram)
			if (val > limit) {
				clip_count += val - limit;
				val = limit;
			}

		int d = clip_count / histogram.size(); // evenly distributes clipped values to histogram
		int r = clip_count % histogram.size(); // dristubues remainder of clipped values 


		if (d != 0)
			for (auto& v : histogram)
				v += d;

		if (r != 0) {
			int skip = histogram.size() / r;
			for (int el = 0; el < histogram.size(); el += skip)
				histogram[el]++;
		}

		if (r == 0 && d == 0)
			break;

	}
}

template<typename Image>
void ImageOP::LocalHistogramEqualization(Image& img, int kernel_radius, float contrast_limit, float amount, bool circular, int hist_res) {

	assert(kernel_radius >= 16 && contrast_limit >= 1.0 and 0.0f <= amount && amount <= 1.0f);

	struct KernelHist {

		std::unique_ptr<uint32_t[]> histogram;
		int kernel_radius = 0;
		int kernel_dimension = 2 * kernel_radius + 1;
		int m_size = 0;
		int m_multiplier = 0;
		int pix_count = 0;
		bool is_circle = false;

		std::vector<int> back_pix;
		std::vector<int> front_pix;
		std::vector<bool> k_mask;

		KernelHist(int resolution, int kernel_radius, bool circular) :kernel_radius(kernel_radius), is_circle(circular) {

			switch (resolution) {

			case 8:
				m_size = 256;
				m_multiplier = 255;
				break;

			case 10:
				m_size = 1024;
				m_multiplier = 1023;
				break;

			case 12:
				m_size = 4096;
				m_multiplier = 4095;
				break;

			default:
				m_size = 256;
				m_multiplier = 255;
			}

			histogram = std::make_unique<uint32_t[]>(m_size);

			if (is_circle) {
				k_mask.resize(kernel_dimension * kernel_dimension, false);
				back_pix.resize(kernel_dimension);
				front_pix.resize(kernel_dimension);

				for (int j = 0; j < kernel_dimension; ++j) {

					bool new_x = true;
					for (int i = 0; i < kernel_dimension; ++i) {


						int dx = i - kernel_radius;
						int dy = j - kernel_radius;
						int loc = j * kernel_dimension + i;

						if (sqrt(dx * dx + dy * dy) <= kernel_radius)
							k_mask[loc] = true;


						if (new_x && k_mask[loc]) {
							back_pix[j] = dx;
							front_pix[j] = dx;
							new_x = false;
						}

						else if (!new_x && k_mask[loc])
							front_pix[j] = dx;

					}
				}
			}

			else {
				k_mask.resize(kernel_dimension * kernel_dimension, true);
				back_pix.resize(kernel_dimension, -kernel_radius);
				front_pix.resize(kernel_dimension, kernel_radius);
			}
		}

		void PopulateKernelHistogram(Image& img, int y) {

			for (int j = -kernel_radius, j_m = 0; j <= kernel_radius; ++j, ++j_m) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				if (yy >= img.Rows())
					yy = 2 * img.Rows() - (yy + 1);

				for (int i = -kernel_radius, i_m = 0; i <= kernel_radius; ++i, ++i_m) {

					int xx = i;
					if (xx < 0)
						xx = -xx;

					if (k_mask[j_m * kernel_dimension + i_m]) {
						histogram[img(xx, yy) * m_multiplier]++;
						pix_count++;
					}
				}
			}
		}

		void UpdateKernelHistogram(Image& img, int x, int y) {

			for (int j = -kernel_radius; j <= kernel_radius; ++j) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				if (yy >= img.Rows())
					yy = 2 * img.Rows() - (yy + 1);


				int xx = x + front_pix[j + kernel_radius];

				if (xx >= img.Cols())
					xx = 2 * img.Cols() - (xx + 1);

				histogram[img(xx, yy) * m_multiplier]++;


				xx = x + back_pix[j + kernel_radius] - 1;

				if (xx < 0)
					xx = -xx;

				histogram[img(xx, yy) * m_multiplier]--;
			}

		}

	};

	float original_amount = 1.0 - amount;

	Image temp(img.Rows(), img.Cols());


#pragma omp parallel for
	for (int y = 0; y < img.Rows(); ++y) {

		KernelHist k_hist(hist_res, kernel_radius, circular);
		k_hist.PopulateKernelHistogram(img, y);
		int limit = (contrast_limit * k_hist.pix_count) / k_hist.m_multiplier;

		for (int x = 0; x < img.Cols(); ++x) {
			if (x != 0)
				k_hist.UpdateKernelHistogram(img, x, y);

			std::vector<uint32_t> k_hist_cl(k_hist.m_size);
			memcpy(k_hist_cl.data(), k_hist.histogram.get(), k_hist.m_size * 4);

			ClipHistogram(k_hist_cl, limit);

			int sum = 0;
			int val = img(x, y) * k_hist.m_multiplier;

			for (int el = 0; el <= val; ++el)
				sum += k_hist_cl[el];

			int cdf = sum;
			int min = 0;

			for (const auto& val : k_hist_cl)
				if (val != 0) { min = val; break; }

			for (int el = val + 1; el < k_hist_cl.size(); ++el)
				sum += k_hist_cl[el];

			temp(x, y) = (original_amount * img(x, y)) + (amount * float(cdf - min) / (sum - min));
		}
	}

	img.data = std::move(temp.data);
}
template void ImageOP::LocalHistogramEqualization(Image8&, int, float, float, bool, int);
template void ImageOP::LocalHistogramEqualization(Image16&, int, float, float, bool, int);
template void ImageOP::LocalHistogramEqualization(Image32&, int, float, float, bool, int);
