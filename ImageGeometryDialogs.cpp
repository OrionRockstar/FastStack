#include "pch.h"
#include "ImageGeometryDialogs.h"
#include "FastStack.h"
#include "Interpolator.h"
#include"ImageWindow.h"


RotationDialog::RotationDialog(QWidget* parent) :ProcessDialog("ImageRotation", QSize(275, 200), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {

	connect(this, &ProcessDialog::processDropped, this, &RotationDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &RotationDialog::Apply, &RotationDialog::showPreview, &RotationDialog::resetDialog);

	QLabel* label = new QLabel("Rotation Angle(\u00B0):", this);
	label->move(10, 40);

	m_theta_le = new DoubleLineEdit("0.000000", new DoubleValidator(0.0, 359.9999, 4), this);
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

	label = new QLabel("Interpolation: ", this);
	label->move(10, 137);

	m_interpolate_cb = new QComboBox(this);
	m_interpolate_cb->addItems({ "Nearest Neighbor", "Bilinear", "Bicubic Spline", "Bicubic B-Spline",
								"Cubic B-Spline", "Catmull-Rom", "Lanczos-3" });
	m_interpolate_cb->setCurrentIndex(int(Interpolator::Type::bicubic_spline));
	m_interpolate_cb->move(110, 135);

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
	m_theta_le->setText_Validated(QString::number(theta, 'f'));
}

void RotationDialog::editingFinished_theta() {
	float theta = m_theta_le->text().toFloat();
	m_roatation.setTheta(theta);


	int pos = m_dial_offset - (theta / 360) * m_dial->maximum();
	m_dial->setSliderPosition(pos);
}

void RotationDialog::resetDialog() {
	m_roatation.Reset();
	m_interpolate_cb->setCurrentIndex(int(Interpolator::Type::bicubic_spline));
	m_dial->setValue(m_dial_offset);
	m_theta_le->setText("0.000000");
}

void RotationDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_roatation, &Rotation::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_roatation, &Rotation::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_roatation, &Rotation::Apply);
		break;
	}
	}

	setEnabledAll(true);
}





FastRotationDialog::FastRotationDialog(QWidget* parent) :ProcessDialog("FastRotation", QSize(250, 150), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {

	using enum FastRotation::Type;

	connect(this, &ProcessDialog::processDropped, this, &FastRotationDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &FastRotationDialog::Apply, &FastRotationDialog::showPreview, &FastRotationDialog::resetDialog);

	QString str = QString("Rotate 90\u00B0 Clockwise");
	m_90cw_rb = new QRadioButton(str, this);
	m_90cw_rb->setChecked(true);
	connect(m_90cw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CW); });
	m_90cw_rb->move(10, 10);

	str = QString("Rotate 90\u00B0 Counter-clockwise");
	m_90ccw_rb = new QRadioButton(str, this);
	connect(m_90ccw_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate90CCW); });
	m_90ccw_rb->move(10, 30);

	str = QString("Rotate 180\u00B0");
	m_180_rb = new QRadioButton(str, this);
	connect(m_180_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(rotate180); });
	m_180_rb->move(10, 50);

	m_hm_rb = new QRadioButton("Horizontal Mirror", this);
	connect(m_hm_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(horizontalmirror); });
	m_hm_rb->move(10, 70);

	m_vm_rb = new QRadioButton("Vertical Mirror", this);
	connect(m_vm_rb, &QRadioButton::toggled, this, [this]() {m_fastrotation.setFastRotationType(verticalmirror); });
	m_vm_rb->move(10, 90);

	show();
}

void FastRotationDialog::resetDialog() {
	m_90cw_rb->setChecked(true);
	m_fastrotation.setFastRotationType(FastRotation::Type::rotate90CW);
}

void FastRotationDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_fastrotation, &FastRotation::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_fastrotation, &FastRotation::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_fastrotation, &FastRotation::Apply);
		break;
	}
	}

	setEnabledAll(true);

}


