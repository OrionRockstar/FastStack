#include "pch.h"
#include "FITS.h"

FITS::FITSHeader::FITSHeader(int bitpix, std::array<uint32_t, 3> axis, bool end) {

	addLogicalKeyword("SIMPLE", true, "FASTStck FITS");
	addIntegerKeyword("BITPIX", bitpix, "bitdepth of image");

	int naxis = 2;
	if (axis[2] == 3)
		naxis = 3;

	addIntegerKeyword("NAXIS", naxis, "number of axis");
	addIntegerKeyword("NAXIS1", axis[1]);
	addIntegerKeyword("NAXIS2", axis[0]);

	if (naxis == 3)
		addIntegerKeyword("NAXIS3", axis[2], "number of axis");

	if (end)
		endHeader();
}


void FITS::FITSHeader::addKeyword(const std::string& keyword, char* hbp, int& iter) {

	for (; iter < keyword.length(); ++iter)
		hbp[iter] = keyword[iter];

	for (; iter < 8; ++iter)
		hbp[iter] = ' ';

	hbp[iter++] = '=';
	hbp[iter++] = ' ';
}

void FITS::FITSHeader::addKeywordComment(const std::string& comment, char* hbp, int& iter) {
	hbp[iter++] = ' ';
	hbp[iter++] = '/';
	hbp[iter++] = ' ';
	for (char l : comment) {

		if (iter == 79)
			break;

		hbp[iter++] = l;
	}
}

void FITS::FITSHeader::addLogicalKeyword(const std::string& keyword, bool boolean, const std::string& comment) {
	//byte number = iter + 1
	if (keyword_count % 36 == 0 && keyword_count != 0)
		resizeHeaderBlock();

	int iter = 0;
	char* hbp = &header_block[keyword_count][0];

	addKeyword(keyword, hbp, iter);

	for (; iter < 29; ++iter)
		hbp[iter] = ' ';

	if (boolean)
		hbp[iter++] = 'T';
	else
		hbp[iter++] = 'F';

	if (!comment.empty())
		addKeywordComment(comment, hbp, iter);

	for (; iter < 80; ++iter)
		hbp[iter] = ' ';

	keyword_count++;
}

void FITS::FITSHeader::addIntegerKeyword(const std::string& keyword, int integer, const std::string& comment) {
	//byte number = iter + 1
	if (keyword_count % 36 == 0 && keyword_count != 0)
		resizeHeaderBlock();

	int iter = 0;
	char* hbp = &header_block[keyword_count][iter];

	addKeyword(keyword, hbp, iter);

	for (; iter < 29; ++iter)
		hbp[iter] = ' ';

	std::string val = std::to_string(integer);
	std::ranges::reverse(val);

	for (char l : val) {
		if (iter == 11)
			break;
		hbp[iter--] = l;
	}

	iter = 30;

	if (!comment.empty())
		addKeywordComment(comment, hbp, iter);

	for (; iter < 80; ++iter)
		hbp[iter] = ' ';

	keyword_count++;
}

int FITS::FITSHeader::keywordValue(const std::string& keyword) {

	for (auto hl : header_block) {
		bool matches = true;
		for (int i = 0; i < keyword.length(); ++i)
			if (hl[i] != keyword[i]) {
				matches = false;
				break;
			}
		if (matches) {
			std::string value;

			for (int i = 10; i < 80; ++i) {
				if (hl[i] == '/')
					break;

				else if (hl[i] != ' ')
					value.push_back(hl[i]);
			}

			return std::stoi(value);

		}
	}

	//return "NoKeyWordFound";
	return 0;
}

void FITS::FITSHeader::endHeader() {

	if (keyword_count % 36 == 0)
		resizeHeaderBlock();

	char* db = &header_block[keyword_count][0];
	db[0] = 'E';
	db[1] = 'N';
	db[2] = 'D';
	for (int i = 3; i < 80; ++i)
		db[i] = ' ';

	keyword_count++;

	while (keyword_count % 36 != 0) {
		db = &header_block[keyword_count][0];
		for (int i = 0; i < 80; ++i)
			db[i] = ' ';
		keyword_count++;
	}

}

void FITS::FITSHeader::read(std::fstream& stream) {

	stream.seekg(0);

	for (int kw = 0; ; ++kw) {

		if (kw != 0 && kw % 36 == 0)
			resizeHeaderBlock();

		char end[4] = { 'E','N','D',' ' };
		stream.read(header_block[kw].data(), 80);
		bool matches = true;

		for (int i = 0; i < 4; ++i)
			if (header_block[kw][i] != end[i]) {
				matches = false;
				break;
			}

		if (matches)
			return;
	}

}

