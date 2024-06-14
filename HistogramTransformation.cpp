#include "pch.h"
#include "HistogramTransformation.h"
#include "FastStack.h"

using HT = HistogramTransformation;

void HT::MTFCurve::setShadow(float shadow) {
	m_shadow = shadow;
	dv = 1.0 / (m_highlights - m_shadow);
}

void HT::MTFCurve::setMidtone(float midtone) {
	m_midtone = midtone;
	m1 = m_midtone - 1;
	m2 = 2 * m_midtone - 1;
}

void HT::MTFCurve::setHighlight(float hightlight) {
	m_highlights = hightlight;
	dv = 1.0 / (m_highlights - m_shadow);
}

float HT::MTFCurve::MTF(float pixel) {

	if (pixel <= 0.0f) return 0;

	else if (pixel >= 1) return 1;

	else if (pixel == m_midtone)  return 0.5;

	return (m1 * pixel) / ((m2 * pixel) - m_midtone);

}

float HT::MTFCurve::TransformPixel(float pixel) {
	pixel = (pixel - m_shadow) * dv;

	return MTF(pixel);
}

void HT::MTFCurve::Generate16Bit_LUT() {
	m_lut.resize(65536);
	for (int el = 0; el < 65536; ++el)
		m_lut[el] = TransformPixel(el / 65535.0f) * 65535;
}

void HT::MTFCurve::Generate8Bit_LUT() {
	m_lut.resize(256);
	for (int el = 0; el < 256; ++el)
		m_lut[el] = TransformPixel(el / 255.0f) * 255;
}

template <typename T>
void HT::MTFCurve::ApplyChannel(Image<T>& img, int ch) {
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
template void HT::MTFCurve::ApplyChannel(Image8&, int);
template void HT::MTFCurve::ApplyChannel(Image16&, int);
template void HT::MTFCurve::ApplyChannel(Image32&, int);


float HistogramTransformation::Shadow(ColorComponent component) const{
	return (*this)[component].Shadow();
}

float HistogramTransformation::Midtone(ColorComponent component) const {
	return (*this)[component].Midtone();
}

float HistogramTransformation::Highlight(ColorComponent component) const {
	return (*this)[component].Highlight();
}

void HistogramTransformation::setShadow(ColorComponent component, float shadow) {
	(*this)[component].setShadow(shadow);
}

void HistogramTransformation::setMidtone(ColorComponent component, float midtone) {
	(*this)[component].setMidtone(midtone);
}

void HistogramTransformation::setHighlight(ColorComponent component, float highligt) {
	(*this)[component].setHighlight(highligt);
}

float HistogramTransformation::TransformPixel(ColorComponent component, float pixel) {
	return (*this)[component].TransformPixel(pixel);
}



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

float HistogramTransformation::MTF(float pixel, float midtone) {

	if (pixel <= 0.0f) return 0;

	else if (pixel >= 1) return 1;

	else if (pixel == midtone)  return 0.5;

	return ((midtone - 1) * pixel) / (((2 * midtone - 1) * pixel) - midtone);

}

template<typename T>
void HistogramTransformation::ComputeSTFCurve(Image<T>& img){

	auto rgbk = (*this)[ColorComponent::rgb_k];
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
template void HT::ComputeSTFCurve(Image8&);
template void HT::ComputeSTFCurve(Image16&);
template void HT::ComputeSTFCurve(Image32&);

template<typename T>
void HistogramTransformation::STFStretch(Image<T>& img) {
	auto rgbk = (*this)[ColorComponent::rgb_k];
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
template void HT::STFStretch(Image8&);
template void HT::STFStretch(Image16&);
template void HT::STFStretch(Image32&);

template<typename T>
void HistogramTransformation::Apply(Image<T>& img) {

	using enum ColorComponent;

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
template void HT::Apply(Image8&);
template void HT::Apply(Image16&);
template void HT::Apply(Image32&);



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

	AddStyleSheet();
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
	//QSlider::mousePressEvent(event);
}

void HistogramSlider::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::LeftButton) {

		if (m_shadow_act) {
			int x = m_shadow.sliderPosition + (event->x() - click_x);
			click_x = event->x();

			if (x < minimum())
				x = 0;

			else if (x > m_highlight.sliderPosition && x <= maximum()) {
				x = m_highlight.sliderValue = m_highlight.sliderPosition = m_highlight.sliderPosition + 1;
				emit sliderMoved_highlight(m_highlight.sliderPosition);
			}

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

			if (x > maximum())
				x = maximum();

			else if (x < m_shadow.sliderPosition && x >= minimum()) {
				x = m_shadow.sliderValue = m_shadow.sliderPosition = m_shadow.sliderPosition - 1;
				emit sliderMoved_shadow(m_shadow.sliderPosition);
			}

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
	m_shadow.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
	//QPixmap pm = style()->standardPixmap(QStyle::SP_TitleBarShadeButton);
	//pm = pm.scaled(18,18, Qt::KeepAspectRatio);
	//style()->drawItemPixmap(&p, m_shadow.rect, 0, pm);
	// 
	//midtone handle
	opt.sliderPosition = m_midtone.sliderPosition;
	m_midtone.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);

	//highlight handle
	opt.sliderPosition = m_highlight.sliderPosition;
	m_highlight.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	style()->drawComplexControl(QStyle::CC_Slider, &opt, &p, this);
}

void HistogramSlider::AddStyleSheet() {
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

	this->setStyleSheet(str);
}









HistogramTransformationDialog::HistogramTransformationDialog(QWidget* parent): ProcessDialog("HistogramTransformation", QSize(420, 400), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent) {

	using HTD = HistogramTransformationDialog;

	setTimer(250, this, &HTD::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &HTD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &HTD::Apply, &HTD::showPreview, &HTD::resetDialog);
	
	AddHistogramChart();
	AddHistogramSlider();
	AddLineEdits();
	AddChannelSelection();
	AddImageSelectionCombo();
	AddMTFPushButton();
	AddResolutionCombo();

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
			resetScene();

		m_image_sel->removeItem(index);
		m_image_sel->setCurrentIndex(0);
}


void HistogramTransformationDialog::onActivation_imageSelection(int index) {
	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == ptr->ImageName()) {
			switch (ptr->Source().Bitdepth()) {
			case 8:
				showHistogram(ptr->Source());
				break;
			case 16:
				showHistogram(reinterpret_cast<ImageWindow16*>(ptr)->Source());
				break;
			case -32:
				showHistogram(reinterpret_cast<ImageWindow32*>(ptr)->Source());
				break;
			}
			return ApplytoPreview();
		}

		else
			resetScene();
	}
}

