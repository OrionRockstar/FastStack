#include "pch.h"
#include "LocalHistogramEqualizationDialog.h"
#include "Faststack.h"


using LHE = LocalHistogramEqualization;
using LHED = LocalHistogramEqualizationDialog;
LHED::LocalHistogramEqualizationDialog(QWidget* parent) : ProcessDialog("LocalHistogramEqualization", QSize(475, 150), FastStack::recast(parent)->workspace()) {

	setTimerInterval(500);
	setPreviewMethod(this, &LHED::applytoPreview);
	connectToolbar(this, &LHED::apply, &LHED::showPreview, &LHED::resetDialog);

	addKernelRadiusInputs();
	addContrastLimitInputs();
	addAmountInputs();

	m_circular = new CheckBox("Circular Kernel", drawArea());
	m_circular->move(310, 110);
	m_circular->setChecked(true);
	m_lhe.setCircularKernel(m_circular->isChecked());

	auto onClicked = [this](bool v) {
		m_lhe.setKernelRadius(v);
		applytoPreview();
	};

	connect(m_circular, &QCheckBox::clicked, this, onClicked);

	m_histogram_resolution = new ComboBox(drawArea());
	m_histogram_resolution->move(200, 110);
	
	m_histogram_resolution->addItem("8-bit", QVariant::fromValue(Histogram::Resolution::_8bit));
	m_histogram_resolution->addItem("10-bit", QVariant::fromValue(Histogram::Resolution::_10bit));
	m_histogram_resolution->addItem("12-bit", QVariant::fromValue(Histogram::Resolution::_12bit));

	//m_histogram_resolution->addItems({ "8-bit", "10-bit", "12-bit" });
	addLabel(m_histogram_resolution, new QLabel("Histogram Resolution:", drawArea()));

	auto activated = [this](int index) {
		m_lhe.setHistogramResolution(m_histogram_resolution->itemData(index).value<Histogram::Resolution>());
		applytoPreview();
	};

	connect(m_histogram_resolution, &QComboBox::activated, this, activated);

	this->show();
}

void LHED::addKernelRadiusInputs() {

	m_kr_le = new IntLineEdit(64, new IntValidator(16, 512), drawArea());
	m_kr_le->setGeometry(125, 15, 65, 25);
	m_kr_le->setMaxLength(3);
	addLabel(m_kr_le, new QLabel("Kernel Radius:", drawArea()));

	m_kr_slider = new Slider(Qt::Horizontal, drawArea());
	m_kr_slider->setRange(16, 256);
	m_kr_slider->setValue(64);
	m_kr_slider->setSingleStep(2);
	m_kr_slider->setFixedWidth(250);
	m_kr_le->addSlider(m_kr_slider);

	auto action = [this](int) {
		int val = (m_kr_slider->sliderPosition() / 2) * 2;
		m_kr_le->setValue(val);
		m_lhe.setKernelRadius(val);
		startTimer();
	};

	auto edited = [this]() {
		int val = m_kr_le->value();
		m_kr_slider->setValue((val / 2) * 2);
		m_lhe.setKernelRadius(val);

		applytoPreview();
	};

	connect(m_kr_slider, &QSlider::actionTriggered, this, action);
	connect(m_kr_le, &QLineEdit::editingFinished, this, edited);
}

void LHED::addContrastLimitInputs() {

	m_cl_le = new DoubleLineEdit(2.0, new DoubleValidator(1.0, 64, 1), drawArea());
	m_cl_le->setGeometry(125, 45, 65, 25);
	addLabel(m_cl_le, new QLabel("Contrast Limit:", drawArea()));

	m_cl_slider = new Slider(Qt::Horizontal, drawArea());
	m_cl_slider->setRange(2, 74);
	m_cl_slider->setValue(4);
	m_cl_slider->setFixedWidth(250);
	m_cl_slider->setPageStep(4);
	m_cl_le->addSlider(m_cl_slider);

	auto action = [this](int) {
		float contrast = m_cl_slider->sliderPosition();

		if (m_cl_slider->sliderPosition() < 20)
			contrast /= 2.0;
		else
			contrast -= 10.0;

		m_cl_le->setValue(contrast);
		m_lhe.setContrastLimit(contrast);
		startTimer();
	};

	auto edited = [this]() {
		float contrast = m_cl_le->value();
		m_lhe.setContrastLimit(contrast);

		if (contrast < 10.0)
			m_cl_slider->setValue(contrast * 2);
		else
			m_cl_slider->setValue(contrast + 10);

		applytoPreview();
	};

	connect(m_cl_slider, &QSlider::actionTriggered, this, action);
	connect(m_cl_le, &QLineEdit::editingFinished, this, edited);
}

void LHED::addAmountInputs() {

	m_amount_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 3), 5, drawArea());
	m_amount_le->setGeometry(125, 75, 65, 25);
	addLabel(m_amount_le, new QLabel("Amount:", drawArea()));

	m_amount_slider = new Slider(Qt::Horizontal, drawArea());
	m_amount_slider->setRange(0, 100);
	m_amount_slider->setValue(100);
	m_amount_slider->setFixedWidth(250);
	m_amount_le->addSlider(m_amount_slider);

	auto action = [this](int) {
		float val = m_amount_slider->sliderPosition() / 100.0;
		m_amount_le->setValue(val);
		m_lhe.setAmount(val);
		startTimer();
	};

	auto edited = [this]() {
		float val = m_amount_le->value();
		m_lhe.setAmount(val);
		m_amount_slider->setValue(val * 100);
		applytoPreview();
	};

	connect(m_amount_slider, &QSlider::actionTriggered, this, action);
	connect(m_amount_le, &QLineEdit::editingFinished, this, edited);
}

void LHED::showPreview() {

	ProcessDialog::showPreview();
	applytoPreview();
}

void LHED::resetDialog() {

	m_kr_le->setText("64");
	m_kr_slider->setValue(64);

	m_cl_le->setText("2.0");
	m_cl_slider->setValue(4);

	m_amount_le->setText("1.000");
	m_amount_slider->setValue(100);

	m_circular->setChecked(true);
	applytoPreview();
}

void LHED::apply() {

	m_lhe.setKernelRadius(m_kr_le->text().toInt());

	if (m_workspace->subWindowList().size() == 0)
		return;

	enableSiblings(false);

	if (m_pd == nullptr)
		m_pd = std::make_unique<ProgressDialog>(*m_lhe.progressSignal(), this);

	auto funca = [this]() {

		auto iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

		switch (iwptr->type()) {
		case ImageType::UBYTE: {
			iwptr->applyToSource(m_lhe, &LHE::apply);
			break;
		}
		case ImageType::USHORT: {
			auto iw16 = imageRecast<uint16_t>(iwptr);
			iw16->applyToSource(m_lhe, &LHE::apply);
			break;
		}
		case ImageType::FLOAT: {
			auto iw32 = imageRecast<float>(iwptr);
			iw32->applyToSource(m_lhe, &LHE::apply);
			break;
		}
		}

		emit finished();
	};

	auto funcb = [this]() {
		enableSiblings(true);
		m_pd->close();
		m_pd.reset();
		applytoPreview();
	};

	connect(this, &LHED::finished, this, funcb);
	std::thread(funca).detach();
}

void LHED::applytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = previewRecast(m_preview);

	m_lhe.setKernelRadius(m_kr_le->value() * iwptr->scaleFactor());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->updatePreview(m_lhe, &LHE::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_lhe, &LHE::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_lhe, &LHE::apply);
	}
	}
}