void FITS::FITSHeader::write(std::fstream& stream) {
	stream.write((char*)header_block.data(), header_block.size() * 80);
}


ImageType FITS::imageTypefromFile() {

	switch (m_fits_header.keywordValue("BITPIX")) {
	case 8:
		return ImageType::UBYTE;
	case 16:
		return ImageType::USHORT;
	case -32:
		return ImageType::FLOAT;
	default:
		return ImageType::UBYTE;
	}
}

bool FITS::isFITSFile() {

	int cur_pos = m_stream.tellg();
	m_stream.seekg(0);

	std::array<char, 80> buffer;
	m_stream.read(buffer.data(), 80);

	std::string simple;
	for (int i = 0; i < 8; ++i) {
		if (buffer[i] != ' ')
			simple.push_back(buffer[i]);
	}

	if (simple != "SIMPLE")
		return false;

	for (int i = 10; i < 80; ++i) {
		if (buffer[i] = 'T') {
			m_stream.seekg(m_data_pos);
			return true;
		}

		else if (buffer[i] == 'F' || buffer[i] == '/')
			return false;
	}

	return false;
}

void FITS::open(std::filesystem::path path) {

	ImageFile::open(path);

	if (!isFITSFile())
		return;

	//m_type = FileType::FITS;

	m_fits_header.read(m_stream);

	m_data_pos = m_fits_header.header_block.size() * 80;

	m_img_type = imageTypefromFile();
	//m_bitdepth = m_fits_header.keywordValue("BITPIX");

	int naxis = m_fits_header.keywordValue("NAXIS");
	m_cols = m_fits_header.keywordValue("NAXIS1");
	m_rows = m_fits_header.keywordValue("NAXIS2");

	if (naxis > 2) 
		m_channels = m_fits_header.keywordValue("NAXIS3");
	else
		m_channels = 1;

	m_px_count = rows() * cols();

	resizeBuffer();
}

void FITS::create(std::filesystem::path path) {

	path += ".fits";

	ImageFile::create(path);
}

void FITS::close() {

	ImageFile::close();

	m_fits_header = FITSHeader();
	m_data_pos = 2880;

}

template<typename T>
void FITS::read(Image<T>& dst) {

	dst = Image<T>(rows(), cols(), channels());

	m_stream.seekg(m_data_pos);

	m_stream.read((char*)dst.data.get(), dst.TotalPxCount() * sizeof(T));

	if (m_img_type == ImageType::USHORT) {
		for (auto& pixel : dst)
			pixel = _byteswap_ushort(pixel) + 32768;
	}

	else if (m_img_type == ImageType::FLOAT) {
		for (auto& pixel : dst)
			pixel = byteswap_float(pixel);
	}

	if (dst.is_float())
		dst.normalize();

	close();
}
template void FITS::read(Image8&);
template void FITS::read(Image16&);
template void FITS::read(Image32&);

void FITS::readAny(Image32& dst) {

	dst = Image32(m_rows, m_cols, m_channels);

	m_stream.seekg(m_data_pos);

	switch (m_img_type) {
	case ImageType::UBYTE: {
		std::vector<uint8_t> buffer(dst.cols());
		int mem_size = buffer.size() * sizeof(uint8_t);

		for (int ch = 0; ch < dst.channels(); ++ch) {
			for (int y = 0; y < dst.rows(); ++y) {

				m_stream.read((char*)buffer.data(), mem_size);

				for (int x = 0; x < dst.cols(); ++x) {
					dst(x, y, ch) = buffer[x] / 255.0f;
				}
			}
		}
		break;
	}

	case ImageType::USHORT: {
		std::vector<int16_t> buffer(dst.cols());
		int mem_size = buffer.size() * sizeof(uint16_t);

		for (int ch = 0; ch < dst.channels(); ++ch) {
			for (int y = 0; y < dst.rows(); ++y) {
				m_stream.read((char*)buffer.data(), mem_size);
				for (int x = 0; x < dst.cols(); ++x) {
					dst(x, y, ch) = uint16_t(_byteswap_ushort(buffer[x]) + 32768) / 65535.0f;
				}
			}
		}
		break;
	}

	case ImageType::FLOAT: {
		m_stream.read((char*)dst.data.get(), dst.TotalPxCount() * sizeof(float));

		for (auto& pixel : dst)
			pixel = byteswap_float(pixel);
		dst.normalize();
		break;
	}
	}

	close();
}

