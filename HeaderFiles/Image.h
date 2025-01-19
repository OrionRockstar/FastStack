#pragma once
//#include"Matrix.h"
#include "RGBColorSpace.h"
#include "Maths.h"

enum class ImageType : uint8_t {
	UBYTE,
	USHORT,
	FLOAT
};

template<typename T>
constexpr static ImageType getImageType() {
	if (std::is_same < T, uint8_t>::value)
		return ImageType::UBYTE;
	else if (std::is_same < T, uint16_t>::value)
		return ImageType::USHORT;
	else if (std::is_same < T, float>::value)
		return ImageType::FLOAT;
}

constexpr static size_t typeSize(ImageType type) {
	switch (type) {
	case ImageType::UBYTE:
		return 1;
	case ImageType::USHORT:
		return 2;
	case ImageType::FLOAT:
		return 4;
	default:
		return 1;
	}
}

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


template <typename T = uint8_t>
class Image {

private:
	uint32_t m_rows = 0;
	uint32_t m_cols = 0;
	uint32_t m_channels = 0;
	ImageType m_type = ImageType::UBYTE;

	uint32_t m_pixel_count = m_rows * m_cols;
	uint32_t m_total_pixel_count = m_pixel_count * m_channels;

	T* m_red = nullptr;
	T* m_green = nullptr;
	T* m_blue = nullptr;

public:
	std::unique_ptr<T[]> data;

	Image(uint32_t rows, uint32_t cols, uint32_t channels = 1);

	Image() = default;

	Image(const Image& other);

	Image(Image&& other)noexcept;

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

