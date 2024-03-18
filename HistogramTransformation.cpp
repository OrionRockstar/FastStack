#include "pch.h"
#include "HistogramTransformation.h"
#include "FastStack.h"

template <typename Image>
void HistogramTransformation::HistogramCurve::ApplyChannel(Image& img, int ch) {
	if (img.is_float())

		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = TransformPixel(*pixel);

	else {
		if (img.is_uint8())
			Generate8Bit_LUT();

		if (img.is_uint16())
			Generate16Bit_LUT();

		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = m_lut[*pixel];
	}
}
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image8&, int);
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image16&, int);
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image32&, int);


float HistogramTransformation::Shadow(Component component) const{
	return (*this)[component].Shadow();
}

float HistogramTransformation::Midtone(Component component) const {
	return (*this)[component].Midtone();
}

float HistogramTransformation::Highlight(Component component) const {
	return (*this)[component].Highlight();
}

void HistogramTransformation::setShadow(Component component, float shadow) {
	(*this)[component].setShadow(shadow);
}

void HistogramTransformation::setMidtone(Component component, float midtone) {
	(*this)[component].setMidtone(midtone);
}

void HistogramTransformation::setHighlight(Component component, float highligt) {
	(*this)[component].setHighlight(highligt);
}


/*void HistogramTransformation::setShadow(Component component, float shadow) {
	using enum Component;
	switch (component) {
	case red:
		Red.setShadow(shadow);
		return;
	case green:
		Green.setShadow(shadow);
		return;
	case blue:
		Blue.setShadow(shadow);
		return;
	case rgb_k:
		RGB_K.setShadow(shadow);
		return;
	}
}

void HistogramTransformation::setMidtone(Component component, float midtone) {
	using enum Component;
	switch (component) {
	case red:
		Red.setMidtone(midtone);
		return;
	case green:
		Green.setMidtone(midtone);
		return;
	case blue:
		Blue.setMidtone(midtone);
		return;
	case rgb_k:
		RGB_K.setMidtone(midtone);
		return;
	}
}

void HistogramTransformation::setHighlight(Component component, float hightlight) {
	using enum Component;
	switch (component) {
	case red:
		Red.setHighlight(hightlight);
		return;
	case green:
		Green.setHighlight(hightlight);
		return;
	case blue:
		Blue.setHighlight(hightlight);
		return;
	case rgb_k:
		RGB_K.setHighlight(hightlight);
		return;
	}
}*/

template<typename T>
static T AverageMedian(Image<T>& img) {
	float sum = 0;
	for (int ch = 0; ch < img.Channels(); ++ch)
		sum += img.Median(ch);
	return sum / img.Channels();
}

template<typename T>
static T AverageMAD(Image<T>& img) {
	float sum = 0;
	for (int ch = 0; ch < img.Channels(); ++ch)
		sum += img.MAD(ch);
	return sum / img.Channels();
}


template<typename T>
void HistogramTransformation::ComputeSTFCurve(Image<T>& img){

	auto rgbk = (*this)[Component::rgb_k];
	rgbk.setMidtone(0.25);

	float median = 0, nMAD = 0;
	for (int ch = 0; ch < img.Channels(); ++ch) {
		T cm = img.ComputeMedian(ch, true);
		median += cm;
		nMAD += img.ComputeMAD(ch, cm, true);
	}

	median = Pixel<float>::toType(T(median / img.Channels()));
	nMAD = Pixel<float>::toType(T(1.4826f * (nMAD / img.Channels())));

	float shadow = (median > 2.8 * nMAD) ? median - 2.8f * nMAD : 0;
	float midtone = rgbk.MTF(median - shadow);

	rgbk.setShadow(shadow);
	rgbk.setMidtone(midtone);
}
template void HistogramTransformation::ComputeSTFCurve(Image8&);
template void HistogramTransformation::ComputeSTFCurve(Image16&);
template void HistogramTransformation::ComputeSTFCurve(Image32&);

