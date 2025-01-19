#include "pch.h"
#include "ImageGeometryDialogs.h"
#include "FastStack.h"
#include "Interpolator.h"
#include"ImageWindow.h"


RotationDialog::RotationDialog(QWidget* parent) :ProcessDialog("ImageRotation", QSize(275, 200), FastStack::recast(parent)->workspace(), false) {

	connectToolbar(this, &RotationDialog::apply, &RotationDialog::showPreview, &RotationDialog::resetDialog);

	QLabel* label = new QLabel("Rotation Angle(\u00B0):", this);
	label->move(10, 40);

	m_theta_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 359.9999, 3), this);
	m_theta_le->resize(75, m_theta_le->size().height());
	m_theta_le->move(30, 65);
	connect(m_theta_le, &DoubleLineEdit::editingFinished, this, &RotationDialog::editingFinished_theta);

	m_dial = new QDial(this);
	m_dial->setRange(0, 240);
	m_dial->setWrapping(true);
	m_dial_offset = m_dial->maximum() * 0.75;
	m_dial->setValue(m_dial_offset);
	m_dial->setNotchesVisible(true);
	m_dial->setNotchTarget(15);
	m_dial->move(150, 10);
	connect(m_dial, &QDial::sliderMoved, this, &RotationDialog::dialMoved);
	connect(m_dial, &QDial::actionTriggered, this, &RotationDialog::actionDial);


	m_interpolate_cb = new InterpolationComboBox(this);
	m_interpolate_cb->move(110, 135);
	m_interpolate_cb->addLabel(new QLabel("Interpolation:   ", this));
	this->show();
}

void RotationDialog::actionDial(int action) {
	if (action == 7)
		dialMoved(m_dial->sliderPosition());
}

void RotationDialog::dialMoved(int pos) {

	float theta = 0.0;

	if (pos <= m_dial_offset)
		theta = 360 * (double(m_dial_offset - pos) / m_dial->maximum());
	else
		theta = ((m_dial_offset + double(m_dial->maximum() - pos)) / m_dial->maximum()) * 360;

	m_roatation.setTheta(theta);
	m_theta_le->setValue(theta);
}

void RotationDialog::editingFinished_theta() {
	float theta = m_theta_le->text().toFloat();
	m_roatation.setTheta(theta);

	int pos = m_dial_offset - (theta / 360) * m_dial->maximum();
	m_dial->setValue(pos);
}

void RotationDialog::resetDialog() {
	m_roatation.reset();
	m_interpolate_cb->reset();
	m_dial->setValue(m_dial_offset);
	m_theta_le->setValue(0.0);
}

void RotationDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource_Geometry(m_roatation, &Rotation::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource_Geometry(m_roatation, &Rotation::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource_Geometry(m_roatation, &Rotation::apply);
		break;
	}
	}

	setEnabledAll(true);
	updateImageLabel(iwptr->subwindow());
}





FastRotationDialog::FastRotationDialog(QWidget* parent) :ProcessDialog("FastRotation", QSize(250, 150), FastStack::recast(parent)->workspace(), false) {

	using enum FastRotation::Type;

	connectToolbar(this, &FastRotationDialog::apply, &FastRotationDialog::showPreview, &FastRotationDialog::resetDialog);

	QString str = QString("Rotate 90\u00B0 Clockwise");
	m_90cw_rb = new RadioButton(str, this);
	m_90cw_rb->setChecked(true);
	connect(m_90cw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CW); });
	m_90cw_rb->move(10, 10);

	str = QString("Rotate 90\u00B0 Counter-clockwise");
	m_90ccw_rb = new RadioButton(str, this);
	connect(m_90ccw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CCW); });
	m_90ccw_rb->move(10, 30);

	str = QString("Rotate 180\u00B0");
	m_180_rb = new RadioButton(str, this);
	connect(m_180_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate180); });
	m_180_rb->move(10, 50);

	m_hm_rb = new RadioButton("Horizontal Mirror", this);
	connect(m_hm_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(horizontalmirror); });
	m_hm_rb->move(10, 70);

	m_vm_rb = new RadioButton("Vertical Mirror", this);
	connect(m_vm_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(verticalmirror); });
	m_vm_rb->move(10, 90);

	show();
}

void FastRotationDialog::resetDialog() {
	m_90cw_rb->setChecked(true);
	m_fastrotation.setFastRotationType(FastRotation::Type::rotate90CW);
}

void FastRotationDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource_Geometry(m_fastrotation, &FastRotation::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource_Geometry(m_fastrotation, &FastRotation::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource_Geometry(m_fastrotation, &FastRotation::apply);
		break;
	}
	}

	setEnabledAll(true);
	updateImageLabel(iwptr->subwindow());
}