		int operator -(const Iterator& other) const {
			return m_ptr - other.m_ptr;
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

	Image& operator=(Image&& other)noexcept {

		if (this != &other) {
			m_rows = other.m_rows;
			m_cols = other.m_cols;
			m_channels = other.m_channels;

			m_type = other.m_type;
			m_pixel_count = other.m_pixel_count;
			m_total_pixel_count = other.m_total_pixel_count;

			//homography = other.homography;
			data = std::move(other.data);

			if (m_channels == 3) {
				m_red = data.get();
				m_green = data.get() + m_pixel_count;
				m_blue = data.get() + 2 * m_pixel_count;
			}
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

	const T& operator()(int x, int y)const {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) {
		return data[ch * m_pixel_count + y * m_cols + x];
	}

	const T& operator()(int x, int y, int ch)const {
		return data[ch * m_pixel_count + y * m_cols + x];
	}

	T& operator()(const Point& p) {
		return data[p.y() * m_cols + p.x()];
	}

	const T& operator()(const Point& p)const {
		return data[p.y() * m_cols + p.x()];
	}

	T& operator()(const ImagePoint& p) {
		return data[p.channel() * m_pixel_count + p.y() * m_cols + p.x()];
	}

	const T& operator()(const ImagePoint& p)const {
		return data[p.channel() * m_pixel_count + p.y() * m_cols + p.x()];
	}

	Iterator begin() {
		return Iterator(this->data.get());
	}

	Iterator begin()const {
		return Iterator(this->data.get());
	}

	Iterator begin(int channel) {
		return Iterator(this->data.get() + channel * m_pixel_count);
	}

	Iterator begin(int channel)const {
		return Iterator(this->data.get() + channel * m_pixel_count);
	}

	ConstIterator cbegin()const {
		return ConstIterator(this->data.get());
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

	Iterator end(int channel)const {
		return Iterator(this->data.get() + (channel + 1) * m_pixel_count);
	}

	ConstIterator cend()const {
		return ConstIterator(this->data.get() + m_total_pixel_count);
	}

	ConstIterator cend(int channel)const {
		return ConstIterator(this->data.get() + (channel + 1) * m_pixel_count);
	}

	uint32_t rows()const { return m_rows; }

	uint32_t cols()const { return m_cols; }

	uint32_t channels()const { return m_channels; }

	ImageType type()const { return m_type; }

	int pxCount()const { return m_pixel_count; }

	int totalPxCount()const { return m_total_pixel_count; }

	bool exists()const { return data != nullptr; }

	template <typename P>
	bool isSameSize(const Image<P>& other)const {
		return(rows() == other.rows() && cols() == other.cols());
	}

	template<typename P = T>
	bool isSameShape(const Image<P>& other)const {
		if (rows() != other.rows())
			return false;
		if (cols() != other.cols())
			return false;
		if (channels() != other.channels())
			return false;
		return true;
	}


private:
	template<typename P = T>
	Color<P> color(int el)const {

		P r = Pixel<P>::toType(m_red[el]);
		P g = Pixel<P>::toType(m_green[el]);
		P b = Pixel<P>::toType(m_blue[el]);

		return { r,g,b };
	}

	template<typename P = T>
	void setColor(int el, const Color<P>& color) {

		m_red[el] = Pixel<T>::toType(color.red());
		m_green[el] = Pixel<T>::toType(color.green());
		m_blue[el] = Pixel<T>::toType(color.blue());
	}

public:
	template<typename P = T>
	Color<P> color(int x, int y)const {

		int el = y * cols() + x;
		P r = Pixel<P>::toType(m_red[el]);
		P g = Pixel<P>::toType(m_green[el]);
		P b = Pixel<P>::toType(m_blue[el]);

		return { r,g,b };
	}

	template<typename P = T>
	Color<P> color(const Point& p)const {

		int el = p.y() * cols() + p.x();
		P r = Pixel<P>::toType(m_red[el]);
		P g = Pixel<P>::toType(m_green[el]);
		P b = Pixel<P>::toType(m_blue[el]);

		return { r,g,b };
	}

	template<typename P = T>
	void setColor(int x, int y, const Color<P>& color) {
		int el = y * cols() + x;
		m_red[el] = Pixel<T>::toType(color.red());
		m_green[el] = Pixel<T>::toType(color.green());
		m_blue[el] = Pixel<T>::toType(color.blue());
	}

	template<typename P = T>
	void setColor(const Point& p, const Color<P>& color) {
		int el = p.y() * cols() + p.x();
		m_red[el] = Pixel<T>::toType(color.red());
		m_green[el] = Pixel<T>::toType(color.green());
		m_blue[el] = Pixel<T>::toType(color.blue());
	}


	void RGBtoGray() {

		if (m_channels != 3)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < pxCount(); ++el)
			data[el] = Pixel<T>::toType(ColorSpace::CIEL(color<double>(el)));
		

		T* temp = (T*)realloc(data.get(), pxCount() * sizeof(T));
		data.release();
		data.reset(temp);


		m_channels = 1;
		m_total_pixel_count = m_pixel_count;
	}

	void RGBtoCIELab() {

		if (m_channels != 3)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < pxCount(); ++el) {
			double L, a, b;
			ColorSpace::RGBtoCIELab(color<double>(el), L, a, b);
			setColor<>(el, Color<double>(L, a, b));
		}
	}

	void CIELabtoRGB() {

		if (m_channels != 3)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < pxCount(); ++el) {
			auto c = color<double>(el);//
			setColor<>(el, ColorSpace::CIELabtoRGB(c.red(), c.green(), c.blue()));
		}
	}


	void copyTo(Image<T>& dst)const {
		if (!this->isSameShape(dst))
			dst = Image<T>(rows(), cols(), channels());
		memcpy(dst.data.get(), data.get(), totalPxCount() * sizeof(T));
	}

	void copyFrom(const Image<T>& src){
		if (!this->isSameShape(src))
			*this = Image<T>(rows(), cols(), channels());
		memcpy(data.get(), src.data.get(), totalPxCount() * sizeof(T));
	}

	void moveTo(Image<T>& dest) {
		dest = std::move(*this);
	}

	void swap(Image<T>& other) {
		std::swap(*this, other);
	}


	bool isInBounds(int x, int y)const {
		return (0 <= x && x < cols() && 0 <= y && y < rows());
	}

	T at(int x, int y)const {
		return (isInBounds(x, y)) ? (*this)(x, y) : 0;
	}

	T at(int x, int y, int ch)const {
		if (isInBounds(x, y) && ch < channels())
			return (*this)(x, y, ch);
		else
			return 0;
	}

	T at_mirrored(int x, int y, int ch)const {
		if (ch >= channels())
			0;

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

	void FillZero() {
		for (T& pixel : *this)
			pixel = 0;
	}

	void FillValue(T val) {
		for (T& pixel : *this)
			pixel = val;
	}


	void truncate(T a, T b);

	void normalize();

	void rescale(T a, T b);

	void binerize(T threshold);

private:
	bool isClipped(T pixel)const { return (pixel == Pixel<T>::min() || pixel == Pixel<T>::max()); }

	T computeMax_clipped(int ch)const;

	T computeMin_clipped(int ch)const;

	float computeMean_Clipped(int ch)const;

	float computeStdDev_Clipped(int ch, float mean)const;

	float computeAvgDev_Clipped(int ch, T median)const;

public:
	uint32_t computeClippedPxCount(int ch)const;

	void computeMinMax(T& min, T& max)const;

	T computeMax(int ch, bool clip = false)const;

	T computeMin(int ch, bool clip = false)const;

	float computeMean(int ch, bool clip = false)const;

	T computeMedian(int ch, bool clip = false)const;

	float computeStdDev(int ch, bool clip = false)const;

	float computeStdDev(int ch, float mean, bool clip = false)const;

	float computeAvgDev(int ch, bool clip = false)const;

	float computeAvgDev(int ch, T median, bool clip = false)const;

	T computeMAD(int ch, bool clip = false)const;

	T computeMAD(int ch, T median, bool clip = false)const;

	float compute_nMAD(int ch, bool clip = false)const;

	float compute_nMAD(int ch, T median, bool clip = false)const;

	float computeBWMV(int ch, bool clip = false)const;

	float computeBWMV(int ch, T median, bool clip = false)const;
};

typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;


template<typename T>
constexpr static bool isUByteImage(const Image<T>& img) {
	return (img.type() == ImageType::UBYTE);
}

template<typename T>
constexpr static bool isUShortImage(const Image<T>& img) {
	return (img.type() == ImageType::USHORT);
}

template<typename T>
constexpr static bool isFloatImage(const Image<T>& img) {
	return (img.type() == ImageType::FLOAT);
}





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
typedef std::vector<std::filesystem::path> FileVector;
typedef std::vector<std::filesystem::path> PathVector;
