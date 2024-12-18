#include "pch.h"
#include "ColorSaturation.h"
#include "RGBColorSpace.h"
#include "FastStack.h"

static void shiftHue(double& H, double amount) {

	H += amount;

	if (H >= 1)
		H -= 1;

	else if (H < 0)
		H += 1;
}

static double scalingFactor(double k) {
	k += (k < 0) ? -1 : +1;

	if (k < 0)
		k = 1 / (-k);

	return k;
}

template<typename T>
void ColorSaturation::Apply(Image<T>& src) {

    if (!src.exists() || src.channels() == 1)
        return;

#pragma omp parallel for
    for (int y = 0; y < src.rows(); ++y) {
		for (int x = 0; x < src.cols(); ++x) {

			auto rgb = src.color<double>(x, y);

			double H, S, V, L;
			ColorSpace::RGBtoHSVL(rgb, H, S, V, L);

			shiftHue(H, m_hue_shift);
			double k = scalingFactor(m_scale * m_saturation_curve.interpolate(H));
			shiftHue(H, -m_hue_shift);

			src.setColor<>(x, y, ColorSpace::HSVLtoRGB(H, Clip(S * k), V, L));
		}
    }
}








ColorSaturationScene::ColorSaturationScene(ColorSaturation* cs, QRect rect, QWidget* parent) : CurveScene(rect, parent) {
	
	m_cs = cs;

	this->setSceneRect(rect);
	drawGrid();

	this->backgroundBrush().color().green();

	m_comp = ColorComponent::saturation;

	auto ci = m_curve_items[int(m_comp)] = new CurveItem(m_color[int(m_comp)], m_color[int(m_comp)], this, 0.5, 0.5);
	ci->setLinkEnds(true);
	ci->setCurveVisibility(true);
	m_values.resize(rect.width());
	m_current = ci->endItem_Left();
}

void ColorSaturationScene::normalizePoint(QPointF& p) {
	p.rx() = p.x() / width();
	p.ry() = 2 * (p.y() / height() - 0.5);
}


void ColorSaturationScene::resetScene() {

	clear();
	drawGrid();

	auto ci = m_curve_items[int(m_comp)] = new CurveItem(m_color[int(m_comp)], m_color[int(m_comp)], this, 0.5, 0.5);
	ci->setLinkEnds(true);
	ci->setCurveVisibility(true);
	m_current = ci->endItem_Left();
}

void ColorSaturationScene::onCurveTypeChange(int id) {
	CurveItem* curve = m_curve_items[int(m_comp)];

	(*m_cs).setInterpolation(Curve::Type(id));
	curve->setCurveType(Curve::Type(id));
	renderCurve(curve);
}

void ColorSaturationScene::renderCurve(CurveItem* curve) {

	Curve& sc = m_cs->saturationCurve();

	std::vector<QPointF> pv = curve->curvePoints_norm();
	for (auto& p : pv)
		p.ry() = 2 * (p.y() - 0.5);

	sc.setDataPoints(pv);
	sc.computeCoeffecients();

	for (int i = 0; i < m_values.size(); ++i)
		m_values[i] = i / width();

	for (auto& v : m_values)
		v = (sc.interpolate(v) / 2) + 0.5;

	curve->setCurvePoints(m_values);
}

void ColorSaturationScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mousePressEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}

void ColorSaturationScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mouseMoveEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}

void ColorSaturationScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mouseReleaseEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}





static double ComputePolynomial(const Matrix& coefficients, int x, int poly_degree = 3) {

	std::vector<double> xv(poly_degree + 1);
	xv[0] = 1;

	for (int i = 1, j = 0; i <= poly_degree; ++i, ++j) {
		xv[i] = xv[j] * x;
	}

	double sum = 0;

	for (int i = 0; i <= poly_degree; ++i)
		sum += xv[i] * coefficients[i];
	
	return sum;
}

RGBBar::RGBBar(const QSize& size, QWidget* parent) : QLabel(parent) {

	m_src = QImage(size.width() * 2, size.height(), QImage::Format::Format_RGBA8888);
	m_dst = QImage(size, QImage::Format::Format_RGBA8888);

	m_coef = computeCoeficients(size.width());

	fillSource();
	updateBar_shift(0);

	this->setAttribute(Qt::WA_TransparentForMouseEvents);
	this->show();
}

Matrix RGBBar::computeCoeficients(uint32_t width_size) {

	int poly_d = 3;
	int p_1 = poly_d + 1;

	int n_pts = 9;
	int _1_n = width_size / n_pts;

	Matrix var(p_1, p_1);
	Matrix r_sum(p_1);

	for (int j = 0, h = 0; j < var.rows(); ++j, ++h) {
		for (int i = 0, f = 0; i < var.cols(); ++i, ++f) {

			if (i == 0 && j == 0)
				var(j, i) = n_pts;

			else {
				double sum = 0;
				for (int N = 0; N < n_pts; ++N)
					sum += pow(N * _1_n, f + h);
				var(j, i) = sum;
			}
		}

		double s = 0;
		double v = pow(_1_n, h);

		for (int n = 0; n < p_1; ++n)
			s += v;

		r_sum[j] = s;
	}
	return Matrix::leastSquares(var, r_sum);
}

