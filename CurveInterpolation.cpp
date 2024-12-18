#include "pch.h"
#include "CurveInterpolation.h"
#include "Matrix.h"
#include "ProcessDialog.h"


Curve::Curve(const QPointF& a, const QPointF& b) {
	m_points = { a, b };
	computeCoeffecients();
}

void Curve::setDataPoints(const std::vector<QPointF>& pts) {

	if (m_points.size() != pts.size())
		m_points.resize(pts.size());

	memcpy(&m_points[0], &pts[0], m_points.size() * sizeof(QPointF));
}

bool Curve::isIdentity()const {
	return (m_points.size() == 2 && m_points[0].x() == 0.0f && m_points[0].y() == 0.0f && m_points[1].x() == 1.0f && m_points[1].y() == 1.0f);
}

void Curve::cubicSplineCurve() {

	std::sort(m_points.begin(), m_points.end(), comaparePoints);

	int dim = 4 * (m_points.size() - 1);

	Matrix x(dim, dim);
	Matrix constants(dim);
	Matrix y(dim);

	int start = 0;
	for (int row = start, offset = 0, el = 0; row < 2 * m_points.size() - 2; ++row) {

		x(row, 3 + offset) = 1;
		x(row, 2 + offset) = m_points[el].x();
		x(row, 1 + offset) = m_points[el].x() * m_points[el].x();
		x(row, offset) = m_points[el].x() * x(row, 1 + offset);

		y[row] = m_points[el].y();

		if (row % 2 != 0 && row != start)
			offset += 4;
		if (row % 2 == 0)
			el++;
	}

	start += (2 * m_points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (m_points.size() - 2); ++row, offset += 4, ++el) {

		x(row, offset) = 3 * (m_points[el].x() * m_points[el].x());
		x(row, 1 + offset) = 2 * m_points[el].x();
		x(row, 2 + offset) = 1;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -x(row, 1 + offset);
		x(row, 6 + offset) = -1;

		y[row] = 0;
	}

	start += (m_points.size() - 2);
	for (int row = start, offset = 0, el = 1; row < start + (m_points.size() - 2); ++row, offset += 4, ++el) {
		x(row, offset) = 6 * m_points[el].x();
		x(row, 1 + offset) = 2;
		x(row, 4 + offset) = -x(row, offset);
		x(row, 5 + offset) = -2;

		y[row] = 0;
	}

	start += (m_points.size() - 2);
	x(start, 1) = 2;
	start++;
	x(start, dim - 4) = 6 * m_points[m_points.size() - 1].x();
	x(start, dim - 3) = 2;


	constants = Matrix::leastSquares(x, y);

	m_splc.resize(m_points.size() - 1);


	for (int el = 0, offset = 0; el < m_splc.size(); ++el, offset += 4)
		m_splc[el] = { constants[offset],constants[1 + offset],constants[2 + offset],constants[3 + offset] };

}

void Curve::akimaSplineCurve() {

	std::sort(m_points.begin(), m_points.end(), comaparePoints);

	int spline_count = m_points.size() - 1;

	MyVector<double, 2> m_vec(spline_count + 4);
	for (int el = 0; el < spline_count; ++el)
		m_vec[el] = (m_points[el + 1].y() - m_points[el].y()) / (m_points[el + 1].x() - m_points[el].x());

	std::vector<double> s_vec(m_points.size());

	m_vec[-2] = 3 * m_vec[0] - 2 * m_vec[1];
	m_vec[-1] = 2 * m_vec[0] - m_vec[1];
	m_vec[spline_count] = 2 * m_vec[spline_count - 1] - m_vec[spline_count - 2];
	m_vec[spline_count + 1] = 3 * m_vec[spline_count - 1] - 2 * m_vec[spline_count - 2];


	for (int el = 0; el <= spline_count; ++el) {

		double a = abs(m_vec[el + 1] - m_vec[el]);
		double b = abs(m_vec[el - 1] - m_vec[el - 2]);

		s_vec[el] = (a + b != 0) ? ((a * m_vec[el - 1]) + (b * m_vec[el])) / (a + b) : 0.5 * (m_vec[el - 1] + m_vec[el]);
	}

	m_splc.resize(spline_count);

	for (int el = 0; el < spline_count; ++el) {
	
		m_splc[el][3] = m_points[el].y();
		m_splc[el][2] = s_vec[el];
		double dx = m_points[el + 1].x() - m_points[el].x();
		m_splc[el][1] = (3 * m_vec[el] - 2 * s_vec[el] - s_vec[el + 1]) / dx;
		m_splc[el][0] = (s_vec[el] + s_vec[el + 1] - 2 * m_vec[el]) / (dx * dx);
	}
}

