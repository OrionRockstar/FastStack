#include "pch.h"
#include "CurvesTransformation.h"
#include "FastStack.h"

void CurveTransform::setInterpolation(ColorComponent comp, Curve::Type type) {
	rCurve(comp).setInterpolation(type);
}

void CurveTransform::setDataPoints(ColorComponent comp, std::vector<QPointF> points) {
	rCurve(comp).setDataPoints(points);
}

void CurveTransform::computeCoefficients(ColorComponent comp) {
	rCurve(comp).computeCoeffecients();
}

void CurveTransform::interpolateValues(ColorComponent comp, std::vector<double>& values) {

	if (m_comp_curves[int(comp)].isIdentity())
		return;

	for (auto& val : values) {
		val = m_comp_curves[int(comp)].interpolate(val);
	}
}

template<typename T>
void CurveTransform::Apply(Image<T>& img) {

	using CC = ColorComponent;

	auto applyChannel = [&, this](Curve* curve, int ch) {
		for (auto& p : ImageChannel(img, ch))
			p = p = Clip(curve->interpolate(p));
	};

	Curve RGB_K = rCurve(CC::rgb_k);

	if (!RGB_K.isIdentity()) {

#pragma omp parallel for num_threads(img.channels())
		for (int ch = 0; ch < img.channels(); ++ch)
			applyChannel(&RGB_K, ch);
	}



	if (img.channels() == 1)
		return;

	Curve red = rCurve(CC::red);
	Curve green = rCurve(CC::green);
	Curve blue = rCurve(CC::blue);

	if (!red.isIdentity() || !green.isIdentity() || !blue.isIdentity()) {

		std::thread t0 = std::thread(applyChannel, &red, 0);
		std::thread t1 = std::thread(applyChannel, &green, 1);
		std::thread t2 = std::thread(applyChannel, &blue, 2);

		t0.join();
		t1.join();
		t2.join();
	}

	Curve Lightness = rCurve(CC::Lightness);
	Curve a = rCurve(CC::a);
	Curve b = rCurve(CC::b);

	if (!Lightness.isIdentity() || !a.isIdentity() || !b.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double L, _a, _b;
				ColorSpace::RGBtoCIELab(rgb, L, _a, _b);
				rgb = ColorSpace::CIELabtoRGB(Lightness.interpolate(L), a.interpolate(_a), b.interpolate(_b));
				
				img.setColor<>(x, y, rgb);
			}
		}
	}



	Curve c = rCurve(CC::c);

	if (!c.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double L, _c, _h;
				ColorSpace::RGBtoCIELch(rgb, L, _c, _h);
				rgb = ColorSpace::CIELchtoRGB(L, c.interpolate(_c), _h);

				img.setColor<>(x, y, rgb);
			}
		}
	}



	Curve Hue = rCurve(CC::hue);
	Curve Saturation = rCurve(CC::saturation);

	if (!Hue.isIdentity() || !Saturation.isIdentity()) {
#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double H, S, V, L;
				ColorSpace::RGBtoHSVL(rgb, H, S, V, L);
				rgb = ColorSpace::HSVLtoRGB(Hue.interpolate(H), Saturation.interpolate(S), V, L);

				img.setColor(x, y, rgb);
			}
		}
	}
}
template void CurveTransform::Apply(Image8&);
template void CurveTransform::Apply(Image16&);
template void CurveTransform::Apply(Image32&);





CurveTransformScene::CurveTransformScene(CurveTransform* ctp, QRect rect, QWidget* parent) : CurveScene( rect, parent) {
	m_ctp = ctp;
}

void CurveTransformScene::onCurveTypeChange(int id) {
	CurveItem* curve = m_curve_items[int(m_comp)];

	(*m_ctp).setInterpolation(m_comp, Curve::Type(id));
	curve->setCurveType(Curve::Type(id));
	renderCurve(curve);
}

void CurveTransformScene::onUpdatePoint(QPointF point) {
	CurveScene::onUpdatePoint(point);
	renderCurve(m_curve_items[int(m_comp)]);
}

void CurveTransformScene::renderCurve(CurveItem* curve) {

	(*m_ctp).setDataPoints(m_comp, curve->curvePoints_norm());

	(*m_ctp).computeCoefficients(m_comp);

	for (int i = 0; i < m_values.size(); ++i)
		m_values[i] = i / width();


	(*m_ctp).interpolateValues(m_comp, m_values);

	curve->setCurvePoints(m_values);
}

void CurveTransformScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mousePressEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}

void CurveTransformScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mouseMoveEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}

void CurveTransformScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	CurveScene::mouseReleaseEvent(event);
	renderCurve(m_curve_items[int(m_comp)]);
}






CurveGraphicsView::CurveGraphicsView(QGraphicsScene* scene, QWidget* parent) : QGraphicsView(scene, parent) {

	m_cts = static_cast<CurveTransformScene*>(scene);

	this->setRenderHints(QPainter::Antialiasing);
	this->scale(1, -1);
	this->setGeometry(9, 9, 382, 382);
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	loadAxis();
}

