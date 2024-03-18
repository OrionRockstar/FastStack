#pragma once
#include "Image.h"
#include "Toolbar.h"

class LocalHistogramEqualization {

	int m_kernel_radius = 64;
	float m_contrast_limit = 2.0;
	float m_amount = 1.0;

	bool m_is_circular = false;

	int m_hist_res = 8;
public:
	LocalHistogramEqualization() = default;

private:
	struct KernelHistogram {

		std::unique_ptr<uint32_t[]> histogram;
		int m_kernel_radius = 64;
		int m_size = 256;
		int pix_count = 0;

		int m_multiplier = m_size - 1;
		bool m_is_circular = false;

		std::vector<int> back_pix;
		std::vector<int> front_pix;
		std::vector<bool> k_mask;

		uint32_t& operator[](int val) { return histogram[val]; }

		KernelHistogram(int resolution);

		KernelHistogram(int resolution, int kernel_radius, bool circular);

		int Size()const noexcept { return m_size; }

		int Multiplier()const noexcept { return m_multiplier; }

		template<typename T>
		void Populate(Image<T>& img, int y);

		template<typename T>
		void Update(Image<T>& img, int x, int y);

		void Clip(int limit);

		void CopyTo(KernelHistogram& other);
	};

public:

	void setKernelRadius(int kernel_radius) { m_kernel_radius = kernel_radius; }

	void setContrastLimit(float contrast_limit) { m_contrast_limit = contrast_limit; }

	void setAmount(float amount) { m_amount = amount; }

	void setCircularKernel(bool is_circular) { m_is_circular = is_circular; }

	void setHistogramResolution(int resolution) { m_hist_res = resolution; }

	template<typename T>
	void Apply(Image<T>&img);
};









class LocalHistogramEqualizationDialog : public ProcessDialog {
	Q_OBJECT

	LocalHistogramEqualization m_lhe;

	//kernel radius
	QLabel* m_kr_label;
	QLineEdit* m_kr_le;
	QSlider* m_kr_slider;

	//contrast limit
	QLabel* m_cl_label;
	DoubleLineEdit* m_cl_le;
	QSlider* m_cl_slider;

	//amount
	QLabel* m_amount_label;
	DoubleLineEdit* m_amount_le;
	QSlider* m_amount_slider;

	QCheckBox* m_circular;
	QLabel* m_circular_label;

	QComboBox* m_histogram_resolution;
	std::array<int, 3> m_res = { 8,10,12 };
	QLabel* m_hr_label;

public:
	LocalHistogramEqualizationDialog(QWidget* parent = nullptr);

private:
	void actionSlider_kr(int action);

	void sliderMoved_kr(int value);


	void actionSlider_cl(int action);

	void sliderMoved_cl(int value);


	void actionSlider_amount(int action);

	void sliderMoved_amount(int value);


	void itemSelected(int index);

	void AddKernelRadiusInputs();

	void AddContrastLimitInputs();

	void AddAmountInputs();

	void showPreview();

	void resetDialog();

	void Apply();

	void ApplytoPreview();
};