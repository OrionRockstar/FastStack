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






template<typename T>
HistogramDialog::HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : Dialog(parent,true) {

	this->setTitle(img_name + " Histogram");
	this->setDefaultSize({ 630, 400 });
	this->setMinimumSize(630,400);
	this->setFocus();

	m_img = reinterpret_cast<const Image8*>(&img);

	m_hist_view = new HistogramView({ 600,300 }, drawArea());
	m_hist_view->move(15, 85);

	m_clip_cb = new CheckBox("Clip", drawArea());
	m_clip_cb->setToolTip("Clip Shadows && Highlights");
	m_clip_cb->move(50, 18);
	m_clip_cb->setChecked(true);
	connect(m_clip_cb, &QCheckBox::clicked, this, [this](bool v) { m_hist_view->clipHistogram(v); });

	QWidget* label_background = new QWidget(drawArea());
	label_background->setGeometry(135, 55, 360, 20);
	label_background->setStyleSheet("QWidget{background: rgb(69,69,69);}");

	m_hist_data = new QLabel(label_background);
	m_hist_data->move(30, 0);
	//m_hist_data->move(165, 53);

	auto setLabel = [this](uint16_t pixelValue_low, uint16_t pixelValue_high, uint32_t pixel_count) {

		//if (pixelValue_low == 0 && pixelValue_high == 0 && pixel_count == 0)
			//return m_hist_data->setText("");

		QString data = "Pixel Value Range : Count   " + QString::number(pixelValue_low) + "-" + QString::number(pixelValue_high) + " : ";
		data += QString::number(pixel_count);
		m_hist_data->setText(data);
		m_hist_data->adjustSize();
	};

	connect(m_hist_view, &HistogramView::histogramValue, this, setLabel);
	connect(m_hist_view, &HistogramView::cursorLeave, this, [this]() { m_hist_data->setText(""); });

	addResolutionCombo();
	addGraphStyleCombo();
	this->show();
}
template HistogramDialog::HistogramDialog(const QString&, const Image8&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image16&, QWidget*);
template HistogramDialog::HistogramDialog(const QString&, const Image32&, QWidget*);

void HistogramDialog::addResolutionCombo() {

	m_resolution_combo = new ComboBox(drawArea());
	m_resolution_combo->addItems({ "8-bit", "10-bit", "12-bit", "14-bit", "16-bit" });
	m_resolution_combo->setFixedWidth(75);
	m_resolution_combo->move(300, 15);
	m_resolution_combo->addLabel(new QLabel("Histogram Resolution:   ", drawArea()));

	for (int i = 0, s = 8; i < m_resolution_combo->count(); ++i, s += 2)
		m_resolution_combo->setItemData(i, 1 << s);

	int index = (m_img->type() == ImageType::UBYTE) ? 0 : 4;
	m_resolution_combo->setCurrentIndex(index);

	auto activation = [this](int index) {
		auto hr = Histogram::Resolution(m_resolution_combo->itemData(index).toInt());
		switch (m_img->type()) {
		case ImageType::UBYTE:
			return m_hist_view->drawHistogramView(*m_img, hr);

		case ImageType::USHORT:
			return m_hist_view->drawHistogramView(*reinterpret_cast<const Image16*>(m_img), hr);

		case ImageType::FLOAT:
			return m_hist_view->drawHistogramView(*reinterpret_cast<const Image32*>(m_img), hr);
		}
	};
	connect(m_resolution_combo, &QComboBox::activated, this, activation);
	activation(index);
}

void HistogramDialog::addGraphStyleCombo() {

	using GS = HistogramView::GraphStyle;
	m_gstyle_combo = new ComboBox(drawArea());
	m_gstyle_combo->setFixedWidth(75);
	m_gstyle_combo->move(500, 15);
	m_gstyle_combo->addLabel(new QLabel("Graph Style:   ", drawArea()));

	m_gstyle_combo->addItem("Line", QVariant::fromValue(GS::line));
	m_gstyle_combo->addItem("Area", QVariant::fromValue(GS::area));
	m_gstyle_combo->addItem("Bars", QVariant::fromValue(GS::bars));
	m_gstyle_combo->addItem("Dots", QVariant::fromValue(GS::dots));

	connect(m_gstyle_combo, &QComboBox::activated, this, [this](int index)  {m_hist_view->setGraphStyle(m_gstyle_combo->itemData(index).value<GS>()); });
}

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
Image3DDialog::Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent) : QWidget(parent) {

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

	m_container = QWidget::createWindowContainer(m_graph, parent, Qt::Tool);
	m_container->setAttribute(Qt::WA_DeleteOnClose);
	connect(m_container, &QWidget::destroyed, this, [this]() { emit windowClosed(); });

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
	m_container->resize(w, h);

	auto camera = m_graph->scene()->activeCamera();
	camera->setCameraPreset(Q3DCamera::CameraPresetIsometricRight);
	camera->setZoomLevel(camera->zoomLevel() * 2);

	m_container->setWindowModality(Qt::WindowModality::WindowModal);
	m_container->show();
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
			max = math::max(max, Pixel<float>::toType(img(p.x, p.y, ch)));
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