void RGBBar::fillSource() {

	int r_off = (1 * m_dst.width()) / 9;
	int g_off = (2 * m_dst.width()) / 9;
	int b_off = (5 * m_dst.width()) / 9;

	for (int y = 0; y < m_dst.height(); ++y) {

		for (int x = 0; x < m_src.width(); ++x) {

			int s = (x >= m_dst.width()) ? x - m_dst.width() : x;

			m_src.scanLine(y)[4 * x + 0] = 255 * Clip(ComputePolynomial(m_coef, s + r_off));
			m_src.scanLine(y)[4 * x + 1] = 255 * Clip(ComputePolynomial(m_coef, s - g_off));
			m_src.scanLine(y)[4 * x + 2] = 255 * Clip(ComputePolynomial(m_coef, s - b_off));
			m_src.scanLine(y)[4 * x + 3] = m_alpha;
		}
	}
}

void RGBBar::updateBar_shift(int shift) {

	int size = 4 * m_dst.width();
	for (int y = 0; y < m_dst.height(); ++y) 
		memcpy(m_dst.scanLine(y), &m_src.scanLine(y)[4 * (m_dst.width() - shift)], size);

	int min = Min(m_dst.width(), m_dst.height());
	int ex = min;

	for (int y = 0; y < min; ++y, --ex)
		for (int x = 0; x < ex; ++x) 
			m_dst.scanLine(y)[4 * x + 3] = 0;

	this->setPixmap(QPixmap::fromImage(m_dst));
}



ColorGraphicsView::ColorGraphicsView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {

	m_css = static_cast<ColorSaturationScene*>(scene);

	this->setRenderHints(QPainter::Antialiasing);
	this->scale(1, -1);
	this->setGeometry(9, 9, 482, 202);
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_rgb = new RGBBar(QSize(m_css->width(), 8), this);
	m_rgb->move(1, 193);

	m_sat = GradientImage(QSize(8, m_css->height()), Qt::Vertical);
	m_sat.setGradient({ {0,255,255,255}, XRGB(m_css->height(),178,102,255) }, 0.9);
	m_sat.cutCorners(GradientImage::top_right);
}

void ColorGraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {
	painter->fillRect(rect, QColor(19,19,19));
	painter->drawImage(0, 0, m_sat.image());
}





ColorSaturationDialog::ColorSaturationDialog(QWidget* parent) : ProcessDialog("Color Saturation", QSize(500,355), FastStack::recast(parent)->workspace()) {

	this->setStyleSheet("QDialog::title {background-color:blue;}");

	setTimer(250, this, &ColorSaturationDialog::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &ColorSaturationDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &ColorSaturationDialog::Apply, &ColorSaturationDialog::showPreview, &ColorSaturationDialog::resetDialog);

	m_css = new ColorSaturationScene(&m_cs, QRect(0, 0, 480, 200), this);
	connect(m_css, &CurveScene::itemARC, this, &ColorSaturationDialog::onItemARC);

	m_gv = new ColorGraphicsView(m_css, this);

	//move below point label
	m_scale_sb = new SpinBox(this);
	m_scale_sb->move(285, 255);
	m_scale_sb->addLabel(new QLabel("Scale:   ", this));
	m_scale_sb->setRange(1, 8);
	m_scale_sb->setValue(1);
	connect(m_scale_sb, &QSpinBox::valueChanged, this, &ColorSaturationDialog::onValueChanged_scale);

	m_curve_type_bg = new CurveTypeButtonGroup(this);
	m_curve_type_bg->move(350, 235);
	connect(m_curve_type_bg, &CurveTypeButtonGroup::idClicked, m_css, &ColorSaturationScene::onCurveTypeChange);
	
	addHueSaturation();
	addHueShift();
	addPointSelection();

    this->show();
}


void ColorSaturationDialog::onItemARC() {

	auto curve = m_css->curveItem(ColorComponent::saturation);

	int current = curve->itemIndex(curve->currentItem()) + 1;
	int total = curve->itemListSize();

	QString c = QString::number(current);
	QString t = QString::number(total);

	if (current < 10)
		c.insert(0, "  ");

	m_current_point->setText(c + " / " + t);
}

void ColorSaturationDialog::onCurrentPos(QPointF point) {
	m_css->normalizePoint(point);
	m_hue_le->setValue(point.x());
	m_saturation_le->setValue(m_scale_sb->value() * point.y());
	
	startTimer();
}

void ColorSaturationDialog::onValueChanged_scale(int value) {
	m_cs.setScale(m_scale_sb->value());
	double v = (m_saturation_le->value() * m_scale_sb->value()) / m_old_value;
	m_saturation_le->setValue(v);
	m_old_value = m_scale_sb->value();
	startTimer();
}

