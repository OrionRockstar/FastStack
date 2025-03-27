#include "pch.h"
#include "GaussianFilterDialog.h"
#include "ImageWindow.h"
#include "FastStack.h"


GaussianFilterDialog::GaussianFilterDialog(QWidget* parent) : ProcessDialog("GaussianFilter", QSize(405, 85), FastStack::recast(parent)->workspace()) {

	setTimerInterval(250);
	setPreviewMethod(this, &GaussianFilterDialog::applytoPreview);
	connectToolbar(this, &GaussianFilterDialog::apply, &GaussianFilterDialog::showPreview, &GaussianFilterDialog::resetDialog);

	addSigmaInputs();

	m_ks_label = new QLabel(drawArea());
	m_ks_label->move(135, 50);
	m_ks_label->setFixedWidth(150);
	onValueChanged(2.0);

	this->show();
}

void GaussianFilterDialog::addSigmaInputs() {

	m_sigma_le = new DoubleLineEdit(new DoubleValidator(0.1, 20.0, 2), drawArea());
	m_sigma_le->setValue(2.0);
	m_sigma_le->setFixedWidth(50);
	m_sigma_le->move(80, 15);
	addLabel(m_sigma_le, new QLabel("StdDev:", drawArea()));

	m_sigma_slider = new Slider(Qt::Horizontal, drawArea());
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
	m_sigma_le->setValue(m_gf.sigma());
	m_sigma_slider->setValue(m_gf.sigma() * 10);
	onValueChanged(m_gf.sigma());

	applytoPreview();
}

void GaussianFilterDialog::showPreview() {

	ProcessDialog::showPreview();
	applytoPreview();
}

void GaussianFilterDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	enableSiblings(false);

	auto iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	m_gf.setSigma(m_sigma_le->text().toFloat());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_gf, &GaussianFilter::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_gf, &GaussianFilter::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_gf, &GaussianFilter::apply);
		break;
	}
	}

	enableSiblings(true);

	applytoPreview();
}

void GaussianFilterDialog::applytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast(m_preview);

	m_gf.setSigma(m_sigma_le->text().toFloat() * iwptr->scaleFactor());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_gf, &GaussianFilter::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_gf, &GaussianFilter::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_gf, &GaussianFilter::apply);
	}
	}
}