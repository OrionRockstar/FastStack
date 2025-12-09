#include "pch.h"
#include "FastStack.h"
#include "BilateralFilterDialog.h"
#include "ImageWindow.h"


BilateralFilterDialog::BilateralFilterDialog(Workspace* parent) : ProcessDialog("BilateralFilter", QSize(470, 145), parent) {

	setDefaultTimerInterval(750);

	addSigmaInputs();
	addSigmaIntensityInputs();
	addKernelSizeInputs();

	m_circular_cb = new CheckBox("Circular Kernel", drawArea());
	m_circular_cb->move(30, 102);
	connect(m_circular_cb, &QCheckBox::clicked, this, [this](bool v) { m_bf.setCircularKernel(v); applytoPreview(); });

	this->show();
}

void BilateralFilterDialog::addSigmaInputs() {

	m_sigma_s_le = new DoubleLineEdit(new DoubleValidator(0.10, 10.0, 2), drawArea());
	m_sigma_s_le->setValue(m_bf.sigmaSpatial());
	m_sigma_s_le->setFixedWidth(50);
	m_sigma_s_le->move(140, 15);
	addLabel(m_sigma_s_le, new QLabel("StdDev Spatial:", drawArea()));

	m_sigma_s_slider = new Slider(drawArea());
	m_sigma_s_slider->setRange(1, 100);
	m_sigma_s_slider->setFixedWidth(240);
	m_sigma_s_slider->setValue(m_bf.sigmaSpatial() * 10);
	m_sigma_s_slider->setSingleStep(10);
	m_sigma_s_le->addSlider(m_sigma_s_slider);

	auto action = [this](int) {
		float sigma = m_sigma_s_slider->sliderPosition() / 10.0;
		m_sigma_s_le->setValue(sigma);
		m_bf.setSigmaSpatial(sigma);
		startTimer();
	};

	auto edited = [this]() {
		float sigma = m_sigma_s_le->valuef();
		m_bf.setSigmaSpatial(sigma);
		m_sigma_s_slider->setValue(sigma * 10);
		applytoPreview();
	};

	connect(m_sigma_s_slider, &QSlider::actionTriggered, this, action);
	connect(m_sigma_s_le, &QLineEdit::editingFinished, this, edited);
}

void BilateralFilterDialog::addSigmaIntensityInputs() {

	m_sigma_r_le = new DoubleLineEdit(new DoubleValidator(0.01, 1.0, 2), drawArea());
	m_sigma_r_le->setValue(m_bf.sigmaRange());
	m_sigma_r_le->setFixedWidth(50);
	m_sigma_r_le->move(140, 55);
	addLabel(m_sigma_r_le, new QLabel("StdDev Range:", drawArea()));

	m_sigma_r_slider = new Slider(drawArea());
	m_sigma_r_slider->setRange(1, 100);
	m_sigma_r_slider->setFixedWidth(240);
	m_sigma_r_slider->setValue(m_bf.sigmaRange() * 100);
	m_sigma_r_le->addSlider(m_sigma_r_slider);

	auto action = [this](int) {
		float sigmaI = m_sigma_r_slider->sliderPosition() / 100.0;
		m_sigma_r_le->setValue(sigmaI);
		m_bf.setSigmaRange(sigmaI);
		startTimer();
	};

	auto edited = [this]() {
		float sigmaI = m_sigma_r_le->valuef();
		m_bf.setSigmaRange(sigmaI);
		m_sigma_r_slider->setValue(sigmaI * 100);
		applytoPreview();
	};

	connect(m_sigma_r_slider, &QSlider::actionTriggered, this, action);
	connect(m_sigma_r_le, &QLineEdit::editingFinished, this, edited);
}

void BilateralFilterDialog::addKernelSizeInputs() {

	m_kernel_size_cb = new ComboBox(drawArea());

	for (int i = 3; i <= 15; i += 2) {
		QString d = QString::number(i);
		QString t = QString::number(i * i);
		m_kernel_size_cb->addItem(d + "x" + d + " (" + t + " elements)");
	}

	m_kernel_size_cb->move(270, 100);
	addLabel(m_kernel_size_cb, new QLabel("Kernel Size:", drawArea()));

	auto activation = [this](int index) {
		m_bf.setKernelSize(3 + 2 * index);
		applytoPreview();
	};

	connect(m_kernel_size_cb, &QComboBox::activated, this, activation);
}

void BilateralFilterDialog::resetDialog() {

	m_bf = BilateralFilter();

	m_sigma_s_le->setValue(m_bf.sigmaSpatial());
	m_sigma_s_slider->setSliderPosition(m_bf.sigmaSpatial() * 10);

	m_sigma_r_le->setValue(m_bf.sigmaRange());
	m_sigma_r_slider->setSliderPosition(m_bf.sigmaRange() * 100);

	m_kernel_size_cb->setCurrentIndex(0);
	m_circular_cb->setChecked(m_bf.isCircular());

	applytoPreview();
}

void BilateralFilterDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	//std::unique_ptr<ProgressDialog> pd(std::make_unique<ProgressDialog>(m_bf.progressSignal()));

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->applyToSource(m_bf, &BilateralFilter::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		return iw16->applyToSource(m_bf, &BilateralFilter::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		return iw32->applyToSource(m_bf, &BilateralFilter::apply);
	}
	}
}

void BilateralFilterDialog::applyPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast(m_preview);

	switch (iwptr->type()) {

	case ImageType::UBYTE:
		return iwptr->updatePreview(m_bf, &BilateralFilter::applyTo);

	case ImageType::USHORT:
		return previewRecast<uint16_t>(iwptr)->updatePreview(m_bf, &BilateralFilter::applyTo);

	case ImageType::FLOAT:
		return previewRecast<float>(iwptr)->updatePreview(m_bf, &BilateralFilter::applyTo);
	}
}