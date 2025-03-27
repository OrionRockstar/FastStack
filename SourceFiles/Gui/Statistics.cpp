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
		stats.type = img.type();
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

Statistics Statistics::toBitDepth(BitDepth bitdepth)const {

	Statistics s;

	s.pixel_count = pixel_count;

	switch (type) {
	case ImageType::UBYTE:
		normalizedFromType<uint8_t>(s);
		break;
	case ImageType::USHORT:
		normalizedFromType<uint16_t>(s);
		break;
	case ImageType::FLOAT:
		normalizedFromType<float>(s);
		break;
	}

	s *= bitdepth;

	return s;
}





StatisticsDialog::StatisticsDialog(const QString& img_name, const Statistics::StatsVector& statsvector, int precision, QWidget* parent) : m_stats_vector(&statsvector), Dialog(parent) {

	this->setTitle(img_name + " Statistics");
	this->resizeDialog(530, 340);

	this->setFocus();

	m_clip_cb = new CheckBox("Clipped", drawArea());
	m_clip_cb->move(440, 10);
	connect(m_clip_cb, &QCheckBox::clicked, this, [this](bool v) { emit clipped(v); });

	addStatsTable();
	addBitDepthCombo();

	this->show();
}

void StatisticsDialog::updateStats(const Statistics::StatsVector& statsvector) {

	m_stats_vector = &statsvector;

	if (statsvector.size() == 1 && m_stats_table->columnCount() != 3) {
		m_stats_table->setColumnCount(3);

		m_stats_table->setColumnWidth(0, 100);
		m_stats_table->setColumnWidth(1, 100);
		m_stats_table->setHorizontalHeaderLabels({ "","K","" });
	}

	else if (statsvector.size() == 3 && m_stats_table->columnCount() != 4) {
		m_stats_table->setColumnCount(4);

		for (int c = 0; c < m_stats_table->columnCount(); ++c)
			m_stats_table->setColumnWidth(c, 100);

		m_stats_table->setHorizontalHeaderLabels({ "","Red","Green","Blue" });
	}


	Statistics::BitDepth bitdepth = m_bit_depth_combo->currentData().value<Statistics::BitDepth>();
	int precision = (bitdepth == Statistics::BitDepth::_float) ? 7 : 1;

	for (int ch = 0; ch < (*m_stats_vector).size(); ++ch) {
		int col = ch + 1;

		Statistics s = (*m_stats_vector)[ch].toBitDepth(bitdepth);

		m_stats_table->setCellWidget(0, col, new Label(QString::number(s.pixel_count)));
		m_stats_table->setCellWidget(1, col, new Label(QString::number(s.mean, 'f', precision)));
		m_stats_table->setCellWidget(2, col, new Label(QString::number(s.median, 'f', precision)));
		m_stats_table->setCellWidget(3, col, new Label(QString::number(s.stdDev, 'f', precision)));
		m_stats_table->setCellWidget(4, col, new Label(QString::number(s.avgDev, 'f', precision)));
		m_stats_table->setCellWidget(5, col, new Label(QString::number(s.MAD, 'f', precision)));
		m_stats_table->setCellWidget(6, col, new Label(QString::number(s.min, 'f', precision)));
		m_stats_table->setCellWidget(7, col, new Label(QString::number(s.max, 'f', precision)));
	}
}

void StatisticsDialog::addStatsTable() {

	m_stats_table = new QTableWidget(8, 1, drawArea());
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
}

void StatisticsDialog::addBitDepthCombo() {

	using BitDepth = Statistics::BitDepth;

	m_bit_depth_combo = new ComboBox(drawArea());
	m_bit_depth_combo->move(15, 10);

	QFont f = QFont("Serif");
	f.setStyleHint(QFont::Monospace);
	m_bit_depth_combo->setFont(f);

	m_bit_depth_combo->addItem("Normalized   [0.0...1.0]", QVariant::fromValue<BitDepth>(BitDepth::_float));
	m_bit_depth_combo->addItem("8-bit        [0.....255]", QVariant::fromValue<BitDepth>(BitDepth::_8bit));
	m_bit_depth_combo->addItem("10-bit       [0....1023]", QVariant::fromValue<BitDepth>(BitDepth::_10bit));
	m_bit_depth_combo->addItem("12-bit       [0....4095]", QVariant::fromValue<BitDepth>(BitDepth::_12bit));
	m_bit_depth_combo->addItem("14-bit       [0...16383]", QVariant::fromValue<BitDepth>(BitDepth::_14bit));
	m_bit_depth_combo->addItem("16-bit       [0...65535]", QVariant::fromValue<BitDepth>(BitDepth::_16bit));

	connect(m_bit_depth_combo, &QComboBox::currentIndexChanged, this, [this](int) { this->updateStats(*m_stats_vector); });

	switch ((*m_stats_vector)[0].type) {
	case ImageType::UBYTE: {
		m_bit_depth_combo->currentIndexChanged(1);
		break;
	}
	case ImageType::USHORT: {
		m_bit_depth_combo->currentIndexChanged(2);
		break;
	}
	case ImageType::FLOAT: {
		m_bit_depth_combo->currentIndexChanged(0);
		break;
	}
	}
}






