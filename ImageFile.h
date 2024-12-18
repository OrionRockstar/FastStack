#pragma once
#include <fstream>
#include<filesystem>
#include "Image.h"


enum class FileType {
	FITS,
	TIFF,
	XISF,
	BMP
};

class ImageFile {
protected:
	std::fstream m_stream;

private:
	std::unique_ptr<char[]> m_stream_buffer;
	std::streamsize m_size = 0;

protected:
	int m_rows = 1;
	int m_cols = 1;
	int m_channels = 1;
	int m_bitdepth = 8;
	int m_px_count = 1;
	ImageType m_img_type = ImageType::UBYTE;

	FileType m_type;

public:
	ImageFile() = default;

	ImageFile(ImageFile&& other) noexcept {

		m_stream = std::move(other.m_stream);

		m_stream_buffer = std::move(other.m_stream_buffer);
		m_size = other.m_size;

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;
		m_bitdepth = other.m_bitdepth;
		m_px_count = other.m_px_count;
	}

	//~ImageFile() {}

	int rows()const { return m_rows; }

	int cols()const { return m_cols; }

	int channels()const { return m_channels; }

	int16_t bitdepth()const { return m_bitdepth; }

	FileType type()const { return m_type; }

	ImageType imageType()const { return m_img_type; }

private:

	void setBuffer() {

		m_size = m_cols * typeSize(m_img_type);

		m_size = (m_size > 4096) ? m_size : 4096;

		m_stream_buffer = std::make_unique<char[]>(m_size);
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), m_size);

	}

protected:
	void resizeBuffer(std::streamsize size = 0) {

		if (size == 0)
			return setBuffer();

		if (size == m_size)
			return;

		m_size = size;
		m_stream_buffer = std::make_unique<char[]>(m_size);
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), m_size);
	}


	virtual void open(std::filesystem::path path) {
		m_stream.open(path, std::ios::in | std::ios::out | std::ios::binary);
	}

	virtual void create(std::filesystem::path path) {
		m_stream.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
	}

public:
	virtual void close() {

		m_stream.close();

		m_stream_buffer.reset();
		m_size = 0;

		m_rows = 1;
		m_cols = 1;
		m_channels = 1;
		m_bitdepth = 8;
		m_px_count = 1;

		m_img_type = ImageType::UBYTE;
	}

};