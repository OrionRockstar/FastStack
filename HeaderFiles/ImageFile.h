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
	void setBuffer();

protected:
	void resizeBuffer(std::streamsize size = 0);

	virtual void open(std::filesystem::path path);

	virtual void create(std::filesystem::path path);

public:
	virtual void close();
};





class WeightMapImage : public ImageFile {
#pragma pack(push, 1)
	struct Header {
		const char signature[3] = { 'W','M','I' };
		uint32_t file_size = 0; //bytes
		const uint16_t data_offset = 30;
		uint32_t rows = 0;
		uint32_t cols = 0;
		uint16_t channels = 0;
		uint32_t pixels_per_channel = 0;
		uint32_t total_pixels = 0;
		const int16_t bitdepth = 8;
		uint8_t compression = 1;
	};
#pragma pack(pop)

	struct RLE {
		uint8_t count = 0;
		uint8_t value = 0;
	};

	bool m_compression = true;

	void writeHeader(const Image8& src, bool compression = true);

public:

	WeightMapImage() : ImageFile(Type::WMI) {}
	
	static bool isWeightMapImage(const std::filesystem::path& path);

	void open(std::filesystem::path path)override;

	void create(std::filesystem::path path)override;

	void read(Image8& dst);

	void write(const Image8& src, bool compression = true);
};