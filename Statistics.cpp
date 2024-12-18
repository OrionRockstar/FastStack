#include "pch.h"
#include "Statistics.h"
#include "FastStack.h"
#include "Maths.h"
#include "ImageGeometry.h"
#include "RGBColorSpace.h"


template<typename T>
Statistics::StatsVector  Statistics::computeStatistics(const Image<T>& img, bool clip) {

	StatsVector  statsvector;
	statsvector.reserve(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch) {
		Statistics stats;
		stats.pixel_count = (clip) ?  img.computeClippedPxCount(ch) : img.pxCount();
		stats.min = img.computeMin(ch, clip);
		stats.max = img.computeMax(ch, clip);
		stats.mean = img.computeMean(ch, clip);
		stats.median = img.computeMedian(ch, clip);
		stats.stdDev = img.computeStdDev(ch, stats.mean, clip);
		stats.avgDev = img.computeAvgDev(ch, T(stats.median), clip);
		stats.MAD = img.computeMAD(ch, T(stats.median), clip);

		statsvector.emplace_back(stats);
	}

	return statsvector;
}
template Statistics::StatsVector  Statistics::computeStatistics(const Image8& img, bool clip);
template Statistics::StatsVector  Statistics::computeStatistics(const Image16& img, bool clip);
template Statistics::StatsVector  Statistics::computeStatistics(const Image32& img, bool clip);

Statistics::StatsVector Statistics::computeStatistics_optimized(const Image16* img, bool clip) {

	StatsVector  statsvector;
	statsvector.reserve(img->channels());

	Histogram hist;

	auto t = GetTimePoint();

	for (int ch = 0; ch < img->channels(); ++ch) {
		Statistics stats;

		//hist.ConstructHistogram(img->begin(ch), img->end(ch));
		stats.pixel_count = (clip) ? img->computeClippedPxCount(ch) : img->pxCount();
		stats.min = hist.min();
		stats.max = hist.max();
		stats.mean = hist.mean();

		stats.median = hist.Median<uint16_t>();

		statsvector.emplace_back(stats);
	}
	DisplayTimeDuration(t);
	return statsvector;
}





StatisticsDialog::StatisticsDialog(const QString& img_name, const Statistics::StatsVector& statsvector, int precision, QWidget* parent) : QDialog(parent) {

	this->setWindowTitle(img_name + " Statistics");
	this->resize(530, 340);

	this->setFocus();
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	this->setAttribute(Qt::WA_DeleteOnClose);

	m_clip_cb = new CheckBox("Clipped", this);
	m_clip_cb->move(15, 15);
	connect(m_clip_cb, &QCheckBox::clicked, this, [this](bool v) { emit clipped(v); });

	m_stats_table = new QTableWidget(8, 1, this);
	m_stats_table->setFocusPolicy(Qt::FocusPolicy::NoFocus);
	m_stats_table->resize(500, 280);
	m_stats_table->move(15, 45);

	m_stats_table->horizontalHeader()->setFixedHeight(35);
	m_stats_table->verticalHeader()->hide();
	m_stats_table->setShowGrid(false);


	m_stats_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_stats_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_stats_table->setSelectionMode(QAbstractItemView::SelectionMode::ContiguousSelection);
	m_stats_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_stats_table->setHorizontalHeaderLabels({ "" });

	QPalette p;
	p.setColor(QPalette::Base, QColor(69, 69, 69));
	p.setColor(QPalette::Button, QColor(69, 69, 69));
	p.setColor(QPalette::ButtonText, Qt::white);
	p.setColor(QPalette::AlternateBase, QColor(78, 37, 117));
	p.setColor(QPalette::Highlight, QColor(0, 169, 255, 39));

	p.setColor(QPalette::Text, Qt::white);
	m_stats_table->setPalette(p);
	m_stats_table->setAlternatingRowColors(true);


	for (int row = 0; row < m_stats_table->rowCount(); ++row) {
		m_stats_table->setRowHeight(row, 30);
		m_stats_table->setCellWidget(row, 0, new Label(m_stat_labels[row]));
	}

	populateStats(statsvector, precision);

	this->show();
}

