#pragma once
#include <fstream>
#include<filesystem>
#include "Image.h"


class ImageFile {

public:
	enum class Type {
		FITS,
		TIFF,
		XISF,
		BMP,
		WMI
	};

protected:
	std::fstream m_stream;

private:
	std::unique_ptr<char[]> m_stream_buffer;
	std::streamsize m_size = 0;

	std::streamsize streamsize()const { return m_size; }

	Type m_type;
protected:
	uint32_t m_rows = 0;
	uint32_t m_cols = 0;
	uint32_t m_channels = 1;
	uint32_t m_px_count = 0;
	ImageType m_img_type = ImageType::UBYTE;

public:
	//ImageFile() = default;

	ImageFile(Type file_type) : m_type(file_type) {}

	ImageFile(ImageFile&& other) noexcept {

		m_stream = std::move(other.m_stream);

		m_stream_buffer = std::move(other.m_stream_buffer);
		m_size = other.m_size;

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;
		m_px_count = other.m_px_count;
	}

	uint32_t rows()const { return m_rows; }

	uint32_t cols()const { return m_cols; }

	uint32_t channels()const { return m_channels; }

	uint32_t pxCount()const { return m_px_count; }

	Type type()const { return m_type; }

	ImageType imageType()const { return m_img_type; }

private:
	void setBuffer() {

		m_size = cols() * typeSize(imageType());

		m_size = (streamsize() > 4096) ? streamsize() : 4096;

		m_stream_buffer = std::make_unique<char[]>(streamsize());
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), streamsize());
	}

protected:
	void resizeBuffer(std::streamsize size = 0) {

		if (size == 0)
			return setBuffer();

		if (size == this->streamsize())
			return;

		m_size = size;
		m_stream_buffer = std::make_unique<char[]>(streamsize());
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), streamsize());
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

		m_rows = 0;
		m_cols = 0;
		m_channels = 1;
		m_px_count = 0;

		m_img_type = ImageType::UBYTE;
	}

};