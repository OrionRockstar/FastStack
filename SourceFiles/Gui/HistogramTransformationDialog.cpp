#include "pch.h"
#include "FastStack.h"
#include "ImageWindow.h"
#include "HistogramTransformationDialog.h"

int HistogramSlider::computeMidtone() {

	auto& shadow = m_handles[Tone::shadow].option;
	auto& highlight = m_handles[Tone::highlight].option;

	return shadow.sliderPosition + (highlight.sliderPosition - shadow.sliderPosition) * m_med;
}

HistogramSlider::HistogramSlider(QWidget* parent) : QSlider(Qt::Horizontal, parent) {

	this->setFixedWidth(412);
	this->setRange(0, 400);

	for (int i = 0; i < m_handles.data.size(); ++i) {
		auto& opt = m_handles.data[i].option;
		initStyleOption(&opt);
		opt.subControls = QStyle::SC_SliderHandle;
		opt.sliderValue = opt.sliderPosition = (maximum() * i) / 2;
	}
}

void HistogramSlider::resetSlider() {

	for (int i = 0; i < m_handles.data.size(); ++i) {
		auto& opt = m_handles.data[i].option;
		opt.sliderValue = opt.sliderPosition = (maximum() * i) / 2;
	}
	m_med = 0.5;
	update();
}

void HistogramSlider::setSliderPosition_shadow(int value) {

	auto& shadow = m_handles[Tone::shadow].option;
	auto& midtone = m_handles[Tone::midtone].option;
	auto& highlight = m_handles[Tone::highlight].option;

	shadow.sliderValue = shadow.sliderPosition = value = math::clip(value, this->minimum(), this->maximum());

	if (highlight.sliderPosition <= value && value <= maximum())
		emit sliderMoved_highlight(highlight.sliderValue = highlight.sliderPosition = math::min(value + 1, maximum()));

	midtone.sliderValue = midtone.sliderPosition = computeMidtone();
	update();
}

void HistogramSlider::setSliderPosition_midtone(int value) {

	int shadow_pos = m_handles[Tone::shadow].option.sliderPosition;
	auto& midtone = m_handles[Tone::midtone].option;
	int highlight_pos = m_handles[Tone::highlight].option.sliderPosition;

	m_med = value / double(maximum());
	midtone.sliderValue = midtone.sliderPosition = shadow_pos + (highlight_pos - shadow_pos) * m_med;

	update();
}

void HistogramSlider::setSliderPosition_highlight(int value) {

	auto& shadow = m_handles[Tone::shadow].option;
	auto& midtone = m_handles[Tone::midtone].option;
	auto& highlight = m_handles[Tone::highlight].option;

	highlight.sliderValue = highlight.sliderPosition = value = math::clip(value, this->minimum(), this->maximum());

	if (minimum() <= value && value <= shadow.sliderPosition)
		emit sliderMoved_shadow(shadow.sliderValue = shadow.sliderPosition = math::max(value - 1, 0));

	midtone.sliderValue = midtone.sliderPosition = computeMidtone();
	update();
}

void HistogramSlider::mousePressEvent(QMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {
		for (auto& handle : m_handles.data) {
			if (handle.option.rect.contains(event->pos())) {
				handle.active = true;
				return;
			}
		}
	}
}

void HistogramSlider::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {
		int diff = width() - maximum();
		int x = QStyle::sliderValueFromPosition(minimum(), maximum(), e->x() - diff / 2, width() - diff, false);

		auto& shadow_handle = m_handles[Tone::shadow];
		auto& midtone_handle = m_handles[Tone::midtone];
		auto& highlight_handle = m_handles[Tone::highlight];

		if (shadow_handle.active) {
			setSliderPosition_shadow(x);
			emit sliderMoved_shadow(x);
		}

		if (midtone_handle.active) {
			auto& shadow = shadow_handle.option;
			auto& midtone = midtone_handle.option;
			auto& highlight = highlight_handle.option;
			midtone.sliderValue = midtone.sliderPosition = x = math::clip(x, shadow.sliderPosition, highlight.sliderPosition);
			m_med = (x - shadow.sliderPosition) / double(highlight.sliderPosition - shadow.sliderPosition);
			update();
			emit sliderMoved_midtone(x);
		}

		if (highlight_handle.active) {
			setSliderPosition_highlight(x);
			emit sliderMoved_highlight(x);
		}
	}
}

