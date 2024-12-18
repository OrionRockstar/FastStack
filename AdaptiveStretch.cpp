#include "pch.h"
#include "AdaptiveStretch.h"
#include "FastStack.h"

template<typename T>
static Image<T> RGBtoL(const Image<T>& src) {

	if (src.channels() == 1)
		return src;

	Image<T> dst(src.rows(), src.cols());

	for (int y = 0; y < src.rows(); ++y) {
		for (int x = 0; x < src.cols(); ++x) {
			auto c = src.color<>(x, y);
			dst(x,y) = 0.5 * (c.maxColor() + c.minColor());
		}
	}

	return dst;
}

template<typename T>
void AdaptiveStretch::computeCDF(const Image<T>& img) {

	if (img.channels() == 3) {
		Image<T> temp = RGBtoL(img);
		return computeCDF(temp);
	}

	std::vector<uint32_t> pos;
	std::vector<uint32_t> neg;

	int multiplier = (img.is_float()) ? m_data_points - 1 : 1;
	int size = [&]() {
		switch (img.type()) {
		case ImageType::UBYTE:
			return 256;
		case ImageType::USHORT:
			return 65536;
		case ImageType::FLOAT:
			return m_data_points;
		default:
			return 256;
		}
	}();

	pos.resize(size, 0);
	neg.resize(size, 0);
	m_cdf_curve.resize(size);	

	T noise = Pixel<T>::toType(m_noise_thresh);
	float contrast = (m_contrast_protection) ? m_contrast_threshold : 0;

#pragma omp parallel for num_threads(4)
	for (int y = 0; y < img.rows() - 1; ++y) {
		for (int x = 0; x < img.cols() - 1; ++x) {

			T a0 = img(x, y);

			T a1 = img(x + 1, y);
			uint32_t l1 = Min(a0, a1) * multiplier;

			T a2 = img(x - 1, y + 1);
			uint32_t l2 = Min(a0, a2) * multiplier;

			T a3 = img(x, y + 1);
			uint32_t l3 = Min(a0, a3) * multiplier;

			T a4 = img(x + 1, y + 1);
			uint32_t l4 = Min(a0, a4) * multiplier;

			if (abs(a0 - a1) > noise)
				pos[l1]++;
			else
				neg[l1]++;

			if (abs(a0 - a2) > noise)
				pos[l2]++;
			else
				neg[l2]++;

			if (abs(a0 - a3) > noise)
				pos[l3]++;
			else
				neg[l3]++;

			if (abs(a0 - a4) > noise)
				pos[l4]++;
			else
				neg[l4]++;
		}
	}

	m_cdf_curve[0] = pos[0] - contrast * neg[0];
	for (int i = 1; i < m_cdf_curve.size(); ++i)
		m_cdf_curve[i] = pos[i] - contrast * neg[i] + m_cdf_curve[i - 1];

	T max = img.computeMax(0);
	T min = img.computeMin(0);

	int bm = min * multiplier, bM = max * multiplier;
	double mindiff = 0;

	//find "greatest" net_cdf decrease
	for (int k = bm; k < bM; ++k) {
		float d = m_cdf_curve[k + 1] - m_cdf_curve[k];
		if (d < mindiff)
			mindiff = d;
	}

	if (mindiff < 0) {
		mindiff = -mindiff;
		for (int k = bm; k <= bM; ++k)
			m_cdf_curve[k] += (k - bm) * (mindiff + 1 / (bM - bm));
	}

	m_cdf_curve.setMin(m_cdf_curve[bm]);
	m_cdf_curve.setMax(m_cdf_curve[bM]);
}
template void AdaptiveStretch::computeCDF(const Image8& img);
template void AdaptiveStretch::computeCDF(const Image16& img);
template void AdaptiveStretch::computeCDF(const Image32& img);


template <typename T>
void AdaptiveStretch::Apply(Image<T>& img) {

	computeCDF(img);

	int multiplier = (img.is_float()) ? m_data_points - 1 : 1;

	float n = 1.0 / (m_cdf_curve.max() - m_cdf_curve.min());

	for (auto& v : img)
		v = Pixel<T>::toType((m_cdf_curve[v * multiplier] - m_cdf_curve.min()) * n);

}
template void AdaptiveStretch::Apply(Image8&);
template void AdaptiveStretch::Apply(Image16&);
template void AdaptiveStretch::Apply(Image32&);

template <typename T>
void AdaptiveStretch::Apply_NoCDF(Image<T>& img) {

	int multiplier = (img.is_float()) ? m_data_points - 1 : 1;

	float n = 1.0 / (m_cdf_curve.max() - m_cdf_curve.min());

	for (auto& v : img)
		v = Pixel<T>::toType((m_cdf_curve[v * multiplier] - m_cdf_curve.min()) * n);
}
template void AdaptiveStretch::Apply_NoCDF(Image8&);
template void AdaptiveStretch::Apply_NoCDF(Image16&);
template void AdaptiveStretch::Apply_NoCDF(Image32&);







