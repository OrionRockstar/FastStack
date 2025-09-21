#include "pch.h"
#include "ImageGeometryDialogs.h"
#include "FastStack.h"
#include "Interpolator.h"
#include"ImageWindow.h"


RotationDialog::RotationDialog(QWidget* parent) :ProcessDialog("ImageRotation", QSize(275, 175), FastStack::recast(parent)->workspace(), false) {

	QLabel* label = new QLabel("Rotation Angle(\u00B0):", drawArea());
	label->move(10, 40);

	m_theta_le = new DoubleLineEdit(0.0, new DoubleValidator(0.0, 359.9999, 3), drawArea());
	m_theta_le->resize(75, m_theta_le->size().height());
	m_theta_le->move(30, 65);
	connect(m_theta_le, &DoubleLineEdit::editingFinished, this, &RotationDialog::editingFinished_theta);

	m_dial = new QDial(drawArea());
	m_dial->setRange(0, 240);
	m_dial->setWrapping(true);
	m_dial_offset = m_dial->maximum() * 0.75;
	m_dial->setValue(m_dial_offset);
	m_dial->setNotchesVisible(true);
	m_dial->setNotchTarget(15);
	m_dial->move(150, 10);
	connect(m_dial, &QDial::sliderMoved, this, &RotationDialog::dialMoved);
	connect(m_dial, &QDial::actionTriggered, this, &RotationDialog::actionDial);


	m_interpolate_cb = new InterpolationComboBox(drawArea());
	m_interpolate_cb->move(110, 130);
	addLabel(m_interpolate_cb, new QLabel("Interpolation:", drawArea()));
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
}





FastRotationDialog::FastRotationDialog(QWidget* parent) :ProcessDialog("FastRotation", QSize(250, 130), FastStack::recast(parent)->workspace(), false) {

	using enum FastRotation::Type;

	QString str = QString("Rotate 90\u00B0 Clockwise");
	m_90cw_rb = new RadioButton(str, drawArea());
	m_90cw_rb->setChecked(true);
	connect(m_90cw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CW); });
	m_90cw_rb->move(10, 10);

	str = QString("Rotate 90\u00B0 Counter-clockwise");
	m_90ccw_rb = new RadioButton(str, drawArea());
	connect(m_90ccw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CCW); });
	m_90ccw_rb->move(10, 30);

	str = QString("Rotate 180\u00B0");
	m_180_rb = new RadioButton(str, drawArea());
	connect(m_180_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate180); });
	m_180_rb->move(10, 50);

	m_hm_rb = new RadioButton("Horizontal Mirror", drawArea());
	connect(m_hm_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(horizontalmirror); });
	m_hm_rb->move(10, 70);

	m_vm_rb = new RadioButton("Vertical Mirror", drawArea());
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
}







IntegerResampleDialog::IntegerResampleDialog(QWidget* parent) :ProcessDialog("IntegerResample", QSize(210, 155), FastStack::recast(parent)->workspace(), false) {

	m_type_bg = new QButtonGroup(this);
	RadioButton* rb = new RadioButton("Downsample", drawArea());
	rb->setChecked(true);
	rb->move(45, 50);
	m_type_bg->addButton(rb, int(IntegerResample::Type::downsample));

	rb = new RadioButton("Upsample", drawArea());
	m_type_bg->addButton(rb, int(IntegerResample::Type::upsample));
	rb->move(45, 75);

	connect(m_type_bg, &QButtonGroup::idClicked, this, [this](int id) { m_ir.setType(IntegerResample::Type(id)); m_method_combo->setDisabled(id); });


	m_factor_sb = new SpinBox(drawArea());
	m_factor_sb->move(140, 10);
	m_factor_sb->setRange(1, Pixel<uint8_t>::max());
	addLabel(m_factor_sb, new QLabel("Resample Factor:", drawArea()));
	connect(m_factor_sb, &QSpinBox::valueChanged, this, [this](int value) {m_ir.setFactor(value); });

	m_method_combo = new ComboBox(drawArea());
	m_method_combo->addItems({ "Average", "Median", "Max", "Min" });
	m_method_combo->move(95, 110);
	addLabel(m_method_combo, new QLabel("Method:", drawArea()));
	connect(m_method_combo, &QComboBox::activated, this, [this](int index) { m_ir.setMethod(IntegerResample::Method(index)); });

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
}







CropFrame::CropFrame(const QRect& geometry, QWidget* parent) : m_default_geometry(geometry), QWidget(parent) {

	this->setGeometry(geometry);
	this->setCursor(m_cursor);
	
	this->setMouseTracking(true);
	this->setBackgroundRole(QPalette::Window);
	this->setMinimumSize(QSize(20, 20));

	QPalette pal;
	pal.setBrush(QPalette::Window, QColor(0, 0, 0, 0));
	this->setPalette(pal);

	this->show();
}

QRect CropFrame::viewGeometry()const {

	QRect rect = this->geometry();
	rect.adjust(m_pen_width, m_pen_width, -m_pen_width, -m_pen_width);
	return rect;
}