template<typename T>
void HistogramTransformation::STFStretch(Image<T>& img) {
	auto rgbk = (*this)[Component::rgb_k];
	rgbk.setMidtone(0.25);

	float median = 0, nMAD = 0;
	for (int ch = 0; ch < img.Channels(); ++ch) {
		T cm = img.ComputeMedian(ch, true);
		median += cm;
		nMAD += img.ComputeMAD(ch, cm, true);
	}

	median = Pixel<float>::toType(T(median / img.Channels()));
	nMAD = Pixel<float>::toType(T(1.4826f * (nMAD / img.Channels())));

	float shadow = (median > 2.8 * nMAD) ? median - 2.8f * nMAD : 0;
	float midtone = rgbk.MTF(median - shadow);

	rgbk.setShadow(shadow);
	rgbk.setMidtone(midtone);

	Apply(img);

}
template void HistogramTransformation::STFStretch(Image8&);
template void HistogramTransformation::STFStretch(Image16&);
template void HistogramTransformation::STFStretch(Image32&);

template<typename Image>
void HistogramTransformation::Apply(Image& img) {

	using enum Component;

	if (!(*this)[rgb_k].IsIdentity()) {
		auto rgbk = (*this)[rgb_k];

		if (img.is_float())
			for (auto& pixel : img)
				pixel = rgbk.TransformPixel(pixel);

		else {
			if (img.is_uint8())
				rgbk.Generate8Bit_LUT();

			if (img.is_uint16())
				rgbk.Generate16Bit_LUT();

			for (auto& pixel : img)
				pixel = rgbk.m_lut[pixel];
		}

	}

	if (img.Channels() == 1)
		return;

	if (!(*this)[red].IsIdentity())
		(*this)[red].ApplyChannel(img, 0);

	if (!(*this)[green].IsIdentity())
		(*this)[green].ApplyChannel(img, 1);

	if (!(*this)[blue].IsIdentity())
		(*this)[blue].ApplyChannel(img, 2);

}
template void HistogramTransformation::Apply(Image8&);
template void HistogramTransformation::Apply(Image16&);
template void HistogramTransformation::Apply(Image32&);



//middle of slider be slider position
//align slider/slider handles with hist graph
HistogramSlider::HistogramSlider(Qt::Orientation orientation, QWidget* parent) : QSlider(parent) {
	this->setOrientation(orientation);
	initStyleOption(&m_shadow);
	m_shadow.sliderPosition = 0;
	m_shadow.subControls = QStyle::SC_SliderHandle;

	initStyleOption(&m_midtone);
	m_midtone.sliderPosition = max_val / 2;
	m_midtone.subControls = QStyle::SC_SliderHandle;

	initStyleOption(&m_highlight);
	m_highlight.sliderPosition = max_val;
	m_highlight.subControls = QStyle::SC_SliderHandle;
}

void HistogramSlider::setMedian(float median) { m_med = median; }

int HistogramSlider::sliderPosition_shadow()const { return m_shadow.sliderPosition; }

void HistogramSlider::setSliderPosition_shadow(int pos) { 
	m_shadow.sliderPosition = pos; 
	update(); 
}

int HistogramSlider::sliderPosition_midtone()const { return m_midtone.sliderPosition; }

void HistogramSlider::setSliderPosition_midtone(int pos) { 
	m_midtone.sliderPosition = pos; 
	update(); 
}

int HistogramSlider::sliderPosition_highlight()const { return m_highlight.sliderPosition; }

void HistogramSlider::setSliderPosition_highlight(int pos) { 
	m_highlight.sliderPosition = pos; 
	update(); 
}

void HistogramSlider::resetSliderPositions() {
	m_shadow.sliderPosition = 0;
	m_midtone.sliderPosition = max_val / 2;
	m_highlight.sliderPosition = max_val;
	m_med = 0.5;

	update();
}


void HistogramSlider::mousePressEvent(QMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {
		QStyle* style = QApplication::style();
		QStyleOptionSlider opt;
		initStyleOption(&opt);

		if (style->hitTestComplexControl(QStyle::CC_Slider, &opt, event->pos(), this) == QStyle::SC_SliderGroove)
			m_shadow_act = m_midtone_act = m_highlight_act = false;
		

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_shadow, event->pos(), this) == QStyle::SC_SliderHandle) {
			click_x = event->x();
			m_shadow_act = true;
			m_midtone_act = m_highlight_act = false;
			this->triggerAction(this->SliderMove);
			this->setRepeatAction(this->SliderNoAction);
			this->setSliderDown(true);
		}

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_midtone, event->pos(), this) == QStyle::SC_SliderHandle) {
			click_x = event->x();
			m_midtone_act = true;
			m_shadow_act = m_highlight_act = false;
			this->triggerAction(this->SliderMove);
			this->setRepeatAction(this->SliderNoAction);
			this->setSliderDown(true);
		}

		if (style->hitTestComplexControl(QStyle::CC_Slider, &m_highlight, event->pos(), this) == QStyle::SC_SliderHandle) {
			click_x = event->x();
			m_highlight_act = true;
			m_shadow_act = m_midtone_act = false;
			this->triggerAction(this->SliderMove);
			this->setRepeatAction(this->SliderNoAction);
			this->setSliderDown(true);
		}
	}
	QSlider::mousePressEvent(event);
}

