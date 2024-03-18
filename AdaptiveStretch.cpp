#include "pch.h"
#include "AdaptiveStretch.h"
#include "FastStack.h"

template<typename T>
Image<T> AdaptiveStretch::RGBtoI(Image<T>& img) {

	if (img.Channels() == 1)
		return img;

	Image<T> output(img.Rows(), img.Cols());

	for (int el = 0; el < img.PxCount(); ++el) {
		T R, G, B;
		img.getRGB(el, R, G, B);

		output[el] = 0.5 * (Max(R, Max(G, B)) + Min(R, Min(G, B)));
	}

	return output;
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

template <typename T>
void AdaptiveStretch::Apply(Image<T>& img) {

	std::vector<float> cumulative_net;

	T min, max;

	if (img.Channels() == 3) {
		Image<T> temp = RGBtoI(img);
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

	float c_max = cumulative_net[bM], c_min = cumulative_net[bm];

	for (auto& v : img)
		v = Pixel<T>::toType((cumulative_net[v * multiplier] - c_min) / (c_max - c_min));

}
template void AdaptiveStretch::Apply(Image8&);
template void AdaptiveStretch::Apply(Image16&);
template void AdaptiveStretch::Apply(Image32&);

template<typename T>
void AdaptiveStretch::ApplytoDest(Image<T>& src, Image<T>& dst, int factor) {
	std::vector<float> cumulative_net;

	T min, max;

	if (src.Channels() == 3) {
		Image<T> temp = RGBtoI(src);
		cumulative_net = GetCumulativeNetForces(temp);
		temp.ComputeMinMax(min, max, 0);
	}

	else {
		cumulative_net = GetCumulativeNetForces(src);
		src.ComputeMinMax(min, max, 0);
	}

	int multiplier = (src.is_float()) ? m_data_points - 1 : 1;
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

	float c_max = cumulative_net[bM], c_min = cumulative_net[bm];

	for (int ch = 0; ch < dst.Channels(); ++ch)
		for (int y = 0, y_s = 0; y < dst.Rows(); ++y, y_s += factor)
			for (int x = 0, x_s = 0; x < dst.Cols(); ++x, x_s += factor)
				dst(x, y, ch) = Pixel<T>::toType((cumulative_net[src(x_s, y_s, ch) * multiplier] - c_min) / (c_max - c_min));

	//for (auto& v : src)
		//v = Pixel<T>::toType((cumulative_net[v * multiplier] - c_min) / (c_max - c_min));
}
template void AdaptiveStretch::ApplytoDest(Image8& src, Image8& dst, int factor);
template void AdaptiveStretch::ApplytoDest(Image16& src, Image16& dst, int factor);
template void AdaptiveStretch::ApplytoDest(Image32& src, Image32& dst, int factor);






AdaptiveStretchDialog::AdaptiveStretchDialog(QWidget* parent) : ProcessDialog("AdaptiveStretch", parent) {
	this->resize(650, 150);
	m_timer = new Timer(500, this);
	connect(m_timer, &QTimer::timeout, this, &AdaptiveStretchDialog::ApplytoPreview);

	setWorkspace(reinterpret_cast<FastStack*>(parentWidget())->workspace);
	setToolbar(new Toolbar(this));

	connect(m_tb, &Toolbar::sendApply, this, &AdaptiveStretchDialog::Apply);
	connect(m_tb, &Toolbar::sendPreview, this, &AdaptiveStretchDialog::showPreview);
	connect(m_tb, &Toolbar::sendReset, this, &AdaptiveStretchDialog::resetDialog);

	AddNoiseThresholdInputs();
	AddContrastProtectionInputs();
	m_curve_pts_label = new QLabel("Maximum Curve Points: ", this);
	m_curve_pts_label->move(20, 87);
	m_curve_pts_le = new QLineEdit("1000000", this);
	m_curve_pts_le->setGeometry(190,85,75, 25);
	m_curve_pts_le->setValidator(new QIntValidator(100, 10'000'000, this));

	//m_series = new QLineSeries;

	//m_graph = new QChart;
	//m_graph->addSeries(m_series);
	//m_graph->resize(400, 400);
	//QChartView* cv = new QChartView(m_graph, parent);
	//cv->resize(400, 400);

	

	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}


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
			if (m_noise_coef_le->Validator()->bottom() < temp && temp < m_noise_coef_le->Validator()->top()) {
				val /= pow(10, -exp);
				m_noise_coef_exp->setCurrentIndex(exp);
			}
		}
	}
	m_noise_coef_le->setText(QString::number(val, 'f'));
	m_noise_coef_slider->setSliderPosition(val * 100);

	ApplytoPreview();
}

