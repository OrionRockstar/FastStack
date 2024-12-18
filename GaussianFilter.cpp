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

	m_gaussian_kernel = std::vector<float>(m_kernel_dim);

	float s = 2 * m_sigma * m_sigma;
	float k1 = 1 / sqrtf(std::_Pi * s), k2 = 1 / s;
	float g_sum = 0;

	for (int j = -k_rad; j <= k_rad; ++j)
		g_sum += m_gaussian_kernel[j + k_rad] = k1 * expf(-k2 * (j * j));

	for (auto& val : m_gaussian_kernel)
		val /= g_sum;
}

template<typename T>
void GaussianFilter::Apply(Image<T>& img) {

	if (m_sigma == 0.0f)
		return;

	Image32 temp(img.rows(), img.cols());

	BuildGaussianKernel();

	PixelWindow window(m_kernel_dim);

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {

#pragma omp parallel for firstprivate(window)
		for (int y = 0; y < img.rows(); ++y) {
			window.populateRowWindow(img, { 0,y,ch });
			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					window.updateRowWindow(img, { x,y,ch });

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * m_gaussian_kernel[j];

				temp(x, y) = sum;

			}
		}

#pragma omp parallel for firstprivate(window)
		for (int x = 0; x < img.cols(); ++x) {
			window.populateColWindow(temp, { x,0,0 });
			for (int y = 0; y < img.rows(); ++y) {

				if (y != 0)
					window.updateColWindow(temp, { x,y,0 });

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * m_gaussian_kernel[j];

				img(x, y, ch) = Pixel<T>::toType(sum);
			}
		}
	}

	img.normalize();
}
template void GaussianFilter::Apply(Image8&);
template void GaussianFilter::Apply(Image16&);
template void GaussianFilter::Apply(Image32&);








GaussianFilterDialog::GaussianFilterDialog(QWidget* parent) : ProcessDialog("GaussianFilter", QSize(405, 105), FastStack::recast(parent)->workspace()) {
	
	setTimer(250, this, &GaussianFilterDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &GaussianFilterDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &GaussianFilterDialog::Apply, &GaussianFilterDialog::showPreview, &GaussianFilterDialog::resetDialog);

	addSigmaInputs();

	m_ks_label = new QLabel(this);
	m_ks_label->move(135, 50);
	m_ks_label->setFixedWidth(150);
	onValueChanged(2.0);

	this->show();
}

void GaussianFilterDialog::addSigmaInputs() {

	m_sigma_le = new DoubleLineEdit(new DoubleValidator(0.0, 24.0, 2), this);
	m_sigma_le->setValue(2.0);
	m_sigma_le->setFixedWidth(50);
	m_sigma_le->move(80, 15);
	m_sigma_le->addLabel(new QLabel("StdDev:   ", this));

	m_sigma_slider = new Slider(Qt::Horizontal, this);
	m_sigma_slider->setRange(0, 240);
	m_sigma_slider->setFixedWidth(250);
	m_sigma_slider->setValue(20);
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
}

void GaussianFilterDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void GaussianFilterDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	m_gf.setSigma(m_sigma_le->text().toFloat());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_gf, &GaussianFilter::Apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_gf, &GaussianFilter::Apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_gf, &GaussianFilter::Apply);
		break;
	}
	}

	setEnabledAll(true);

	ApplytoPreview();
}

void GaussianFilterDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast(m_preview);

	m_gf.setSigma(m_sigma_le->text().toFloat() /  iwptr->parentWindow()->computeZoomFactor()) ;

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_gf, &GaussianFilter::Apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_gf, &GaussianFilter::Apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_gf, &GaussianFilter::Apply);
	}
	}
}