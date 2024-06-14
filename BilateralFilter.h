#pragma once
#include "Image.h"
#include "ProcessDialog.h"

class BilateralFilter {

	ProgressSignal* m_ps = new ProgressSignal();

	float m_sigma = 2.0;
	float m_sigma_range = 2.0;
	int m_kernel_dim = 3;

	template <typename T>
	struct FloatKernel2D {
		const Image<T>* m_img;
		std::unique_ptr<float[]> data;
		int m_dim = 0;
		int m_radius = 0;
		int m_size = 0;

		FloatKernel2D(const Image<T>& img, int radius) : m_radius(radius) {
			m_img = &img;
			m_dim = 2 * m_radius + 1;
			m_size = m_dim * m_dim;
			data = std::make_unique<float[]>(m_size);
		}

		~FloatKernel2D() {};

		float& operator[](int el) {
			return data[el];
		}

		void Populate(int y, int ch) {

			for (int j = -m_radius, el = 0; j <= m_radius; ++j)
				for (int i = -m_radius; i <= m_radius; ++i)
					data[el++] = Pixel<float>::toType(m_img->At_mirrored(i, y + j, ch));
		}

		void Update(int x, int y, int ch) {

			int xx = x + m_radius;
			if (xx >= m_img->Cols())
				xx = 2 * m_img->Cols() - (xx + 1);

			for (int j = 0; j < m_size; j += m_dim)
				for (int i = 0; i < m_dim - 1; ++i)
					data[j + i] = data[j + i + 1];

			for (int j = -m_radius, el = m_dim - 1; j <= m_radius; ++j, el += m_dim) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				else if (yy >= m_img->Rows())
					yy = 2 * m_img->Rows() - (yy + 1);

				data[el] = Pixel<float>::toType((*m_img)(xx, yy, ch));

			}
			
		}

	};

public:
	ProgressSignal* progressSignal() const { return m_ps; }

	float Sigma()const { return m_sigma; }

	void setSigma(float sigma) { m_sigma = sigma; }

	float SigmaRange()const { return m_sigma_range; }

	void setSigmaRange(float sigma_range) { m_sigma_range = sigma_range; }

	void setKernelSize(int kernel_dim) { m_kernel_dim = kernel_dim; }

	template<typename T>
	void Apply(Image<T>& img);
};








class BilateralFilterDialog : public ProcessDialog {

	BilateralFilter m_bf;

	DoubleLineEdit* m_sigma_le;
	QSlider* m_sigma_slider;

	DoubleLineEdit* m_sigma_intensity_le;
	QSlider* m_sigma_intensity_slider;

	QComboBox* m_kerenl_size_cb;

public:
	BilateralFilterDialog(QWidget* parent = nullptr);

private:
	void onActionTriggered_sigma(int action);

	void onActionTriggered_sigmaIntensity(int action);

	void AddSigmaInputs();

	void AddSigmaIntensityInputs();

	void onActivation_ks(int index);

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();

};