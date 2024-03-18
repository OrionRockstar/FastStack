#include "pch.h"
#include "FastStack.h"
#include "ASinhStretch.h"

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

template<typename Image>
void ASinhStretch::ComputeBlackpoint(Image& img) {
	Histogram histogram(img);

	int sum = 0;
	int i = 0;

	while (sum < img.TotalPxCount() * .02)
		sum += histogram[i++];

	m_blackpoint = (i - 1) / 65535.0f;

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









ASinhStretchDialog::ASinhStretchDialog(QWidget* parent): ProcessDialog("ASinhStretch", parent) {

	this->setWindowTitle(Name());
	this->setGeometry(400, 400, 500, 175);
	this->setFocus();

	m_timer = new Timer(500, this);
	connect(m_timer, &QTimer::timeout, this, &ASinhStretchDialog::ApplytoPreview);

	setWorkspace(reinterpret_cast<FastStack*>(parentWidget())->workspace);
	setToolbar(new Toolbar(this));

	connect(m_tb, &Toolbar::sendApply, this, &ASinhStretchDialog::Apply);
	connect(m_tb, &Toolbar::sendPreview, this, &ASinhStretchDialog::showPreview);
	connect(m_tb, &Toolbar::sendReset, this, &ASinhStretchDialog::resetDialog);

	m_bp_comp = new QPushButton(this);
	m_bp_comp->setText("Compute Blackpoint");
	m_bp_comp->move(62, 110);
	m_bp_comp->setAutoDefault(false);
	connect(m_bp_comp, &QPushButton::pressed, this, &ASinhStretchDialog::computeBlackpoint);
	connect(m_bp_comp, &QPushButton::pressed, this, &ASinhStretchDialog::ApplytoPreview);

	AddStretchFactorInputs();
	AddBlackpointInputs();
	AddFinetuneInputs();

	m_rgb_cb = new QCheckBox("Use sRGB", this);
	m_rgb_cb->setChecked(true);
	m_rgb_cb->move(300, 112);

	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint); 
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}

void ASinhStretchDialog::actionSlider_sf(int action) {
	if (action == 3 || action == 4) {
		m_sf_le->setText(QString::number(pow(1.024, m_sf_slider->sliderPosition()), 'f'));
		m_sf_le->removeEndDecimal();
		ApplytoPreview();
	}
}

void ASinhStretchDialog::repositionSlider_sf() {

	int new_pos = log10(m_sf_le->text().toFloat()) / log10(1.024) + 0.5;
	if (new_pos == m_sf_slider->sliderPosition())
		return;

	m_sf_slider->setSliderPosition(new_pos);
	ApplytoPreview();
}

void ASinhStretchDialog::sliderMoved_sf(int value) {
	m_sf_le->setText(QString::number(pow(1.024, value), 'f'));
	m_sf_le->removeEndDecimal();

	m_timer->start();
	//ApplytoPreview();
}


void ASinhStretchDialog::actionSlider_bp(int action) {
	if (action == 3 || action == 4) {
		m_bp_le->setText(QString::number(m_bp_slider->sliderPosition() / 1000.0, 'f'));
		ApplytoPreview();
	}
}

void ASinhStretchDialog::repositionSlider_bp() {

	int new_pos = m_bp_le->text().toFloat() * 1000;
	if (new_pos == m_bp_slider->sliderPosition())
		return;

	m_bp_slider->setSliderPosition(new_pos);
	ApplytoPreview();
}

void ASinhStretchDialog::sliderMoved_bp(int value) {

	m_bp_le->setText(QString::number(value/1000.0,'f'));
	m_timer->start();
	//ApplytoPreview();
}


void ASinhStretchDialog::actionSlider_ft(int action) {
	if (action == 3 || action == 4)
		m_fine_tune->setValue(0);
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
	
	m_bp_le->setText(QString::number(new_bp,'f', 6));
	m_bp_slider->setValue(new_bp * 1000);

	m_timer->start();
	//ApplytoPreview();
}

void ASinhStretchDialog::sliderReleased_ft() {
	m_fine_tune->setValue(0);
}


