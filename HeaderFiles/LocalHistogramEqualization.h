#pragma once
#include "Image.h"
#include "ProcessDialog.h"

class LocalHistogramEqualization {

	ProgressSignal m_ps;

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
		int m_size = 256;
		int m_count = 0;

		int m_radius = 64;
		int m_dimension = 2 * m_radius + 1;

		int m_multiplier = m_size - 1;
		bool m_is_circular = false;

		std::vector<int> back_pix;//contains displacement from center coloumn
		std::vector<int> front_pix;//
		std::vector<char> k_mask;

		uint32_t& operator[](int val) { return histogram[val]; }

		KernelHistogram(int resolution);

		KernelHistogram(int resolution, int kernel_radius, bool circular);

		int Size()const { return m_size; }

		int Count()const { return m_count; }

		int Multiplier()const { return m_multiplier; }

		template<typename T>
		void Populate(Image<T>& img, int y);

		template<typename T>
		void Update(Image<T>& img, int x, int y);

		void Clip(int limit);

		void CopyTo(KernelHistogram& other);
	};

public:
	const ProgressSignal* progressSignal()const { return &m_ps; }

	int kernelRadius()const { return m_kernel_radius; }

	void setKernelRadius(int kernel_radius) { m_kernel_radius = kernel_radius; }

	void setContrastLimit(float contrast_limit) { m_contrast_limit = contrast_limit; }

	void setAmount(float amount) { m_amount = amount; }

	void setCircularKernel(bool is_circular) { 
		m_is_circular = is_circular; }

	void setHistogramResolution(int resolution) { m_hist_res = resolution; }

	template<typename T>
	void apply(Image<T>&img);
};









class LocalHistogramEqualizationDialog : public ProcessDialog {
	Q_OBJECT

	LocalHistogramEqualization m_lhe;

	std::unique_ptr<ProgressDialog> m_pd;

	//kernel radius
	IntLineEdit* m_kr_le;
	Slider* m_kr_slider;

	//contrast limit
	DoubleLineEdit* m_cl_le;
	Slider* m_cl_slider;

	//amount
	DoubleLineEdit* m_amount_le;
	Slider* m_amount_slider;

	CheckBox* m_circular;

	ComboBox* m_histogram_resolution;
	std::array<int, 3> m_res = { 8,10,12 };

public:
	LocalHistogramEqualizationDialog(QWidget* parent = nullptr);

private:
	signals:
	void finished();

private:
	void addKernelRadiusInputs();

	void addContrastLimitInputs();

	void addAmountInputs();

	void showPreview();

	void resetDialog();

	void apply();

	void applytoPreview();
};