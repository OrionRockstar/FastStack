#include "pch.h"
#include "FastStack.h"
#include "LocalHistogramEqualization.h"


using LHE = LocalHistogramEqualization;

LHE::KernelHistogram::KernelHistogram(int resolution) {
	switch (resolution) {

	case 8:
		m_size = 256;
		break;

	case 10:
		m_size = 1024;
		break;

	case 12:
		m_size = 4096;
		break;

	default:
		m_size = 256;
	}
	m_multiplier = m_size - 1;
	histogram = std::make_unique<uint32_t[]>(m_size);

}

LHE::KernelHistogram::KernelHistogram(int resolution, int kernel_radius, bool circular) : KernelHistogram(resolution) {

	m_kernel_radius = kernel_radius;
	m_is_circular = circular;

	int kernel_dimension = 2 * kernel_radius + 1;

	if (m_is_circular) {

		k_mask.resize(kernel_dimension * kernel_dimension, false);
		back_pix.resize(kernel_dimension);
		front_pix.resize(kernel_dimension);

		for (int j = 0; j < kernel_dimension; ++j) {

			bool new_x = true;
			for (int i = 0; i < kernel_dimension; ++i) {


				int dx = i - kernel_radius;
				int dy = j - kernel_radius;
				int loc = j * kernel_dimension + i;

				if (sqrt(dx * dx + dy * dy) <= kernel_radius)
					k_mask[loc] = true;


				if (new_x && k_mask[loc]) {
					back_pix[j] = dx;
					front_pix[j] = dx;
					new_x = false;
				}

				else if (!new_x && k_mask[loc])
					front_pix[j] = dx;

			}
		}
	}

	else {
		k_mask.resize(kernel_dimension * kernel_dimension, true);
		back_pix.resize(kernel_dimension, -kernel_radius);
		front_pix.resize(kernel_dimension, kernel_radius);
	}
}

template<typename T>
void LHE::KernelHistogram::Populate(Image<T>& img, int y) {

	int kernel_dimension = 2 * m_kernel_radius + 1;
	float pix;

	for (int j = -m_kernel_radius, j_m = 0; j <= m_kernel_radius; ++j, ++j_m) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		if (yy >= img.Rows())
			yy = 2 * img.Rows() - (yy + 1);

		for (int i = -m_kernel_radius, i_m = 0; i <= m_kernel_radius; ++i, ++i_m) {

			int xx = i;
			if (xx < 0)
				xx = -xx;

			if (k_mask[j_m * kernel_dimension + i_m]) {
				Pixel<T>::fromType(img(xx, yy), pix);
				histogram[pix * m_multiplier]++;
				pix_count++;
			}
		}
	}
}
template void LHE::KernelHistogram::Populate(Image8&, int);
template void LHE::KernelHistogram::Populate(Image16&, int);
template void LHE::KernelHistogram::Populate(Image32&, int);

template<typename T>
void LHE::KernelHistogram::Update(Image<T>& img, int x, int y) {

	float pix;

	for (int j = -m_kernel_radius; j <= m_kernel_radius; ++j) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		if (yy >= img.Rows())
			yy = 2 * img.Rows() - (yy + 1);

		int xx = x + front_pix[j + m_kernel_radius];

		if (xx >= img.Cols())
			xx = 2 * img.Cols() - (xx + 1);

		Pixel<T>::fromType(img(xx, yy), pix);

		histogram[pix * m_multiplier]++;


		xx = x + back_pix[j + m_kernel_radius] - 1;

		if (xx < 0)
			xx = -xx;

		Pixel<T>::fromType(img(xx, yy), pix);

		histogram[pix * m_multiplier]--;
	}
}
template void LHE::KernelHistogram::Update(Image8&, int, int);
template void LHE::KernelHistogram::Update(Image16&, int, int);
template void LHE::KernelHistogram::Update(Image32&, int, int);

void LHE::KernelHistogram::Clip(int limit) {

	for (int iter = 0; iter < 5; ++iter) {
		int clip_count = 0;

		for (int el = 0; el < m_size; ++el)
			if (histogram[el] > limit) {
				clip_count += (histogram[el] - limit);
				histogram[el] = limit;
			}

		int d = clip_count / m_size; // evenly distributes clipped values to histogram
		int r = clip_count % m_size; // dristubues remainder of clipped values 

		if (d != 0)
			for (int el = 0; el < m_size; ++el)
				histogram[el] += d;

		if (r != 0) {
			int skip = (m_size - 1) / r;
			for (int el = 0; el < m_size; el += skip)
				histogram[el]++;
		}

		if (r == 0 && d == 0)
			break;
	}
}

void LHE::KernelHistogram::CopyTo(KernelHistogram& other) {
	if (this->m_size != other.m_size)
		return;
	memcpy(other.histogram.get(), histogram.get(), m_size * 4);
}

