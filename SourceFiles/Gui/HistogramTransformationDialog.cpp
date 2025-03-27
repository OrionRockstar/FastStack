#include "pch.h"
#include "FastStack.h"
#include "ImageWindow.h"
#include "HistogramTransformationDialog.h"

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

	initStyleOption(&m_trackbar);
	m_trackbar.subControls = QStyle::SC_SliderGroove;

	//AddStyleSheet();
}

void HistogramSlider::setMedian(float median) { m_med = median; }

int HistogramSlider::sliderPosition_shadow()const { return m_shadow.sliderPosition; }

void HistogramSlider::setSliderPosition_Shadow(int pos) {
	m_shadow.sliderValue = m_shadow.sliderPosition = pos;
	emit valueChanged(pos);
	update();
}

int HistogramSlider::sliderPosition_midtone()const { return m_midtone.sliderPosition; }

void HistogramSlider::setSliderPosition_Midtone(int pos) {
	m_midtone.sliderValue = m_midtone.sliderPosition = pos;
	emit valueChanged(pos);
	update();
}

int HistogramSlider::sliderPosition_highlight()const { return m_highlight.sliderPosition; }

void HistogramSlider::setSliderPosition_Highlight(int pos) {
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

		if (style()->hitTestComplexControl(QStyle::CC_Slider, &m_trackbar, event->pos(), this) == QStyle::SC_SliderGroove) {
			m_shadow_act = m_midtone_act = m_highlight_act = false;
		}

		if (style()->hitTestComplexControl(QStyle::CC_Slider, &m_shadow, event->pos(), this) == QStyle::SC_SliderHandle) {
			m_shadow_act = true;
			m_midtone_act = m_highlight_act = false;
		}

		if (style()->hitTestComplexControl(QStyle::CC_Slider, &m_midtone, event->pos(), this) == QStyle::SC_SliderHandle) {
			m_midtone_act = true;
			m_shadow_act = m_highlight_act = false;
		}

		if (style()->hitTestComplexControl(QStyle::CC_Slider, &m_highlight, event->pos(), this) == QStyle::SC_SliderHandle) {
			m_highlight_act = true;
			m_shadow_act = m_midtone_act = false;
		}
	}
}

void HistogramSlider::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton) {

																			//m_handle_width / 2 is orig
		int x = QStyle::sliderValueFromPosition(minimum(), maximum(), e->x() - m_handle_width, width() - (3 * m_handle_width), false);

		if (m_shadow_act) {

			m_shadow.sliderValue = m_shadow.sliderPosition = x;

			if (x >= m_highlight.sliderPosition && x < maximum())
				emit sliderMoved_highlight(m_highlight.sliderValue = m_highlight.sliderPosition = m_shadow.sliderPosition + 1);

			m_midtone.sliderValue = m_midtone.sliderPosition = m_shadow.sliderPosition + (m_highlight.sliderPosition - m_shadow.sliderPosition) * m_med;

			emit sliderMoved_shadow(m_shadow.sliderPosition);
		}

		if (m_midtone_act) {

			if (x < m_shadow.sliderPosition)
				x = m_shadow.sliderPosition;

			else if (x > m_highlight.sliderPosition)
				x = m_highlight.sliderPosition;

			m_midtone.sliderValue = m_midtone.sliderPosition = x;
			m_med = (x - m_shadow.sliderPosition) / double(m_highlight.sliderPosition - m_shadow.sliderPosition);

			emit sliderMoved_midtone(m_midtone.sliderPosition);
		}

		if (m_highlight_act) {

			m_highlight.sliderValue = m_highlight.sliderPosition = x;

			if (x <= m_shadow.sliderPosition && x > minimum())
				emit sliderMoved_shadow(m_shadow.sliderValue = m_shadow.sliderPosition = m_highlight.sliderPosition - 1);

			m_midtone.sliderValue = m_midtone.sliderPosition = m_shadow.sliderPosition + (m_highlight.sliderPosition - m_shadow.sliderPosition) * m_med;

			emit sliderMoved_highlight(m_highlight.sliderPosition);
		}

	}

	update();

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
	m_trackbar.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove);
	m_trackbar.rect.translate(0, 2);
	m_trackbar.rect.setHeight(6);
	p.drawRect(m_trackbar.rect);

	opt.subControls = QStyle::SC_SliderHandle;
	p.setPen("#5c5c5c");

	//shadow handle	
	opt.sliderPosition = m_shadow.sliderPosition;
	m_shadow.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	drawHandle(m_shadow.rect, opt, p);

	// 
	//midtone handle
	opt.sliderPosition = m_midtone.sliderPosition;
	m_midtone.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	drawHandle(m_midtone.rect, opt, p);

	//highlight handle
	opt.sliderPosition = m_highlight.sliderPosition;
	m_highlight.rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle);
	drawHandle(m_highlight.rect, opt, p);
}

