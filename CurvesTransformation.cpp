#include "pch.h"
#include "CurvesTransformation.h"
#include "FastStack.h"

void CurveTransform::SetInterpolationMethod(ColorComponent comp, CurveType type) {
	rCurve(comp).SetInterpolationCurve(type);
}

void CurveTransform::SetDataPoints(ColorComponent comp, std::vector<QPointF> points) {
	rCurve(comp).InsertDataPoints(points);
}

void CurveTransform::ComputeCoefficients(ColorComponent comp) {
	rCurve(comp).SetCoeffecients();
}

void CurveTransform::InterpolateValues(ColorComponent comp, std::vector<double>& values) {

	if (m_comp_curves[int(comp)].IsIdentity())
		return;

	for (auto& val : values) {
		val = m_comp_curves[int(comp)].Interpolate(val);
	}
}



template<typename T>
void CurveTransform::Apply(Image<T>& img) {

	using CC = ColorComponent;

	Curve RGB_K = rCurve(CC::rgb_k);

	if (!RGB_K.IsIdentity()) {
		if (img.Channels() == 1)
			for (T& pixel : img) {
				double val = Pixel<double>::toType(pixel);
				val = Clip(RGB_K.Interpolate(val));
				pixel = Pixel<T>::toType(val);
			}

		else if (img.Channels() == 3)
#pragma omp parallel for num_threads(4)
			for (int el = 0; el < img.PxCount(); ++el) {
				double R, G, B;
				img.getRGB(el, R, G, B);

				R = Clip(RGB_K.Interpolate(R));
				G = Clip(RGB_K.Interpolate(G));
				B = Clip(RGB_K.Interpolate(B));

				img.setRGB(el, R, G, B);
			}

	}

	if (img.Channels() == 1)
		return;





	Curve Red = rCurve(CC::red);
	Curve Green = rCurve(CC::green);
	Curve Blue = rCurve(CC::blue);

	if (!Red.IsIdentity() || !Green.IsIdentity() || !Blue.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.PxCount(); ++el) {
			double R, G, B;
			img.getRGB(el, R, G, B);

			R = Clip(Red.Interpolate(R));
			G = Clip(Green.Interpolate(G));
			B = Clip(Blue.Interpolate(B));

			img.setRGB(el, R, G, B);
		}
	}





	Curve Lightness = rCurve(CC::Lightness);
	Curve a = rCurve(CC::a);
	Curve b = rCurve(CC::b);

	if (!Lightness.IsIdentity() || !a.IsIdentity() || !b.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.PxCount(); ++el) {
			double R, G, B;
			img.getRGB(el, R, G, B);

			double L, _a, _b;
			ColorSpace::RGBtoCIELab(R, G, B, L, _a, _b);
			ColorSpace::CIELabtoRGB(Lightness.Interpolate(L), a.Interpolate(_a), b.Interpolate(_b), R, G, B);

			img.setRGB(el, Clip(R), Clip(G), Clip(B));
		}
	}





	Curve c = rCurve(CC::c);

	if (!c.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.PxCount(); ++el) {
			double R, G, B;
			img.getRGB(el, R, G, B);

			double L, _c, _h;
			ColorSpace::RGBtoCIELch(R, G, B, L, _c, _h);
			ColorSpace::CIELchtoRGB(L, c.Interpolate(_c), _h, R, G, B);

			img.setRGB(el, Clip(R), Clip(G), Clip(B));
		}
	}





	Curve Hue = rCurve(CC::hue);
	Curve Saturation = rCurve(CC::saturation);

	if (!Hue.IsIdentity() || !Saturation.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.PxCount(); ++el) {
			double R, G, B;
			img.getRGB(el, R, G, B);

			double H, S, V, L;
			ColorSpace::RGBtoHSVL(R, G, B, H, S, V, L);
			ColorSpace::HSVLtoRGB(Hue.Interpolate(H), Saturation.Interpolate(S), V, L, R, G, B);

			img.setRGB(el, Clip(R), Clip(G), Clip(B));
		}
	}

}
template void CurveTransform::Apply(Image8&);
template void CurveTransform::Apply(Image16&);
template void CurveTransform::Apply(Image32&);










