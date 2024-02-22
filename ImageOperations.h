#pragma once
#include "Image.h"
#include "Interpolator.h"

namespace ImageOP {

	void AlignFrame(Image32& img, Matrix& homography, Interpolate interp_type);

	void AlignedStats(Image32& img, Matrix& homography, Interpolate interp_type);

	void AlignImageStack(ImageVector& img_stack, Interpolate interp_type);


	template<typename Image>
	extern void RotateImage(Image& img, float theta_degrees, Interpolate interp_type);

	template<typename Image>
	extern void FastRotation(Image& img, FastRotate type);

	void Crop(Image32& img, int top, int bottom, int left, int right);

	template<typename Image>
	extern void ImageResize_Bicubic(Image& img, int new_rows, int new_cols);

	template<typename Image>
	extern void BinImage(Image& img, int factor, int method);

	template<typename Image>
	extern void UpsampleImage(Image& img, int factor);

	class Morphology {
		int m_kernel_dim = 3;
		int m_kernel_radius = (m_kernel_dim - 1) / 2;
		std::vector<bool> m_kmask = std::vector<bool>(m_kernel_dim * m_kernel_dim, true);

		float m_selection = 0.5;

		template <typename T>
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

			T& operator()(int x, int y) {
				return data[x * m_dim + y];
			}

			int Size() const { return m_size; }

			void Populate(Image<T>& img, int y, int ch) {

				for (int i = -m_radius, el = 0; i <= m_radius; ++i) {

					for (int j = -m_radius; j <= m_radius; ++j) {

						data[el++] = img.MirrorEdgePixel(i, y + j, ch);
					}
				}
			}

			void Update(Image<T>& img, int x, int y, int ch) {

				int xx = x + m_radius;
				if (xx >= img.Cols())
					xx = 2 * img.Cols() - (xx + 1);

				int n = m_size - m_dim;

				for (int el = 0; el < n; ++el)
					data[el] = data[el + m_dim];

				for (int j = -m_radius, el = n; j <= m_radius; ++j, ++el) {

					int yy = y + j;
					if (yy < 0)
						yy = -yy;
					else if (yy >= img.Rows())
						yy = 2 * img.Rows() - (yy + 1);
					data[el] = img(xx, yy, ch);

				}

			}

		};

	public:
		Morphology() = default;

		Morphology(int kernel_dimension, bool circular = false) : m_kernel_dim(kernel_dimension) {
			if (m_kernel_dim % 2 == 0)
				m_kernel_dim -= 1;

			m_kernel_radius = (m_kernel_dim - 1) / 2;

			if (circular) {
				m_kmask = std::vector<bool>(kernel_dimension * kernel_dimension, false);

				for (int j = 0; j < m_kernel_dim; ++j) {
					for (int i = 0; i < m_kernel_dim; ++i) {
						int dx = i - m_kernel_radius;
						int dy = j - m_kernel_radius;
						if (sqrt(dx * dx + dy * dy) <= m_kernel_radius + 0.5)
							m_kmask[j * m_kernel_dim + i] = true;
					}
				}

			}

			else
				m_kmask.resize(kernel_dimension * kernel_dimension, true);
		}

	private:
		std::vector<int> GetMaskedLocations() {
			int count = 0; //number of true values
			for (auto v : m_kmask)
				if (v) count++;

			std::vector<int> locations(count);
			for (int j = 0, el = 0; j < m_kernel_dim; ++j)
				for (int i = 0; i < m_kernel_dim; ++i)
					if (m_kmask[j * m_kernel_dim + i])
						locations[el++] = j * m_kernel_dim + i; //location of true values in the kernel

			return locations;
		}

		template <typename T>
		void FastMedian3x3(Image<T>& img);

		template <typename T>
		void FastMedian5x5(Image<T>& img);

		template <typename T>
		void FastMedian7x7(Image<T>& img);

		template <typename T>
		void FastMedian9x9(Image<T>& img);

		template <typename T>
		void FastMedian(Image<T>& img, int kernel_dim);

	public:
		template <typename T>
		void Erosion(Image<T>& img);

		template <typename T>
		void Dialation(Image<T>& img);

		template <typename T>
		void Opening(Image<T>& img);

		template <typename T>
		void Closing(Image<T>& img);

		template <typename T>
		void Selection(Image<T>& img);

		template <typename T>
		void Median(Image<T>& img);

		template <typename T>
		void Midpoint(Image<T>& img);
	};

	template<typename Image>
	extern void SobelEdge(Image& img);

	template<typename T>
	extern void BilateralFilter(Image<T>& img, float std_dev, float std_dev_range);

	template<typename Image>
	extern void PowerofInvertedPixels(Image& img, float order, bool lightness_mask = true);

	template<typename Image>
	extern void AdaptiveStretch(Image& img, float thresh_coef = 1.0f, int thresh_exp = -3, float contrast_coef = 0.0f, int contrast_exp = -2, int num_data_points = 1'000'000);
}