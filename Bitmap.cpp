#include "pch.h"
#include "Bitmap.h"
#include <iostream>

void Bitmap::ReadBitmapHeader() {
	m_stream.seekg(0);
	m_stream.read((char*)&m_bmp_header, 14);
}

void Bitmap::ReadInfoHeader() {
	m_stream.seekg(14);
	m_stream.read((char*)&m_info_header, sizeof(m_info_header));

	m_rows = m_info_header.rows;
	m_cols = m_info_header.cols;
	m_channels = (m_info_header.bits_per_pixel == 8) ? 1 : 3;
	m_bitdepth = (m_info_header.bits_per_pixel == 24) ? 8 : m_info_header.bits_per_pixel;
	m_px_count = m_rows * m_cols;

	m_bits_per_pixel = m_info_header.bits_per_pixel;

	if (m_bits_per_pixel <= 8) {
		int c_size = pow(2, m_bits_per_pixel);
		color_table = std::vector<RGBA>(c_size);
		m_stream.read((char*)color_table.data(), c_size * 4);
	}

}

void Bitmap::WriteHeaders(const Image8* src, bool compression) {

	m_info_header.rows = src->Rows();
	m_info_header.cols = src->Cols();

	m_info_header.bits_per_pixel = (src->Channels() == 3) ? 24 : 8;

	if (m_info_header.bits_per_pixel == 24)
		padding_length = (4 - (m_info_header.cols * 3) % 4) % 4;

	else if (src->Channels() == 1)
		padding_length = (4 - (m_info_header.cols) % 4) % 4;

	m_info_header.sizeof_image = src->PxCount() + padding_length * src->Rows();

	//assumes all 8bit images are monochrome/grayscale
	//uses color table to align with bitmap standards
	if (m_info_header.bits_per_pixel <= 8) {
		m_bmp_header.data_offset += (256 * 4);
		color_table = std::vector<RGBA>(256);

		for (int i = 0; i < 256; i++)
			color_table[i].red = color_table[i].green = color_table[i].blue = i;

		m_info_header.num_colors_used = 256;
		m_info_header.num_colors_important = 256;

	}

	m_bmp_header.file_size = m_bmp_header.data_offset + m_info_header.sizeof_image;//+24

	if (compression)
		m_info_header.compression = 1;

	m_stream.write((char*)&m_bmp_header, sizeof(m_bmp_header));
	m_stream.write((char*)&m_info_header, sizeof(m_info_header));

	if (color_table.data()) {
		m_stream.write((char*)color_table.data(), 256 * 4);
	}
}

bool Bitmap::isGreyscale() {

	for (auto color : color_table)
		if (color.red != color.green || color.red != color.blue || color.green != color.blue)
			return false;

	return true;
}

void Bitmap::ReadFromColorTable(Image8& dst) {

	bool is_grey = isGreyscale();

	dst = Image8(m_rows, m_cols, (is_grey) ? 1 : 3);

	std::vector<uint8_t> buffer(dst.Cols());

	for (int y = dst.Rows() - 1; y >= 0; --y) {
		m_stream.read((char*)buffer.data(), buffer.size());
		m_stream.read((char*)&pad[0], padding_length);

		if (is_grey) {
			for (int x = 0; x < dst.Cols(); ++x)
				dst(x, y) = buffer[x];
		}

		else {
			for (int x = 0; x < dst.Cols(); ++x) {
				dst(x, y, 0) = color_table[buffer[x]].red;
				dst(x, y, 1) = color_table[buffer[x]].green;
				dst(x, y, 2) = color_table[buffer[x]].blue;
			}
		}
	}

	Close();

}

void Bitmap::Read8bitCompression(Image8& dst) {

	bool is_grey = isGreyscale();

	dst = Image8(m_rows, m_cols, (is_grey) ? 1 : 3);

	for (int x = 0, y = m_rows - 1;;) {
		rle8 run;

		m_stream.read((char*)&run, 2);

		//end of line
		if (run.count == 0 && run.index == 0)
			x = 0, y--;

		//end of file
		else if (run.count == 0 && run.index == 1)
			break;

		if (is_grey)
			for (int i = 0; i < run.count; ++i)
				dst(x++, y) = run.index;
		else
			for (int i = 0; i < run.count; ++i) {
				dst(x, y, 0) = color_table[run.index].red;
				dst(x, y, 1) = color_table[run.index].green;
				dst(x++, y, 2) = color_table[run.index].blue;
			}
	}

	/*for (int y = dst.Rows() - 1; y >= 0; --y) {
		int x = 0;
		while (x < dst.Cols()) {
			m_stream.read((char*)&run, 2);

			for (int i = 0; i < run.count; ++i)
				dst(x++, y) =
				run.index;
		}
		m_stream.seekg(m_stream.tellp() + (std::streampos)2);
	}*/

	Close();
}


