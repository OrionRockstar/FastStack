#include "pch.h"
#include "GaussianFilter.h"
#include "FastStack.h"

GaussianFilter::GaussianFilter(float sigma) : m_sigma(sigma) {
	int k_rad = (3.0348 * m_sigma) + 0.5;
	m_kernel_dim = 2 * k_rad + 1;
}

void GaussianFilter::setSigma(float sigma) {
	m_sigma = sigma;
	int k_rad = (3.0348 * m_sigma) + 0.5;
	m_kernel_dim = 2 * k_rad + 1;
}

void GaussianFilter::setKernelDimension(int kernel_dimension) {
	m_kernel_dim = kernel_dimension;
	int k_rad = (m_kernel_dim - 1) / 2;
	m_sigma = k_rad / 3.0348;
}

void GaussianFilter::BuildGaussianKernel() {

	int k_rad = (m_kernel_dim - 1) / 2;

	gaussian_kernel = std::vector<float>(m_kernel_dim);

	float s = 2 * m_sigma * m_sigma;
	float k1 = 1 / sqrtf(M_PI * s), k2 = 1 / s;
	float g_sum = 0;

	for (int j = -k_rad; j <= k_rad; ++j)
		g_sum += gaussian_kernel[j + k_rad] = k1 * expf(-k2 * (j * j));

	for (auto& val : gaussian_kernel)
		val /= g_sum;
}

template<typename T>
void GaussianFilter::Apply(Image<T>& img) {

	Image<T> temp(img.Rows(), img.Cols(), img.Channels());

	BuildGaussianKernel();

	for (int ch = 0; ch < img.Channels(); ++ch) {

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			PixelWindow<T> window(m_kernel_dim);
			window.PopulateRowWindow(img, y, ch);
			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					window.UpdateRowWindow(img, x, y, ch);

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				temp(x, y, ch) = sum;

			}
		}

#pragma omp parallel for 
		for (int x = 0; x < img.Cols(); ++x) {
			PixelWindow<T> window(m_kernel_dim);
			window.PopulateColWindow(temp, x, ch);
			for (int y = 0; y < img.Rows(); ++y) {

				if (y != 0)
					window.UpdateColWindow(temp, x, y, ch);

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				img(x, y, ch) = sum;
			}
		}
	}
}
template void GaussianFilter::Apply(Image8&);
template void GaussianFilter::Apply(Image16&);
template void GaussianFilter::Apply(Image32&);









GaussianFilterDialog::GaussianFilterDialog(QWidget* parent) : ProcessDialog("GaussianFilter", QSize(420, 75), *reinterpret_cast<FastStack*>(parent)->workspace(), parent) {
	
	setTimer(250, this, &GaussianFilterDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &GaussianFilterDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &GaussianFilterDialog::Apply, &GaussianFilterDialog::showPreview, &GaussianFilterDialog::resetDialog);

	m_sigma_le = new DoubleLineEdit(new DoubleValidator(0.10, 24.0, 2), this);
	m_sigma_le->setValue(2.0);
	m_sigma_le->setFixedWidth(50);
	m_sigma_le->move(80, 10);
	m_sigma_le->addLabel(new QLabel("StdDev:   ", this));

	m_sigma_slider = new QSlider(Qt::Horizontal, this);
	m_sigma_slider->setRange(1, 240);
	m_sigma_slider->setFixedWidth(240);
	m_sigma_slider->setValue(20);
	m_sigma_le->addSlider(m_sigma_slider);
	connect(m_sigma_slider, &QSlider::actionTriggered, this, &GaussianFilterDialog::onActionTriggered_sigma);

	this->show();
}

void GaussianFilterDialog::onActionTriggered_sigma(int action) {
	float sigma = m_sigma_slider->sliderPosition() / 10.0;
	m_sigma_le->setValue(sigma);
	m_gf.setSigma(sigma);
	startTimer();
}

void GaussianFilterDialog::resetDialog() {
	m_gf = GaussianFilter();
	m_sigma_le->setValue(2.0);
	m_sigma_slider->setValue(20);
}

void GaussianFilterDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void GaussianFilterDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	m_gf.setSigma(m_sigma_le->text().toFloat());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_gf, &GaussianFilter::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_gf, &GaussianFilter::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_gf, &GaussianFilter::Apply);
		break;
	}
	}

	setEnabledAll(true);

	ApplytoPreview();
}

void GaussianFilterDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	m_gf.setSigma(m_sigma_le->text().toFloat() / iwptr->IdealZoomFactor());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		return iw8->UpdatePreview(m_gf, &GaussianFilter::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		return iw16->UpdatePreview(m_gf, &GaussianFilter::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		return iw32->UpdatePreview(m_gf, &GaussianFilter::Apply);
	}
	}
}