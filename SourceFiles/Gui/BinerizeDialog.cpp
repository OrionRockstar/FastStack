#include "pch.h"
#include "FastStack.h"
#include "BinerizeDialog.h"
#include "ImageWindow.h"


BinerizeDialog::BinerizeDialog(Workspace* parent) : ProcessDialog("Binerize", QSize(500, 175), parent) {

	setDefaultTimerInterval(150);

	addRGBRadioInputs();
	addInputs();

	m_rgb_bg->idClicked(0);

	this->show();
}

void BinerizeDialog::addRGBRadioInputs() {

	m_rgb_bg = new QButtonGroup(drawArea());

	RadioButton* joined_rgb_rb = new RadioButton("Joined RGB/K Channels", drawArea());
	joined_rgb_rb->move(50, 15);
	m_rgb_bg->addButton(joined_rgb_rb, 0);

	RadioButton* seperate_rgb_rb = new RadioButton("Seperate RGB/K Channels", drawArea());
	seperate_rgb_rb->move(255, 15);
	m_rgb_bg->addButton(seperate_rgb_rb, 1);

	auto clicked = [this](int id) {

		m_rgb_bg->button(id)->setChecked(true);

		m_green_inp->setEnabled(id);
		m_blue_inp->setEnabled(id);

		if (id == 0) {
			m_rgbk_red_inp->setText(m_rgbk);

			float v = m_rgbk_red_inp->valuef();
			m_green_inp->setValue(v);
			m_binerize.setGreenThreshold(v);
			m_blue_inp->setValue(v);
			m_binerize.setBlueThreshold(v);
		}

		else if (id == 1)
			m_rgbk_red_inp->setText(m_rk);

		applytoPreview();
	};

	connect(m_rgb_bg, &QButtonGroup::idClicked, this, clicked);
}

void BinerizeDialog::addInputs() {

	m_rgbk_red_inp = new DoubleInput(m_rgbk, m_binerize.rgbk_RedThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea(), 200.0f);
	m_rgbk_red_inp->move(75, 50);
	m_rgbk_red_inp->setSliderWidth(300);

	m_green_inp = new DoubleInput("G:   ", m_binerize.greenThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea(), 200.0f);
	m_green_inp->move(75, 90);
	m_green_inp->setSliderWidth(300);

	m_blue_inp = new DoubleInput("B:   ", m_binerize.blueThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea(), 200.0f);
	m_blue_inp->move(75, 130);
	m_blue_inp->setSliderWidth(300);


	auto func = [this]() {
		m_binerize.setRGBK_RedThreshold(m_rgbk_red_inp->valuef());
		if (m_rgb_bg->checkedId() == 0) {
			float v = m_rgbk_red_inp->valuef();
			m_green_inp->setValue(v);
			m_binerize.setGreenThreshold(v);
			m_blue_inp->setValue(v);
			m_binerize.setBlueThreshold(v);
		}
	};

	connect(m_rgbk_red_inp, &InputBase::actionTriggered, this, [this, func](int) { func(); startTimer(); });
	connect(m_rgbk_red_inp, &InputBase::editingFinished, this, [this, func]() { func(); applytoPreview(); });

	connect(m_green_inp, &InputBase::actionTriggered, this, [this](int) {
		m_binerize.setGreenThreshold(m_green_inp->valuef());
		startTimer(); });
	connect(m_green_inp, &InputBase::editingFinished, this, [this]() {
		m_binerize.setGreenThreshold(m_green_inp->valuef());
		applytoPreview(); });

	connect(m_blue_inp, &InputBase::actionTriggered, this, [this](int) {
		m_binerize.setBlueThreshold(m_blue_inp->valuef());
		startTimer(); });
	connect(m_blue_inp, &InputBase::editingFinished, this, [this]() {
		m_binerize.setBlueThreshold(m_blue_inp->valuef());
		applytoPreview(); });
}

void BinerizeDialog::resetDialog() {

	m_binerize = Binerize();
	m_rgbk_red_inp->reset();
	m_green_inp->reset();
	m_green_inp->setEnabled(false);
	m_blue_inp->reset();
	m_blue_inp->setEnabled(false);
	m_rgb_bg->button(0)->setChecked(true);
	applytoPreview();
}

void BinerizeDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	ImageWindow8* iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->applyToSource(m_binerize, &Binerize::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		return iw16->applyToSource(m_binerize, &Binerize::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		return iw32->applyToSource(m_binerize, &Binerize::apply);
	}
	}
}

void BinerizeDialog::applyPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* iwptr = previewRecast(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_binerize, &Binerize::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_binerize, &Binerize::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_binerize, &Binerize::apply);
	}
	}
}