CurveItem::CurveItem(QPen pen, QBrush brush, QGraphicsScene* scene) : m_pen(pen), m_brush(brush) {
	m_scene = scene;

	m_pen.setWidthF(1.01);

	m_current = m_left = m_scene->addEllipse(0, 0, 8, 8, pen, brush);
	m_left->moveBy(-4, -4);
	m_item_list.append(m_left);
	m_input_pts.append(ItemCenter(m_left));

	m_right = m_scene->addEllipse(380, 380, 8, 8, pen, brush);
	m_right->setOpacity(0.5);
	m_right->moveBy(-4, -4);
	m_item_list.append(m_right);
	m_input_pts.append(ItemCenter(m_right));

	m_curve_pts.resize(scene->width());
	for (int i = 0; i < m_curve_pts.size(); ++i)
		m_curve_pts[i] = QPointF(i, i);

	QPainterPath path;
	path.addPolygon(m_curve_pts);
	m_curve = m_scene->addPath(path, m_pen);
}

QPointF CurveItem::ItemCenter(const QGraphicsItem* item) {
	QRectF rect = item->sceneBoundingRect();

	float x = rect.x() + (rect.width() / 2.0);
	float y = rect.y() + (rect.height() / 2.0);

	return QPointF(x, y);
}

QGraphicsItem* CurveItem::AddEllipse(qreal x, qreal y) {
	auto new_item = m_scene->addEllipse(x, y, 5, 5, m_pen, m_brush);
	new_item->moveBy(-2.5, -2.5);
	for (auto item : m_item_list)
		item->setOpacity(0.5);

	for (auto item : m_item_list) {
		if (item->collidesWithItem(new_item)) {
			m_scene->removeItem(new_item);
			item->setOpacity(1.0);
			return m_current = item;
		}
	}


	new_item->setOpacity(1.0);
	m_input_pts.append(ItemCenter(new_item));
	m_item_list.append(new_item);
	std::sort(m_input_pts.begin(), m_input_pts.end(), [this](QPointF a, QPointF b) {return a.x() < b.x(); });
	std::sort(m_item_list.begin(), m_item_list.end(), [this](QGraphicsItem* a, QGraphicsItem* b) { return ItemCenter(a).x() < ItemCenter(b).x(); });

	return m_current = new_item;
}

bool CurveItem::CollidesWithOtherItem(QGraphicsItem* current) {

	for (auto item : m_item_list) {
		if (item == current)
			continue;
		if (current->collidesWithItem(item))
			return true;
	}

	return false;
}

std::vector<QPointF> CurveItem::GetNormalizedInputs() {
	std::vector<QPointF> p(m_input_pts.size());

	for (int i = 0; i < p.size(); ++i)
		p[i] = QPointF(m_input_pts[i].x() / m_scene->width(), m_input_pts[i].y() / m_scene->height());

	return p;
}

//removes item sets current to previous item(lower x pos)
void CurveItem::RemoveItem(QPointF point) {
	auto other = m_scene->addEllipse(point.x(), point.y(), 1, 1);

	for (int i = 0; i < m_item_list.size(); ++i) {
		auto item = m_item_list[i];

		if (item != m_left) {
			if (item != m_right) {
				if (other->collidesWithItem(item)) {

					QPointF center = ItemCenter(item);
					for (int j = 0; j < m_input_pts.size(); ++j) {
						if (center == m_input_pts[j]) {
							m_input_pts.removeAt(j);
							break;
						}
					}

					m_item_list.removeAt(i);
					m_scene->removeItem(item);
					m_scene->removeItem(other);
					m_item_list[i - 1]->setOpacity(1.0);
					m_current = m_item_list[i - 1];
					return;
				}
			}
		}
	}

	m_scene->removeItem(other);
}

void CurveItem::SetCurvePoints(const std::vector<double>& y_values) {

	if (m_curve_pts.size() != y_values.size())
		m_curve_pts.resize(y_values.size());

	for (int i = 0; i < m_curve_pts.size(); ++i)
		m_curve_pts[i] = QPointF(i, y_values[i] * m_scene->height());

	m_scene->removeItem(m_curve);
	QPainterPath path;
	path.addPolygon(m_curve_pts);
	m_curve = m_scene->addPath(path, m_pen);
}

void CurveItem::setCurveVisibility(bool visible) {
	if (m_item_list.size() != 2 || ItemCenter(m_left) != QPointF(0, 0) || ItemCenter(m_right) != QPointF(380, 380))
		m_curve->setVisible(true);
	else
		m_curve->setVisible(visible);

	for (auto item : m_item_list)
		item->setVisible(visible);
}