void HistogramSlider::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::LeftButton) {

		if (m_shadow_act) {
			int x = m_shadow.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < minimum())
				x = 0;
			else if (x > m_highlight.sliderPosition)
				x = m_highlight.sliderPosition;

			m_shadow.sliderValue = m_shadow.sliderPosition = x;

			m_midtone.sliderValue = m_midtone.sliderPosition = m_shadow.sliderPosition + (m_highlight.sliderPosition - m_shadow.sliderPosition) * m_med;

			emit sliderMoved_shadow(m_shadow.sliderPosition);
		}

		if (m_midtone_act) {
			int x = m_midtone.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < m_shadow.sliderPosition)
				x = m_shadow.sliderPosition;

			else if (x > m_highlight.sliderPosition)
				x = m_highlight.sliderPosition;

			m_midtone.sliderValue = m_midtone.sliderPosition = x;
			m_med = (x - m_shadow.sliderPosition) / double(m_highlight.sliderPosition - m_shadow.sliderPosition);

			emit sliderMoved_midtone(m_midtone.sliderPosition);
		}

		if (m_highlight_act) {
			int x = m_highlight.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < m_shadow.sliderPosition)
				x = m_shadow.sliderPosition;

			else if (x > maximum())
				x = maximum();

			m_highlight.sliderValue = m_highlight.sliderPosition = x;
			m_midtone.sliderValue = m_midtone.sliderPosition = m_shadow.sliderPosition + (m_highlight.sliderPosition - m_shadow.sliderPosition) * m_med;

			emit sliderMoved_highlight(m_highlight.sliderPosition);
		}

	}

	update();

}

void HistogramSlider::paintEvent(QPaintEvent* event) {
	//Q_UNUSED(ev);
	QPainter p(this);
	QStyleOptionSlider opt;


	initStyleOption(&opt);

	//trackbar
	opt.subControls = QStyle::SC_SliderGroove;
	
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	opt.subControls = QStyle::SC_SliderHandle;

	//shadow handle
	opt.sliderPosition = m_shadow.sliderPosition;
	m_shadow.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	//m_shadow.rect.adjust(-6, 0, 0, 0);
	style()->drawComplexControl(QStyle::CC_Slider, &m_shadow, &p, this);


	//midtone handle
	opt.sliderPosition = m_midtone.sliderPosition;
	m_midtone.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	//if(0.45 > m_med || m_med > 0.55)
		//m_midtone.rect.adjust(-6, 0, -6, 0);
	style()->drawComplexControl(QStyle::CC_Slider, &m_midtone, &p, this);

	//highlight handle
	opt.sliderPosition = m_highlight.sliderPosition;
	m_highlight.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
	style()->drawComplexControl(QStyle::CC_Slider, &m_highlight, &p, this);
}




