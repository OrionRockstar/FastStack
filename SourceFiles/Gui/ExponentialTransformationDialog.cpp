#include "pch.h"
#include "ExponentialTransformationDialog.h"
#include "FastStack.h"

using ET = ExponentialTransformation;
using ETD = ExponentialTransformationDialog;

ETD::ExponentialTransformationDialog(QWidget* parent) : ProcessDialog("ExponentialTransformation", {395,175}, FastStack::recast(parent)->workspace()) {
	
	setDefaultTimerInterval(250);

	addMethodInput();
	addOrderInputs();
	addSmoothnessInputs();

	m_lightness_cb = new CheckBox("Lightness Mask", drawArea());
	m_lightness_cb->setChecked(m_et.lightnessMask());
	m_lightness_cb->move(150, 135);
	connect(m_lightness_cb, &QCheckBox::clicked, this, [this](bool v) {m_et.applyLightnessMask(v); applytoPreview(); });

	this->show();
}

void ETD::addMethodInput() {

	m_method_combo = new ComboBox(drawArea());
	m_method_combo->addItem("Power of Inverted Pixels", QVariant::fromValue(ET::Method::power_inverted_pixels));
	m_method_combo->addItem("Screen/Mask/Invert", QVariant::fromValue(ET::Method::screen_mask_invert));
	m_method_combo->setFixedWidth(265);
	m_method_combo->move(110, 20);
	m_method_combo->addLabel(new QLabel("Method:   ", drawArea()));

	auto activation = [this](int index) {
		m_et.setMethod(m_method_combo->itemData(index).value<ET::Method>());
		applytoPreview();
	};

	connect(m_method_combo, &QComboBox::activated, this, activation);
}

void ETD::addOrderInputs() {

	m_order_inputs = new DoubleInput("Order:   ", m_et.order(), new DoubleValidator(0.1, 6.0, 1), drawArea(), 10);
	m_order_inputs->move(110, 60);
	m_order_inputs->setLineEditWidth(50);
	m_order_inputs->setSliderWidth(200);

	auto action = [this](int action) {
		m_et.setOrder(m_order_inputs->valuef());
		startTimer();
	};

	auto edited = [this]() {
		m_et.setOrder(m_order_inputs->valuef());
		applytoPreview();
	};

	connect(m_order_inputs, &InputBase::actionTriggered, this, action);
	connect(m_order_inputs, &InputBase::editingFinished, this, edited);
}

void ETD::addSmoothnessInputs() {

	m_smoothness_inputs = new DoubleInput("Smoothness:   ", m_et.sigma(), new DoubleValidator(0.0, 40.0, 2), drawArea(), 5);
	m_smoothness_inputs->move(110, 100);
	m_smoothness_inputs->setLineEditWidth(50);
	m_smoothness_inputs->setSliderWidth(200);

	auto action = [this](int action) {
		m_et.setSigma(m_smoothness_inputs->valuef());
		startTimer();
	};

	auto edited = [this]() {
		m_et.setSigma(m_smoothness_inputs->valuef());
		applytoPreview();
	};
	connect(m_smoothness_inputs, &InputBase::actionTriggered, this, action);
	connect(m_smoothness_inputs, &InputBase::editingFinished, this, edited);
}

void ETD::resetDialog() {

	m_et = ExponentialTransformation();

	m_method_combo->setCurrentIndex(m_method_combo->findData(QVariant::fromValue(m_et.method())));

	m_order_inputs->reset();
	m_smoothness_inputs->reset();

	m_lightness_cb->setChecked(m_et.lightnessMask());

	applytoPreview();
}

void ETD::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_et, &ET::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_et, &ET::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_et, &ET::apply);
		break;
	}
	}
}

void ETD::applyPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast<>(m_preview);

	ET et = m_et;
	et.setSigma(et.sigma() * iwptr->scaleFactor());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->updatePreview(et, &ET::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(et, &ET::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(et, &ET::apply);
	}
	}
}