void AdaptiveStretchDialog::editingFinished_noise_coef() {

	double val = m_noise_coef_le->text().toDouble();
	m_noise_coef_slider->setSliderPosition(val * 100);

	val *= pow(10, -m_noise_coef_exp->currentIndex());
	m_noise_le->setText(QString::number(val, 'f'));

	ApplytoPreview();
}

void AdaptiveStretchDialog::actionSlider_noise_coef(int action) {
	if (action == 3 || action == 4) {
		sliderMoved_noise_coef(m_noise_coef_slider->sliderPosition());
		ApplytoPreview();
	}
}

void AdaptiveStretchDialog::sliderMoved_noise_coef(int value) {
	if (m_noise_coef_exp->currentText().toInt() == 0)
		return m_noise_coef_slider->setSliderPosition(0);

	double c = (value - value % 5) / 100.0;

	QString str = QString::number(c, 'f');
	m_noise_coef_le->Validator()->fixup(str);
	m_noise_coef_le->setText(str);

	c = str.toDouble() * pow(10, -m_noise_coef_exp->currentIndex());
	m_noise_le->setText(QString::number(c, 'f'));

	m_timer->start();
	//ApplytoPreview();
}

void AdaptiveStretchDialog::itemSelected_noise_coef(int index) {

	int value = m_noise_coef_exp->itemText(index).toInt();

	if (value == 0) {
		m_noise_le->setText(QString::number(1.0, 'f',6));
		m_noise_coef_le->setText(QString::number(1.0, 'f',2));
		m_noise_coef_slider->setSliderPosition(0);
	}
	else {
		double c = m_noise_coef_le->text().toDouble();
		c *= pow(10, -m_noise_coef_exp->currentIndex());
		m_noise_le->setText(QString::number(c, 'f'));
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
			if (m_contrast_coef_le->Validator()->bottom() < temp && temp < m_contrast_coef_le->Validator()->top()) {
				val /= pow(10, -exp);
				m_contrast_coef_exp->setCurrentIndex(exp);
			}
		}

		m_contrast_coef_le->setText(QString::number(val, 'f'));
		m_contrast_coef_slider->setSliderPosition(val * 100);
	}
	ApplytoPreview();
}

void AdaptiveStretchDialog::editingFinished_contrast_coef() {

	double val = m_contrast_coef_le->text().toDouble();
	m_contrast_coef_slider->setSliderPosition(val * 100);

	val *= pow(10, -m_contrast_coef_exp->currentIndex());
	m_contrast_le->setText(QString::number(val, 'f'));

	ApplytoPreview();
}

void AdaptiveStretchDialog::actionSlider_contrast_coef(int action) {
	if (action == 3 || action == 4) {
		sliderMoved_contrast_coef(m_contrast_coef_slider->sliderPosition());
		ApplytoPreview();
	}
}

void AdaptiveStretchDialog::sliderMoved_contrast_coef(int value) {
	if (m_contrast_coef_exp->currentText().toInt() == 0)
		return m_contrast_coef_slider->setSliderPosition(0);

	double c = (value - value % 5) / 100.0;

	QString str = QString::number(c, 'f');
	m_contrast_coef_le->Validator()->fixup(str);
	m_contrast_coef_le->setText(str);

	c = str.toDouble() * pow(10, -m_contrast_coef_exp->currentIndex());
	m_contrast_le->setText(QString::number(c, 'f'));

	m_timer->start();
	//ApplytoPreview();
}