void HistogramSlider::mouseReleaseEvent(QMouseEvent* e) {

	for (auto& handle : m_handles.data)
		handle.active = false;
}

void HistogramSlider::paintEvent(QPaintEvent* event) {

	QStylePainter p(this);
	p.setRenderHint(QPainter::Antialiasing);

	QStyleOptionSlider opt;
	initStyleOption(&opt);

	QPen pen;
	pen.setWidthF(0.5);
	pen.setColor(Qt::white);
	p.setPen(pen);

	//trackbar
	opt.subControls = QStyle::SC_SliderGroove;
	auto trackbar_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove);
	trackbar_rect.translate(0, 2);
	trackbar_rect.setHeight(6);
	p.drawRect(trackbar_rect);

	p.setPen("#5c5c5c");

	//handles
	auto& shadow = m_handles[Tone::shadow].option;
	drawHandle(shadow, p);

	auto& midtone = m_handles[Tone::midtone].option;
	drawHandle(midtone, p);

	auto& highlight = m_handles[Tone::highlight].option;
	drawHandle(highlight, p);
}

void HistogramSlider::drawHandle(QStyleOptionSlider& handle, QPainter& p) {

	QStyleOptionSlider opt;
	initStyleOption(&opt);
	opt.subControls = QStyle::SC_SliderHandle;
	opt.sliderPosition = handle.sliderPosition;
	QRectF r = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);

	float dx = (r.width() - m_handle_width) / 2.0;
	r.translate((opt.sliderPosition - r.x()) - (dx - (m_handle_width / 2)), 0);
	r.adjust(dx, 4, -dx, 0);

	if (r.height() < m_handle_height) {
		int dy = r.height() - m_handle_height;
		r.adjust(0, 0, 0, -dy);
	}

	handle.rect = r.toRect();

	QPainterPath pp;
	pp.moveTo(r.bottomLeft().x(), r.bottomLeft().y() + 1);

	float hw = r.width() / 2.0;
	int h = r.height() - hw;

	pp.lineTo(r.x(), h);
	pp.lineTo(r.x() + hw, r.y());
	pp.lineTo(r.topRight().x() + 1, h);
	pp.lineTo(r.bottomRight().x() + 1, r.bottomRight().y() + 1);
	pp.closeSubpath();

	p.drawPath(pp);
	p.fillPath(pp, m_brush);
}





using HTV = HistogramTransformationView;
HTV::HistogramTransformationView(HistogramTransformation& ht, const QSize& size, QWidget* parent) : m_ht(&ht), HistogramView(size, parent) {

	drawMTFCurve();
	this->show();
}

void HTV::clearScene(bool draw_grid) {

	HistogramView::clearScene(draw_grid);
	m_mtf_curve = nullptr;
	m_highlight_dash = m_midtone_dash = nullptr;
}

void HTV::clearHistogram() {

	HistogramView::clearHistogram();
	this->drawMTFCurve();
}

void HTV::drawHistogram() {

	HistogramView::drawHistogram();
	drawMTFCurve();
}

void HTV::setMTFVisible(bool visible) {

	m_mtf_visible = visible;
	m_mtf_curve->setVisible(visible);
	m_midtone_dash->setVisible(visible);
	m_highlight_dash->setVisible(visible);
}

void HTV::drawMTFCurve() {

	if (m_mtf_curve != nullptr && !m_mtf_curve->isVisible())
		return;

	//pointer may be created when drawing grid
	for (auto item : scene()->items())
		if (item == m_mtf_curve || item == m_highlight_dash || item == m_midtone_dash)
			scene()->removeItem(item);

	int w = scene()->width();
	int h = scene()->height();

	int start = m_ht->shadow(colorComponent()) * w;
	int end = m_ht->highlight(colorComponent()) * w;
	int mid = start + (end - start) * m_ht->midtone(colorComponent());

	QPolygonF line(end - start + 2);

	double s = m_ht->shadow(colorComponent());
	double dx = 1.0 / w;

	for (int i = 0; i < line.size(); ++i, s += dx)
		line[i] = QPointF(start + i, (1 - m_ht->transformPixel(colorComponent(), s)) * h);

	QPainterPath mtf_path;
	mtf_path.addPolygon(line);
	m_mtf_curve = scene()->addPath(mtf_path, m_pens[int(colorComponent())]);

	QPen pen = m_pens[int(colorComponent())];
	pen.setDashPattern({ 10,5 });
	QColor c = pen.color();
	pen.setColor({ c.red(),c.green(),c.blue(),169 });

	m_highlight_dash = scene()->addLine(QLine( QPoint(end,0),{end, h}), pen);
	m_midtone_dash = scene()->addLine(QLine(QPoint(mid, line[mid - start].y()), { mid, h }), pen);

	m_mtf_curve->setVisible(m_mtf_visible);
	m_midtone_dash->setVisible(m_mtf_visible);
	m_highlight_dash->setVisible(m_mtf_visible);
}



