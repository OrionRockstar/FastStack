#pragma once
#include "ImageFile.h"
#include "Image.h"
#include "Maths.h"

class FITS : public ImageFile {

	struct FITSHeader {
		typedef std::array<char, 80> header_line;

		std::vector<header_line> header_block = std::vector<header_line>(36);
		int keyword_count = 0;

		FITSHeader() = default;

		FITSHeader(int bitpix, std::array<uint32_t, 3> axis, bool end = true);

	private:
		void addKeyword(const std::string& keyword, char* hbp, int& iter);

		void addKeywordComment(const std::string& comment, char* hbp, int& iter);

	public:
		void addLogicalKeyword(const std::string& keyword, bool boolean, const std::string& comment = "");

		void addIntegerKeyword(const std::string& keyword, int integer, const std::string& comment = "");

		//void addStringKeyword(const std::string& keword, const std::string& value);

		//void addCommentKeyword(const std::string& data);

		//void addHistoryKeyword(const std::string& data);

		int keywordValue(const std::string& keyword);

		void resizeHeaderBlock() {
			header_block.resize(header_block.size() + 36);
		}

		void endHeader();

		void read(std::fstream& stream);

		void write(std::fstream& stream);

	};

	FITSHeader m_fits_header;

	std::streampos m_data_pos = 2880;

public:

	FITS() : ImageFile(Type::FITS) {};// = default;

	FITS(FITS&& other) noexcept : ImageFile(std::move(other)) {
		m_data_pos = other.m_data_pos;
	}


private:
	ImageType imageTypefromFile();

public:
	std::streampos dataPosition()const { return m_data_pos; }

	static bool isFITS(std::filesystem::path file_name) {

		std::string ext = file_name.extension().string();

		return (ext == ".fits" || ext == ".fit" || ext == ".fts");
	}

	bool isFITSFile();

	void open(std::filesystem::path path) override;

	void create(std::filesystem::path path) override;

	void close() override;

	template<typename T>
	void read(Image<T>& dst);

	void readAny(Image32& dst);

private:
	template<typename T>
	void readRow(T* dst, uint32_t row, uint32_t channel);

public:
	void readRow_toFloat(float* dst, uint32_t row, uint32_t channel);

private:
	template<typename T>
	void writePixels_8(const Image<T>& src);

	template<typename T>
	void writePixels_16(const Image<T>& src);

	template<typename T>
	void writePixels_float(const Image<T>& src);

public:
	template <typename T>
	void write(const Image<T>& src, ImageType new_type);
};
