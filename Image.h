#pragma once
#include"Matrix.h"
#include "RGBColorSpace.h"
#include<filesystem>
#include <fstream>
#include "Histogram.h"

struct Stats {
	float max = std::numeric_limits<float>::min();
	float min = std::numeric_limits<float>::max();
	float median = 0;
	float mean = 0;
	float stdev = 0;
	float mad = 0;
	float avgDev = 0;
	float bwmv = 0;

};

template<typename T>
class Pixel {
};

template<>
class Pixel<double> {
public:
	static double max() { return 1.0f; }

	static double min() { return 0.0f; }

	static double toType(uint8_t pixel) {
		return pixel / 255.0;
	}

	static double toType(uint16_t pixel) {
		return pixel / 65535.0;
	}

	static double toType(float pixel) {
		return double(pixel);
	}

	static double toType(double pixel) {
		return pixel;
	}

	static void fromType(double a, uint8_t& pixel) {
		pixel = a * 255;
	}

	static void fromType(double a, uint16_t& pixel) {
		pixel = a * 65535;
	}

	static void fromType(double a, float& pixel) {
		pixel = float(a);
	}

	static void fromType(double a, double& pixel) {
		pixel = a;
	}
};

template<>
class Pixel<float> {
public:
	static float max() { return 1.0f; }

	static float min() { return 0.0f; }

	static float toType(uint8_t pixel) {
		return pixel / 255.0;
	}

	static float toType(uint16_t pixel) {
		return pixel / 65535.0;
	}

	static float toType(float pixel) {
		return pixel;
	}

	static float toType(double pixel) {
		return float(pixel);
	}

	static void fromType(float a, uint8_t& pixel) {
		pixel = a * 255;
	}

	static void fromType(float a, uint16_t& pixel) {
		pixel = a * 65535;
	}

	static void fromType(float a, float& pixel) {
		pixel = a;
	}

	static void fromType(float a, double& pixel) {
		pixel = double(a);
	}
};
typedef Pixel<float> Pixelf;

template<>
class Pixel<uint16_t> {
public:
	static uint16_t max() { return 65535; }

	static uint16_t min() { return 0; }

	static uint16_t toType(uint8_t pixel) {
		return pixel * 257;
	}

	static uint16_t toType(uint16_t pixel) {
		return pixel;
	}

	static uint16_t toType(float pixel) {
		return pixel * 65535;
	}

	static uint16_t toType(double pixel) {
		return pixel * 65535;
	}

	static void fromType(uint16_t a, uint8_t& pixel) {
		pixel = a / 257;
	}

	static void fromType(uint16_t a, uint16_t& pixel) {
		pixel = a;
	}

	static void fromType(uint16_t a, float& pixel) {
		pixel = a / 65535.0f;
	}

	static void fromType(uint16_t a, double& pixel) {
		pixel = a / 65535.0;
	}
};

template<>
class Pixel<uint8_t> {
public:
	static uint8_t max() { return 255; }

	static uint8_t min() { return 0; }

	static uint8_t toType(uint8_t pixel) {
		return pixel;
	}

	static uint8_t toType(uint16_t pixel) {
		return pixel / 257;
	}

	static uint8_t toType(float pixel) {
		return pixel * 255.0f;
	}

	static uint8_t toType(double pixel) {
		return pixel * 255.0;
	}

	static void fromType(uint8_t a, uint8_t& pixel) {
		pixel = a;
	}

	static void fromType(uint8_t a, uint16_t& pixel) {
		pixel = a * 257;
	}

	static void fromType(uint8_t a, float& pixel) {
		pixel = a / 255.0f;
	}

	static void fromType(uint8_t a, double& pixel) {
		pixel = a / 255.0;
	}
};


template <typename T>
class Image {
private:
	int m_rows = 0;
	int m_cols = 0;
	int m_channels = 0;
	int m_pixel_count = m_rows * m_cols;
	int m_total_pixel_count = m_pixel_count * m_channels;

