#include "pch.h"
#include "FastStack.h"
#include "BinerizeDialog.h"
#include "ImageWindow.h"


BinerizeDialog::BinerizeDialog(QWidget* parent) : ProcessDialog("Binerize", QSize(500, 175), FastStack::recast(parent)->workspace()) {

	setTimerInterval(150);
	setPreviewMethod(this, &BinerizeDialog::applytoPreview);
	connectToolbar(this, &BinerizeDialog::apply, &BinerizeDialog::showPreview, &BinerizeDialog::resetDialog);

	addRGBRadioInputs();
	addRGBK_RedInputs();
	addGreenInputs();
	addBlueInputs();
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

		if (id == 0) {

			m_rgbk_label->setVisible(true);
			m_rk_label->setVisible(false);

			float v = m_rgbk_red_le->valuef();
			int pos = m_rgbk_red_slider->sliderPosition();

			m_green_le->setValue(v);
			m_green_slider->setSliderPosition(pos);
			m_binerize.setGreenThreshold(v);
			m_green_le->setDisabled(true);
			m_green_slider->setDisabled(true);

			m_blue_le->setValue(v);
			m_blue_slider->setSliderPosition(pos);
			m_binerize.setBlueThreshold(v);
			m_blue_le->setDisabled(true);
			m_blue_slider->setDisabled(true);
		}

		else if (id == 1) {

			m_rgbk_label->setVisible(false);
			m_rk_label->setVisible(true);

			m_green_le->setEnabled(true);
			m_green_slider->setEnabled(true);

			m_blue_le->setEnabled(true);
			m_blue_slider->setEnabled(true);
		}
	};

	connect(m_rgb_bg, &QButtonGroup::idClicked, this, clicked);
}

void BinerizeDialog::addRGBK_RedInputs() {

	m_rgbk_red_le = new DoubleLineEdit(m_binerize.rgbk_RedThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_rgbk_red_le->move(75, 50);

	m_rgbk_label = new QLabel(m_rgbk, drawArea());
	m_rk_label = new QLabel(m_rk, drawArea());
	m_rk_label->setVisible(false);

	addLabel(m_rgbk_red_le, m_rgbk_label);
	addLabel(m_rgbk_red_le, m_rk_label);

	m_rgbk_red_slider = new Slider(Qt::Horizontal, drawArea());
	m_rgbk_red_slider->setFixedWidth(300);
	m_rgbk_red_slider->setRange(0, 200);
	m_rgbk_red_slider->setValue(m_binerize.rgbk_RedThreshold() * m_rgbk_red_slider->maximum());
	m_rgbk_red_le->addSlider(m_rgbk_red_slider);

	auto action = [this](int) {
		float v = float(m_rgbk_red_slider->sliderPosition()) / m_rgbk_red_slider->maximum();
		m_rgbk_red_le->setValue(v);
		m_binerize.setRGBK_RedThreshold(v);

		if (m_rgb_bg->checkedId() == 0) {
			int pos = m_rgbk_red_slider->sliderPosition();

			m_green_le->setValue(v);
			m_green_slider->setSliderPosition(pos);
			m_binerize.setGreenThreshold(v);

			m_blue_le->setValue(v);
			m_blue_slider->setSliderPosition(pos);
			m_binerize.setBlueThreshold(v);
		}

		startTimer();
	};

	auto edited = [this]() {
		float v = m_rgbk_red_le->valuef();
		m_rgbk_red_slider->setValue(v * m_rgbk_red_slider->maximum());
		m_binerize.setRGBK_RedThreshold(v);

		if (m_rgb_bg->checkedId() == 0) {
			int pos = m_rgbk_red_slider->sliderPosition();

			m_green_le->setValue(v);
			m_green_slider->setSliderPosition(pos);
			m_binerize.setGreenThreshold(v);
		}
		applytoPreview();
	};

	connect(m_rgbk_red_slider, &QSlider::actionTriggered, this, action);
	connect(m_rgbk_red_le, &QLineEdit::editingFinished, this, edited);

}

void BinerizeDialog::addGreenInputs() {

	m_green_le = new DoubleLineEdit(m_binerize.greenThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_green_le->move(75, 90);
	addLabel(m_green_le, new QLabel("G:", drawArea()));

	m_green_slider = new Slider(Qt::Horizontal, drawArea());
	m_green_slider->setFixedWidth(300);
	m_green_slider->setRange(0, 200);
	m_green_slider->setValue(m_binerize.greenThreshold() * m_green_slider->maximum());
	m_green_le->addSlider(m_green_slider);


	auto action = [this](int) {
		float v = float(m_green_slider->sliderPosition()) / m_green_slider->maximum();
		m_green_le->setValue(v);
		m_binerize.setGreenThreshold(v);
		startTimer();
	};

	auto edited = [this]() {
		float v = m_green_le->valuef();
		m_green_slider->setValue(v * m_green_slider->maximum());
		m_binerize.setGreenThreshold(v);
		applytoPreview();
	};

	connect(m_green_slider, &QSlider::actionTriggered, this, action);
	connect(m_green_le, &QLineEdit::editingFinished, this, edited);
}

void BinerizeDialog::addBlueInputs() {

	m_blue_le = new DoubleLineEdit(m_binerize.blueThreshold(), new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_blue_le->move(75, 130);
	addLabel(m_blue_le, new QLabel("B:", drawArea()));

	m_blue_slider = new Slider(Qt::Horizontal, drawArea());
	m_blue_slider->setFixedWidth(300);
	m_blue_slider->setRange(0, 200);
	m_blue_slider->setValue(m_binerize.blueThreshold() * m_blue_slider->maximum());
	m_blue_le->addSlider(m_blue_slider);

	auto action = [this](int) {
		float v = float(m_blue_slider->sliderPosition()) / m_blue_slider->maximum();
		m_blue_le->setValue(v);
		m_binerize.setBlueThreshold(v);
		startTimer();
	};

	auto edited = [this]() {
		float v = m_blue_le->valuef();
		m_blue_slider->setValue(v * m_blue_slider->maximum());
		m_binerize.setBlueThreshold(v);
		applytoPreview();
	};

	connect(m_blue_slider, &QSlider::actionTriggered, this, action);
	connect(m_blue_le, &QLineEdit::editingFinished, this, edited);
}

void BinerizeDialog::resetDialog() {

	m_binerize = Binerize();
	m_rgbk_red_le->setValue(m_binerize.rgbk_RedThreshold());
	m_rgbk_red_slider->setSliderPosition(m_binerize.rgbk_RedThreshold() * m_rgbk_red_slider->maximum());
	m_rgb_bg->idClicked(0);
}

void BinerizeDialog::showPreview() {

	ProcessDialog::showPreview();
	applytoPreview();
}

void BinerizeDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	ImageWindow8* iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_binerize, &Binerize::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_binerize, &Binerize::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_binerize, &Binerize::apply);
		break;
	}
	}

	applytoPreview();
}

void BinerizeDialog::applytoPreview() {

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