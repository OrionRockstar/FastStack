#include "pch.h"
#include "MorphologicalTransformationDialog.h"
#include "FastStack.h"



MorphologicalKernelScene::MorphologicalKernelScene(MorphologicalTransformation& mt, QRect rect) {

	//this->setBackgroundBrush(QColor(19, 19, 19));
	this->setBackgroundBrush(QColor(96, 96, 96));
	this->setSceneRect(rect);

	m_mt = &mt;

	drawElements();
}

void MorphologicalKernelScene::drawElements() {
	this->clear();

	double el_dim = width() / m_mt->kernelDimension();

	int el = 0;
	for (double y = 0; y < height() - 1; y += el_dim) {
		for (double x = 0; x < width() - 1; x += el_dim, ++el) {
			bool val = m_mt->kernelMaskAt(el);
			addRect(x, y, el_dim, el_dim, m_pens[val], m_brushes[val])->setZValue(el);
		}
	}
	m_items = items(Qt::AscendingOrder);
}

void MorphologicalKernelScene::recolorElements() {

	for (int i = 0; i < m_mt->kernelSize(); ++i) {
		QGraphicsRectItem* rect = reinterpret_cast<QGraphicsRectItem*>(m_items[i]);
		bool val = m_mt->kernelMaskAt(i);
		rect->setBrush(m_brushes[val]);
		rect->setPen(m_pens[val]);
	}

}

void MorphologicalKernelScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {

	if (event->buttons() == Qt::LeftButton) {

		auto item = itemAt(event->scenePos(), QTransform());
		for (int i = 0; i < m_mt->kernelSize(); ++i) {
			if (item == m_items[i]) {
				if (m_mt->kernelMaskAt(i)) {
					m_mt->setKernelMaskAt(i, false);
					QGraphicsRectItem* rect = reinterpret_cast<QGraphicsRectItem*>(item);
					rect->setBrush(m_brushes[false]);
					rect->setPen(m_pens[false]);
				}
				else {
					m_mt->setKernelMaskAt(i, true);
					QGraphicsRectItem* rect = reinterpret_cast<QGraphicsRectItem*>(item);
					rect->setBrush(m_brushes[true]);
					rect->setPen(m_pens[true]);
				}
			}
		}
	}
}





using MT = MorphologicalTransformation;
using MTD = MorphologicalTransformationDialog;

MTD::MorphologicalTransformationDialog(Workspace* parent) : ProcessDialog("MorphologicalTransformation", QSize(310, 460), parent, false) {

	addKernelScene();
	addKernelSizeCombo();

	addKernelPB();
	addFilterSelectionCombo();
	addSelectionInputs();
	addAmountInputs();

	this->show();
}


void MTD::setMask_true() {
	m_mt.setMask_All(true);
	m_mks->recolorElements();
}

void MTD::setMask_false() {
	m_mt.setMask_All(false);
	m_mks->recolorElements();
}

void MTD::setMask_Circular() {
	m_mt.setMask_Circular();
	m_mks->recolorElements();
}

void MTD::setMask_Diamond() {
	m_mt.setMask_Diamond();
	m_mks->recolorElements();
}

void MTD::invertMask() {
	m_mt.invertMask();
	m_mks->recolorElements();
}

void MTD::rotateMask() {
	m_mt.rotateMask();
	m_mks->recolorElements();
}


void MTD::addKernelPB() {

	QSize size = QSize(25, 25);
	int x = 15, dx = 30, dy = 310;

	PushButton* all_true = new PushButton("1", drawArea());
	all_true->resize(size);
	all_true->move(x, dy);
	all_true->setToolTip("Set all elements to true");
	connect(all_true, &QPushButton::pressed, this, &MTD::setMask_true);

	PushButton* all_false = new PushButton("0", drawArea());
	all_false->resize(size);
	all_false->move(x += dx, dy);
	all_false->setToolTip("Set all elements to false");
	connect(all_false, &QPushButton::pressed, this, &MTD::setMask_false);


	PushButton* circular = new PushButton("C", drawArea());
	//circular->setIcon(Qt::SP_)
	circular->resize(size);
	circular->move(x += dx, dy);
	circular->setToolTip("Circular Kernel");
	connect(circular, &QPushButton::pressed, this, &MTD::setMask_Circular);


	x = 15;
	dy += 35;
	PushButton* diamond = new PushButton("D", drawArea());
	diamond->resize(size);
	diamond->move(x, dy);
	diamond->setToolTip("Diamond Kernel");
	connect(diamond, &QPushButton::pressed, this, &MTD::setMask_Diamond);


	PushButton* invert = new PushButton("I", drawArea());
	invert->resize(size);
	invert->move(x += dx, dy);
	invert->setToolTip("Invert all elements");
	connect(invert, &QPushButton::pressed, this, &MTD::invertMask);

	PushButton* rotate = new PushButton("R", drawArea());
	rotate->resize(size);
	rotate->move(x += dx, dy);
	rotate->setToolTip("Rotate kernel 90\u00B0 clock-wise");
	connect(rotate, &QPushButton::pressed, this, &MTD::rotateMask);
}

