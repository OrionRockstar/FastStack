#include "pch.h"
#include "ChannelCombinationDialog.h"
#include "FastStack.h"

using CCD = ChannelCombinationDialog;

CCD::ChannelCombinationDialog(Workspace* workspace) : ProcessDialog("ChannelCombination", { 560, 165 }, workspace, false, false) {

	addColorSpaceBG();
	addRedInputs();
	addGreenInputs();
	addBlueInputs();

	for (auto sw : workspace->subWindowList()) {
		auto iw = imageRecast<>(sw->widget());
		if (iw->channels() == 1) {
			m_red_combo->addImage(iw);
			m_green_combo->addImage(iw);
			m_blue_combo->addImage(iw);
		}
	}

	this->show();
}

void CCD::addColorSpaceBG() {

	using CST = ColorSpace::Type;

	m_colorspace_bg = new QButtonGroup(this);

	GroupBox* gb = new GroupBox("ColorSpace", drawArea());
	gb->move(10, 10);
	gb->resize(200, 140);

	QGridLayout* gl = new QGridLayout;

	RadioButton* rb = new RadioButton("RGB", this);
	rb->setChecked(true);
	m_colorspace_bg->addButton(rb, (int)CST::rgb);
	gl->addWidget(rb, 0, 0);

	rb = new RadioButton("HSV", this);
	m_colorspace_bg->addButton(rb, (int)CST::hsv);
	gl->addWidget(rb, 1, 0);

	rb = new RadioButton("HSI", this);
	m_colorspace_bg->addButton(rb, (int)CST::hsi);
	gl->addWidget(rb, 2, 0);

	rb = new RadioButton("CIE XYZ", this);
	m_colorspace_bg->addButton(rb, (int)CST::ciexyz);
	gl->addWidget(rb, 0, 1);

	rb = new RadioButton("CIE L*a*b*", this);
	m_colorspace_bg->addButton(rb, (int)CST::cielab);
	gl->addWidget(rb, 1, 1);

	rb = new RadioButton("CIE L*c*h*", this);
	m_colorspace_bg->addButton(rb, (int)CST::cielch);
	gl->addWidget(rb, 2, 1);

	connect(m_colorspace_bg, &QButtonGroup::idClicked, this, [this](int id) {m_cc.setColorspace(ColorSpace::Type(id)); });
	gb->setLayout(gl);
}

void CCD::onImageWindowCreated() {

	auto iw = imageRecast<>(workspace()->subWindowList().last()->widget());

	if (iw->channels() == 1) {
		m_red_combo->addImage(iw);
		m_green_combo->addImage(iw);
		m_blue_combo->addImage(iw);
	}
}

void CCD::onImageWindowClosed() {

	auto img = &imageRecast<>(workspace()->currentSubWindow()->widget())->source();

	int index = m_red_combo->findImage(img);
	if (index == m_red_combo->currentIndex()) {
		m_cc.setRed(nullptr);
		m_red_combo->setCurrentIndex(0);
	}
	m_red_combo->removeItem(index);

	index = m_green_combo->findImage(img);
	if (index == m_green_combo->currentIndex()) {
		m_cc.setGreen(nullptr);
		m_green_combo->setCurrentIndex(0);
	}
	m_green_combo->removeItem(index);

	index = m_blue_combo->findImage(img);
	if (index == m_blue_combo->currentIndex()) {
		m_cc.setBlue(nullptr);
		m_blue_combo->setCurrentIndex(0);
	}
	m_blue_combo->removeItem(index);
}

void CCD::addRedInputs() {

	m_red_cb = new CheckBox("Red", drawArea());
	m_red_cb->setChecked(true);
	m_red_cb->move(225, 30);
	connect(m_red_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableRed(v); });

	m_red_combo = new ImageComboBox(drawArea());
	m_red_combo->setFixedWidth(250);
	m_red_combo->addItem("No Selected Image", 0);
	m_red_combo->move(300, 28);

	connect(m_red_combo, &QComboBox::activated, this, [this]() { m_cc.setRed(m_red_combo->currentImage()); });
}

void CCD::addGreenInputs() {

	m_green_cb = new CheckBox("Green", drawArea());
	m_green_cb->setChecked(true);
	m_green_cb->move(225, 70);
	connect(m_green_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableGreen(v); });

	m_green_combo = new ImageComboBox(drawArea());
	m_green_combo->setFixedWidth(250);
	m_green_combo->addItem("No Selected Image", 0);
	m_green_combo->move(300, 68);

	connect(m_green_combo, &QComboBox::activated, this, [this]() { m_cc.setGreen(m_green_combo->currentImage()); });
}

void CCD::addBlueInputs() {

	m_blue_cb = new CheckBox("Blue", drawArea());
	m_blue_cb->setChecked(true);
	m_blue_cb->move(225, 110);
	connect(m_blue_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableBlue(v); });

	m_blue_combo = new ImageComboBox(drawArea());
	m_blue_combo->setFixedWidth(250);
	m_blue_combo->addItem("No Selected Image", 0);
	m_blue_combo->move(300, 108);

	connect(m_blue_combo, &QComboBox::activated, this, [this]() { m_cc.setBlue(m_blue_combo->currentImage()); });
}

void CCD::resetDialog() {

	m_cc = ChannelCombination();
	m_colorspace_bg->button(int(m_cc.colorspace()))->setChecked(true);

	m_red_cb->setChecked(m_cc.isRedEnabled());
	m_red_combo->setCurrentIndex(0);

	m_green_cb->setChecked(m_cc.isGreenEnabled());
	m_green_combo->setCurrentIndex(0);

	m_blue_cb->setChecked(m_cc.isBlueEnabled());
	m_blue_combo->setCurrentIndex(0);
}

void CCD::apply() {

	Status status = m_cc.isImagesSameSize();

	if (status) {
		Image32  rgb = m_cc.generateRGBImage();
		ImageWindow32* iw = new ImageWindow32(std::move(rgb), "RGB Image", workspace());
	}

	else
		QMessageBox::information(this, "", status.m_message);
}