template<typename T>
void LHE::Apply(Image<T>& img) {

	float original_amount = 1.0 - m_amount;

	Image<T> temp(img);

	if (img.Channels() == 3) {
		img.RGBtoCIELab();
		img.CopyTo(temp);
	}

#pragma omp parallel for
	for (int y = 0; y < img.Rows(); ++y) {
		KernelHistogram k_hist(m_hist_res, m_kernel_radius, m_is_circular);
		k_hist.Populate(img, y);

		int limit = (m_contrast_limit * k_hist.pix_count) / (k_hist.m_size - 1);

		if (limit == 0)
			limit = 1;

		for (int x = 0; x < img.Cols(); ++x) {
			float pixel = Pixel<float>::toType(img(x, y));

			if (x != 0)
				k_hist.Update(img, x, y);

			KernelHistogram k_hist_cl(m_hist_res);
			k_hist.CopyTo(k_hist_cl);

			k_hist_cl.Clip(limit);

			int sum = 0;
			int val = pixel * k_hist.Multiplier();

			for (int el = 0; el <= val; ++el)
				sum += k_hist_cl[el];

			int cdf = sum;
			int min = 0;

			for (int el = 0; el < k_hist_cl.Size(); ++el)
				if (k_hist_cl[el] != 0) { min = k_hist_cl[el]; break; }

			for (int el = val + 1; el < k_hist_cl.Size(); ++el)
				sum += k_hist_cl[el];

			temp(x, y) = Pixel<T>::toType((original_amount * pixel) + (m_amount * float(cdf - min) / (sum - min)));
			//Pixel::fromFloat((original_amount * pixel) + (m_amount * float(cdf - min) / (sum - min)), temp(x,y));
		}

	}
	img = std::move(temp);

	if (img.Channels() == 3)
		img.CIELabtoRGB();
}
template void LHE::Apply(Image8&);
template void LHE::Apply(Image16&);
template void LHE::Apply(Image32&);











using LHED = LocalHistogramEqualizationDialog;
LHED::LocalHistogramEqualizationDialog(QWidget* parent) : ProcessDialog("LocalHistogramEqualization", parent) {

	this->setWindowTitle(Name());
	this->resize(500, 170);
	this->setFocus();

	m_timer = new Timer(500, this);
	connect(m_timer, &QTimer::timeout, this, &LHED::ApplytoPreview);

	setWorkspace(reinterpret_cast<FastStack*>(parentWidget())->workspace);
	setToolbar(new Toolbar(this));

	connect(m_tb, &Toolbar::sendApply, this, &LHED::Apply);
	connect(m_tb, &Toolbar::sendPreview, this, &LHED::showPreview);
	connect(m_tb, &Toolbar::sendReset, this, &LHED::resetDialog);


	AddKernelRadiusInputs();
	AddContrastLimitInputs();
	AddAmountInputs();

	m_circular = new QCheckBox(this);
	m_circular->move(300, 115);
	m_circular->setChecked(true);

	m_circular_label = new QLabel(this);
	m_circular_label->setText("Circular Kernel");
	m_circular_label->move(325, 113);

	m_histogram_resolution = new QComboBox(this);
	m_histogram_resolution->move(50, 110);
	m_histogram_resolution->addItems({"8-bit", "10-bit", "12-bit"});
	connect(m_histogram_resolution, &QComboBox::activated, this, &LHED::itemSelected);
	

	m_hr_label = new QLabel(this);
	m_hr_label->setText("Histogram Resolution");
	m_hr_label->move(125, 113);
	//apply->setIcon(QStyle::standardIcon(QStyle::SP_MediaPlay));
	//apply->setIcon(apply->style()->standardIcon(QStyle::SP_MediaPlay));
	//apply->icon();
	//apply->setStyleSheet("QIcon {color : blue;}");

	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}

void LHED::actionSlider_kr(int action) {
	if (action == 3 || action == 4) {
		m_kr_le->setText(QString::number((m_kr_slider->sliderPosition() / 2) * 2));
		ApplytoPreview();
	}
}

void LHED::sliderMoved_kr(int value) {
	m_kr_le->setText(QString::number((value / 2) * 2));
	m_timer->start();
	//ApplytoPreview();
}


void LHED::actionSlider_cl(int action) {
	if (action == 3 || action == 4) {
		int value = m_cl_slider->sliderPosition();

		if (value < 20)
			m_cl_le->setText(QString::number(value / 2.0, 'f', 1));
		else
			m_cl_le->setText(QString::number(value - 10));

		ApplytoPreview();
	}
}

void LHED::sliderMoved_cl(int value) {
	if (value < 20)
		m_cl_le->setText(QString::number(value / 2.0, 'f', 1));
	else
		m_cl_le->setText(QString::number(value - 10));
	m_timer->start();
	//ApplytoPreview();
}


