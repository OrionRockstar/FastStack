#include "pch.h"
#include "HistogramView.h"




HistogramView::HistogramView(const QSize& size, QWidget* parent) : QGraphicsView(parent) {

	this->resize(size);
	this->setRenderHints(QPainter::Antialiasing);

	this->setViewportUpdateMode(QGraphicsView::ViewportUpdateMode::MinimalViewportUpdate);
	this->setMouseTracking(true);
	this->installEventFilter(this);
	addScrollBars();

	m_gs = new QGraphicsScene();
	m_gs->setBackgroundBrush(QColor(19, 19, 19));
	m_gs->setItemIndexMethod(QGraphicsScene::NoIndex);
	m_gs->installEventFilter(this);
	this->setScene(m_gs);

	this->setCursor(Qt::BlankCursor);

	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	m_gs->setSceneRect(rect().adjusted(0, 0, -2, -2));
	m_hist_rect = this->sceneRect().toRect();

	this->drawGrid();

	this->show();
}

template<typename T>
void HistogramView::drawHistogramView(const Image<T>& img, Histogram::Resolution resolution) {

	m_histograms.resize(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch) {
		m_histograms[ch].constructHistogram(img, ch);
		m_histograms[ch].resample(resolution);
	}

	this->drawHistogram();
}
template void HistogramView::drawHistogramView(const Image8&, Histogram::Resolution);
template void HistogramView::drawHistogramView(const Image16&, Histogram::Resolution);
template void HistogramView::drawHistogramView(const Image32&, Histogram::Resolution);

void HistogramView::adjustHistogramRect() {

	QRect r0 = this->sceneRect().toRect();

	int x0 = r0.x() + m_offsetx;
	int y0 = r0.y() + m_offsety;
	int x1 = x0 + m_gs->width() * scale();
	int y1 = y0 + m_gs->height() * scale();

	m_hist_rect = QRect(QPoint(x0, y0), QPoint(x1, y1));

	if (scale() != 1)
		m_hist_rect.adjust(0, 0, -m_vsb->thickness(), -m_hsb->thickness());
}

void HistogramView::emitHistogramValue(const QPoint& p) {

	if (m_histograms.size() > 0) {
		double k = double(m_hist_rect.width() - 1) / (m_histograms[0].resolution() - 1);

		uint16_t low = (p.x() + m_hist_rect.x()) / k;
		uint16_t high = (p.x() + 1 + m_hist_rect.x()) / k;

		if (m_clip) {
			if (low == 0)
				low++;
			if (high == (m_histograms[0].resolution() - 1))
				high--;
		}

		if (k > 1)
			high = low;

		uint32_t pixel_count = 0;
		for (int ch = 0; ch < m_histograms.size(); ++ch) {

			Histogram& hist = m_histograms[ch];
			uint32_t sum = 0;

			for (uint32_t i = low; i <= high; ++i)
				sum += hist[i];

			pixel_count = math::max(pixel_count, sum);
		}

		emit histogramValue(low, high, pixel_count);
	}
}

void HistogramView::removeCursor() {

	if (m_cursor_lines[0]) {
		m_gs->removeItem(m_cursor_lines[0]);
		m_cursor_lines[0] = nullptr;
	}

	if (m_cursor_lines[1]) {
		m_gs->removeItem(m_cursor_lines[1]);
		m_cursor_lines[1] = nullptr;
	}
}

void HistogramView::drawCursor(const QPoint& p) {

	removeCursor();

	QPen pen({ 150,69,255 });
	pen.setWidth(2);

	m_cursor_lines[0] = m_gs->addLine(0, p.y(), m_gs->width(), p.y(), pen);
	m_cursor_lines[1] = m_gs->addLine(p.x(), 0, p.x(), m_gs->height(), pen);
}

void HistogramView::moveCursor(const QPoint& dst) {

	if (!m_gs->sceneRect().contains(dst))
		return;

	auto y = m_cursor_lines[0]->sceneBoundingRect().center().y();
	m_cursor_lines[0]->moveBy(0, dst.y() - y);

	auto x = m_cursor_lines[1]->sceneBoundingRect().center().x();
	m_cursor_lines[1]->moveBy(dst.x() - x, 0);
}

