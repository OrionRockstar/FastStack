#include "pch.h"
#include "Masks.h"
#include "GaussianFilter.h"
#include "FastStack.h"

template <typename T>
Image<T> RangeMask::GenerateMask(Image<T>& img) {

    int ch = (m_lightness && img.Channels() == 3) ? 1 : img.Channels();

    Image<T> mask(img.Rows(), img.Cols(), ch);

    float z = (m_high - m_low) * m_fuzziness * 0.5;

    float a_p = m_low + z;
    float b_p = m_high - z;

    float base_a = m_low;
    float base_b = b_p;

    for (int el = 0; el < mask.TotalPxCount(); ++el) {
        float pixel = 0;

        if (m_lightness && img.Channels() == 3) {
            double R, G, B;
            img.getRGB(el, R, G, B);
            pixel = ColorSpace::CIEL(R, G, B);
        }

        else
            pixel = Pixel<float>::toType(img[el]);


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

        mask[el] = Pixel<T>::toType(pixel);
    }

    if (m_smoothness != 0.0)
        GaussianFilter(m_smoothness).Apply(mask);

    return mask;
}
template Image8 RangeMask::GenerateMask(Image8&);
template Image16 RangeMask::GenerateMask(Image16&);
template Image32 RangeMask::GenerateMask(Image32&);





RangeSlider::RangeSlider(Qt::Orientation orientation, QWidget* parent) : QSlider(parent) {
	this->setOrientation(orientation);
	initStyleOption(&m_low);
	m_low.sliderPosition = 0;
	m_low.subControls = QStyle::SC_SliderHandle;

	initStyleOption(&m_high);
	m_high.sliderPosition = maximum();
	m_high.subControls = QStyle::SC_SliderHandle;
}

int RangeSlider::sliderPosition_low()const { return m_low.sliderPosition; }

void RangeSlider::setSliderPosition_low(int pos) {
	m_low.sliderPosition = pos;
	update();
}

int RangeSlider::sliderPosition_high()const { return m_high.sliderPosition; }

void RangeSlider::setSliderPosition_high(int pos) {
	m_high.sliderPosition = pos;
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

	//QSlider::mousePressEvent(event);
}

void RangeSlider::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::LeftButton) {

		if (m_low_act) {
			int x = m_low.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < minimum())
				x = 0;

			else if (x > m_high.sliderPosition && x <= maximum()) {
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

			else if (x < m_low.sliderPosition && x >= minimum()) {
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

	//trackbar
	opt.subControls = QStyle::SC_SliderGroove;
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	opt.subControls = QStyle::SC_SliderHandle;

	//low handle
	opt.sliderPosition = m_low.sliderPosition;
	m_low.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	//high handle
	opt.sliderPosition = m_high.sliderPosition;
	m_high.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}






RangeMaskDialog::RangeMaskDialog(QWidget* parent) : ProcessDialog("RangeMask", QSize(455, 215), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent) {
    
    setTimer(250, this, &RangeMaskDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &RangeMaskDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &RangeMaskDialog::Apply, &RangeMaskDialog::showPreview, &RangeMaskDialog::resetDialog);

	AddRangeSlider();
	AddFuzzinessInputs();
	AddSmoothnessInputs();

	int dx = 90;
	m_lightness_cb = new QCheckBox("Lightness", this);
	m_lightness_cb->setChecked(true);
	m_lightness_cb->move(dx, 150);
	connect(m_lightness_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setLightness(checked); ApplytoPreview(); });

	m_screening_cb = new QCheckBox("Screening", this);
	m_screening_cb->move(dx + 110, 150);
	connect(m_screening_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setScreening(checked); ApplytoPreview(); });

	m_invert_cb = new QCheckBox("Invert", this);
	m_invert_cb->move(dx + 220, 150);
	connect(m_invert_cb, &QCheckBox::clicked, this, [this](bool checked) { m_rm.setInvert(checked); ApplytoPreview(); });
    
    this->show();
}