void StatisticsDialog::populateStats(const Statistics::StatsVector& statsvector, int precision) {

	if (statsvector.size() == 1) {
		m_stats_table->setColumnCount(3);

		m_stats_table->setColumnWidth(0, 100);
		m_stats_table->setColumnWidth(1, 100);
		m_stats_table->setHorizontalHeaderLabels({ "","K","" });
	}

	else if (statsvector.size() == 3) {
		m_stats_table->setColumnCount(4);

		for (int c = 0; c < m_stats_table->columnCount(); ++c)
			m_stats_table->setColumnWidth(c, 100);

		m_stats_table->setHorizontalHeaderLabels({ "","Red","Green","Blue" });
	}

	for (int ch = 0; ch < statsvector.size(); ++ch) {
		int col = ch + 1;

		m_stats_table->setCellWidget(0, col, new Label(QString::number(statsvector[ch].pixel_count)));
		m_stats_table->setCellWidget(1, col, new Label(QString::number(statsvector[ch].mean, 'f', precision)));
		m_stats_table->setCellWidget(2, col, new Label(QString::number(statsvector[ch].median, 'f', precision)));
		m_stats_table->setCellWidget(3, col, new Label(QString::number(statsvector[ch].stdDev, 'f', precision)));
		m_stats_table->setCellWidget(4, col, new Label(QString::number(statsvector[ch].avgDev, 'f', precision)));
		m_stats_table->setCellWidget(5, col, new Label(QString::number(statsvector[ch].MAD, 'f', precision)));
		m_stats_table->setCellWidget(6, col, new Label(QString::number(statsvector[ch].min, 'f', precision)));
		m_stats_table->setCellWidget(7, col, new Label(QString::number(statsvector[ch].max, 'f', precision)));
	}
}





HistogramView::HistogramView(const QSize& size, QWidget* parent) : QGraphicsView(parent) {

	this->resize(size);
	this->setRenderHints(QPainter::Antialiasing);

	addScrollBars();

	m_gs = new QGraphicsScene();
	m_gs->setBackgroundBrush(QColor(19, 19, 19));
	this->setScene(m_gs);

	this->setCursor(Qt::CrossCursor);

	m_gs->setSceneRect(QRect({ 0,0 }, QSize(width()-2,height()-2)));
	m_rect = this->sceneRect().toRect();

	this->drawGrid();

	this->show();
}

void HistogramView::drawGrid() {

	QPen pen = QColor(255, 255, 255);
	pen.setWidthF(0.5);

	for (int y = (50 - int(m_offsety) % 50); y < m_gs->height(); y += 50)
		m_gs->addLine(0, y, m_gs->width(), y, pen);

	for (int x = (100 - int(m_offsetx) % 100); x < m_gs->width(); x += 100)
		m_gs->addLine(x, 0, x, m_gs->height(), pen);
}

template<typename T>
void HistogramView::drawHistogramScene(const Image<T>& img) {

	m_histograms.resize(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch)
		m_histograms[ch].constructHistogram(img, ch);

	this->clearScene();
	this->drawHistogram();
}

template<typename T>
void HistogramView::constructHistogram(const Image<T>& img) {

	m_histograms.resize(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch)
		m_histograms[ch].constructHistogram(img, ch);
}
template void HistogramView::constructHistogram(const Image8&);
template void HistogramView::constructHistogram(const Image16&);
template void HistogramView::constructHistogram(const Image32&);

template<typename T>
void HistogramView::constructHistogram(const Image<T>& img, Resolution resolution) {

	m_histograms.resize(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch) {
		m_histograms[ch].constructHistogram(img, ch);
		m_histograms[ch].resample(uint32_t(resolution));
	}
}
template void HistogramView::constructHistogram(const Image8&, Resolution);
template void HistogramView::constructHistogram(const Image16&, Resolution);
template void HistogramView::constructHistogram(const Image32&, Resolution);