void HistogramTransformationDialog::onActivation_resolution(int index) {

	std::array<int, 5> r = { 8,10,12,14,16 };
	m_current_hist_res = r[index];

	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == ptr->ImageName()) {
			switch (ptr->Source().Bitdepth()) {
			case 8:
				showHistogram(ptr->Source());
				break;
			case 16:
				showHistogram(reinterpret_cast<ImageWindow16*>(ptr)->Source());
				break;
			case -32:
				showHistogram(reinterpret_cast<ImageWindow32*>(ptr)->Source());
				break;
			}
			return ApplytoPreview();
		}

		else
			resetScene();
	}
}


void HistogramTransformationDialog::onClick(int id) {

	m_current_comp = ColorComponent(id);

	float shadow = m_ht.Shadow(m_current_comp);
	float midtone = m_ht.Midtone(m_current_comp);
	float highlight = m_ht.Highlight(m_current_comp);

	m_shadow_le->setText(QString::number(shadow, 'f', 8));
	m_midtone_le->setText(QString::number(midtone, 'f', 8));
	m_highlight_le->setText(QString::number(highlight, 'f', 8));

	m_histogram_slider->setSliderPosition_shadow(shadow * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_midtone((midtone + shadow) * (highlight + shadow) * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_highlight(highlight * m_histogram_slider->maximum());

	m_histogram_slider->setMedian(midtone);

	showMTFCurve();

	onActivation_imageSelection(m_image_sel->currentIndex());

}


void HistogramTransformationDialog::sliderMoved_shadow(int value) {
	float s = float(value) / m_histogram_slider->maximum();

	m_shadow_le->setValue(s);
	m_ht.setShadow(m_current_comp, s);
	showMTFCurve();

	startTimer();
}

void HistogramTransformationDialog::sliderMoved_midtone(int value) {
	int s = m_histogram_slider->sliderPosition_shadow();
	int h = m_histogram_slider->sliderPosition_highlight();
	float m = float(value - s) / (h - s);

	m_midtone_le->setValue(m);
	m_ht.setMidtone(m_current_comp, m);

	showMTFCurve();

	startTimer();
}

void HistogramTransformationDialog::sliderMoved_highlight(int value) {
	float h = double(value) / m_histogram_slider->maximum();

	m_highlight_le->setValue(h);
	m_ht.setHighlight(m_current_comp, h);
	showMTFCurve();

	startTimer();
}

void HistogramTransformationDialog::editingFinished_smh() {
	float shadow = m_shadow_le->text().toFloat();
	float midtone = m_midtone_le->text().toFloat();
	float highlight = m_highlight_le->text().toFloat();

	int s = shadow * m_histogram_slider->maximum();
	int h = m_highlight_le->text().toFloat() * m_histogram_slider->maximum();

	if (s > h)
		s = h;
	if (h < s)
		h = s;

	m_histogram_slider->setSliderPosition_shadow(s);
	m_ht.setShadow(m_current_comp, shadow);

	m_histogram_slider->setSliderPosition_midtone(midtone * (h - s) + s);
	m_ht.setMidtone(m_current_comp, midtone);

	m_histogram_slider->setSliderPosition_highlight(h);
	m_ht.setHighlight(m_current_comp, highlight);

	showMTFCurve();

	ApplytoPreview();
}



void HistogramTransformationDialog::AddHistogramChart() {

	for (auto pen : m_pens)
		pen.setWidthF(0.9);

	m_gs = new QGraphicsScene(0, 0, 400, 200);
	m_gs->setBackgroundBrush(QBrush("#404040"));

	m_gv = new QGraphicsView(m_gs, this);
	m_gv->setRenderHints(QPainter::Antialiasing);
	m_gv->scale(1, -1);
	m_gv->setGeometry(10, 10, 400, 200);
	m_gv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_gv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	addGrid();
}

void HistogramTransformationDialog::resetScene() {
	m_gs->clear();
	m_mtf_curve = nullptr;
	addGrid();
	showMTFCurve();
}

void HistogramTransformationDialog::addGrid() {

	QPen pen = QColor(255, 255, 255);
	pen.setWidthF(0.5);

	for (int i = 50; i < 200; i += 50)
		m_gs->addLine(0, i, 400, i, pen);

	for (int x = 100; x < 400; x += 100)
		m_gs->addLine(x, 0, x, 200, pen);
}

template<typename T>
void HistogramTransformationDialog::showHistogram(Image<T>& img) {

	using enum ColorComponent;

	if (m_workspace->subWindowList().size() == 0)
		return;

	resetScene();

	ColorComponent id = m_current_comp;

	if (img.Channels() == 1 && (id == red || id == green || id == blue))
		return;

	int max = 0;
	int bins = m_gs->width();

	QPolygonF line(m_gs->width());

	int start = 0;
	int end = img.Channels();

	if (id == red || id == green || id == blue)
		end = 1 + (start = int(id));

	std::array<Histogram, 3> hist_array;

	for (int ch = start; ch < end; ++ch) {

		hist_array[ch].ConstructHistogram(img.cbegin(ch), img.cend(ch));
		Histogram* histogram = &hist_array[ch];
		histogram->Resample(m_current_hist_res);

		for (int i = 1; i < (*histogram).Size() - 1; ++i)
			if ((*histogram)[i] > max)
				max = (*histogram)[i];
	}

	float spb = float(hist_array[start].Size()) / bins;

	for (int ch = start; ch < end; ++ch) {
		Histogram* histogram = &hist_array[ch];

		float s = 0;
		for (int i = 0; i < bins; ++i, s += spb) {

			int M = 0;

			for (int j = 0; j < spb; ++j)
				if ((*histogram)[s + j] > M)
					M = (*histogram)[s + j];


			line[i] = QPointF(i, (double(M) / max) * m_gs->height());
		}

		QPainterPath path;
		path.addPolygon(line);
		if (img.Channels() == 3)
			m_gs->addPath(path, m_pens[ch]);
		else 
			m_gs->addPath(path, m_pens[3]);
	}

}
template void HistogramTransformationDialog::showHistogram(Image8&);
template void HistogramTransformationDialog::showHistogram(Image16&);
template void HistogramTransformationDialog::showHistogram(Image32&);

void HistogramTransformationDialog::showMTFCurve() {

	if (m_mtf_curve != nullptr)
		m_gs->removeItem(m_mtf_curve);

	QPolygonF line(m_gs->width());
	for (int i = 0; i < m_gs->width(); ++i)
		line[i] = QPointF(i, m_ht.TransformPixel(m_current_comp, 0.5 * i / m_gs->height()) * m_gs->height());

	QPainterPath path;
	path.addPolygon(line);
	m_mtf_curve = m_gs->addPath(path, m_pens[m_component_bg->checkedId()]);
	
	if (!m_mtf_curve_pb->isChecked())
		m_mtf_curve->setVisible(false);
}

void HistogramTransformationDialog::AddHistogramSlider() {
	m_histogram_slider = new HistogramSlider(Qt::Horizontal, this);

	m_histogram_slider->setFixedWidth(412);//orrig 392
	m_histogram_slider->setRange(0, 400);//orig380
	//m_histogram_slider->style()->standardIcon(QStyle::SP_TitleBarShadeButton);
	m_histogram_slider->move(4, 208);

	connect(m_histogram_slider, &HistogramSlider::sliderMoved_shadow, this, &HistogramTransformationDialog::sliderMoved_shadow);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_midtone, this, &HistogramTransformationDialog::sliderMoved_midtone);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_highlight, this, &HistogramTransformationDialog::sliderMoved_highlight);
}


