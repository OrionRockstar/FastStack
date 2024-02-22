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









class LocalHistogramEqualizationDialog : public QDialog {
	Q_OBJECT

	QString m_name = "LocalHistogramEqualization";

	LocalHistogramEqualization m_lhe;
	QMdiArea* m_workspace;

	QLabel* m_kernelradius_label;
	QLineEdit* m_kernelradius_le;
	QIntValidator* m_kernelradius_iv;
	QSlider* m_kernelradius_slider;

	QLabel* m_contrastlimit_label;
	QLineEdit* m_contrastlimit_le;
	QDoubleValidator* m_contrastlimit_dv;
	QSlider* m_contrastlimit_slider;

	QLabel* m_amount_label;
	QLineEdit* m_amount_le;
	QDoubleValidator* m_amount_dv;
	QSlider* m_amount_slider;

	QCheckBox* m_circular;
	QLabel* m_circular_label;

	QComboBox* m_histogram_resolution;
	std::array<int, 3> m_res = { 8,10,12 };
	QLabel* m_hr_label;

	QPushButton* apply;

	Toolbar* tb;

public:
	LocalHistogramEqualizationDialog(QWidget* parent = nullptr);

	void closeEvent(QCloseEvent* close) {
		onClose();
		close->accept();
	}

	signals:
	void onClose();

private:
	//work on textbox inputs

	void sliderMoved_kernelradius(int value) {
		m_kernelradius_le->setText(QString::number((value / 2) * 2));

		//ApplytoPreview();
	}

	void sliderMoved_contrastlimit(int value) {
		if (value < 20)
			m_contrastlimit_le->setText(QString::number(value / 2.0, 'f', 1));
		else
			m_contrastlimit_le->setText(QString::number(value - 10));

		//ApplytoPreview();
	}

	void sliderMoved_amount(int value) {
		m_amount_le->setText(QString::number(value / 100.0, 'f', 3));
		//ApplytoPreview();
	}

	void itemSelected(int index) {
		//m_lhe.setHistogramResolution(m_res[index]);
		ApplytoPreview();
	}

	void AddKernelRadiusInputs();

	void AddContrastLimitInputs();

	void AddAmountInputs();

	void showPreview();

	void resetDialog();

	void Apply();

	void ApplytoPreview();
};