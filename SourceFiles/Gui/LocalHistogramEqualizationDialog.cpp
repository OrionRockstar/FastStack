#include "pch.h"
#include "LocalHistogramEqualizationDialog.h"
#include "Faststack.h"


using LHE = LocalHistogramEqualization;
using LHED = LocalHistogramEqualizationDialog;
LHED::LocalHistogramEqualizationDialog(Workspace* parent) : ProcessDialog("LocalHistogramEqualization", QSize(475, 180), parent) {

	setDefaultTimerInterval(500);

	addKernelRadiusInputs();
	addContrastLimitInputs();
	addAmountInputs();

	m_circular_cb = new CheckBox("Circular Kernel", drawArea());
	m_circular_cb->move(310, 135);
	m_circular_cb->setChecked(true);
	m_lhe.setCircularKernel(m_circular_cb->isChecked());

	auto onClicked = [this](bool v) {
		m_lhe.setCircularKernel(v);
		applytoPreview();
	};
	connect(m_circular_cb, &QCheckBox::clicked, this, onClicked);

	m_hist_res_combo = new ComboBox(drawArea());
	m_hist_res_combo->move(200, 135);
	
	using HR = Histogram::Resolution;
	m_hist_res_combo->addItem("8-bit", QVariant::fromValue(HR::_8bit));
	m_hist_res_combo->addItem("10-bit", QVariant::fromValue(HR::_10bit));
	m_hist_res_combo->addItem("12-bit", QVariant::fromValue(HR::_12bit));
	addLabel(m_hist_res_combo, new QLabel("Histogram Resolution:", drawArea()));

	auto activated = [this](int index) {
		m_lhe.setHistogramResolution(m_hist_res_combo->itemData(index).value<HR>());
		applytoPreview();
	};
	connect(m_hist_res_combo, &QComboBox::activated, this, activated);

	this->show();
}

void LHED::addKernelRadiusInputs() {

	m_kr_input = new IntegerInput("Kernel Radius:   ", m_lhe.kernelRadius(), new IntValidator(16, 256), drawArea());
	m_kr_input->onAction([this](int) {m_kr_input->setLineEditValue((m_kr_input->sliderValue() >> 1) << 1); });
	m_kr_input->onEdited([this]() {m_kr_input->setSliderValue((m_kr_input->value() >> 1) << 1); });
	m_kr_input->move(125, 15);
	m_kr_input->setLineEditWidth(65);
	m_kr_input->setMaxLength(3);

	m_kr_input->setSliderWidth(250);
	m_kr_input->setSliderAttributes(16, 256, 2);

	auto action = [this](int) {
		m_lhe.setKernelRadius(m_kr_input->value());
		startTimer();
	};

	auto edited = [this]() {
		m_lhe.setKernelRadius(m_kr_input->value());
		applytoPreview();
	};

	connect(m_kr_input, &InputBase::actionTriggered, this, action);
	connect(m_kr_input, &InputBase::editingFinished, this, edited);
}

void LHED::addContrastLimitInputs() {

	m_contrast_input = new DoubleInput("Contrast Limit:   ", m_lhe.contrastLimit(), new DoubleValidator(0.0, 64, 1), drawArea());

	m_contrast_input->onAction([this](int) {
		float c = m_contrast_input->sliderValue();
		m_contrast_input->setLineEditValue((c < 20) ? c / 2 : c - 10);
		});
	m_contrast_input->onEdited([this]() {
		float c = m_contrast_input->valuef();
		m_contrast_input->setSliderValue((c < 10.0f) ? c * 2 : c + 10);
		});

	m_contrast_input->setLineEditWidth(65);
	m_contrast_input->move(125, 55);

	m_contrast_input->setMaxLength(3);
	m_contrast_input->setSliderAttributes(2, 74, 1, 4);
	m_contrast_input->setSliderValue(2 * m_lhe.contrastLimit());
	m_contrast_input->setSliderWidth(250);

	auto action = [this](int) {
		m_lhe.setContrastLimit(m_contrast_input->valuef());
		startTimer();
	};

	auto edited = [this]() {
		m_lhe.setContrastLimit(m_contrast_input->valuef());
		applytoPreview();
	};

	connect(m_contrast_input, &InputBase::actionTriggered, this, action);
	connect(m_contrast_input, &InputBase::editingFinished, this, edited);
}

void LHED::addAmountInputs() {

	m_amount_input = new DoubleInput("Amount:   ", 1.0, new DoubleValidator(0.0, 1.0, 3), drawArea(), 100);
	m_amount_input->setLineEditWidth(65);
	m_amount_input->move(125, 95);
	m_amount_input->setSliderWidth(250);

	auto action = [this](int) {
		m_lhe.setAmount(m_amount_input->valuef());
		startTimer();
	};

	auto edited = [this]() {
		m_lhe.setAmount(m_amount_input->valuef());
		applytoPreview();
	};

	connect(m_amount_input, &InputBase::actionTriggered, this, action);
	connect(m_amount_input, &InputBase::editingFinished, this, edited);
}

void LHED::resetDialog() {

	m_lhe = LHE();

	m_kr_input->reset();
	m_contrast_input->reset();
	m_amount_input->reset();

	m_circular_cb->setChecked(true);

	applytoPreview();
}

void LHED::apply() {

	if (!workspace()->hasSubWindows())
		return;

	ProgressDialog* pd = new ProgressDialog(m_lhe.progressSignal(), this);

	auto iwptr = imageRecast(workspace()->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->applyToSource(m_lhe, &LHE::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		return iw16->applyToSource(m_lhe, &LHE::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		return iw32->applyToSource(m_lhe, &LHE::apply);
	}
	}
}

void LHED::applyPreview() {

	if (!isPreviewValid())
		return;

	auto lhe = m_lhe;
	double sf = preview()->scaleFactor();

	//needs to run in own event loop
	if (sf < 1.0)
		lhe.setKernelRadius(m_kr_input->value() / int(1.0 / sf));
	else 
		lhe.setKernelRadius(m_kr_input->value() * sf);

	switch (preview()->type()) {
	case ImageType::UBYTE:
		return preview()->updatePreview(lhe, &LHE::apply);
	
	case ImageType::USHORT: 
		return preview<uint16_t>()->updatePreview(lhe, &LHE::apply);
	
	case ImageType::FLOAT:
		return preview<float>()->updatePreview(lhe, &LHE::apply);
	}
}