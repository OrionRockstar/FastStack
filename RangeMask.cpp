#include "pch.h"
#include "RangeMask.h"
#include "GaussianFilter.h"
#include "FastStack.h"

template <typename T>
Image<T> RangeMask::GenerateMask(const Image<T>& img) {

    int ch = (m_lightness && img.channels() == 3) ? 1 : img.channels();

    Image<T> mask(img.rows(), img.cols(), ch);

    float z = (m_high - m_low) * m_fuzziness * 0.5;

    float a_p = m_low + z;
    float b_p = m_high - z;

    float base_a = m_low;
    float base_b = b_p;

#pragma omp parallel for num_threads(mask.channels())
	for (int ch = 0; ch < mask.channels(); ++ch) {
		for (int y = 0; y < mask.rows(); ++y) {
			for (int x = 0; x < mask.cols(); ++x) {
				float pixel = 0;

				if (m_lightness && img.channels() == 3)
					pixel = ColorSpace::CIEL(img.color<double>(x, y));

				else
					pixel = Pixel<float>::toType(img(x, y, ch));


				if (m_low <= pixel && pixel <= m_high) {
					if (pixel < a_p)
						pixel = (pixel - base_a) / z;
					else if (pixel > b_p)
						pixel = 1 - (pixel - base_b) / z;
					else
						pixel = 1;
				}

				else
					pixel = 0;

				if (m_screening)
					pixel *= pixel;

				if (m_invert)
					pixel = 1 - pixel;

				mask(x, y, ch) = Pixel<T>::toType(pixel);
			}
		}
    }

    if (m_smoothness != 0.0)
        GaussianFilter(m_smoothness).Apply(mask);

    return mask;
}
template Image8 RangeMask::GenerateMask(const Image8&);
template Image16 RangeMask::GenerateMask(const Image16&);
template Image32 RangeMask::GenerateMask(const Image32&);

template<typename T>
Image<T> RangeMask::GenerateMask_Reduced(const Image<T>& img, int factor) {

	factor = Max(factor, 1);

	int ch = (m_lightness && img.channels() == 3) ? 1 : img.channels();

	Image<T> mask(img.rows() / factor, img.cols() / factor, ch);

	float z = (m_high - m_low) * m_fuzziness * 0.5;

	float a_p = m_low + z;
	float b_p = m_high - z;

	float base_a = m_low;
	float base_b = b_p;

	for (int ch = 0; ch < mask.channels(); ++ch) {
		for (int y = 0, y_s = 0; y < mask.rows(); ++y, y_s+=factor) {
			for (int x = 0, x_s = 0; x < mask.cols(); ++x, x_s+=factor) {
				float pixel = 0;

				if (m_lightness && img.channels() == 3) 
					pixel = ColorSpace::CIEL(img.color<double>(x_s, y_s));

				else
					pixel = Pixel<float>::toType(img(x_s, y_s, ch));

				if (m_low <= pixel && pixel <= m_high) {
					if (pixel < a_p)
						pixel = (pixel - base_a) / z;
					else if (pixel > b_p)
						pixel = 1 - (pixel - base_b) / z;
					else
						pixel = 1;
				}

				else
					pixel = 0;

				if (m_screening)
					pixel *= pixel;

				if (m_invert)
					pixel = 1 - pixel;

				mask(x, y, ch) = Pixel<T>::toType(pixel);
			}
		}
	}

	if (m_smoothness != 0.0)
		GaussianFilter(m_smoothness).Apply(mask);

	return mask;
}
template Image8 RangeMask::GenerateMask_Reduced(const Image8&, int);
template Image16 RangeMask::GenerateMask_Reduced(const Image16&, int);
template Image32 RangeMask::GenerateMask_Reduced(const Image32&, int);

