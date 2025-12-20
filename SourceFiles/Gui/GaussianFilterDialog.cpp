#include "pch.h"
#include "GaussianFilterDialog.h"
#include "ImageWindow.h"
#include "FastStack.h"


GaussianFilterDialog::GaussianFilterDialog(Workspace* parent) : ProcessDialog("GaussianFilter", QSize(405, 85), parent) {

	setDefaultTimerInterval(250);

	addSigmaInputs();

	m_ks_label = new QLabel(drawArea());
	m_ks_label->move(135, 50);
	m_ks_label->setFixedWidth(150);
	onValueChanged(2.0);

	this->show();
}

void GaussianFilterDialog::addSigmaInputs() {

	m_sigma_le = new DoubleLineEdit(m_gf.sigma(), new DoubleValidator(0.1, 20.0, 2), drawArea());
	m_sigma_le->setFixedWidth(50);
	m_sigma_le->move(80, 15);
	addLabel(m_sigma_le, new QLabel("StdDev:", drawArea()));

	m_sigma_slider = new Slider(drawArea());
	m_sigma_slider->setRange(1, 200);
	m_sigma_slider->setFixedWidth(250);
	m_sigma_slider->setValue(20);
	m_sigma_slider->setSingleStep(10);
	m_sigma_le->addSlider(m_sigma_slider);

	auto action = [this](int) {
		float sigma = m_sigma_slider->sliderPosition() / 10.0;
		m_sigma_le->setValue(sigma);
		m_gf.setSigma(sigma);
		onValueChanged(sigma);
		startTimer();
	};

	auto edited = [this]() {
		float sigma = m_sigma_le->valuef();;
		m_sigma_slider->setValue(sigma * 10);
		m_gf.setSigma(sigma);
		onValueChanged(sigma);
	};

	connect(m_sigma_slider, &QSlider::actionTriggered, this, action);
	connect(m_sigma_le, &QLineEdit::editingFinished, this, edited);
}

void GaussianFilterDialog::onValueChanged(float sigma) {
	int k_rad = (3.0348 * sigma) + 0.5;
	QString kd = QString::number(2 * k_rad + 1);

	m_ks_label->setText(m_ksize + kd + " x " + kd);
}

void GaussianFilterDialog::resetDialog() {

	m_gf = GaussianFilter();
	m_sigma_le->reset();
	m_sigma_slider->setValue(m_gf.sigma() * 10);
	onValueChanged(m_gf.sigma());

	applytoPreview();
}

void GaussianFilterDialog::apply() {

	if (workspace()->subWindowList().size() == 0)
		return;

	switch (currentImageType()) {
	case ImageType::UBYTE: 
		return currentImageWindow()->applyToSource(m_gf, &GaussianFilter::apply);
	
	case ImageType::USHORT: 
		return currentImageWindow<uint16_t>()->applyToSource(m_gf, &GaussianFilter::apply);
	
	case ImageType::FLOAT: 
		return currentImageWindow<float>()->applyToSource(m_gf, &GaussianFilter::apply);
	}
}

void GaussianFilterDialog::applyPreview() {

	if (!isPreviewValid())
		return;

	GaussianFilter gf(m_sigma_le->value() * preview()->scaleFactor());

	switch (preview()->type()) {
	case ImageType::UBYTE:
		return preview()->updatePreview(gf, &GaussianFilter::apply);
	
	case ImageType::USHORT: 
		return preview<uint16_t>()->updatePreview(gf, &GaussianFilter::apply);
	
	case ImageType::FLOAT:
		return preview<float>()->updatePreview(gf, &GaussianFilter::apply);
	}
}