AdaptiveStretchDialog::AdaptiveStretchDialog(QWidget* parent) : ProcessDialog("AdaptiveStretch", QSize(650, 150), FastStack::recast(parent)->workspace()) {

	using ASD = AdaptiveStretchDialog;

	setTimer(500, this, &ASD::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &ASD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ASD::Apply, &ASD::showPreview, &ASD::resetDialog);

	addNoiseThresholdInputs();
	addContrastProtectionInputs();

	m_curve_pts_le = new IntLineEdit(1000000, new IntValidator(100,10'000'000),this);
	//m_curve_pts_le->setFixedWidth(75);
	m_curve_pts_le->move(190, 90);
	m_curve_pts_le->addLabel(new QLabel("Maximum Curve Points:   ", this));
	connect(m_curve_pts_le, &QLineEdit::editingFinished, this, [this]() {
		m_as.setDataPoints(m_curve_pts_le->value());
		ApplytoPreview(); });


	//m_series = new QLineSeries;
	//m_graph = new QChart;
	//m_graph->addSeries(m_series);
	//m_graph->resize(400, 400);
	//QChartView* cv = new QChartView(m_graph, parent);
	//cv->resize(400, 400);

	this->show();
}


void AdaptiveStretchDialog::addNoiseThresholdInputs() {

	int dy = 10;

	m_noise_le = new DoubleLineEdit(0.001, new DoubleValidator(0.0, 1.0, 8), this);
	m_noise_le->setFixedWidth(85);
	m_noise_le->move(135, dy);
	m_noise_le->addLabel(new QLabel("Noise Threshold:   ", this));

	m_noise_coef_le = new DoubleLineEdit(1.0, new DoubleValidator(1.00, 9.99, 2), 4, this);
	m_noise_coef_le->setFixedWidth(50);
	m_noise_coef_le->move(225, dy);

	m_noise_coef_slider = new Slider(Qt::Horizontal, this);
	m_noise_coef_slider->setRange(100, 999);
	m_noise_coef_slider->setFixedWidth(250);
	m_noise_coef_le->addSlider(m_noise_coef_slider);


	m_noise_coef_exp = new SpinBox(this);
	m_noise_coef_exp->setRange(-6, 0);
	m_noise_coef_exp->setValue(-3);
	m_noise_coef_exp->move(550, dy);

	connectNoiseInputs();
}

void AdaptiveStretchDialog::connectNoiseInputs() {

	auto edited_noise = [this]() {

		double val = m_noise_le->value();

		int exp = floor(log10(val));
		double coef = val / pow(10, exp);
		m_noise_coef_exp->setValue(exp);
		m_noise_coef_le->setValue(coef);

		if (exp == 0)
			m_noise_coef_slider->setValue(0);
		else
			m_noise_coef_slider->setValue(coef * 100);

		m_as.setNoiseThreshold(val);
		ApplytoPreview();
	};
	connect(m_noise_le, &QLineEdit::editingFinished, this, edited_noise);

	auto edited_coef = [this]() {

		double val = m_noise_coef_le->value();

		m_noise_coef_slider->setValue(val * 100);

		val *= pow(10, m_noise_coef_exp->value());
		m_noise_le->setValue(val);

		m_as.setNoiseThreshold(val);
		ApplytoPreview();
	};
	connect(m_noise_coef_le, &QLineEdit::editingFinished, this, edited_coef);

	auto action = [this](int) {

		if (m_noise_coef_exp->value() == 0)
			return m_noise_coef_slider->setValue(0);

		double coef = m_noise_coef_slider->sliderPosition() / 100.0;

		m_noise_coef_le->setValue(coef);
		coef *= pow(10, m_noise_coef_exp->value());
		m_noise_le->setValue(coef);

		m_as.setNoiseThreshold(coef);
		startTimer();
	};
	connect(m_noise_coef_slider, &QSlider::actionTriggered, this, action);

	auto onValueChanged = [this](int val) {

		double coef = m_noise_coef_le->value();

		if (val == 0) {
			m_noise_coef_slider->setValue(0);
			m_noise_coef_le->setValue(coef = 1.0);
		}

		coef *= pow(10, val);
		m_noise_le->setValue(coef);

		m_as.setNoiseThreshold(coef);
		startTimer();
	};
	connect(m_noise_coef_exp, &QSpinBox::valueChanged, this, onValueChanged);
}

