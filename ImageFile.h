#pragma once
#include <fstream>
#include<filesystem>


class ImageFile {
protected:
	std::fstream m_stream;

private:
	std::unique_ptr<char[]> m_stream_buffer;
	std::streamsize m_count = 0;

protected:
	int m_rows = 1;
	int m_cols = 1;
	int m_channels = 1;
	int m_bitdepth = 8;
	int m_px_count = 1;

	ImageFile() = default;

	ImageFile(ImageFile&& other) noexcept {

		m_stream = std::move(other.m_stream);

		m_stream_buffer = std::move(other.m_stream_buffer);
		m_count = other.m_count;

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;
		m_bitdepth = other.m_bitdepth;
		m_px_count = other.m_px_count;
	}

private:
	void SetBuffer() {

		m_count = m_cols;

		m_count *= SizeofBitdepth(m_bitdepth);

		m_count = (m_count > 4096) ? m_count : 4096;

		m_stream_buffer = std::make_unique<char[]>(m_count);
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), m_count);

	}

protected:
	void ResizeBuffer(std::streamsize count = 0) {

		if (count == 0)
			return SetBuffer();

		if (count == m_count)
			return;

		m_count = count;
		m_stream_buffer = std::make_unique<char[]>(m_count);
		m_stream.rdbuf()->pubsetbuf(m_stream_buffer.get(), m_count);
	}

	uint32_t SizeofBitdepth(int bitdepth)const noexcept {
		switch (bitdepth) {
		case 8:
			return 1;
		case 16:
			return 2;
		case -32:
			return 4;
		default:
			return 4;
		}
	}

	virtual void Open(std::filesystem::path path) {
		m_stream.open(path, std::ios::in | std::ios::binary);
	}

	virtual void Create(std::filesystem::path path) {
		m_stream.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
	}

	virtual void Close() {

		m_stream.close();

		m_stream_buffer.reset();
		m_count = 0;

		m_rows = 1;
		m_cols = 1;
		m_channels = 1;
		m_bitdepth = 8;
		m_px_count = 1;
	}

};