void CurveItem::UpdateItemPos(QGraphicsItem* item, QPointF delta) {

	for (int i = 0; i < m_input_pts.size(); ++i)
		if (item == m_item_list[i])
			m_input_pts[i] += delta;

	item->moveBy(delta.x(), delta.y());

	std::sort(m_input_pts.begin(), m_input_pts.end(), [this](QPointF a, QPointF b) {return a.x() < b.x(); });
	std::sort(m_item_list.begin(), m_item_list.end(), [this](QGraphicsItem* a, QGraphicsItem* b) { return ItemCenter(a).x() < ItemCenter(b).x(); });
}

const QGraphicsItem* CurveItem::nextItem() {
	for (int i = 0; i < m_item_list.size(); ++i) {
		if (m_item_list[i] == m_current) {
			if (m_item_list[i] == Right())
				return Right();
			else {
				m_current->setOpacity(0.5);
				m_current = m_item_list[i + 1];
				m_current->setOpacity(1.0);
				return m_current;
			}
		}
	}

	return m_current;
}

const QGraphicsItem* CurveItem::previousItem() {
	for (int i = m_item_list.size() - 1; i >= 0; --i) {
		if (m_item_list[i] == m_current) {
			if (m_item_list[i] == Left())
				return Left();
			else {
				m_current->setOpacity(0.5);
				m_current = m_item_list[i - 1];
				m_current->setOpacity(1.0);
				return m_current;
			}
		}
	}

	return m_current;
}








CurveScene::CurveScene(CurveTransform* ct, QRect rect, QWidget* parent) : QGraphicsScene(rect, parent) {
	using CC = ColorComponent;
	this->setSceneRect(rect);
	m_ctp = ct;

	drawGrid();

	for (ColorComponent cc = CC::red; cc <= CC::saturation; cc = CC(int(cc) + 1)) {
		int i = int(cc);
		m_curve_items[i] = new CurveItem(m_color[i], m_color[i], this);
		m_curve_items[i]->setCurveVisibility(false);
	}	
	
	m_curve_items[int(CC::rgb_k)]->setCurveVisibility(true);

	m_current = m_curve_items[int(CC::rgb_k)]->Left();
}

void CurveScene::drawGrid() {
	int step_x = width() / 4;
	int step_y = height() / 4;

	int x = step_x;
	int y = step_y;

	QPen pen = QColor(123, 123, 123);
	pen.setWidthF(.75);

	for (int i = 1; i < 4; ++i) {
		QLineF line(step_x * i, 0, step_x * i, height());

		addLine(line, pen);
		line = QLine(0, step_y * i, width(), step_y * i);
		addLine(line, pen);
	}
}

void CurveScene::resetScene() {
	using CC = ColorComponent;
	clear();
	drawGrid();

	for (ColorComponent cc = CC::red; cc <= CC::saturation; cc = CC(int(cc) + 1)) {
		int i = int(cc);
		m_curve_items[i] = new CurveItem(m_color[i], m_color[i], this);
		m_curve_items[i]->setCurveVisibility(false);
	}

	m_curve_items[int(m_comp)]->setCurveVisibility(true);

	m_current = m_curve_items[int(m_comp)]->Left();
}

void CurveScene::ChannelChanged(int id) {
	CurveItem* curve = m_curve_items[int(m_comp)];

	curve->setCurveVisibility(false);

	m_comp = ColorComponent(id);
	curve = m_curve_items[int(m_comp)];
	curve->setCurveVisibility(true);
	m_current = curve->Current();
	itemARC();
	sendCurrentPos(CurveItem::ItemCenter(m_current));
	sendCurveType(int(curve->CurveType()));
}

void CurveScene::CurveTypeChanged(int id) {
	CurveItem* curve = m_curve_items[int(m_comp)];

	(*m_ctp).SetInterpolationMethod(m_comp, CurveType(id));
	curve->SetCurveType(CurveType(id));
	RenderCurve(curve);
}

void CurveScene::onPressed_previous_next() {
	m_current = m_curve_items[int(m_comp)]->Current();
}

