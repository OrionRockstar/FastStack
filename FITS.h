#pragma once
#include "ImageFile.h"
#include "Image.h"

class FITS : public ImageFile {

	struct FITSHeader {
		typedef std::array<char, 80> header_line;

		std::vector<header_line> header_block = std::vector<header_line>(36);
		int keyword_count = 0;

		FITSHeader() = default;

		FITSHeader(int bitpix, std::array<int, 3> axis, bool end = true) {
			AddLogicalKeyword("SIMPLE", true, "FASTStck FITS");
			AddIntegerKeyword("BITPIX", bitpix, "bitdepth of image");

			int naxis = 2;
			if (axis[2] == 3)
				naxis = 3;

			AddIntegerKeyword("NAXIS", naxis, "number of axis");
			AddIntegerKeyword("NAXIS1", axis[1]);
			AddIntegerKeyword("NAXIS2", axis[0]);

			if (naxis == 3)
				AddIntegerKeyword("NAXIS3", axis[2], "number of axis");

			if (end)
				EndHeader();
		}

	private:
		void AddKW(const std::string& keyword, char* hbp, int& iter);

		void AddKWC(const std::string& comment, char* hbp, int& iter);

	public:
		void AddLogicalKeyword(const std::string& keyword, bool boolean, const std::string& comment = "");

		void AddIntegerKeyword(const std::string& keyword, int integer, const std::string& comment = "");

		void AddStringKeyword(const std::string& keword, const std::string& value);

		void AddCommentKeyword(const std::string& data);

		void AddHistoryKeyword(const std::string& data);

		std::string GetKeyWordValue(const std::string& keyword);

		void ResizeHeaderBlock() {
			header_block.resize(header_block.size() + 36);
		}

		void EndHeader();

		void Read(std::fstream& stream);

		void Write(std::fstream& stream);

	};

	FITSHeader m_fits_header;

	std::streampos m_data_pos = 2880;

public:

	FITS() = default;

	FITS(const FITS& fits) {}

	FITS(FITS&& other) noexcept : ImageFile(std::move(other)) {

		//move fits header???
		m_data_pos = other.m_data_pos;
	}

	~FITS() {};

private:

	bool FindImageData();

	void ReadHeader();

	int GetKeyWordValue(const std::string& keyword) {
		return std::stoi(m_fits_header.GetKeyWordValue(keyword));
	}

	template<typename T>
	void WritePixels_8(const Image<T>& src);

	template<typename T>
	void WritePixels_16(const Image<T>& src);

	template<typename T>
	void WritePixels_float(const Image<T>& src);

public:

	int Rows() const noexcept { return m_rows; }

	int Cols() const noexcept { return m_cols; }

	int Channels() const noexcept { return m_channels; }


	bool is_FitsFile();

	int GetFITSBitDepth();


	void Open(std::filesystem::path path) override;

	void Create(std::filesystem::path path) override;

	void Close() override;


	template<typename T>
	void Read(Image<T>& dst);

	template<typename T>
	void ReadSome(T* buffer, std::array<int, 3>& start_point, int num_elements);

	void ReadAny(Image32& dst);

	template <typename T>
	void Write(const Image<T>& img, int new_bit_depth);
};