	//int m_total = m_rows * m_cols;
	//int m_total_image = m_total * m_channels;
	int16_t m_bitdepth = 0;

	std::vector<Stats> statistics;

	T* m_red = nullptr;
	T* m_green = nullptr;
	T* m_blue = nullptr;

public:
	std::unique_ptr<T[]> data;
	//std::vector<bool> weight_map;

	Matrix homography = Matrix(3,3).Identity();

	Image(int r, int c, int ch = 1) :m_rows(r), m_cols(c), m_channels(ch) {
		assert(ch == 1 || ch == 3);

		m_bitdepth = sizeof(T) * 8;

		if (is_float())
			m_bitdepth *= -1;

		data = std::make_unique<T[]>(r * c * ch);

		if (m_channels == 3) {
			m_red = data.get();
			m_green = data.get() + m_pixel_count;
			m_blue = data.get() + 2 * m_pixel_count;
		}

	}

	Image(Image<T>& other, bool copy = false) {
		*this = Image<T>(other.m_rows, other.m_cols, other.m_channels);
		if (copy)
			memcpy(this->data.get(), other.data.get(), other.m_total_pixel_count * sizeof(T));
	}

	Image() = default;

	Image(const Image& other) {
		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_pixel_count = other.m_pixel_count;
		m_total_pixel_count = other.m_total_pixel_count;

		//statistics = other.statistics;

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;

		homography = other.homography;
		memcpy(data.get(), other.data.get(), m_total_pixel_count * sizeof(T));
		//weight_map = other.weight_map;
	}

	Image(Image&& other) {

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_pixel_count = other.m_pixel_count;
		m_total_pixel_count = other.m_total_pixel_count;

		//statistics = std::move(other.statistics);

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;

		homography = other.homography;
		data = std::move(other.data);
		//weight_map = std::move(other.weight_map);

	}

	~Image() {}

	struct Iterator {
		using ValueType = T;
		using PointerType = ValueType*;
		using ReferenceType = ValueType&;
		using difference_type = ptrdiff_t;


		Iterator(PointerType ptr) : m_ptr(ptr) {};

		Iterator operator++() { m_ptr++; return *this; }

		Iterator operator++(int) {
			Iterator iterator = *this;
			++(*this);
			return iterator;
		}

		Iterator operator--() { m_ptr--; return *this; }

		Iterator operator--(int) {
			Iterator iterator = *this;
			--(*this);
			return iterator;
		}

		Iterator& operator+=(difference_type offset) noexcept {
			m_ptr += offset;
			return *this;
		}

		Iterator operator+(difference_type offset) noexcept {
			Iterator t = *this;
			t += offset;
			return t;
		}

		Iterator& operator-=(difference_type offset) noexcept {
			m_ptr -= offset;
			return *this;
		}

		Iterator operator-(difference_type offset) noexcept {
			Iterator t = *this;
			t -= offset;
			return t;
		}

		ReferenceType operator*() { return *m_ptr; }

		PointerType operator->() { return m_ptr; }

		bool operator ==(const Iterator& other) const {
			return m_ptr == other.m_ptr;
		}

		bool operator !=(const Iterator& other) const {
			return m_ptr != other.m_ptr;
		}


	private:
		PointerType m_ptr;

	};

	struct ConstIterator {
		using ValueType = T;
		using PointerType = const ValueType*;
		using ReferenceType = const ValueType&;
		using difference_type = ptrdiff_t;


		ConstIterator(PointerType ptr) : m_ptr(ptr) {};

		ConstIterator operator++() { m_ptr++; return *this; }

		ConstIterator operator++(int) {
			ConstIterator iterator = *this;
			++(*this);
			return iterator;
		}

		ConstIterator operator--() { m_ptr--; return *this; }

		ConstIterator operator--(int) {
			ConstIterator iterator = *this;
			--(*this);
			return iterator;
		}

		ConstIterator& operator+=(difference_type offset) noexcept {
			m_ptr += offset;
			return *this;
		}