void CurveScene::onUpdatePoint(QPointF point) {
	CurveItem* curve = m_curve_items[int(m_comp)];
	curve->UpdateItemPos(m_current, point -= CurveItem::ItemCenter(m_current));

	if (xValueExists()) {
		curve->UpdateItemPos(m_current, QPointF(1, 0));
		sendCurrentPos(CurveItem::ItemCenter(m_current));
	}

	RenderCurve(curve);
	itemARC();
}

bool CurveScene::xValueExists()const {
	auto curve = m_curve_items[int(m_comp)];

	for (auto item : curve->ItemList())
		if (item != m_current)
			if (CurveItem::ItemCenter(item).x() == CurveItem::ItemCenter(m_current).x())
				return true;
		
	return false;
}

bool CurveScene::isInScene(const QRectF rect)const {
	double mid_x = m_current->boundingRect().width() / 2.0;
	double mid_y = m_current->boundingRect().height() / 2.0;

	if (0 < rect.x() + mid_x && rect.x() + mid_x < width() && 0 < rect.y() + mid_y && rect.y() + mid_y < height())
		return true;
	else
		return false;
}

bool CurveScene::isInScene_ends(const QRectF rect)const {
	double mid_y = m_current->boundingRect().height() / 2;
	if (0 <= rect.y() + mid_y && (rect.y() + mid_y) <= height())
		return true;
	else
		return false;
}

void CurveScene::RenderCurve(CurveItem* curve) {

	(*m_ctp).SetDataPoints(m_comp, curve->GetNormalizedInputs());

	(*m_ctp).ComputeCoefficients(m_comp);

	for (int i = 0; i < m_values.size(); ++i)
		m_values[i] = i / width();


	(*m_ctp).InterpolateValues(m_comp, m_values);

	curve->SetCurvePoints(m_values);
}


void CurveScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {

	auto curve = m_curve_items[int(m_comp)];

	if (event->buttons() == Qt::RightButton) {
		curve->RemoveItem(event->scenePos());
		m_current = curve->Current();
	}

	if (event->buttons() == Qt::LeftButton) {

		click_x = event->scenePos().x();
		click_y = event->scenePos().y();

		m_current = curve->AddEllipse(click_x, click_y);
	}

	RenderCurve(curve);
	QPointF p = CurveItem::ItemCenter(m_current);
	sendCurrentPos(CurveItem::ItemCenter(m_current));
	end((m_current == curve->Left() || m_current == curve->Right()));
	itemARC();
}

void CurveScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {

		int dx = event->scenePos().x() - click_x;
		int dy = event->scenePos().y() - click_y;

		click_x = event->scenePos().x();
		click_y = event->scenePos().y();


		auto curve = m_curve_items[int(m_comp)];

		QRectF rect = m_current->sceneBoundingRect();

		if (m_current == curve->Left() || m_current == curve->Right()) {
			rect.adjust(0, dy, 0, dy);
			if (isInScene_ends(rect)) 
				curve->UpdateItemPos(m_current, QPointF(0, dy));		
		}
		else {

			if (curve->CollidesWithOtherItem(m_current))
				dx *= -1, dy *= -1;

			rect.adjust(dx, dy, dx, dy);
			if (isInScene(rect)) 
				curve->UpdateItemPos(m_current, QPointF(dx, dy));
			
		}

		RenderCurve(curve);
		sendCurrentPos(CurveItem::ItemCenter(m_current));
	}
}

void CurveScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	if (xValueExists()) {
		auto curve = m_curve_items[int(m_comp)];
		curve->UpdateItemPos(m_current, QPointF(1, 0));
		RenderCurve(curve);
		sendCurrentPos(CurveItem::ItemCenter(m_current));
	}
}






CurveTransformDialog::CurveTransformDialog(QWidget* parent): ProcessDialog("CurveTransform", QSize(400,535), *reinterpret_cast<FastStack*>(parent)->m_workspace, parent) {

	using CTD = CurveTransformDialog;

	this->setWindowTitle(Name());

	setTimer(250, this, &CTD::ApplytoPreview);

	connect(this, &ProcessDialog::processDropped, this, &CTD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &CTD::Apply, &CTD::showPreview, &CTD::resetDialog);


	m_cs = new CurveScene(&m_ct, QRect(0, 0, 380, 380));
	connect(m_cs, &CurveScene::itemARC, this, &CurveTransformDialog::onItemARC);
	m_cs->setBackgroundBrush(QBrush("#404040"));

	m_gv = new QGraphicsView(m_cs, this);
	m_gv->setRenderHints(QPainter::Antialiasing);
	m_gv->scale(1, -1);
	m_gv->setGeometry(9, 9, 382, 382);
	m_gv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_gv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	AddComponentSelection();
	AddPointLineEdits();
	AddCuveTypeSelection();
	AddPointSelection();
	this->show();
}

