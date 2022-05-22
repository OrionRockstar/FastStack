#include "Image.h"

Image::Image(int r, int c, int bitdepth) :rows(r), cols(c), type(bitdepth) {
	switch (bitdepth) {
	case 8:
		Image::data = new unsigned char[r * c];
		break;
	case 16:
		//Image::data16 = new unsigned short[r * c];
		Image::data = new unsigned short[r * c];
		break;
	case 32:
		Image::data = new float[r * c];
		break;
	default:
		Image::data = nullptr;

	}
}

Image::Image(const Image &img) {
	rows = img.rows;
	cols = img.cols;
	type = img.type;
	total = img.total;
	if (data)
		delete[] data;

	switch (type) {
	case 8:
		data = new unsigned char[total];
		memcpy(data, img.data, (size_t)total);
		break;
	case 16:
		data = new unsigned short[total];
		memcpy(data, img.data, (size_t)total * 2);
		break;
	case 32:
		data = new float[total];
		memcpy(data, img.data, (size_t)total * 4);
		break;
	}
}

Image Image::DeepCopy() {
	Image temp(Image::rows, Image::cols, Image::type);
	memcpy(temp.data, Image::data, (size_t)Image::total*(Image::type/8));
	return temp;
}

void Image::Release()
{
	if (data) {
		Image::~Image();
		Image::rows = 0,
		Image::cols = 0,
		Image::total = 0,
		Image::type = 0;
	}
}

void Image::Update(int r, int c, int bd) {
	Image::rows = r;
	Image::cols = c;
	Image::total = r * c;
	Image::type = bd;
	switch (bd) {
	case 8:
		Image::data = new unsigned char[r * c];
		break;
	case 16:
		Image::data = new unsigned short[r * c];
		break;
	case 32:
		Image::data = new float[r * c];
		break;
	default:
		Image::data = nullptr;
		break;
	}
}