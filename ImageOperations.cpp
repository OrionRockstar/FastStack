#include "ImageOperations.h"

void ImageOP::AlignFrame(Image32& img, Matrix& homography, Interpolation_Type interp_type) {

	Image32 temp(img.Rows(), img.Cols());
	int ch = 0;

	temp.homography = img.homography;

#pragma omp parallel for
	for (int y = 0; y < img.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < img.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2);
			double y_s = x * homography(1, 0) + yy + homography(1, 2);

			temp(x, y) = Interpolator<Image32>::InterpolatePixel(img, x_s, y_s, ch, interp_type);

		}
	}

	temp.ComputeStatistics(true);
	img = std::move(temp);
}

void ImageOP::AlignedStats(Image32& img, Matrix& homography, Interpolation_Type interp_type) {

	Image32 temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);

			for (int x = 0; x < img.Cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				temp(x, y, ch) = Interpolator<Image32>::InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}
	temp.ComputeStatistics(true);
	img.MoveStatsFrom(temp);
	//img.CopyStatsFrom(temp);

}

void ImageOP::AlignImageStack(ImageVector& img_stack, Interpolation_Type interp_type) {

	for (auto im = img_stack.begin(); im != img_stack.end(); im++) {
		if (im == img_stack.begin())
			im->ComputeStatistics();
		else
			ImageOP::AlignFrame(*im, im->homography, interp_type);
	}
}

template<typename Image>
void ImageOP::RotateImage(Image& img, float theta, Interpolation_Type interp_type) {

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

				temp(x, y, ch) = Interpolator<Image>::InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}

	img = std::move(temp);

}
template void ImageOP::RotateImage(Image8&, float, Interpolation_Type);
template void ImageOP::RotateImage(Image16&, float, Interpolation_Type);
template void ImageOP::RotateImage(Image32&, float, Interpolation_Type);


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

	Image temp(new_rows, new_cols, img.Channels());

	double ry = double(img.Rows()) / temp.Rows();
	double rx = double(img.Cols()) / temp.Cols();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.Cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = Interpolator<Image>::InterpolatePixel(img, x_s, y_s, ch, Interpolation_Type::Bicubic_Spline);

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
void ImageOP::MedianBlur3x3(Image<T>& img) {
	Image<T> imgbuf(img.Rows(), img.Cols());
	std::array<T, 9>kernel{ 0 };

#pragma omp parallel for firstprivate(kernel)
	for (int y = 1; y < img.Rows() - 1; ++y) {
		for (int x = 1; x < img.Cols() - 1; ++x) {

			kernel = { img(x - 1, y - 1), img(x, y - 1), img(x + 1, y - 1),
					   img(x - 1, y), img(x, y), img(x + 1, y),
					   img(x - 1, y + 1), img(x, y + 1), img(x + 1, y + 1) };

			for (int r = 0; r < 3; ++r) {
				for (int i = 0, j = 5; i < 4; ++i, ++j) {
					if (kernel[i] > kernel[4])
						std::swap(kernel[i], kernel[4]);
					if (kernel[j] < kernel[4])
						std::swap(kernel[j], kernel[4]);
				}
			}

			imgbuf(x, y) = kernel[4];//kernelmedian(kernel);
		}
	}
	img.data = std::move(imgbuf.data);
}
template void ImageOP::MedianBlur3x3(Image8&);
template void ImageOP::MedianBlur3x3(Image16&);
template void ImageOP::MedianBlur3x3(Image32&);

