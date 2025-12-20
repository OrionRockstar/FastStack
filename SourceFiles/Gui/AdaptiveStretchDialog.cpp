#include "pch.h"
#include "AdaptiveStretchDialog.h"
#include "FastStack.h"

using ASD = AdaptiveStretchDialog;

ASD::AdaptiveStretchDialog(Workspace* parent) : ProcessDialog("AdaptiveStretch", QSize(650, 135), parent) {

	setDefaultTimerInterval(500);

	addNoiseThresholdInputs();
	addContrastProtectionInputs();

	m_curve_pts_le = new IntLineEdit(1000000, new IntValidator(100, 10'000'000), drawArea());
	//m_curve_pts_le->setFixedWidth(75);
	m_curve_pts_le->move(360, 90);
	addLabel(m_curve_pts_le, new QLabel("Maximum Curve Points:", drawArea()));
	connect(m_curve_pts_le, &QLineEdit::editingFinished, this, [this]() {
		m_as.setDataPoints(m_curve_pts_le->value());
		applytoPreview(); });


	//m_series = new QLineSeries;
	//m_graph = new QChart;
	//m_graph->addSeries(m_series);
	//m_graph->resize(400, 400);
	//QChartView* cv = new QChartView(m_graph, parent);
	//cv->resize(400, 400);

	this->show();
}

void ASD::addNoiseThresholdInputs() {

	int dy = 15;

	m_noise_le = new DoubleLineEdit(0.001, new DoubleValidator(0.0, 1.0, 8), drawArea());
	m_noise_le->setFixedWidth(85);
	m_noise_le->move(150, dy);
	addLabel(m_noise_le, new QLabel("Noise Threshold:", drawArea()));

	m_noise_coef_le = new DoubleLineEdit(1.0, new DoubleValidator(1.00, 9.99, 2), 4, drawArea());
	m_noise_coef_le->setFixedWidth(50);
	m_noise_coef_le->move(240, dy);

	m_noise_coef_slider = new Slider(drawArea());
	m_noise_coef_slider->setRange(100, 999);
	m_noise_coef_slider->setFixedWidth(250);
	m_noise_coef_le->addSlider(m_noise_coef_slider);


	m_noise_coef_exp = new SpinBox(drawArea());
	m_noise_coef_exp->setRange(-6, 0);
	m_noise_coef_exp->setValue(-3);
	m_noise_coef_exp->move(565, dy);

	connectNoiseInputs();
}

void ASD::connectNoiseInputs() {

	auto edited_noise = [this]() {

		double val = m_noise_le->value();

		int exp = floor(log10(val));
		double coef = val / pow(10, exp);
		m_noise_coef_exp->setValue(exp);
		m_noise_coef_le->setValue(coef);

		if (exp == 0)
			m_noise_coef_slider->setValue(0);
		else
			m_noise_coef_slider->setValue(coef * 100);

		m_as.setNoiseThreshold(val);
		applytoPreview();
	};
	connect(m_noise_le, &QLineEdit::editingFinished, this, edited_noise);

	auto edited_coef = [this]() {

		double val = m_noise_coef_le->value();

		m_noise_coef_slider->setValue(val * 100);

		val *= pow(10, m_noise_coef_exp->value());
		m_noise_le->setValue(val);

		m_as.setNoiseThreshold(val);
		applytoPreview();
	};
	connect(m_noise_coef_le, &QLineEdit::editingFinished, this, edited_coef);

	auto action = [this](int) {

		if (m_noise_coef_exp->value() == 0)
			return m_noise_coef_slider->setValue(0);

		double coef = m_noise_coef_slider->sliderPosition() / 100.0;

		m_noise_coef_le->setValue(coef);
		coef *= pow(10, m_noise_coef_exp->value());
		m_noise_le->setValue(coef);

		m_as.setNoiseThreshold(coef);
		startTimer();
	};
	connect(m_noise_coef_slider, &QSlider::actionTriggered, this, action);

	auto onValueChanged = [this](int val) {

		double coef = m_noise_coef_le->value();

		if (val == 0) {
			m_noise_coef_slider->setValue(0);
			m_noise_coef_le->setValue(coef = 1.0);
		}

		coef *= pow(10, val);
		m_noise_le->setValue(coef);

		m_as.setNoiseThreshold(coef);
		startTimer();
	};
	connect(m_noise_coef_exp, &QSpinBox::valueChanged, this, onValueChanged);
}

