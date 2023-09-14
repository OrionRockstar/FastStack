#include "pch.h"
#include "Bitmap.h"
#include <iostream>

template<typename Image>
void Bitmap::Tobmp(Image& src) {

	if (src.is_float()) {

		for (int y = 0, yb = src.Rows() - 1; y < src.Rows(); ++y, --yb)
			for (int x = 0, xb = 0; x < src.Cols(); ++x)
				for (int ch = src.Channels() - 1; ch >= 0; --ch)
					bmp(xb++, yb) = src(x, y, ch) * 255;

	}

	else if (src.is_uint16()) {
		float m = 255.0f / 65535.0f;
		for (int y = 0, yb = src.Rows() - 1; y < src.Rows(); ++y, --yb)
			for (int x = 0, xb = 0; x < src.Cols(); ++x)
				for (int ch = src.Channels() - 1; ch >= 0; --ch)
					bmp(xb++, yb) = src(x, y, ch) * m;

	}

	else if (src.is_uint8()) {

		for (int y = 0, yb = src.Rows() - 1; y < src.Rows(); ++y, --yb)
			for (int x = 0, xb = 0; x < src.Cols(); ++x)
				for (int ch = src.Channels() - 1; ch >= 0; --ch)
					bmp(xb++, yb) = src(x, y, ch);

	}

}
template void Bitmap::Tobmp(Image8&);
template void Bitmap::Tobmp(Image16&);
template void Bitmap::Tobmp(Image32&);

void Bitmap::Frombmp(Image8& dst) {

	for (int y = 0, yb = dst.Rows() - 1; y < dst.Rows(); ++y, --yb)
		for (int x = 0, xb = 0; x < dst.Cols(); ++x)
			for (int ch = dst.Channels() - 1; ch >= 0; --ch)
				dst(x, y, ch) = bmp(xb++, yb);

}

template<typename Image>
void Bitmap::BuildBitmap(Image& img) {

	info_header.rows = img.Rows();
	info_header.cols = img.Cols();

	info_header.bits_per_pixel = (img.Channels() == 3) ? 24 : 8;

	if (info_header.bits_per_pixel == 24)
		padding_length = (4 - (info_header.cols * 3) % 4) % 4;

	bmp = Image8(img.Rows(), img.Cols() * img.Channels());
	Tobmp(img);

	info_header.sizeof_image = bmp.Total() + padding_length * bmp.Rows();

	//assumes all 8bit images are monochrome/grayscale
	//uses color table to align with bitmap standards
	if (info_header.bits_per_pixel <= 8) {
		file_header.data_offset += (256 * 4);
		color_data = std::make_unique<RGBA[]>(256);

		for (int i = 0; i < 256; i++)
			color_data[i].red = color_data[i].green = color_data[i].blue = i;

		info_header.num_colors_used = 256;
		info_header.num_colors_important = 256;

		for (auto& pixel : bmp)
			pixel = color_data[pixel].red;
	}

	file_header.file_size = file_header.data_offset + info_header.sizeof_image;//+24

}

void Bitmap::ResetMembers() {
	file_header.file_size = 0;
	file_header.reserved1 = 0;
	file_header.reserved2 = 0;
	file_header.data_offset = 54;

	info_header.sizeof_dib = 40;
	info_header.cols = 0;
	info_header.rows = 0;
	info_header.color_planes = 1;
	info_header.bits_per_pixel = 8;
	info_header.compression = 0;
	info_header.sizeof_image = 0;
	info_header.horizontal_resolution = 0;
	info_header.vertical_resolution = 0;
	info_header.num_colors_used = 0;
	info_header.num_colors_important = 0;

	bmp = Image8();
	color_data.release();
	padding_length = 0;
}

bool Bitmap::isBitmap(std::filesystem::path path) {

	std::ifstream ifs(path, std::ios::binary);
	char sig[2];
	ifs.read(sig, 2);

	if (sig[0] == 'B' && sig[1] == 'M')
		return true;

	return false;
}

void Bitmap::Read(std::filesystem::path path, Image8& dst) {

	if (!isBitmap(path))
		throw std::runtime_error("Invalid bitmap file");

	color_data.release();
	padding_length = 0;

	std::ifstream ifs(path, std::ios::binary);

	ifs.read((char*)&file_header, 14);
	ifs.read((char*)&info_header, sizeof(info_header));

	if (info_header.bits_per_pixel <= 8) {
		int c_size = pow(2, info_header.bits_per_pixel);
		color_data = std::make_unique<RGBA[]>(c_size);
		ifs.read((char*)color_data.get(), c_size * 4);
	}

	if (info_header.bits_per_pixel == 24) {

		bmp = Image8(info_header.rows, info_header.cols * 3);
		dst = Image8(info_header.rows, info_header.cols, 3);

		padding_length = (4 - (info_header.cols * 3) % 4) % 4;

		for (int y = 0; y < bmp.Rows(); ++y) {
			ifs.read((char*)&bmp[y * bmp.Cols()], bmp.Cols());
			ifs.seekg(ifs.tellg() + (std::streampos)padding_length); // passes over padded zeros

		}
	}

	else if (info_header.bits_per_pixel == 8) {
		bmp = Image8(info_header.rows, info_header.cols);
		dst = Image8(bmp);
		ifs.read((char*)bmp.data.get(), bmp.Total());

		for (auto& pixel : bmp)
			pixel = color_data[pixel].red;

	}

	ifs.close();

	Frombmp(dst);

	ResetMembers();
}

template<typename Image>
void Bitmap::Write(Image& src, std::filesystem::path path) {

	BuildBitmap(src);

	std::ofstream ofs(path, std::ios::binary);

	if (ofs) {
		ofs.write((char*)&file_header, sizeof(file_header));
		ofs.write((char*)&info_header, sizeof(info_header));

		if (color_data.get()) {
			ofs.write((char*)color_data.get(), 256 * 4);
		}

		for (int y = 0; y < bmp.Rows(); ++y) {
			ofs.write((char*)&bmp[y * bmp.Cols()], bmp.Cols());
			ofs.write((char*)&pad[0], padding_length);
		}
	}

	ResetMembers();

	ofs.close();
}
template void Bitmap::Write(Image8&, std::filesystem::path);
template void Bitmap::Write(Image16&, std::filesystem::path);
template void Bitmap::Write(Image32&, std::filesystem::path);