HistogramView::HistogramView(const QSize& size, QWidget* parent) : QGraphicsView(parent) {

	this->resize(size);
	this->setRenderHints(QPainter::Antialiasing);
	this->setMouseTracking(true);
	this->installEventFilter(this);

	addScrollBars();

	m_gs = new QGraphicsScene();
	m_gs->setBackgroundBrush(QColor(19, 19, 19));
	
	this->setScene(m_gs);

	this->setCursor(Qt::BlankCursor);

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
void HistogramView::constructHistogram(const Image<T>& img, Histogram::Resolution resolution) {

	m_histograms.resize(img.channels());

	for (int ch = 0; ch < img.channels(); ++ch) {
		m_histograms[ch].constructHistogram(img, ch);
		m_histograms[ch].resample(resolution);
	}
}
template void HistogramView::constructHistogram(const Image8&, Histogram::Resolution);
template void HistogramView::constructHistogram(const Image16&, Histogram::Resolution);
template void HistogramView::constructHistogram(const Image32&, Histogram::Resolution);

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

	m_cursor_lines.fill(nullptr);

	if (draw_grid)
		this->drawGrid();
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
	int y = e->scenePosition().y();

	double x_old = x * m_old_scale;
	double x_new = x * m_scale;
	m_offsetx += (x_new - x_old);

	double y_old = y * m_old_scale;
	double y_new = y * m_scale;
	m_offsety += (y_new - y_old);

	m_offsetx = math::clip(m_offsetx, 0.0, (m_gs->width() * m_scale) - m_gs->width());
	m_offsety = math::clip(m_offsety, 0.0, (m_gs->height() * m_scale) - m_gs->height());

	this->showScrollBars();
	this->drawHistogram();
	this->drawCursor(e->position().toPoint());

	m_old_scale = m_scale;
}

bool HistogramView::eventFilter(QObject* obj, QEvent* e) {

	if (obj == this && e->type() == QEvent::Enter)
		mouseMoveEvent((QMouseEvent*)e);
	
	if (obj == this && e->type() == QEvent::Leave) {
		emit histogramValue(0, 0, 0);
		removeCursor();
	}

	if ((obj == horizontalScrollBar() || obj == verticalScrollBar()) && e->type() == QEvent::Enter)
		removeCursor();

	return false;
}

void HistogramView::mouseMoveEvent(QMouseEvent* e) {

	drawCursor(e->pos());

	if (m_histograms.size() > 0) {
		double k = double(m_rect.width() - 1) / (m_histograms[0].resolution() - 1);

		uint16_t low = (e->pos().x() + m_rect.x()) / k;
		uint16_t high = (e->pos().x() + 1 + m_rect.x()) / k;

		if (m_clip) {
			if (low == 0)
				low++;
			if (high == (m_histograms[0].resolution() - 1))
				high--;
		}

		uint32_t pixel_count = 0;
		for (int ch = 0; ch < m_histograms.size(); ++ch) {
			Histogram& hist = m_histograms[ch];
			for (uint32_t i = low; i <= high; ++i)
				pixel_count += hist[i];
		}

		emit histogramValue(low, high, pixel_count);
	}
}




template<typename T>
HistogramDialog::HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : Dialog(parent,true) {

	this->setTitle(img_name + " Histogram");
	this->setDefaultSize({ 630, 365 });
	this->setMinimumSize(400,265);
	

	this->setFocus();
	//this->setAttribute(Qt::WA_DeleteOnClose);

	m_hist_view = new HistogramView({ 600,300 }, drawArea());
	m_hist_view->drawHistogramScene(img);
	m_hist_view->move(15, 50);

	m_clip_cb = new CheckBox("Clip Shadows && Highlights", drawArea());
	m_clip_cb->move(15, 15);
	m_clip_cb->setChecked(true);
	connect(m_clip_cb, &QCheckBox::clicked, this, [this](bool v) { m_hist_view->clipHistogram(v); });

	m_hist_data = new QLabel(drawArea());
	m_hist_data->move(275, 18);

	auto setLabel = [this](uint16_t pixelValue_low, uint16_t pixelValue_high, uint32_t pixel_count) {

		if (pixelValue_low == 0 && pixelValue_high == 0 && pixel_count == 0)
			return m_hist_data->setText("");

		QString data = "Pixel Value Range : Count   " + QString::number(pixelValue_low) + "-" + QString::number(pixelValue_high) + " : ";
		data += QString::number(pixel_count);
		m_hist_data->setText(data);
		m_hist_data->adjustSize();
	};

	connect(m_hist_view, &HistogramView::histogramValue, this, setLabel);

	this->show();
}
template HistogramDialog::HistogramDialog(const QString&, const Image8&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image16&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image32&, QWidget*);