		ConstIterator operator+(difference_type offset) noexcept {
			ConstIterator t = *this;
			t += offset;
			return t;
		}

		ConstIterator& operator-=(difference_type offset) noexcept {
			m_ptr -= offset;
			return *this;
		}

		ConstIterator operator-(difference_type offset) noexcept {
			ConstIterator t = *this;
			t -= offset;
			return t;
		}

		ReferenceType operator*()const { return *m_ptr; }

		PointerType operator->()const { return m_ptr; }

		bool operator ==(const ConstIterator& other) const {
			return m_ptr == other.m_ptr;
		}

		bool operator !=(const ConstIterator& other) const {
			return m_ptr != other.m_ptr;
		}


	private:
		PointerType m_ptr;

	};

	Image& operator=(Image&& other) {

		if (this != &other) {
			m_rows = other.m_rows;
			m_cols = other.m_cols;
			m_channels = other.m_channels;

			m_bitdepth = other.m_bitdepth;
			m_pixel_count = other.m_pixel_count;
			m_total_pixel_count = other.m_total_pixel_count;

			//statistics = std::move(other.statistics);

			//m_max_val = other.m_max_val;

			m_red = other.m_red;
			m_green = other.m_green;
			m_blue = other.m_blue;

			homography = other.homography;
			data = std::move(other.data);
		}
		return *this;
	}