void CurveGraphicsView::loadAxis() {

	m_color_axis[0].x_axis.load("./Icons//horizontal_red.png");
	m_color_axis[0].y_axis.load("./Icons//vertical_red.png");

	m_color_axis[1].x_axis.load("./Icons//horizontal_green.png");
	m_color_axis[1].y_axis.load("./Icons//vertical_green.png");

	m_color_axis[2].x_axis.load("./Icons//horizontal_blue.png");
	m_color_axis[2].y_axis.load("./Icons//vertical_blue.png");

	m_color_axis[3].x_axis.load("./Icons//horizontal_K_L.png");
	m_color_axis[3].y_axis.load("./Icons//vertical_K_L.png");
	m_color_axis[4].x_axis.load("./Icons//horizontal_K_L.png");
	m_color_axis[4].y_axis.load("./Icons//vertical_K_L.png");

	m_color_axis[5].x_axis.load("./Icons//horizontal_a.png");
	m_color_axis[5].y_axis.load("./Icons//vertical_a.png");

	m_color_axis[6].x_axis.load("./Icons//horizontal_b.png");
	m_color_axis[6].y_axis.load("./Icons//vertical_b.png");

	m_color_axis[7].x_axis.load("./Icons//horizontal_c.png");
	m_color_axis[7].y_axis.load("./Icons//vertical_c.png");

	m_color_axis[8].x_axis.load("./Icons//horizontal_hue.png");
	m_color_axis[8].y_axis.load("./Icons//vertical_hue.png");

	m_color_axis[9].x_axis.load("./Icons//horizontal_saturation.png");
	m_color_axis[9].y_axis.load("./Icons//vertical_saturation.png");
}

void CurveGraphicsView::drawBackground(QPainter* painter, const QRectF& rect) {

	painter->fillRect(rect, QColor(19,19,19));

	painter->drawImage( 0, 0, m_color_axis[int(m_cts->currentComponent())].x_axis);// , QRect(150, 0, 50, 8));
	painter->drawImage(0, 0, m_color_axis[int(m_cts->currentComponent())].y_axis);
}




CurveTransformDialog::CurveTransformDialog(QWidget* parent): ProcessDialog("CurveTransform", QSize(400,535), FastStack::recast(parent)->workspace()) {

	using CTD = CurveTransformDialog;

	setTimer(250, this, &CTD::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &CTD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &CTD::Apply, &CTD::showPreview, &CTD::resetDialog);


	m_cs = new CurveTransformScene(&m_ct, QRect(0, 0, 380, 380), this);
	connect(m_cs, &CurveScene::itemARC, this, &CurveTransformDialog::onItemARC);

	m_gv = new CurveGraphicsView(m_cs, this);


	addComponentSelection();
	addPointLineEdits();
	addPointSelection();


	m_curve_type_bg = new CurveTypeButtonGroup(this);
	m_curve_type_bg->move(148, 470);
	connect(m_curve_type_bg, &CurveTypeButtonGroup::idClicked, m_cs, &CurveTransformScene::onCurveTypeChange);

	this->show();
}

void CurveTransformDialog::onItemARC() {
	auto curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));

	int current = curve->itemIndex(curve->currentItem()) + 1;
	int total = curve->itemListSize();

	QString c = QString::number(current);
	QString t = QString::number(total);

	if (current < 10)
		c.insert(0, "  ");

	m_current_point->setText(c + " / " + t);	
}

void CurveTransformDialog::onEditingFinished_io() {
	int x = m_input_le->text().toDouble() * m_cs->width();
	int y = m_output_le->text().toDouble() * m_cs->height();
	updatePoint(QPoint(x, y));
}

void CurveTransformDialog::onCurrentPos(QPointF point) {
	m_input_le->setValue(point.x() / m_cs->width());
	m_output_le->setValue(point.y() / m_cs->height());
	startTimer();
}