void ASinhStretchDialog::AddStretchFactorInputs() {

	int dy = 20;

	m_sf_label = new QLabel("Stretch Factor:", this);
	m_sf_label->move(25, dy);

	m_sf_le = new DoubleLineEdit("1.00", new DoubleValidator(1.00, 375, 2, this), this);
	m_sf_le->setGeometry(135, dy, 70, 25);
	m_sf_le->setMaxLength(4);

	m_sf_slider = new QSlider(Qt::Horizontal, this);
	m_sf_slider->setRange(0, 250);
	m_sf_slider->setFixedWidth(250);
	m_sf_slider->move(225, dy);

	connect(m_sf_le, &DoubleLineEdit::editingFinished, this, &ASinhStretchDialog::repositionSlider_sf);
	connect(m_sf_slider, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_sf);
	connect(m_sf_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_sf);
}

void ASinhStretchDialog::AddBlackpointInputs() {

	int dy = 50;

	m_bp_label = new QLabel("Blackpoint:", this);
	m_bp_label->move(45, dy);

	m_bp_slider = new QSlider(Qt::Horizontal, this);
	m_bp_slider->setRange(0, 200);
	m_bp_slider->setFixedWidth(250);
	m_bp_slider->move(225,dy);

	m_bp_le = new DoubleLineEdit("0.000000", new DoubleValidator(0.0, 0.2, 6, this), this);
	m_bp_le->setGeometry(135, dy, 70, 25);
	m_bp_le->setMaxLength(8);

	connect(m_bp_le, &DoubleLineEdit::editingFinished, this, &ASinhStretchDialog::repositionSlider_bp);
	connect(m_bp_slider, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_bp);
	connect(m_bp_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_bp);
}

void ASinhStretchDialog::AddFinetuneInputs() {

	m_fine_tune = new QSlider(Qt::Horizontal, this);
	m_fine_tune->move(135, 85);
	m_fine_tune->setFixedWidth(340);
	m_fine_tune->setRange(-500, 500);
	m_fine_tune->setValue(0);

	connect(m_fine_tune, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_ft);
	connect(m_fine_tune, &QSlider::sliderPressed, this, &ASinhStretchDialog::sliderPressed_ft);
	connect(m_fine_tune, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_ft);
	connect(m_fine_tune, &QSlider::sliderReleased, this, &ASinhStretchDialog::sliderReleased_ft);
}

void ASinhStretchDialog::computeBlackpoint() {
	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		ash.ComputeBlackpoint(iwptr->source);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		ash.ComputeBlackpoint(iw16->source);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		ash.ComputeBlackpoint(iw32->source);
		break;
	}
	}

	m_bp_le->setText(QString::number(ash.Blackpoint()));
	repositionSlider_bp();
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

	ash.setStretchFactor(m_sf_le->text().toFloat());
	ash.setBlackpoint(m_bp_le->text().toFloat());
	ash.setsRGB(m_rgb_cb->isChecked());

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		ash.Apply(iwptr->source);
		iwptr->DisplayImage();
		if (iwptr->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->UpdatefromParent();
			reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->DisplayImage();
			ApplytoPreview();
		}
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		ash.Apply(iw16->source);
		iw16->DisplayImage();
		if (iw16->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow16*>(iw16->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		ash.Apply(iw32->source);
		iw32->DisplayImage();
		if (iw32->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow32*>(iw32->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	}

}

void ASinhStretchDialog::ApplytoPreview() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	if (!reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->rtpExists())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	if (iwptr->rtp->windowTitle().sliced(19, iwptr->rtp->windowTitle().length() - 19).compare(m_name) != 0)
		return;

	ash.setStretchFactor(m_sf_le->text().toFloat());
	ash.setBlackpoint(m_bp_le->text().toFloat());
	ash.setsRGB(m_rgb_cb->isChecked());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp);
		iw8->UpdatefromParent();
		ash.Apply(iw8->source);
		iwptr->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		iw16->UpdatefromParent();
		ash.Apply(iw16->source);
		iw16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		iw32->UpdatefromParent();
		ash.Apply(iw32->source);
		iw32->DisplayImage();
		break;
	}
	}
}