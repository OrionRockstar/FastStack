#pragma once
#include "Image.h"
#include "ProcessDialog.h"

class BilateralFilter {

	//ProgressSignal* m_ps = new ProgressSignal();

	float m_sigma_s = 2.0; //spatial/gaussiam
	float m_sigma_r = 0.5; //pixel_intensity
	int m_kernel_dim = 3;

	class FloatKernel {
		std::unique_ptr<float[]> data;

		int m_dim = 1;
		int m_radius = 0;
		int m_size = 0;
	public:
		FloatKernel(int kernel_dim) : m_dim(kernel_dim) {
			//m_img = &img;
			//m_dim = 2 * m_radius + 1;
			m_radius = (m_dim - 1) / 2;
			m_size = m_dim * m_dim;
			data = std::make_unique<float[]>(m_size);
		}

		FloatKernel(const FloatKernel& other) {
			//m_img = other.m_img;
			m_dim = other.m_dim;
			m_radius = other.m_radius;
			m_size = other.m_size;

			data = std::make_unique<float[]>(m_size);
			memcpy(data.get(), other.data.get(), m_size * sizeof(float));
		}

		~FloatKernel() {};

		float& operator[](int el) {
			return data[el];
		}

		float& operator()(int x, int y) {
			return data[y * m_dim + x];
		}

		int count()const { return m_size; }

		int dimension()const { return m_dim; }

		int radius()const { return m_radius; }

		template<typename T>
		void populate(const Image<T>& src, const ImagePoint& p) {

			for (int j = -radius(), el = 0; j <= radius(); ++j)
				for (int i = -radius(); i <= radius(); ++i)
					data[el++] = Pixel<float>::toType(src.At_mirrored(p.x() + i, p.y() + j, p.channel()));
		}

		template<typename T>
		void update(const Image<T>& src, const ImagePoint& p) {

			int xx = p.x() + radius();
			if (xx >= src.cols())
				xx = 2 * src.cols() - (xx + 1);

			memcpy(data.get(), data.get() + 1, sizeof(float) * count() - 1);

			for (int j = -radius(), el = dimension() - 1; j <= radius(); ++j, el += dimension()) {

				int yy = p.y() + j;
				if (yy < 0)
					yy = -yy;
				else if (yy >= src.rows())
					yy = 2 * src.rows() - (yy + 1);

				data[el] = Pixel<float>::toType(src(xx, yy, p.channel()));
			}
		}
	};

public:
	//ProgressSignal* progressSignal() const { return m_ps; }

	float sigmaSpatial()const { return m_sigma_s; }

	void setSigmaSpatial(float sigma) { m_sigma_s = sigma; }

	float sigmaRange()const { return m_sigma_r; }

	void setSigmaRange(float sigma_range) { m_sigma_r = sigma_range; }

	void setKernelSize(int kernel_dim) { m_kernel_dim = Max(kernel_dim, 1); }

	template<typename T>
	void apply(Image<T>& img);

	template<typename T>
	void applyTo(const Image<T>&src, Image<T>& dst, int factor);
};








class BilateralFilterDialog : public ProcessDialog {

	BilateralFilter m_bf;

	DoubleLineEdit* m_sigma_s_le;
	Slider* m_sigma_s_slider;

	DoubleLineEdit* m_sigma_r_le;
	Slider* m_sigma_r_slider;

	ComboBox* m_kernel_size_cb;

public:
	BilateralFilterDialog(QWidget* parent = nullptr);

private:
	void addSigmaInputs();

	void addSigmaIntensityInputs();

	void addKernelSizeInputs();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();

};