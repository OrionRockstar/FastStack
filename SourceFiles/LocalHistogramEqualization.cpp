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

	m_radius = kernel_radius;
	m_is_circular = circular;
	m_dimension = 2 * kernel_radius + 1;

	int total = m_dimension * m_dimension;
	if (m_is_circular) {

		k_mask.resize(total, false);
		back_pix.resize(m_dimension);
		front_pix.resize(m_dimension);

		for (int j = 0; j < m_dimension; ++j) {

			bool new_x = true;
			for (int i = 0; i < m_dimension; ++i) {


				int dx = i - kernel_radius;
				int dy = j - kernel_radius;
				int loc = j * m_dimension + i;

				if (sqrt(dx * dx + dy * dy) <= kernel_radius) {
					k_mask[loc] = true;
					m_count++;
				}

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
		k_mask.resize(total, true);
		back_pix.resize(m_dimension, -kernel_radius);
		front_pix.resize(m_dimension, kernel_radius);
		m_count = total;
	}

}

template<typename T>
void LHE::KernelHistogram::Populate(Image<T>& img, int y) {

	for (int j = -m_radius, j_mask = 0; j <= m_radius; ++j, j_mask += m_dimension) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		for (int i = -m_radius, i_m = 0; i <= m_radius; ++i, ++i_m) {

			int xx = i;
			if (xx < 0)
				xx = -xx;

			if (k_mask[j_mask + i_m]) 
				histogram[Pixel<float>::toType(img(xx,yy)) * m_multiplier]++;
			
		}
	}
}
template void LHE::KernelHistogram::Populate(Image8&, int);
template void LHE::KernelHistogram::Populate(Image16&, int);
template void LHE::KernelHistogram::Populate(Image32&, int);

template<typename T>
void LHE::KernelHistogram::Update(Image<T>& img, int x, int y) {

	for (int j = -m_radius, s = 0; j <= m_radius; ++j, ++s) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		//front
		int xx = x + front_pix[s];

		if (xx >= img.cols())
			xx = 2 * img.cols() - (xx + 1);

		histogram[Pixel<float>::toType(img(xx, yy)) * m_multiplier]++;

		//back
		xx = x + back_pix[s] - 1;

		if (xx < 0)
			xx = -xx;

		histogram[Pixel<float>::toType(img(xx, yy)) * m_multiplier]--;
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
void LHE::apply(Image<T>& img) {

	float original_amount = 1.0 - m_amount;

	Image<T> temp;

	if (img.channels() == 3) {
		m_ps.emitText("Getting CIE Luminance...");
		img.RGBtoCIELab();
		img.copyTo(temp);
	}
	else
		temp = Image<T>(img);

	int sum = 0;
	m_ps.emitText("CLAHE...");

#pragma omp parallel for
	for (int y = 0; y < img.rows(); ++y) {

		KernelHistogram k_hist(m_hist_res, m_kernel_radius, m_is_circular);
		k_hist.Populate(img, y);

		int limit = (m_contrast_limit * k_hist.Count()) / (k_hist.Size() - 1);

		if (limit == 0)
			limit = 1;

		for (int x = 0; x < img.cols(); ++x) {
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
		}

#pragma omp atomic
		++sum;

		if (omp_get_thread_num() == 0) 
			m_ps.emitProgress((sum * 100) / img.rows());		
	}

	m_ps.emitProgress(100);

	temp.moveTo(img);

	if (img.channels() == 3) {
		m_ps.emitText("CIE Lab to RGB...");
		img.CIELabtoRGB();
	}
}
template void LHE::apply(Image8&);
template void LHE::apply(Image16&);
template void LHE::apply(Image32&);






//move labels to be align by colons
//ensure kernel size is not bigger than image
using LHED = LocalHistogramEqualizationDialog;
LHED::LocalHistogramEqualizationDialog(QWidget* parent) : ProcessDialog("LocalHistogramEqualization", QSize(475, 170), FastStack::recast(parent)->workspace()) {

	setTimer(500, this, &LHED::applytoPreview);

	connectToolbar(this, &LHED::apply, &LHED::showPreview, &LHED::resetDialog);
	connectZoomWindow(this, &LHED::applytoPreview);

	addKernelRadiusInputs();
	addContrastLimitInputs();
	addAmountInputs();

	m_circular = new CheckBox("Circular Kernel", this);
	m_circular->move(300, 110);
	m_circular->setChecked(true);
	m_lhe.setCircularKernel(m_circular->isChecked());

	auto onClicked = [this](bool v) {
		m_lhe.setKernelRadius(v);
		applytoPreview();
	};

	connect(m_circular, &QCheckBox::clicked, this, onClicked);

	m_histogram_resolution = new ComboBox(this);
	m_histogram_resolution->move(175, 110);
	m_histogram_resolution->addItems({"8-bit", "10-bit", "12-bit"});
	m_histogram_resolution->addLabel(new QLabel("Histogram Resolution:   ", this));

	auto activated = [this](int index) {
		m_lhe.setHistogramResolution(m_res[index]);
		applytoPreview();
	};

	connect(m_histogram_resolution, &QComboBox::activated, this, activated);

	this->showDialog();
}

void LHED::addKernelRadiusInputs() {

	m_kr_le = new IntLineEdit(64, new IntValidator(16, 512), this);
	m_kr_le->setGeometry(125, 15, 65, 25);
	m_kr_le->setMaxLength(3);
	m_kr_le->addLabel(new QLabel("Kernel Radius:   ", this));

	m_kr_slider = new Slider(Qt::Horizontal, this);
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

	m_cl_le = new DoubleLineEdit(2.0, new DoubleValidator(1.0, 64, 1), this);
	m_cl_le->setGeometry(125, 45, 65, 25);
	m_cl_le->addLabel(new QLabel("Contrast Limit:   ", this));

	m_cl_slider = new Slider(Qt::Horizontal, this);
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

	m_amount_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 3), 5,this);
	m_amount_le->setGeometry(125, 75, 65, 25);
	m_amount_le->addLabel(new QLabel("Amount:   ", this));

	m_amount_slider = new Slider(Qt::Horizontal, this);
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

	setEnabledAll(false);

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
		setEnabledAll(true);
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

	if (iwptr->parentWindow()->zoomWindow())
		m_lhe.setKernelRadius(m_kr_le->value() * iwptr->zoomWindowScaleFactor());
	else
		m_lhe.setKernelRadius(m_kr_le->value() / iwptr->parentWindow()->computeZoomFactor());

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