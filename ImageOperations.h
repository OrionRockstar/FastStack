#pragma once
#include "Image.h"
#include "Interpolator.h"

namespace ImageOP {

	void AlignFrame(Image32& img, Matrix& homography, Interpolation_Type interp_type);

	void AlignedStats(Image32& img, Matrix& homography, Interpolation_Type interp_type);

	void AlignImageStack(ImageVector& img_stack, Interpolation_Type interp_type);


	//void DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_estimator);

	template<typename Image>
	extern void RotateImage(Image& img, float theta_degrees, Interpolation_Type interp_type);

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

			int Size() const { return m_size; }
			int Radius() const { return m_radius; }
			int Dimension() const { return m_dim; }

			void PopulateKernelCC(Image<T>& img, int y) {

				for (int i = -Radius(), el = 0; i <= Radius(); ++i) {

					for (int j = -Radius(); j <= Radius(); ++j) {

						data[el++] = img.MirrorEdgePixel(i, y + j, 0);
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

	public:
		template <typename T>
		void Erosion(Image<T>& img);

		template <typename T>
		void Dialation(Image<T>& img);

		template <typename T>
		void Opening(Image<T>& img);

		template <typename T>
		void Closing(Image<T>& img);

		template<typename T>
		void Median(Image<T>& img);
	};

	template<typename Image>
	extern void SobelEdge(Image& img);

	template<typename T>
	extern void MedianBlur(Image<T>& img, int kernel_dim);

	template<typename T>
	extern void MedianBlur(Image<T>& img, int kernel_dim, Image<T>& output);

	template<typename T>
	extern void BilateralFilter(Image<T>& img, float std_dev, float std_dev_range);


	//void ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator type);

	//void ScaleImageStack(ImageVector& img_stack, ScaleEstimator type);


	void STFImageStretch(Image32& img);

	void ASinhStretch(Image32& img, float stretch_factor);

	template<typename Image>
	extern void HistogramTransformation(Image& img, float shadow = 0.0f, float midtone = 0.5f, float highlight = 1.0f);

	template<typename Image>
	extern void PowerofInvertedPixels(Image& img, float order, bool lightness_mask = true);

	template<typename Image>
	extern void AdaptiveStretch(Image& img, float thresh_coef = 1.0f, int thresh_exp = -3, float contrast_coef = 0.0f, int contrast_exp = -2, int num_data_points = 1'000'000);

	template<typename Image>
	extern void LocalHistogramEqualization(Image& img, int kernel_radius = 64, float contrast_limit = 1.5f, float amount = 1.0f, bool circular = false, int hist_res = 8);
}