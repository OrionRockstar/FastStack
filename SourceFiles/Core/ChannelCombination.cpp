#include "pch.h"
#include "ChannelCombination.h"
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

	using CST = ColorSpace::Type;

	switch (m_color_space) {

	case CST::rgb:
		return inp;

	case CST::hsv:
		return ColorSpace::HSVtoRGB(inp.red(), inp.green(), inp.blue());

	case CST::hsi:
		return ColorSpace::HSItoRGB(inp.red(), inp.green(), inp.blue());

	case CST::ciexyz:
		return ColorSpace::XYZtoRGB(inp.red(), inp.green(), inp.blue());

	case CST::cielab:
		return ColorSpace::CIELabtoRGB(inp.red(), inp.green(), inp.blue());

	case CST::cielch:
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