template<typename T>
void RangeMask::GenerateMask_ReducedTo(const Image<T>& src, Image<T>& mask, int factor) {

	factor = Max(factor, 1);

	int ch = (m_lightness && src.channels() == 3) ? 1 : src.channels();

	if (mask.rows() != src.rows() / factor || mask.cols() != src.cols() / factor || mask.channels() != ch)
		mask = Image<T>(src.rows() / factor, src.cols() / factor, ch);

	float z = (m_high - m_low) * m_fuzziness * 0.5;

	float a_p = m_low + z;
	float b_p = m_high - z;

	float base_a = m_low;
	float base_b = b_p;

	for (int ch = 0; ch < mask.channels(); ++ch) {
		for (int y = 0, y_s = 0; y < mask.rows(); ++y, y_s += factor) {
			for (int x = 0, x_s = 0; x < mask.cols(); ++x, x_s += factor) {
				float pixel = 0;

				if (m_lightness && src.channels() == 3)
					pixel = ColorSpace::CIEL(src.color<double>(x_s, y_s));

				else
					pixel = Pixel<float>::toType(src(x_s, y_s, ch));

				if (m_low <= pixel && pixel <= m_high) {
					if (pixel < a_p)
						pixel = (pixel - base_a) / z;
					else if (pixel > b_p)
						pixel = 1 - (pixel - base_b) / z;
					else
						pixel = 1;
				}

				else
					pixel = 0;

				if (m_screening)
					pixel *= pixel;

				if (m_invert)
					pixel = 1 - pixel;

				mask(x, y, ch) = Pixel<T>::toType(pixel);
			}
		}
	}

	if (m_smoothness != 0.0)
		GaussianFilter(m_smoothness).Apply(mask);

}



RangeSlider::RangeSlider(Qt::Orientation orientation, QWidget* parent) : QSlider(orientation, parent) {
	initStyleOption(&m_low);
	m_low.sliderPosition = 0;
	m_low.subControls = QStyle::SC_SliderHandle;

	initStyleOption(&m_high);
	m_high.sliderPosition = maximum();
	m_high.subControls = QStyle::SC_SliderHandle;
}

int RangeSlider::sliderPosition_low()const { return m_low.sliderPosition; }

void RangeSlider::setValue_low(int pos) {
	m_low.sliderValue = m_low.sliderPosition = pos;
	emit valueChanged(pos);
	update();
}

int RangeSlider::sliderPosition_high()const { return m_high.sliderPosition; }

void RangeSlider::setValue_high(int pos) {
	m_high.sliderValue = m_high.sliderPosition = pos;
	emit valueChanged(pos);
	update();
}

void RangeSlider::resetSliderPositions() {
	m_low.sliderPosition = 0;
	m_high.sliderPosition = maximum();

	emit sliderMoved_low(m_low.sliderPosition);
	emit sliderMoved_high(m_high.sliderPosition);

	update();
}



void RangeSlider::mousePressEvent(QMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {
		QStyle* style = QApplication::style();
		QStyleOptionSlider opt;
		initStyleOption(&opt);

		if (style->hitTestComplexControl(QStyle::CC_Slider, &opt, event->pos(), this) == QStyle::SC_SliderGroove)
			m_low_act = m_high_act = false;


		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_low, event->pos(), this) == QStyle::SC_SliderHandle) {
			click_x = event->x();
			m_low_act = true;
			m_high_act = false;
			this->triggerAction(this->SliderMove);
			this->setRepeatAction(this->SliderNoAction);
			this->setSliderDown(true);
		}

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_high, event->pos(), this) == QStyle::SC_SliderHandle) {
			click_x = event->x();
			m_high_act = true;
			m_low_act  = false;
			this->triggerAction(this->SliderMove);
			this->setRepeatAction(this->SliderNoAction);
			this->setSliderDown(true);
		}
	}
}

void RangeSlider::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::LeftButton) {

		if (m_low_act) {
			int x = m_low.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < minimum())
				x = 0;

			else if (x >= m_high.sliderPosition && x <= maximum()) {
				x = m_high.sliderValue = m_high.sliderPosition = m_high.sliderPosition + 1;
				emit sliderMoved_high(m_high.sliderPosition);
			}

			m_low.sliderValue = m_low.sliderPosition = x;

			emit sliderMoved_low(m_low.sliderPosition);
		}


		if (m_high_act) {
			int x = m_high.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x > maximum())
				x = maximum();

			else if (x <= m_low.sliderPosition && x >= minimum()) {
				x = m_low.sliderValue = m_low.sliderPosition = m_low.sliderPosition - 1;
				emit sliderMoved_low(m_low.sliderPosition);
			}

			m_high.sliderValue = m_high.sliderPosition = x;

			emit sliderMoved_high(m_high.sliderPosition);
		}

	}

	update();
}

