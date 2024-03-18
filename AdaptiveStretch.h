#pragma once
#include "Image.h"
#include "Toolbar.h"
#include "qchart.h"
#include "qlineseries.h"
#include "qchartview.h"

class AdaptiveStretch {
	//rename to noise thresh
	float m_noise_thresh = 1;

	bool m_contrast_protection = false;
	float m_contrast = 0;

	int m_data_points = 1'000'000;

public:
	AdaptiveStretch() = default;

	void setNoiseThreshold(float noise_thresh) { m_noise_thresh = noise_thresh; }

	void setContrastProtection(bool contrast_protection) { m_contrast_protection = contrast_protection; }

	void setContrast(float contrast) { m_contrast = contrast; }

	void setDataPoints(int num) { m_data_points = num; }
private:
	template<typename T>
	Image<T> RGBtoI(Image<T>&img);

	template<typename T>
	std::vector<float> GetCumulativeNetForces(Image<T>&img);

public:
	template <typename T>
	void Apply(Image<T>&img);

	template<typename T>
	void ApplytoDest(Image<T>& src, Image<T>& dst, int factor);
};









class AdaptiveStretchDialog : public ProcessDialog {
	Q_OBJECT

private:
	const QString m_name = "AdaptiveStretch";

	AdaptiveStretch m_as;

	QLabel* m_noise_label;
	DoubleLineEdit* m_noise_le;

	DoubleLineEdit* m_noise_coef_le;
	QSlider* m_noise_coef_slider;
	QComboBox* m_noise_coef_exp;

	QLabel* m_contrast_label;
	DoubleLineEdit* m_contrast_le;

	DoubleLineEdit* m_contrast_coef_le;
	QSlider* m_contrast_coef_slider;
	QComboBox* m_contrast_coef_exp;

	QCheckBox* m_contrast_cb;

	QLabel* m_curve_pts_label;
	QLineEdit* m_curve_pts_le;

	QLineSeries* m_series;
	QChart* m_graph;

	
public:
	AdaptiveStretchDialog(QWidget* parent);

private:

	void editingFinished_noise();

	void editingFinished_noise_coef();

	void actionSlider_noise_coef(int action);

	void sliderMoved_noise_coef(int value);

	void itemSelected_noise_coef(int index);



	void editingFinished_contrast();

	void editingFinished_contrast_coef();

	void actionSlider_contrast_coef(int action);

	void sliderMoved_contrast_coef(int value);

	void itemSelected_contrast_coef(int index);

	void onContrastStateChanged(int state);


	void editingFinished_data_pts();

	void AddNoiseThresholdInputs();

	void AddContrastProtectionInputs();

	void showPreview();

	void resetDialog();

	void Apply();

	void ApplytoPreview();
};

