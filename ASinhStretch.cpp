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









//disable slider movement from arrow key
ASinhStretchDialog::ASinhStretchDialog(QWidget* parent): QDialog(parent) {

	m_workspace = reinterpret_cast<FastStack*>(parentWidget())->workspace;

	this->setWindowTitle("ArcSinh Stretch");
	this->setGeometry(400, 400, 500, 175);
	m_tb = new Toolbar(this);

	connect(m_tb, &Toolbar::sendApply, this, &ASinhStretchDialog::Apply);
	connect(m_tb, &Toolbar::sendPreview, this, &ASinhStretchDialog::showPreview);
	connect(m_tb, &Toolbar::sendReset, this, &ASinhStretchDialog::resetDialog);

	m_bp_comp = new QPushButton(this);
	m_bp_comp->setText("Compute Blackpoint");
	m_bp_comp->move(62, 110);
	m_bp_comp->setAutoDefault(false);
	connect(m_bp_comp, &QPushButton::pressed, this, &ASinhStretchDialog::computeBlackpoint);

	AddStretchFactorInputs();
	AddBlackpointInputs();
	AddFinetuneInputs();

	m_rgb_cb = new QCheckBox("Use sRGB", this);
	m_rgb_cb->setChecked(true);
	m_rgb_cb->move(300, 112);

	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint); 
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}

void ASinhStretchDialog::actionSlider_sf(int action) {
	if (action == 1 || action == 2)
		repositionSlider_sf();
}

void ASinhStretchDialog::repositionSlider_sf() {
	m_sf_slider->setSliderPosition(log10(m_sf_le->text().toFloat()) / log10(1.024) + 0.5);
	ApplytoPreview();
}

void ASinhStretchDialog::sliderMoved_sf(int value) {
	std::string val = std::to_string(pow(1.024, value));
	m_sf_le->setText(((val[3] == '.') ? val.substr(0, 3) : val.substr(0, 4)).c_str());
	ApplytoPreview();
}


void ASinhStretchDialog::actionSlider_bp(int action) {
	if (action == 1 || action == 2)
		repositionSlider_bp();
}

void ASinhStretchDialog::repositionSlider_bp() {
	m_bp_slider->setSliderPosition(m_bp_le->text().toFloat() * 1000);

	ApplytoPreview();
}

void ASinhStretchDialog::sliderMoved_bp(int value) {
	std::string str = std::to_string(value / 1000.0);
	m_bp_le->setText(str.c_str());

	ApplytoPreview();
}

void ASinhStretchDialog::sliderPressed_ft() {
	m_current_bp = m_bp_le->text().toFloat();
}

void ASinhStretchDialog::sliderMoved_ft(int value) {

	value += (value - m_fine_tune->value());

	value = Clip( value, m_fine_tune->minimum(), m_fine_tune->maximum());

	double new_bp = m_current_bp + value / 1'000'000.0;

	if (new_bp < m_bpdv->bottom())
		new_bp = m_bpdv->bottom();

	if (new_bp > m_bpdv->top())
		new_bp = m_bpdv->top();
	
	m_bp_le->setText(QString::number(new_bp,'f', 6));
	m_bp_slider->setValue(new_bp * 1000);

	ApplytoPreview();
}

void ASinhStretchDialog::sliderReleased_ft() {
	m_fine_tune->setValue(0);
}


void ASinhStretchDialog::AddStretchFactorInputs() {

	int dy = 20;

	m_sf_label = new QLabel("Stretch Factor:", this);
	m_sf_label->move(25, dy);

	m_sf_slider = new QSlider(Qt::Horizontal, this);
	m_sf_slider->setRange(0, 250);
	m_sf_slider->setFixedWidth(250);
	m_sf_slider->move(225, dy);


	m_sfdv = new QDoubleValidator(1.00, 375, 2, this);

	m_sf_le = new QLineEdit(this);

	m_sf_le->setGeometry(135, dy, 70, 25);
	m_sf_le->setValidator(m_sfdv);
	m_sf_le->setMaxLength(4);
	m_sf_le->setText("1.00");

	connect(m_sf_slider, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_sf);
	connect(m_sf_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_sf);
	connect(m_sf_le, &QLineEdit::returnPressed, this, &ASinhStretchDialog::repositionSlider_sf);
}

void ASinhStretchDialog::AddBlackpointInputs() {

	int dy = 50;

	m_bp_label = new QLabel("Blackpoint:", this);
	m_bp_label->move(45, dy);

	m_bp_slider = new QSlider(Qt::Horizontal, this);
	m_bp_slider->setRange(0, 200);
	m_bp_slider->setFixedWidth(250);
	m_bp_slider->move(225,dy);

	m_bpdv = new QDoubleValidator(0.0, 0.2, 6, this);

	m_bp_le = new QLineEdit(this);

	m_bp_le->setGeometry(135, dy, 70, 25);
	m_bp_le->setValidator(m_bpdv);
	m_bp_le->setMaxLength(8);
	m_bp_le->setText("0.000000");

	connect(m_bp_slider, &QSlider::sliderMoved, this, &ASinhStretchDialog::sliderMoved_bp);
	connect(m_bp_slider, &QSlider::actionTriggered, this, &ASinhStretchDialog::actionSlider_bp);
	connect(m_bp_le, &QLineEdit::returnPressed, this, &ASinhStretchDialog::repositionSlider_bp);
}

void ASinhStretchDialog::AddFinetuneInputs() {

	m_fine_tune = new QSlider(Qt::Horizontal, this);
	m_fine_tune->move(135, 85);
	m_fine_tune->setFixedWidth(340);
	m_fine_tune->setRange(-500, 500);
	m_fine_tune->setValue(0);
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

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());


	switch (iwptr->source.Bitdepth()) {
	case 8: {
		iwptr->ShowRTP();
		break;
	}
	case 16: {
		reinterpret_cast<ImageWindow16*>(iwptr)->ShowRTP();
		break;
	}
	case -32: {
		reinterpret_cast<ImageWindow32*>(iwptr)->ShowRTP();
		break;
	}
	}
	iwptr->rtp->setWindowTitle("Real-Time Preview: " + m_name);

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

			//if (iwptr->rtp->windowTitle().sliced(19, iwptr->rtp->windowTitle().length() - 19).compare(m_name) != 0)
				//reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp)->DisplayImage();
				//send signal to upate with current process instance preview, eg local hist eq
			//else
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
		ash.Apply(reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->modified);
		iwptr->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		ash.Apply(iw16->modified);
		iw16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		ash.Apply(iw32->modified);
		iw32->DisplayImage();
		break;
	}
	}
}