HistogramTransformationDialog::HistogramTransformationDialog(QWidget* parent): ProcessDialog("HistogramTransformation", parent) {

	this->setWindowTitle(Name());
	this->resize(400, 600);
	this->setFocus();

	m_timer = new Timer(500, this);
	connect(m_timer, &QTimer::timeout, this, &HistogramTransformationDialog::ApplytoPreview);

	setWorkspace(reinterpret_cast<FastStack*>(parentWidget())->workspace);
	setToolbar(new Toolbar(this));

	connect(m_tb, &Toolbar::sendApply, this, &HistogramTransformationDialog::Apply);
	connect(m_tb, &Toolbar::sendPreview, this, &HistogramTransformationDialog::showPreview);
	connect(m_tb, &Toolbar::sendReset, this, &HistogramTransformationDialog::resetDialog);
	
	AddHistogramChart();

	m_image_sel = new QComboBox(this);
	m_image_sel->addItem("No Image Selected");
	m_image_sel->move(5, 520);
	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(reinterpret_cast<ImageWindow8*>(sw->widget())->ImageName());

	//auto cw = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow());

	connect(reinterpret_cast<Workspace*>(m_workspace), &Workspace::sendClose, this, &HistogramTransformationDialog::onWindowClose);
	connect(reinterpret_cast<Workspace*>(m_workspace), &Workspace::sendOpen, this, &HistogramTransformationDialog::onWindowOpen);
	connect(m_image_sel, &QComboBox::activated, this, &HistogramTransformationDialog::onActivation);

	AddChannelSelection();
	AddLineEdits();

	QString str = "QSlider::groove:horizontal{"
	"border: 1px solid #999999;"
	"height: 8px;"
	"background: qlineargradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 #B1B1B1, stop:1 #c4c4c4);"
	"margin: 0px 0;}"

	"QSlider::handle:horizontal{"
			"background: qlineargradient(x1 : 0, y1 : 0, x2 : 1, y2 : 1, stop : 0 #b4b4b4, stop:1 #8f8f8f);"
			"border: 1px solid #5c5c5c;"
			"width: 6px;"
			"margin: -3px 0;"
	"border-top-left-radius: 2px;"
		"border-top-right-radius: 2px;"
		"}";
	m_histogram_slider = new HistogramSlider(Qt::Horizontal, this);

	m_histogram_slider->setFixedWidth(392);
	m_histogram_slider->setRange(0, 380);
	m_histogram_slider->setStyleSheet(str);

	m_histogram_slider->move(4, 385);

	connect(m_histogram_slider, &HistogramSlider::sliderMoved_shadow, this, &HistogramTransformationDialog::sliderMoved_shadow);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_midtone, this, &HistogramTransformationDialog::sliderMoved_midtone);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_highlight, this, &HistogramTransformationDialog::sliderMoved_highlight);


	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->show();

}

void HistogramTransformationDialog::onWindowOpen() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->ImageName();
	m_image_sel->addItem(str);
}

void HistogramTransformationDialog::onWindowClose() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->ImageName();
		int index = m_image_sel->findText(str);
		if (index == m_image_sel->currentIndex()) 
			m_histogram->removeAllSeries();
		m_image_sel->removeItem(index);
		m_image_sel->setCurrentIndex(0);
}


void HistogramTransformationDialog::onActivation(int index) {
	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == ptr->ImageName()) {
			switch (ptr->source.Bitdepth()) {
			case 8:
				showHistogram(ptr->source);
				break;
			case 16:
				showHistogram(reinterpret_cast<ImageWindow16*>(ptr)->source);
				break;
			case -32:
				showHistogram(reinterpret_cast<ImageWindow32*>(ptr)->source);
				break;
			}
			ApplytoPreview();
		}
	}
}