void HistogramSlider::drawHandle(const QRect& rect, const QStyleOptionSlider& opt, QPainter& p) {

	QRect r = rect;

	int dx = (r.width() - m_handle_width) / 2;
	r.translate((opt.sliderPosition - r.x()) - (dx - (m_handle_width / 2)), 0);
	r.adjust(dx, 4, -dx, 0);

	if (r.height() < m_handle_height) {
		int dy = r.height() - m_handle_height;
		r.adjust(0, 0, 0, -dy);
	}

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

void HistogramSlider::AddStyleSheet() {
	QString str = "QSlider::groove:horizontal{"
		"border: 1px solid #999999;"
		"height: 8px;"
		"background: ;"
		"margin: 0px 0;}";

	/*"QSlider::handle:horizontal{"
	"background: white;"
	"border: 1px solid #5c5c5c;"
	"width: 6px;"
	"margin: -3px 0;"
	"border-top-left-radius: 2px;"
	"border-top-right-radius: 2px;"
	"}";*/

	this->setStyleSheet(str);
}






using HTV = HistogramTransformationView;
HTV::HistogramTransformationView(HistogramTransformation& ht, const QSize& size, QWidget* parent) : m_ht(&ht), HistogramView(size, parent) {

	drawMTFCurve();
	this->show();
}

void HTV::drawScene() {

	this->drawHistogram();
	this->drawMTFCurve();
}

void HTV::clearScene(bool draw_grid) {

	HistogramView::clearScene(draw_grid);
	m_mtf_curve = nullptr;
}

void HTV::resetScene() {

	this->clearHistogram();
	this->drawMTFCurve();
}

void HTV::drawMTFCurve() {

	if (m_mtf_curve != nullptr && !m_mtf_curve->isVisible())
		return;

	for (auto item : scene()->items())
		if (item == m_mtf_curve) {
			scene()->removeItem(item);
		}

	int w = scene()->width();
	int h = scene()->height();

	int start = m_ht->shadow(colorComponent()) * w;// m_histogram_slider->sliderPosition_shadow();
	int end = m_ht->highlight(colorComponent()) * w;

	double dx = 1.0 / w;

	QPolygonF line(end - start + 2);

	double s = m_ht->shadow(colorComponent());

	for (int i = 0; i < line.size(); ++i, ++start, s += dx)
		line[i] = QPointF(start, (1 - m_ht->transformPixel(colorComponent(), s)) * h);

	QPainterPath path;
	path.addPolygon(line);
	m_mtf_curve = scene()->addPath(path, m_pens[int(colorComponent())]);
}





using HTD = HistogramTransformationDialog;

HTD::HistogramTransformationDialog(QWidget* parent) : ProcessDialog("HistogramTransformation", QSize(422, 385), FastStack::recast(parent)->workspace()) {

	setTimerInterval(250);
	setPreviewMethod(this, &HTD::applytoPreview);
	connectToolbar(this, &HTD::apply, &HTD::showPreview, &HTD::resetDialog);

	m_htv = new HistogramTransformationView(m_ht, { 402,200 }, drawArea());
	m_htv->move(10, 10);

	addHistogramSlider();
	addLineEdits();
	addChannelSelection();
	addImageSelectionCombo();
	addMTFPushButton();
	addResolutionCombo();

	this->show();
}

void HTD::onWindowOpen() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->subWindowList().last()->widget())->name();
	m_image_sel->addItem(str);
}

void HTD::onWindowClose() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->name();
	int index = m_image_sel->findText(str);

	if (index == m_image_sel->currentIndex())
		m_htv->resetScene();

	m_image_sel->removeItem(index);
	m_image_sel->setCurrentIndex(0);
}


void HTD::onActivation_imageSelection(int index) {

	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = imageRecast(sw->widget());
		if (m_image_sel->itemText(index) == ptr->name()) {
			switch (ptr->type()) {
			case ImageType::UBYTE:
				m_htv->constructHistogram(ptr->source());
				return m_htv->drawScene();
			case ImageType::USHORT:
				m_htv->constructHistogram(imageRecast<uint16_t>(ptr)->source());
				return m_htv->drawScene();
			case ImageType::FLOAT:
				m_htv->constructHistogram(imageRecast<float>(ptr)->source());
				return m_htv->drawScene();
			}
		}
	}

	m_htv->resetScene();
}