bool Bitmap::isBitmap(std::filesystem::path path) {

	char sig[2];

	m_stream.seekg(0);
	m_stream.read(sig, 2);
	m_stream.seekg(0);

	if (sig[0] == 'B' && sig[1] == 'M')
		return true;

	return false;
}

void Bitmap::Open(std::filesystem::path path) {
	ImageFile::Open(path);

	if (!isBitmap(path))
		throw std::runtime_error("Invalid bitmap file");
}

void Bitmap::Create(std::filesystem::path path) {

	ImageFile::Create(path);

}

void Bitmap::Close() {

	ImageFile::Close();

	m_bmp_header = BitmapHeader();
	m_info_header = DIBHeader();

	color_table.clear();

	pad = { 0,0,0 };
	padding_length = 0;
}


template<typename T>
void Bitmap::Write8bitCompression(const Image<T>& src) {

	WriteHeaders(reinterpret_cast<const Image8*>(&src), true);

	std::vector<rle8> compress;// (src.Cols());
	compress.reserve(src.Cols());
	rle8 eol = rle8(0, 0);
	rle8 eof = rle8(0, 1);

	int file_size = m_bmp_header.data_offset;

	for (int y = src.Rows() - 1; y >= 0; --y) {

		compress.emplace_back(rle8(1, Pixel<uint8_t>::toType(src(0, y))));

		for (int x = 1, xb = 0; x < src.Cols(); ++x) {

			uint8_t val = Pixel<uint8_t>::toType(src(x, y));

			if (val == compress[xb].index) {
				if (compress[xb].count == 255)
					goto newcount;
				else
					compress[xb].count++;
			}

			else {
			newcount:
				compress.emplace_back(rle8(1, val));
				xb++;
			}

		}

		m_stream.write((char*)compress.data(), compress.size() * 2);
		file_size += (compress.size() * 2);
		compress.clear();
		m_stream.write((char*)&eol, 2);

	}

	m_stream.write((char*)&eof, 2);

	m_stream.seekp(2);
	m_stream.write((char*)&file_size, 4);

	Close();
}
template void Bitmap::Write8bitCompression(const Image8&);
template void Bitmap::Write8bitCompression(const Image16&);
template void Bitmap::Write8bitCompression(const Image32&);

void Bitmap::Read(Image8& dst) {

	ReadBitmapHeader();
	ReadInfoHeader();

	if (m_info_header.compression == 1)
		return Read8bitCompression(dst);

	int n = m_bits_per_pixel / 8;
	padding_length = (4 - (m_cols * n) % 4) % 4;

	if (m_info_header.bits_per_pixel <= 8)
		return ReadFromColorTable(dst);

	dst = Image8(m_rows, m_cols, 3);

	std::vector<uint8_t> buffer(dst.Cols() * n);//+cols for alpha

	m_stream.seekg(m_bmp_header.data_offset);

	for (int y = dst.Rows() - 1; y >= 0; --y) {

		m_stream.read((char*)buffer.data(), buffer.size());
		m_stream.read((char*)&pad[0], padding_length);

		for (int x = 0, xb = 0; x < dst.Cols(); ++x) {

			for (int ch = dst.Channels() - 1; ch >= 0; --ch) {
				dst(x, y, ch) = buffer[xb++];
			}

			if (m_bits_per_pixel == 32)
				xb++;
		}
	}

	Close();
}

template<typename T>
void Bitmap::Write(const Image<T>& src, bool compression) {

	if (compression && src.Channels() == 1)
		return Write8bitCompression(src);

	WriteHeaders(reinterpret_cast<const Image8*>(&src));

	std::vector<uint8_t> buffer(src.Cols() * src.Channels());

	for (int y = src.Rows() - 1; y >= 0; --y) {
		for (int x = 0, xb = 0; x < src.Cols(); ++x) {
			for (int ch = src.Channels() - 1; ch >= 0; --ch) {
				buffer[xb++] = Pixel<uint8_t>::toType(src(x, y, ch));
			}
		}
		m_stream.write((char*)buffer.data(), buffer.size());
		m_stream.write((char*)&pad[0], padding_length);
	}

	Close();
}
template void Bitmap::Write(const Image8&, bool);
template void Bitmap::Write(const Image16&, bool);
template void Bitmap::Write(const Image32&, bool);