static void AddLabeltoWidget(QWidget* widget, QLabel* label) {

	int x = widget->geometry().x();
	int y = widget->geometry().y();
	int h = widget->geometry().height();
	int fh = label->fontMetrics().height();
	int tw = label->fontMetrics().horizontalAdvance(label->text());

	label->move(x - tw, ((2 * y) + h - fh) / 2);
}


IntegerResampleDialog::IntegerResampleDialog(QWidget* parent) :ProcessDialog("IntegerResample", QSize(215, 180), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {

	using IR = IntegerResample;

	connect(this, &ProcessDialog::processDropped, this, &IntegerResampleDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &IntegerResampleDialog::Apply, &IntegerResampleDialog::showPreview, &IntegerResampleDialog::resetDialog);


	m_type_bg = new QButtonGroup(this);
	QRadioButton* rb = new QRadioButton("Downsample", this);
	rb->setChecked(true);
	rb->move(45, 50);
	m_type_bg->addButton(rb, int(IR::Type::downsample));

	rb = new QRadioButton("Upsample", this);
	m_type_bg->addButton(rb, int(IR::Type::upsample));
	rb->move(45, 75);

	connect(m_type_bg, &QButtonGroup::idClicked, this, [this](int id) { m_ir.setType(IR::Type(id)); });


	m_factor_sb = new QSpinBox(this);
	m_factor_sb->move(140, 10);
	AddLabeltoWidget(m_factor_sb, new QLabel("Resample Factor:   ", this));
	m_factor_sb->setRange(1, Pixel<uint8_t>::max());
	connect(m_factor_sb, &QSpinBox::valueChanged, this, [this](int value) {m_ir.setFactor(value); });

	m_method_combo = new QComboBox(this);
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

void IntegerResampleDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_ir, &IntegerResample::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_ir, &IntegerResample::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_ir, &IntegerResample::Apply);
		break;
	}
	}

	setEnabledAll(true);
}





template<typename T>
CropDialog::CropPreview<T>::CropPreview(CropDialog* cd, QWidget* parent) : m_cd(cd), PreviewWindow<T>(parent) {
	m_rb = new QMdiSubWindow(this);
	m_rb->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
	m_rb->setGeometry(20, 20, this->size().width() - 40, this->size().height() - 40);
	//QPalette p;
	//p.setColor(QPalette::ColorRole::Window, QColor(100,100,122,60));
	//m_rb->setPalette(p);
	m_rb->setStyleSheet("QMdiSubWindow {border-width: 3px; border-style: solid; border-color: rgb(75,0,130); background-color: rgba(50,50,70,20);}");

	m_rb->setAttribute(Qt::WA_StyledBackground);
	m_rb->show();
}

template<typename T>
QRect CropDialog::CropPreview<T>::cropRect()const {
	int x1 = (m_rb->geometry().x() + 3) * this->BinFactor();
	int w = (m_rb->geometry().width() - 6) * this->BinFactor();
	int y1 = (m_rb->geometry().y() + 3) * this->BinFactor();
	int h = (m_rb->geometry().height() - 6) * this->BinFactor();
	return QRect(x1, y1, w, h);
}
template class CropDialog::CropPreview<uint8_t>;
template class CropDialog::CropPreview<uint16_t>;
template class CropDialog::CropPreview<float>;

CropDialog::CropDialog(QWidget* parent) :ProcessDialog("CropImage", QSize(180, 80), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {
	
	//connect(this, &ProcessDialog::processDropped, this, &FastRotationDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &CropDialog::Apply, &CropDialog::showPreview, &CropDialog::resetDialog);

	m_image_sel = new QComboBox(this);
	m_image_sel->addItem("No Image Selected");
	m_image_sel->move(10, 10);
	for (auto sw : m_workspace->subWindowList())
		m_image_sel->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->ImageName());

	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendClose, this, &CropDialog::onWindowClose);
	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::sendOpen, this, &CropDialog::onWindowOpen);
	connect(m_image_sel, &QComboBox::activated, this, &CropDialog::onActivation_imageSelection); // show preview on activation

	show();
}

