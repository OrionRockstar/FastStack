#include "pch.h"
#include "FastStack.h"
#include "BilateralFilter.h"

template<typename T>
void BilateralFilter::Apply(Image<T>& img) {

	int kernel_radius = (m_kernel_dim - 1) / 2;

	Image<T> temp(img);

	float k = 1 / (2 * m_sigma * m_sigma);
	float k_r = 1 / (2 * m_sigma_range * m_sigma_range);


	std::vector<float> gk((2 * kernel_radius + 1) * (2 * kernel_radius + 1));

	for (int j = -kernel_radius, el = 0; j <= kernel_radius; ++j)
		for (int i = -kernel_radius; i <= kernel_radius; ++i)
			gk[el++] = -k * ((j * j) + (i * i));

	int sum = 0;

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			FloatKernel2D kernel(img, kernel_radius);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				float sum = 0.0f;
				float weight = 0.0f;

				for (int el = 0; el < kernel.m_size; ++el) {
					float d = Pixel<float>::toType(img(x, y, ch)) - kernel[el];
					float w = exp(gk[el] - (k_r * d * d));
					weight += w;
					sum += w * kernel[el];
				}

				temp(x, y, ch) = Pixel<T>::toType(sum / weight);

			}
#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / (img.Rows() * img.Channels()));
		}
	}

	m_ps->emitProgress(100);

	temp.MoveTo(img);
}
template void BilateralFilter::Apply(Image8&);
template void BilateralFilter::Apply(Image16&);
template void BilateralFilter::Apply(Image32&);









BilateralFilterDialog::BilateralFilterDialog(QWidget* parent) : ProcessDialog("BilateralFilter", QSize(470, 140), *reinterpret_cast<FastStack*>(parent)->workspace(), parent) {

	setTimer(250, this, &BilateralFilterDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &BilateralFilterDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &BilateralFilterDialog::Apply, &BilateralFilterDialog::showPreview, &BilateralFilterDialog::resetDialog);

	AddSigmaInputs();
	AddSigmaIntensityInputs();

	m_kerenl_size_cb = new QComboBox(this);

	for (int i = 3; i <= 15; i += 2) {

		QString d = QString::number(i);
		QString t = QString::number(i * i);
		m_kerenl_size_cb->addItem(d + "x" + d + " (" + t + " elements)");
	}
	//kernel dim = 3 + index * 2
	m_kerenl_size_cb->resize(175, 20);
	m_kerenl_size_cb->move(140, 85);

	connect(m_kerenl_size_cb, &QComboBox::activated, this, &BilateralFilterDialog::onActivation_ks);

	this->show();
}

void BilateralFilterDialog::onActionTriggered_sigma(int action) {
	float sigma = m_sigma_slider->sliderPosition() / 10.0;
	m_sigma_le->setValue(sigma);
	m_bf.setSigma(sigma);
	startTimer();
}

void BilateralFilterDialog::onActionTriggered_sigmaIntensity(int action) {
	float sigmaR = m_sigma_intensity_slider->sliderPosition() / 10.00;
	m_sigma_intensity_le->setValue(sigmaR);
	m_bf.setSigmaRange(sigmaR);
	startTimer();
}

void BilateralFilterDialog::AddSigmaInputs() {

	m_sigma_le = new DoubleLineEdit(new DoubleValidator(0.10, 24.0, 2), this);
	m_sigma_le->setValue(m_bf.Sigma());
	m_sigma_le->setFixedWidth(50);
	m_sigma_le->move(140, 10);
	m_sigma_le->addLabel(new QLabel("StdDev Spatial:   ", this));

	m_sigma_slider = new QSlider(Qt::Horizontal, this);
	m_sigma_slider->setRange(1, 240);
	m_sigma_slider->setFixedWidth(240);
	m_sigma_slider->setValue(m_bf.Sigma() * 10);
	m_sigma_le->addSlider(m_sigma_slider);

	connect(m_sigma_slider, &QSlider::sliderMoved, this, &BilateralFilterDialog::onActionTriggered_sigma);
}

void BilateralFilterDialog::AddSigmaIntensityInputs() {

	m_sigma_intensity_le = new DoubleLineEdit(new DoubleValidator(0.10, 24.0, 2), this);
	m_sigma_intensity_le->setValue(m_bf.SigmaRange());
	m_sigma_intensity_le->setFixedWidth(50);
	m_sigma_intensity_le->move(140, 45);
	m_sigma_intensity_le->addLabel(new QLabel("StdDev Intensity:   ", this));

	m_sigma_intensity_slider = new QSlider(Qt::Horizontal, this);
	m_sigma_intensity_slider->move(205, 50);
	m_sigma_intensity_slider->setRange(1, 240);
	m_sigma_intensity_slider->setFixedWidth(240);
	m_sigma_intensity_slider->setValue(m_bf.SigmaRange() * 10);
	connect(m_sigma_intensity_slider, &QSlider::sliderMoved, this, &BilateralFilterDialog::onActionTriggered_sigmaIntensity);
}

void BilateralFilterDialog::onActivation_ks(int index) {
	m_bf.setKernelSize(3 + 2 * index);
	ApplytoPreview();
}


void BilateralFilterDialog::resetDialog() {

	m_bf = BilateralFilter();

	m_sigma_le->setValue(m_bf.Sigma());
	m_sigma_slider->setSliderPosition(m_bf.Sigma() * 10);

	m_sigma_intensity_le->setValue(m_bf.SigmaRange());
	m_sigma_intensity_slider->setSliderPosition(m_bf.SigmaRange() * 10);

	m_kerenl_size_cb->setCurrentIndex(0);
}

void BilateralFilterDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void BilateralFilterDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	std::unique_ptr<ProgressDialog> pd(std::make_unique<ProgressDialog>(m_bf.progressSignal()));

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	//m_bf.setSigma(m_sigma_le->text().toFloat());
	m_bf.setKernelSize(3 + 2 * m_kerenl_size_cb->currentIndex());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_bf, &BilateralFilter::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_bf, &BilateralFilter::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_bf, &BilateralFilter::Apply);
		break;
	}
	}

	setEnabledAll(true);

	ApplytoPreview();
}

void BilateralFilterDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	//m_bf.setSigma(m_sigma_le->text().toFloat() / iwptr->IdealZoomFactor());
	m_bf.setKernelSize((3 + 2 * m_kerenl_size_cb->currentIndex())/ iwptr->IdealZoomFactor());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		return iw8->UpdatePreview(m_bf, &BilateralFilter::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		return iw16->UpdatePreview(m_bf, &BilateralFilter::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		return iw32->UpdatePreview(m_bf, &BilateralFilter::Apply);
	}
	}
}