void RangeSlider::paintEvent(QPaintEvent* event) {

	QPainter p(this);
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QPalette pal;
	pal.setBrush(QPalette::Highlight, QColor(123, 0, 216));

	//trackbar
	opt.subControls = QStyle::SC_SliderGroove;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);


	QRect r = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove);
	r.adjust(2, 2, -2, -2);

	opt.subControls = QStyle::SC_SliderHandle;

	pal.setBrush(QPalette::Button, QColor(129, 129, 129));
	opt.palette = pal;

	//low handle
	opt.sliderPosition = m_low.sliderPosition;	

	QRect low = r;
	m_low.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	low.setRight(m_low.rect.left());
	p.fillRect(low, pal.color(QPalette::Highlight));
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);


	//high handle
	opt.sliderPosition = m_high.sliderPosition;

	QRect high = r;
	m_high.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	high.setLeft(m_high.rect.right());
	p.fillRect(high, pal.color(QPalette::Highlight));
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}






RangeMaskDialog::RangeMaskDialog(QWidget* parent) : ProcessDialog("RangeMask", QSize(455, 215), FastStack::recast(parent)->workspace()) {
    
    setTimer(250, this, &RangeMaskDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &RangeMaskDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &RangeMaskDialog::Apply, &RangeMaskDialog::showPreview, &RangeMaskDialog::resetDialog);

	addRangeSlider();
	addFuzzinessInputs();
	addSmoothnessInputs();

	int dx = 90;
	m_lightness_cb = new CheckBox("Lightness", this);
	m_lightness_cb->setChecked(true);
	m_lightness_cb->move(dx, 150);
	connect(m_lightness_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setLightness(checked); ApplytoPreview(); });

	m_screening_cb = new CheckBox("Screening", this);
	m_screening_cb->move(dx + 110, 150);
	connect(m_screening_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setScreening(checked); ApplytoPreview(); });

	m_invert_cb = new CheckBox("Invert", this);
	m_invert_cb->move(dx + 220, 150);
	connect(m_invert_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setInvert(checked); ApplytoPreview(); });
    
    this->show();
}

