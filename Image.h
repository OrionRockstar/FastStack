#pragma once
#include <memory>
class Image
{
public:
	//unsigned char* data8 = nullptr;
	//unsigned short* data16 = new unsigned short[0];
	//float* data32 = nullptr;
	void* data = nullptr;
	int rows = 0;
	int cols = 0;
	int total = rows * cols;
	int type = 0;

	Image(int rows, int cols, int bitdepth);
	Image() = default;
	Image(const Image& img);
	~Image() {
		delete[](void*)data;
		data = nullptr;
	}

	Image& operator=(Image &&img) {
		if (this != &img) {
			rows = img.rows;
			cols = img.cols;
			type = img.type;
			total = img.total;
			if (data) {
				delete[] data;
				data = nullptr;
			}
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
		return *this;
	}

	Image& operator=(const Image &img) {
		rows = img.rows;
		cols = img.cols;
		type = img.type;
		total = img.total;
		data = img.data;

		return *this;
	}

	template<typename Old, typename New>
	void Convert(Image &output) {

		Old* ptr = (Old*)Image::data;

		output.data = new New[output.total];
		New* nptr = (New*)output.data;

		std::copy(&ptr[0], &ptr[Image::total], &nptr[0]);

		switch (sizeof(New)) {
		case 1: output.type = 8; break;
		case 2: output.type = 16; break;
		case 4: output.type = 32; break;
		default: output.type = 0; break;
		}
		delete ptr;
	}

	Image DeepCopy();

	void Release();

	void Update(int r, int c, int bd);
};

