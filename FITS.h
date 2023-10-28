#pragma once
#include "ImageFile.h"
#include "Image.h"

class FITS : public ImageFile {

	struct FITSHeader {
		typedef std::array<char, 80> header_line;

		std::vector<header_line> header_block = std::vector<header_line>(36);
		int keyword_count = 0;

		FITSHeader() = default;

		FITSHeader(int bitpix, std::array<int, 3> axis) {
			AddKeyWord("SIMPLE", "T          /complies with fits standard");
			AddKeyWord("BITPIX", std::to_string(bitpix) + "          /bitdepth of image");
			int naxis = 2;
			if (axis[2] == 3)
				naxis = 3;
			AddKeyWord("NAXIS", std::to_string(naxis) + "          /number of axis");
			AddKeyWord("NAXIS1", std::to_string(axis[1]) + "          /columns");
			AddKeyWord("NAXIS2", std::to_string(axis[0]) + "          /rows");

			if (naxis == 3)
				AddKeyWord("NAXIS3", std::to_string(axis[2]) + "          /channels");
		}

		template<typename Image>
		FITSHeader(Image& img, bool end = true) {

			AddKeyWord("SIMPLE", "T          /complies with fits standard");
			AddKeyWord("BITPIX", std::to_string(img.Bitdepth()) + "          /bitdepth of image");
			int naxis = 2;
			if (img.Channels() == 3)
				naxis = 3;
			AddKeyWord("NAXIS", std::to_string(naxis) + "          /number of axis");
			AddKeyWord("NAXIS1", std::to_string(img.Cols()) + "          /columns");
			AddKeyWord("NAXIS2", std::to_string(img.Rows()) + "          /rows");

			if (naxis == 3)
				AddKeyWord("NAXIS3", std::to_string(img.Channels()) + "          /channels");

			if (end)
				EndHeader();
		}


		void AddKeyWord(const std::string& keyword, const std::string& data);

		std::string GetKeyWordValue(const std::string& keyword);

		void AddCommentKeyword(const std::string& data);

		void AddHistoryKeyword(const std::string& data);

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

	template<typename Image>
	void WritePixels_8(Image& src, float bscale);

	template<typename Image>
	void WritePixels_16(Image& src, float bscale);

	template<typename Image>
	void WritePixels_float(Image& src, float bscale);

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

	template <typename Image>
	void Write(Image& img, int new_bit_depth);
};