void RangeMaskDialog::onEditingFinished_low() {

	int pos = m_low_le->Value() * m_range_slider->maximum();
	m_range_slider->setSliderPosition_low(pos);

	if (pos > m_range_slider->sliderPosition_high())
		m_range_slider->setSliderPosition_high(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onSliderMoved_low(int value) {

	float low = float(value) / m_range_slider->maximum();
	m_low_le->setValue(low);
	m_rm.setLow(low);

	startTimer();
}

void RangeMaskDialog::onEditingFinished_high() {

	int pos = m_high_le->Value() * m_range_slider->maximum();
	m_range_slider->setSliderPosition_high(pos);

	if (pos < m_range_slider->sliderPosition_low())
		m_range_slider->setSliderPosition_low(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onSliderMoved_high(int value) {

	float high = float(value) / m_range_slider->maximum();
	m_high_le->setValue(high);
	m_rm.setHigh(high);

	startTimer();
}

void RangeMaskDialog::onEditingFinished_fuzzy() {

	int pos = m_fuzzy_le->Value() * m_fuzzy_slider->maximum();
	m_fuzzy_slider->setSliderPosition(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onActionTriggered_fuzzy(int action) {

	float fuzzy = float(m_fuzzy_slider->sliderPosition()) / m_fuzzy_slider->maximum();
	m_fuzzy_le->setValue(fuzzy);
	m_rm.setFuzziness(fuzzy);

	startTimer();
	//if (action == 3 || action == 4)
		//onSliderMoved_fuzzy(m_fuzzy_slider->sliderPosition());
}

void RangeMaskDialog::onEditingFinished_smooth() {

	int pos = m_smooth_le->Value() * m_smooth_slider->maximum();
	m_smooth_slider->setSliderPosition(pos);

	ApplytoPreview();
}

void RangeMaskDialog::onActionTriggered_smooth(int action) {
	float smooth = float(m_smooth_slider->sliderPosition() * 100) / m_smooth_slider->maximum();
	m_smooth_le->setValue(smooth);
	m_rm.setSmoothness(smooth);

	startTimer();
}

void RangeMaskDialog::AddRangeSlider() {

	m_range_slider = new RangeSlider(Qt::Horizontal, this);
	m_range_slider->setFixedWidth(250);
	m_range_slider->setRange(0, 250);
	m_range_slider->setSliderPosition_high(m_range_slider->maximum());
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

void RangeMaskDialog::AddFuzzinessInputs() {

	m_fuzzy_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 2), this);
	m_fuzzy_le->setValue(0.0);
	m_fuzzy_le->setFixedWidth(50);
	m_fuzzy_le->move(110, 75);
	m_fuzzy_le->addLabel(new QLabel("Fuzziness:   ", this));

	m_fuzzy_slider = new QSlider(Qt::Horizontal, this);
	m_fuzzy_slider->setFixedWidth(250);
	m_fuzzy_slider->setRange(0, 250);
	m_fuzzy_le->addSlider(m_fuzzy_slider);

	connect(m_fuzzy_slider, &QSlider::actionTriggered, this, &RangeMaskDialog::onActionTriggered_fuzzy);
	connect(m_fuzzy_le, &QLineEdit::editingFinished, this, &RangeMaskDialog::onEditingFinished_fuzzy);
}

void RangeMaskDialog::AddSmoothnessInputs() {

	m_smooth_le = new DoubleLineEdit(new DoubleValidator(0.0, 100.0, 2), this);
	m_smooth_le->setValue(0.0);
	m_smooth_le->setFixedWidth(50);
	m_smooth_le->move(110, 110);
	m_smooth_le->addLabel(new QLabel("Smoothnes:   ", this));

	m_smooth_slider = new QSlider(Qt::Horizontal, this);
	m_smooth_slider->setFixedWidth(250);
	m_smooth_slider->setRange(0, 250);
	m_smooth_le->addSlider(m_smooth_slider);

	connect(m_smooth_slider, &QSlider::actionTriggered, this, &RangeMaskDialog::onActionTriggered_smooth);
	connect(m_smooth_le, &QLineEdit::editingFinished, this, &RangeMaskDialog::onEditingFinished_smooth);
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

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		Image8 rm = m_rm.GenerateMask(iwptr->Source());
		ImageWindow8* iw = new ImageWindow8(rm, "RangeMask_" + iwptr->ImageName(), m_workspace);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		Image16 rm = m_rm.GenerateMask(iw16->Source());
		ImageWindow16* iw = new ImageWindow16(rm, "RangeMask_" + iw16->ImageName(), m_workspace);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		Image32 rm = m_rm.GenerateMask(iw32->Source());
		ImageWindow32* iw = new ImageWindow32(rm, "RangeMask_" + iw32->ImageName(), m_workspace);
		break;
	}
	}

	ApplytoPreview();
}

void RangeMaskDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		Image8 rm = m_rm.GenerateMask(iwptr->Source());
		return iw8->UpdatePreview(rm);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		Image16 rm = m_rm.GenerateMask(reinterpret_cast<ImageWindow16*>(iwptr)->Source());
		return iw16->UpdatePreview(rm);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		Image32 rm = m_rm.GenerateMask(reinterpret_cast<ImageWindow32*>(iwptr)->Source());
		return iw32->UpdatePreview(rm);
	}
	}
}