void HistogramView::drawGrid() {

	QPen pen = QColor(255, 255, 255);
	pen.setWidthF(0.5);

	int t = (m_hsb->isVisible()) ? m_hsb->thickness() : 0;

	//for (int y = m_gs->height() * scale() - m_offsety - t; y > 0; y -= 50)
		//if (y < m_gs->height())
			//m_gs->addLine(0, y, m_gs->width(), y, pen);

	//for (int x = (100 - m_offsetx % 100); x < m_gs->width(); x += 100)
		//m_gs->addLine(x, 0, x, m_gs->height(), pen);

	//not updating when no histogram
	double y_gap = m_gs->height() / 4.0;
	for (double y = m_gs->height() * scale() - m_offsety - t; y > 0; y -= y_gap)
		if (y < m_gs->height())
			m_gs->addLine(0, y, m_gs->width(), y, pen);

	double x_gap = m_gs->width() / 4.0;
	for (double x = (x_gap - m_offsetx % (int)x_gap); x < m_gs->width(); x += x_gap)
		m_gs->addLine(x, 0, x, m_gs->height(), pen);

	//pen.setDashPattern({ 10,5 });
	pen.setStyle(Qt::DotLine);
	pen.setWidthF(0.25);
	for (double y = m_gs->height() * scale() - m_offsety - t - y_gap / 2; y > 0; y -= y_gap)
		if (y < m_gs->height())
			m_gs->addLine(0, y, m_gs->width(), y, pen);

	for (double x = (x_gap - m_offsetx % (int)x_gap) - x_gap / 2; x < m_gs->width(); x += x_gap)
		m_gs->addLine(x, 0, x, m_gs->height(), pen);
}

void HistogramView::clearHistogram() {

	clearScene();
	m_histograms.clear();
	m_histograms.shrink_to_fit();
}

void HistogramView::drawHistogram() {

	using GS = GraphStyle;

	this->clearScene();

	if (m_histograms.size() == 0)
		return;

	this->adjustHistogramRect();

	int channel_start = 0;
	int channel_end = m_histograms.size();

	auto comp = colorComponent();

	if (comp == ColorComponent::red || comp == ColorComponent::green || comp == ColorComponent::blue) {

		if (channel_end == 1)
			return;

		channel_end = 1 + (channel_start = int(comp));
	}

	uint32_t peak = 0;
	for (int ch = channel_start; ch < channel_end; ++ch)
		peak = math::max(peak, m_histograms[ch].maxCount(m_clip));

	for (int ch = channel_start; ch < channel_end; ++ch) {
		auto& hist = m_histograms[ch];

		QPolygonF points;

		int r = m_histograms[ch].resolution() - 1;
		double k = double(m_hist_rect.width() - 1) / r;
		int start = math::max<int>((m_hist_rect.x() / k) - 1, 0);
		int end = math::min(r, start + int(m_hist_rect.topRight().x() / (scale() * k)) + 1);


		for (int i = start, x0 = 0, y0 = 0; i <= end; ++i) {

			int x = int((i * k) + 0.5) - m_hist_rect.x();
			int y = math::max(-1, (int)round((1 - double(hist[i]) / peak) * (m_hist_rect.height() - 1)) - m_hist_rect.y());

			if (i == start || i == end || x != x0)
				points.push_back(QPointF(x0 = x, y0 = y));

			else if (y < y0)
				points.back().setY(y0 = y);
		}

		if (m_clip) {
			if (start == 0)
				points.pop_front();

			if (end == r)
				points.pop_back();
		}

		QPen pen = (m_histograms.size() == 3) ? m_pens[ch] : m_pens[3];
		auto c = pen.color();
		c.setAlpha(169);
		pen.setColor(c);

		QPainterPath path;
		qreal h = m_gs->height() - 1;

		switch (m_graph_style) {
		case GS::line: {
			path.addPolygon(points);
			m_gs->addPath(path, pen);
			break;
		}
		case GS::area: {
			//slow with color brush
			points.push_back({ points.back().x(), h });
			points.push_back({ 0, h });
			path.addPolygon(points);
			m_gs->addPath(path, pen, pen.color());
			break;
		}
		case GS::bars: {
			pen.setWidthF(math::max<float>(1, k / 2));
			for (auto p : points)
				m_gs->addLine({ p, {p.x(), h} }, pen);
			break;
		}
		case GS::dots: {
			for (auto p : points)
				m_gs->addLine({ p, p }, pen);
		}
		}

	}
}

void HistogramView::clearScene(bool draw_grid) {

	m_gs->clear();
	m_cursor_lines.fill(nullptr);

	if (draw_grid)
		this->drawGrid();
}


void HistogramView::addScrollBars() {

	m_vsb = new ScrollBar(Qt::Vertical, this);
	m_vsb->setCursor(Qt::ArrowCursor);

	auto verticalAction = [this](int) {
		m_offsety = m_vsb->sliderPosition();
		drawHistogram();
	};

	connect(m_vsb, &QScrollBar::actionTriggered, this, verticalAction);

	m_hsb = new ScrollBar(Qt::Horizontal, this);
	m_hsb->setCursor(Qt::ArrowCursor);

	auto horizontalAction = [this](int) {
		m_offsetx = m_hsb->sliderPosition();
		drawHistogram();
	};

	connect(m_hsb, &QScrollBar::actionTriggered, this, horizontalAction);

	m_corner = new QWidget(this);
	m_corner->setBackgroundRole(QPalette::Button);
	m_corner->setAutoFillBackground(true);
	m_corner->resize(m_vsb->thickness() - 1, m_hsb->thickness() - 1);
	m_corner->setCursor(Qt::ArrowCursor);
}