void LHED::actionSlider_amount(int action) {
	if (action == 3 || action == 4) {
		m_amount_le->setText(QString::number(m_amount_slider->sliderPosition() / 100.0, 'f'));
		ApplytoPreview();
	}
}

void LHED::sliderMoved_amount(int value) {
	m_amount_le->setText(QString::number(value / 100.0, 'f'));
	m_timer->start();
	//ApplytoPreview();
}


void LHED::itemSelected(int index) {
	ApplytoPreview();
}

void LHED::AddKernelRadiusInputs() {

	int dy = 15;

	m_kr_label = new QLabel(this);
	m_kr_label->setText("Kernel Radius: ");
	m_kr_label->move(30, dy);


	m_kr_le = new QLineEdit("64", this);
	m_kr_le->setGeometry(150, dy, 65, 25);
	m_kr_le->setMaxLength(3);
	m_kr_le->setValidator(new QIntValidator(16, 512, this));

	m_kr_slider = new QSlider(Qt::Horizontal, this);
	m_kr_slider->setRange(16, 512);
	m_kr_slider->setValue(64);
	m_kr_slider->setFixedWidth(250);
	m_kr_slider->move(225, dy);

	connect(m_kr_slider, &QSlider::actionTriggered, this, &LHED::actionSlider_kr);
	connect(m_kr_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_kr);
	//connect(m_kr_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::AddContrastLimitInputs() {

	int dy = 45;

	m_cl_label = new QLabel(this);
	m_cl_label->setText("Contrast Limit: ");
	m_cl_label->move(30, dy);

	m_cl_le = new DoubleLineEdit("2.0", new DoubleValidator(1.0, 64, 1, this), this);
	m_cl_le->setGeometry(150, dy, 65, 25);
	m_cl_le->setMaxLength(3);

	m_cl_slider = new QSlider(Qt::Horizontal, this);
	m_cl_slider->setRange(2, 74);
	m_cl_slider->setValue(4);
	m_cl_slider->setFixedWidth(250);
	m_cl_slider->move(225, dy);

	connect(m_cl_slider, &QSlider::actionTriggered, this, &LHED::actionSlider_cl);
	connect(m_cl_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_cl);
	//connect(m_cl_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::AddAmountInputs() {

	int dy = 75;

	m_amount_label = new QLabel(this);
	m_amount_label->setText("Amount: ");
	m_amount_label->move(30, dy);


	m_amount_le = new DoubleLineEdit("1.000", new DoubleValidator(0.0, 1.0, 3, this), this);
	m_amount_le->setGeometry(150, dy, 65, 25);
	m_amount_le->setMaxLength(5);

	m_amount_slider = new QSlider(Qt::Horizontal, this);
	m_amount_slider->setRange(0, 100);
	m_amount_slider->setValue(100);
	m_amount_slider->setFixedWidth(250);
	m_amount_slider->move(225, dy);

	connect(m_amount_slider, &QSlider::actionTriggered, this, &LHED::actionSlider_amount);
	connect(m_amount_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_amount);
	//connect(m_amount_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::showPreview() {
	
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void LHED::resetDialog() {

	m_kr_le->setText("64");
	m_kr_slider->setValue(64);

	m_cl_le->setText("2.0");
	m_cl_slider->setValue(4);

	m_amount_le->setText("1.000");
	m_amount_slider->setValue(100);

	m_circular->setChecked(true);
}

void LHED::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	m_lhe.setHistogramResolution(m_res[m_histogram_resolution->currentIndex()]);
	m_lhe.setContrastLimit(m_cl_le->text().toFloat());
	m_lhe.setKernelRadius(m_kr_le->text().toInt());
	m_lhe.setAmount(m_amount_le->text().toFloat());
	m_lhe.setCircularKernel(m_circular->isChecked());

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		m_lhe.Apply(iwptr->source);
		iwptr->DisplayImage();
		if (iwptr->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_lhe.Apply(iw16->source);
		iw16->DisplayImage();
		if (iw16->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow16*>(iw16->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_lhe.Apply(iw32->source);
		iw32->DisplayImage();
		if (iw32->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow32*>(iw32->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	}
}

void LHED::ApplytoPreview() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	if (!reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->rtpExists())
		return;

	m_lhe.setHistogramResolution(m_res[m_histogram_resolution->currentIndex()]);
	m_lhe.setContrastLimit(m_cl_le->text().toFloat());
	m_lhe.setAmount(m_amount_le->text().toFloat());
	m_lhe.setCircularKernel(m_circular->isChecked());

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	m_lhe.setKernelRadius(m_kr_le->text().toInt() / iwptr->IdealFactor());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp);
		iw8->UpdatefromParent();
		m_lhe.Apply(iw8->source);
		iwptr->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		iw16->UpdatefromParent();
		m_lhe.Apply(iw16->source);
		iw16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		iw32->UpdatefromParent();
		m_lhe.Apply(iw32->source);
		iw32->DisplayImage();
		break;
	}
	}
}