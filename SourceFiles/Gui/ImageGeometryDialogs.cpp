#include "pch.h"
#include "ImageGeometryDialogs.h"
#include "FastStack.h"
#include "Interpolator.h"
#include"ImageWindow.h"


RotationDialog::RotationDialog(Workspace* parent) :ProcessDialog("ImageRotation", QSize(275, 175), parent, false) {

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

	if (workspace()->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(workspace()->currentSubWindow()->widget());

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





FastRotationDialog::FastRotationDialog(Workspace* parent) :ProcessDialog("FastRotation", QSize(250, 130), parent, false) {

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

	if (workspace()->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(workspace()->currentSubWindow()->widget());

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







IntegerResampleDialog::IntegerResampleDialog(Workspace* parent) :ProcessDialog("IntegerResample", QSize(210, 155), parent, false) {

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

	if (workspace()->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(workspace()->currentSubWindow()->widget());

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
CropPreview<T>::CropPreview(ImageWindow<T>* image_window) : PreviewWindow<T>(imageRecast<T>(image_window), true) {

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





void ImageWindowComboBox::addImageWindow(ImageWindow8* iw) {

	if (iw)
		this->addItem(iw->name(), QVariant::fromValue(iw));
}

ImageWindow8* ImageWindowComboBox::currentImageWindow() {
	return currentData().value<ImageWindow8*>();
}

ImageWindow8* ImageWindowComboBox::imageWindowAt(int index) {
	return itemData(index).value<ImageWindow8*>();
}

int ImageWindowComboBox::findImageWindow(ImageWindow8* iw) {

	for (int i = 0; i < count(); ++i)
		if (iw == imageWindowAt(i))
			return i;

	return -1;
}





CropDialog::CropDialog(Workspace* parent) : ProcessDialog("Crop Image", QSize(220, 60), parent, false, false) {
	
	m_image_sel = new ImageWindowComboBox(drawArea());
	m_image_sel->move(10, 15);

	for (auto sw : workspace()->subWindowList())
		m_image_sel->addImageWindow(imageRecast(sw->widget()));

	connect(m_image_sel, &QComboBox::activated, this, &CropDialog::onActivation_imageSelection); // show preview on activation

	show();
}

void CropDialog::onImageWindowCreated() {

	m_image_sel->addImageWindow(imageRecast(workspace()->subWindowList().last()->widget()));
}

void CropDialog::onImageWindowClosed() {

	int index = m_image_sel->findImageWindow(imageRecast(workspace()->currentSubWindow()->widget()));
	if (index == m_image_sel->currentIndex())
		m_image_sel->setCurrentIndex(0);
	m_image_sel->removeItem(index);
}

void CropDialog::onActivation_imageSelection(int index) {

	if (index == 0) {
		if (preview())
			preview()->close();
		return;
	}

	if (preview()) {
		disconnect(m_connection);
		preview()->close();
	}

	auto iwptr = m_image_sel->currentImageWindow();
	workspace()->setActiveSubWindow(iwptr->subwindow());

	switch (currentImageType()) {
	case ImageType::UBYTE: {
		this->showPreviewWindow(false, new CropPreview(currentImageWindow()));
		break;
	}
	case ImageType::USHORT: {
		this->showPreviewWindow(false, new CropPreview<uint16_t>(currentImageWindow<uint16_t>()));
		break;
	}
	case ImageType::FLOAT: {
		this->showPreviewWindow(false, new CropPreview<float>(currentImageWindow<float>()));
		break;
	}
	}
	m_connection = connect(preview(), &PreviewWindowBase::windowClosed, std::bind(&QComboBox::setCurrentIndex, m_image_sel, 0));
	preview()->setTitle(currentImageWindow()->name() + " Crop Window");
}

void CropDialog::resetDialog() {

	for (auto sw : workspace()->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->name()) {
			if (iwptr->previewExists())
				reinterpret_cast<CropPreview<uint8_t>*>(iwptr->preview())->resetFrame();
		}
	}
}

void CropDialog::apply() {

	if (!workspace()->hasSubWindows())
		return;

	auto iwptr = m_image_sel->currentImageWindow();

	if (iwptr == nullptr)
		return;

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		m_crop.setRegion(dynamic_cast<CropPreview<uint8_t>*>(iwptr->preview())->cropRect());
		iwptr->applyToSource_Geometry(m_crop, &Crop::apply);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		m_crop.setRegion(dynamic_cast<CropPreview<uint16_t>*>(iw16->preview())->cropRect());
		iw16->applyToSource_Geometry(m_crop, &Crop::apply);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		m_crop.setRegion(dynamic_cast<CropPreview<float>*>(iw32->preview())->cropRect());
		iw32->applyToSource_Geometry(m_crop, &Crop::apply);
		break;
	}
	}

	disconnect(m_connection);
	preview()->close();
}

void CropDialog::applyPreview() {

	if (!isPreviewValid())
		return;

	switch (preview()->type()) {
	case ImageType::UBYTE: 
		return preview()->updatePreview();
	
	case ImageType::USHORT: 
		return preview<uint16_t>()->updatePreview();
	
	case ImageType::FLOAT: 
		return preview<float>()->updatePreview();
	}
}





LinkButton::LinkButton(QWidget* parent) : PushButton("",parent) {

	this->resize(30, 30);
	this->setCheckable(true);
	this->setAutoDefault(false);
	this->setFlat(true);
	this->setChecked(true);
}

void LinkButton::paintEvent(QPaintEvent* e) {

	QPainter p(this);
	QPen pen;
	pen.setWidth(3);
	pen.setColor(QColor(169, 169, 169));
	p.setPen(pen);
	pen.setJoinStyle(Qt::RoundJoin);
	p.setRenderHint(QPainter::Antialiasing);

	//p.drawRoundedRect(width() / 3, height() / 3 - 2, width() / 3, height() / 3 + 4, 2, 2);
	p.drawRoundedRect(10, 8, 10, 14, 2, 2);
	p.drawLine(15, 6, 15, 10);
	p.drawLine(15, 20, 15, 24);

	if (isDown())
		p.drawRect(this->rect());

	if (!isChecked())
		p.fillRect(8, 13, 14, 4, QColor(39, 39, 39));//palette().brush(QPalette::Button));
}





ResizeDialog::ResizeDialog(Workspace* parent) : ProcessDialog("Resize Image", QSize(320, 225), parent, false, false) {

	m_image_sel = new ImageWindowComboBox(drawArea());
	m_image_sel->move(40, 15);
	m_image_sel->resize(240, m_image_sel->height());

	for (auto sw : workspace()->subWindowList())
		m_image_sel->addImageWindow(imageRecast(sw->widget()));

	connect(m_image_sel, &QComboBox::currentIndexChanged, this, &ResizeDialog::imageSelection); // show preview on activation

	m_new_size_label = new QLabel("New Size: ", drawArea());
	m_new_size_label->move(90, 145);
	m_new_size_label->hide();

	m_link_pb = new LinkButton(drawArea());
	m_link_pb->move(180, 80);
	m_link_pb->setToolTip("maintain aspect ratio");
	connect(m_link_pb, &QPushButton::clicked, this, [this]() { m_aspect_ratio = float(n_size.width()) / n_size.height(); });

	addWidthInput();
	addHeightInput();
	addUnitCombo();

	m_interpolation_combo = new InterpolationComboBox(drawArea());
	m_interpolation_combo->move(130, 180);
	addLabel(m_interpolation_combo, new QLabel("Interpolation:", drawArea()));
	connect(m_interpolation_combo, &QComboBox::activated, this, [this](int index) { m_rs.setInterpolation(Interpolator::Type(index)); });

	show();
}

void ResizeDialog::setPrecision(int prec) {
	m_width_sb->setDecimals(prec);
	m_height_sb->setDecimals(prec);
}

void ResizeDialog::updateLabel() {

	QString txt = m_new_size_label->text();
	txt.truncate(txt.indexOf(':') + 2);

	if (m_image_sel->currentIndex() != 0)
		txt += QString::number(n_size.width()) + " x " + QString::number(n_size.height());

	m_new_size_label->setText(txt);
	m_new_size_label->adjustSize();
}

void ResizeDialog::addWidthInput() {

	m_width_sb = new DoubleSpinBox(1, 0, 100'000, 0, drawArea());
	m_width_sb->setFixedWidth(100);
	m_width_sb->move(80, 60);
	m_width_sb->setDisabled(true);
	addLabel(m_width_sb, new QLabel("Width:", drawArea()));

	auto func = [this]() {

		auto iwptr = m_image_sel->currentImageWindow();

		if (iwptr == nullptr)
			return;

		auto& img = iwptr->source();

		double v = m_width_sb->value();
		int w = 0;

		switch (m_unit_combo->currentData().value<Unit>()) {
		case Unit::pixel:
			w = v;
			break;
		case Unit::percent:
			w = v * img.cols() / 100;
			break;
		//case Unit::inches:
			//w = v
		default:
			w = v;
		}
		//pnly needed on pixel and percent
		n_size.setWidth(w);
		if (m_link_pb->isChecked()) {
			int h =	w / m_aspect_ratio + 0.5;

		switch (m_unit_combo->currentData().value<Unit>()) {
			case Unit::pixel:
				m_height_sb->setValue(h);
				break;
			case Unit::percent:
				m_height_sb->setValue(h * 100 / img.rows());
				break;
			default:
				m_height_sb->setValue(h);
			}
			n_size.setHeight(h);
		}

		updateLabel();
	};

	connect(m_width_sb, &DoubleSpinBox::editingFinished, this, func);
	connect(m_width_sb, &DoubleSpinBox::step, this, func);
}

void ResizeDialog::addHeightInput() {

	m_height_sb = new DoubleSpinBox(1, 0, 100'000, 0, drawArea());
	m_height_sb->setFixedWidth(100);
	m_height_sb->move(80, 100);
	m_height_sb->setDisabled(true);
	addLabel(m_height_sb, new QLabel("Height:", drawArea()));


	auto func = [this]() {

		auto iwptr = m_image_sel->currentImageWindow();

		if (iwptr == nullptr)
			return;

		auto& img = iwptr->source();

		int v = m_height_sb->value();
		int h = 0;

		switch (m_unit_combo->currentData().value<Unit>()) {
		case Unit::pixel:
			h = v;
			break;
		case Unit::percent:
			h = v * img.rows() / 100;
			break;
		default:
			h = v;
		}

		n_size.setHeight(h);

		if (m_link_pb->isChecked()) {
			int w = m_aspect_ratio * h + 0.5;
			switch (m_unit_combo->currentData().value<Unit>()) {
			case Unit::pixel:
				m_width_sb->setValue(w);
				break;
			case Unit::percent:
				m_width_sb->setValue(w * 100 / img.cols());
				break;
			default:
				m_width_sb->setValue(w);
			}
			n_size.setWidth(w);
		}

		updateLabel();
	};
	connect(m_height_sb, &QDoubleSpinBox::editingFinished, this, func);
	connect(m_height_sb, &DoubleSpinBox::step, this, func);
}

void ResizeDialog::addUnitCombo() {

	m_unit_combo = new ComboBox(drawArea());
	m_unit_combo->move(215, 80);
	m_unit_combo->resize(90, m_unit_combo->height());
	m_unit_combo->addItem("pixels", QVariant::fromValue(Unit::pixel));
	m_unit_combo->addItem("percent", QVariant::fromValue(Unit::percent));
	connect(m_unit_combo, &QComboBox::currentIndexChanged, this, &ResizeDialog::unitSelection);

	auto func = [this](int index) {
		switch (m_unit_combo->itemData(index).value<Unit>()) {
		case Unit::pixel:
			return setPrecision(0);
		case Unit::percent:
			return setPrecision(2);
		default:
			return setPrecision(0);
		}
	};
	connect(m_unit_combo, &QComboBox::currentIndexChanged, this, func);
}

QSize ResizeDialog::getNewSize() {

	auto iwptr = m_image_sel->currentImageWindow();

	if (iwptr == nullptr)
		return QSize();

	auto& img = iwptr->source();

	int w = m_width_sb->value();
	int h = m_height_sb->value();

	switch (m_unit_combo->currentData().value<Unit>()) {
	case Unit::pixel:
		return QSize(w, h);
	case Unit::percent:
		return QSize(w * img.cols() / 100, h * img.rows() / 100);
	default:
		return QSize();
	}
}

void ResizeDialog::onImageWindowCreated() {

	m_image_sel->addImageWindow(imageRecast(workspace()->subWindowList().last()->widget()));
}

void ResizeDialog::onImageWindowClosed() {

	int index = m_image_sel->findImageWindow(imageRecast(workspace()->currentSubWindow()->widget()));

	if (index == m_image_sel->currentIndex())
		m_image_sel->setCurrentIndex(0);

	m_image_sel->removeItem(index);
}

void ResizeDialog::imageSelection(int index) {

	auto iwptr = m_image_sel->currentImageWindow();

	if (iwptr) {
		Image8& img = iwptr->source();
		n_size = QSize(img.cols(), img.rows());
		unitSelection(m_unit_combo->currentIndex());
		m_aspect_ratio = float(img.cols()) / img.rows();
		m_height_sb->setEnabled(true);
		m_width_sb->setEnabled(true);
		m_new_size_label->show();
	}
	else {
		m_height_sb->setValue(1.0);
		m_width_sb->setValue(1.0);
		m_height_sb->setDisabled(true);
		m_width_sb->setDisabled(true);
		n_size = QSize();
		m_new_size_label->hide();
	}
	updateLabel();
}

void ResizeDialog::unitSelection(int index) {

	auto iwptr = m_image_sel->currentImageWindow();

	if (iwptr == nullptr)
		return;

	auto& img = iwptr->source();

	switch (m_unit_combo->itemData(index).value<Unit>()) {
	case Unit::pixel:
		m_width_sb->setValue(n_size.width());
		m_height_sb->setValue(n_size.height());
		break;
	case Unit::percent:
		m_width_sb->setValue(n_size.width() * 100 / img.cols());
		m_height_sb->setValue(n_size.height() * 100 / img.rows());
		break;

	//case Unit::inches:
		//m_width_sb->setValue(n_size.width() / 72); //72 is resolution in px/in
	}
}

void ResizeDialog::resetDialog() {

	m_rs = Resize();
	imageSelection(m_image_sel->currentIndex());
	m_interpolation_combo->reset();
}

void ResizeDialog::apply() {

	if (!workspace()->hasSubWindows())
		return;

	auto iwptr = m_image_sel->currentImageWindow();
	if (iwptr == nullptr)
		return;

	m_rs.setNewSize(n_size.height(), n_size.width());

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

	n_size = getNewSize();
	updateLabel();
}

void ResizeDialog::paintEvent(QPaintEvent* e) {

	ProcessDialog::paintEvent(e);

	QPainter p(this);
	p.translate(0, titlebar()->height());
	QPen pen;
	pen.setWidth(2);
	pen.setColor(QColor(169,169,169));
	pen.setJoinStyle(Qt::RoundJoin);
	p.setPen(pen);
	std::array<QPoint,3> l = { QPoint(190,75),{198,75},{198,80} };
	p.drawLine(l[0], l[1]);
	p.drawLine(l[1], l[2]);

	l = { QPoint(190,115),{198,115},{198,110} };
	p.drawLine(l[0], l[1]);
	p.drawLine(l[1], l[2]);	
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

	if (!workspace()->hasSubWindows())
		return;

	auto iwptr = imageRecast<>(workspace()->currentSubWindow()->widget());

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