using HTD = HistogramTransformationDialog;

HTD::HistogramTransformationDialog(Workspace* parent) : ProcessDialog("HistogramTransformation", QSize(422, 600), parent) {

	setDefaultTimerInterval(150);

	m_hv = new HistogramView({ 402, 200 }, drawArea());
	m_hv->move(10, 10);

	m_htv = new HistogramTransformationView(m_ht, { 402,200 }, drawArea());
	m_htv->move(10, 220);

	QWidget* w = new QWidget(drawArea());
	w->setGeometry(10, 214, 402, 2);
	w->setStyleSheet("QWidget{background: rgb(69,0,128);}");
	
	addHistogramSlider();
	addLineEdits();
	addChannelSelection();
	addImageSelectionCombo();
	addMTFPushButton();
	addResolutionCombo();

	this->show();
}

void HTD::onImageWindowCreated() {

	m_image_sel->addImage(imageRecast<>(workspace()->subWindowList().last()->widget()));
}

void HTD::onImageWindowClosed() {

	int index = m_image_sel->findImage(&imageRecast<>(workspace()->currentSubWindow()->widget())->source());

	if (index == m_image_sel->currentIndex()) {
		m_current_histogram.clear();
		m_htv->clearHistogram();
		m_hv->clearHistogram();
		m_image_sel->setCurrentIndex(0);
	}

	m_image_sel->removeItem(index);
}

void HTD::onClick(int id) {

	m_current_comp = ColorComponent(id);
	m_htv->setColorComponent(m_current_comp);
	m_histogram_slider->setHandleColor(m_colors[id]);

	float shadow = m_ht.shadow(m_current_comp);
	float midtone = m_ht.midtone(m_current_comp);
	float highlight = m_ht.highlight(m_current_comp);

	m_shadow_le->setValue(shadow);
	m_midtone_le->setValue(midtone);
	m_highlight_le->setValue(highlight);

	m_histogram_slider->setSliderPosition(Tone::shadow, toPos(shadow));
	m_histogram_slider->setSliderPosition(Tone::midtone, toPos(midtone));
	m_histogram_slider->setSliderPosition(Tone::highlight, toPos(highlight));

	m_htv->drawHistogram();
}

void HTD::onImageSelection() {

	auto img = m_image_sel->currentImage();
	if (img) {
		m_current_histogram.resize(img->channels());
		for (int ch = 0; ch < img->channels(); ++ch) {
			switch (img->type()) {
			case ImageType::UBYTE:
				m_current_histogram[ch].constructHistogram(*img, ch);
				break;

			case ImageType::USHORT:
				m_current_histogram[ch].constructHistogram(*recastImage<uint16_t>(img), ch);
				break;

			case ImageType::FLOAT:
				m_current_histogram[ch].constructHistogram(*recastImage<float>(img), ch);
				break;
			}
		}
		m_htv->setHistogram(m_current_histogram);
		return onChange();
	}

	/*for (auto sw : m_workspace->subWindowList()) {
		auto ptr = imageRecast(sw->widget());
		if (m_image_sel->currentText() == ptr->name()) {
			m_current_histogram.resize(ptr->source().channels());
			for (int ch = 0; ch < ptr->source().channels(); ++ch) {
				switch (ptr->type()) {
				case ImageType::UBYTE:
					m_current_histogram[ch].constructHistogram(ptr->source(), ch);
					break;

				case ImageType::USHORT:
					m_current_histogram[ch].constructHistogram(imageRecast<uint16_t>(ptr)->source(), ch);
					break;

				case ImageType::FLOAT:
					m_current_histogram[ch].constructHistogram(imageRecast<float>(ptr)->source(), ch);
					break;
				}
			}
			m_htv->setHistogram(m_current_histogram);
			return onChange();
		}
	}*/

	m_current_histogram.clear();
	m_hv->clearHistogram();
	m_htv->clearHistogram();
}

