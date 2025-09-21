#pragma once
#include "Image.h"
#include "Workspace.h"

struct Status {
	bool m_success = true;
	QString m_message = "";

	Status() = default;

	Status(bool success, QString message) : m_success(success), m_message(message) {}

	explicit operator bool()const { return m_success; }
};

template<typename T>
static void ImagetoQImage(const Image<T>& src, QImage& dst) {

	if (src.rows() != dst.height() || src.cols() != dst.width() || src.channels() != dst.depth() / 3) {

		QImage::Format format;

		if (src.channels() == 1)
			format = QImage::Format::Format_Grayscale8;

		else if (src.channels() == 3)
			format = QImage::Format::Format_RGB888;

		dst = QImage(src.cols(), src.rows(), format);
	}

	for (int ch = 0; ch < src.channels(); ++ch)
		for (int y = 0; y < src.rows(); ++y)
			for (int x = 0; x < src.cols(); ++x)
				dst.scanLine(y)[src.channels() * x + ch] = Pixel<uint8_t>::toType(src(x, y, ch));
}

template<typename T>
static void QImagetoImage(const QImage& src, Image<T>& dst) {

	int channels = 1;

	if (src.format() == QImage::Format_Grayscale8)
		channels = 1;

	else if (src.format() == QImage::Format_RGB888)
		channels = 3;
	
	dst = Image<T>(src.height(), src.width(), channels);

	for (int ch = 0; ch < dst.channels(); ++ch)
		for (int y = 0; y < dst.rows(); ++y)
			for (int x = 0; x < dst.cols(); ++x)
				dst(x, y, ch) = Pixel<T>::toType(src.scanLine(y)[channels * x + ch]);
}


class ImageFileReader {

	Workspace* m_workspace;

	inline static QString m_typelist =
		"All Accepted Formats(*.bmp *.fits *.fts *.fit *.jpg *.jpeg *.png *.tiff *.tif);;"
		"BMP file(*.bmp);;"
		"FITS file(*.fits *.fts *.fit);;"
		"JPEG file(*.jpg *.jpeg);;"
		"PNG file(*.png);;"
		"XISF file(*.xisf);;"
		"TIFF file(*.tiff *.tif)";

public:
	ImageFileReader(Workspace* workspace);

	static const QString& typelist() { return m_typelist; }

	Status read(std::filesystem::path file_path);
};