void HistogramTransformationDialog::AddChannelSelection() {

	m_component_bg = new QButtonGroup(this);

	ComponentPushButton* pb = new ComponentPushButton("Red", this);
	m_component_bg->addButton(pb, int(ColorComponent::red));
	pb->move(350, 235);

	pb = new ComponentPushButton("Green", this);
	m_component_bg->addButton(pb, int(ColorComponent::green));
	pb->move(300, 235);

	pb = new ComponentPushButton("Blue", this);
	m_component_bg->addButton(pb, int(ColorComponent::blue));
	pb->move(250, 235);

	pb = new ComponentPushButton("RGB/K", this);
	m_component_bg->addButton(pb, int(ColorComponent::rgb_k));
	pb->setChecked(true);
	pb->move(200, 235);

	for (auto b : m_component_bg->buttons())
		b->resize(50, 25);

	connect(m_component_bg, &QButtonGroup::idClicked, this, &HistogramTransformationDialog::onClick);
}

void HistogramTransformationDialog::AddLineEdits() {
	m_shadow_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
	m_shadow_le->setValue(0.0);
	m_shadow_le->move(85, 235);
	m_shadow_le->addLabel(new QLabel("Shadow:   ", this));
	//connect(m_shadow_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_shadow);

	m_midtone_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
	m_midtone_le->setValue(0.5);
	m_midtone_le->move(85, 270);
	m_midtone_le->addLabel(new QLabel("Midtone:   ", this));
	//connect(m_midtone_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_midtone);

	m_highlight_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 8), this);
	m_highlight_le->setValue(1.0);
	m_highlight_le->move(85, 305);
	m_highlight_le->addLabel(new QLabel("Highlight:   ", this));
	//connect(m_highlight_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_highlight);

	connect(m_shadow_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_smh);
	connect(m_midtone_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_smh);
	connect(m_highlight_le, &QLineEdit::editingFinished, this, &HistogramTransformationDialog::editingFinished_smh);

}

