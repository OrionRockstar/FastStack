#include "pch.h"
#include "Statistics.h"
#include "FastStack.h"
#include "Maths.h"
#include "RGBColorSpace.h"

PSFTable::PSFTable(QWidget* parent) : QTableWidget(0, m_columns, parent) {

	this->setStyleSheet("QTableWidget::item { border-bottom: 1px solid rgba(69,0,169,169); }");

	this->setShowGrid(false);
	this->setSelectionBehavior(QAbstractItemView::SelectRows);
	this->setSelectionMode(QAbstractItemView::SingleSelection);
	this->setFocusPolicy(Qt::NoFocus);
	this->horizontalHeader()->setHighlightSections(false);
	this->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);

	this->setHorizontalHeaderLabels({ "B","A","xc","yc","\u03C3x","\u03C3y","FWHMx","FWHMy","Flux","Mag.","r","theta","RMSE" });
	for (int c = 0; c < m_columns; ++c)
		this->setColumnWidth(c, 75);

	this->setVerticalScrollBar(new ScrollBar(this));
	this->setHorizontalScrollBar(new ScrollBar(this));
	this->setHorizontalScrollMode(QAbstractItemView::ScrollMode::ScrollPerPixel);
	this->verticalHeader()->setVisible(false);

	connect(this, &QTableWidget::cellActivated, this, [this](int row, int col) {
		if (rowCount() == 0)
			emit psfSelected(nullptr);
		emit psfSelected(selectedItems()[0]->data(Qt::UserRole).value<const PSF*>()); });
}

void PSFTable::selectPSF(const PSF* psf) {

	for (int row = 0; row < rowCount(); ++row)
		if (item(row, 0)->data(Qt::UserRole).value<const PSF*>() == psf)
			return selectRow(row);
}

void PSFTable::addRow(const PSF& psf) {

	int row = this->rowCount();
	this->insertRow(row);
	this->setRowHeight(row, 30);

	setItem(row, 0, new QTableWidgetItem(QString::number(psf.B, 'f')));
	item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(&psf));
	setItem(row, 1, new QTableWidgetItem(QString::number(psf.A, 'f')));
	setItem(row, 2, new QTableWidgetItem(QString::number(psf.xc, 'f', 2)));
	setItem(row, 3, new QTableWidgetItem(QString::number(psf.yc, 'f', 2)));
	setItem(row, 4, new QTableWidgetItem(QString::number(psf.sx, 'f', 2)));
	setItem(row, 5, new QTableWidgetItem(QString::number(psf.sy, 'f', 2)));
	setItem(row, 6, new QTableWidgetItem(QString::number(psf.fwhmx, 'f', 2) + "px"));
	setItem(row, 7, new QTableWidgetItem(QString::number(psf.fwhmy, 'f', 2) + "px"));
	setItem(row, 8, new QTableWidgetItem(QString::number(psf.flux)));
	setItem(row, 9, new QTableWidgetItem(QString::number(psf.magnitude, 'f', 2)));
	setItem(row, 10, new QTableWidgetItem(QString::number(psf.roundness, 'f')));
	setItem(row, 11, new QTableWidgetItem(QString::number(math::radiansToDegrees(psf.theta), 'f', 2)));
	setItem(row, 12, new QTableWidgetItem(QString::number(psf.rmse,'f')));

	for (int i = 0; i < m_columns; ++i)
		this->item(row, i)->setTextAlignment(Qt::AlignCenter);
}

bool PSFTable::event(QEvent* e) {

	if (e->type() == QEvent::ContextMenu)
		sortItems(1, Qt::DescendingOrder);

	return QTableWidget::event(e);
}

void PSFTable::mousePressEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton)
		QTableWidget::mousePressEvent(e);
}

void PSFTable::mouseReleaseEvent(QMouseEvent* e) {

	if (e->button() == Qt::LeftButton) {
		const int row = rowAt(e->pos().y());
		if (row != -1)
			emit cellActivated(row, 0);
	}
}





PSFUtilityDialog::PSFUtilityDialog(QWidget* parent) : Dialog(parent) {

	this->setTitle("PSF Utilities");
	this->resizeDialog(630, 570);
	this->setFocus();


	m_psf_table = new PSFTable(drawArea());
	m_psf_table->setGeometry(15, 15, 600, 400);
	connect(m_psf_table, &PSFTable::psfSelected, [this](const PSF* psf) { emit psfSelected(psf); });

	addStarDetctionInputs();
	addButtons();

	this->show();
}

template<typename T>
PSFUtilityDialog::PSFUtilityDialog(const Image<T>& img, const QString& name, QWidget* parent) : Dialog(parent) {

	m_img = reinterpret_cast<const Image8*>(&img);
	this->setTitle(name + " PSF Utilities");
	this->resizeDialog(630, 600);
	this->setFocus();

	m_stars_detected = new QLabel("Stars Detected: ", drawArea());
	m_stars_detected->move(105, 15);
	//m_stars_detected->setHidden(true);

	m_average_FWHM = new QLabel("Average FWHM: ", drawArea());
	m_average_FWHM->move(405, 16);

	m_psf_table = new PSFTable(drawArea());
	m_psf_table->setGeometry(15, 45, 600, 400);
	connect(m_psf_table, &PSFTable::psfSelected, [this](const PSF* psf) { emit psfSelected(psf); });

	addStarDetctionInputs();
	addButtons();


	this->show();
}
template PSFUtilityDialog::PSFUtilityDialog(const Image8& img, const QString& name, QWidget* parent);
template PSFUtilityDialog::PSFUtilityDialog(const Image16& img, const QString& name, QWidget* parent);
template PSFUtilityDialog::PSFUtilityDialog(const Image32& img, const QString& name, QWidget* parent);