template<typename T>
void ImageOP::MedianBlur5x5(Image<T>& img) {
	Image<T> imgbuf(img.Rows(), img.Cols());

	std::array<T, 25>kernel{ 0 };

#pragma omp parallel for private(kernel)
	for (int y = 2; y < img.Rows() - 2; ++y) {
		for (int x = 2; x < img.Cols() - 2; ++x) {

			kernel = { img(x - 2, y - 2), img(x - 1, y - 2), img(x, y - 2), img(x + 1, y - 2), img(x + 2, y - 2),
				img(x - 2, y - 1), img(x - 1, y - 1), img(x, y - 1), img(x + 1, y - 1), img(x + 2, y - 1),
				img(x - 2, y), img(x - 1, y), img(x, y), img(x + 1, y), img(x + 2, y),
				img(x - 2, y + 1), img(x - 1, y + 1), img(x, y + 1), img(x + 1, y + 1), img(x + 2, y + 1),
				img(x - 2, y + 2), img(x - 1, y + 2), img(x, y + 2), img(x + 1, y + 2), img(x + 2, y + 2) };

			for (int r = 0; r < 7; ++r)
				for (int i = 0, j = 13; i < 12; ++i, ++j) {
					if (kernel[i] > kernel[12])
						std::swap(kernel[i], kernel[12]);
					if (kernel[j] < kernel[12])
						std::swap(kernel[j], kernel[12]);
				}

			imgbuf(x, y) = kernel[12];
		}
	}
	img.data = std::move(imgbuf.data);
}
template void ImageOP::MedianBlur5x5(Image8&);
template void ImageOP::MedianBlur5x5(Image16&);
template void ImageOP::MedianBlur5x5(Image32&);


template<typename Image>
void ImageOP::GaussianBlur(Image& img, float std_dev) {

	struct PixelWindow {
		std::vector<float> data;
		int m_size = 0;
		int m_radius = 0;

		PixelWindow(int size) {
			data.resize(size);
			m_size = size;
			m_radius = (data.size() - 1) / 2;
		}
		~PixelWindow() {};

		float& operator[](int el) {
			return data[el];
		}

		int Size()const { return m_size; }
		int Radius()const { return m_radius; }

		void PopulateRowWindow(Image& img, int y, int ch) {

			for (int j = 0; j < Size(); ++j) {
				int xx = j - Radius();
				if (xx < 0)
					xx = -xx;

				data[j] = img(xx, y, ch);
			}
		}

		void UpdateRowWindow(Image& img, int x, int y, int ch) {

			data.erase(data.begin());

			int xx = x + Radius();
			if (xx >= img.Cols())
				xx = 2 * img.Cols() - (xx + 1);

			data.emplace_back(img(xx, y, ch));

		}

		void PopulateColWindow(Image& img, int x, int ch) {

			for (int j = 0; j < Size(); ++j) {
				int yy = j - Radius();
				if (yy < 0)
					yy = -yy;

				data[j] = img(x, yy, ch);
			}
		}

		void UpdateColWindow(Image& img, int x, int y, int ch) {

			data.erase(data.begin());

			int yy = y + Radius();
			if (yy >= img.Rows())
				yy = 2 * img.Rows() - (yy + 1);

			data.emplace_back(img(x, yy, ch));

		}

	};

	int kernel_radius = 3 * ceil(std_dev);

	int kernel_dim = kernel_radius + kernel_radius + 1;
	Image temp(img.Rows(), img.Cols(), img.Channels());

	std::vector<float> gaussian_kernel(kernel_dim);


	float k1 = 1 / sqrtf(2 * M_PI * std_dev * std_dev), k2 = 1 / (2 * std_dev * std_dev);
	float g_sum = 0;

	for (int j = -kernel_radius; j <= kernel_radius; ++j)
		g_sum += gaussian_kernel[j + kernel_radius] = k1 * expf(-k2 * (j * j));

	for (auto& val : gaussian_kernel)
		val /= g_sum;

	for (int ch = 0; ch < img.Channels(); ++ch) {

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			PixelWindow window(kernel_dim);
			window.PopulateRowWindow(img, y, ch);
			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					window.UpdateRowWindow(img, x, y, ch);

				float sum = 0.0f;
				for (int j = 0; j < kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				temp(x, y, ch) = sum;

			}
		}

#pragma omp parallel for 
		for (int x = 0; x < img.Cols(); ++x) {
			PixelWindow window(kernel_dim);
			window.PopulateColWindow(temp, x, ch);
			for (int y = 0; y < img.Rows(); ++y) {

				if (y != 0)
					window.UpdateColWindow(temp, x, y, ch);

				float sum = 0;
				for (int j = 0; j < kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				img(x, y, ch) = sum;
			}
		}

	}
}
template void ImageOP::GaussianBlur(Image8&, float);
template void ImageOP::GaussianBlur(Image16&, float);
template void ImageOP::GaussianBlur(Image32&, float);


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