static void AddLabeltoWidget(QWidget* widget, QLabel* label) {

	int x = widget->geometry().x();
	int y = widget->geometry().y();
	int h = widget->geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}





IntegerResampleDialog::IntegerResampleDialog(QWidget* parent) :ProcessDialog("IntegerResample", QSize(215, 180), FastStack::recast(parent)->workspace(), false) {

	using IR = IntegerResample;

	connectToolbar(this, &IntegerResampleDialog::apply, &IntegerResampleDialog::showPreview, &IntegerResampleDialog::resetDialog);


	m_type_bg = new QButtonGroup(this);
	RadioButton* rb = new RadioButton("Downsample", this);
	rb->setChecked(true);
	rb->move(45, 50);
	m_type_bg->addButton(rb, int(IR::Type::downsample));

	rb = new RadioButton("Upsample", this);
	m_type_bg->addButton(rb, int(IR::Type::upsample));
	rb->move(45, 75);

	connect(m_type_bg, &QButtonGroup::idClicked, this, [this](int id) { m_ir.setType(IR::Type(id)); });


	m_factor_sb = new SpinBox(this);
	m_factor_sb->move(140, 10);
	m_factor_sb->addLabel(new QLabel("Resample Factor:   ", this));
	m_factor_sb->setRange(1, Pixel<uint8_t>::max());
	connect(m_factor_sb, &QSpinBox::valueChanged, this, [this](int value) {m_ir.setFactor(value); });

	m_method_combo = new ComboBox(this);
	m_method_combo->addItems({ "Average", "Median", "Max", "Min" });
	m_method_combo->move(95, 110);
	AddLabeltoWidget(m_method_combo, new QLabel("Method:   ", this));
	connect(m_method_combo, &QComboBox::activated, this, [this](int index) { m_ir.setMethod(IR::Method(index)); });

	show();
}

void IntegerResampleDialog::resetDialog() {
	m_ir = IntegerResample();
	m_method_combo->setCurrentIndex(int(m_ir.method()));
	m_type_bg->button(int(m_ir.type()))->setChecked(true);
	m_factor_sb->setValue(m_ir.factor());
}

void IntegerResampleDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource_Geometry(m_ir, &IntegerResample::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource_Geometry(m_ir, &IntegerResample::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource_Geometry(m_ir, &IntegerResample::apply);
		break;
	}
	}

	setEnabledAll(true);
	updateImageLabel(iwptr->subwindow());
}







CropFrame::CropFrame(const QRect& rect, QWidget* parent) : m_default_geometry(rect), QMdiSubWindow(parent) {

	this->setGeometry(rect);
	this->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
	this->setBackgroundRole(QPalette::Window);
	this->setMinimumSize(QSize(20, 20));

	QPalette pal;
	pal.setBrush(QPalette::Window, QColor(0, 0, 0, 0));
	this->setPalette(pal);

	this->show();
}

QRect CropFrame::frameRect()const {

	float t = m_pen_width / 2.0;

	QRect rect = this->rect();

	rect.setX(rect.x() + t);
	rect.setY(rect.y() + t);
	rect.setWidth(rect.width() - 2 * t);
	rect.setHeight(rect.height() - 2 * t);

	return rect;
}

QRect CropFrame::frameGeometry()const {

	float t = m_pen_width / 2.0;

	QRect rect = this->geometry();

	rect.setX(rect.x() + t);
	rect.setY(rect.y() + t);
	rect.setWidth(rect.width() - 2 * t);
	rect.setHeight(rect.height() - 2 * t);

	return rect;
}

void CropFrame::resetFrame() {
	this->setGeometry(m_default_geometry);
}

void CropFrame::paintEvent(QPaintEvent* event) {

	QPainter p(this);

	QPen pen(QColor(75, 0, 130));
	pen.setWidth(m_pen_width);
	p.setPen(pen);

	p.drawRect(frameRect());
}



NonCropArea::NonCropArea(const CropFrame& cf, QWidget* parent) : m_cf(&cf), QWidget(parent) { 
	this->setAttribute(Qt::WA_TransparentForMouseEvents);
	this->resize(parentWidget()->size()); 
	this->show(); 
}

void NonCropArea::paintEvent(QPaintEvent* e) {
	QPainter p(this);

	int width = this->size().width();
	int height = this->size().height();

	QRect frame = m_cf->frameGeometry();

	QRect top(0, 0, width, frame.y());
	QRect bottom(QPoint(0, frame.bottom()), QPoint(width, height));

	QRect left(0, frame.top(), frame.x(), frame.height() - 1);
	QRect right(frame.topRight(), QPoint(width, frame.bottom() - 1));

	p.fillRect(top, QColor(50, 50, 70, 169));
	p.fillRect(bottom, QColor(50, 50, 70, 169));
	p.fillRect(left, QColor(50, 50, 70, 169));
	p.fillRect(right, QColor(50, 50, 70, 169));

	e->accept();
}



