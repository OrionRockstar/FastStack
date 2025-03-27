#include "pch.h"
#include "FastStack.h"
#include "SCNRDialog.h"


SCNRDialog::SCNRDialog(QWidget* parent) : ProcessDialog("SCNR", { 400,180 }, FastStack::recast(parent)->workspace(), false) {

	connectToolbar(this, &SCNRDialog::apply, &SCNRDialog::showPreview, &SCNRDialog::resetDialog);

	m_color_combo = new ComboBox(drawArea());
	m_color_combo->move(200, 20);
	addLabel(m_color_combo, new QLabel("Remove Color:", drawArea()));
	m_color_combo->addItem("Red", QVariant::fromValue(SCNR::Colors::red));
	m_color_combo->addItem("Green", QVariant::fromValue(SCNR::Colors::green));
	m_color_combo->addItem("Blue", QVariant::fromValue(SCNR::Colors::blue));
	m_color_combo->setCurrentIndex((int)m_scnr.removeColor());
	connect(m_color_combo, &QComboBox::activated, this, [this](int index) { m_scnr.setRemoveColor(m_color_combo->itemData(index).value<SCNR::Colors>()); });

	m_protection_combo = new ComboBox(drawArea());
	m_protection_combo->move(200, 60);
	addLabel(m_protection_combo, new QLabel("Protection Method:", drawArea()));
	m_protection_combo->addItem("Maximum Mask", QVariant::fromValue(SCNR::Method::maximum_mask));
	m_protection_combo->addItem("Additive Mask", QVariant::fromValue(SCNR::Method::additive_mask));
	m_protection_combo->addItem("Average Neutral", QVariant::fromValue(SCNR::Method::average_neutral));
	m_protection_combo->addItem("Maximum Neutral", QVariant::fromValue(SCNR::Method::maximum_neutral));
	m_protection_combo->setCurrentIndex((int)m_scnr.protectionMethod());
	connect(m_protection_combo, &QComboBox::activated, this, [this](int index) { m_scnr.setProtectionMethod(m_protection_combo->itemData(index).value<SCNR::Method>()); });

	addAmountInputs();

	m_preserve_lightness_cb = new CheckBox("Preserve Lightness", drawArea());
	m_preserve_lightness_cb->move(135, 140);
	m_preserve_lightness_cb->setChecked(m_scnr.preserveLightness());

	this->show();
}

void SCNRDialog::addAmountInputs() {

	m_amount_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 2), drawArea());
	m_amount_le->move(85, 100);
	addLabel(m_amount_le, new QLabel("Amount:", drawArea()));

	m_amount_slider = new Slider(Qt::Horizontal, drawArea());
	m_amount_slider->setRange(0, 100);
	m_amount_slider->setValue(100);
	m_amount_slider->setFixedWidth(200);
	m_amount_le->addSlider(m_amount_slider);

	auto edited = [this]() {
		m_amount_slider->setValue(m_amount_le->value() * m_amount_slider->maximum());
		m_scnr.setAmount(m_amount_le->value());
	};

	auto action = [this](int action) {
		float v = float(m_amount_slider->sliderPosition()) / m_amount_slider->maximum();
		m_amount_le->setValue(v);
		m_scnr.setAmount(v);
	};

	connect(m_amount_le, &QLineEdit::editingFinished, this, edited);
	connect(m_amount_slider, &QSlider::actionTriggered, this, action);
}

void SCNRDialog::resetDialog() {

	m_scnr = SCNR();
	m_color_combo->setCurrentIndex((int)m_scnr.removeColor());
	m_protection_combo->setCurrentIndex((int)m_scnr.protectionMethod());

	m_amount_le->setValue(m_scnr.amount());
	emit m_amount_le->editingFinished();

	m_preserve_lightness_cb->setChecked(m_scnr.preserveLightness());
}

void SCNRDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	if (iwptr->channels() != 3) {
		QMessageBox::information(this, "", "Image must be a RGB image.");
		return;
	}

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_scnr, &SCNR::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_scnr, &SCNR::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_scnr, &SCNR::apply);
		break;
	}
	}
}