void HTD::onActivation_resolution(int index) {

	auto hr = Histogram::Resolution(m_hist_res_combo->itemData(index).toInt());

	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = imageRecast(sw->widget());
		if (m_image_sel->currentText() == ptr->name()) {
			switch (ptr->type()) {
			case ImageType::UBYTE:
				m_htv->constructHistogram(ptr->source(), hr);
				break;
			case ImageType::USHORT:
				m_htv->constructHistogram(imageRecast<uint16_t>(ptr)->source(), hr);
				break;
			case ImageType::FLOAT:
				m_htv->constructHistogram(imageRecast<float>(ptr)->source(), hr);
				break;
			}
		}
	}
	m_htv->drawScene();
}


void HTD::onClick(int id) {

	m_current_comp = ColorComponent(id);
	m_htv->setColorComponent(m_current_comp);
	m_histogram_slider->setHandleColor(m_colors[id]);

	float shadow = m_ht.shadow(m_current_comp);
	float midtone = m_ht.midtone(m_current_comp);
	float highlight = m_ht.highlight(m_current_comp);

	m_shadow_le->setText(QString::number(shadow, 'f', 8));
	m_midtone_le->setText(QString::number(midtone, 'f', 8));
	m_highlight_le->setText(QString::number(highlight, 'f', 8));

	m_histogram_slider->setSliderPosition_Shadow(shadow * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_Midtone((midtone + shadow) * (highlight + shadow) * m_histogram_slider->maximum());
	m_histogram_slider->setSliderPosition_Highlight(highlight * m_histogram_slider->maximum());

	m_histogram_slider->setMedian(midtone);

	m_htv->drawHistogram();
	m_htv->drawMTFCurve();
}


void HTD::sliderMoved_shadow(int pos) {
	float s = float(pos) / m_histogram_slider->maximum();

	m_shadow_le->setValue(s);
	m_ht.setShadow(m_current_comp, s);

	m_htv->drawMTFCurve();

	startTimer();
}

void HTD::sliderMoved_midtone(int pos) {
	int s = m_histogram_slider->sliderPosition_shadow();
	int h = m_histogram_slider->sliderPosition_highlight();
	float m = float(pos - s) / (h - s);

	m_midtone_le->setValue(m);
	m_ht.setMidtone(m_current_comp, m);

	m_htv->drawMTFCurve();

	startTimer();
}

void HTD::sliderMoved_highlight(int pos) {
	float h = double(pos) / m_histogram_slider->maximum();

	m_highlight_le->setValue(h);
	m_ht.setHighlight(m_current_comp, h);

	m_htv->drawMTFCurve();

	startTimer();
}

void HTD::editingFinished_smh() {
	float shadow = m_shadow_le->text().toFloat();
	float midtone = m_midtone_le->text().toFloat();
	float highlight = m_highlight_le->text().toFloat();

	int s = shadow * m_histogram_slider->maximum();
	int h = m_highlight_le->text().toFloat() * m_histogram_slider->maximum();

	if (s > h)
		s = h;
	if (h < s)
		h = s;

	m_histogram_slider->setSliderPosition_Shadow(s);
	m_ht.setShadow(m_current_comp, shadow);

	m_histogram_slider->setSliderPosition_Midtone(midtone * (h - s) + s);
	m_ht.setMidtone(m_current_comp, midtone);

	m_histogram_slider->setSliderPosition_Highlight(h);
	m_ht.setHighlight(m_current_comp, highlight);

	m_htv->drawMTFCurve();

	applytoPreview();
}


void HTD::addHistogramSlider() {

	m_histogram_slider = new HistogramSlider(Qt::Horizontal, drawArea());

	m_histogram_slider->setFixedWidth(412);
	m_histogram_slider->setRange(0, 400);
	m_histogram_slider->move(5, 208);


	connect(m_histogram_slider, &HistogramSlider::sliderMoved_shadow, this, &HTD::sliderMoved_shadow);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_midtone, this, &HTD::sliderMoved_midtone);
	connect(m_histogram_slider, &HistogramSlider::sliderMoved_highlight, this, &HTD::sliderMoved_highlight);
}