void AdaptiveStretchDialog::itemSelected_contrast_coef(int index) {

	int value = m_contrast_coef_exp->itemText(index).toInt();

	if (value == 0) {
		m_contrast_le->setText(QString::number(1.0, 'f'));
		m_contrast_coef_le->setText(QString::number(1.0, 'f'));
		m_contrast_coef_slider->setSliderPosition(0);
	}
	else {
		double c = m_contrast_coef_le->text().toDouble();
		c *= pow(10, -m_contrast_coef_exp->currentIndex());
		m_contrast_le->setText(QString::number(c, 'f'));
	}

	ApplytoPreview();
}

void AdaptiveStretchDialog::onContrastStateChanged(int state) {

	if (state == Qt::Checked) {
		m_contrast_le->setEnabled(true);
		m_contrast_coef_le->setEnabled(true);
		m_contrast_coef_slider->setEnabled(true);
		m_contrast_coef_exp->setEnabled(true);
	}

	else if (state == Qt::Unchecked) {
		m_contrast_le->setDisabled(true);
		m_contrast_coef_le->setDisabled(true);
		m_contrast_coef_slider->setDisabled(true);
		m_contrast_coef_exp->setDisabled(true);
	}
}

void AdaptiveStretchDialog::editingFinished_data_pts() {}


void AdaptiveStretchDialog::AddNoiseThresholdInputs() {

	int dy = 20;
	m_noise_label = new QLabel("Noise Threshold:", this);
	m_noise_label->move(20, dy);

	m_noise_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6, this), this);
	m_noise_le->setGeometry(135, dy, 75, 25);

	m_noise_coef_le = new DoubleLineEdit("1.00", new DoubleValidator(1.00, 9.99, 2, this), this);
	m_noise_coef_le->setGeometry(215, dy, 50, 25);
	m_noise_coef_le->setMaxLength(4);
	connect(m_noise_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_noise);
	connect(m_noise_coef_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_noise_coef);

	m_noise_coef_slider = new QSlider(Qt::Horizontal, this);
	m_noise_coef_slider->setRange(100, 1000);
	m_noise_coef_slider->setFixedWidth(250);
	m_noise_coef_slider->move(275, dy);

	connect(m_noise_coef_slider, &QSlider::actionTriggered, this, &AdaptiveStretchDialog::actionSlider_noise_coef);
	connect(m_noise_coef_slider, &QSlider::sliderMoved, this, &AdaptiveStretchDialog::sliderMoved_noise_coef);

	m_noise_coef_exp = new QComboBox(this);
	m_noise_coef_exp->move(550, dy);
	m_noise_coef_exp->addItems({ "0","-1","-2","-3","-4","-5","-6","-7","-8"});
	m_noise_coef_exp->setCurrentIndex(3);
	connect(m_noise_coef_exp, &QComboBox::activated, this, &AdaptiveStretchDialog::itemSelected_noise_coef);

	m_noise_le->setText(QString::number(m_noise_coef_le->text().toDouble() * pow(10, -m_noise_coef_exp->currentIndex()), 'f'));

}

