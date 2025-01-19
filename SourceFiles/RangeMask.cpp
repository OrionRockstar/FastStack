#include "pch.h"
#include "RangeMask.h"
#include "GaussianFilter.h"
#include "FastStack.h"

template <typename T>
Image<T> RangeMask::generateMask(const Image<T>& img) {

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
        GaussianFilter(m_smoothness).apply(mask);

    return mask;
}
template Image8 RangeMask::generateMask(const Image8&);
template Image16 RangeMask::generateMask(const Image16&);
template Image32 RangeMask::generateMask(const Image32&);

template <typename T>
void RangeMask::generateMask_overwrite(Image<T>& img) {

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
		GaussianFilter(m_smoothness).apply(mask);

	mask.moveTo(img);
}
template void RangeMask::generateMask_overwrite(Image8&);
template void RangeMask::generateMask_overwrite(Image16&);
template void RangeMask::generateMask_overwrite(Image32&);


template<typename T>
Image<T> RangeMask::generateMask_reduced(const Image<T>& img, int factor) {

	factor = math::max(factor, 1);

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
		GaussianFilter(m_smoothness).apply(mask);

	return mask;
}
template Image8 RangeMask::generateMask_reduced(const Image8&, int);
template Image16 RangeMask::generateMask_reduced(const Image16&, int);
template Image32 RangeMask::generateMask_reduced(const Image32&, int);

template<typename T>
void RangeMask::generateMask_reducedTo(const Image<T>& src, Image<T>& mask, int factor) {

	factor = math::max(factor, 1);

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
		GaussianFilter(m_smoothness).apply(mask);

}
template void RangeMask::generateMask_reducedTo(const Image8&, Image8&, int);
template void RangeMask::generateMask_reducedTo(const Image16&, Image16&, int);
template void RangeMask::generateMask_reducedTo(const Image32&, Image32&, int);






RangeSlider::RangeSlider(int min, int max, QWidget* parent) : QSlider(parent) {

	this->setOrientation(Qt::Horizontal);
	this->setRange(min, max);

	QPalette p; 
	p.setBrush(QPalette::Button, QColor(129, 129, 129));

	initStyleOption(&m_low);
	m_low.sliderPosition = 0;
	m_low.subControls = QStyle::SC_SliderHandle;
	m_low.palette = p;

	initStyleOption(&m_high);
	m_high.sliderPosition = maximum();
	m_high.subControls = QStyle::SC_SliderHandle;
	m_high.palette = p;
}

void RangeSlider::resetSlider() {
	m_low.sliderPosition = minimum();
	m_high.sliderPosition = maximum();

	emit sliderMoved_low(m_low.sliderPosition);
	emit sliderMoved_high(m_high.sliderPosition);

	update();
}

void RangeSlider::mousePressEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {
		QStyle* style = QApplication::style();
		QStyleOptionSlider opt;
		initStyleOption(&opt);

		if (style->hitTestComplexControl(QStyle::CC_Slider, &opt, e->pos(), this) == QStyle::SC_SliderGroove)
			m_low_act = m_high_act = false;

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_low, e->pos(), this) == QStyle::SC_SliderHandle) {
			m_low_act = true;
			m_high_act = false;
		}

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_high, e->pos(), this) == QStyle::SC_SliderHandle) {
			m_high_act = true;
			m_low_act  = false;
		}
	}
}

void RangeSlider::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {

		int x = QStyle::sliderValueFromPosition(minimum(), maximum(), e->x() - m_handle_width / 2, width() - m_handle_width, false);

		if (m_low_act) {

			m_low.sliderValue = m_low.sliderPosition = x;
			emit sliderMoved_low(x);

			if (m_low.sliderPosition >= m_high.sliderPosition && m_low.sliderPosition < maximum())
				emit sliderMoved_high(m_high.sliderValue = m_high.sliderPosition = m_low.sliderPosition + 1);
		}


		if (m_high_act) {

			m_high.sliderValue = m_high.sliderPosition = x;
			emit sliderMoved_high(x);

			if (x <= m_low.sliderPosition && x > minimum())
				emit sliderMoved_low(m_low.sliderValue = m_low.sliderPosition = m_high.sliderPosition - 1);
		}
	}

	update();
}

void RangeSlider::mouseReleaseEvent(QMouseEvent* e) {
	m_low_act = m_high_act = false;
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


	//low handle
	opt.sliderPosition = m_low.sliderPosition;	

	QRect low = r;
	m_low.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	low.setRight(m_low.rect.left());
	p.fillRect(low, pal.color(QPalette::Highlight));
	style()->drawComplexControl(QStyle::CC_Slider, &m_low, &p, this);


	//high handle
	opt.sliderPosition = m_high.sliderPosition;

	QRect high = r;
	m_high.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	high.setLeft(m_high.rect.right());
	p.fillRect(high, pal.color(QPalette::Highlight));
	style()->drawComplexControl(QStyle::CC_Slider, &m_high, &p, this);
}