void HistogramTransformationDialog::AddImageSelectionCombo() {
	using HTD = HistogramTransformationDialog;

	m_image_sel = new QComboBox(this);
	m_image_sel->addItem("No Image Selected");
	m_image_sel->move(225, 305);
	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->ImageName());

	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendClose, this, &HTD::onWindowClose);
	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendOpen, this, &HTD::onWindowOpen);
	connect(m_image_sel, &QComboBox::activated, this, &HTD::onActivation_imageSelection);
}

void HistogramTransformationDialog::AddMTFPushButton() {
	m_mtf_curve_pb = new QPushButton("MTF Curve", this);
	m_mtf_curve_pb->setCheckable(true);
	m_mtf_curve_pb->setChecked(true);
	m_mtf_curve_pb->setAutoDefault(false);
	m_mtf_curve_pb->move(260, 270);
	connect(m_mtf_curve_pb, &QPushButton::clicked, this, &HistogramTransformationDialog::onButtonPress);
	showMTFCurve();
}

void HistogramTransformationDialog::AddResolutionCombo() {
	QComboBox* hr = new QComboBox(this);
	hr->addItems({ "8-bit", "10-bit", "12-bit", "14-bit", "16-bit" });
	hr->setCurrentIndex(4);
	hr->move(250, 340);
	connect(hr, &QComboBox::activated, this, &HistogramTransformationDialog::onActivation_resolution);

	QLabel* l = new QLabel("Histogram Resolution:   ", this);
	l->move(hr->geometry().x() - l->fontMetrics().horizontalAdvance(l->text()), 342);
}


void HistogramTransformationDialog::resetDialog() {

	m_ht = HistogramTransformation();
	
	m_shadow_le->setValue(m_ht.Shadow(ColorComponent::rgb_k));
	m_midtone_le->setValue(m_ht.Midtone(ColorComponent::rgb_k));
	m_highlight_le->setValue(m_ht.Highlight(ColorComponent::rgb_k));
	m_histogram_slider->resetSliderPositions();

	showMTFCurve();

	ApplytoPreview();
}

void HistogramTransformationDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void HistogramTransformationDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_ht, &HT::Apply);

		if (m_image_sel->currentText() == iwptr->ImageName())
			showHistogram(iwptr->Source());

		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_ht, &HT::Apply);

		if (m_image_sel->currentText() == iw16->ImageName())
			showHistogram(iw16->Source());

		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_ht, &HT::Apply);

		if (m_image_sel->currentText() == iw32->ImageName())
			showHistogram(iw32->Source());

		break;
	}
	}

	setEnabledAll(true);

	ApplytoPreview();
}

void HistogramTransformationDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		return iw8->UpdatePreview(m_ht, &HT::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		return iw16->UpdatePreview(m_ht, &HT::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		return iw32->UpdatePreview(m_ht, &HT::Apply);
	}
	}
}