void HTD::addChannelSelection() {

	using CC = ColorComponent;

	m_component_bg = new QButtonGroup(this);

	ComponentPushButton* pb = new ComponentPushButton("Red", drawArea());
	m_component_bg->addButton(pb, int(CC::red));
	pb->move(250, 235);

	pb = new ComponentPushButton("Green", drawArea());
	m_component_bg->addButton(pb, int(CC::green));
	pb->move(300, 235);

	pb = new ComponentPushButton("Blue", drawArea());
	m_component_bg->addButton(pb, int(CC::blue));
	pb->move(350, 235);

	pb = new ComponentPushButton("RGB/K", drawArea());
	m_component_bg->addButton(pb, int(CC::rgb_k));
	pb->setChecked(true);
	pb->move(200, 235);

	for (auto b : m_component_bg->buttons())
		b->resize(50, 25);

	connect(m_component_bg, &QButtonGroup::idClicked, this, &HistogramTransformationDialog::onClick);
}

void HTD::addLineEdits() {

	m_shadow_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_shadow_le->move(85, 235);
	m_shadow_le->addLabel(new QLabel("Shadow:   ", drawArea()));

	m_midtone_le = new DoubleLineEdit(0.5, new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_midtone_le->move(85, 270);
	m_midtone_le->addLabel(new QLabel("Midtone:   ", drawArea()));

	m_highlight_le = new DoubleLineEdit(1.0, new DoubleValidator(0.0, 1.0, 6), drawArea());
	m_highlight_le->move(85, 305);
	m_highlight_le->addLabel(new QLabel("Highlight:   ", drawArea()));

	connect(m_shadow_le, &QLineEdit::editingFinished, this, &HTD::editingFinished_smh);
	connect(m_midtone_le, &QLineEdit::editingFinished, this, &HTD::editingFinished_smh);
	connect(m_highlight_le, &QLineEdit::editingFinished, this, &HTD::editingFinished_smh);

}

void HTD::addImageSelectionCombo() {

	m_image_sel = new ComboBox(drawArea());
	m_image_sel->addItem("No Image Selected");
	m_image_sel->move(200, 305);
	m_image_sel->setFixedWidth(200);
	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(imageRecast<>(sw->widget())->name());

	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::imageWindowClosed, this, &HTD::onWindowClose);
	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::imageWindowCreated, this, &HTD::onWindowOpen);
	connect(m_image_sel, &QComboBox::activated, this, &HTD::onActivation_imageSelection);
}

void HTD::addMTFPushButton() {

	m_mtf_curve_pb = new CheckablePushButton("MTF Curve", drawArea());
	m_mtf_curve_pb->setChecked(true);
	m_mtf_curve_pb->move(260, 270);
	connect(m_mtf_curve_pb, &QPushButton::clicked, this, [this](bool v) { m_htv->mtfCurve()->setVisible(v); m_htv->drawMTFCurve(); });
}

void HTD::addResolutionCombo() {

	m_hist_res_combo = new ComboBox(drawArea());
	m_hist_res_combo->addItems({ "8-bit", "10-bit", "12-bit", "14-bit", "16-bit" });


	for (int i = 0, s = 8; i < m_hist_res_combo->count(); ++i, s += 2)
		m_hist_res_combo->setItemData(i, 1 << s);

	m_hist_res_combo->setCurrentIndex(4);
	m_hist_res_combo->move(253, 345);
	connect(m_hist_res_combo, &QComboBox::activated, this, &HTD::onActivation_resolution);

	addLabel(m_hist_res_combo, new QLabel("Histogram Resolution:", drawArea()));
}


void HTD::resetDialog() {

	m_ht = HistogramTransformation();

	m_shadow_le->setValue(m_ht.shadow(ColorComponent::rgb_k));
	m_midtone_le->setValue(m_ht.midtone(ColorComponent::rgb_k));
	m_highlight_le->setValue(m_ht.highlight(ColorComponent::rgb_k));
	m_histogram_slider->resetSliderPositions();
	m_mtf_curve_pb->clicked(true);

	applytoPreview();
}

void HTD::showPreview() {

	ProcessDialog::showPreview();
	applytoPreview();
}

void HTD::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	enableSiblings(false);

	ImageWindow8* iwptr = imageRecast(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource(m_ht, &HistogramTransformation::apply);
		break;
	}
	}

	onActivation_imageSelection(m_image_sel->currentIndex());
	enableSiblings(true);

	applytoPreview();
}

void HTD::applytoPreview() {

	if (!isPreviewValid())
		return;

	PreviewWindow8* iwptr = previewRecast(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->updatePreview(m_ht, &HistogramTransformation::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = previewRecast<uint16_t>(iwptr);
		return iw16->updatePreview(m_ht, &HistogramTransformation::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = previewRecast<float>(iwptr);
		return iw32->updatePreview(m_ht, &HistogramTransformation::apply);
	}
	}
}