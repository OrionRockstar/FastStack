#pragma once
#include "Image.h"
#include <vector>
#include "ProcessDialog.h"

class GaussianFilter {
	std::vector<float> m_gaussian_kernel;
	int m_kernel_dim = 14;
	float m_sigma = 2.0;

	struct PixelWindow {
		std::vector<float> data;
		int m_size = 0;
		int m_radius = 0;

		PixelWindow(int size) {
			data.resize(size);
			m_size = size;
			m_radius = (data.size() - 1) / 2;
		}

		PixelWindow(const PixelWindow& other) {
			data = other.data;
			m_size = other.m_size;
			m_radius = other.m_radius;
		}

		~PixelWindow() {};

		float& operator[](int el) {
			return data[el];
		}

		int size()const { return m_size; }

		int radius()const { return m_radius; }

		template<typename T>
		void populateRowWindow(const Image<T>& img, const ImagePoint& p) {

			for (int j = 0; j < m_size; ++j) {
				int xx = j - m_radius;
				if (xx < 0)
					xx = -xx;

				data[j] = Pixel<float>::toType(img(xx, p.y(), p.channel()));
			}
		}

		template<typename T>
		void updateRowWindow(const Image<T>& img, const ImagePoint& p) {

			data.erase(data.begin());

			int xx = p.x() + m_radius;
			if (xx >= img.cols())
				xx = 2 * img.cols() - (xx + 1);

			data.emplace_back(Pixel<float>::toType(img(xx, p.y(), p.channel())));

		}

		template<typename T>
		void populateColWindow(const Image<T>& img, const ImagePoint& p) {

			for (int j = 0; j < m_size; ++j) {
				int yy = j - m_radius;
				if (yy < 0)
					yy = -yy;

				data[j] = Pixel<float>::toType(img(p.x(), yy, p.channel()));
			}
		}

		template<typename T>
		void updateColWindow(const Image<T>& img, const ImagePoint& p) {

			data.erase(data.begin());

			int yy = p.y() + m_radius;
			if (yy >= img.rows())
				yy = 2 * img.rows() - (yy + 1);

			data.emplace_back(Pixel<float>::toType(img(p.x(), yy, p.channel())));
		}
	};

public:
	GaussianFilter() = default;

	GaussianFilter(float sigma);

	void setSigma(float sigma);

	void setKernelDimension(int kernel_dimension);

	float sigma()const { return m_sigma; }
private:

	void buildGaussianKernel();

public:

	template<typename T>
	void apply(Image<T>& img);

};







class GaussianFilterDialog : public ProcessDialog {

	GaussianFilter m_gf;

	DoubleLineEdit* m_sigma_le;
	Slider* m_sigma_slider;
	QString m_ksize = "Kernel Size:   ";
	QLabel* m_ks_label;

public:
	GaussianFilterDialog(QWidget* parent = nullptr);

private:
	void addSigmaInputs();

	void onValueChanged(float sigma);

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};