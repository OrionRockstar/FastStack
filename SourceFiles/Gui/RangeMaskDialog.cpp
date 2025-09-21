#include "pch.h"
#include "RangeMaskDialog.h"
#include "FastStack.h"



RangeSlider::RangeSlider(int min, int max, QWidget* parent) : Slider(parent) {

	//this->setOrientation(Qt::Horizontal);
	this->setRange(min, max);
	this->setMouseTracking(true);

	QPalette p;
	p.setBrush(QPalette::Button, QColor(129, 129, 129));
	p.setBrush(QPalette::Highlight, QColor(123, 0, 216));
	this->setPalette(p);

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

bool RangeSlider::event(QEvent* e) {
	Slider::event(e);

	if (e->type() == QEvent::Leave) {
		m_low.state &= ~QStyle::State_MouseOver;
		m_high.state &= ~QStyle::State_MouseOver;
	}

	return true;
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
			m_low_act = false;
		}
	}
}

void RangeSlider::mouseMoveEvent(QMouseEvent* e) {

	if (m_low.rect.contains(e->pos()) || m_low_act)
		m_low.state |= QStyle::State_MouseOver;
	else
		m_low.state &= ~QStyle::State_MouseOver;

	if (m_high.rect.contains(e->pos()) || m_high_act)
		m_high.state |= QStyle::State_MouseOver;
	else
		m_high.state &= ~QStyle::State_MouseOver;

	if (e->buttons() == Qt::LeftButton) {
		int diff = width() - maximum();
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
	p.setRenderHint(QPainter::Antialiasing);
	QStyleOptionSlider opt;
	initStyleOption(&opt);

	//trackbar
	opt.subControls = QStyle::SC_SliderGroove;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);


	QRect r = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove);
	r.adjust(2, 2, -2, -2);

	opt.subControls = QStyle::SC_SliderHandle;

	//low handle
	opt.sliderPosition = m_low.sliderPosition;
	opt.state = m_low.state;
	m_low.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	QRect low = r;
	low.setRight(m_low.rect.left());
	p.fillRect(low, opt.palette.color(QPalette::Highlight));
	drawHandle(p, opt);
	//style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);


	//high handle
	opt.sliderPosition = m_high.sliderPosition;
	opt.state = m_high.state;
	m_high.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	QRect high = r;
	high.setLeft(m_high.rect.right() - 1);
	p.fillRect(high, opt.palette.color(QPalette::Highlight));
	drawHandle(p, opt);
	//style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}






RangeMaskDialog::RangeMaskDialog(QWidget* parent) : ProcessDialog("RangeMask", QSize(455, 190), FastStack::recast(parent)->workspace()) {

	setDefaultTimerInterval(250);

	addRangeSlider();
	addFuzzinessInputs();
	addSmoothnessInputs();

	int dx = 90;
	m_lightness_cb = new CheckBox("Lightness", drawArea());
	m_lightness_cb->setChecked(true);
	m_lightness_cb->move(dx, 150);
	connect(m_lightness_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setLightness(checked); applytoPreview(); });

	m_screening_cb = new CheckBox("Screening", drawArea());
	m_screening_cb->move(dx + 110, 150);
	connect(m_screening_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setScreening(checked); applytoPreview(); });

	m_invert_cb = new CheckBox("Invert", drawArea());
	m_invert_cb->move(dx + 220, 150);
	connect(m_invert_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setInvert(checked); applytoPreview(); });

	this->show();
}

void RangeMaskDialog::addRangeSlider() {

	m_range_slider = new RangeSlider(0, 200, drawArea());
	m_range_slider->setFixedWidth(250);
	m_range_slider->move(100, 40);

	QLabel* label = new QLabel("Low", drawArea());
	label->move(35, 10);

	m_low_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), drawArea());
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

	label = new QLabel("High", drawArea());
	label->move(382, 10);

	m_high_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6), drawArea());
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

	m_fuzzy_le = new DoubleLineEdit(m_rm.fuzziness(), new DoubleValidator(0.0, 1.0, 2), drawArea());
	m_fuzzy_le->setFixedWidth(50);
	m_fuzzy_le->move(110, 75);
	addLabel(m_fuzzy_le, new QLabel("Fuzziness:", drawArea()));

	m_fuzzy_slider = new Slider(drawArea());
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

	m_smooth_le = new DoubleLineEdit(m_rm.smoothness(),new DoubleValidator(0.0, 100.0, 2), drawArea());
	m_smooth_le->setFixedWidth(50);
	m_smooth_le->move(110, 110);
	addLabel(m_smooth_le, new QLabel("Smoothnes:", drawArea()));

	m_smooth_slider = new Slider(drawArea());
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

	m_fuzzy_le->reset();
	m_fuzzy_slider->setSliderPosition(m_rm.fuzziness() * m_fuzzy_slider->maximum());

	m_smooth_le->reset();
	m_smooth_slider->setSliderPosition((m_rm.smoothness() * m_fuzzy_slider->maximum()) / 100);

	m_lightness_cb->setChecked(m_rm.lightness());
	m_screening_cb->setChecked(m_rm.screening());
	m_invert_cb->setChecked(m_rm.invert());
}

void RangeMaskDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	std::string name = "RangeMask";

	enableSiblings_Subwindows(false);

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
	enableSiblings_Subwindows(true);
}

void RangeMaskDialog::applyPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* pwptr = previewRecast(m_preview);

	auto zw = [&]<typename T>(PreviewWindow<T>*pw) {
		float s = m_rm.smoothness();
		m_rm.setSmoothness(s * pw->scaleFactor());
		pw->updatePreview(m_rm, &RangeMask::generateMask_overwrite);
		m_rm.setSmoothness(s);
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