	Image& operator+=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s += *o;
		return *this;
	}

	Image& operator+=(const Image& other) {
		for (int i = 0; i < m_total_pixel_count; ++i)
			data[i] += other[i];
		return *this;
	}

	Image& operator-=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s -= *o;
		return *this;
	}

	Image& operator-=(const Image& other) {
		for (int i = 0; i < m_total_pixel_count; ++i)
			data[i] -= other[i];
		return *this;
	}

	Image& operator*=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s *= *o;
		return *this;
	}

	Image& operator*=(const Image& other) {
		for (int i = 0; i < m_total_pixel_count; ++i)
			data[i] *= other[i];
		return *this;
	}

	Image& operator/=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s /= *o;
		return *this;
	}

	Image& operator/=(const Image& other) {
		for (int i = 0; i < m_total_pixel_count; ++i)
			data[i] /= other[i];
		return *this;
	}

	T& operator[](int el) {
		return data[el];
	}

	const T& operator[](int el) const {
		return data[el];
	}

	T& operator()(int x, int y) {
		return data[y * m_cols + x];
	}

	const T& operator()(int x, int y) const {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) {
		return data[ch * m_pixel_count + y * m_cols + x];
	}

	const T& operator()(int x, int y, int ch) const {
		return data[ch * m_pixel_count + y * m_cols + x];
	}

	Iterator begin() {
		return Iterator(this->data.get());
	}

	Iterator begin()const {
		return Iterator(this->data.get());
	}

	ConstIterator cbegin()const {
		return ConstIterator(this->data.get());
	}

	Iterator begin(int channel) {
		return Iterator(this->data.get() + channel * m_pixel_count);
	}

	ConstIterator cbegin(int channel)const {
		return ConstIterator(this->data.get() + channel * m_pixel_count);
	}

	Iterator end() {
		return Iterator(this->data.get() + m_total_pixel_count);
	}

	Iterator end()const {
		return Iterator(this->data.get() + m_total_pixel_count);
	}

	ConstIterator cend()const {
		return ConstIterator(this->data.get() + m_total_pixel_count);
	}

	Iterator end(int channel) {
		return Iterator(this->data.get() + (channel + 1) * m_pixel_count);
	}

	ConstIterator cend(int channel)const {
		return ConstIterator(this->data.get() + (channel + 1) * m_pixel_count);
	}

	bool Exists()const { return data != nullptr; }

	template<typename P>
	bool Matches(Image<P>& other)const {
		return (m_rows == other.Rows() && m_cols == other.Cols() && m_channels == other.Channels() && m_bitdepth == other.Bitdepth());
	}

	template <typename P>
	bool IsSameDim(Image<P>& other)const {
		return(m_rows == other.Rows() && m_cols == other.Cols() && m_channels == other.Channels());
	}


	int Rows()const { return m_rows; }

	int Cols()const { return m_cols; }

	int PxCount()const { return m_pixel_count; }

	int TotalPxCount()const { return m_total_pixel_count; }

	int Channels()const { return m_channels; }

	int16_t Bitdepth()const { return m_bitdepth; }

	bool isSame(const Image<T>& other)const {
		if (m_rows != other.m_rows)
			return false;
		if (m_cols != other.m_cols)
			return false;
		if (m_channels != other.m_channels)
			return false;
		return true;
	}


	void getRGB(int el, T& R, T& G, T& B)const {
		R = m_red[el];
		G = m_green[el];
		B = m_blue[el];
	}

	void getRGB(int x, int y, T& R, T& G, T& B)const {
		int el = y * m_cols + x;
		R = m_red[el];
		G = m_green[el];
		B = m_blue[el];
	}

	void setRGB(int el, T R, T G, T B) {
		m_red[el] = R;
		m_green[el] = G;
		m_blue[el] = B;
	}

	void getRGBf(int el, float& R, float& G, float& B)const {
		R = Pixel<float>::toType(m_red[el]);
		G = Pixel<float>::toType(m_green[el]);
		B = Pixel<float>::toType(m_blue[el]);
	}

	void getRGB(int el, double& R, double& G, double& B)const {
		R = Pixel<double>::toType(m_red[el]);
		G = Pixel<double>::toType(m_green[el]);
		B = Pixel<double>::toType(m_blue[el]);
	}
	
	void setRGBf(int el, float R, float G, float B) {
		m_red[el] = Pixel<T>::toType(R);
		m_green[el] = Pixel<T>::toType(G);
		m_blue[el] = Pixel<T>::toType(B);
	}

	void setRGB(int el, double R, double G, double B) {
		m_red[el] = Pixel<T>::toType(R);
		m_green[el] = Pixel<T>::toType(G);
		m_blue[el] = Pixel<T>::toType(B);
	}


	void RGBtoGray() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double R, G, B;
			getRGB(el, R, G, B);

			Pixel<double>::fromType(ColorSpace::CIEL(R, G, B), data[el]);
		}

		T* temp = (T*)realloc(data.get(), m_pixel_count * sizeof(T));
		data.release();
		data.reset(temp);

		m_channels = 1;
		m_total_pixel_count = m_pixel_count;
	}

	void RGBtoCIELab() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double R, G, B, L, a, b;
			getRGB(el, R, G, B);
			ColorSpace::RGBtoCIELab(R, G, B, L, a, b);
			setRGB(el, L, a, b);
		}

	}

	void CIELabtoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double L, a, b, R, G, B;
			getRGB(el, L, a, b);
			ColorSpace::CIELabtoRGB(L, a, b, R, G, B);
			setRGB(el, R, G, B);
		}
	}

	void RGBtoCIELch() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double R, G, B, L, c, h;
			getRGB(el, R, G, B);
			ColorSpace::RGBtoCIELch(R, G, B, L, c, h);
			setRGB(el, L, c, h);
		}

	}

	void CIELchtoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double L, c, h, R, G, B;
			getRGB(el, L, c, h);
			ColorSpace::CIELchtoRGB(L, c, h, R, G, B);
			setRGB(el, R, G, B);
		}
	}

	void RGBtoHSI() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double R, G, B, H, S, I;
			getRGB(el, R, G, B);
			ColorSpace::RGBtoHSI(R, G, B, H, S, I);
			setRGB(el, H, S, I);
		}

	}

	void HSItoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_pixel_count; ++el) {
			double H, S, I, R, G, B;
			getRGB(el, H, S, I);
			ColorSpace::HSItoRGB(H, S, I, R, G, B);
			setRGB(el, R, G, B);
		}
	}

	/*void CreateWeightMap() {
		weight_map = std::vector<bool>(m_total, true);
	}

	int Weight_At(int x, int y) { return weight_map[y * m_cols + x]; }

	void Set_Weight_At(int x, int y, bool val) { weight_map[y * m_cols + x] = val; }*/


	void CopyTo(Image<T>& dest)const {
		if (dest.m_rows != m_rows || dest.m_bitdepth != m_bitdepth)
			dest = Image<T>(m_rows, m_cols, m_channels);
		memcpy(dest.data.get(), data.get(), m_total_pixel_count * sizeof(T));
	}

	void MoveTo(Image<T>& dest) {
		dest = std::move(*this);
	}

	void Swap(Image<T>& other) {
		std::swap(*this, other);
	}


	void MoveStatsFrom(Image<T>& src) {
		statistics = std::move(src.statistics);
	}


	bool is_uint8()const { return std::is_same<T, uint8_t>::value; }

	bool is_uint16()const { return std::is_same<T, uint16_t>::value; }

	bool is_float()const { return std::is_same<T, float>::value; }


	bool IsInBounds(int x, int y)const {
		return (0 <= y && y < m_rows && 0 <= x && x < m_cols);
	}

	bool IsInBounds(int x, int y, int buffer)const {
		return(buffer <= y && y < m_rows - buffer && buffer <= x && x < m_cols - buffer);
	}

	//rename
	/*T MirroredEdgePixel(int x, int y, int ch)const {
		if (y < 0)
			y = -y;
		else if (y >= m_rows)
			y = 2 * m_rows - (y + 1);

		if (x < 0)
			x = -x;
		else if (x >= m_cols)
			x = 2 * m_cols - (x + 1);

		return (*this)(x, y, ch);
	}*/

	T At(int x, int y, int ch)const {
		if (IsInBounds(x, y))
			return (*this)(x, y, ch);
		else
			return 0;
	}

	T At_mirrored(int x, int y, int ch)const {
		if (y < 0)
			y = -y;
		else if (y >= m_rows)
			y = 2 * m_rows - (y + 1);

		if (x < 0)
			x = -x;
		else if (x >= m_cols)
			x = 2 * m_cols - (x + 1);

		return (*this)(x, y, ch);
	}

	T ClipPixel(float pixel)const {
		if (pixel > Pixel<T>::max())
			return Pixel<T>::max();
		else if (pixel < 0)
			return 0;
		return pixel;
	}

	void FillZero() {
		for (auto& pixel : *this)
			pixel = 0;
	}

	void FillValue(T val) {
		for (auto& pixel : *this)
			pixel = val;
	}


	void Truncate(T a, T b) {
		for (T& pixel : *this) {
			if (pixel < a)
				pixel = a;
			else if (pixel > b)
				pixel = b;
		}
	}

	void Truncate(T a, T b, int ch) {

		for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
			if (*pixel < a)
				*pixel = a;
			else if (*pixel > b)
				*pixel = b;
		}

	}

	void Normalize() {
		T max, min;

		ComputeMinMax(min, max);

		if (min < Pixel<T>::min() || Pixel<T>::max() < max)
			Rescale(min, max);

	}

	void Rescale(T a, T b) {
		if (b == a)
			return;

		float dba = 1 / float(b - a);

		for (T& pixel : *this) {
			if (pixel < a)
				pixel = 0;
			else if (pixel > b)
				pixel = b;
			else
				pixel = (pixel - a) * dba;
		}

	}

	void Rescale(T a, T b, int ch) {
		if (b == a)
			return;

		float dba = 1 / float(b - a);

		for (T& pixel : image_channel(*this, ch)) {
			if (pixel < a)
				pixel = 0;
			else if (pixel > b)
				pixel = b;
			else
				pixel = T((pixel - a) * dba);
		}
	}

	void Binerize(T threshold) {
		for (auto& pixel : *this)
			pixel = (pixel >= threshold) ? 1 : 0;
	}