void ImageOP::ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator type) {

	float rse, cse;
	for (int ch = 0; ch < ref.Channels(); ++ch) {
		switch (type) {
		case ScaleEstimator::median:
			rse = ref.Median(ch);
			cse = tgt.Median(ch);
			break;
		case ScaleEstimator::avgdev:
			rse = ref.AvgDev(ch);
			cse = ref.AvgDev(ch);
			break;
		case ScaleEstimator::mad:
			rse = ref.MAD(ch);
			cse = tgt.MAD(ch);
			break;
		case ScaleEstimator::bwmv:
			rse = ref.BWMV(ch);
			cse = tgt.BWMV(ch);
			break;
		case ScaleEstimator::none:
			return;
		}

		float k = rse / cse;
		for (auto pixel = tgt.begin(ch); pixel != tgt.end(ch); ++pixel)
			*pixel = tgt.ClipPixel(k * (*pixel - tgt.Median(ch)) + ref.Median(ch));
	}

}

void ImageOP::ScaleImageStack(ImageVector& img_stack, ScaleEstimator type) {

	for (auto iter = img_stack.begin() + 1; iter != img_stack.end(); ++iter)
		ImageOP::ScaleImage(*img_stack.begin(), *iter, type);

}


void ImageOP::ASinhStretch(Image32& img, float stretch_factor) {

	std::vector<int> hist(65536);

	for (const auto& pixel : img)
		hist[pixel * 65535]++;

	int sum = 0;
	int i = 0;

	while (sum < img.TotalImage() * .02) {
		sum += hist[i];
		++i;
	}

	float blackpoint = i / 65535.0f;
	float low = 0,
		high = 10000,
		mid;

	for (int i = 0; i < 20; i++)
	{
		mid = (low + high) / 2;
		float multiplier_mid = mid / asinh(mid);
		(stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}

	float beta = mid;
	float asinhb = asinh(beta);

	if (img.Channels() == 1) {
		img.ComputeMax();
		float max = img.Max();
		for (auto& pixel : img) {
			float r = (pixel - blackpoint) / (max - blackpoint);
			//float k = (r != 0) ? asinh(beta * r) / (r * asinhb) : 0;
			//pixel = ClipPixel(r * k);

			pixel = img.ClipPixel((r != 0) ? asinh(r * beta) / asinhb : 0);
		}
	}

	bool srbg = false;
	if (img.Channels() == 3) {

		float max = img.Max();
		std::array<float, 3> color = { 0.333333f, 0.333333f, 0.333333f };
		if (srbg) color = { 0.222491f, 0.716888f, 0.060621f };

		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {
				float I = ((color[0] * img.RedPixel(x, y) + color[1] * img.GreenPixel(x, y) + color[2] * img.BluePixel(x, y)) - blackpoint) / (max - blackpoint);
				float k = (I != 0) ? asinh(beta * I) / (I * asinhb) : 0;

				img.RedPixel(x, y) = img.ClipPixel(((img.RedPixel(x, y) - blackpoint) / (max - blackpoint)) * k);
				img.GreenPixel(x, y) = img.ClipPixel(((img.GreenPixel(x, y) - blackpoint) / (max - blackpoint)) * k);
				img.BluePixel(x, y) = img.ClipPixel(((img.BluePixel(x, y) - blackpoint) / (max - blackpoint)) * k);
			}
	}

}

template<typename Image>
void ImageOP::HistogramTransformation(Image& img, float shadow, float midtone, float highlight) {
	assert(highlight > shadow && 0.0 < midtone && midtone < 1.0f);

	float m1 = midtone - 1, m2 = (2 * midtone) - 1;
	float s = 1.0f / (highlight - shadow);

	for (auto& pixel : img) {

		pixel = (pixel - shadow) * s;

		if (pixel <= 0.0f) pixel = 0.0f;

		else if (pixel > 1.0f) pixel = 1.0f;

		else if (pixel == midtone)  pixel = 0.5f;

		else
			pixel = (m1 * pixel) / ((m2 * pixel) - midtone);

	}
}
template void ImageOP::HistogramTransformation(Image8&, float, float, float);
template void ImageOP::HistogramTransformation(Image16&, float, float, float);
template void ImageOP::HistogramTransformation(Image32&, float, float, float);

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