void HistogramView::clearHistogram() {
	clearScene();
	m_histograms.clear();
	m_histograms.shrink_to_fit();
}

void HistogramView::drawHistogram() {

	if (m_histograms.size() == 0)
		return;

	this->clearScene();
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
	for (int ch = channel_start; ch < channel_end; ++ch) {
		if (m_clip)
			peak = math::max(peak, m_histograms[ch].peak(1, m_histograms[ch].resolution() - 2));
		else
			peak = math::max(peak, m_histograms[ch].peak());
	}

	for (int ch = channel_start; ch < channel_end; ++ch) {
		auto& hist = m_histograms[ch];

		QPolygonF line;

		double k = double(m_rect.width() - 1) / (m_histograms[ch].resolution() - 1);
		int start = m_rect.x() / k;
		int end = math::min((int)hist.resolution() - 1, int(m_rect.topRight().x() / k));

		for (int i = start, x0 = 0, y0 = 0; i <= end; ++i) {

			int x = (i * k) - m_rect.x();
			int y = math::max(-1, (int)round((1 - double(hist[i]) / peak) * (m_rect.height() - 1)) - m_rect.y());

			if (i == start || i == end || x != x0)
				line.push_back(QPointF(x0 = x, y0 = y));

			else if (y < y0)
				line.back().setY(y0 = y);

		}

		if (m_clip) {
			line.pop_front();
			line.pop_back();
		}

		QPainterPath path;
		path.addPolygon(line);
		if (m_histograms.size() == 3)
			m_gs->addPath(path, m_pens[ch]);
		else
			m_gs->addPath(path, m_pens[3]);
	}
}


void HistogramView::addScrollBars() {

	m_vsb = new ScrollBar();
	this->setVerticalScrollBar(m_vsb);
	m_vsb->setCursor(Qt::ArrowCursor);

	auto verticalAction = [this](int) {

		m_rect = QRect({m_rect.x(), m_vsb->value()}, m_rect.size());
		m_offsety = m_vsb->sliderPosition();
		m_gs->clear();
		this->drawGrid();
		drawHistogram();
	};

	connect(m_vsb, &QScrollBar::actionTriggered, this, verticalAction);

	m_hsb = new ScrollBar();
	this->setHorizontalScrollBar(m_hsb);
	m_hsb->setCursor(Qt::ArrowCursor);

	auto horizontalAction = [this](int) {
		m_rect = QRect({ m_hsb->value(), m_rect.y()}, m_rect.size());
		m_offsetx = m_hsb->sliderPosition();
		m_gs->clear();
		this->drawGrid();
		drawHistogram();
	};

	connect(m_hsb, &QScrollBar::actionTriggered, this, horizontalAction);

}

void HistogramView::showHorizontalScrollBar() {

	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	int w = m_gs->width();
	int m = (w * m_scale) - w;
	m_hsb->setRange(0, m);
	m_hsb->setValue(math::clip(int(m_offsetx), 0, m));
	m_hsb->setPageStep(w);
}

