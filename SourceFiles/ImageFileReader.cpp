#include "pch.h"
#include "ImageFileReader.h"
#include "FastStack.h"
#include "FITS.h"
#include "TIFF.h"
#include "Bitmap.h"

ImageFileReader::ImageFileReader(Workspace* workspace) : m_workspace(workspace) {}

Status ImageFileReader::read(std::filesystem::path file_path) {
	Image8 img8;
	Image16 img16;
	Image32 img32;

	std::string ext = file_path.extension().string();
	for (auto& c : ext)
		c = std::tolower(c);

	std::string filename = file_path.filename().string();
	std::string name = file_path.stem().string();

	if (FITS::isFITS(filename)) {
		FITS fits;
		fits.open(file_path);
		switch (fits.imageType()) {
		case ImageType::UBYTE: {
			fits.read(img8);
			break;
		}
		case ImageType::USHORT: {
			fits.read(img16);
			break;
		}
		case ImageType::FLOAT: {
			fits.read(img32);
			break;
		}
		}
		fits.close();
	}

	else if (TIFF::isTIFF(filename)) {
		
		TIFF tiff;
		tiff.open(file_path);

		switch (tiff.imageType()) {
		case ImageType::UBYTE: {
			tiff.read(img8);
			break;
		}
		case ImageType::USHORT: {
			tiff.read(img16);
			break;
		}
		case ImageType::FLOAT: {
			tiff.read(img32);
			break;
		}
		}
		tiff.close();
	}

	else if (ext == ".bmp") {
		Bitmap bitmap;
		bitmap.open(file_path);
		bitmap.Read(img8);
		bitmap.close();
	}

	else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
		QImageReader imgReader(file_path.string().c_str());
		imgReader.setAutoTransform(true);
		QImage img = imgReader.read();		
		if (img.format() == QImage::Format_RGB32 || img.format() == QImage::Format_ARGB32)
			img = img.convertToFormat(QImage::Format_RGB888);

		QImagetoImage(img, img8);
	}

	else
		return { false, "Unsupported File Type!" };


	if (img8.exists())
		ImageWindow8* iw8 = new ImageWindow8(std::move(img8), name.c_str(), m_workspace);

	if (img16.exists())
		ImageWindow16* iw16 = new ImageWindow16(std::move(img16), name.c_str(), m_workspace);

	if (img32.exists())
		ImageWindow32* iw32 = new ImageWindow32(std::move(img32), name.c_str(), m_workspace);

	return Status();
}