void CropFrame::resetFrame() {
	this->setGeometry(m_default_geometry);
}

void CropFrame::enterEvent(QEnterEvent* e) {

}

void CropFrame::leaveEvent(QEvent* e) {

}

void CropFrame::mousePressEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton && m_frame.isOnFrame(e->pos())) {
		m_resizing = true;
		m_start_pos = e->pos();
		m_start_rect = geometry();
		m_current_border = m_frame.rectBorder(e->pos());
	}

	else if (e->buttons() == Qt::LeftButton) {
		this->setCursor(Qt::CursorShape::ClosedHandCursor);
		m_moving = true;
		m_start_pos = e->pos();
	}

	if (e->buttons() == Qt::RightButton)
		this->setCursor(QCursor(QPixmap(style()->standardPixmap(QStyle::SP_BrowserReload))));
}

void CropFrame::mouseMoveEvent(QMouseEvent* e) {

	if (e->buttons() == Qt::LeftButton && m_resizing) {

		QPoint dp = e->pos() - m_start_pos;
		QRect r = geometry();
		QRect pr = parentWidget()->rect();

		int left = math::min(r.left() + dp.x(), r.right() - minimumWidth());
		int top = math::min(r.top() + dp.y(), r.bottom() - minimumHeight());

		switch (m_current_border) {

		case RectBorder::TopEdge:
			r.setTop(top);
			break;

		case RectBorder::LeftEdge:;
			r.setLeft(left);
			break;

		case RectBorder::RightEdge:
			r.setRight(m_start_rect.right() + dp.x());
			break;

		case RectBorder::BottomEdge:
			r.setBottom(m_start_rect.bottom() + dp.y());
			break;

		case RectBorder::TopLeftCorner:
			r.setTopLeft({left,top});
			break;

		case RectBorder::TopRightCorner:
			r.setTopRight(QPoint(m_start_rect.right() + dp.x(), top));
			break;

		case RectBorder::BottomLeftCorner:
			r.setBottomLeft(QPoint(left, m_start_rect.bottom() + dp.y()));
			break;

		case RectBorder::BottomRightCorner:
			r.setBottomRight(m_start_rect.bottomRight() + dp);
			break;
		}

		//keeps in parent
		r.setTop(math::max(r.y(), 0));
		r.setLeft(math::max(r.x(), 0));
		r.setRight(math::min(r.right(), pr.right()));
		r.setBottom(math::min(r.bottom(), pr.bottom()));

		this->setGeometry(r);
	}

	else if (e->buttons() == Qt::LeftButton && m_moving) {

		QPoint dp = e->pos() - m_start_pos;	
		QRect r = geometry().translated(dp);
		QRect pr = parentWidget()->rect();

		r.moveTop(math::max(r.y(), 0));
		r.moveLeft(math::max(r.x(), 0));
		r.moveRight(math::min(r.right(), pr.right()));
		r.moveBottom(math::min(r.bottom(), pr.bottom()));

		this->move(r.topLeft());
	}

	else if (e->buttons() == Qt::NoButton)
		setCursor(m_frame.cursorAt(e->pos(), m_cursor));	
}

void CropFrame::mouseReleaseEvent(QMouseEvent* e) {

	if (e->button() == Qt::LeftButton) {
		m_resizing = false;
		m_moving = false;
		m_current_border = RectBorder::None;

		setCursor(m_frame.cursorAt(e->pos(), m_cursor));
	}

	if (e->button() == Qt::RightButton && rect().contains(e->pos()))
		resetFrame();
}

void CropFrame::resizeEvent(QResizeEvent* e) {

	QWidget::resizeEvent(e);
	m_frame.resize(rect());
}