template<typename T>
CropPreview<T>::CropPreview(QWidget* parent) : PreviewWindow<T>(parent, true) {

	QRect rect = QRect(20, 20, this->size().width() - 40, this->size().height() - 40);
	m_cf = new CropFrame(rect, this);
	m_nca = new NonCropArea(*m_cf, this);
}

template<typename T>
QRect CropPreview<T>::cropRect()const {
	QRect rect = m_cf->frameGeometry();	

	int x1 = rect.x() * this->binFactor();
	int w = rect.width() * this->binFactor();
	int y1 = rect.y() * this->binFactor();
	int h = rect.height() * this->binFactor();

	return QRect(x1, y1, w, h);
}

template<typename T>
void CropPreview<T>::resetFrame() {
	m_cf->resetFrame();
}

template class CropPreview<uint8_t>;
template class CropPreview<uint16_t>;
template class CropPreview<float>;



CropDialog::CropDialog(QWidget* parent) :ProcessDialog(m_crop_image_str, QSize(220, 80), FastStack::recast(parent)->workspace(), false, false) {
	
	connectToolbar(this, &CropDialog::apply, &CropDialog::showPreview, &CropDialog::resetDialog);

	m_image_sel = new ComboBox(this);
	m_image_sel->addItem("No Image Selected");
	m_image_sel->setFixedWidth(200);
	m_image_sel->move(10, 10);

	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(imageRecast(sw->widget())->name());

	connect(reinterpret_cast<Workspace*>(m_workspace), &Workspace::imageWindowClosed, this, &CropDialog::onWindowClose);
	connect(reinterpret_cast<Workspace*>(m_workspace), &Workspace::imageWindowCreated, this, &CropDialog::onWindowOpen);
	connect(m_image_sel, &QComboBox::activated, this, &CropDialog::onActivation_imageSelection); // show preview on activation

	show();
}

void CropDialog::closeEvent(QCloseEvent* e) {

	for (auto sw : m_workspace->subWindowList()) {
		auto iw = imageRecast<>(sw->widget());
		auto pw = iw->preview();
		if (pw && pw->processType() == name())
			iw->closePreview();
	}

	emit windowClosed();
	e->accept();
}

void CropDialog::onWindowOpen() {
	QString str = imageRecast<>(m_workspace->currentSubWindow()->widget())->name();
	m_image_sel->addItem(str);
}

void CropDialog::onWindowClose() {

	QString str = imageRecast<>(m_workspace->currentSubWindow()->widget())->name();
	int index = m_image_sel->findText(str);

	m_preview = nullptr;
	m_image_sel->removeItem(index);
	m_image_sel->setCurrentIndex(0);
}

void CropDialog::onActivation_imageSelection(int index) {

	auto f = [this]() {
		m_preview = nullptr;
		m_image_sel->setCurrentIndex(0);
	};

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = imageRecast<>(sw->widget());

		if (iwptr->previewExists() && (iwptr->preview()->processType() == name() || iwptr->name() == m_image_sel->currentText()))
			iwptr->closePreview();
	}
	
	m_image_sel->setCurrentIndex(index);

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = imageRecast<>(sw->widget());

		if (m_image_sel->currentText() == iwptr->name()) {

			switch (iwptr->type()) {
			case ImageType::UBYTE: {
				iwptr->showPreview(new CropPreview<uint8_t>(iwptr));
				connect(iwptr->preview()->windowSignals(), &WindowSignals::windowClosed, this, f);
				break;
			}
			case ImageType::USHORT: {
				auto iw16 = imageRecast<uint16_t>(iwptr);
				iw16->showPreview(new CropPreview<uint16_t>(iw16));
				connect(iw16->preview()->windowSignals(), &WindowSignals::windowClosed, this, f);
				break;
			}
			case ImageType::FLOAT: {
				auto iw32 = imageRecast<float>(iwptr);
				iw32->showPreview(new CropPreview<float>(iw32));
				connect(iw32->preview()->windowSignals(), &WindowSignals::windowClosed, this, f);
				break;
			}
			}

			m_preview = iwptr->preview();
			iwptr->preview()->setWindowTitle(iwptr->name() + " Crop Window");
			iwptr->preview()->setProcessType(name());
		}

	}
}

void CropDialog::resetDialog() {

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->name()) {
			if (iwptr->previewExists())
				reinterpret_cast<CropPreview<uint8_t>*>(iwptr->preview())->resetFrame();
		}
	}
}

void CropDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->name()) {

			switch (iwptr->type()) {
			case ImageType::UBYTE: {
				auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint8_t>*>(iw8->preview())->cropRect());
				iw8->applyToSource_Geometry(m_crop, &Crop::apply);
				iw8->closePreview();
				updateImageLabel(iw8->subwindow());
				break;
			}
			case ImageType::USHORT: {
				auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint16_t>*>(iw16->preview())->cropRect());
			    iw16->applyToSource_Geometry(m_crop, &Crop::apply);
				iw16->closePreview();
				updateImageLabel(iw16->subwindow());
				break;
			}
			case ImageType::FLOAT: {
				auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<float>*>(iw32->preview())->cropRect());
				iw32->applyToSource_Geometry(m_crop, &Crop::apply);
				iw32->closePreview();
				updateImageLabel(iw32->subwindow());
				break;
			}
			}
		}
	}

	setEnabledAll(true);
}



ResizeDialog::ResizeDialog(QWidget* parent) :ProcessDialog("Resize Image", QSize(280, 115), FastStack::recast(parent)->workspace(), false) {

	connectToolbar(this, &ResizeDialog::apply, &ResizeDialog::showPreview, &ResizeDialog::resetDialog);


	m_row_le = new IntLineEdit(1'000, new IntValidator(1, 100'000), this);
	m_row_le->setFixedWidth(70);
	m_row_le->move(200, 10);
	m_row_le->addLabel(new QLabel("Height:   ", this));
	connect(m_row_le, &QLineEdit::editingFinished, this, [this]() {m_rs.setNewRows(m_row_le->text().toInt()); });

	m_col_le = new IntLineEdit(1'000, new IntValidator(1, 100'000), this);
	m_col_le->setFixedWidth(70);
	m_col_le->move(60, 10);
	m_col_le->addLabel(new QLabel("Width:   ", this));
	connect(m_col_le, &QLineEdit::editingFinished, this, [this]() {m_rs.setNewCols(m_col_le->text().toInt()); });

	m_interpolation_combo = new InterpolationComboBox(this);
	m_interpolation_combo->move(110, 50);
	m_interpolation_combo->addLabel(new QLabel("Interpolation:   ", this));

	connect(m_interpolation_combo, &QComboBox::activated, this, [this](int index) { m_rs.setInterpolation(Interpolator::Type(index)); });

	show();
}

void ResizeDialog::resetDialog() {
	m_rs = Resize();

	m_row_le->setValue(m_rs.newRows());
	m_col_le->setValue(m_rs.newCols());

	m_interpolation_combo->reset();
}

void ResizeDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource_Geometry(m_rs, &Resize::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource_Geometry(m_rs, &Resize::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource_Geometry(m_rs, &Resize::apply);
		break;
	}
	}
		
	setEnabledAll(true);
	updateImageLabel(iwptr->subwindow());
}





HomographyTransformationDialog::HomographyTransformationDialog(QWidget* parent) :ProcessDialog("ImageTransformation", QSize(300, 250), FastStack::recast(parent)->workspace(), parent, false) {

	connectToolbar(this, &HomographyTransformationDialog::apply, &HomographyTransformationDialog::showPreview, &HomographyTransformationDialog::resetDialog);

	QLabel* label = new QLabel("Homography:", this);
	label->move(15, 5);

	m_layout = new QGridLayout(this);

	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 3; ++i) {
			QString str = (i == j) ? "1.000000" : "0.000000";
			m_le_array[j * 3 + i] = new DoubleLineEdit(str.toDouble(), new DoubleValidator(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 6), this);
			connect(m_le_array[j * 3 + i], &DoubleLineEdit::editingFinished, this, &HomographyTransformationDialog::onEditingFinished);
			m_layout->addWidget(m_le_array[j * 3 + i], j, i);
			if (i == 2 && j == 2)
				m_le_array[8]->setDisabled(true);
		}
	}
	show();
}

void HomographyTransformationDialog::onEditingFinished() {
	Matrix h(3, 3);
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 3; ++i) {
			h(j, i) = m_le_array[j * 3 + i]->text().toDouble();
		}
	}
	m_transformation.setHomography(h);
}

void HomographyTransformationDialog::resetDialog() {
	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 3; ++i) {
			QString str = (i == j) ? "1.000000" : "0.000000";
			m_le_array[j * 3 + i]->setText(str);
		}
	}

}

void HomographyTransformationDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		iwptr->applyToSource_Geometry(m_transformation, &HomographyTransformation::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		iw16->applyToSource_Geometry(m_transformation, &HomographyTransformation::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		iw32->applyToSource_Geometry(m_transformation, &HomographyTransformation::apply);
		break;
	}
	}

	setEnabledAll(true);
	updateImageLabel(iwptr->subwindow());
}