void HTD::onResolutionSelection() {

	auto hr = Histogram::Resolution(m_hist_res_combo->currentData().toInt());

	if (m_current_histogram.size() > 0) {
		auto histograms = m_current_histogram;
		for (auto& hist : histograms)
			hist.resample(hr);
		m_htv->setHistogram(histograms);
		onChange();
	}
}

void HTD::onChange() {

	auto hr = Histogram::Resolution(m_hist_res_combo->currentData().toInt());
	HistogramVector transformed(m_current_histogram.size());

	for (int ch = 0; ch < transformed.size(); ++ch) {
		transformed[ch] = m_ht.transformHistogram(ColorComponent::rgb_k, m_current_histogram[ch]);

		if (transformed.size() > 1)
			transformed[ch] = m_ht.transformHistogram(ColorComponent(ch), transformed[ch]);

		transformed[ch].resample(hr);
	}

	m_hv->setHistogram(transformed);
}

void HTD::addHistogramSlider() {

	m_histogram_slider = new HistogramSlider(drawArea());

	m_histogram_slider->move(5, 418);

	auto shadow = [this](int pos) {
		float s = toValue(pos);
		m_shadow_le->setValue(s);
		m_ht.setShadow(m_current_comp, s);
		m_htv->drawMTFCurve();
		startTimer();
	};

	auto midtone = [this](int pos) {
		int s = m_histogram_slider->sliderPosition(Tone::shadow);
		int h = m_histogram_slider->sliderPosition(Tone::highlight);
		float m = float(pos - s) / (h - s);
		m_midtone_le->setValue(m);
		m_ht.setMidtone(m_current_comp, m);
		m_htv->drawMTFCurve();
		startTimer();
	};

	auto highlight = [this](int pos) {
		float h = toValue(pos);
		m_highlight_le->setValue(h);
		m_ht.setHighlight(m_current_comp, h);
		m_htv->drawMTFCurve();
		startTimer();
	};

	connect(m_histogram_slider, &HistogramSlider::sliderMoved_shadow, this, shadow);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_midtone, this, midtone);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_highlight, this, highlight);
}

void HTD::addChannelSelection() {

	using CC = ColorComponent;

	m_component_bg = new QButtonGroup(this);

	ComponentPushButton* pb = new ComponentPushButton("Red", drawArea());
	m_component_bg->addButton(pb, int(CC::red));
	pb->move(250, 450);

	pb = new ComponentPushButton("Green", drawArea());
	m_component_bg->addButton(pb, int(CC::green));
	pb->move(300, 450);

	pb = new ComponentPushButton("Blue", drawArea());
	m_component_bg->addButton(pb, int(CC::blue));
	pb->move(350, 450);

	pb = new ComponentPushButton("RGB/K", drawArea());
	m_component_bg->addButton(pb, int(CC::rgb_k));
	pb->setChecked(true);
	pb->move(200, 450);

	for (auto b : m_component_bg->buttons())
		b->resize(50, 25);

	connect(m_component_bg, &QButtonGroup::idClicked, this, &HistogramTransformationDialog::onClick);
}