void CurveTransformDialog::onItemARC() {
	auto curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));
	QString c = QString::number(curve->ItemIndex(curve->Current()) + 1);
	QString t = QString::number(curve->ItemListSize());
	
	m_current_point->setText(c + "/" + t);	
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

void CurveTransformDialog::onPressed_previous() {
	CurveItem* curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));
	const QGraphicsItem* item = curve->previousItem();
	int index = curve->ItemIndex(item);

	m_input_le->setValue(curve->InputPoint(index).x() / m_cs->width());
	m_output_le->setValue(curve->InputPoint(index).y() / m_cs->height());

	if (item == curve->Left() || item == curve->Right())
		m_input_le->setDisabled(true);
	else
		m_input_le->setEnabled(true);

	onItemARC();
}

void CurveTransformDialog::onPressed_next() {
	CurveItem* curve = m_cs->curveItem(ColorComponent(m_component_bg->checkedId()));
	const QGraphicsItem* item = curve->nextItem();
	int index = curve->ItemIndex(item);

	m_input_le->setValue(curve->InputPoint(index).x() / m_cs->width());
	m_output_le->setValue(curve->InputPoint(index).y() / m_cs->height());

	if (item == curve->Left() || item == curve->Right())
		m_input_le->setDisabled(true);
	else
		m_input_le->setEnabled(true);

	onItemARC();
}


void CurveTransformDialog::AddComponentSelection() {
	using CC = ColorComponent;

	m_component_bg = new QButtonGroup(this);

	QSize size(35, 25);
	int x = 10;
	int dx = 35;
	int y = 395;
	
	QPixmap pm = QPixmap(15, 15);
	pm.fill(QColor(255, 0, 0));

	ComponentPushButton* pb = new ComponentPushButton(QIcon(pm), "R", this);
	m_component_bg->addButton(pb, int(CC::red));
	pb->resize(size);
	pb->move(x, y);

	pm.fill(QColor(0, 255, 0));
	pb = new ComponentPushButton(QIcon(pm), "G", this);
	m_component_bg->addButton(pb, int(CC::green));
	pb->resize(size);
	pb->move(x += dx, y);

	pm.fill(QColor(0, 0, 255));
	pb = new ComponentPushButton(QIcon(pm), "B", this);
	m_component_bg->addButton(pb, int(CC::blue));
	pb->resize(size);
	pb->move(x += dx, y);

	pm = QPixmap("C:\\Users\\Zack\\Desktop\\rgb.png");
	pb = new ComponentPushButton(QIcon(pm),"RGB/K", this);
	m_component_bg->addButton(pb, int(CC::rgb_k));
	pb->setChecked(true);
	pb->resize(65,25);
	pb->move(x += dx, y);



	pb = new ComponentPushButton("L", this);
	m_component_bg->addButton(pb, int(CC::Lightness));
	pb->resize(size);
	pb->move(x += 65, y);

	pb = new ComponentPushButton("a", this);
	m_component_bg->addButton(pb, int(CC::a));
	pb->resize(size);
	pb->move(x += dx, y);

	pb = new ComponentPushButton("b", this);
	m_component_bg->addButton(pb, int(CC::b));
	pb->resize(size);
	pb->move(x += dx, y);

	pb = new ComponentPushButton("c", this);
	m_component_bg->addButton(pb, int(CC::c));
	pb->resize(size);
	pb->move(x += dx, y);



	pb = new ComponentPushButton("H", this);
	m_component_bg->addButton(pb, int(CC::hue));
	pb->resize(size);
	pb->move(x += dx, y);

	pb = new ComponentPushButton("S", this);
	m_component_bg->addButton(pb, int(CC::saturation));
	pb->resize(size);
	pb->move(x += dx, y);

	connect(m_component_bg, &QButtonGroup::idClicked, m_cs, &CurveScene::ChannelChanged);
}