void Curve::linearCurve() {

	std::sort(m_points.begin(), m_points.end(), comaparePoints);

	m_splc.resize(m_points.size() - 1);

	for (int i = 0; i < m_points.size() - 1; ++i)
		m_splc[i][0] = (m_points[i + 1].y() - m_points[i].y()) / (m_points[i + 1].x() - m_points[i].x());

}

void Curve::computeCoeffecients() {
	using enum Type;

	switch (m_type) {
	case akima_spline:
		if (m_points.size() > 4)
			return akimaSplineCurve();

	case cubic_spline:
		return cubicSplineCurve();

	case linear:
		return linearCurve();

	}
}

double Curve::interpolate(double pixel)const {

	if (isIdentity())
		return pixel;

	using enum Type;
	switch (m_type) {
	case akima_spline:
		if (m_points.size() > 4)
			return akimaInterpolator(pixel);

	case cubic_spline:
		return cubicInterpolator(pixel);

	case linear:
		return linearInterpolator(pixel);

	default:
		return cubicInterpolator(pixel);
	}
}

double Curve::akimaInterpolator(double pixel)const {
	for (int el = 0; el < m_points.size() - 1; ++el) {
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x()) {
			pixel -= m_points[el].x();
			return (m_splc[el][0] * pixel * pixel * pixel + m_splc[el][1] * pixel * pixel + m_splc[el][2] * pixel + m_splc[el][3]);
		}
	}
	return pixel;
}

double Curve::cubicInterpolator(double pixel)const {
	if (m_points.size() == 0) return pixel;
	for (int el = 0; el < m_points.size() - 1; ++el)
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x()) 
			return (m_splc[el][0] * pixel * pixel * pixel + m_splc[el][1] * pixel * pixel + m_splc[el][2] * pixel + m_splc[el][3]);
		
	return pixel;
}

double Curve::linearInterpolator(double pixel)const {
	if (m_points.size() == 0) return pixel;

	for (int el = 0; el < m_points.size() - 1; ++el)
		if (m_points[el].x() <= pixel && pixel < m_points[el + 1].x())
			return (m_splc[el][0] * (pixel - m_points[el].x()) + m_points[el].y());

	return pixel;
}






CurveTypeButtonGroup::CurveTypeButtonGroup(QWidget* parent) : QWidget(parent) {

	this->resize(108, 36);

	m_bg = new QButtonGroup(this);
	QIcon icon = QIcon("./Icons//akima.bmp");


	QSize s(32, 32);
	ComponentPushButton* pb = new ComponentPushButton(icon, "", this);
	pb->setIconSize(QSize(25, 25));
	pb->resize(s);
	pb->move(2, 2);
	pb->setToolTip("Akima Spline Interpolation");

	pb->setChecked(true);
	m_bg->addButton(pb, int(Curve::Type::akima_spline));

	icon = QIcon("./Icons//cubic.bmp");
	pb = new ComponentPushButton(icon, "", this);
	pb->setIconSize(QSize(25, 25));
	pb->resize(s);
	pb->move(38, 2);

	pb->setToolTip("Cubic Spline Interpolation");

	m_bg->addButton(pb, int(Curve::Type::cubic_spline));

	icon = QIcon("./Icons//linear.bmp");
	pb = new ComponentPushButton(icon, "", this);
	pb->setIconSize(QSize(25, 25));
	pb->resize(s);
	pb->move(2 + 72, 2);
	pb->setToolTip("Linear Interpolation");

	m_bg->addButton(pb, int(Curve::Type::linear));

	connect(m_bg, &QButtonGroup::idClicked, this, [this](int id) { idClicked(id); });

}