void CropDialog::onWindowOpen() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->ImageName();
	m_image_sel->addItem(str);
}

void CropDialog::onWindowClose() {
	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->ImageName();
	int index = m_image_sel->findText(str);

	m_image_sel->removeItem(index);
	m_image_sel->setCurrentIndex(0);
}

  void CropDialog::onActivation_imageSelection(int index) {

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->ImageName()) {

			switch (iwptr->Source().Bitdepth()) {
			case 8: {
				auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
				if (!iw8->previewExists())
					iw8->ShowPreview(new CropPreview<float>(this, iw8));
				break;
			}
			case 16: {
				auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
				if (!iw16->previewExists())
					iw16->ShowPreview(new CropPreview<float>(this, iw16));
				break;				
			}
			case -32: {
				auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
				if (!iw32->previewExists())
					iw32->ShowPreview(new CropPreview<float>(this, iw32));
				break;
			}
			}
			iwptr->Preview()->setWindowTitle(iwptr->ImageName() + " CropWindow");
		}

		else 
			reinterpret_cast<ImageWindow8*>(sw->widget())->ClosePreview();
	}
}

void CropDialog::resetDialog() {

	for (auto sw : m_workspace->subWindowList())
	 reinterpret_cast<ImageWindow8*>(sw->widget())->ClosePreview();

	onActivation_imageSelection(m_image_sel->currentIndex());
}

void CropDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	for (auto sw : m_workspace->subWindowList()) {
		auto iwptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		if (m_image_sel->currentText() == iwptr->ImageName()) {

			switch (iwptr->Source().Bitdepth()) {
			case 8: {
				auto iw8 = reinterpret_cast<ImageWindow8*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint8_t>*>(iw8->Preview())->cropRect());
				iw8->UpdateImage(m_crop, &Crop::Apply);
				iw8->ClosePreview();
				break;
			}
			case 16: {
				auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<uint16_t>*>(iw16->Preview())->cropRect());
			    iw16->UpdateImage(m_crop, &Crop::Apply);
				iw16->ClosePreview();
				break;
			}
			case -32: {
				auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
				m_crop.setRegion(reinterpret_cast<CropPreview<float>*>(iw32->Preview())->cropRect());
				iw32->UpdateImage(m_crop, &Crop::Apply);
				iw32->ClosePreview();
				break;
			}
			}
		}
	}

	setEnabledAll(true);
}







HomographyTransformationDialog::HomographyTransformationDialog(QWidget* parent) :ProcessDialog("ImageTransformation", QSize(300, 250), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {


	connect(this, &ProcessDialog::processDropped, this, &HomographyTransformationDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &HomographyTransformationDialog::Apply, &HomographyTransformationDialog::showPreview, &HomographyTransformationDialog::resetDialog);

	QLabel* label = new QLabel("Homography:", this);
	label->move(15, 5);

	m_layout = new QGridLayout(this);

	for (int j = 0; j < 3; ++j) {
		for (int i = 0; i < 3; ++i) {
			QString str = (i == j) ? "1.000000" : "0.000000";
			m_le_array[j * 3 + i] = new DoubleLineEdit(str, new DoubleValidator(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 6), this);
			connect(m_le_array[j * 3 + i], &DoubleLineEdit::editingFinished, this, &HomographyTransformationDialog::onEditingFinished);
			m_layout->addWidget(m_le_array[j * 3 + i], j, i);
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

void HomographyTransformationDialog::Apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		iwptr->UpdateImage(m_transformation, &HomographyTransformation::Apply);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		iw16->UpdateImage(m_transformation, &HomographyTransformation::Apply);
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		iw32->UpdateImage(m_transformation, &HomographyTransformation::Apply);
		break;
	}
	}

	setEnabledAll(true);
}