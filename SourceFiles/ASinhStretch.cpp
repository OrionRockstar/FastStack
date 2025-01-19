#include "pch.h"
#include "FastStack.h"
#include "ASinhStretch.h"
#include "Histogram.h"

template<typename T>
void ASinhStretch::computeBlackpoint(const Image<T>& img) {
	Histogram histogram;
	histogram.constructHistogram(img);

	int sum = 0;
	int i = 0;

	while (sum < img.totalPxCount() * .02)
		sum += histogram[i++];

	m_blackpoint = float(i + 1) / (histogram.resolution() - 1);
}
template void ASinhStretch::computeBlackpoint(const Image8&);
template void ASinhStretch::computeBlackpoint(const Image16&);
template void ASinhStretch::computeBlackpoint(const Image32&);

float ASinhStretch::computeBeta() {

	float low = 0,
		high = 10000,
		mid = 5000;

	for (int i = 0; i < 20; i++){
		mid = (low + high) / 2;
		float multiplier_mid = mid / asinh(mid);
		(m_stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}

	return mid;
}

template<typename T, typename P>
static void testThread(int num_threads, Image<T>& img, P&& func) {

	std::vector<std::thread> threads(num_threads);
	int d = (img.end() - img.begin()) / num_threads;
	int r = (img.end() - img.begin()) % num_threads;

	for (int i = 0; i < num_threads; ++i)
		threads[i] = std::thread(func, img.begin() + i * d, img.begin() + (i + 1) * d);

	for (auto& t : threads)
		t.join();
}

static void testThread(int num_threads, int iterations, std::function<void (int,int)> func) {

	std::vector<std::thread> threads(num_threads);

	int step = iterations / num_threads;

	for (int i = 0; i < num_threads; ++i) {
		int r = (i + 1 == num_threads) ? iterations % num_threads : 0;
		threads[i] = std::thread(func, i * step, (i + 1) * step + r);
	}

	for (auto& t : threads)
		t.join();
}

template<typename T>
void ASinhStretch::applyMono(Image<T>& img) {

	float beta = computeBeta();
	float asinhb = asinh(beta);

	float max = img.computeMax(0);

#pragma omp parallel for num_threads(2)
	for (int el = 0; el < img.pxCount(); ++el) {
		float r = (Pixel<float>::toType(img[el]) - blackpoint()) / (max - blackpoint());
		img[el] = Pixel<T>::toType(math::clip((r != 0) ? asinh(r * beta) / asinhb : beta / asinhb));
	}
}
template void ASinhStretch::applyMono(Image8&);
template void ASinhStretch::applyMono(Image16&);
template void ASinhStretch::applyMono(Image32&);

template<typename T>
void ASinhStretch::applyRGB(Image<T>& img) {

	float beta = computeBeta();
	float asinhb = asinh(beta);

	float max = 0;
	for (int i = 0; i < 3; ++i)
		max = math::max(max, Pixel<float>::toType(img.computeMax(i)));

	auto rescale = [max, this](float pixel) { return (pixel - blackpoint()) / (max - blackpoint()); };

	std::array<float, 3> color_space = { 0.333333f, 0.333333f, 0.333333f };
	if (m_srbg) color_space = { 0.222491f, 0.716888f, 0.060621f };

#pragma omp parallel for num_threads(2)
	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			auto color = img.color<float>(x,y);

			float I = rescale(color_space[0] * color.red() + color_space[1] * color.green() + color_space[2] * color.blue());
			float k = (I != 0) ? asinh(beta * I) / (I * asinhb) : beta / asinhb;

			color.setRed(math::clip(rescale(color.red()) * k));
			color.setGreen(math::clip(rescale(color.green()) * k));
			color.setBlue(math::clip(rescale(color.blue()) * k));

			img.setColor<>(x, y, color);
		}
	}
}
template void ASinhStretch::applyRGB(Image8&);
template void ASinhStretch::applyRGB(Image16&);
template void ASinhStretch::applyRGB(Image32&);

template<typename Image>
void ASinhStretch::apply(Image& img) {

	if (img.channels() == 1)
		return applyMono(img);
	else if (img.channels() == 3)
		return applyRGB(img);
}
template void ASinhStretch::apply(Image8&);
template void ASinhStretch::apply(Image16&);
template void ASinhStretch::apply(Image32&);







using ASSD = ASinhStretchDialog;

ASinhStretchDialog::ASinhStretchDialog(QWidget* parent) : ProcessDialog("ASinhStretch",QSize(500,170),FastStack::recast(parent)->workspace()) {

	setTimer(250, this, &ASinhStretchDialog::applytoPreview);

	connectToolbar(this, &ASSD::apply, &ASSD::showPreview, &ASSD::resetDialog);
	connectZoomWindow(this, &ASSD::applytoPreview);

	m_bp_comp = new PushButton("Compute Blackpoint", this);
	m_bp_comp->move(62, 105);
	m_bp_comp->setDisabled(true);
	connect(m_bp_comp, &QPushButton::released, this, [this]() { computeBlackpoint(); applytoPreview(); });


	addStretchFactorInputs();
	addBlackpointInputs();
	addFinetuneInputs();

	m_rgb_cb = new CheckBox("Use sRGB", this);
	m_rgb_cb->setChecked(true);
	m_rgb_cb->move(300, 107);
	connect(m_rgb_cb, &QCheckBox::clicked, this, [this](bool v) { m_ash.setsRGB(v); applytoPreview(); });

	this->show();
}

