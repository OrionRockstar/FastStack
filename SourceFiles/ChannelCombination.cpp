#include "pch.h"
#include "ChannelCombination.h"
#include "FastStack.h"
//#include "RGBColorSpace.h"

double ChannelCombination::redPixel(int el)const {

	if (m_R == nullptr || !m_enable_red)
		return 0.0;

	switch (m_R->type()) {
	case ImageType::UBYTE:
		return Pixel<double>::toType((*m_R)[el]);
	case ImageType::USHORT:
		return Pixel<double>::toType((*reinterpret_cast<const Image16*>(m_R))[el]);
	case ImageType::FLOAT:
		return Pixel<double>::toType((*reinterpret_cast<const Image32*>(m_R))[el]);
	default:
		return 0.0;
	}
}

double ChannelCombination::greenPixel(int el)const {

	if (m_G == nullptr || !m_enable_green)
		return 0.0;

	switch (m_G->type()) {
	case ImageType::UBYTE:
		return Pixel<double>::toType((*m_G)[el]);
	case ImageType::USHORT:
		return Pixel<double>::toType((*reinterpret_cast<const Image16*>(m_G))[el]);
	case ImageType::FLOAT:
		return Pixel<double>::toType((*reinterpret_cast<const Image32*>(m_G))[el]);
	default:
		return 0.0;
	}
}

double ChannelCombination::bluePixel(int el)const {

	if (m_B == nullptr)
		return 0.0;

	switch (m_B->type()) {
	case ImageType::UBYTE:
		return Pixel<double>::toType((*m_B)[el]);
	case ImageType::USHORT:
		return Pixel<double>::toType((*reinterpret_cast<const Image16*>(m_B))[el]);
	case ImageType::FLOAT:
		return Pixel<double>::toType((*reinterpret_cast<const Image32*>(m_B))[el]);
	default:
		return 0.0;
	}
}

Color<double> ChannelCombination::outputColor(const Color<double>& inp)const {

	using enum ColorSpace::Type;

	switch (m_color_space) {

	case rgb:
		return inp;

	case hsv:
		return ColorSpace::HSVtoRGB(inp.red(), inp.green(), inp.blue());

	case hsi:
		return ColorSpace::HSItoRGB(inp.red(), inp.green(), inp.blue());

	case ciexyz:
		return ColorSpace::XYZtoRGB(inp.red(), inp.green(), inp.blue());

	case cielab:
		return ColorSpace::CIELabtoRGB(inp.red(), inp.green(), inp.blue());

	case cielch:
		return ColorSpace::CIELchtoRGB(inp.red(), inp.green(), inp.blue());

	default:
		return inp;
	}
}

QSize ChannelCombination::outputSize()const {
	if (m_R)
		return QSize(m_R->cols(), m_R->rows());
	else if (m_G)
		return QSize(m_G->cols(), m_G->rows());
	else if (m_B)
		return QSize(m_B->cols(), m_B->rows());

	return { 0,0 };
}

Status ChannelCombination::isImagesSameSize()const {
	
	Status status(false, "Incompatible Image Sizes!");

	//red & green & blue
	if (m_R && m_G && m_B) {
		if (!m_R->isSameSize(*m_G) || !m_R->isSameSize(*m_B) || !m_G->isSameSize(*m_B))
			return status;
	}
	//red & green
	else if (m_R && m_G && m_B == nullptr) {
		if (!m_R->isSameSize(*m_G))
			return status;
	}
	//red && blue
	else if (m_R && m_B && m_G == nullptr) {
		if (!m_R->isSameSize(*m_B))
			return status;
	}
	//green & blue
	else if (m_G && m_B && m_R == nullptr) {
		if (!m_G->isSameSize(*m_B))
			return status;
	}

	//bool only_red = (m_R && m_G == nullptr && m_B == nullptr);
	//bool only_green = (m_R == nullptr && m_G && m_B == nullptr);
	//bool only_blue = (m_R && m_G == nullptr && m_B == nullptr);
	//if 


	return Status();
}

Image32 ChannelCombination::generateRGBImage() {
	using enum ColorSpace::Type;

	QSize s = outputSize();
	Image32 output(s.height(), s.width(), 3);

#pragma omp parallel for
	for (int y = 0; y < output.rows(); ++y) {
		for (int x = 0; x < output.cols(); ++x) {

			int el = y * output.cols() + x;
			auto color = Color<double>(redPixel(el), greenPixel(el), bluePixel(el));

			output.setColor(x, y, outputColor(color));
		}
	}
	return output;
}




