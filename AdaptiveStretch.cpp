#include "pch.h"
#include "AdaptiveStretch.h"
#include "FastStack.h"

template<typename T>
static void RGBtoI(Image<T>& src, Image<T>& dst) {

	if (src.Channels() == 1)
		return;

	dst = Image<T>(src.Rows(), src.Cols());

	for (int el = 0; el < src.PxCount(); ++el) {
		T R, G, B;
		src.getRGB(el, R, G, B);

		dst[el] = 0.5 * (Max(R, Max(G, B)) + Min(R, Min(G, B)));
	}

}


template<typename T>
std::vector<float> AdaptiveStretch::GetCumulativeNetForces(Image<T>& img) {

	std::vector<uint32_t> pos;
	std::vector<uint32_t> neg;

	std::vector<float> cumulative_net;

	int multiplier = 1;

	if (img.is_uint8()) {
		pos.resize(256, 0);
		neg.resize(256, 0);
		cumulative_net.resize(256);
	}

	else if (img.is_uint16()) {
		pos.resize(65536, 0);
		neg.resize(65536, 0);
		cumulative_net.resize(65536);
	}

	else if (img.is_float()) {
		pos.resize(m_data_points, 0);
		neg.resize(m_data_points, 0);
		cumulative_net.resize(m_data_points);
		multiplier = m_data_points - 1;
	}

	T noise = Pixel<T>::toType(m_noise_thresh);
	float contrast = (m_contrast_protection) ? m_contrast : 0;

#pragma omp parallel for num_threads(4)
	for (int y = 0; y < img.Rows() - 1; ++y) {
		for (int x = 0; x < img.Cols() - 1; ++x) {

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


	cumulative_net[0] = pos[0] - contrast * neg[0];
	for (int i = 1; i < cumulative_net.size(); ++i)
		cumulative_net[i] = pos[i] - contrast * neg[i] + cumulative_net[i - 1];

	return cumulative_net;
}
template std::vector<float> AdaptiveStretch::GetCumulativeNetForces(Image8&);
template std::vector<float> AdaptiveStretch::GetCumulativeNetForces(Image16&);
template std::vector<float> AdaptiveStretch::GetCumulativeNetForces(Image32&);

template<typename T>
void AdaptiveStretch::ComputeCDF(Image<T>& img) {

	Image<T> temp;
	if (img.Channels() == 3) {
		RGBtoI(img, temp);
		img.Swap(temp);
	}

	std::vector<uint32_t> pos;
	std::vector<uint32_t> neg;

	int multiplier = 1;

	if (img.is_uint8()) {
		pos.resize(256, 0);
		neg.resize(256, 0);
		m_cdf_curve.Resize(256);
		//m_cdf_curve.resize(256);
	}

	else if (img.is_uint16()) {
		pos.resize(65536, 0);
		neg.resize(65536, 0);
		m_cdf_curve.Resize(65536);
		//cumulative_net.resize(65536);
	}

	else if (img.is_float()) {
		pos.resize(m_data_points, 0);
		neg.resize(m_data_points, 0);
		m_cdf_curve.Resize(m_data_points);
		//cumulative_net.resize(m_data_points);
		multiplier = m_data_points - 1;
	}

	T noise = Pixel<T>::toType(m_noise_thresh);
	float contrast = (m_contrast_protection) ? m_contrast : 0;

#pragma omp parallel for num_threads(4)
	for (int y = 0; y < img.Rows() - 1; ++y) {
		for (int x = 0; x < img.Cols() - 1; ++x) {

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
	for (int i = 1; i < m_cdf_curve.curve.size(); ++i)
		m_cdf_curve[i] = pos[i] - contrast * neg[i] + m_cdf_curve.curve[i - 1];

	T min, max;
	img.ComputeMinMax(min, max, 0);

	if (temp.Channels() == 3)
		temp.Swap(img);

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

	m_cdf_curve.min = m_cdf_curve[bm];
	m_cdf_curve.max = m_cdf_curve[bM];
}
template void AdaptiveStretch::ComputeCDF(Image8& img);

template <typename T>
void AdaptiveStretch::Apply(Image<T>& img) {

	/*std::vector<float> cumulative_net;

	T min, max;

	if (img.Channels() == 3) {
		Image<T> temp;
		RGBtoI(img, temp);
		cumulative_net = GetCumulativeNetForces(temp);
		temp.ComputeMinMax(min, max, 0);
	}

	else {
		cumulative_net = GetCumulativeNetForces(img);
		img.ComputeMinMax(min, max, 0);
	}

	int multiplier = (img.is_float()) ? m_data_points - 1 : 1;
	int bm = min * multiplier, bM = max * multiplier;
	double mindiff = 0;

	//find "greatest" net_cdf decrease
	for (int k = bm; k < bM; ++k) {
		float d = cumulative_net[k + 1] - cumulative_net[k];
		if (d < mindiff)
			mindiff = d;
	}

	if (mindiff < 0) {
		mindiff = -mindiff;
		for (int k = bm; k <= bM; ++k)
			cumulative_net[k] += (k - bm) * (mindiff + 1 / (bM - bm));
	}

	float c_max = cumulative_net[bM], c_min = cumulative_net[bm];*/


	//ComputeCDF(img);

	int multiplier = (img.is_float()) ? m_data_points - 1 : 1;

	for (auto& v : img)
		v = Pixel<T>::toType((m_cdf_curve[v * multiplier] - m_cdf_curve.min) / (m_cdf_curve.max - m_cdf_curve.min));

}
template void AdaptiveStretch::Apply(Image8&);
template void AdaptiveStretch::Apply(Image16&);
template void AdaptiveStretch::Apply(Image32&);

template<typename T>
void AdaptiveStretch::ApplyTo(Image<T>& src, Image<T>& dst, int factor) {


	ComputeCDF(src);

	int multiplier = (src.is_float()) ? m_data_points - 1 : 1;


	for (int ch = 0; ch < dst.Channels(); ++ch)
		for (int y = 0, y_s = 0; y < dst.Rows(); ++y, y_s += factor)
			for (int x = 0, x_s = 0; x < dst.Cols(); ++x, x_s += factor)
				dst(x, y, ch) = Pixel<T>::toType((m_cdf_curve[src(x_s, y_s, ch) * multiplier] - m_cdf_curve.min) / (m_cdf_curve.max - m_cdf_curve.min));

	//for (auto& v : src)
		//v = Pixel<T>::toType((cumulative_net[v * multiplier] - c_min) / (c_max - c_min));
}
template void AdaptiveStretch::ApplyTo(Image8& src, Image8& dst, int factor);
template void AdaptiveStretch::ApplyTo(Image16& src, Image16& dst, int factor);
template void AdaptiveStretch::ApplyTo(Image32& src, Image32& dst, int factor);






AdaptiveStretchDialog::AdaptiveStretchDialog(QWidget* parent) : ProcessDialog("AdaptiveStretch", QSize(650, 150), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent) {

	using ASD = AdaptiveStretchDialog;

	setTimer(500, this, &ASD::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &ASD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ASD::Apply, &ASD::showPreview, &ASD::resetDialog);

	AddNoiseThresholdInputs();
	AddContrastProtectionInputs();
	m_curve_pts_label = new QLabel("Maximum Curve Points:   ", this);
	m_curve_pts_label->move(190 - m_curve_pts_label->fontMetrics().horizontalAdvance(m_curve_pts_label->text()), 92);

	m_curve_pts_le = new QLineEdit("1000000", this);
	m_curve_pts_le->setFixedWidth(75);
	m_curve_pts_le->move(190, 90);
	m_curve_pts_le->setValidator(new QIntValidator(100, 10'000'000, this));
	
	//m_series = new QLineSeries;

	//m_graph = new QChart;
	//m_graph->addSeries(m_series);
	//m_graph->resize(400, 400);
	//QChartView* cv = new QChartView(m_graph, parent);
	//cv->resize(400, 400);

	this->show();
}

//first le
void AdaptiveStretchDialog::editingFinished_noise() {

	double val = m_noise_le->text().toDouble();

	if (val >= 1.0) {
		double temp = m_noise_coef_le->text().toDouble();
		temp *= pow(10, -m_noise_coef_exp->currentIndex());
		m_noise_le->setText(QString::number(temp, 'f'));

	}

	else {
		for (int exp = 0; exp < m_noise_coef_exp->count(); ++exp) {
			double temp = val / pow(10, -exp);
			if (m_noise_coef_le->Validator()->bottom() <= temp && temp < m_noise_coef_le->Validator()->top()) {
				m_noise_coef_exp->setCurrentIndex(exp);
				m_noise_coef_le->setText(QString::number(temp, 'f'));
				m_noise_coef_slider->setSliderPosition(temp * 100);
				break;
			}
		}

	}

	m_as.setNoiseThreshold(val);

	ApplytoPreview();
}
//second le
void AdaptiveStretchDialog::editingFinished_noise_coef() {

	double val = m_noise_coef_le->text().toDouble();
	m_noise_coef_slider->setSliderPosition(val * 100);

	val *= pow(10, -m_noise_coef_exp->currentIndex());
	m_as.setNoiseThreshold(val);
	m_noise_le->setText(QString::number(val, 'f'));

	ApplytoPreview();
}

void AdaptiveStretchDialog::actionSlider_noise_coef(int action) {

	if (m_noise_coef_exp->currentText().toInt() == 0)
		return m_noise_coef_slider->setSliderPosition(0);

	int pos = m_noise_coef_slider->sliderPosition();

	double c = (pos - pos % 5) / 100.0;

	m_noise_coef_le->setValue(c);

	c *= pow(10, -m_noise_coef_exp->currentIndex());
	m_noise_le->setValue(c);
	m_as.setNoiseThreshold(c);

	startTimer();
}

void AdaptiveStretchDialog::itemSelected_noise_coef(int index) {

	int value = m_noise_coef_exp->itemText(index).toInt();

	if (value == 0) {
		m_noise_le->setValue(1.0);
		m_noise_coef_le->setValue(1.0);
		m_noise_coef_slider->setSliderPosition(0);
		m_as.setNoiseThreshold(1.0);
	}
	else {
		double c = m_noise_coef_le->text().toDouble();
		c *= pow(10, -m_noise_coef_exp->currentIndex());
		m_noise_le->setValue(c);
		m_as.setNoiseThreshold(c);
	}
	ApplytoPreview();
}



void AdaptiveStretchDialog::editingFinished_contrast() {

	double val = m_contrast_le->text().toDouble();

	if (val >= 1.0) {
		double temp = m_contrast_coef_le->text().toDouble();
		temp *= pow(10, -m_contrast_coef_exp->currentIndex());
		m_contrast_le->setText(QString::number(temp, 'f'));
	}

	else {
		for (int exp = 0; exp < m_contrast_coef_exp->count(); ++exp) {
			double temp = val / pow(10, -exp);
			if (m_contrast_coef_le->Validator()->bottom() <= temp && temp < m_contrast_coef_le->Validator()->top()) {
				m_contrast_coef_exp->setCurrentIndex(exp);
				m_contrast_coef_le->setText(QString::number(temp, 'f'));
				m_contrast_coef_slider->setSliderPosition(temp * 100);
				break;
			}
		}

	}

	m_as.setContrast(val);
	ApplytoPreview();
}

void AdaptiveStretchDialog::editingFinished_contrast_coef() {

	double val = m_contrast_coef_le->text().toDouble();
	m_contrast_coef_slider->setSliderPosition(val * 100);

	val *= pow(10, -m_contrast_coef_exp->currentIndex());
	m_as.setContrast(val);
	m_contrast_le->setText(QString::number(val, 'f'));

	ApplytoPreview();
}

void AdaptiveStretchDialog::actionSlider_contrast_coef(int action) {

	if (m_contrast_coef_exp->currentText().toInt() == 0)
		return m_contrast_coef_slider->setSliderPosition(0);

	int pos = m_contrast_coef_slider->sliderPosition();

	double c = (pos - pos % 5) / 100.0;

	m_contrast_coef_le->setValue(c);

	c *= pow(10, -m_contrast_coef_exp->currentIndex());
	m_contrast_le->setValue(c);
	m_as.setContrast(c);

	startTimer();
}

void AdaptiveStretchDialog::itemSelected_contrast_coef(int index) {

	int value = m_contrast_coef_exp->itemText(index).toInt();

	if (value == 0) {
		m_contrast_le->setValue(1.0);
		m_contrast_coef_le->setValue(1.0);
		m_contrast_coef_slider->setSliderPosition(0);
		m_as.setContrast(1.0);
	}
	else {
		double c = m_contrast_coef_le->text().toDouble();
		c *= pow(10, -m_contrast_coef_exp->currentIndex());
		m_contrast_le->setValue(c);
		m_as.setContrast(c);
	}

	ApplytoPreview();
}

void AdaptiveStretchDialog::onClick_constrast(bool val) {
	m_contrast_le->setEnabled(val);
	m_contrast_coef_le->setEnabled(val);
	m_contrast_coef_slider->setEnabled(val);
	m_contrast_coef_exp->setEnabled(val);

	m_as.setContrastProtection(val);

	float c = m_contrast_coef_le->text().toFloat() * pow(10, -m_contrast_coef_exp->currentIndex());
	m_as.setContrast(c);

	ApplytoPreview();
}


void AdaptiveStretchDialog::editingFinished_data_pts() {
	m_as.setDataPoints(m_curve_pts_le->text().toInt());
	ApplytoPreview();
}


void AdaptiveStretchDialog::AddNoiseThresholdInputs() {

	int dy = 10;

	m_noise_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
	m_noise_le->setFixedWidth(85);
	m_noise_le->move(135, dy);
	m_noise_le->addLabel(new QLabel("Noise Threshold:   ", this));

	m_noise_coef_le = new DoubleLineEdit("", new DoubleValidator(1.00, 9.99, 2), 4,this);
	m_noise_coef_le->setValue(1.0);
	m_noise_coef_le->setFixedWidth(50);
	m_noise_coef_le->move(225, dy);

	m_noise_coef_slider = new QSlider(Qt::Horizontal, this);
	m_noise_coef_slider->setRange(100, 1000);
	m_noise_coef_slider->setFixedWidth(250);
	m_noise_coef_le->addSlider(m_noise_coef_slider);

	connect(m_noise_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_noise);
	connect(m_noise_coef_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_noise_coef);
	connect(m_noise_coef_slider, &QSlider::actionTriggered, this, &AdaptiveStretchDialog::actionSlider_noise_coef);

	m_noise_coef_exp = new QComboBox(this);
	m_noise_coef_exp->move(550, dy);
	m_noise_coef_exp->addItems({ "0","-1","-2","-3","-4","-5","-6" });// "-7", "-8"});
	m_noise_coef_exp->setCurrentIndex(3);
	connect(m_noise_coef_exp, &QComboBox::activated, this, &AdaptiveStretchDialog::itemSelected_noise_coef);

	m_noise_le->setValue(m_noise_coef_le->text().toDouble() * pow(10, -m_noise_coef_exp->currentIndex()));
}

void AdaptiveStretchDialog::AddContrastProtectionInputs() {

	int dy = 50;

	m_contrast_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
	m_contrast_le->setFixedWidth(85);
	m_contrast_le->move(135, dy);
	m_contrast_le->addLabel(new QLabel("Contrast:   ", this));

	m_contrast_coef_le = new DoubleLineEdit("", new DoubleValidator(1.00, 9.99, 2), 4, this);
	m_contrast_coef_le->setValue(1.0);
	m_contrast_coef_le->setFixedWidth(50);
	m_contrast_coef_le->move(225, dy);

	m_contrast_coef_slider = new QSlider(Qt::Horizontal, this);
	m_contrast_coef_slider->setRange(100, 1000);
	m_contrast_coef_slider->setFixedWidth(250);
	m_contrast_coef_le->addSlider(m_contrast_coef_slider);

	connect(m_contrast_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_contrast);
	connect(m_contrast_coef_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_contrast_coef);
	connect(m_contrast_coef_slider, &QSlider::actionTriggered, this, &AdaptiveStretchDialog::actionSlider_contrast_coef);

	m_contrast_coef_exp = new QComboBox(this);
	m_contrast_coef_exp->move(550, dy);
	m_contrast_coef_exp->addItems({ "0","-1","-2","-3","-4","-5","-6" });// , "-7", "-8"});
	m_contrast_coef_exp->setCurrentIndex(4);
	connect(m_contrast_coef_exp, &QComboBox::activated, this, &AdaptiveStretchDialog::itemSelected_contrast_coef);

	m_contrast_le->setValue(m_contrast_coef_le->text().toDouble() * pow(10, -m_contrast_coef_exp->currentIndex()));

	m_contrast_cb = new QCheckBox(this);
	m_contrast_cb->move(605, dy+5);
	connect(m_contrast_cb, &QCheckBox::clicked, this, &AdaptiveStretchDialog::onClick_constrast);
	m_contrast_cb->clicked(false);
}

void AdaptiveStretchDialog::showPreview() {

	ProcessDialog::showPreview();
	ApplytoPreview();
}

void AdaptiveStretchDialog::resetDialog() {
	m_noise_coef_le->setValue(1.0);
	m_noise_coef_slider->setSliderPosition(0);
	m_noise_coef_exp->setCurrentIndex(3);
	m_noise_le->setValue(0.001);

	m_contrast_coef_le->setValue(1.0);
	m_contrast_coef_slider->setSliderPosition(0);
	m_contrast_coef_exp->setCurrentIndex(4);
	m_contrast_le->setValue(0.0001);

	m_contrast_cb->setChecked(false);
	m_contrast_cb->clicked(false);
}

void AdaptiveStretchDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
		m_as.ComputeCDF(iw8->Source());
		iw8->UpdateImage(m_as, &AdaptiveStretch::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_as.ComputeCDF(iw16->Source());
		iw16->UpdateImage(m_as, &AdaptiveStretch::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_as.ComputeCDF(iw32->Source());
		iw32->UpdateImage(m_as, &AdaptiveStretch::Apply);
		break;
	}
	}

	ApplytoPreview();
}

void AdaptiveStretchDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
		auto rtp8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		m_as.ComputeCDF(iw8->Source());
		return rtp8->UpdatePreview(m_as, &AdaptiveStretch::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		auto rtp16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		m_as.ComputeCDF(iw16->Source());
		return rtp16->UpdatePreview(m_as, &AdaptiveStretch::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		auto rtp32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		m_as.ComputeCDF(iw32->Source());
		return rtp32->UpdatePreview(m_as, &AdaptiveStretch::Apply);
	}
	}
}