#pragma once
#include "ProcessDialog.h"
#include "AdaptiveStretch.h"

class AdaptiveStretchDialog : public ProcessDialog {

	Q_OBJECT

private:
	//const QString m_name = "AdaptiveStretch";

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

	void resetDialog();

	void apply();

	void applyPreview();
};