ChannelCombinationDialog::ChannelCombinationDialog(QWidget* parent) : ProcessDialog("ChannelCombination", { 570, 180 }, FastStack::recast(parent)->workspace(), false, false) {

	using CCD = ChannelCombinationDialog;

	connectToolbar(this, &CCD::Apply, &CCD::showPreview, &CCD::resetDialog);

	addColorSpaceBG();
	addRedInputs();
	addGreenInputs();
	addBlueInputs();
	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::imageWindowClosed, this, &CCD::onWindowClose);
	connect(reinterpret_cast<const Workspace*>(m_workspace), &Workspace::imageWindowCreated, this, &CCD::onWindowOpen);

	this->show();
}

void ChannelCombinationDialog::addColorSpaceBG() {

	using CST = ColorSpace::Type;

	m_colorspace_bg = new QButtonGroup(this);

	GroupBox* gb = new GroupBox("ColorSpace", this);
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

void ChannelCombinationDialog::onWindowOpen() {

	auto iw = reinterpret_cast<ImageWindow8*>(m_workspace->subWindowList().last()->widget());

	if (iw->channels() == 1) {
		m_red_combo->addItem(iw->name());
		m_green_combo->addItem(iw->name());
		m_blue_combo->addItem(iw->name());
	}
}

void ChannelCombinationDialog::onWindowClose() {

	QString str = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget())->name();

	int index = m_red_combo->findText(str);
	if (index == m_red_combo->currentIndex()) {
		m_cc.setRed(nullptr);
		m_red_combo->setCurrentIndex(0);
	}
	m_red_combo->removeItem(index);


	index = m_green_combo->findText(str);
	if (index == m_green_combo->currentIndex()) {
		m_cc.setGreen(nullptr);
		m_green_combo->setCurrentIndex(0);
	}
	m_green_combo->removeItem(index);


	index = m_blue_combo->findText(str);
	if (index == m_blue_combo->currentIndex()) {
		m_cc.setBlue(nullptr);
		m_blue_combo->setCurrentIndex(0);
	}
	m_blue_combo->removeItem(index);
}

void ChannelCombinationDialog::addRedInputs() {

	m_red_cb = new CheckBox("Red", this);
	m_red_cb->setChecked(true);
	m_red_cb->move(225, 30);
	connect(m_red_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableRed(v); });

	m_red_combo = new ComboBox(this);
	m_red_combo->setFixedWidth(250);
	m_red_combo->addItem("No Selected Image", -1);
	m_red_combo->move(300, 28);

	for (auto sw : m_workspace->subWindowList())
		m_red_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

	auto activation = [this]() {
		for (auto sw : m_workspace->subWindowList()) {
			auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
			if (m_red_combo->currentText() == iw->name())
				return m_cc.setRed(&iw->source());
		}
		m_cc.setRed(nullptr);
	};
	connect(m_red_combo, &QComboBox::activated, this, activation);
}

void ChannelCombinationDialog::addGreenInputs() {

	m_green_cb = new CheckBox("Green", this);
	m_green_cb->setChecked(true);
	m_green_cb->move(225, 70);
	connect(m_green_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableGreen(v); });

	m_green_combo = new ComboBox(this);
	m_green_combo->setFixedWidth(250);
	m_green_combo->addItem("No Selected Image", 0);
	m_green_combo->move(300, 68);

	for (auto sw : m_workspace->subWindowList())
		m_green_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

	auto activation = [this]() {
		for (auto sw : m_workspace->subWindowList()) {
			auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
			if (m_green_combo->currentText() == iw->name())
				return m_cc.setGreen(&iw->source());
		}
		m_cc.setGreen(nullptr);
	};
	connect(m_green_combo, &QComboBox::activated, this, activation);
}

void ChannelCombinationDialog::addBlueInputs() {

	m_blue_cb = new CheckBox("Blue", this);
	m_blue_cb->setChecked(true);
	m_blue_cb->move(225, 110);
	connect(m_blue_cb, &QCheckBox::clicked, this, [this](bool v) { m_cc.enableBlue(v); });

	m_blue_combo = new ComboBox(this);
	m_blue_combo->setFixedWidth(250);
	m_blue_combo->addItem("No Selected Image", 0);
	m_blue_combo->move(300, 108);

	for (auto sw : m_workspace->subWindowList())
		m_blue_combo->addItem(reinterpret_cast<const ImageWindow8*>(sw->widget())->name());

	auto activation = [this]() {
		for (auto sw : m_workspace->subWindowList()) {
			auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
			if (m_blue_combo->currentText() == iw->name())
				return m_cc.setBlue(&iw->source());
		}
		m_cc.setBlue(nullptr);
	};
	connect(m_blue_combo, &QComboBox::activated, this, activation);
}

void ChannelCombinationDialog::resetDialog() {}

void ChannelCombinationDialog::Apply() {

	Status status = m_cc.isImagesSameSize();

	if (status) {
		Image32  rgb = m_cc.generateRGBImage();
		ImageWindow32* iw = new ImageWindow32(std::move(rgb), "RGB Image", m_workspace);
	}

	else
		QMessageBox::information(this, "", status.m_message);
}