void AdaptiveStretchDialog::AddContrastProtectionInputs() {

	int dy = 50;
	m_contrast_label = new QLabel("Contrast:", this);
	m_contrast_label->move(20, dy);

	m_contrast_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6, this), this);
	m_contrast_le->setGeometry(135, dy, 75, 25);

	m_contrast_coef_le = new DoubleLineEdit("1.00", new DoubleValidator(1.00, 9.99, 2, this), this);
	m_contrast_coef_le->setGeometry(215, dy, 50, 25);
	m_contrast_coef_le->setMaxLength(4);
	connect(m_contrast_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_contrast);
	connect(m_contrast_coef_le, &QLineEdit::editingFinished, this, &AdaptiveStretchDialog::editingFinished_contrast_coef);

	m_contrast_coef_slider = new QSlider(Qt::Horizontal, this);
	m_contrast_coef_slider->setRange(100, 1000);
	m_contrast_coef_slider->setFixedWidth(250);
	m_contrast_coef_slider->move(275, dy);
	connect(m_contrast_coef_slider, &QSlider::actionTriggered, this, &AdaptiveStretchDialog::actionSlider_contrast_coef);
	connect(m_contrast_coef_slider, &QSlider::sliderMoved, this, &AdaptiveStretchDialog::sliderMoved_contrast_coef);

	m_contrast_coef_exp = new QComboBox(this);
	m_contrast_coef_exp->move(550, dy);
	m_contrast_coef_exp->addItems({ "0","-1","-2","-3","-4","-5","-6","-7","-8" });
	m_contrast_coef_exp->setCurrentIndex(4);
	connect(m_contrast_coef_exp, &QComboBox::activated, this, &AdaptiveStretchDialog::itemSelected_contrast_coef);

	m_contrast_le->setText(QString::number(m_contrast_coef_le->text().toDouble() * pow(10, -m_contrast_coef_exp->currentIndex()), 'f'));

	m_contrast_cb = new QCheckBox(this);
	//m_contrast_cb->setChecked(true);
	m_contrast_cb->move(600, dy+5);
	connect(m_contrast_cb, &QCheckBox::stateChanged, this, &AdaptiveStretchDialog::onContrastStateChanged);
	m_contrast_cb->stateChanged(m_contrast_cb->isChecked());
	//m_contrast_cb->

}

void AdaptiveStretchDialog::showPreview() {

	ProcessDialog::showPreview();
	ApplytoPreview();
}

void AdaptiveStretchDialog::resetDialog() {
	m_noise_coef_le->setText(QString::number(1.0, 'f'));
	m_noise_coef_slider->setSliderPosition(0);
	m_noise_coef_exp->setCurrentIndex(3);
	m_noise_le->setText(QString::number(0.001, 'f'));

	m_contrast_coef_le->setText(QString::number(1.0, 'f'));
	m_contrast_coef_slider->setSliderPosition(0);
	m_contrast_coef_exp->setCurrentIndex(4);
	m_contrast_le->setText(QString::number(0.0001, 'f'));
}

void AdaptiveStretchDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	m_as.setNoiseThreshold(m_noise_le->text().toFloat());
	m_as.setContrast(m_contrast_le->text().toFloat());
	m_as.setContrastProtection(m_contrast_cb->isChecked());
	m_as.setDataPoints(m_curve_pts_le->text().toInt());

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
		m_as.Apply(iw8->source);
		iw8->DisplayImage();
		if (iw8->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow32*>(iw8->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_as.Apply(iw16->source);
		iw16->DisplayImage();
		if (iw16->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow16*>(iw16->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_as.Apply(iw32->source);
		iw32->DisplayImage();
		if (iw32->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow32*>(iw32->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	}
}

void AdaptiveStretchDialog::ApplytoPreview() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	if (!reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->rtpExists())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	if (iwptr->rtp->windowTitle().sliced(19, iwptr->rtp->windowTitle().length() - 19).compare(Name()) != 0)
		return;

	m_as.setNoiseThreshold(m_noise_le->text().toFloat());
	m_as.setContrast(m_contrast_le->text().toFloat());
	m_as.setContrastProtection(m_contrast_cb->isChecked());
	m_as.setDataPoints(m_curve_pts_le->text().toInt());


	switch (iwptr->source.Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
		auto rtp8 = reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp);
		m_as.ApplytoDest(iw8->source, rtp8->source, rtp8->BinFactor());
		rtp8->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		auto rtp16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		m_as.ApplytoDest(iw16->source, rtp16->source, rtp16->BinFactor());
		rtp16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		auto rtp32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		m_as.ApplytoDest(iw32->source, rtp32->source, rtp32->BinFactor());
		rtp32->DisplayImage();
		break;
	}
	}
}