CurveItem::CurveItem(QPen pen, QBrush brush, QGraphicsScene* scene, float left_y, float right_y) : m_pen(pen), m_brush(brush) {

	m_scene = scene;

	m_pen.setWidthF(1.01);

	m_left_default = { 0, left_y * m_scene->height() };
	m_right_default = { m_scene->width(), right_y * m_scene->height() };

	m_current = m_left = m_scene->addEllipse(m_left_default.x(), m_left_default.y(), 8, 8, pen, brush);
	m_left->moveBy(-4, -4);
	m_item_list.append(m_left);

	m_right = m_scene->addEllipse(m_right_default.x(), m_right_default.y(), 8, 8, pen, brush);
	m_right->setOpacity(0.5);
	m_right->moveBy(-4, -4);
	m_item_list.append(m_right);

	m_curve_pts.resize(scene->width());

	float m = (m_right_default.y() - m_left_default.y()) / (m_right_default.x() - m_left_default.x());

	for (int i = 0; i < m_curve_pts.size(); ++i)
		m_curve_pts[i] = QPointF(i, (m * i) + m_left_default.y());

	QPainterPath path;
	path.addPolygon(m_curve_pts);
	m_curve = m_scene->addPath(path, m_pen);
}

QPointF CurveItem::itemCenter(const QGraphicsItem* item) {
	return item->sceneBoundingRect().center();
}

QPointF CurveItem::itemCenter_norm(const QGraphicsItem* item) {
	QPointF p = item->sceneBoundingRect().center();

	return QPointF(p.x() / m_scene->width(), p.y() / m_scene->height());
}

QGraphicsItem* CurveItem::addItem(qreal x, qreal y) {
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

	//m_input_pts.append(itemCenter(new_item));
	m_item_list.append(new_item);

	//std::sort(m_input_pts.begin(), m_input_pts.end(), [this](QPointF a, QPointF b) {return a.x() < b.x(); });
	std::sort(m_item_list.begin(), m_item_list.end(), [this](QGraphicsItem* a, QGraphicsItem* b) { return itemCenter(a).x() < itemCenter(b).x(); });

	return m_current = new_item;
}

bool CurveItem::curveCollision(const QGraphicsItem* other)const {

	for (auto item : m_item_list) {
		if (item == other)
			continue;
		if (other->collidesWithItem(item))
			return true;
	}

	return false;
}

std::vector<QPointF> CurveItem::curvePoints_norm() {

	std::vector<QPointF> p(m_item_list.size());

	for (int i = 0; i < p.size(); ++i)
		p[i] = this->itemCenter_norm(m_item_list[i]);

	return p;
}

