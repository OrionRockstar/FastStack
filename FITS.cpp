#include "pch.h"
#include "FITS.h"

void FITS::FITSHeader::AddKeyWord(const std::string& keyword, const std::string& data) {

	if (keyword_count % 36 == 0 && keyword_count != 0)
		ResizeHeaderBlock();

	int iter = 0;
	char* hbp = &header_block[keyword_count][iter];


	for (; iter < keyword.length(); ++iter)
		hbp[iter] = keyword[iter];


	for (; iter < 8; ++iter)
		hbp[iter] = ' ';

	hbp[iter++] = '=';
	hbp[iter++] = ' ';

	for (int i = 0; i < data.length(); ++i, ++iter)
		hbp[iter] = data[i];

	for (; iter < 79; ++iter)
		hbp[iter] = ' ';

	hbp[79] = '\n';
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

	db[79] = '\n';

	keyword_count++;
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

	SetBuffer();

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

	dst.AutoRescale();

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

template<typename Image>
void FITS::WritePixels_8(Image& src, float bscale) {

	if (bscale == 1.0f) {
		m_stream.rdbuf()->pubsetbuf((char*)src.data.get(), src.TotalPxCount())->pubseekpos(2880);
		m_stream.write((char*)src.data.get(), src.TotalPxCount());
		return;
	}

	std::vector<uint8_t> buffer(src.Cols());
	std::streamsize mem_size = buffer.size();
	m_stream.rdbuf()->pubsetbuf((char*)buffer.data(), mem_size)->pubseekpos(2880);

	auto bi = buffer.begin();
	for (auto& pixel : src) {

		*bi++ = pixel * bscale;

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_8(Image8& src, float);
template void FITS::WritePixels_8(Image16& src, float);
template void FITS::WritePixels_8(Image32& src, float);

template<typename Image>
void FITS::WritePixels_16(Image& src, float bscale) {
	std::vector<int16_t> buffer(src.Cols());
	std::streamsize mem_size = buffer.size() * 2;
	m_stream.rdbuf()->pubsetbuf((char*)buffer.data(), mem_size)->pubseekpos(2880);

	auto bi = buffer.begin();

	if (bscale == 1.0f) {
		for (auto pixel : src) {

			*bi++ = _byteswap_ushort(pixel - 32768);

			if (bi == buffer.end()) {
				m_stream.write((char*)buffer.data(), mem_size);
				bi = buffer.begin();
			}
		}
		return;
	}


	for (auto pixel : src) {

		*bi++ = _byteswap_ushort(pixel * bscale - 32768);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_16(Image8&, float);
template void FITS::WritePixels_16(Image16&, float);
template void FITS::WritePixels_16(Image16&, float);

template<typename Image>
void FITS::WritePixels_float(Image& src, float bscale) {
	std::vector<float> buffer(src.Cols());
	std::streamsize mem_size = buffer.size() * 4;
	m_stream.rdbuf()->pubsetbuf((char*)buffer.data(), mem_size)->pubseekpos(m_data_pos);

	auto bi = buffer.begin();

	if (bscale == 1.0f) {

		for (auto pixel : src) {

			*bi++ = byteswap_float(pixel);

			if (bi == buffer.end()) {
				m_stream.write((char*)buffer.data(), mem_size);
				bi = buffer.begin();
			}
		}
		return;
	}

	for (auto& pixel : src) {

		*bi++ = byteswap_float(pixel * bscale);

		if (bi == buffer.end()) {
			m_stream.write((char*)buffer.data(), mem_size);
			bi = buffer.begin();
		}
	}
}
template void FITS::WritePixels_float(Image8&, float);
template void FITS::WritePixels_float(Image16&, float);
template void FITS::WritePixels_float(Image32&, float);

template <typename Image>
void FITS::Write(Image& src, int new_bit_depth) {

	m_fits_header = FITSHeader(src);
	m_fits_header.Write(m_stream);

	float bscale = 1.0f;

	switch (new_bit_depth) {
	case 8:
	{
		if (src.is_uint16())
			bscale /= 255;
		else if (src.is_float())
			bscale = 255;

		WritePixels_8(src, bscale);
		break;
	}
	case 16:
	{
		if (src.is_uint8())
			bscale = 255;
		else if (src.is_float())
			bscale = 65535;

		WritePixels_16(src, bscale);
		break;
	}
	case -32:
	{
		if (src.is_uint8())
			bscale /= 255;
		else if (src.is_uint16())
			bscale /= 65535;

		WritePixels_float(src, bscale);
		break;
	}
	}

	Close();
	//m_stream.close();
}
template void FITS::Write(Image8&, int);
template void FITS::Write(Image16&, int);
template void FITS::Write(Image32&, int);