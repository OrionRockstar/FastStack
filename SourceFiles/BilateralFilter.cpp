#include "pch.h"
#include "FastStack.h"
#include "BilateralFilter.h"

template<typename T>
void BilateralFilter::apply(Image<T>& img) {

	int radius = (m_kernel_dim - 1) / 2;

	Image<T> temp(img.rows(), img.cols(), img.channels());

	float k_s = 1 / (2 * m_sigma_s * m_sigma_s);
	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);

	FloatKernel gaussian(m_kernel_dim);

	for (int j = -radius; j <= radius; ++j)
		for (int i = -radius; i <= radius; ++i)
			gaussian(i + radius, j + radius) = -k_s * ((j * j) + (i * i));

	int sum = 0;
	FloatKernel kernel(m_kernel_dim);

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(img, { 0,y,ch });

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(img, { x,y,ch });

				float sum = 0.0f;
				float weight = 0.0f;
				float pixel = Pixel<float>::toType(img(x, y, ch));

				for (int el = 0; el < kernel.count(); ++el) {
					float d = pixel - kernel[el];
					float d2 = d * d;
					float w = exp(gaussian[el] - (k_r * (d2 + d2)));
					weight += w;
					sum += w * kernel[el];
				}

				temp(x, y, ch) = Pixel<T>::toType(sum / weight);

			}
//#pragma omp atomic
			//++sum;

			//if (omp_get_thread_num() == 0)
				//m_ps->emitProgress((sum * 100) / (img.rows() * img.channels()));
		}
	}

	//m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void BilateralFilter::apply(Image8&);
template void BilateralFilter::apply(Image16&);
template void BilateralFilter::apply(Image32&);

template<typename T>
void BilateralFilter::applyTo(const Image<T>& src, Image<T>& dst, int factor) {

	factor = math::max(factor, 1);

	int radius = (m_kernel_dim - 1) / 2;

	if (dst.rows() != src.rows() / factor || dst.cols() != src.cols() / factor || dst.channels() != src.channels())
		dst = Image<T>(src.rows() / factor, src.cols() / factor, src.channels());

	float k_s = 1 / (2 * m_sigma_s * m_sigma_s);
	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);

	FloatKernel gaussian(m_kernel_dim);

	for (int j = -radius; j <= radius; ++j)
		for (int i = -radius; i <= radius; ++i)
			gaussian(i + radius, j + radius) = -k_s * ((j * j) + (i * i));

	int sum = 0;
	FloatKernel kernel(m_kernel_dim);

	for (uint32_t ch = 0; ch < dst.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel), num_threads(2)
		for (int y = 0; y < dst.rows(); ++y) {
			int y_s = y * factor;

			for (int x = 0; x < dst.cols(); ++x) {
				int x_s = x * factor;

				kernel.populate(src, { x_s,y_s,ch });

				float sum = 0.0f;
				float weight = 0.0f;
				float pixel = Pixel<float>::toType(src(x_s, y_s, ch));

				for (int el = 0; el < kernel.count(); ++el) {
					float d = pixel - kernel[el];
					float d2 = d * d;
					float w = exp(gaussian[el] - (k_r * (d2 + d2)));
					weight += w;
					sum += w * kernel[el];
				}

				dst(x, y, ch) = Pixel<T>::toType(sum / weight);
			}
		}
	}
}
template void BilateralFilter::applyTo(const Image8&, Image8&, int);
template void BilateralFilter::applyTo(const Image16&, Image16&, int);
template void BilateralFilter::applyTo(const Image32&, Image32&, int);







BilateralFilterDialog::BilateralFilterDialog(QWidget* parent) : ProcessDialog("BilateralFilter", QSize(470, 165), FastStack::recast(parent)->workspace()) {

	setTimer(750, this, &BilateralFilterDialog::applytoPreview);

	connectToolbar(this, &BilateralFilterDialog::apply, &BilateralFilterDialog::showPreview, &BilateralFilterDialog::resetDialog);	
	connectZoomWindow(this, &BilateralFilterDialog::applytoPreview);

	addSigmaInputs();
	addSigmaIntensityInputs();
	addKernelSizeInputs();

	this->show();
}

void BilateralFilterDialog::addSigmaInputs() {

	m_sigma_s_le = new DoubleLineEdit(new DoubleValidator(0.10, 20.0, 2), this);
	m_sigma_s_le->setValue(m_bf.sigmaSpatial());
	m_sigma_s_le->setFixedWidth(50);
	m_sigma_s_le->move(140, 15);
	m_sigma_s_le->addLabel(new QLabel("StdDev Spatial:   ", this));

	m_sigma_s_slider = new Slider(Qt::Horizontal, this);
	m_sigma_s_slider->setRange(1, 200);
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

	m_sigma_r_le = new DoubleLineEdit(new DoubleValidator(0.01, 1.0, 2), this);
	m_sigma_r_le->setValue(m_bf.sigmaRange());
	m_sigma_r_le->setFixedWidth(50);
	m_sigma_r_le->move(140, 55);
	m_sigma_r_le->addLabel(new QLabel("StdDev Range:   ", this));

	m_sigma_r_slider = new Slider(Qt::Horizontal, this);
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
	m_kernel_size_cb = new ComboBox(this);

	for (int i = 3; i <= 15; i += 2) {

		QString d = QString::number(i);
		QString t = QString::number(i * i);
		m_kernel_size_cb->addItem(d + "x" + d + " (" + t + " elements)");
	}

	m_kernel_size_cb->move(180, 100);
	m_kernel_size_cb->addLabel(new QLabel("Kernel Size:   ", this));

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
	m_sigma_r_slider->setSliderPosition(m_bf.sigmaRange() * 10);

	m_kernel_size_cb->setCurrentIndex(0);
}

void BilateralFilterDialog::showPreview() {
	ProcessDialog::showPreview();
	applytoPreview();
}

void BilateralFilterDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	//std::unique_ptr<ProgressDialog> pd(std::make_unique<ProgressDialog>(m_bf.progressSignal()));

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_bf, &BilateralFilter::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->applyToSource(m_bf, &BilateralFilter::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->applyToSource(m_bf, &BilateralFilter::apply);
		break;
	}
	}

	setEnabledAll(true);

	applytoPreview();
}

void BilateralFilterDialog::applytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast(m_preview);

	auto zw = [&]<typename T>(PreviewWindow<T>*pw) {

		for (auto sw : m_workspace->subWindowList()) {
			auto iw = imageRecast<T>(sw->widget());
			if (iw->preview() == m_preview)
				if (iw->zoomWindow())
					return pw->updatePreview(m_bf, &BilateralFilter::apply);
		}
		pw->updatePreview(m_bf, &BilateralFilter::applyTo);
	};

	switch (iwptr->type()) {

	case ImageType::UBYTE: 
		return zw(iwptr);
	
	case ImageType::USHORT: 
		return zw(previewRecast<uint16_t>(iwptr));
	
	case ImageType::FLOAT: 
		return zw(previewRecast<float>(iwptr));
	}
}