void HistogramView::hideHorizontalScrollBar() {

	this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void HistogramView::showVerticalScrollBar() {

	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	int h = m_gs->height();
	int m =(h * m_scale) - h;
	m_vsb->setRange(0, m);
	m_vsb->setValue(math::clip(int(m_offsety), 0, m));
	m_vsb->setPageStep(h);
}

void HistogramView::hideVerticalScrollBar() {

	this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void HistogramView::showScrollBars() {

	if (m_scale > 1) {
		showVerticalScrollBar();
		showHorizontalScrollBar();
	}
	else {
		hideVerticalScrollBar();
		hideHorizontalScrollBar();
	}
}

void HistogramView::adjustHistogramRect() {

	QRect r0 = this->sceneRect().toRect();

	int x0 = r0.x() + m_offsetx;
	int y0 = r0.y() + m_offsety;
	int x1 = x0 + m_gs->width() * m_scale;
	int y1 = y0 + m_gs->height() * m_scale;

	m_rect = QRect(QPoint(x0, y0), QPoint(x1, y1));

	if (m_scale != 1)
		m_rect.adjust(0, 0, -verticalScrollBar()->width(), -horizontalScrollBar()->height());
}

void HistogramView::clearScene(bool draw_grid) {
	
	m_gs->clear();

	if (draw_grid)
		this->drawGrid();
}


void HistogramView::resizeEvent(QResizeEvent* e) {

	QGraphicsView::resizeEvent(e);
	m_gs->setSceneRect(QRect({ 0,0 }, QSize(width() - 2, height() - 2)));

	this->showScrollBars();
	this->drawHistogram();
}

void HistogramView::wheelEvent(QWheelEvent* e) {

	int dy = e->angleDelta().y() / 120;

	if (m_histograms.size() == 0)
		return;

	if (m_scale + dy > 200)
		return;

	else if (m_scale + dy < 1)
		return;

	m_scale += dy;

	int x = e->scenePosition().x();
	int y = e->scenePosition().y() + 32;

	if (m_histograms[0].resolution() > m_gs->width() * m_scale) {
		double x_old = x * m_old_scale;
		double x_new = x * m_scale;
		m_offsetx += (x_new - x_old);
	}

	else {
		double x_old = x / m_old_scale;
		double x_new = x / m_scale;
		m_offsetx += (x_old - x_new);
	}

	double y_old = y * m_old_scale;
	double y_new = y * m_scale;

	m_offsety += (y_new - y_old);

	m_offsetx = math::clip(m_offsetx, 0.0, (m_gs->width() * m_scale) - m_gs->width());
	m_offsety = math::clip(m_offsety, 0.0, (m_gs->height() * m_scale) - m_gs->height());

	this->showScrollBars();
	this->clearScene();
	this->drawHistogram();

	m_old_scale = m_scale;
}





template<typename T>
HistogramDialog::HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : QDialog(parent) {

	this->setWindowTitle(img_name + " Histogram");
	this->resize(630, 330);
	this->setMinimumSize(430, 230);

	this->setFocus();
	this->setAttribute(Qt::WA_DeleteOnClose);

	m_hist_view = new HistogramView({ 600,300 }, this);
	m_hist_view->move(15, 15);
	m_hist_view->drawHistogramScene(img);

	this->show();
}
template HistogramDialog::HistogramDialog(const QString&, const Image8&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image16&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image32&, QWidget*);




template<typename T>
Image3DDialog::Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : QDialog(parent) {

	this->setWindowTitle(img_name + "_3D");
	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);


	m_graph = new Q3DSurface();
	m_graph->setSelectionMode(QAbstract3DGraph::SelectionNone);


	std::unique_ptr<Q3DScatter> s = std::make_unique<Q3DScatter>();
	auto surf_cam = m_graph->scene()->activeCamera();
	m_graph->scene()->setActiveCamera(s->scene()->activeCamera());
	s->scene()->setActiveCamera(surf_cam);


	auto activeTheme = m_graph->activeTheme();
	activeTheme->setType(Q3DTheme::ThemeArmyBlue);
	activeTheme->setGridEnabled(false);
	activeTheme->setBackgroundColor(QColor(0, 0, 0, 0));
	activeTheme->setLabelTextColor(QColor(0,0,0,0));
	activeTheme->setLabelBackgroundEnabled(false);


	m_container = QWidget::createWindowContainer(m_graph);
	m_container->setParent(this);


	m_img_proxy = new QSurfaceDataProxy();
	m_img_series = new QSurface3DSeries(m_img_proxy);
	QLinearGradient gr;
	gr.setColorAt(0.0, Qt::black);
	gr.setColorAt(0.25, Qt::blue);
	gr.setColorAt(0.5, Qt::red);
	gr.setColorAt(0.75, Qt::yellow);
	gr.setColorAt(1.0, Qt::white);

	m_img_series->setBaseGradient(gr);
	m_img_series->setDrawMode(QSurface3DSeries::DrawSurface);
	m_img_series->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
	fillImageProxy(img);


	int factor = computeBinFactor(img);
	float w = float(img.cols()) / factor;
	float h = float(img.rows()) / factor;

	m_graph->axisX()->setRange(0, w);
	m_graph->axisY()->setRange(0, 1.0);
	m_graph->axisZ()->setRange(0, h);
	m_graph->setAspectRatio(w/100);

	m_graph->addSeries(m_img_series);
	m_container->resize(w, h);


	auto camera = m_graph->scene()->activeCamera();
	camera->setCameraPreset(Q3DCamera::CameraPresetIsometricRight);
	camera->setZoomLevel(camera->zoomLevel() * 2);

	this->show();
}
template Image3DDialog::Image3DDialog(const QString&, const Image8&, QWidget*);
template Image3DDialog::Image3DDialog(const QString&, const Image16&, QWidget*);
template Image3DDialog::Image3DDialog(const QString&, const Image32&, QWidget*);

template<typename T>
int Image3DDialog::computeBinFactor(const Image<T>& img) {

	QSize size = screen()->availableSize();

	int width = size.width();
	int height = size.height();

	int factor = 1;
	for (; factor < 10; ++factor) {

		int new_cols = img.cols() / factor;
		int new_rows = img.rows() / factor;

		if (new_cols < 0.75 * width && new_rows < 0.75 * height)
			break;
	}

	return factor;
}

template<typename T>
Image<T> Image3DDialog::binImage(const Image<T>& img) {

	int factor = computeBinFactor(img);
	int factor2 = factor * factor;

	Image<T> dst = Image<T>(img.rows() / factor, img.cols() / factor, img.channels());

	for (int ch = 0; ch < dst.channels(); ++ch) {

		for (int y = 0; y < dst.rows(); ++y) {
			int y_s = factor * y;

			for (int x = 0; x < dst.cols(); ++x) {
				int x_s = factor * x;

				float pix = 0;
				for (int j = 0; j < factor; ++j)
					for (int i = 0; i < factor; ++i)
						pix += img(x_s + i, y_s + j, ch);

				dst(x, y, ch) = pix / factor2;

			}
		}
	}

	return dst;
}

template<typename T>
float Image3DDialog::lumValue(const Image<T>& img, const Point<>& p) {

	switch (img.channels()) {
	case 1:
		return Pixel<float>::toType(img(p));

	case 3: {
		float r = Pixel<float>::toType(img(p));
		float g = Pixel<float>::toType(img(p.x(), p.y(), 1));
		float b = Pixel<float>::toType(img(p.x(), p.y(), 2));
		return ColorSpace::CIEL( r, g, b);
	}
	default: {
		float max = 0;
		for (int ch = 0; ch < img.channels(); ++ch)
			max = math::max(max, Pixel<float>::toType(img(p.x(), p.y(), ch)));
		return max;
	}
	}
}

template<typename T>
void Image3DDialog::fillImageProxy(const Image<T>& img) {

	Image<T> temp = binImage(img);
	FastRotation(FastRotation::Type::verticalmirror).apply(temp);

	QSurfaceDataArray* data = new QSurfaceDataArray;
	data->reserve(temp.TotalPxCount());

	for (int y = 0; y < temp.rows(); ++y) {
		QSurfaceDataRow* row = new QSurfaceDataRow(temp.cols());
		for (int x = 0; x < temp.cols(); ++x) {
			(*row)[x].setPosition(QVector3D(x, lumValue(temp, { x,y }), y));
		}
		*data << row;
	}

	m_img_proxy->resetArray(data);
}