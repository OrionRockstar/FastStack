#include "pch.h"
#include "FITS.h"

void FITS::FITSHeader::AddKW(const std::string& keyword, char* hbp, int& iter) {

	for (; iter < keyword.length(); ++iter)
		hbp[iter] = keyword[iter];

	for (; iter < 8; ++iter)
		hbp[iter] = ' ';

	hbp[iter++] = '=';
	hbp[iter++] = ' ';
}

void FITS::FITSHeader::AddKWC(const std::string& comment, char* hbp, int& iter) {
	hbp[iter++] = ' ';
	hbp[iter++] = '/';
	hbp[iter++] = ' ';
	for (char l : comment) {

		if (iter == 79)
			break;

		hbp[iter++] = l;
	}
}

void FITS::FITSHeader::AddLogicalKeyword(const std::string& keyword, bool boolean, const std::string& comment) {
	//byte number = iter + 1
	if (keyword_count % 36 == 0 && keyword_count != 0)
		ResizeHeaderBlock();

	int iter = 0;
	char* hbp = &header_block[keyword_count][0];

	AddKW(keyword, hbp, iter);

	for (; iter < 29; ++iter)
		hbp[iter] = ' ';

	if (boolean)
		hbp[iter++] = 'T';
	else
		hbp[iter++] = 'F';

	if (!comment.empty())
		AddKWC(comment, hbp, iter);

	for (; iter < 80; ++iter)
		hbp[iter] = ' ';

	keyword_count++;
}

void FITS::FITSHeader::AddIntegerKeyword(const std::string& keyword, int integer, const std::string& comment) {
	//byte number = iter + 1
	if (keyword_count % 36 == 0 && keyword_count != 0)
		ResizeHeaderBlock();

	int iter = 0;
	char* hbp = &header_block[keyword_count][iter];

	AddKW(keyword, hbp, iter);

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
		AddKWC(comment, hbp, iter);

	for (; iter < 80; ++iter)
		hbp[iter] = ' ';

	keyword_count++;
}

std::string FITS::FITSHeader::GetKeyWordValue(const std::string& keyword) {

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

			return value;

		}
	}

	// would be better to throw exception?!?!
	return "NoKeyWordFound";
}

