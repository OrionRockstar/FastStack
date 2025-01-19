#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "qchart.h"
#include "qlineseries.h"
#include "qchartview.h"

class AdaptiveStretch {
public:
	class CDFCurve {
	public:
		std::vector<float> m_curve;
		float m_min = 0.0;
		float m_max = 1.0;

	public:
		size_t size()const { return m_curve.size(); }

		void resize(size_t new_size) { m_curve.resize(new_size); }

		float& operator[](int el) { return m_curve[el]; }

		float min()const { return m_min; }

		void setMin(float min) { m_min = min; }

		float max()const { return m_max; }

		void setMax(float max) { m_max = max; }
	};

private:
	float m_noise_thresh = 0.001;

	bool m_contrast_protection = false;
	float m_contrast_threshold = 0.0001;

	int m_data_points = 1'000'000;

	CDFCurve m_cdf_curve;

public:
	AdaptiveStretch() = default;

	float noiseThreshold()const { return m_noise_thresh; }

	void setNoiseThreshold(float noise_thresh) { m_noise_thresh = noise_thresh; }

	void setContrastProtection(bool contrast_protection) { m_contrast_protection = contrast_protection; }

	float contrastTreshold()const { return m_contrast_threshold; }

	void setContrastThreshold(float contrast) { m_contrast_threshold = contrast; }

	void setDataPoints(int num) { m_data_points = num; }

	template<typename T>
	void computeCDF(const Image<T>& img);

	template <typename T>
	void apply(Image<T>&img);

	template <typename T>
	void apply_NoCDF(Image<T>& img);
};









class AdaptiveStretchDialog : public ProcessDialog {
	Q_OBJECT

private:
	const QString m_name = "AdaptiveStretch";

	AdaptiveStretch m_as;

	DoubleLineEdit* m_noise_le;
	DoubleLineEdit* m_noise_coef_le;
	Slider* m_noise_coef_slider;
	SpinBox* m_noise_coef_exp;

	DoubleLineEdit* m_contrast_le;
	DoubleLineEdit* m_contrast_coef_le;
	Slider* m_contrast_coef_slider;
	SpinBox* m_contrast_coef_exp;

	QCheckBox* m_contrast_cb;

	IntLineEdit* m_curve_pts_le;

	//QLineSeries* m_series;
	//QChart* m_graph;
public:
	AdaptiveStretchDialog(QWidget* parent);

private:
	void addNoiseThresholdInputs();

	void connectNoiseInputs();

	void addContrastProtectionInputs();

	void connectContrastInputs();

	void showPreview();

	void resetDialog();

	void apply();

	void applytoPreview();
};