void HistogramView::showHorizontalScrollBar() {

	m_hsb->setVisible(true);
	int w = m_gs->width();
	int m = (w * scale()) - w;
	m_hsb->setRange(0, m);
	m_hsb->setPageStep(w);
	m_hsb->setSliderPosition(math::clip(m_offsetx, 0, m));
}

void HistogramView::showVerticalScrollBar() {

	m_vsb->setVisible(true);
	int h = m_gs->height();
	int m = (h * scale()) - h;
	m_vsb->setRange(0, m);
	m_vsb->setPageStep(h);
	m_vsb->setSliderPosition(math::clip(m_offsety, 0, m));
}

void HistogramView::showScrollBars() {

	m_corner->setVisible(m_scale != 1);

	if (m_scale > 1) {
		showVerticalScrollBar();
		showHorizontalScrollBar();
	}
	else {
		m_vsb->setHidden(true);
		m_hsb->setHidden(true);
	}

}


bool HistogramView::eventFilter(QObject* obj, QEvent* e) {

	if (obj == m_gs) {
		if (e->type() == QEvent::Enter) {
			auto p = reinterpret_cast<QEnterEvent*>(e)->pos();
			drawCursor(p);
			emitHistogramValue(p);
		}
		else if (e->type() == QEvent::GraphicsSceneLeave) {
			removeCursor();
			emit cursorLeave();// histogramValue(0, 0, 0);
		}
	}

	return false;
}

void HistogramView::mouseMoveEvent(QMouseEvent* e) {

	if (!m_gs->sceneRect().contains(e->pos()))
		return;

	moveCursor(e->pos());
	emitHistogramValue(e->pos());
}

void HistogramView::mouseReleaseEvent(QMouseEvent* e) {

	if (e->button() == Qt::MiddleButton) {

		if (!sceneRect().contains(e->pos()))
			return;

		if (scale() != 1) {
			m_old_scale = m_scale = 1;
			m_offsetx = m_offsety = 0;
			this->showScrollBars();
			this->drawHistogram();
			this->drawCursor(e->position().toPoint());
			this->emitHistogramValue(e->position().toPoint());
		}
	}
}

void HistogramView::resizeEvent(QResizeEvent* e) {

	m_gs->setSceneRect(rect().adjusted(0, 0, -2, -2));

	int dy = (scale() - 1) * (e->size().height() - e->oldSize().height());

	m_offsetx = math::clip(m_offsetx, 0, int((m_gs->width() * scale()) - m_gs->width()));
	m_offsety = math::clip(m_offsety += dy, 0, int((m_gs->height() * scale()) - m_gs->height()));

	m_hsb->resize(width() - m_hsb->thickness() + 1, m_hsb->thickness());
	m_hsb->move(0, height() - m_hsb->thickness());
	m_vsb->resize(m_vsb->thickness(), height() - m_vsb->thickness() + 1);
	m_vsb->move(width() - m_vsb->thickness(), 0);
	m_corner->move(rect().bottomRight() - QPoint(m_corner->width() - 1, m_corner->height() - 1));

	this->showScrollBars();
	this->drawHistogram();
}

void HistogramView::wheelEvent(QWheelEvent* e) {

	int dy = e->angleDelta().y() / 120;

	//if (m_histograms.size() == 0)
		//return;

	if (scale() + dy > 100)
		return;

	else if (scale() + dy < 1)
		return;

	m_scale += dy;

	float x = e->scenePosition().x();
	float y = e->scenePosition().y();
	int w = m_gs->width();
	int h = m_gs->height();

	//int x_old = x * oldScale();
	//int x_new = x * scale();
	m_offsetx += (x / oldScale()) * scale() - w / 2;// (x_new - x_old);


	//int y_old = y * oldScale();
	//int y_new = y * scale();
	m_offsety += ((y / oldScale()) + 8) * scale() - h / 2;// (y_new - y_old);

	m_old_scale = scale();

	m_offsetx = math::clip(m_offsetx, 0, (w * scale()) - w);
	m_offsety = math::clip(m_offsety, 0, (h * scale()) - h);

	if (scale() == 1)
		m_offsetx = m_offsety = 0;

	this->showScrollBars();
	this->drawHistogram();
	this->drawCursor(e->position().toPoint());
	this->emitHistogramValue(e->position().toPoint());
}