private:
	bool isClippedVal(uint8_t pixel)const {
		return (pixel == 0 || 255 == pixel);
	}

	bool isClippedVal(uint16_t pixel)const {
		return (pixel == 0 || 65535 == pixel);
	}

	bool isClippedVal(float pixel)const {
		return (pixel == 0 || 1 == pixel);
	}

	bool isClippedVal2(T pixel) {
		return (pixel == Pixel<T>::min() || pixel == Pixel<T>::max());
	}

public:
	//need to complete
	//return stats vector??
	void ComputeStatistics(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch) {
			float max = std::numeric_limits<float>::min();
			float min = std::numeric_limits<float>::max();

			int count = 0;
			double meansum = 0;
			//Histogram histogram((*this), ch);

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
				if (clip && isClippedVal(*pixel)) continue;

				if (*pixel > max)
					max = *pixel;
				if (*pixel < min)
					min = *pixel;

				meansum += *pixel;
				count++;

			}

			statistics[ch].max = max;
			statistics[ch].min = min;
			float median = statistics[ch].median;// = histogram.Median<T>(clip);
			float mean = statistics[ch].mean = meansum / count;

			//for (int i = 0; i < histogram.Size(); ++i)
				//histogram[i] = 0;

			double avgDevsum = 0;

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
				if (clip && isClippedVal(*pixel)) continue;

				float t = abs(*pixel - median);
				//histogram[(is_float()) ? t * 65535 : t]++;
				avgDevsum += t;

			}

			statistics[ch].avgDev = avgDevsum / count;

			statistics[ch].mad;// = histogram.Median<T>();

			double x9mad = 1 / (9 * statistics[ch].mad);
			double sum1 = 0, sum2 = 0;
			double Y, a;
			double d, var = 0;

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {

				if (clip && isClippedVal(*pixel)) continue;

				d = *pixel - mean;
				var += d * d;

				Y = (*pixel - median) * x9mad;

				(abs(Y) < 1) ? a = 1 : a = 0;

				Y *= Y;

				sum1 += (a * pow(*pixel - median, 2) * pow(1 - Y, 4));
				sum2 += (a * (1 - Y) * (1 - 5 * Y));

			}

			statistics[ch].stdev = sqrt(var / count);
			statistics[ch].bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
		}
	}