void FITS::FITSHeader::EndHeader() {

	if (keyword_count % 36 == 0)
		ResizeHeaderBlock();

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

void FITS::FITSHeader::Read(std::fstream& stream) {

	stream.seekg(0);

	for (int kw = 0; ; ++kw) {

		if (kw != 0 && kw % 36 == 0)
			ResizeHeaderBlock();

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

void FITS::FITSHeader::Write(std::fstream& stream) {
	stream.write((char*)header_block.data(), header_block.size() * 80);
}



bool FITS::is_FitsFile() {

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

int FITS::GetFITSBitDepth() {

	m_bitdepth = GetKeyWordValue("BITPIX");

	return m_bitdepth;
}


void FITS::Open(std::filesystem::path path) {

	ImageFile::Open(path);

	if (!is_FitsFile())
		return;

	m_fits_header.Read(m_stream);

	m_data_pos = m_fits_header.header_block.size() * 80;

	m_bitdepth = GetKeyWordValue("BITPIX");
	int naxis = GetKeyWordValue("NAXIS");
	m_cols = GetKeyWordValue("NAXIS1");
	m_rows = GetKeyWordValue("NAXIS2");

	if (naxis > 2) {
		m_channels = GetKeyWordValue("NAXIS3");
	}

	else
		m_channels = 1;

	m_px_count = m_rows * m_cols;

	ResizeBuffer();
}

void FITS::Create(std::filesystem::path path) {
	ImageFile::Create(path);
}

void FITS::Close() {

	ImageFile::Close();

	m_fits_header = FITSHeader();
	m_data_pos = 2880;

}

template<typename T>
void FITS::Read(Image<T>& dst) {

	dst = Image<T>(m_rows, m_cols, m_channels);

	m_stream.seekg(m_data_pos);

	m_stream.read((char*)dst.data.get(), dst.TotalPxCount() * sizeof(T));

	if (m_bitdepth == 16) {
		for (auto& pixel : dst)
			pixel = _byteswap_ushort(pixel) + 32768;
	}

	if (m_bitdepth == -32) {
		for (auto& pixel : dst)
			pixel = byteswap_float(pixel);
	}

	if (dst.is_float())
		dst.Normalize();

	Close();
}
template void FITS::Read(Image8&);
template void FITS::Read(Image16&);
template void FITS::Read(Image32&);


template<typename T>
void FITS::ReadSome(T* buffer, std::array<int, 3>& start_point, int num_elements) {

	std::streampos offset = (start_point[2] * m_px_count + start_point[1] * m_cols + start_point[0]) * sizeof(T);

	m_stream.seekg(m_data_pos + offset);
	m_stream.read((char*)buffer, num_elements * sizeof(T));

	if (std::is_same<T, float>::value)
		for (int x = 0; x < num_elements; ++x)
			buffer[x] = byteswap_float(buffer[x]);

	else if (std::is_same<T, uint16_t>::value)
		for (int x = 0; x < num_elements; ++x)
			buffer[x] = _byteswap_ushort(buffer[x]);

}
template void FITS::ReadSome(uint8_t*, std::array<int, 3>&, int);
template void FITS::ReadSome(uint16_t*, std::array<int, 3>&, int);
template void FITS::ReadSome(float*, std::array<int, 3>&, int);

void FITS::ReadAny(Image32& dst) {

	dst = Image32(m_rows, m_cols, m_channels);

	m_stream.seekg(m_data_pos);

	if (m_bitdepth == 8) {
		std::vector<uint8_t> buffer(dst.Cols());
		int mem_size = buffer.size() * sizeof(uint8_t);

		for (int ch = 0; ch < dst.Channels(); ++ch) {
			for (int y = 0; y < dst.Rows(); ++y) {

				m_stream.read((char*)buffer.data(), mem_size);

				for (int x = 0; x < dst.Cols(); ++x) {
					dst(x, y, ch) = buffer[x] / 255.0f;
				}
			}
		}
	}

	else if (m_bitdepth == 16) {

		std::vector<int16_t> buffer(dst.Cols());
		int mem_size = buffer.size() * sizeof(int16_t);

		for (int ch = 0; ch < dst.Channels(); ++ch) {
			for (int y = 0; y < dst.Rows(); ++y) {
				m_stream.read((char*)buffer.data(), mem_size);
				for (int x = 0; x < dst.Cols(); ++x) {
					dst(x, y, ch) = uint16_t(_byteswap_ushort(buffer[x]) + 32768) / 65535.0f;
				}
			}
		}
	}

	else if (m_bitdepth == -32) {

		m_stream.read((char*)dst.data.get(), dst.TotalImage() * sizeof(float));

		for (auto& pixel : dst)
			pixel = byteswap_float(pixel);
	}

	Close();
}

template<typename T>
void FITS::WritePixels_8(const Image<T>& src) {

	if (std::is_same<T, uint8_t>::value) {
		m_stream.write((char*)src.data.get(), src.TotalPxCount());
		return;
	}

	std::vector<uint8_t> buffer(src.Cols());
	std::streamsize mem_size = buffer.size();

	auto bi = buffer.begin();

	for (const T& pixel : src) {

		*bi++ = Pixel<uint8_t>::toType(pixel);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_8(const Image8&);
template void FITS::WritePixels_8(const Image16&);
template void FITS::WritePixels_8(const Image32&);

template<typename T>
void FITS::WritePixels_16(const Image<T>& src) {

	std::vector<int16_t> buffer(src.Cols());
	std::streamsize mem_size = buffer.size() * 2;

	auto bi = buffer.begin();

	for (const T& pixel : src) {

		*bi++ = _byteswap_ushort(Pixel<uint16_t>::toType(pixel) - 32768);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_16(const Image8&);
template void FITS::WritePixels_16(const Image16&);
template void FITS::WritePixels_16(const Image32&);

template<typename T>
void FITS::WritePixels_float(const Image<T>& src) {

	std::vector<float> buffer(src.Cols());
	std::streamsize mem_size = buffer.size() * 4;

	auto bi = buffer.begin();

	for (const T& pixel : src) {

		*bi++ = byteswap_float(Pixel<float>::toType(pixel));

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_float(const Image8&);
template void FITS::WritePixels_float(const Image16&);
template void FITS::WritePixels_float(const Image32&);

template <typename T>
void FITS::Write(const Image<T>& src, int new_bit_depth) {

	ResizeBuffer(src.Cols() * SizeofBitdepth(new_bit_depth));

	m_fits_header = FITSHeader(new_bit_depth, { src.Rows(), src.Cols(), src.Channels() });
	m_fits_header.Write(m_stream);

	switch (new_bit_depth) {
		case 8:
		{
			WritePixels_8(src);
			break;
		}
		case 16:
		{
			WritePixels_16(src);
			break;
		}
		case -32:
		{
			WritePixels_float(src);
			break;
		}
	}

	int padding_length = ceil(float(m_stream.tellp()) / m_data_pos) * m_data_pos - m_stream.tellp();
	std::vector<uint8_t> zeros(padding_length, 0);
	m_stream.write((char*)zeros.data(), padding_length);

	Close();
}
template void FITS::Write(const Image8&, int);
template void FITS::Write(const Image16&, int);
template void FITS::Write(const Image32&, int);