void CurveTransformDialog::AddCuveTypeSelection() {
	m_curvetype_bg = new QButtonGroup(this);

	QPushButton* pb = new QPushButton("Akima", this);
	pb->setCheckable(true);
	pb->setChecked(true);
	pb->setAutoDefault(false);
	m_curvetype_bg->addButton(pb, int(CurveType::akima_spline));
	pb->move(75, 470);

	pb = new QPushButton("Cubic", this);
	pb->setAutoDefault(false);
	pb->setCheckable(true);
	m_curvetype_bg->addButton(pb, int(CurveType::cubic_spline));
	pb->move(160, 470);

	pb = new QPushButton("Linear", this);
	pb->setAutoDefault(false);
	pb->setCheckable(true);
	m_curvetype_bg->addButton(pb, int(CurveType::linear));
	pb->move(245, 470);

	connect(m_curvetype_bg, &QButtonGroup::idClicked, m_cs, &CurveScene::CurveTypeChanged);
	connect(m_cs, &CurveScene::sendCurveType, this, [this](int id) { m_curvetype_bg->button(id)->setChecked(true); });
}


void CurveTransformDialog::AddPointLineEdits() {

	m_input_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6, this), this);
	m_input_le->setValue(0.0);
	m_input_le->setFixedWidth(75);
	m_input_le->move(65, 430);
	m_input_le->addLabel(new QLabel("Input:   ", this));
	m_input_le->setDisabled(true);


	m_output_le = new DoubleLineEdit(new DoubleValidator(0.0, 1.0, 6, this), this);
	m_output_le->setValue(0.0);
	m_output_le->setFixedWidth(75);
	m_output_le->move(210, 430);
	m_output_le->addLabel(new QLabel("Output:   ", this));

	connect(m_input_le, &DoubleLineEdit::editingFinished, this, &CurveTransformDialog::onEditingFinished_io);
	connect(m_output_le, &DoubleLineEdit::editingFinished, this, &CurveTransformDialog::onEditingFinished_io);
	connect(this, &CurveTransformDialog::updatePoint, m_cs, &CurveScene::onUpdatePoint);

	connect(m_cs, &CurveScene::sendCurrentPos, this, &CurveTransformDialog::onCurrentPos);
	connect(m_cs, &CurveScene::end, this, [this](bool v) { m_input_le->setDisabled(v); });

}

void CurveTransformDialog::AddPointSelection() {

	QPushButton* pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekForward), "", this);
	pb->setAutoDefault(false);
	pb->setFlat(true);
	pb->move(315, 430);

	connect(pb, &QPushButton::pressed, this, &CurveTransformDialog::onPressed_next);
	connect(pb, &QPushButton::pressed, m_cs, &CurveScene::onPressed_previous_next);

	pb = new QPushButton(style()->standardIcon(QStyle::SP_MediaSeekBackward), "", this);
	pb->setAutoDefault(false);
	pb->setFlat(true);
	pb->move(285, 430);

	connect(pb, &QPushButton::pressed, this, &CurveTransformDialog::onPressed_previous);
	connect(pb, &QPushButton::pressed, m_cs, &CurveScene::onPressed_previous_next);

	m_current_point = new QLabel("1/2", this);
	m_current_point->move(350, 427);
	m_current_point->resize(m_current_point->fontMetrics().horizontalAdvance("00/00"), m_current_point->size().height());
}

void CurveTransformDialog::resetDialog() {
	m_ct = CurveTransform();

	m_input_le->setText("0.000000");
	m_output_le->setText("0.000000");
	m_input_le->setDisabled(true);
	m_curvetype_bg->button(int(CurveType::akima_spline))->setChecked(true);
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

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_ct, &CurveTransform::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_ct, &CurveTransform::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_ct, &CurveTransform::Apply);
		break;
	}
	}

	ApplytoPreview();

}

void CurveTransformDialog::ApplytoPreview() {

	if (!isPreviewValid())
		return;

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		auto iw8 = reinterpret_cast<PreviewWindow8*>(iwptr->Preview());
		return iw8->UpdatePreview(m_ct, &CurveTransform::Apply);
	}
	case 16: {
		auto iw16 = reinterpret_cast<PreviewWindow16*>(iwptr->Preview());
		return iw16->UpdatePreview(m_ct, &CurveTransform::Apply);
	}
	case -32: {
		auto iw32 = reinterpret_cast<PreviewWindow32*>(iwptr->Preview());
		return iw32->UpdatePreview(m_ct, &CurveTransform::Apply);
	}
	}
}