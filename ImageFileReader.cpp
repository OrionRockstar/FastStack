#include "pch.h"
#include "ImageFileReader.h"
#include "FastStack.h"
#include "FITS.h"
#include "TIFF.h"
#include "Bitmap.h"

ImageFileReader::ImageFileReader(QMdiArea* workspace) {
	m_workspace = workspace;
}


Status ImageFileReader::Read(std::filesystem::path file_path) {
	Image8 img8;
	Image16 img16;
	Image32 img32;

	std::string ext = file_path.extension().string();
	std::string filename = file_path.filename().string();

	if (ext == ".fit" || ext == ".fits" || ext == ".fts") {
		FITS fits;
		fits.Open(file_path);
		switch (fits.GetFITSBitDepth()) {
		case 8: {
			fits.Read(img8);
			break;
		}
		case 16: {
			fits.Read(img16);
			break;
		}
		case -32: {
			fits.Read(img32);
			break;
		}
		}
		fits.Close();
	}

	else if (ext == ".tif" || ext == ".tiff") {
		TIFF tiff;
		tiff.Open(file_path);
		switch (tiff.GetTiffValue(TIFF::TIFFTAG::BitsPerSample)) {
		case 8: {
			tiff.Read(img8);
			break;
		}
		case 16: {
			tiff.Read(img16);
			break;
		}
		case 32: {
			tiff.Read(img32);
			break;
		}
		}
		tiff.Close();
	}

	else if (ext == ".bmp") {
		Bitmap bitmap;
		bitmap.Open(file_path);
		bitmap.Read(img8);
		bitmap.Close();
	}

	else
		return { false, "Unsupported File Type!" };

	int count = 0;
	for (auto sw : m_workspace->subWindowList()) {
		auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
		std::string img_name = ptr->ImageName().toStdString();
		if (filename == img_name || (file_path.stem().string() + "_" + std::to_string(count) + ext) == img_name)
			filename = file_path.stem().string() + "_" + std::to_string(++count) + ext;
	}

	if (img8.Exists())
		ImageWindow8* iw8 = new ImageWindow8(img8, filename.c_str(), m_workspace);

	if (img16.Exists())
		ImageWindow16* iw16 = new ImageWindow16(img16, filename.c_str(), m_workspace);

	if (img32.Exists())
		ImageWindow32* iw32 = new ImageWindow32(img32, filename.c_str(), m_workspace);

	return Status();
}
