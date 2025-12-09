#include "pch.h"
#include "ImageFile.h"


void ImageFile::setBuffer() {

	m_size = cols() * typeSize(imageType());

	m_size = (streamsize() > 4096) ? streamsize() : 4096;

	m_stream_buffer = std::make_unique<char[]>(streamsize());
	m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), streamsize());
}

void ImageFile::resizeBuffer(std::streamsize size) {

	if (size == 0)
		return setBuffer();

	if (size == this->streamsize())
		return;

	m_size = size;
	m_stream_buffer = std::make_unique<char[]>(streamsize());
	m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), streamsize());
}

void ImageFile::open(std::filesystem::path path) {
	m_stream.open(path, std::ios::in | std::ios::out | std::ios::binary);
}

void ImageFile::create(std::filesystem::path path) {
	m_stream.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
}

void ImageFile::close() {

	m_stream.close();
	m_stream_buffer.reset();
	m_size = 0;

	m_rows = 0;
	m_cols = 0;
	m_channels = 1;
	m_px_count = 0;

	m_img_type = ImageType::UBYTE;
}





void WeightMapImage::writeHeader(const Image8& src, bool compression) {

	Header h;

	h.rows = src.rows();
	h.cols = src.cols();
	h.channels = src.channels();
	h.pixels_per_channel = src.pxCount();
	h.total_pixels = src.totalPxCount();
	h.compression = compression;

	m_stream.write((char*)&h, sizeof(h));
}


bool WeightMapImage::isWeightMapImage(const std::filesystem::path& path) {

	return (path.extension().string() == ".wmi");
}


void WeightMapImage::open(std::filesystem::path path) {

	ImageFile::open(path);

	Header h;
	m_stream.read((char*)&h, sizeof(h));

	m_rows = h.rows;
	m_cols = h.cols;
	m_channels = h.channels;
	m_px_count = h.pixels_per_channel;

	m_compression = h.compression;
}

void WeightMapImage::create(std::filesystem::path path) {

	path += ".wmi";

	ImageFile::create(path);
}

void WeightMapImage::read(Image8& dst) {

	dst = Image8(rows(), cols(), channels());

	if (!m_compression) {
		m_stream.read((char*)dst.data(), dst.totalPxCount());
		return close();
	}

	for (int x = 0, y = 0, ch = 0;;) {
		RLE run;

		m_stream.read((char*)&run, 2);

		//end of line
		if (run.count == 0 && run.value == 0)
			x = 0, y++;

		//end of file
		else if (run.count == 0 && run.value == 1)
			break;

		if (y == dst.rows())
			x = 0, y = 0, ch++;

		for (int i = 0; i < run.count; ++i)
			dst(x++, y, ch) = run.value;
	}

	close();
}

void WeightMapImage::write(const Image8& src, bool compression) {

	writeHeader(src, compression);

	if (!compression) {
		m_stream.write((char*)src.data(), src.totalPxCount());
		RLE eof(0, 1);
		m_stream.write((char*)&eof, 2);
		return close();
	}

	uint32_t file_size = 0;

	std::vector<RLE> compress;
	compress.reserve(src.cols());
	RLE eol = RLE(0, 0); // end of line
	RLE eof = RLE(0, 1); // end of file

	for (int ch = 0; ch < src.channels(); ++ch) {
		for (int y = 0; y < src.rows(); ++y) {

			compress.emplace_back(RLE(1, src(0, y)));

			for (int x = 1, xb = 0; x < src.cols(); ++x) {

				uint8_t val = src(x, y);

				if (src(x, y) == compress[xb].value) {
					if (compress[xb].count == 255)
						goto newcount;
					else
						compress[xb].count++;
				}

				else {
				newcount:
					compress.emplace_back(RLE(1, src(x, y)));
					xb++;
				}

			}

			m_stream.write((char*)compress.data(), compress.size() * sizeof(RLE));
			m_stream.write((char*)&eol, sizeof(RLE));
			file_size += (compress.size() * sizeof(RLE));
			compress.clear();
		}
	}

	m_stream.write((char*)&eof, 2);

	m_stream.seekp(3);
	m_stream.write((char*)&file_size, 4);

	close();
}