void RangeMaskDialog::onEditingFinished_low() {
	double low = m_low_le->value();
	int pos = low * m_range_slider->maximum();
	m_range_slider->setValue_low(pos);
	m_rm.setLow(low);

	if (pos > m_range_slider->sliderPosition_high())
		m_range_slider->setValue_high(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onSliderMoved_low(int value) {

	float low = float(value) / m_range_slider->maximum();
	m_low_le->setValue(low);
	m_rm.setLow(low);

	startTimer();
}

void RangeMaskDialog::onEditingFinished_high() {

	double high = m_high_le->value();
	int pos = high * m_range_slider->maximum();
	m_range_slider->setValue_high(pos);
	m_rm.setHigh(high);

	if (pos < m_range_slider->sliderPosition_low())
		m_range_slider->setValue_low(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onSliderMoved_high(int value) {

	float high = float(value) / m_range_slider->maximum();
	m_high_le->setValue(high);
	m_rm.setHigh(high);

	startTimer();
}

void RangeMaskDialog::addRangeSlider() {

	m_range_slider = new RangeSlider(Qt::Horizontal, this);
	m_range_slider->setFixedWidth(250);
	m_range_slider->setRange(0, 250);
	m_range_slider->setValue_high(m_range_slider->maximum());
	m_range_slider->move(100, 40);

	QLabel* label = new QLabel("Low", this);
	label->move(35, 10);

	m_low_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
	m_low_le->setValue(0.0);
	m_low_le->move(10, 35);
	m_low_le->setFixedWidth(m_le_width);
	connect(m_low_le, &QLineEdit::editingFinished, this, &RangeMaskDialog::onEditingFinished_low);
	connect(m_range_slider, &RangeSlider::sliderMoved_low, this, &RangeMaskDialog::onSliderMoved_low);

	label = new QLabel("High", this);
	label->move(382, 10);

	m_high_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
	m_high_le->setValue(1.0);
	m_high_le->move(360, 35);
	m_high_le->setFixedWidth(m_le_width);
	connect(m_high_le, &QLineEdit::editingFinished, this, &RangeMaskDialog::onEditingFinished_high);
	connect(m_range_slider, &RangeSlider::sliderMoved_high, this, &RangeMaskDialog::onSliderMoved_high);
}

void RangeMaskDialog::addFuzzinessInputs() {

	m_fuzzy_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
	m_fuzzy_le->setValue(0.0);
	m_fuzzy_le->setFixedWidth(50);
	m_fuzzy_le->move(110, 75);
	m_fuzzy_le->addLabel(new QLabel("Fuzziness:   ", this));

	m_fuzzy_slider = new Slider(Qt::Horizontal, this);
	m_fuzzy_slider->setFixedWidth(250);
	m_fuzzy_slider->setRange(0, 250);
	m_fuzzy_le->addSlider(m_fuzzy_slider);

	auto action = [this](int) {
		float fuzzy = float(m_fuzzy_slider->sliderPosition()) / m_fuzzy_slider->maximum();
		m_fuzzy_le->setValue(fuzzy);
		m_rm.setFuzziness(fuzzy);
		startTimer();
	};

	auto edited = [this]() {
		float fuzzy = m_fuzzy_le->valuef();
		m_fuzzy_slider->setValue(fuzzy * m_fuzzy_slider->maximum());
		m_rm.setFuzziness(fuzzy);
		ApplytoPreview();
	};

	connect(m_fuzzy_slider, &QSlider::actionTriggered, this, action);
	connect(m_fuzzy_le, &QLineEdit::editingFinished, this, edited);
}

void RangeMaskDialog::addSmoothnessInputs() {

	m_smooth_le = new DoubleLineEdit(new DoubleValidator(0.0, 100.0, 2), this);
	m_smooth_le->setValue(0.0);
	m_smooth_le->setFixedWidth(50);
	m_smooth_le->move(110, 110);
	m_smooth_le->addLabel(new QLabel("Smoothnes:   ", this));

	m_smooth_slider = new Slider(Qt::Horizontal, this);
	m_smooth_slider->setFixedWidth(250);
	m_smooth_slider->setRange(0, 250);
	m_smooth_le->addSlider(m_smooth_slider);

	auto action = [this](int) {
		float smooth = float(m_smooth_slider->sliderPosition() * 100) / m_smooth_slider->maximum();
		m_smooth_le->setValue(smooth);
		m_rm.setSmoothness(smooth);
		startTimer();
	};

	auto edited = [this]() {
		float smooth = m_smooth_le->valuef();
		m_smooth_slider->setValue((smooth * m_smooth_slider->maximum()) / 100);
		m_rm.setSmoothness(smooth);
		ApplytoPreview();
	};

	connect(m_smooth_slider, &QSlider::actionTriggered, this, action);
	connect(m_smooth_le, &QLineEdit::editingFinished, this, edited);
}

void RangeMaskDialog::resetDialog() {
	m_rm = RangeMask();
	m_range_slider->resetSliderPositions();
	
	m_fuzzy_le->setValue(m_rm.Fuzziness());
	m_fuzzy_slider->setSliderPosition(m_rm.Fuzziness() * m_fuzzy_slider->maximum());
	
	m_smooth_le->setValue(m_rm.Smoothness());
	m_smooth_slider->setSliderPosition((m_rm.Smoothness() * m_fuzzy_slider->maximum()) / 100);

	m_lightness_cb->setChecked(m_rm.Lightness());
	m_screening_cb->setChecked(m_rm.Screening());
	m_invert_cb->setChecked(m_rm.Invert());
}

void RangeMaskDialog::showPreview() {

	ProcessDialog::showPreview();
	ApplytoPreview();
}

void RangeMaskDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	std::string name = "RangeMask";

	int count = 0;
	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		std::string img_name = ptr->name().toStdString();
		if (name == img_name)
			name += std::to_string(++count);
	}

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		Image8 rm = m_rm.GenerateMask(iwptr->source());
		ImageWindow8* iw = new ImageWindow8(rm, name.c_str(), m_workspace);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		Image16 rm = m_rm.GenerateMask(iw16->source());
		ImageWindow16* iw = new ImageWindow16(rm, name.c_str(), m_workspace);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		Image32 rm = m_rm.GenerateMask(iw32->source());
		ImageWindow32* iw = new ImageWindow32(rm, name.c_str(), m_workspace);
		break;
	}
	}

}

void RangeMaskDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* iwptr = previewRecast(m_preview);

	auto t = GetTimePoint();
	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_rm, &RangeMask::GenerateMask_ReducedTo);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_rm, &RangeMask::GenerateMask_ReducedTo);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		 iw32->updatePreview(m_rm, &RangeMask::GenerateMask_ReducedTo);
	}
	}
	DisplayTimeDuration(t);
}