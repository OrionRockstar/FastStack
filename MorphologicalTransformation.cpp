#include "pch.h"
#include"FastStack.h"
#include "MorphologicalTransformation.h"

using MT = MorphologicalTransformation;

MT::MorphologicalTransformation(int kernel_dimension) : m_kernel_dim(kernel_dimension) {
	if (m_kernel_dim % 2 == 0)
		m_kernel_dim -= 1;

	m_kernel_radius = (m_kernel_dim - 1) / 2;
	m_kernel_size = kernel_dimension * kernel_dimension;
	m_kmask.resize(m_kernel_size, true);
}

void MT::resizeKernel(int new_dim) {
	int old_dim = m_kernel_dim;
	m_kernel_dim = new_dim;
	m_kernel_radius = (m_kernel_dim - 1) / 2;
	m_kernel_size = m_kernel_dim * m_kernel_dim;

	std::vector<char> new_mask(m_kernel_size, false);

	int d_dim = (m_kernel_dim - old_dim) / 2;

	if (d_dim > 0)
		for (int y_n = d_dim, y = 0; y < old_dim; ++y, ++y_n)
			for (int x_n = d_dim, x = 0; x < old_dim; ++x, ++x_n)
				new_mask[y_n * m_kernel_dim + x_n] = m_kmask[y * old_dim + x];

	else {
		d_dim *= -1;
		for (int y_n = 0, y = d_dim; y_n < m_kernel_dim; ++y, ++y_n)
			for (int x_n = 0, x = d_dim; x_n < m_kernel_dim; ++x, ++x_n)
				new_mask[y_n * m_kernel_dim + x_n] = m_kmask[y * old_dim + x];
	}

	m_kmask = new_mask;
}

void MT::setMask_All(bool value) {
	for (char& val : m_kmask)
		val = value;
}

void MT::setMask_Circular() {

	for (char& value : m_kmask)
		value = false;

	for (int j = 0; j < m_kernel_dim; ++j) {
		for (int i = 0; i < m_kernel_dim; ++i) {
			int dx = i - m_kernel_radius;
			int dy = j - m_kernel_radius;
			if (sqrt(dx * dx + dy * dy) <= m_kernel_radius + 0.5) {
				m_kmask[j * m_kernel_dim + i] = true;
			}
		}
	}
}

void MT::setMask_Diamond() {

	for (char& value : m_kmask)
		value = false;

	int mid = m_kernel_dim / 2;

	for (int j = 0; j < m_kernel_dim; ++j) {

		int x_start = abs(mid - j);
		int x_end = (j > mid) ? m_kernel_dim - x_start - 1 : mid + j;

		for (int i = x_start; i <= x_end; ++i) {
			m_kmask[j * m_kernel_dim + i] = true;
		}
	}
}

void MT::invertMask() {
	for (auto& val : m_kmask)
		val = (val) ? false : true;
}

void MT::rotateMask() {

	std::vector<char> temp(m_kmask.size());

	int s = m_kernel_dim - 1;

	for (int y = 0; y < m_kernel_dim; ++y) {

		for (int x = 0, x_s = m_kernel_dim - 1; x < m_kernel_dim; ++x) {
			temp[y * m_kernel_dim + x] = m_kmask[(s - x) * m_kernel_dim + y];

		}
	}

	m_kmask = temp;
}


void MT::GetMaskedLocations() {
	int count = 0; //number of true values
	for (auto v : m_kmask)
		if (v) count++;

	m_mask_loc = std::vector<int>(count);

	for (int j = 0, el = 0; j < m_kernel_dim; ++j)
		for (int i = 0; i < m_kernel_dim; ++i)
			if (m_kmask[j * m_kernel_dim + i])
				m_mask_loc[el++] = j * m_kernel_dim + i; //location of true values in the kernel
}

template<typename T>
void MT::erosion(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Erosion...");
	int sum = 0, total = img.channels() * img.rows();

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.minimum());

			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);

}
template void MT::erosion(Image8&);
template void MT::erosion(Image16&);
template void MT::erosion(Image32&);

template<typename T>
void MT::dialation(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Dialation...");
	int sum = 0, total = img.channels() * img.rows();

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.maximum());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::dialation(Image8&);
template void MT::dialation(Image16&);
template void MT::dialation(Image32&);

template <typename T>
void MT::opening(Image<T>& img) {

	dialation(img);
	erosion(img);
}
template void MT::opening(Image8&);
template void MT::opening(Image16&);
template void MT::opening(Image32&);

template <typename T>
void MT::closing(Image<T>& img) {

	erosion(img);
	dialation(img);
}
template void MT::closing(Image8&);
template void MT::closing(Image16&);
template void MT::closing(Image32&);

