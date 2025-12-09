#include "pch.h"
#include "ASinhStretchDialog.h"
#include "FastStack.h"

using ASSD = ASinhStretchDialog;

ASSD::ASinhStretchDialog(Workspace* parent) : ProcessDialog("ASinhStretch", QSize(500, 155), parent) {

	setDefaultTimerInterval(250);
	connect(this, &ProcessDialog::previewRemoved, this, [this]() { m_bp_comp->setEnabled(false); });

	m_bp_comp = new PushButton("Compute Blackpoint", drawArea());
	m_bp_comp->move(90, 110);
	m_bp_comp->setDisabled(true);
	connect(m_bp_comp, &QPushButton::released, this, [this]() { computeBlackpoint(); applytoPreview(); });


	addStretchFactorInputs();
	addBlackpointInputs();
	addFinetuneInputs();

	m_rgb_cb = new CheckBox("Use sRGB", drawArea());
	m_rgb_cb->setChecked(true);
	m_rgb_cb->move(300, 112);
	connect(m_rgb_cb, &QCheckBox::clicked, this, [this](bool v) { m_ash.setsRGB(v); applytoPreview(); });

	this->show();
}

void ASSD::addStretchFactorInputs() {

	m_sf_input = new DoubleInput("Stretch Factor:   ", m_ash.stretchFactor(), new DoubleValidator(1.0, 375.0, 2), drawArea());
	m_sf_input->onAction([this](int) { m_sf_input->setLineEditValue(pow(1.024, m_sf_input->sliderValue())); });
	m_sf_input->onEdited([this]() { m_sf_input->setSliderValue(log10(m_sf_input->valuef()) / log10(1.024) + 0.5); });

	m_sf_input->setMaxLength(4);
	m_sf_input->setLineEditWidth(75);
	m_sf_input->move(135, 15);

	m_sf_input->setSliderAttributes(0, 250);
	m_sf_input->setSliderWidth(250);

	auto action = [this](int) {
		m_ash.setStretchFactor(m_sf_input->valuef());
		startTimer();
	};

	auto edited = [this]() {
		m_ash.setStretchFactor(m_sf_input->valuef());
		applytoPreview();
	};

	connect(m_sf_input, &InputBase::actionTriggered, this, action);
	connect(m_sf_input, &InputBase::editingFinished, this, edited);
}

void ASSD::addBlackpointInputs() {

	m_bp_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 0.2, 6), drawArea());
	m_bp_le->setFixedWidth(75);
	m_bp_le->move(135, 50);
	m_bp_le->setMaxLength(8);
	addLabel(m_bp_le, new QLabel("Blackpoint:", drawArea()));

	m_bp_slider = new Slider(drawArea());
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

void ASSD::addFinetuneInputs() {

	m_fine_tune = new Slider(drawArea());
	m_fine_tune->move(135, 85);
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


void ASSD::computeBlackpoint() {
	if (m_workspace->subWindowList().size() == 0 || m_preview == nullptr)
		return;

	auto iwptr = previewRecast(m_preview)->imageWindow();

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		m_ash.computeBlackpoint(iwptr->source());
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<const ImageWindow16*>(iwptr);
		m_ash.computeBlackpoint(iw16->source());
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<const ImageWindow32*>(iwptr);
		m_ash.computeBlackpoint(iw32->source());
		break;
	}
	}

	m_bp_slider->setValue(m_ash.blackpoint() * 1'000);
	m_bp_le->setValue(m_ash.blackpoint());
}

void ASSD::resetDialog() {

	m_ash = ASinhStretch();

	m_sf_input->reset();
	//m_sf_le->setText("1.00");
	//m_sf_slider->setSliderPosition(0);

	m_bp_le->setText("0.000000");
	m_bp_slider->setSliderPosition(0);

	m_rgb_cb->setChecked(true);

	applytoPreview();
}

void ASSD::showPreview() {

	ProcessDialog::showPreview();

	if (m_preview != nullptr && !m_bp_comp->isEnabled())
		m_bp_comp->setEnabled(true);
}

void ASSD::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	ImageWindow8* iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->applyToSource(m_ash, &ASinhStretch::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		return iw16->applyToSource(m_ash, &ASinhStretch::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		return iw32->applyToSource(m_ash, &ASinhStretch::apply);
	}
	}
}

void ASSD::applyPreview() {

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