void ColorSaturationDialog::addHueSaturation() {

	m_hue_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 6, this), this);
	m_hue_le->setFixedWidth(75);
	m_hue_le->move(95, 220);
	m_hue_le->addLabel(new QLabel("Hue:   ", this));
	m_hue_le->setDisabled(true);

	//double r = 1.0 * 8;
	m_saturation_le = new DoubleLineEdit(0.0, new DoubleValidator( -8.0, 8.0, 6, this), this);
	m_saturation_le->setFixedWidth(75);
	m_saturation_le->move(95, 255);
	m_saturation_le->addLabel(new QLabel("Saturation:   ", this));

	auto current = [this](QPointF p) {
		m_css->normalizePoint(p);
		m_hue_le->setValue(p.x());
		m_saturation_le->setValue(m_scale_sb->value() * p.y());

		startTimer();
	};
	connect(m_css, &CurveScene::currentPos, this, current);
	connect(m_css, &CurveScene::endItem, this, [this](bool v) { m_hue_le->setDisabled(v); });

}

void ColorSaturationDialog::addHueShift() {

	m_hue_shift_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 4), this);
	m_hue_shift_le->setValue(0.0);
	m_hue_shift_le->setFixedWidth(75);
	m_hue_shift_le->move(95, 290);
	m_hue_shift_le->addLabel(new QLabel("Hue Shift:   ", this));

	m_hue_shift_slider = new Slider(Qt::Horizontal, this);
	m_hue_shift_slider->setFixedWidth(280);
	m_hue_shift_slider->setRange(0, 480);
	m_hue_shift_le->addSlider(m_hue_shift_slider);

	auto action = [this](int) {
		int pos = m_hue_shift_slider->sliderPosition();
		m_gv->shiftRGB(pos);
		float hs = float(pos) / m_hue_shift_slider->maximum();
		m_hue_shift_le->setValue(hs);
		m_cs.setHueShift(hs);
		startTimer();
	};

	auto edited = [this]() {
		float hue_shift = m_hue_shift_le->valuef();
		m_cs.setHueShift(hue_shift);
		m_hue_shift_slider->setValue(hue_shift * m_hue_shift_slider->maximum());
		m_gv->shiftRGB(m_hue_shift_slider->sliderPosition());
		ApplytoPreview();
	};

	connect(m_hue_shift_slider, &QSlider::actionTriggered, this, action);
	connect(m_hue_shift_le, &DoubleLineEdit::editingFinished, this, edited);
}


void ColorSaturationDialog::addPointSelection() {

	QPushButton* pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekForward), "", this);
	pb->setAutoDefault(false);
	pb->move(185, 257);

	auto next = [this]() {
		CurveItem* curve = m_css->curveItem(ColorComponent::saturation);
		const QGraphicsItem* item = curve->nextItem();
		QPointF p = CurveItem::itemCenter(item);

		m_hue_le->setValue(p.x() / m_css->width());
		m_saturation_le->setValue(p.y() / m_css->height());

		if (curve->isEnd(item))
			m_hue_le->setDisabled(true);
		else
			m_hue_le->setEnabled(true);

		onItemARC();
	};

	connect(pb, &QPushButton::pressed, this, next);

	pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekBackward), "", this);
	pb->setAutoDefault(false);
	pb->move(185, 222);

	auto previous = [this]() {
		CurveItem* curve = m_css->curveItem(ColorComponent::saturation);
		const QGraphicsItem* item = curve->previousItem();
		QPointF p = CurveItem::itemCenter(item);

		m_hue_le->setValue(p.x() / m_css->width());
		m_saturation_le->setValue(p.y() / m_css->height());

		if (curve->isEnd(item))
			m_hue_le->setDisabled(true);
		else
			m_hue_le->setEnabled(true);

		onItemARC();
	};
	connect(pb, &QPushButton::pressed, this, previous);

	m_current_point = new QLabel("  1 / 2", this);
	m_current_point->move(230, 220);
	m_current_point->resize(m_current_point->fontMetrics().horizontalAdvance("00 / 00"), m_current_point->size().height());
}

void ColorSaturationDialog::resetDialog() {
	m_cs = ColorSaturation();

	m_scale_sb->setValue(1);
	m_hue_le->setValue(0.0);
	m_saturation_le->setValue(0.0);
	m_hue_le->setDisabled(true);
	m_hue_shift_slider->setSliderPosition(0);
	m_hue_shift_le->setValue(0.0);
	m_gv->shiftRGB(0);

	m_curve_type_bg->button(0)->setChecked(true);

	m_css->resetScene();
	onItemARC();

	ApplytoPreview();
}

void ColorSaturationDialog::showPreview() {
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void ColorSaturationDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_cs, &ColorSaturation::Apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->applyToSource(m_cs, &ColorSaturation::Apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->applyToSource(m_cs, &ColorSaturation::Apply);
		break;
	}
	}

	ApplytoPreview();
}

void ColorSaturationDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<PreviewWindow8*>(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_cs, &ColorSaturation::Apply);
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr);
		return iw16->updatePreview(m_cs, &ColorSaturation::Apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr);
		return iw32->updatePreview(m_cs, &ColorSaturation::Apply);
	}
	}
}