void HistogramTransformationDialog::onClick(int id) {
	Component comp = Component(id);

	float shadow = m_ht.Shadow(comp);
	float midtone = m_ht.Midtone(comp);
	float highlight = m_ht.Highlight(comp);

	m_shadow_le->setText(QString::number(shadow, 'f', 8));
	m_midtone_le->setText(QString::number(midtone, 'f', 8));
	m_highlight_le->setText(QString::number(highlight, 'f', 8));

	m_histogram_slider->setSliderPosition_shadow(shadow * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_midtone((midtone + shadow) * (highlight + shadow) * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_highlight(highlight * m_histogram_slider->maximum());

	m_histogram_slider->setMedian(midtone);

	onActivation(m_image_sel->currentIndex());
}


void HistogramTransformationDialog::sliderMoved_shadow(int value) {
	m_shadow_le->setText(QString::number(double(value) / m_histogram_slider->maximum(), 'f', m_shadow_le->Validator()->decimals()));
	m_timer->start();
}

void HistogramTransformationDialog::sliderMoved_midtone(int value) {
	int s = m_histogram_slider->sliderPosition_shadow();
	int h = m_histogram_slider->sliderPosition_highlight();

	m_midtone_le->setText(QString::number(double(value - s) / (h - s), 'f', m_midtone_le->Validator()->decimals()));
	m_timer->start();
}

void HistogramTransformationDialog::sliderMoved_highlight(int value) {
	m_highlight_le->setText(QString::number(double(value) / m_histogram_slider->maximum(), 'f', m_highlight_le->Validator()->decimals()));
	m_timer->start();
}

void HistogramTransformationDialog::editingFinished_shadow() {
	m_histogram_slider->setSliderPosition_shadow(m_shadow_le->text().toFloat() * m_histogram_slider->maximum());
	ApplytoPreview();
}

void HistogramTransformationDialog::editingFinished_midtone() {
	m_histogram_slider->setSliderPosition_midtone(m_midtone_le->text().toFloat() * m_histogram_slider->maximum());
	ApplytoPreview();
}

void HistogramTransformationDialog::editingFinished_highlight() {
	m_histogram_slider->setSliderPosition_highlight(m_highlight_le->text().toFloat() * m_histogram_slider->maximum());
	ApplytoPreview();
}


void HistogramTransformationDialog::AddHistogramChart() {

	m_histogram = new QChart;
	m_histogram->setGeometry(0, 0, 400, 400);
	m_histogram->setPlotArea(QRect(10, 10, 380, 378));
	m_histogram->legend()->setVisible(false);
	m_histogram->setBackgroundBrush(QColor(96, 96, 96));
	m_histogram->setAutoFillBackground(true);

	cv = new QChartView(m_histogram, this);
	cv->setGeometry(0, 0, 400, 400);


	m_axisX = new QValueAxis;
	m_axisX->setLabelsVisible(false);
	m_axisX->setRange(0, 256);
	m_histogram->addAxis(m_axisX, Qt::AlignBottom);

	m_axisY = new QValueAxis;
	m_axisY->setLabelsVisible(false);
	m_axisY->setRange(0, 1);
	m_histogram->addAxis(m_axisY, Qt::AlignLeft);
}

template<typename T>
static std::vector<uint32_t> BinnedHistogram(Image<T>& img, int ch) {
	std::vector<uint32_t>histogram(65536, 0);

	auto t = GetTimePoint();
	for (const T& pixel : image_channel(img, ch))
		histogram[Pixel<uint16_t>::toType(pixel)]++;
	DisplayTimeDuration(t);

	std::vector<uint32_t> bin_hist(256, 0);
	for (int i = 0, s = 0; i < 256; ++i, s += 256) {
		for (int j = 0; j < 256; ++j)
			bin_hist[i] += histogram[s + j];
		bin_hist[i] /= 255;
	}

	return bin_hist;
}

template<typename T>
void HistogramTransformationDialog::showHistogram(Image<T>&img) {
	using enum Component;

	if (m_workspace->subWindowList().size() == 0)
		return;

	m_histogram->removeAllSeries();

	int max = 0;
	QLineSeries* series = nullptr;


	Component id = Component(m_component_bg->checkedId());

	if (id == rgb_k) {
		for (int ch = 0; ch < img.Channels(); ++ch) {
			series = new QLineSeries;

			std::vector<uint32_t> bin_histogram = BinnedHistogram(img, ch);

			for (auto v : bin_histogram)
				if (v > max)
					max = v;

			for (int i = 0; i < 256; ++i)
				series->append(i, bin_histogram[i]);

			if (img.Channels() == 1)
				series->setColor(QColor(255, 255, 255));

			else if (img.Channels() == 3) {
				if (ch == 0)
					series->setColor(QColor(255, 0, 0));
				if (ch == 1)
					series->setColor(QColor(0, 255, 0));
				if (ch == 2)
					series->setColor(QColor(0, 0, 255));
			}
			m_histogram->addSeries(series);
			series->attachAxis(m_axisY);
			series->attachAxis(m_axisX);
		}
	}

	else if ((id == red || id == green || id == blue) && img.Channels() == 3) {
		series = new QLineSeries;
		int ch = int(id);

		std::vector<uint32_t> bin_histogram = BinnedHistogram(img, ch);

		for (auto v : bin_histogram)
			if (v > max)
				max = v;

		for (int i = 0; i < 256; ++i)
			series->append(i, bin_histogram[i]);

		if (ch == 0)
			series->setColor(QColor(255, 0, 0));
		if (ch == 1)
			series->setColor(QColor(0, 255, 0));
		if (ch == 2)
			series->setColor(QColor(0, 0, 255));
		
		m_histogram->addSeries(series);
		series->attachAxis(m_axisY);
		series->attachAxis(m_axisX);
	}

	//m_histogram->createDefaultAxes();
	m_axisY->setRange(0, max);

}
template void HistogramTransformationDialog::showHistogram(Image8&);
template void HistogramTransformationDialog::showHistogram(Image16&);
template void HistogramTransformationDialog::showHistogram(Image32&);

void HistogramTransformationDialog::AddChannelSelection() {

	m_component_bg = new QButtonGroup(this);

	ComponentPushButton* pb = new ComponentPushButton("Red", this);
	m_component_bg->addButton(pb, int(Component::red));

	pb = new ComponentPushButton("Green", this);
	m_component_bg->addButton(pb, int(Component::green));
	pb->move(0, 25);

	pb = new ComponentPushButton("Blue", this);
	m_component_bg->addButton(pb, int(Component::blue));
	pb->move(0, 50);

	pb = new ComponentPushButton("RGB/K", this);
	m_component_bg->addButton(pb, int(Component::rgb_k));
	pb->setChecked(true);
	pb->move(0, 75);

	connect(m_component_bg, &QButtonGroup::idClicked, this, &HistogramTransformationDialog::onClick);
}


void HistogramTransformationDialog::AddLineEdits() {
	m_shadow_le = new DoubleLineEdit("0.00000000", new DoubleValidator(0.0, 1.0, 8, this), this);
	m_shadow_le->setGeometry(0, 430, 95, 25);
	connect(m_shadow_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_shadow);

	m_midtone_le = new DoubleLineEdit("0.50000000", new DoubleValidator(0.0, 1.0, 8, this), this);
	m_midtone_le->setGeometry(0, 460, 95, 25);
	connect(m_midtone_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_midtone);

	m_highlight_le = new DoubleLineEdit("1.00000000", new DoubleValidator(0.0, 1.0, 8, this), this);
	m_highlight_le->setGeometry(0, 490, 95, 25);
	connect(m_highlight_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_highlight);
}

void HistogramTransformationDialog::setHistogramTransformation() {
	Component comp = Component(m_component_bg->checkedId());

	m_ht.setShadow(comp, m_shadow_le->text().toFloat());
	m_ht.setMidtone(comp, m_midtone_le->text().toFloat());
	m_ht.setHighlight(comp, m_highlight_le->text().toFloat());
}

void HistogramTransformationDialog::resetDialog() {
	
	m_shadow_le->setText("0.00000000");
	m_midtone_le->setText("0.50000000");
	m_highlight_le->setText("1.00000000");
	m_histogram_slider->resetSliderPositions();

	m_ht = HistogramTransformation();

	ApplytoPreview();
}

void HistogramTransformationDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void HistogramTransformationDialog::Apply() {

	setHistogramTransformation();

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		m_ht.Apply(iwptr->source);

		if (m_image_sel->currentText() == iwptr->ImageName())
			showHistogram(iwptr->source);

		iwptr->DisplayImage();

		if (iwptr->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);

		m_ht.Apply(iw16->source);

		if (m_image_sel->currentText() == iw16->ImageName())
			showHistogram(iw16->source);

		iw16->DisplayImage();

		if (iw16->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow16*>(iw16->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);

		m_ht.Apply(iw32->source);

		if (m_image_sel->currentText() == iw32->ImageName())
			showHistogram(iw32->source);

		iw32->DisplayImage();

		if (iw32->rtpExists()) {
			reinterpret_cast<RTP_ImageWindow32*>(iw32->rtp)->UpdatefromParent();
			ApplytoPreview();
		}
		break;
	}
	}
}

void HistogramTransformationDialog::ApplytoPreview() {

	setHistogramTransformation();
		
	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	if (!iwptr->rtpExists())
		return;

	if (iwptr->rtp->windowTitle().sliced(19, iwptr->rtp->windowTitle().length() - 19).compare(m_name) != 0)
		return;

	switch (iwptr->source.Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<RTP_ImageWindow8*>(iwptr->rtp);
		iw8->UpdatefromParent();
		m_ht.Apply(iw8->source);
		iwptr->DisplayImage();
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<RTP_ImageWindow16*>(iwptr->rtp);
		iw16->UpdatefromParent();
		m_ht.Apply(iw16->source);
		iw16->DisplayImage();
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<RTP_ImageWindow32*>(iwptr->rtp);
		iw32->UpdatefromParent();
		m_ht.Apply(iw32->source);
		iw32->DisplayImage();
		break;
	}
	}
}