void HistogramDialog::resizeEvent(QResizeEvent* e) {

	Dialog::resizeEvent(e);

	if (m_resizing) {
		QSize ds = size() - e->oldSize();
		if (e->oldSize().width() != -1)
			m_hist_view->resize(m_hist_view->size() + ds);
	}
}


SideWidget::SideWidget(QWidget* parent) : QWidget(parent) {

	this->setMinimumWidth(25);
	QPalette p;
	p.setBrush(QPalette::Window, QColor(69, 69, 69));
	this->setPalette(p);
	this->setAutoFillBackground(true);

	this->setWindowFlags(Qt::WindowStaysOnTopHint);
	this->raise();


	m_resize_pb = new PushButton(style()->standardIcon(QStyle::SP_MediaSeekForward),"", this);
	m_resize_pb->setGeometry(5, 5, 15, 15);

	connect(m_resize_pb, &PushButton::released, this, &SideWidget::onPress);

	this->resize(m_collapsed_width, parentWidget()->height());
}

void SideWidget::onPress() {

	if (m_state == State::collapsed) {
		m_resize_pb->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
		this->resize(m_expanded_width, height());
		for (auto child : children())
			if (child != m_resize_pb)
				reinterpret_cast<QWidget*>(child)->setVisible(true);

		m_state = State::expanded;
	}

	else if (m_state == State::expanded) {
		m_resize_pb->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
		this->resize(m_collapsed_width, height());
		for (auto child : children())
			if (child != m_resize_pb)
				reinterpret_cast<QWidget*>(child)->setVisible(false);

		m_state = State::collapsed;
	}

	emit sizeChanged();
}




template<typename T>
Image3DDialog::Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : Dialog(parent) {

	this->setTitle(img_name + "_3D");
	m_default_opacity = 1.0;

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
	activeTheme->setWindowColor(this->palette().color(QPalette::Window));

	m_container = QWidget::createWindowContainer(m_graph, drawArea());

	m_img_proxy = new QSurfaceDataProxy();
	m_img_series = new QSurface3DSeries(m_img_proxy);
	
	m_img_series->setBaseGradient(m_gradient);
	m_img_series->setDrawMode(QSurface3DSeries::DrawSurface);
	m_img_series->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
	fillImageProxy(img);


	int factor = computeBinFactor(img);
	float w = m_width = float(img.cols()) / factor;
	float h = float(img.rows()) / factor;

	m_graph->axisX()->setRange(0, w);
	m_graph->axisY()->setRange(0, 1.0);
	m_graph->axisZ()->setRange(0, h);
	m_graph->setAspectRatio(w/100);

	m_graph->addSeries(m_img_series);
	this->resizeDialog(w, h);
	m_container->resize(w, h);

	auto camera = m_graph->scene()->activeCamera();
	camera->setCameraPreset(Q3DCamera::CameraPresetIsometricRight);
	camera->setZoomLevel(camera->zoomLevel() * 2);

	this->show();
}
template Image3DDialog::Image3DDialog(const QString&, const Image8&, QWidget*);
template Image3DDialog::Image3DDialog(const QString&, const Image16&, QWidget*);
template Image3DDialog::Image3DDialog(const QString&, const Image32&, QWidget*);


QLinearGradient Image3DDialog::defaultGradient() {

	QLinearGradient gr;
	gr.setColorAt(0.0, Qt::black);
	gr.setColorAt(0.25, Qt::blue);
	gr.setColorAt(0.5, Qt::red);
	gr.setColorAt(0.75, Qt::yellow);
	gr.setColorAt(1.0, Qt::white);

	return gr;
}

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
float Image3DDialog::lumValue(const Image<T>& img, const Point& p) {

	switch (img.channels()) {
	case 1:
		return Pixel<float>::toType(img(p));

	case 3: 
		return ColorSpace::CIEL(img.color<double>(p));
	
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
	data->reserve(temp.totalPxCount());

	for (int y = 0; y < temp.rows(); ++y) {
		QSurfaceDataRow* row = new QSurfaceDataRow(temp.cols());
		for (int x = 0; x < temp.cols(); ++x) {
			(*row)[x].setPosition(QVector3D(x, lumValue(temp, { x,y }), y));
		}
		*data << row;
	}

	m_img_proxy->resetArray(data);
}
