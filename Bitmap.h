#pragma once
#include "Image.h"
class Bitmap {

#pragma pack(push, 1)
	struct BitmapHeader {
		const char signature[2] = { 'B','M' };
		uint32_t file_size = 0; //bytes
		uint16_t reserved1 = 0;
		uint16_t reserved2 = 0;
		uint32_t data_offset = 54; //bytes
	};
#pragma pack(pop)

	struct DIBHeader {
		uint32_t sizeof_dib = 40; //bytes
		int32_t cols = 0;
		int32_t rows = 0;

		uint16_t color_planes = 1; //must be 1
		uint16_t bits_per_pixel = 8;
		uint32_t compression = 0;
		uint32_t sizeof_image = 0; //bytes

		int32_t horizontal_resolution = 0; //pixel_per_meter
		int32_t vertical_resolution = 0; //pixels_per_meter
		uint32_t num_colors_used = 0;
		uint32_t num_colors_important = 0;
	};

	/*struct ColorHeader {
		uint32_t alpha_mask = 0xff000000;
		uint32_t red_mask = 0x00ff0000;
		uint32_t green_mask = 0x0000ff00;
		uint32_t blue_mask = 0x000000ff;
		uint32_t color_space = 0x73524742;
		uint32_t unsused=0;
	};*/

	struct RGBA {
		uint8_t blue = 0;
		uint8_t green = 0;
		uint8_t red = 0;
		uint8_t a = 0x00;
	};

	BitmapHeader file_header;
	DIBHeader info_header;
	//ColorHeader color_header;

	Image8 bmp;
	std::unique_ptr<RGBA[]> color_data;
	std::array<uint8_t, 3> pad = { 0,0,0 };
	int padding_length = 0;

public:
	Bitmap() = default;

	~Bitmap() {}

private:
	template<typename Image>
	void Tobmp(Image & src);

	void Frombmp(Image8 & dst);

	template<typename Image>
	void BuildBitmap(Image & img);

	void ResetMembers();

public:
	bool isBitmap(std::filesystem::path path);

	void Read(std::filesystem::path path, Image8 & dst);

	template<typename Image>
	void Write(Image & src, std::filesystem::path path);
};