template <typename T>
void MT::selection(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Selection...");
	int sum = 0, total = img.channels() * img.rows();

	int pivot = (m_selection == 1.0) ? m_mask_loc.size() - 1 : m_mask_loc.size() * m_selection;

	for (int ch = 0; ch < img.channels(); ++ch) {

#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.selection(pivot));
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::selection(Image8&);
template void MT::selection(Image16&);
template void MT::selection(Image32&);

template <typename T>
void MT::median(Image<T>& img) {

	if (m_mask_loc.size() == m_kmask.size() && m_kernel_dim <= 9)
		return fastMedian(img, m_kernel_dim);

	Image<T> temp(img);

	m_ps->emitText("Median...");
	int sum = 0, total = img.channels() * img.rows();

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.median());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::median(Image8&);
template void MT::median(Image16&);
template void MT::median(Image32&);

template<typename T>
void MT::midpoint(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Selection...");
	int sum = 0, total = img.channels() * img.rows();

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.midpoint());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::midpoint(Image8&);
template void MT::midpoint(Image16&);
template void MT::midpoint(Image32&);


template <typename T>
void MT::fastMedian3x3(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 9>kernel{ 0 };

				kernel[0] = img.At_mirrored(x - 1, y - 1, ch);
				kernel[1] = img.At_mirrored(x, y - 1, ch);
				kernel[2] = img.At_mirrored(x + 1, y - 1, ch);

				kernel[3] = img.At_mirrored(x - 1, y, ch);
				kernel[4] = img(x, y, ch);
				kernel[5] = img.At_mirrored(x + 1, y, ch);

				kernel[6] = img.At_mirrored(x - 1, y + 1, ch);
				kernel[7] = img.At_mirrored(x, y + 1, ch);
				kernel[8] = img.At_mirrored(x + 1, y + 1, ch);

				for (int r = 0; r < 3; ++r) {
					for (int i = 0, j = 5; i < 4; ++i, ++j) {
						if (kernel[i] > kernel[4])
							std::swap(kernel[i], kernel[4]);
						if (kernel[j] < kernel[4])
							std::swap(kernel[j], kernel[4]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[4]);
			}
		}
	}
	temp.moveTo(img);
}
template void MT::fastMedian3x3(Image8&);
template void MT::fastMedian3x3(Image16&);
template void MT::fastMedian3x3(Image32&);

template <typename T>
void MT::fastMedian5x5(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 25>kernel{ 0 };

				for (int j = -2, el = 0; j <= 2; ++j)
					for (int i = -2; i <= 2; ++i)
						kernel[el++] = img.At_mirrored(x + i, y + j, ch);

				for (int r = 0; r < 5; ++r)
					for (int i = 0, j = 13; i < 12; ++i, ++j) {
						if (kernel[i] > kernel[12])
							std::swap(kernel[i], kernel[12]);
						if (kernel[j] < kernel[12])
							std::swap(kernel[j], kernel[12]);
					}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[12]);
			}
		}
	}
	temp.moveTo(img);
}
template void MT::fastMedian5x5(Image8&);
template void MT::fastMedian5x5(Image8&);
template void MT::fastMedian5x5(Image8&);

template <typename T>
void MT::fastMedian7x7(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 49> kernel;

				for (int j = -3, el = 0; j <= 3; ++j)
					for (int i = -3; i <= 3; ++i)
						kernel[el++] = img.At_mirrored(x + i, y + j, ch);

				for (int r = 0; r < 7; ++r) {
					for (int i = 0, j = 25; i < 24; ++i, ++j) {
						if (kernel[i] > kernel[24])
							std::swap(kernel[i], kernel[24]);
						if (kernel[j] < kernel[24])
							std::swap(kernel[j], kernel[24]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[24]);
			}
		}
	}

	temp.moveTo(img);
}
template void MT::fastMedian7x7(Image8&);
template void MT::fastMedian7x7(Image16&);
template void MT::fastMedian7x7(Image32&);