//removes item sets current to previous item(lower x pos)
void CurveItem::removeItem(QPointF point) {
	auto other = m_scene->addEllipse(point.x(), point.y(), 1, 1);

	for (int i = 0; i < m_item_list.size(); ++i) {
		auto item = m_item_list[i];

		if (item != m_left) {
			if (item != m_right) {
				if (other->collidesWithItem(item)) {

					QPointF center = itemCenter(item);
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

void CurveItem::setCurvePoints(const std::vector<double>& y_values) {

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
	if (m_item_list.size() != 2 || itemCenter(m_left) != m_left_default || itemCenter(m_right) != m_right_default)
		m_curve->setVisible(true);
	else
		m_curve->setVisible(visible);

	for (auto item : m_item_list)
		item->setVisible(visible);
}

void CurveItem::updateItemPos(QGraphicsItem* item, QPointF delta) {

	item->moveBy(delta.x(), delta.y());

	std::sort(m_item_list.begin(), m_item_list.end(), [this](QGraphicsItem* a, QGraphicsItem* b) { return itemCenter(a).x() < itemCenter(b).x(); });
}

const QGraphicsItem* CurveItem::nextItem() {
	for (int i = 0; i < m_item_list.size(); ++i) {
		if (m_item_list[i] == m_current) {
			if (m_item_list[i] == endItem_Right())
				return endItem_Right();
			else {
				m_current->setOpacity(0.5);
				m_current = m_item_list[i + 1];
				m_current->setOpacity(1.0);
				return m_current;
			}
		}
	}
	
	itemChanged(m_current);
	return m_current;
}

const QGraphicsItem* CurveItem::previousItem() {
	for (int i = m_item_list.size() - 1; i >= 0; --i) {
		if (m_item_list[i] == m_current) {
			if (m_item_list[i] == endItem_Left())
				return endItem_Left();
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





CurveScene::CurveScene(QRect rect, QWidget* parent) : QGraphicsScene(rect, parent) {
	using CC = ColorComponent;
	this->setSceneRect(rect);
	drawGrid();
	for (ColorComponent cc = CC::red; cc <= CC::saturation; cc = CC(int(cc) + 1)) {
		int i = int(cc);
		m_curve_items[i] = new CurveItem(m_color[i], m_color[i], this);
		m_curve_items[i]->setCurveVisibility(false);
		connect(m_curve_items[i], &CurveItem::itemChanged, this, [this](QGraphicsItem* item) { m_current = item; });
	}

	m_curve_items[int(CC::rgb_k)]->setCurveVisibility(true);
	m_values.resize(rect.width());
	m_current = m_curve_items[int(CC::rgb_k)]->endItem_Left();
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

	m_current = m_curve_items[int(m_comp)]->endItem_Left();
}

void CurveScene::onChannelChange(int id) {
	CurveItem* curve = m_curve_items[int(m_comp)];

	curve->setCurveVisibility(false);

	m_comp = ColorComponent(id);
	curve = m_curve_items[int(m_comp)];
	curve->setCurveVisibility(true);
	m_current = curve->currentItem();

	itemARC();
	endItem(curve->isEnd(m_current));
	currentPos(CurveItem::itemCenter(m_current));
}

void CurveScene::onUpdatePoint(QPointF point) {

	CurveItem* curve = m_curve_items[int(m_comp)];
	curve->updateItemPos(m_current, point -= CurveItem::itemCenter(m_current));

	if (xValueExists()) {
		curve->updateItemPos(m_current, QPointF(1, 0));
		currentPos(CurveItem::itemCenter(m_current));
	}

	itemARC();
}

bool CurveScene::xValueExists()const {
	auto curve = m_curve_items[int(m_comp)];

	for (auto item : curve->itemList())
		if (item != m_current)
			if (CurveItem::itemCenter(item).x() == CurveItem::itemCenter(m_current).x())
				return true;

	return false;
}


bool CurveScene::isInScene(const QPointF& p)const {

	if ((0 < p.x() && p.x() < width()) && (0 < p.y() && p.y() < height()))
		return true;
	else
		return false;
}

bool CurveScene::isInScene_Edge(const QPointF& p)const {

	if (0 <= p.y() && p.y() <= height())
		return true;
	else
		return false;
}

void CurveScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {

	auto curve = m_curve_items[int(m_comp)];

	if (event->buttons() == Qt::RightButton) {
		curve->removeItem(event->scenePos());
		m_current = curve->currentItem();
	}

	if (event->buttons() == Qt::LeftButton) {

		if (curve->itemList().size() >= 50)
			return;

		click_x = event->scenePos().x();
		click_y = event->scenePos().y();

		m_current = curve->addItem(click_x, click_y);
	}

	currentPos(CurveItem::itemCenter(m_current));
	endItem(curve->isEnd(m_current));
	itemARC();
}

void CurveScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {

		int dx = event->scenePos().x() - click_x;
		int dy = event->scenePos().y() - click_y;

		click_x = event->scenePos().x();
		click_y = event->scenePos().y();


		auto curve = m_curve_items[int(m_comp)];
		QPointF p = CurveItem::itemCenter(m_current);

		if (curve->isEnd(m_current)) {

			p.ry() += dy;

			if (isInScene_Edge(p)) {
				curve->updateItemPos(m_current, QPointF(0, dy));

				if (curve->isEndsLinked()) {
					if (m_current == curve->endItem_Left())
						curve->updateItemPos(curve->endItem_Right(), QPointF(0, dy));
					else if (m_current == curve->endItem_Right())
						curve->updateItemPos(curve->endItem_Left(), QPointF(0, dy));
				}
			}
		}
		else {

			if (curve->curveCollision(m_current))
				dx *= -1, dy *= -1;

			p +=QPointF(dx, dy);
			
			if (isInScene(p))
				curve->updateItemPos(m_current, QPointF(dx, dy));

		}

		currentPos(CurveItem::itemCenter(m_current));
	}
}

void CurveScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
	if (xValueExists()) {
		auto curve = m_curve_items[int(m_comp)];
		curve->updateItemPos(m_current, QPointF(1, 0));
		currentPos(CurveItem::itemCenter(m_current));
	}
}





GradientImage::GradientImage(const QSize& size, Qt::Orientation orientation, int poly_degree) : m_direction(orientation), m_poly(poly_degree) {
	m_src = QImage(size, QImage::Format::Format_ARGB32);
}

double GradientImage::computePolynomial(const Matrix& coefficients, int x, int poly_degree) {
	std::vector<double> xv(poly_degree + 1);
	xv[0] = 1;

	for (int i = 1, j = 0; i <= poly_degree; ++i, ++j)
		xv[i] = xv[j] * x;

	double sum = 0;

	for (int i = 0; i <= poly_degree; ++i)
		sum += xv[i] * coefficients[i];

	return sum;
}

Matrix GradientImage::computeCoeficients(const std::vector<XRGB>& pts, ColorComponent cc, int poly_deg) {

	int n_pts = pts.size();

	Matrix var(poly_deg + 1, poly_deg + 1);
	Matrix r_sum(poly_deg + 1);

	for (int j = 0, h = 0; j < var.rows(); ++j, ++h) {
		for (int i = 0, f = 0; i < var.cols(); ++i, ++f) {

			if (i == 0 && j == 0)
				var(j, i) = n_pts;

			else {
				double sum = 0;
				for (int N = 0; N < n_pts; ++N)
					sum += pow(pts[N].x, f + h);
				var(j, i) = sum;
			}
		}

		double s = 0;

		for (int n = 0; n < n_pts; ++n)
			s += (pts[n](cc) / 255.0) * pow(pts[n].x, h);

		r_sum[j] = s;
	}

	return Matrix::leastSquares(var, r_sum);
}

Matrix GradientImage::computeCoeficients_RGB() {
	int n_pts = 9;
	int _1_n = ((m_direction == Qt::Horizontal) ? m_src.width() : m_src.height()) / n_pts;

	Matrix var(4, 4);
	Matrix r_sum(4);

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

		for (int n = 0; n < (n_pts / 3); ++n)
			s += v;

		r_sum[j] = s;
	}
	return Matrix::leastSquares(var, r_sum);
}

void GradientImage::setGradient(const std::vector<XRGB>& pts, float opacity) {
	uint8_t a = Clip(opacity) * 255;

	Matrix m_rcoef = computeCoeficients(pts, ColorComponent::red, m_poly);
	Matrix m_gcoef = computeCoeficients(pts, ColorComponent::green, m_poly);
	Matrix m_bcoef = computeCoeficients(pts, ColorComponent::blue, m_poly);

	for (int y = 0; y < m_src.height(); ++y) {

		for (int x = 0; x < m_src.width(); ++x) {

			int p = (m_direction == Qt::Horizontal) ? x : y;

			m_src.scanLine(y)[4 * x + 0] = 255 * Clip(computePolynomial(m_bcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 1] = 255 * Clip(computePolynomial(m_gcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 2] = 255 * Clip(computePolynomial(m_rcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 3] = a;

		}
	}
}

void GradientImage::setGradient(const std::vector<XRGB>& r, const std::vector<XRGB>& g, const std::vector<XRGB>& b, float opacity) {

	uint8_t a = Clip(opacity) * 255;

	Matrix m_rcoef = computeCoeficients(r, ColorComponent::red, m_poly);
	Matrix m_gcoef = computeCoeficients(g, ColorComponent::green, m_poly);
	Matrix m_bcoef = computeCoeficients(b, ColorComponent::blue, m_poly);

	for (int y = 0; y < m_src.height(); ++y) {

		for (int x = 0; x < m_src.width(); ++x) {

			int p = (m_direction == Qt::Horizontal) ? x : y;

			m_src.scanLine(y)[4 * x + 0] = 255 * Clip(computePolynomial(m_rcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 1] = 255 * Clip(computePolynomial(m_gcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 2] = 255 * Clip(computePolynomial(m_bcoef, p, m_poly));
			m_src.scanLine(y)[4 * x + 3] = a;

		}
	}
}

void GradientImage::setGradientRGB(float opacity) {

	m_poly = 3;

	uint8_t a = Clip(opacity) * 255;

	Matrix m_rcoef = computeCoeficients_RGB();
	Matrix m_gcoef = computeCoeficients_RGB();
	Matrix m_bcoef = computeCoeficients_RGB();

	int v = ((m_direction == Qt::Horizontal) ? m_src.width() : m_src.height());
	int r_off = v / 9;
	int g_off = (2 * v) / 9;
	int b_off = (5 * v) / 9;

	for (int y = 0; y < m_src.height(); ++y) {

		for (int x = 0; x < m_src.width(); ++x) {

			int p = (m_direction == Qt::Horizontal) ? x : y;

			m_src.scanLine(y)[4 * x + 0] = 255 * Clip(computePolynomial(m_gcoef, p - b_off, m_poly));
			m_src.scanLine(y)[4 * x + 1] = 255 * Clip(computePolynomial(m_gcoef, p - g_off, m_poly));
			m_src.scanLine(y)[4 * x + 2] = 255 * Clip(computePolynomial(m_rcoef, p + r_off, m_poly));

			m_src.scanLine(y)[4 * x + 3] = a;

		}
	}
}

void GradientImage::cutCorners(Corner_t corner_flags) {

	if (corner_flags == Corner::none)
		return;

	int min = Min(m_src.width(), m_src.height());

	if (corner_flags & Corner::top_left) {
		int ex = min;

		for (int y = 0; y < min; ++y, --ex) {
			for (int x = 0; x < ex; ++x) {
				m_src.scanLine(y)[4 * x + 3] = 0;
			}
		}
	}

	if (corner_flags & Corner::top_right) {
		int sx = m_src.width() - min;
		for (int y = 0; y < min; ++y, ++sx) {
			for (int x = sx; x < m_src.width(); ++x) {
				m_src.scanLine(y)[4 * x + 3] = 0;
			}
		}
	}

	if (corner_flags & Corner::bot_left) {
		int sx = m_src.height() - min;
		int ex = 0;

		for (int y = sx; y < m_src.height(); ++y, ++ex) {
			for (int x = 0; x < ex; ++x) {
				m_src.scanLine(y)[4 * x + 3] = 0;
			}
		}
	}

	if (corner_flags & Corner::bot_right) {
		int sy = m_src.height() - min;
		int sx = m_src.width();

		for (int y = sy; y < m_src.height(); ++y, --sx) {
			for (int x = sx; x < m_src.width(); ++x) {
				m_src.scanLine(y)[4 * x + 3] = 0;
			}
		}
	}
}

