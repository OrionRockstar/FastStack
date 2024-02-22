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
LHED::LocalHistogramEqualizationDialog(QWidget* parent) : QDialog(parent) {

	m_workspace = reinterpret_cast<FastStack*>(parentWidget())->workspace;

	this->resize(500, 170);

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

	tb = new Toolbar(this);
	connect(tb, &Toolbar::sendApply, this, &LHED::Apply);
	connect(tb, &Toolbar::sendPreview, this, &LHED::showPreview);
	connect(tb, &Toolbar::sendReset, this, &LHED::resetDialog);

	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();
}

void LHED::AddKernelRadiusInputs() {

	int dy = 15;

	m_kernelradius_label = new QLabel(this);
	m_kernelradius_label->setText("Kernel Radius: ");
	m_kernelradius_label->move(30, dy);

	m_kernelradius_iv = new QIntValidator(16, 512, this);

	m_kernelradius_le = new QLineEdit(this);
	m_kernelradius_le->setGeometry(150, dy, 65, 25);
	m_kernelradius_le->setText("64");
	m_kernelradius_le->setMaxLength(3);
	m_kernelradius_le->setValidator(m_kernelradius_iv);

	m_kernelradius_slider = new QSlider(Qt::Horizontal, this);
	m_kernelradius_slider->setRange(16, 512);
	m_kernelradius_slider->setValue(64);
	m_kernelradius_slider->setFixedWidth(250);
	m_kernelradius_slider->move(225, dy);

	connect(m_kernelradius_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_kernelradius);
	connect(m_kernelradius_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::AddContrastLimitInputs() {

	int dy = 45;

	m_contrastlimit_label = new QLabel(this);
	m_contrastlimit_label->setText("Contrast Limit: ");
	m_contrastlimit_label->move(30, dy);

	m_contrastlimit_dv = new QDoubleValidator(1.0, 64, 1, this);

	m_contrastlimit_le = new QLineEdit(this);
	m_contrastlimit_le->setGeometry(150, dy, 65, 25);
	m_contrastlimit_le->setText("2.0");
	m_contrastlimit_le->setMaxLength(3);
	m_contrastlimit_le->setValidator(m_contrastlimit_dv);

	m_contrastlimit_slider = new QSlider(Qt::Horizontal, this);
	m_contrastlimit_slider->setRange(2, 74);
	m_contrastlimit_slider->setValue(4);
	m_contrastlimit_slider->setFixedWidth(250);
	m_contrastlimit_slider->move(225, dy);

	connect(m_contrastlimit_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_contrastlimit);
	connect(m_contrastlimit_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::AddAmountInputs() {

	int dy = 75;

	m_amount_label = new QLabel(this);
	m_amount_label->setText("Amount: ");
	m_amount_label->move(30, dy);

	m_amount_dv = new QDoubleValidator(0.0, 1.0, 3, this);

	m_amount_le = new QLineEdit(this);
	m_amount_le->setGeometry(150, dy, 65, 25);
	m_amount_le->setText("1.000");
	m_amount_le->setMaxLength(5);
	m_amount_le->setValidator(m_amount_dv);

	m_amount_slider = new QSlider(Qt::Horizontal, this);
	m_amount_slider->setRange(0, 100);
	m_amount_slider->setValue(100);
	m_amount_slider->setFixedWidth(250);
	m_amount_slider->move(225, dy);

	connect(m_amount_slider, &QSlider::sliderMoved, this, &LHED::sliderMoved_amount);
	connect(m_amount_slider, &QSlider::sliderReleased, this, &LHED::ApplytoPreview);
}

void LHED::showPreview() {
	
	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		iwptr->ShowRTP();
		break;
	}
	case 16: {
		reinterpret_cast<ImageWindow16*>(iwptr)->ShowRTP();
		break;
	}
	case -32: {
		reinterpret_cast<ImageWindow32*>(iwptr)->ShowRTP();
		break;
	}
	}

	iwptr->rtp->setWindowTitle("Real-Time Preview: " + m_name);
	ApplytoPreview();
}

void LHED::resetDialog() {

	m_kernelradius_le->setText("64");
	m_kernelradius_slider->setValue(64);

	m_contrastlimit_le->setText("2.0");
	m_contrastlimit_slider->setValue(4);

	m_amount_le->setText("1.000");
	m_amount_slider->setValue(100);

	m_circular->setChecked(true);
}

void LHED::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	m_lhe.setHistogramResolution(m_res[m_histogram_resolution->currentIndex()]);
	m_lhe.setContrastLimit(m_contrastlimit_le->text().toFloat());
	m_lhe.setKernelRadius(m_kernelradius_le->text().toInt());
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
	m_lhe.setContrastLimit(m_contrastlimit_le->text().toFloat());
	m_lhe.setAmount(m_amount_le->text().toFloat());
	m_lhe.setCircularKernel(m_circular->isChecked());

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	m_lhe.setKernelRadius(m_kernelradius_le->text().toInt() / iwptr->IdealFactor());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		m_lhe.Apply(reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->modified);
		iwptr->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		m_lhe.Apply(iw16->modified);
		iw16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		m_lhe.Apply(iw32->modified);
		iw32->DisplayImage();
		break;
	}
	}
}