template <typename T>
void MT::fastMedian9x9(Image<T>& img) {

	Image<T> temp(img);

	int sum = 0, total = img.channels() * img.rows();

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for //private(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			Kernel2D<T> kernel(*this, img);
			kernel.Populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.Update(x, y, ch);

				std::vector<T> k(kernel.m_size);
				memcpy(&k[0], &kernel.data[0], k.size() * sizeof(T));

				for (int r = 0; r < 9; ++r) {
					for (int i = 0, j = 41; i < 40; ++i, ++j) {
						if (k[i] > k[40])
							std::swap(k[i], k[40]);
						if (k[j] < k[40])
							std::swap(k[j], k[40]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), k[40]);
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::fastMedian9x9(Image8&);
template void MT::fastMedian9x9(Image16&);
template void MT::fastMedian9x9(Image32&);

template <typename T>
void MT::fastMedian(Image<T>& img, int kernel_dim) {
	if (kernel_dim > 9 || kernel_dim % 2 == 0)
		return;

	m_ps->emitText("Fast Median...");

	switch (kernel_dim) {
	case 3:
		return fastMedian3x3(img);
	case 5:
		return fastMedian5x5(img);
	case 7:
		return fastMedian7x7(img);
	case 9:
		return fastMedian9x9(img);
	}
}
template void MT::fastMedian(Image8&, int);
template void MT::fastMedian(Image16&, int);
template void MT::fastMedian(Image32&, int);

template <typename T>
void MT::apply(Image<T>& src) {

	using MF = MorphologicalFilter;

	GetMaskedLocations();
	if (m_mask_loc.size() == 0)
		return src.FillZero();

	switch (m_morph_filter) {
	case MF::erosion:
		return erosion(src);
	case MF::dialation:
		return dialation(src);
	case MF::opening:
		return opening(src);
	case MF::closing:
		return closing(src);
	case MF::selection:
		return selection(src);
	case MF::median:
		return median(src);
	case MF::midpoint:
		return midpoint(src);
	default:
		return erosion(src);
	}
}
template void MT::apply(Image8&);
template void MT::apply(Image16&);
template void MT::apply(Image32&);






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






using MTD = MorphologicalTransformationDialog;

MTD::MorphologicalTransformationDialog(QWidget* parent) : ProcessDialog("MorphologicalTransformation", QSize(310,485), FastStack::recast(parent)->workspace(), false) {
	
	connect(this, &ProcessDialog::processDropped, this, &MTD::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &MTD::Apply, &MTD::showPreview, &MTD::resetDialog);

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
	
	PushButton* all_true = new PushButton("1", this);
	all_true->resize(size);
	all_true->move(x, dy);
	all_true->setToolTip("Set all elements to true");
	connect(all_true, &QPushButton::pressed, this, &MTD::setMask_true);

	PushButton* all_false = new PushButton("0", this);
	all_false->resize(size);
	all_false->move(x += dx, dy);
	all_false->setToolTip("Set all elements to false");
	connect(all_false, &QPushButton::pressed, this, &MTD::setMask_false);


	PushButton* circular = new PushButton("C", this);
	//circular->setIcon(Qt::SP_)
	circular->resize(size);
	circular->move(x += dx, dy);
	circular->setToolTip("Circular Kernel");
	connect(circular, &QPushButton::pressed, this, &MTD::setMask_Circular);


	x = 15;
	dy += 35;
	PushButton* diamond = new PushButton("D", this);
	diamond->resize(size);
	diamond->move(x, dy);
	diamond->setToolTip("Diamond Kernel");
	connect(diamond, &QPushButton::pressed, this, &MTD::setMask_Diamond);


	PushButton* invert = new PushButton("I", this);
	invert->resize(size);
	invert->move(x += dx, dy);
	invert->setToolTip("Invert all elements");
	connect(invert, &QPushButton::pressed, this, &MTD::invertMask);

	PushButton* rotate = new PushButton("R", this);
	rotate->resize(size);
	rotate->move(x += dx, dy);
	rotate->setToolTip("Rotate kernel 90\u00B0 clock-wise");
	connect(rotate, &QPushButton::pressed, this, &MTD::rotateMask);
}

void MTD::addKernelScene(){

	m_mks = new MorphologicalKernelScene(m_mt, QRect(0, 0, 280, 280));

	m_gv = new QGraphicsView(m_mks, this);
	m_gv->setRenderHints(QPainter::Antialiasing);
	m_gv->setGeometry(15, 15, 280, 280);
	m_gv->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_gv->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

}

void MTD::addKernelSizeCombo() {

	m_kerenl_size_cb = new ComboBox(this);

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

	m_filter_cb = new ComboBox(this);

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

		if (MorphologicalFilter(index) == MorphologicalFilter::selection) {
			m_selection_le->setEnabled(true);
			m_selection_slider->setEnabled(true);
		}
		else {
			m_selection_le->setDisabled(true);
			m_selection_slider->setDisabled(true);
		}

		m_mt.setMorphologicalFilter(MorphologicalFilter(index));
	};

	connect(m_filter_cb, &QComboBox::activated, this, activate);
}

void MTD::addSelectionInputs() {

	m_selection_le = new DoubleLineEdit(new DoubleValidator(0.00, 1.00, 2, this), this);
	m_selection_le->setValue(m_mt.selectionPoint());
	m_selection_le->resize(50, 25);
	m_selection_le->move(80, 385);
	m_selection_le->setDisabled(true);
	m_selection_le->addLabel(new QLabel("Selection:  ", this));

	m_selection_slider = new Slider(Qt::Horizontal, this);
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

	m_amount_le = new DoubleLineEdit(new DoubleValidator(0.00, 1.00, 2, this), this);
	m_amount_le->setValue(m_mt.blendAmount());
	m_amount_le->resize(50, 25);
	m_amount_le->move(80, 420);
	m_amount_le->addLabel(new QLabel("Amount:  ", this));
	const QString tt = "Blend amount between original pixel and new pixel value.";
	m_amount_le->setToolTip(tt);

	m_amount_slider = new Slider(Qt::Horizontal, this);
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

void MTD::showPreview() {}

void MTD::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast(m_workspace->currentSubWindow()->widget());
	
	std::unique_ptr<ProgressDialog> pd;
	if (m_mt.kernelDimension() >= 9 || m_mt.morphologicalFilter() == MorphologicalFilter::selection)
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

void MTD::ApplytoPreview() {}