void MTD::addKernelScene() {

	m_mks = new MorphologicalKernelScene(m_mt, QRect(0, 0, 280, 280));

	m_gv = new QGraphicsView(m_mks, drawArea());
	m_gv->setRenderHints(QPainter::Antialiasing);
	m_gv->setGeometry(15, 15, 280, 280);
	m_gv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_gv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void MTD::addKernelSizeCombo() {

	m_kerenl_size_cb = new ComboBox(drawArea());

	for (int i = 3; i <= 25; i += 2) {

		QString d = QString::number(i);
		QString t = QString::number(i * i);
		m_kerenl_size_cb->addItem(d + "x" + d + " (" + t + " elements)");
	}
	//kernel dim = 3 + index * 2
	m_kerenl_size_cb->resize(175, 25);
	m_kerenl_size_cb->move(120, 310);

	auto activate = [this](int index) {
		m_mt.resizeKernel(3 + 2 * index);
		m_mks->drawElements();
	};

	connect(m_kerenl_size_cb, &QComboBox::activated, this, activate);
}

void MTD::addFilterSelectionCombo() {

	m_filter_cb = new ComboBox(drawArea());

	m_filter_cb->addItem("Erosion");
	m_filter_cb->addItem("Dialation");
	m_filter_cb->addItem("Opening");
	m_filter_cb->addItem("Closing");
	m_filter_cb->addItem("Selection");
	m_filter_cb->addItem("Median");
	m_filter_cb->addItem("Midpoint");

	m_filter_cb->resize(175, 25);
	m_filter_cb->move(120, 345);

	auto activate = [this](int index) {

		if (MT::Type(index) == MT::Type::selection) {
			m_selection_le->setEnabled(true);
			m_selection_slider->setEnabled(true);
		}
		else {
			m_selection_le->setDisabled(true);
			m_selection_slider->setDisabled(true);
		}

		m_mt.setMorphologicalFilter(MT::Type(index));
	};

	connect(m_filter_cb, &QComboBox::activated, this, activate);
}

void MTD::addSelectionInputs() {

	m_selection_le = new DoubleLineEdit(new DoubleValidator(0.00, 1.00, 2, this), drawArea());
	m_selection_le->setValue(m_mt.selectionPoint());
	m_selection_le->resize(50, 25);
	m_selection_le->move(80, 385);
	m_selection_le->setDisabled(true);
	addLabel(m_selection_le, new QLabel("Selection:", drawArea()), 2);

	m_selection_slider = new Slider(drawArea());
	m_selection_le->addSlider(m_selection_slider);

	m_selection_slider->setRange(0, 100);
	m_selection_slider->setValue(m_mt.selectionPoint() * m_selection_slider->maximum());
	m_selection_slider->setFixedWidth(150);
	m_selection_slider->setDisabled(true);

	QPalette p;
	p.setColor(QPalette::Disabled, QPalette::ColorRole::Highlight, Qt::gray);
	m_selection_slider->setPalette(p);

	auto action = [this](int) {

		float sel = m_selection_slider->sliderPosition() / 100.0f;
		m_selection_le->setText(QString::number(sel, 'f', 2));
		m_mt.setSelectionPoint(sel);
	};

	auto edited = [this]() {

		float val = m_selection_le->text().toFloat();
		m_selection_slider->setValue(val * 100);
		m_mt.setSelectionPoint(val);
	};

	connect(m_selection_slider, &QSlider::actionTriggered, this, action);
	connect(m_selection_le, &QLineEdit::editingFinished, this, edited);
}

void MTD::addAmountInputs() {

	m_amount_le = new DoubleLineEdit(new DoubleValidator(0.00, 1.00, 2, this), drawArea());
	m_amount_le->setValue(m_mt.blendAmount());
	m_amount_le->resize(50, 25);
	m_amount_le->move(80, 420);
	addLabel(m_amount_le, new QLabel("Amount:", drawArea()), 2);
	const QString tt = "Blend amount between original pixel and new pixel value.";
	m_amount_le->setToolTip(tt);

	m_amount_slider = new Slider(drawArea());
	m_amount_le->addSlider(m_amount_slider);
	m_amount_slider->setRange(0, 100);
	m_amount_slider->setValue(100);
	m_amount_slider->setFixedWidth(150);

	auto action = [this](int) {

		float am = m_amount_slider->sliderPosition() / 100.0;
		m_amount_le->setText(QString::number(am, 'f', 2));
		m_mt.setBlendAmount(am);
	};

	auto edited = [this]() {

		float val = m_amount_le->text().toFloat();
		m_amount_slider->setValue(val * 100);
		m_mt.setBlendAmount(val);
	};

	connect(m_amount_slider, &QSlider::valueChanged, this, action);
	connect(m_amount_le, &QLineEdit::editingFinished, this, edited);
}


void MTD::resetDialog() {

	m_mt = MorphologicalTransformation();
	m_kerenl_size_cb->setCurrentIndex(0);
	m_mks->drawElements();

	m_selection_le->setValue(m_mt.selectionPoint());
	m_selection_slider->setSliderPosition(m_mt.selectionPoint() * m_selection_slider->maximum());

	m_amount_le->setValue(m_mt.blendAmount());
	m_amount_slider->setSliderPosition(m_mt.blendAmount() * m_selection_slider->maximum());
}

void MTD::apply() {

	if (!workspace()->hasSubWindows())
		return;

	auto iwptr = imageRecast(workspace()->currentSubWindow()->widget());

	std::unique_ptr<ProgressDialog> pd;
	if (m_mt.kernelDimension() >= 9 || m_mt.morphologicalFilter() == MT::Type::selection)
		pd = std::unique_ptr<ProgressDialog>(new ProgressDialog(m_mt.progressSignal()));

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		return iwptr->applyToSource(m_mt, &MT::apply);
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		return iw16->applyToSource(m_mt, &MT::apply);
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		return iw32->applyToSource(m_mt, &MT::apply);
	}
	}
}