void HTD::addLineEdits() {

	auto validator = new DoubleValidator(0.0, 1.0, 6);
	m_shadow_le = new DoubleLineEdit(m_ht.shadow(m_current_comp), validator, drawArea());
	m_shadow_le->move(85, 450);
	m_shadow_le->addLabel(new QLabel("Shadow:   ", drawArea()));

	m_midtone_le = new DoubleLineEdit(m_ht.midtone(m_current_comp), validator, drawArea());
	m_midtone_le->move(85, 485);
	m_midtone_le->addLabel(new QLabel("Midtone:   ", drawArea()));

	m_highlight_le = new DoubleLineEdit(m_ht.midtone(m_current_comp), validator, drawArea());
	m_highlight_le->move(85, 520);
	m_highlight_le->addLabel(new QLabel("Highlight:   ", drawArea()));

	auto shadow_edited = [this]() {
		float s = m_shadow_le->valuef();
		m_histogram_slider->setSliderPosition(Tone::shadow, toPos(s));
		m_ht.setShadow(m_current_comp, s);
		m_htv->drawMTFCurve();
		applytoPreview();
	};

	auto midtone_edited = [this]() {
		float m = m_midtone_le->valuef();
		m_histogram_slider->setSliderPosition(Tone::midtone, toPos(m));
		m_ht.setMidtone(m_current_comp, m);
		m_htv->drawMTFCurve();
		applytoPreview();
	};

	auto highlight_edited = [this]() {
		float h = m_highlight_le->valuef();
		m_histogram_slider->setSliderPosition(Tone::highlight, toPos(h));
		m_ht.setHighlight(m_current_comp, h);
		m_htv->drawMTFCurve();
		applytoPreview();
	};

	connect(m_shadow_le, &QLineEdit::editingFinished, this, shadow_edited);
	connect(m_midtone_le, &QLineEdit::editingFinished, this, midtone_edited);
	connect(m_highlight_le, &QLineEdit::editingFinished, this, highlight_edited);
}

void HTD::addImageSelectionCombo() {

	m_image_sel = new ImageComboBox(drawArea());
	m_image_sel->addItem("No Image Selected");
	m_image_sel->move(200, 520);
	m_image_sel->setFixedWidth(200);

	for (auto sw : workspace()->subWindowList())
		m_image_sel->addImage(imageRecast<>(sw->widget()));

	connect(m_image_sel, &QComboBox::activated, this, &HTD::onImageSelection);
}

void HTD::addMTFPushButton() {

	m_mtf_curve_pb = new CheckablePushButton("MTF Curve", drawArea());
	m_mtf_curve_pb->setChecked(true);
	m_mtf_curve_pb->move(260, 485);
	connect(m_mtf_curve_pb, &QPushButton::clicked, this, [this](bool v) { m_htv->setMTFVisible(v); m_htv->drawMTFCurve(); });
}

void HTD::addResolutionCombo() {

	m_hist_res_combo = new ComboBox(drawArea());
	m_hist_res_combo->addItems({ "8-bit", "10-bit", "12-bit", "14-bit", "16-bit" });


	for (int i = 0, s = 8; i < m_hist_res_combo->count(); ++i, s += 2)
		m_hist_res_combo->setItemData(i, 1 << s);

	m_hist_res_combo->setCurrentIndex(4);
	m_hist_res_combo->move(253, 560);
	connect(m_hist_res_combo, &QComboBox::activated, this, &HTD::onResolutionSelection);

	addLabel(m_hist_res_combo, new QLabel("Histogram Resolution:", drawArea()));
}

void HTD::resetDialog() {

	m_ht = HistogramTransformation();

	m_shadow_le->setValue(m_ht.shadow(ColorComponent::rgb_k));
	m_midtone_le->setValue(m_ht.midtone(ColorComponent::rgb_k));
	m_highlight_le->setValue(m_ht.highlight(ColorComponent::rgb_k));
	m_histogram_slider->resetSlider();
	m_mtf_curve_pb->clicked(true);

	applytoPreview();
}

void HTD::apply() {

	if (!workspace()->hasSubWindows())
		return;

	switch (currentImageType()) 
	case ImageType::UBYTE: {
		currentImageWindow()->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	
	case ImageType::USHORT: 
		currentImageWindow<uint16_t>()->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	
	case ImageType::FLOAT: 
		currentImageWindow<float>()->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	}

	onImageSelection();
}

void HTD::applyPreview() {

	onChange();

	if (!isPreviewValid())
		return;

	switch (preview()->type()) {
	case ImageType::UBYTE: 
		return preview()->updatePreview(m_ht, &HistogramTransformation::apply);
	
	case ImageType::USHORT: 
		return preview<uint16_t>()->updatePreview(m_ht, &HistogramTransformation::apply);
	
	case ImageType::FLOAT: 
		return preview<float>()->updatePreview(m_ht, &HistogramTransformation::apply);
	}
}