template<typename T>
void FITS::readSome(T* buffer, const ImagePoint& start_point, int num_elements) {

	std::streampos offset = (start_point.channel() * m_px_count + start_point.y() * m_cols + start_point.x()) * sizeof(T);
	m_stream.seekg(m_data_pos + offset);
	m_stream.read((char*)buffer, num_elements * sizeof(T));

	if (std::is_same<T, float>::value)
		for (int x = 0; x < num_elements; ++x)
			buffer[x] = byteswap_float(buffer[x]);

	else if (std::is_same<T, uint16_t>::value)
		for (int x = 0; x < num_elements; ++x)
			buffer[x] = _byteswap_ushort(buffer[x]);

}
template void FITS::readSome(uint8_t*, const ImagePoint&, int);
template void FITS::readSome(uint16_t*, const ImagePoint&, int);
template void FITS::readSome(float*, const ImagePoint&, int);

void FITS::readSome_Any(float* dst, const ImagePoint& start_point, int num_elements) {

	size_t offset = (start_point.channel() * m_px_count + start_point.y() * m_cols + start_point.x());

	switch (m_img_type) {
	case ImageType::UBYTE: {
		std::vector<uint8_t> buff(num_elements);
		m_stream.seekg(m_data_pos + std::streampos(offset));
		m_stream.read((char*)buff.data(), num_elements);
		for (int x = 0; x < num_elements; ++x)
			dst[x] = Pixel<float>::toType(buff[x]);
		return;
	}
	case ImageType::USHORT: {
		std::vector<uint16_t> buffer(num_elements);
		offset *= 2;
		m_stream.seekg(m_data_pos + std::streampos(offset));
		m_stream.read((char*)buffer.data(), num_elements * 2);
		for (int x = 0; x < num_elements; ++x)
			dst[x] = Pixel<float>::toType(uint16_t(_byteswap_ushort(buffer[x]) + 32768));
		return;
	}
	case ImageType::FLOAT: {
		offset *= 4;
		m_stream.seekg(m_data_pos + std::streampos(offset));
		m_stream.read((char*)dst, num_elements * 4);
		for (int x = 0; x < num_elements; ++x)
			dst[x] = byteswap_float(dst[x]);
		return;
	}
	}
}

template<typename T>
void FITS::writePixels_8(const Image<T>& src) {

	if (std::is_same<T, uint8_t>::value) {
		m_stream.write((char*)src.data.get(), src.TotalPxCount());
		return;
	}

	std::vector<uint8_t> buffer(src.cols());
	std::streamsize mem_size = buffer.size();

	auto bi = buffer.begin();

	for (T pixel : src) {

		*bi++ = Pixel<uint8_t>::toType(pixel);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::writePixels_8(const Image8&);
template void FITS::writePixels_8(const Image16&);
template void FITS::writePixels_8(const Image32&);

template<typename T>
void FITS::writePixels_16(const Image<T>& src) {

	std::vector<int16_t> buffer(src.cols());
	std::streamsize mem_size = buffer.size() * 2;

	auto bi = buffer.begin();

	for (T pixel : src) {

		*bi++ = _byteswap_ushort(Pixel<uint16_t>::toType(pixel) - 32768);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::writePixels_16(const Image8&);
template void FITS::writePixels_16(const Image16&);
template void FITS::writePixels_16(const Image32&);

template<typename T>
void FITS::writePixels_float(const Image<T>& src) {

	std::vector<float> buffer(src.cols());
	std::streamsize mem_size = buffer.size() * 4;

	auto bi = buffer.begin();

	for (T pixel : src) {

		*bi++ = byteswap_float(Pixel<float>::toType(pixel));

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::writePixels_float(const Image8&);
template void FITS::writePixels_float(const Image16&);
template void FITS::writePixels_float(const Image32&);


template <typename T>
void FITS::write(const Image<T>& src, ImageType new_type) {

	resizeBuffer(src.cols() * typeSize(new_type));

	int m = (new_type == ImageType::FLOAT) ? -1 : 1;

	m_fits_header = FITSHeader(m * typeSize(new_type) * 8, { src.rows(), src.cols(), src.channels() });
	m_fits_header.write(m_stream);

	switch (new_type) {
	case ImageType::UBYTE: {
		writePixels_8(src);
		break;
	}
	case ImageType::USHORT: {
		writePixels_16(src);
		break;
	}
	case ImageType::FLOAT: {
		writePixels_float(src);
		break;
	}
	}

	int padding_length = ceil(float(m_stream.tellp()) / m_data_pos) * m_data_pos - m_stream.tellp();
	std::vector<uint8_t> zeros(padding_length, 0);
	m_stream.write((char*)zeros.data(), padding_length);

	close();
}
template void FITS::write(const Image8&, ImageType);
template void FITS::write(const Image16&, ImageType);
template void FITS::write(const Image32&, ImageType);