void ASinhStretchDialog::addStretchFactorInputs() {

	m_sf_le = new DoubleLineEdit(1.00, new DoubleValidator(1.00, 375, 2), 4, this);
	m_sf_le->setFixedWidth(75);
	m_sf_le->setMaxLength(4);
	m_sf_le->move(135, 10);
	m_sf_le->addLabel(new QLabel("Stretch Factor:   ", this));

	m_sf_slider = new Slider(Qt::Horizontal, this);
	m_sf_slider->setRange(0, 250);
	m_sf_slider->setFixedWidth(250);
	m_sf_le->addSlider(m_sf_slider);

	auto action = [this](int) {
		float sf = pow(1.024, m_sf_slider->sliderPosition());

		m_sf_le->setValue(sf);
		m_ash.setStretchFactor(sf);

		startTimer();
	};

	auto edited = [this]() {
		float sf = m_sf_le->valuef();

		int new_pos = (log10(sf) / log10(1.024)) + 0.5;
		m_sf_slider->setValue(new_pos);

		m_ash.setStretchFactor(sf);

		applytoPreview();
	};

	connect(m_sf_slider, &QSlider::actionTriggered, this, action);
	connect(m_sf_le, &DoubleLineEdit::editingFinished, this, edited);
}

void ASinhStretchDialog::addBlackpointInputs() {

	m_bp_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 0.2, 6), this);
	m_bp_le->setFixedWidth(75);
	m_bp_le->move(135, 45);
	m_bp_le->setMaxLength(8);
	m_bp_le->addLabel(new QLabel("Blackpoint:   ", this));

	m_bp_slider = new Slider(Qt::Horizontal, this);
	m_bp_slider->setRange(0, 200);
	m_bp_slider->setFixedWidth(250);
	m_bp_le->addSlider(m_bp_slider);

	auto action = [this](int) {
		float bp = m_bp_slider->sliderPosition() / 1000.0;
		m_bp_le->setValue(bp);
		m_ash.setBlackpoint(bp);
		startTimer();
	};

	auto edited = [this]() {
		float bp = m_bp_le->text().toFloat();
		m_ash.setBlackpoint(bp);
		m_bp_slider->setValue(bp * 1000);
		applytoPreview();
	};

	connect(m_bp_slider, &QSlider::actionTriggered, this, action);
	connect(m_bp_le, &DoubleLineEdit::editingFinished, this, edited);
}

void ASinhStretchDialog::addFinetuneInputs() {

	m_fine_tune = new Slider(Qt::Horizontal, this);
	m_fine_tune->move(135, 80);
	m_fine_tune->setFixedWidth(340);
	m_fine_tune->setRange(-500, 500);
	m_fine_tune->setValue(0);
	m_fine_tune->setPageStep(0);
	m_fine_tune->setSingleStep(0);

	auto moved = [this](int value) {
		value += (value - m_fine_tune->value());

		value = math::clip(value, m_fine_tune->minimum(), m_fine_tune->maximum());

		double new_bp = m_current_bp + value / 1'000'000.0;

		auto vp = m_bp_le->doubleValidator();

		if (new_bp < vp->bottom())
			new_bp = vp->bottom();

		if (new_bp > vp->top())
			new_bp = vp->top();

		m_ash.setBlackpoint(new_bp);

		m_bp_le->setValue(new_bp);
		m_bp_slider->setValue(new_bp * 1000);

		startTimer();
	};

	connect(m_fine_tune, &QSlider::sliderPressed, this, [this]() { m_current_bp = m_bp_le->valuef(); });
	connect(m_fine_tune, &QSlider::sliderMoved, this, moved);
	connect(m_fine_tune, &QSlider::sliderReleased, this, [this]() { m_fine_tune->setValue(0); });
}


void ASinhStretchDialog::computeBlackpoint() {
	if (m_workspace->subWindowList().size() == 0 || m_preview == nullptr)
		return;
	
	auto iwptr = previewRecast(m_preview)->parentWindow();

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		m_ash.computeBlackpoint(iwptr->source());
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_ash.computeBlackpoint(iw16->source());
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_ash.computeBlackpoint(iw32->source());
		break;
	}
	}

	m_bp_slider->setValue(m_ash.blackpoint() * 1'000);
	m_bp_le->setValue(m_ash.blackpoint());
}

void ASinhStretchDialog::resetDialog() {
	m_ash = ASinhStretch();

	m_sf_le->setText("1.00");
	m_sf_slider->setSliderPosition(0);

	m_bp_le->setText("0.000000");
	m_bp_slider->setSliderPosition(0);

	m_rgb_cb->setChecked(true);

	applytoPreview();
}

void ASinhStretchDialog::showPreview() {

	ProcessDialog::showPreview();

	if (m_preview != nullptr) {
		m_bp_comp->setEnabled(true);
		connect(previewRecast(m_preview)->windowSignals(), &WindowSignals::windowClosed, this, [this]() {m_bp_comp->setEnabled(false); });
	}

	applytoPreview();
}

void ASinhStretchDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	ImageWindow8* iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_ash, &ASinhStretch::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_ash, &ASinhStretch::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_ash, &ASinhStretch::apply);
		break;
	}
	}

	applytoPreview();
}

void ASinhStretchDialog::applytoPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* iwptr = previewRecast(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_ash, &ASinhStretch::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_ash, &ASinhStretch::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_ash, &ASinhStretch::apply);
	}
	}
}