#include "pch.h"
#include "FastStack.h"
#include "ASinhStretch.h"
#include "Histogram.h"

float ASinhStretch::StretchFactor()const {
	return m_stretch_factor;
}

void ASinhStretch::setStretchFactor(float stretch_factor) {
	m_stretch_factor = stretch_factor;
}

float ASinhStretch::Blackpoint()const { 
	return m_blackpoint; 
}

void ASinhStretch::setBlackpoint(float blackpoint) {
	m_blackpoint = blackpoint;
}

template<typename T>
void ASinhStretch::ComputeBlackpoint(Image<T>& img) {
	Histogram histogram;
	histogram.ConstructHistogram(img.cbegin(), img.cend());

	int sum = 0;
	int i = 0;

	while (sum < img.TotalPxCount() * .02)
		sum += histogram[i++];

	m_blackpoint = (i + 1) / ((img.is_uint8()) ? 255.0 : 65535.0);

}
template void ASinhStretch::ComputeBlackpoint(Image8&);
template void ASinhStretch::ComputeBlackpoint(Image16&);
template void ASinhStretch::ComputeBlackpoint(Image32&);

float ASinhStretch::ComputeBeta() {
	float low = 0,
		high = 10000,
		mid = 5000;

	for (int i = 0; i < 20; i++)
	{
		mid = (low + high) / 2;
		float multiplier_mid = mid / asinh(mid);
		(m_stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}
	return mid;
}

template<typename T>
void ASinhStretch::ApplyMono(Image<T>& img) {

	float beta = ComputeBeta();
	float asinhb = asinh(beta);

	T max = img.ComputeMax(0);
	T bp = Pixel<T>::toType(m_blackpoint);

	for (T& pixel : img) {
		float r = float(pixel - bp) / (max - bp);
		pixel = Pixel<T>::toType(Clip((r != 0) ? asinh(r * beta) / asinhb : beta / asinhb));
	}

}
template void ASinhStretch::ApplyMono(Image8&);
template void ASinhStretch::ApplyMono(Image16&);
template void ASinhStretch::ApplyMono(Image32&);

template<typename T>
void ASinhStretch::ApplyRGB(Image<T>& img) {

	float beta = ComputeBeta();
	float asinhb = asinh(beta);

	T max = 0;
	for (int i = 0; i < 3; ++i)
		max = Max(max, img.ComputeMax(i));

	T blackpoint = Pixel<T>::toType(m_blackpoint);

	auto Rescale = [max, blackpoint](T pixel) { return float(pixel - blackpoint) / (max - blackpoint); };

	std::array<float, 3> color = { 0.333333f, 0.333333f, 0.333333f };
	if (m_srbg) color = { 0.222491f, 0.716888f, 0.060621f };

	for (int el = 0; el < img.PxCount(); ++el) {

		T R, G, B;
		img.getRGB(el, R, G, B);

		float I = Rescale(color[0] * R + color[1] * G + color[2] * B);
		float k = (I != 0) ? asinh(beta * I) / (I * asinhb) : beta / asinhb;

		R = Pixel<T>::toType(Clip(Rescale(R) * k));
		G = Pixel<T>::toType(Clip(Rescale(G) * k));
		B = Pixel<T>::toType(Clip(Rescale(B) * k));

		img.setRGB(el, R, G, B);
	}
}
template void ASinhStretch::ApplyRGB(Image8&);
template void ASinhStretch::ApplyRGB(Image16&);
template void ASinhStretch::ApplyRGB(Image32&);

template<typename Image>
void ASinhStretch::Apply(Image& img) {
	if (img.Channels() == 1)
		return ApplyMono(img);
	else if (img.Channels() == 3)
		return ApplyRGB(img);
}
template void ASinhStretch::Apply(Image8&);
template void ASinhStretch::Apply(Image16&);
template void ASinhStretch::Apply(Image32&);







using ASSD = ASinhStretchDialog;

ASinhStretchDialog::ASinhStretchDialog(QWidget* parent): ProcessDialog("ASinhStretch",QSize(500,170),*reinterpret_cast<FastStack*>(parent)->m_workspace, parent) {

	setTimer(250, this, &ASinhStretchDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &ASSD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ASSD::Apply, &ASSD::showPreview, &ASSD::resetDialog);

	QPushButton* bp_comp = new QPushButton(this);
	bp_comp->setText("Compute Blackpoint");
	bp_comp->move(62, 105);
	bp_comp->setAutoDefault(false);
	connect(bp_comp, &QPushButton::pressed, this, &ASSD::onPressed_blackpoint);


	AddStretchFactorInputs();
	AddBlackpointInputs();
	AddFinetuneInputs();

	m_rgb_cb = new QCheckBox("Use sRGB", this);
	m_rgb_cb->setChecked(true);
	m_rgb_cb->move(300, 107);
	connect(m_rgb_cb, &QCheckBox::clicked, this, &ASSD::onClick_srgb);

	this->show();
}

void ASinhStretchDialog::editingFinished_sf() {

	float sf = m_sf_le->text().toFloat();
	m_ash.setStretchFactor(sf);

	int new_pos = (log10(sf) / log10(1.024)) + 0.5;
	m_sf_slider->setSliderPosition(new_pos);

	ApplytoPreview();
}

void ASinhStretchDialog::actionSlider_sf(int action) {
	float sf = pow(1.024, m_sf_slider->sliderPosition());
	m_sf_le->setValue(sf);
	m_ash.setStretchFactor(sf);
	startTimer();
}

void ASinhStretchDialog::editingFinished_bp() {
	float bp = m_bp_le->text().toFloat();
	m_ash.setBlackpoint(bp);

	m_bp_slider->setSliderPosition(bp * 1000);
	ApplytoPreview();
}

void ASinhStretchDialog::actionSlider_bp(int action) {
	m_bp_le->setValue(m_bp_slider->sliderPosition() / 1000.0);
	startTimer();
}

void ASinhStretchDialog::sliderPressed_ft() {
	m_current_bp = m_bp_le->text().toFloat();
}

void ASinhStretchDialog::sliderMoved_ft(int value) {

	value += (value - m_fine_tune->value());

	value = Clip( value, m_fine_tune->minimum(), m_fine_tune->maximum());

	double new_bp = m_current_bp + value / 1'000'000.0;

	auto vp = m_bp_le->Validator();

	if (new_bp < vp->bottom())
		new_bp = vp->bottom();

	if (new_bp > vp->top())
		new_bp = vp->top();

	m_ash.setBlackpoint(new_bp);
	
	m_bp_le->setValue(new_bp);
	m_bp_slider->setValue(new_bp * 1000);

	startTimer();
}

void ASinhStretchDialog::sliderReleased_ft() {
	m_fine_tune->setValue(0);
}

void ASinhStretchDialog::onClick_srgb(bool val) {
	m_ash.setsRGB(val);
	ApplytoPreview();
}

void ASinhStretchDialog::onPressed_blackpoint() {
	computeBlackpoint();
	ApplytoPreview();
}


void ASinhStretchDialog::AddStretchFactorInputs() {

	m_sf_le = new DoubleLineEdit("1.00", new DoubleValidator(1.00, 375, 2), 4, this);
	m_sf_le->setFixedWidth(75);
	m_sf_le->setMaxLength(4);
	m_sf_le->move(135, 10);
	m_sf_le->addLabel(new QLabel("Stretch Factor:   ", this));

	m_sf_slider = new QSlider(Qt::Horizontal, this);
	m_sf_slider->setRange(0, 250);
	m_sf_slider->setFixedWidth(250);
	m_sf_le->addSlider(m_sf_slider);

	connect(m_sf_le, &DoubleLineEdit::editingFinished, this, &ASinhStretchDialog::editingFinished_sf);
	connect(m_sf_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_sf);
}

void ASinhStretchDialog::AddBlackpointInputs() {

	m_bp_le = new DoubleLineEdit("0.000000", new DoubleValidator(0.0, 0.2, 6), this);
	m_bp_le->setFixedWidth(75);
	m_bp_le->move(135, 45);
	m_bp_le->setMaxLength(8);
	m_bp_le->addLabel(new QLabel("Blackpoint:   ", this));

	m_bp_slider = new QSlider(Qt::Horizontal, this);
	m_bp_slider->setRange(0, 200);
	m_bp_slider->setFixedWidth(250);
	m_bp_le->addSlider(m_bp_slider);

	connect(m_bp_le, &DoubleLineEdit::editingFinished, this, &ASinhStretchDialog::editingFinished_bp);
	connect(m_bp_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_bp);
}

void ASinhStretchDialog::AddFinetuneInputs() {

	m_fine_tune = new QSlider(Qt::Horizontal, this);
	m_fine_tune->move(135, 80);
	m_fine_tune->setFixedWidth(340);
	m_fine_tune->setRange(-500, 500);
	m_fine_tune->setValue(0);
	m_fine_tune->setPageStep(0);
	m_fine_tune->setSingleStep(0);

	connect(m_fine_tune, &QSlider::sliderPressed, this, &ASinhStretchDialog::sliderPressed_ft);
	connect(m_fine_tune, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_ft);
	connect(m_fine_tune, &QSlider::sliderReleased, this, &ASinhStretchDialog::sliderReleased_ft);
}

void ASinhStretchDialog::computeBlackpoint() {
	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		m_ash.ComputeBlackpoint(iwptr->Source());
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_ash.ComputeBlackpoint(iw16->Source());
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_ash.ComputeBlackpoint(iw32->Source());
		break;
	}
	}

	m_bp_le->setText(QString::number(m_ash.Blackpoint()));
	editingFinished_bp();
}

void ASinhStretchDialog::resetDialog() {
	m_sf_le->setText("1.00");
	m_sf_slider->setSliderPosition(0);

	m_bp_le->setText("0.000000");
	m_bp_slider->setSliderPosition(0);

	m_rgb_cb->setChecked(true);

	ApplytoPreview();
}

void ASinhStretchDialog::showPreview() {

	ProcessDialog::showPreview();
	ApplytoPreview();
}

void ASinhStretchDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_ash, &ASinhStretch::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_ash, &ASinhStretch::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_ash, &ASinhStretch::Apply);
		break;
	}
	}

	ApplytoPreview();
}

void ASinhStretchDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		return iw8->UpdatePreview(m_ash, &ASinhStretch::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		return iw16->UpdatePreview(m_ash, &ASinhStretch::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		return iw32->UpdatePreview(m_ash, &ASinhStretch::Apply);
	}
	}
}