void ASD::addContrastProtectionInputs() {

	int dy = 55;

	m_contrast_le = new DoubleLineEdit(0.01, new DoubleValidator(0.0, 1.0, 8), drawArea());
	m_contrast_le->setFixedWidth(85);
	m_contrast_le->move(150, dy);
	addLabel(m_contrast_le, new QLabel("Contrast:", drawArea()));

	m_contrast_coef_le = new DoubleLineEdit(1.0, new DoubleValidator(1.00, 9.99, 2), 4, drawArea());
	m_contrast_coef_le->setFixedWidth(50);
	m_contrast_coef_le->move(240, dy);

	m_contrast_coef_slider = new Slider(drawArea());
	m_contrast_coef_slider->setRange(100, 999);
	m_contrast_coef_slider->setFixedWidth(250);
	m_contrast_coef_le->addSlider(m_contrast_coef_slider);

	m_contrast_coef_exp = new SpinBox(drawArea());
	m_contrast_coef_exp->move(565, dy);
	m_contrast_coef_exp->setRange(-6, 0);
	m_contrast_coef_exp->setValue(-2);

	m_contrast_cb = new QCheckBox(drawArea());
	m_contrast_cb->move(620, dy + 5);

	connectContrastInputs();
	m_contrast_cb->clicked(false);
}

void ASD::connectContrastInputs() {

	auto edited_contrast = [this]() {

		double val = m_contrast_le->value();

		int exp = floor(log10(val));
		double coef = val / pow(10, exp);
		m_contrast_coef_exp->setValue(exp);
		m_contrast_coef_le->setValue(coef);

		if (exp == 0)
			m_contrast_coef_slider->setValue(0);
		else
			m_contrast_coef_slider->setValue(coef * 100);

		m_as.setContrastThreshold(val);
		applytoPreview();
	};
	connect(m_contrast_le, &QLineEdit::editingFinished, this, edited_contrast);

	auto edited_coef = [this]() {

		double val = m_contrast_coef_le->value();

		m_contrast_coef_slider->setValue(val * 100);

		val *= pow(10, m_contrast_coef_exp->value());
		m_contrast_le->setValue(val);

		m_as.setContrastThreshold(val);
		applytoPreview();
	};
	connect(m_contrast_coef_le, &QLineEdit::editingFinished, this, edited_coef);

	auto action = [this](int) {

		if (m_contrast_coef_exp->value() == 0)
			return m_contrast_coef_slider->setValue(0);

		double coef = m_contrast_coef_slider->sliderPosition() / 100.0;

		m_contrast_coef_le->setValue(coef);
		coef *= pow(10, m_contrast_coef_exp->value());
		m_contrast_le->setValue(coef);

		m_as.setContrastThreshold(coef);
		startTimer();
	};
	connect(m_contrast_coef_slider, &QSlider::actionTriggered, this, action);

	auto onValueChanged = [this](int val) {

		double coef = m_contrast_coef_le->value();

		if (val == 0) {
			m_contrast_coef_slider->setValue(0);
			m_contrast_coef_le->setValue(coef = 1.0);
		}

		coef *= pow(10, val);
		m_contrast_le->setValue(coef);

		m_as.setContrastThreshold(coef);
		startTimer();
	};
	connect(m_contrast_coef_exp, &QSpinBox::valueChanged, this, onValueChanged);

	auto onClicked = [this](bool v) {

		m_contrast_le->setEnabled(v);
		m_contrast_coef_le->setEnabled(v);
		m_contrast_coef_slider->setEnabled(v);
		m_contrast_coef_exp->setEnabled(v);

		m_as.setContrastProtection(v);

		applytoPreview();
	};
	connect(m_contrast_cb, &QCheckBox::clicked, this, onClicked);
}

void ASD::resetDialog() {

	m_noise_coef_le->setValue(1.0);
	m_noise_coef_slider->setValue(0);
	m_noise_coef_exp->setValue(-3);
	m_noise_le->setValue(0.001);

	m_contrast_coef_le->setValue(1.0);
	m_contrast_coef_slider->setValue(0);
	m_contrast_coef_exp->setValue(-4);
	m_contrast_le->setValue(0.0001);

	m_contrast_cb->setChecked(false);
	m_contrast_cb->clicked(false);
}

void ASD::apply() {

	if (!workspace()->hasSubWindows())
		return;

	switch (currentImageType()) {
	case ImageType::UBYTE: 
		return currentImageWindow()->applyToSource(m_as, &AdaptiveStretch::apply);
	
	case ImageType::USHORT: 
		return currentImageWindow<uint16_t>()->applyToSource(m_as, &AdaptiveStretch::apply);
	
	case ImageType::FLOAT:
		return currentImageWindow<float>()->applyToSource(m_as, &AdaptiveStretch::apply);
	}
}

void ASD::applyPreview() {

	if (!isPreviewValid())
		return;

	switch (preview()->type()) {
	case ImageType::UBYTE: {
		auto p = preview();
		m_as.computeCDF(p->imageWindow()->source());
		return p->updatePreview(m_as, &AdaptiveStretch::apply_NoCDF);
	}
	case ImageType::USHORT: {
		auto p = preview<uint16_t>();
		m_as.computeCDF(p->imageWindow()->source());
		return p->updatePreview(m_as, &AdaptiveStretch::apply_NoCDF);
	}
	case ImageType::FLOAT: {
		auto p = preview<float>();
		m_as.computeCDF(p->imageWindow()->source());
		return p->updatePreview(m_as, &AdaptiveStretch::apply_NoCDF);
	}
	}
}