void CurveTransformDialog::addComponentSelection() {
	using CC = ColorComponent;

	m_component_bg = new QButtonGroup(this);

	QPixmap pm = QPixmap(15, 15);
	pm.fill(QColor(255, 0, 0));

	ComponentPushButton* pb = new ComponentPushButton(QIcon(pm), "R", this);
	m_component_bg->addButton(pb, int(CC::red));

	pm.fill(QColor(0, 255, 0));
	pb = new ComponentPushButton(QIcon(pm), "G", this);
	m_component_bg->addButton(pb, int(CC::green));


	pm.fill(QColor(0, 0, 255));
	pb = new ComponentPushButton(QIcon(pm), "B", this);
	m_component_bg->addButton(pb, int(CC::blue));


	pm = QPixmap("C:\\Users\\Zack\\Desktop\\rgb.png");
	pb = new ComponentPushButton(QIcon(pm),"RGB/K", this);
	m_component_bg->addButton(pb, int(CC::rgb_k));
	pb->setChecked(true);
	pb->resize(65,25);

	pb = new ComponentPushButton("L", this);
	m_component_bg->addButton(pb, int(CC::Lightness));

	pb = new ComponentPushButton("a", this);
	m_component_bg->addButton(pb, int(CC::a));

	pb = new ComponentPushButton("b", this);
	m_component_bg->addButton(pb, int(CC::b));

	pb = new ComponentPushButton("c", this);
	m_component_bg->addButton(pb, int(CC::c));

	pb = new ComponentPushButton("H", this);
	m_component_bg->addButton(pb, int(CC::hue));

	pb = new ComponentPushButton("S", this);
	m_component_bg->addButton(pb, int(CC::saturation));

	int x = 10;
	for (auto button : m_component_bg->buttons()) {

		if (m_component_bg->id(button) != int(CC::rgb_k))
			button->resize({35,25});

		button->move(x, 395);
		x += button->width();
	}

	connect(m_component_bg, &QButtonGroup::idClicked, m_cs, &CurveScene::onChannelChange);

}

void CurveTransformDialog::addPointLineEdits() {

	m_input_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 6, this), this);
	m_input_le->setFixedWidth(75);
	m_input_le->move(65, 430);
	m_input_le->addLabel(new QLabel("Input:   ", this));
	m_input_le->setDisabled(true);


	m_output_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 1.0, 6, this), this);
	m_output_le->setFixedWidth(75);
	m_output_le->move(210, 430);
	m_output_le->addLabel(new QLabel("Output:   ", this));

	auto edited = [this]() {
		int x = m_input_le->value() * m_cs->width();
		int y = m_output_le->value() * m_cs->height();
		updatePoint(QPoint(x, y));
	};

	connect(m_input_le, &DoubleLineEdit::editingFinished, this, edited);
	connect(m_output_le, &DoubleLineEdit::editingFinished, this, edited);

	connect(this, &CurveTransformDialog::updatePoint, m_cs, &CurveTransformScene::onUpdatePoint);

	connect(m_cs, &CurveScene::currentPos, this, &CurveTransformDialog::onCurrentPos);
	connect(m_cs, &CurveScene::endItem, this, [this](bool v) { m_input_le->setDisabled(v); });

}

void CurveTransformDialog::addPointSelection() {

	QPushButton* pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekForward), "", this);
	pb->setAutoDefault(false);
	pb->move(325, 430);

	auto next = [this]() {
		CurveItem* curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));
		const QGraphicsItem* item = curve->nextItem();
		QPointF p = CurveItem::itemCenter(item);

		m_input_le->setValue(p.x() / m_cs->width());
		m_output_le->setValue(p.y() / m_cs->height());

		if (curve->isEnd(item))
			m_input_le->setDisabled(true);
		else
			m_input_le->setEnabled(true);

		onItemARC();
	};

	connect(pb, &QPushButton::pressed, this, next);

	pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekBackward), "", this);
	pb->setAutoDefault(false);
	pb->move(295, 430);

	auto previous = [this]() {
		CurveItem* curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));
		const QGraphicsItem* item = curve->previousItem();
		QPointF p = CurveItem::itemCenter(item);

		m_input_le->setValue(p.x() / m_cs->width());
		m_output_le->setValue(p.y() / m_cs->height());

		if (curve->isEnd(item))
			m_input_le->setDisabled(true);
		else
			m_input_le->setEnabled(true);

		onItemARC();
	};

	connect(pb, &QPushButton::pressed, this, previous);

	m_current_point = new QLabel("  1 / 2", this);
	m_current_point->move(305, 460);
	m_current_point->resize(m_current_point->fontMetrics().horizontalAdvance("00 / 00"), m_current_point->size().height());
}

void CurveTransformDialog::resetDialog() {
	m_ct = CurveTransform();

	m_input_le->setValue(0.0);
	m_output_le->setValue(0.0);
	m_input_le->setDisabled(true);
	m_curve_type_bg->button(0)->setChecked(true);
	m_cs->resetScene();
	onItemARC();

	ApplytoPreview();
}

void CurveTransformDialog::showPreview() { 
	ProcessDialog::showPreview();
	ApplytoPreview();
}

void CurveTransformDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource(m_ct, &CurveTransform::Apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->applyToSource(m_ct, &CurveTransform::Apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->applyToSource(m_ct, &CurveTransform::Apply);
		break;
	}
	}

	ApplytoPreview();
}

void CurveTransformDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<PreviewWindow8*>(m_preview);

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		auto iw8 = iwptr;
		return iw8->updatePreview(m_ct, &CurveTransform::Apply);
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr);
		return iw16->updatePreview(m_ct, &CurveTransform::Apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr);
		return iw32->updatePreview(m_ct, &CurveTransform::Apply);
	}
	}
}