void CropFrame::paintEvent(QPaintEvent* event) {

	QPainter p(this);

	QPen pen(QColor(75, 0, 130));
	pen.setWidth(m_pen_width);
	pen.setJoinStyle(Qt::MiterJoin);
	p.setPen(pen);

	QRect rect = this->rect();
	rect.adjust(m_pw_2, m_pw_2, -m_pw_2, -m_pw_2);
	p.drawRect(rect);
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

	QRect frame = m_cf->geometry().adjusted(0, 0, 1, 1);

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
CropPreview<T>::CropPreview(QWidget* image_window) : PreviewWindow<T>(imageRecast<T>(image_window), true) {

	this->resizeSource();
	QRect rect = QRect(20, 20, this->m_image_label->width() - 40, this->m_image_label->height() - 40);
	m_cf = new CropFrame(rect, this->m_image_label);
	m_nca = new NonCropArea(*m_cf, this->m_image_label);
	this->m_image_label->setCursor(Qt::ArrowCursor);
	this->updatePreview();
}

template<typename T>
QRect CropPreview<T>::cropRect()const {

	QRect rect = m_cf->viewGeometry();
	float s = 1 / this->scaleFactor();
	return QRect(rect.topLeft() * s, rect.bottomRight() * s);
}

template<typename T>
void CropPreview<T>::resetFrame() {
	m_cf->resetFrame();
}

template class CropPreview<uint8_t>;
template class CropPreview<uint16_t>;
template class CropPreview<float>;



CropDialog::CropDialog(QWidget* parent) : ProcessDialog("Crop Image", QSize(220, 60), FastStack::recast(parent)->workspace(), false, false) {
	
	m_image_sel = new ComboBox(drawArea());
	m_image_sel->addItem("No Image Selected");
	m_image_sel->setFixedWidth(200);
	m_image_sel->move(10, 15);

	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(imageRecast(sw->widget())->name());

	connect(m_image_sel, &QComboBox::activated, this, &CropDialog::onActivation_imageSelection); // show preview on activation

	show();
}

void CropDialog::closeEvent(QCloseEvent* e) {

	if (m_preview)
		m_preview->close();

	ProcessDialog::closeEvent(e);
}

void CropDialog::onImageWindowCreated() {
	//QString str = imageRecast<>(m_workspace->currentSubWindow()->widget())->name();
	m_image_sel->addImage(imageRecast<>(m_workspace->subWindowList().last()->widget()));
}

void CropDialog::onImageWindowClosed() {

	//QString str = imageRecast<>(m_workspace->currentSubWindow()->widget())->name();
	int index = m_image_sel->findImage(&imageRecast<>(m_workspace->currentSubWindow()->widget())->source());//m_image_sel->findText(str);
	m_image_sel->removeItem(index);
	m_image_sel->setCurrentIndex(0);
}

void CropDialog::onActivation_imageSelection(int index) {
	
	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = imageRecast<>(sw->widget());

		if (iwptr->previewExists()) {
			if (index != 0 && iwptr->preview()->processType() == name())
				return;
			else
				m_preview->close();
		}
	}


	//needed???
	auto cp = [this]<typename T>(ImageWindow<T>* iwb) {
		connect(iwb, &ImageWindowBase::windowUpdated, this, [this]() {  previewRecast<T>(m_preview)->updatePreview(); });
	};

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = imageRecast<>(sw->widget());

		if (m_image_sel->currentText() == iwptr->name()) {

			switch (iwptr->type()) {
			case ImageType::UBYTE: {
				iwptr->showPreview(new CropPreview<uint8_t>(iwptr));
				cp(iwptr);
				break;
			}
			case ImageType::USHORT: {
				auto iw16 = imageRecast<uint16_t>(iwptr);
				iw16->showPreview(new CropPreview<uint16_t>(iw16));
				cp(iw16);
				break;
			}
			case ImageType::FLOAT: {
				auto iw32 = imageRecast<float>(iwptr);
				iw32->showPreview(new CropPreview<float>(iw32));
				cp(iw32);
				break;
			}
			}
			connect(iwptr->preview(), &PreviewWindowBase::windowClosed, this, [this]() { m_image_sel->setCurrentIndex(0); });

			m_preview = iwptr->preview();
			iwptr->preview()->setTitle(iwptr->name() + " Crop Window");
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

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->name()) {

			switch (iwptr->type()) {
			case ImageType::UBYTE: {
				auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint8_t>*>(iw8->preview())->cropRect());
				iw8->applyToSource_Geometry(m_crop, &Crop::apply);
				break;
			}
			case ImageType::USHORT: {
				auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint16_t>*>(iw16->preview())->cropRect());
			    iw16->applyToSource_Geometry(m_crop, &Crop::apply);
				break;
			}
			case ImageType::FLOAT: {
				auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<float>*>(iw32->preview())->cropRect());
				iw32->applyToSource_Geometry(m_crop, &Crop::apply);
				break;
			}
			}
		}
	}

	if (m_preview)
		m_preview->close();
}



ResizeDialog::ResizeDialog(QWidget* parent) :ProcessDialog("Resize Image", QSize(280, 115), FastStack::recast(parent)->workspace(), false) {

	m_row_le = new IntLineEdit(1'000, new IntValidator(1, 100'000), drawArea());
	m_row_le->setFixedWidth(70);
	m_row_le->move(200, 10);
	addLabel(m_row_le, new QLabel("Height:", drawArea()));
	connect(m_row_le, &QLineEdit::editingFinished, this, [this]() {m_rs.setNewRows(m_row_le->text().toInt()); });

	m_col_le = new IntLineEdit(1'000, new IntValidator(1, 100'000), drawArea());
	m_col_le->setFixedWidth(70);
	m_col_le->move(60, 10);
	addLabel(m_col_le, new QLabel("Width:", drawArea()));
	connect(m_col_le, &QLineEdit::editingFinished, this, [this]() {m_rs.setNewCols(m_col_le->text().toInt()); });

	m_interpolation_combo = new InterpolationComboBox(drawArea());
	m_interpolation_combo->move(110, 50);
	addLabel(m_interpolation_combo, new QLabel("Interpolation:", drawArea()));

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
}





HomographyTransformationDialog::HomographyTransformationDialog(QWidget* parent) :ProcessDialog("ImageTransformation", QSize(300, 250), FastStack::recast(parent)->workspace(), parent, false) {

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
}