void PSFUtilityDialog::selectPSF(const PSF* psf) {

	m_psf_table->selectPSF(psf);
}

void PSFUtilityDialog::addStarDetctionInputs() {

	QString txt = "Sets K value of star threshold, defined as median + K * avg deviation.";

	m_sigmaK_input = new DoubleInput("Star signal threshold:   ", m_sd.K(), new DoubleValidator(0.1, 5.0, 2), drawArea(), 20);
	m_sigmaK_input->move(225, 460);
	m_sigmaK_input->setSliderWidth(200);
	m_sigmaK_input->setToolTip(txt);

	auto funca = [this]() { m_sd.setK(m_sigmaK_input->valuef()); };
	connect(m_sigmaK_input, &InputBase::actionTriggered, this, funca);
	connect(m_sigmaK_input, &InputBase::editingFinished, this, funca);



	m_roundness_input = new DoubleInput("Roundness threshold:   ", m_sd.roundness(), new DoubleValidator(0.1, 1.0, 2), drawArea(), 100);
	m_roundness_input->move(225, 505);
	m_roundness_input->setSliderWidth(200);

	auto funcb = [this]() { m_sd.setRoundness(m_roundness_input->valuef()); };
	connect(m_roundness_input, &InputBase::actionTriggered, this, funcb);
	connect(m_roundness_input, &InputBase::editingFinished, this, funcb);



	m_psf_combo = new ComboBox(drawArea());
	m_psf_combo->move(200, 550);
	addLabel(m_psf_combo, new QLabel("PSF:"));
	m_psf_combo->addItem("Gaussian", QVariant::fromValue(PSF::Type::gaussian));
	m_psf_combo->addItem("Moffat", QVariant::fromValue(PSF::Type::moffat));

	m_beta_sb = new DoubleSpinBox(m_sd.beta(), 0.0, 10.0, 1, drawArea());
	m_beta_sb->move(385, 550);
	addLabel(m_beta_sb, new QLabel("Beta:"));
	m_beta_sb->setSingleStep(0.1);

	auto activate = [this]() {

		auto psf_t = m_psf_combo->currentData().value<PSF::Type>();
		m_sd.setPSF(psf_t);

		if (psf_t == PSF::Type::moffat)
			m_beta_sb->setEnabled(true);
		else
			m_beta_sb->setDisabled(true);
	};

	connect(m_psf_combo, &QComboBox::activated, this, activate);
	activate();
	connect(m_beta_sb, &QDoubleSpinBox::valueChanged, this, [this](double val) { m_sd.setBeta(val); });
}

void PSFUtilityDialog::addButtons() {

	const int bs = 40;

	m_run_psf_pb = new PushButton(QIcon("./Icons//star-icon2.png"), "", drawArea());
	m_run_psf_pb->setIconSize({30,30});
	m_run_psf_pb->setGeometry(15, 545, bs, bs);

	connect(m_run_psf_pb, &PushButton::released, this, &PSFUtilityDialog::runPSFDetection);

	m_clear_pb = new PushButton("Clear", drawArea());
	m_clear_pb->setGeometry(520, 545, bs, bs);

	connect(m_clear_pb, &PushButton::released, this, [this]() {
		m_psf_vector = PSFVector();
		m_psf_table->setRowCount(0);
		m_psf_table->setSortingEnabled(false);
		m_stars_detected->setText(m_stars_detected->text().remove(QRegularExpression("[0-9]*")));
		m_average_FWHM->setText(m_average_FWHM->text().remove(QRegularExpression("[0-9]*\\.[0-9]*")));
		emit psfCleared(); });

	m_reset_pb = new PushButton("Reset", drawArea());
	m_reset_pb->setGeometry(575, 545, bs, bs);
	
	connect(m_reset_pb, &PushButton::released, this, [this]() {
		m_sd = StarDetector();
		m_sigmaK_input->reset();
		m_roundness_input->reset();
		m_psf_combo->setCurrentIndex(int(m_sd.psf()));
		m_beta_sb->setValue(m_sd.beta());
		m_beta_sb->setEnabled(false); });
}

void PSFUtilityDialog::runPSFDetection() {

	if (m_img == nullptr)
		return;

	if (m_psf_vector.size() != 0)
		return;

	switch (m_img->type()) {
	case ImageType::UBYTE:
		m_psf_vector = m_sd.DAOFIND_PSF(*m_img);
		break;
	case ImageType::USHORT:
		m_psf_vector = m_sd.DAOFIND_PSF(*reinterpret_cast<const Image<uint16_t>*>(m_img));
		break;
	case ImageType::FLOAT:
		m_psf_vector = m_sd.DAOFIND_PSF(*reinterpret_cast<const Image<float>*>(m_img));
		break;
	}

	double avgx = 0;
	double avgy = 0;

	for (const PSF& psf : m_psf_vector) {
		m_psf_table->addRow(psf);
		avgx += psf.fwhmx;
		avgy += psf.fwhmy;
	}

	avgx /= m_psf_vector.size();
	avgy /= m_psf_vector.size();

	m_stars_detected->setText(m_stars_detected->text() + QString::number(m_psf_vector.size()));
	m_stars_detected->adjustSize();

	m_average_FWHM->setText(m_average_FWHM->text() + QString::number((avgx + avgy) / 2.0, 'f', 2));
	m_average_FWHM->adjustSize();

	m_psf_table->setSortingEnabled(true);
	m_psf_table->sortItems(1, Qt::DescendingOrder);
	emit onDetection(&m_psf_vector);
}

void PSFUtilityDialog::closeEvent(QCloseEvent* e) {

	emit psfCleared();
	Dialog::closeEvent(e);
}





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