RangeMaskDialog::RangeMaskDialog(QWidget* parent) : ProcessDialog("RangeMask", QSize(455, 215), FastStack::recast(parent)->workspace()) {
    
    setTimer(250, this, &RangeMaskDialog::applytoPreview);

	connectToolbar(this, &RangeMaskDialog::apply, &RangeMaskDialog::showPreview, &RangeMaskDialog::resetDialog);
	connectZoomWindow(this, &RangeMaskDialog::applytoPreview);

	addRangeSlider();
	addFuzzinessInputs();
	addSmoothnessInputs();

	int dx = 90;
	m_lightness_cb = new CheckBox("Lightness", this);
	m_lightness_cb->setChecked(true);
	m_lightness_cb->move(dx, 150);
	connect(m_lightness_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setLightness(checked); applytoPreview(); });

	m_screening_cb = new CheckBox("Screening", this);
	m_screening_cb->move(dx + 110, 150);
	connect(m_screening_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setScreening(checked); applytoPreview(); });

	m_invert_cb = new CheckBox("Invert", this);
	m_invert_cb->move(dx + 220, 150);
	connect(m_invert_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setInvert(checked); applytoPreview(); });
    
    this->show();
}

void RangeMaskDialog::addRangeSlider() {

	m_range_slider = new RangeSlider(0, 200, this);
	m_range_slider->setFixedWidth(250);
	m_range_slider->move(100, 40);

	QLabel* label = new QLabel("Low", this);
	label->move(35, 10);

	m_low_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
	m_low_le->setValue(0.0);
	m_low_le->move(10, 35);
	m_low_le->setFixedWidth(m_le_width);

	auto moved_low = [this](int value) {
		float low = float(value) / m_range_slider->maximum();
		m_low_le->setValue(low);
		m_rm.setLow(low);
		startTimer();
	};

	auto edited_low = [this]() {
		double low = m_low_le->value();
		m_range_slider->setValue_low(low * m_range_slider->maximum());
		m_rm.setLow(low);
		applytoPreview();
	};

	connect(m_low_le, &QLineEdit::editingFinished, this, edited_low);
	connect(m_range_slider, &RangeSlider::sliderMoved_low, this, moved_low);

	label = new QLabel("High", this);
	label->move(382, 10);

	m_high_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), this);
	m_high_le->setValue(1.0);
	m_high_le->move(360, 35);
	m_high_le->setFixedWidth(m_le_width);

	auto moved_high = [this](int value) {
		float high = float(value) / m_range_slider->maximum();
		m_high_le->setValue(high);
		m_rm.setHigh(high);
		startTimer();
	};

	auto edited_high = [this]() {
		double high = m_high_le->value();
		m_range_slider->setValue_high(high * m_range_slider->maximum());
		m_rm.setHigh(high);
		applytoPreview();
	};

	connect(m_high_le, &QLineEdit::editingFinished, this, edited_high);
	connect(m_range_slider, &RangeSlider::sliderMoved_high, this, moved_high);
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
		applytoPreview();
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
		applytoPreview();
	};

	connect(m_smooth_slider, &QSlider::actionTriggered, this, action);
	connect(m_smooth_le, &QLineEdit::editingFinished, this, edited);
}

void RangeMaskDialog::resetDialog() {
	m_rm = RangeMask();
	m_range_slider->resetSlider();
	
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
	applytoPreview();
}

void RangeMaskDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	std::string name = "RangeMask";

	int count = 0;
	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = imageRecast<>(sw->widget());
		std::string img_name = ptr->name().toStdString();
		if (name == img_name)
			name += std::to_string(++count);
	}

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		Image8 rm = m_rm.generateMask(iwptr->source());
		ImageWindow8* iw = new ImageWindow8(std::move(rm), name.c_str(), m_workspace);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		Image16 rm = m_rm.generateMask(iw16->source());
		ImageWindow16* iw = new ImageWindow16(std::move(rm), name.c_str(), m_workspace);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		Image32 rm = m_rm.generateMask(iw32->source());
		ImageWindow32* iw = new ImageWindow32(std::move(rm), name.c_str(), m_workspace);
		break;
	}
	}

}

void RangeMaskDialog::applytoPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* pwptr = previewRecast(m_preview);

	auto zw = [&]<typename T>(PreviewWindow<T>* pw) {

		for (auto sw : m_workspace->subWindowList()) {
			auto iw = imageRecast<T>(sw->widget());
			if (iw->preview() == m_preview)
				if (iw->zoomWindow())
					return pw->updatePreview(m_rm, &RangeMask::generateMask_overwrite);
		}
		pw->updatePreview(m_rm, &RangeMask::generateMask_reducedTo);
	};

	switch (pwptr->type()) {

	case ImageType::UBYTE:
		return zw(pwptr);
	
	case ImageType::USHORT: 
		return zw(previewRecast<uint16_t>(pwptr));
	
	case ImageType::FLOAT:
		return zw(previewRecast<float>(pwptr));
	}
}