private:
	float ComputeMean_Clipped(int ch)const {

		double sum = 0;
		int count = 0;

		for (const T& pixel : image_channel(*this, ch)) {
			if (isClippedVal(pixel))
				continue;
			sum += pixel;
			count++;
		}

		return sum / count;
	}

	float ComputeStdDev_Clipped(int ch, float mean)const {

		double d, var = 0;
		int count = 0;

		for (const T& pixel : image_channel(*this, ch)) {
			if (isClippedVal(pixel))
				continue;
			d = pixel - mean;
			var += d * d;
			count++;
		}

		return sqrt(var / count);
	}

	float ComputeAvgDev_Clipped(int ch, T median)const {

		double sum = 0;
		int count = 0;

		for (const T& pixel : image_channel(*this, ch)) {
			if (isClippedVal(pixel))
				continue;
			sum += fabs(pixel - median);
			count++;
		}

		return sum / count;
	}

	void ComputeMinMax(T& min, T& max) {

		max = std::numeric_limits<T>::min();
		min = std::numeric_limits<T>::max();

		for (auto pixel : *this) {
			if (pixel > max)
				max = pixel;
			else if (pixel < min)
				min = pixel;
		}
	}

public:
	void ComputeMinMax(T& min, T& max, int ch)const {

		max = Pixel<T>::min();
		min = Pixel<T>::max();

		for (const T& pixel : image_channel(*this, ch)) {
			if (pixel > max)
				max = pixel;
			else if (pixel < min)
				min = pixel;
		}
	}

	T ComputeMax(int ch)const {

		T max = std::numeric_limits<T>::min();

		for (const T& pixel : image_channel(*this, ch))
			if (pixel > max)
				max = pixel;

		return max;
	}

	T ComputeMin(int ch)const {

		T min = std::numeric_limits<T>::max();

		for (const T& pixel : image_channel(*this, ch))
			if (pixel < min)
				min = pixel;

		return min;
	}

	float ComputeMean(int ch, bool clip = false)const {

		if (clip)
			return ComputeMean_Clipped(ch);

		double sum = 0;

		for (const T& pixel : image_channel(*this, ch))
			sum += pixel;
		
		return sum / m_pixel_count;
	}

	T ComputeMedian(int ch, bool clip = false)const {

		Histogram histogram;
		histogram.ConstructHistogram(cbegin(ch), cend(ch));

		return histogram.Median<T>(clip);
	}

	float ComputeStdDev(int ch, float mean, bool clip = false)const {

		if (clip)
			return ComputeStdDev_Clipped(ch, mean);

		double d, var = 0;

		for (const T& pixel : image_channel(*this, ch)) {
			d = pixel - mean;
			var += d * d;
		}

		return sqrt(var / m_pixel_count);
	}

	float ComputeStdDev(int ch, bool clip = false)const {

		float mean = ComputeMean(ch, clip);

		return ComputeStdDev(ch, mean, clip);

	}

	float ComputeAvgDev(int ch, T median, bool clip = false)const {

		if (clip)
			return ComputeAvgDev_Clipped(ch, median);

		double sum = 0;

		for (const T& pixel : image_channel(*this, ch))
			sum += fabs(pixel - median);


		return sum / m_pixel_count;
	}

	float ComputeAvgDev(int ch, bool clip = false)const {

		T median = ComputeMedian(ch, clip);

		return ComputeAvgDev(ch, median, clip);

	}

	T ComputeMAD(int ch, T median, bool clip = false)const {

		Histogram histogram;
		histogram.ConstructMADHistogram(cbegin(ch), cend(ch), median, clip);

		return histogram.Median<T>(clip);
	}

	T ComputenMAD(int ch, T median, bool clip = false)const {

		Histogram histogram;
		histogram.ConstructMADHistogram(cbegin(ch), cend(ch), median, clip);

		return 1.4826 * histogram.Median<T>(clip);
	}

	T ComputeMAD(int ch, bool clip = false)const {

		T median = ComputeMedian(ch, clip);

		Histogram histogram;
		histogram.ConstructMADHistogram(cbegin(ch), cend(ch), median, clip);

		return histogram.Median<T>(clip);
	}

	float ComputeBWMV(int ch, T median, bool clip = false)const {

		T mad = ComputeMAD(ch, median, clip);

		double x9mad = 1 / (9 * mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		int count = 0;

		for (const T& pixel : image_channel(*this, ch)) {

			if (clip && isClippedVal(pixel)) continue;

			Y = (pixel - median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(pixel - median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));
			count++;
		}

		return sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
	}

	float ComputeBWMV(int ch, bool clip = false)const {

		T median = ComputeMedian(ch, clip);

		return ComputeBWMV(ch, median, clip);
	}


	/*friend void CreateStatsText(const std::filesystem::path& file_path, Image<T>& img) {

		std::ofstream myfile;
		std::filesystem::path temp = file_path;
		myfile.open(temp.replace_extension(".txt"));
		myfile << file_path.string() << "\n\n";

		for (int ch = 0; ch < img.Channels(); ++ch) {
			myfile << "Channel Number: " << ch << "\n";
			myfile << "Max: " << std::fixed << std::setprecision(7) << img.Max(ch) << "\n";
			myfile << "Min: " << img.Min(ch) << "\n";
			myfile << "Median: " << img.Median(ch) << "\n";
			myfile << "Mean: " << img.Mean(ch) << "\n";
			myfile << "Standard Deviation: " << img.StdDev(ch) << "\n";
			myfile << "MAD: " << img.MAD(ch) << "\n";
			myfile << "AvgDev: " << img.AvgDev(ch) << "\n";
			myfile << "BWMV: " << img.BWMV(ch) << "\n\n";
		}
		myfile << "Homography: " << img.homography(0, 0) << "," << img.homography(0, 1) << "," << img.homography(0, 2)
			<< "," << img.homography(1, 0) << "," << img.homography(1, 1) << "," << img.homography(1, 2) << ",\n\n";

		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x)
				if (!img.Weight_At(x, y))
					myfile << x << "," << y << "\n";
		}
		myfile.close();
	}

	friend bool ReadStatsText(const std::filesystem::path& file_path, Image<T>& img) {

		std::filesystem::path temp = file_path;
		std::string line;
		std::ifstream myfile(temp.replace_extension(".txt"));
		int ch;

		if (myfile.is_open()) {

			while (std::getline(myfile, line)) {

				if (line.substr(0, 14) == "Channel Number")
					ch = std::stoi(line.substr(16, 17));

				if (line.substr(0, 3) == "Max")
					img.statistics[ch].max = std::stof(line.substr(5, std::string::npos));

				if (line.substr(0, 3) == "Min")
					img.statistics[ch].min = std::stof(line.substr(5, std::string::npos));

				if (line.substr(0, 6) == "Median")
					img.statistics[ch].median = std::stof(line.substr(8, std::string::npos));

				if (line.substr(0, 4) == "Mean")
					img.statistics[ch].mean = std::stof(line.substr(6, std::string::npos));

				if (line.substr(0, 19) == "Standard Deviation")
					img.statistics[ch].stdev = std::stof(line.substr(21, std::string::npos));

				if (line.substr(0, 3) == "MAD")
					img.statistics[ch].mad = std::stof(line.substr(5, std::string::npos));

				if (line.substr(0, 6) == "AvgDev")
					img.statistics[ch].avgDev = std::stof(line.substr(7, std::string::npos));

				if (line.substr(0, 4) == "BWMV")
					img.statistics[ch].bwmv = std::stof(line.substr(6, std::string::npos));

				if (line.substr(0, 10) == "Homography") {
					size_t start = 12;
					size_t comma = 12;
					int i = 0, j = 0;
					while ((comma = line.find(",", comma)) != std::string::npos) {
						img.homography(i, j) = std::stod(line.substr(start, comma - start));
						start = (comma += 1);
						j += 1;
						if (j == 3) { i++, j = 0; }
					}
				}
			}

			myfile.close();
			return true;
		}
		return false;
	}*/
};
template Image<uint8_t>;
template Image<uint16_t>;
template Image<float>;


typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;


template <typename T>
class ImageChannel {
	Image<T>::Iterator b = Image<T>().begin();
	Image<T>::Iterator e = Image<T>().end();

public:

	ImageChannel(Image<T>& img, int ch) {
		b = img.begin(ch);
		e = img.end(ch);
	}

	Image<T>::Iterator begin() { return b; }
	Image<T>::Iterator end() { return e; }

};

template <typename T>
class ConstImageChannel {
	Image<T>::ConstIterator cb = Image<T>().cbegin();
	Image<T>::ConstIterator ce = Image<T>().cend();

public:

	ConstImageChannel(const Image<T>& img, int ch) {
		cb = img.cbegin(ch);
		ce = img.cend(ch);
	}

	Image<T>::ConstIterator begin()const { return cb; }
	Image<T>::ConstIterator end()const { return ce; }

};

template <class T>
ImageChannel<T>
image_channel(Image<T>& img, int channel) {
	return ImageChannel<T>(img, channel);
}

template <class T>
ConstImageChannel<T>
image_channel(const Image<T>& img, int channel) {
	return ConstImageChannel<T>(img, channel);
}

typedef std::vector<Image32> ImageVector;
typedef std::vector<Image8> Image8Vector;
typedef std::vector<bool> WeightMap;
typedef std::vector<std::filesystem::path> FileVector;

enum class FastRotate {
	rotate90CW,
	rotate90CCW,
	rotate180,
	horizontalmirror,
	verticalmirror
};

bool StatsTextExists(const std::filesystem::path& file_path);

void GetImageStackFromTemp(FileVector& light_files, ImageVector& img_stack);