void AdaptiveStretchDialog::addContrastProtectionInputs() {

	int dy = 50;

	m_contrast_le = new DoubleLineEdit(0.01, new DoubleValidator(0.0, 1.0, 8), this);
	m_contrast_le->setFixedWidth(85);
	m_contrast_le->move(135, dy);
	m_contrast_le->addLabel(new QLabel("Contrast:   ", this));

	m_contrast_coef_le = new DoubleLineEdit(1.0, new DoubleValidator(1.00, 9.99, 2), 4, this);
	m_contrast_coef_le->setFixedWidth(50);
	m_contrast_coef_le->move(225, dy);

	m_contrast_coef_slider = new Slider(Qt::Horizontal, this);
	m_contrast_coef_slider->setRange(100, 999);
	m_contrast_coef_slider->setFixedWidth(250);
	m_contrast_coef_le->addSlider(m_contrast_coef_slider);
	
	m_contrast_coef_exp = new SpinBox(this);
	m_contrast_coef_exp->move(550, dy);
	m_contrast_coef_exp->setRange(-6, 0);
	m_contrast_coef_exp->setValue(-2);

	m_contrast_cb = new QCheckBox(this);
	m_contrast_cb->move(605, dy+5);

	connectContrastInputs();
	m_contrast_cb->clicked(false);
}

void AdaptiveStretchDialog::connectContrastInputs() {


	auto edited_contrast = [this]() {

		double val = m_contrast_le->value();

		int exp = floor(log10(val));
		double coef = val / pow(10, exp);
		m_contrast_coef_exp->setValue(exp);
		m_contrast_coef_le->setValue(coef);

		if (exp == 0)
			m_contrast_coef_slider->setValue(0);
		else
			m_contrast_coef_slider->setValue(coef * 100);

		m_as.setContrastThreshold(val);
		ApplytoPreview();
	};
	connect(m_contrast_le, &QLineEdit::editingFinished, this, edited_contrast);

	auto edited_coef = [this]() {

		double val = m_contrast_coef_le->value();

		m_contrast_coef_slider->setValue(val * 100);

		val *= pow(10, m_contrast_coef_exp->value());
		m_contrast_le->setValue(val);

		m_as.setContrastThreshold(val);
		ApplytoPreview();
	};
	connect(m_contrast_coef_le, &QLineEdit::editingFinished, this, edited_coef);

	auto action = [this](int) {

		if (m_contrast_coef_exp->value() == 0)
			return m_contrast_coef_slider->setValue(0);

		double coef = m_contrast_coef_slider->sliderPosition() / 100.0;

		m_contrast_coef_le->setValue(coef);
		coef *= pow(10, m_contrast_coef_exp->value());
		m_contrast_le->setValue(coef);

		m_as.setContrastThreshold(coef);
		startTimer();
	};
	connect(m_contrast_coef_slider, &QSlider::actionTriggered, this, action);

	auto onValueChanged = [this](int val) {

		double coef = m_contrast_coef_le->value();

		if (val == 0) {
			m_contrast_coef_slider->setValue(0);
			m_contrast_coef_le->setValue(coef = 1.0);
		}

		coef *= pow(10, val);
		m_contrast_le->setValue(coef);

		m_as.setContrastThreshold(coef);
		startTimer();
	};
	connect(m_contrast_coef_exp, &QSpinBox::valueChanged, this, onValueChanged);

	auto onClicked = [this](bool v) {

		m_contrast_le->setEnabled(v);
		m_contrast_coef_le->setEnabled(v);
		m_contrast_coef_slider->setEnabled(v);
		m_contrast_coef_exp->setEnabled(v);

		m_as.setContrastProtection(v);

		ApplytoPreview();
	};
	connect(m_contrast_cb, &QCheckBox::clicked, this, onClicked);

}

void AdaptiveStretchDialog::showPreview() {

	ProcessDialog::showPreview();
	ApplytoPreview();
}

void AdaptiveStretchDialog::resetDialog() {
	m_noise_coef_le->setValue(1.0);
	m_noise_coef_slider->setValue(0);
	m_noise_coef_exp->setValue(-3);
	m_noise_le->setValue(0.001);

	m_contrast_coef_le->setValue(1.0);
	m_contrast_coef_slider->setValue(0);
	m_contrast_coef_exp->setValue(-4);
	m_contrast_le->setValue(0.0001);

	m_contrast_cb->setChecked(false);
	m_contrast_cb->clicked(false);
}

void AdaptiveStretchDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
		iw8->applyToSource(m_as, &AdaptiveStretch::Apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->applyToSource(m_as, &AdaptiveStretch::Apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->applyToSource(m_as, &AdaptiveStretch::Apply);
		break;
	}
	}

	ApplytoPreview();
}

void AdaptiveStretchDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<PreviewWindow8*>(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr->parentWindow());
		auto rtp8 = reinterpret_cast<PreviewWindow8*>(m_preview);
		m_as.computeCDF(iw8->source());
		return rtp8->updatePreview(m_as, &AdaptiveStretch::Apply_NoCDF);
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr->parentWindow());
		auto rtp16 = reinterpret_cast<PreviewWindow16*>(m_preview);
		m_as.computeCDF(iw16->source());
		return rtp16->updatePreview(m_as, &AdaptiveStretch::Apply_NoCDF);
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr->parentWindow());
		auto rtp32 = reinterpret_cast<PreviewWindow32*>(m_preview);
		m_as.computeCDF(iw32->source());
		return rtp32->updatePreview(m_as, &AdaptiveStretch::Apply_NoCDF);
	}
	}
}