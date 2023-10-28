#pragma once
#include"Matrix.h"
#include "RGBColorSpace.h"
#include<filesystem>
#include <fstream>

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

struct Histogram {

	std::unique_ptr<uint32_t[]> histogram;
	int m_size = 0;
	int m_count = 0;

	uint32_t& operator[](int val) { return histogram[val]; }

	//buids full image histogramogram
	template<typename Image>
	Histogram(Image& img) {

		m_count = img.TotalPxCount();

		if (Image::is_uint16() || Image::is_uint8()) {

			m_size = (Image::is_uint16()) ? 65536 : 256;

			histogram = std::make_unique<uint32_t[]>(m_size);

			for (auto pixel : img)
				histogram[pixel]++;

			return;
		}

		if (Image::is_float()) {
			m_size = 65536;
			histogram = std::make_unique<uint32_t[]>(m_size);

			for (auto pixel : img)
				histogram[pixel * 65535]++;

		}
	}

	//builds histogram on per channel basis
	template<typename Image>
	Histogram(Image& img, int ch) {

		m_count = img.PxCount();

		if (Image::is_uint16() || Image::is_uint8()) {

			m_size = (Image::is_uint16()) ? 65536 : 256;

			histogram = std::make_unique<uint32_t[]>(m_size);

			for (auto pixel = img.cbegin(ch); pixel != img.cend(ch); ++pixel)
				histogram[*pixel]++;

			return;
		}

		if (Image::is_float()) {

			m_size = 65536;

			histogram = std::make_unique<uint32_t[]>(m_size);

			for (auto pixel = img.cbegin(ch); pixel != img.cend(ch); ++pixel)
				histogram[*pixel * 65535]++;

			return;
		}
	}

	//builds mad histogram
	template<typename Image>
	Histogram(Image& img, int ch, float median, bool clip = false) {

		if (Image::is_uint16() || Image::is_uint8()) {

			m_size = (Image::is_uint16()) ? 65536 : 256;

			histogram = std::make_unique<uint32_t[]>(m_size);

			for (auto pixel = img.cbegin(ch); pixel != img.cend(ch); ++pixel) {
				if (clip && img.IsClippedVal(*pixel))
					continue;
				histogram[abs(*pixel - median)]++;
				m_count++;
			}
			return;
		}

		if (Image::is_float()) {
			histogram = std::make_unique<uint32_t[]>(65536);
			m_size = 65536;

			for (auto pixel = img.cbegin(ch); pixel != img.cend(ch); ++pixel) {
				if (clip && img.IsClippedVal(*pixel))
					continue;
				histogram[abs(*pixel - median) * 65535]++;
				m_count++;
			}
		}
	}

	//computes and returns 8/16 bit median
	float Median(bool clip = false) {
		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = m_count / 2;

		int cl = 0, cu = m_size;
		if (clip) { cl = 1; cu -= 1; medianlength = (m_count - histogram[0] - histogram[m_size - 1]) / 2; }

		for (int i = cl; i < cu; ++i) {
			occurrences += histogram[i];
			if (occurrences > medianlength) {
				median1 = i;
				median2 = i;

				break;
			}
			else if (occurrences == medianlength) {
				median1 = i;
				for (int j = i + 1; j < cu; ++j) {
					if (histogram[j] > 0) {
						median2 = j;
						break;
					}
				}
				break;
			}
		}

		float med = (median1 + median2) / 2.0;

		if (histogram[med] == 0) {
			float k1 = float(medianlength - (occurrences - histogram[median1])) / histogram[median1];
			float k2 = float(medianlength - occurrences) / histogram[median2];
			med += (k1 + k2) / 2;
			return med;
		}

		med += float(medianlength - (occurrences - histogram[median1])) / histogram[med];
		return med;


	}

	void ZeroHistogram() {
		m_count = 0;
		for (int el = 0; el < m_size; ++el)
			histogram[el] = 0;
	}
};

class Pixel {
public:
	static double toDouble(uint8_t pixel) {
		return double(pixel) / 255;
	}

	static double toDouble(uint16_t pixel) {
		return double(pixel) / 65535;
	}

	static double toDouble(float pixel) {
		return double(pixel);
	}

	static void fromDouble(double pixel, uint8_t& a) {
		a = uint8_t(pixel * 255);
	}

	static void fromDouble(double pixel, uint16_t& a) {
		a = uint16_t(pixel * 65535);
	}

	static void fromDouble(double pixel, float& a) {
		a = float(pixel);
	}


	static float toFloat(uint8_t pixel) {
		return float(pixel) / 255;
	}

	static float toFloat(uint16_t pixel) {
		return float(pixel) / 65535;
	}

	static float toFloat(float pixel) {
		return pixel;
	}

	static void fromFloat(float pixel, uint8_t& a) {
		a = uint8_t(pixel * 255);
	}

	static void fromFloat(float pixel, uint16_t& a) {
		a = uint16_t(pixel * 65535);
	}

	static void fromFloat(float pixel, float& a) {
		a = float(pixel);
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

	int m_total = m_rows * m_cols;
	int m_total_image = m_total * m_channels;
	int16_t m_bitdepth = 0;

	std::vector<Stats> statistics;

	T m_max_val = 1;

	T* m_red = nullptr;
	T* m_green = nullptr;
	T* m_blue = nullptr;

public:
	std::unique_ptr<T[]> data;
	std::vector<bool> weight_map;

	Matrix homography = Matrix(3,3).Identity();

	Image(int r, int c, int ch = 1) :m_rows(r), m_cols(c), m_channels(ch) {
		assert(ch == 1 || ch == 3);

		m_bitdepth = sizeof(T) * 8;
		data = std::make_unique<T[]>(r * c * ch);

		if (m_channels == 3) {
			statistics.resize(3, Stats());
			m_red = data.get();
			m_green = data.get() + m_total;
			m_blue = data.get() + 2 * m_total;
		}
		else
			statistics.resize(1, Stats());

		m_max_val = (m_bitdepth == 32) ? 1 : std::numeric_limits<T>::max();
	}

	Image(Image<T>& other, bool copy = false) {
		*this = Image<T>(other.m_rows, other.m_cols, other.m_channels);
		if (copy)
			memcpy(this->data.get(), other.data.get(), other.m_total_image * sizeof(T));
	}

	Image() = default;

	Image(const Image& other) {
		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_pixel_count = other.m_pixel_count;
		m_total_pixel_count = other.m_total_pixel_count;

		m_total = other.m_total;
		m_total_image = other.m_total_image;
		statistics = other.statistics;

		m_max_val = other.m_max_val;

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;

		homography = other.homography;
		memcpy(data.get(), other.data.get(), m_total_image * sizeof(T));
		weight_map = other.weight_map;
	}

	Image(Image&& other) {

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_pixel_count = other.m_pixel_count;
		m_total_pixel_count = other.m_total_pixel_count;

		m_total = other.m_total;
		m_total_image = other.m_total_image;
		statistics = std::move(other.statistics);

		m_max_val = other.m_max_val;

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;

		homography = other.homography;
		data = std::move(other.data);
		weight_map = std::move(other.weight_map);

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
		using ValueType = const T;
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
			m_total = other.m_total;
			m_total_image = other.m_total_image;

			statistics = std::move(other.statistics);

			m_max_val = other.m_max_val;

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
		for (int i = 0; i < m_total_image; ++i)
			data[i] += other[i];
		return *this;
	}

	Image& operator-=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s -= *o;
		return *this;
	}

	Image& operator-=(const Image& other) {
		for (int i = 0; i < m_total_image; ++i)
			data[i] -= other[i];
		return *this;
	}

	Image& operator*=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s *= *o;
		return *this;
	}

	Image& operator*=(const Image& other) {
		for (int i = 0; i < m_total_image; ++i)
			data[i] *= other[i];
		return *this;
	}

	Image& operator/=(Image& other) {
		for (auto s = begin(), o = other.begin(); s != end(); ++s, ++o)
			*s /= *o;
		return *this;
	}

	Image& operator/=(const Image& other) {
		for (int i = 0; i < m_total_image; ++i)
			data[i] /= other[i];
		return *this;
	}

	T& operator[](int el) {
		return data[el];
	}

	T& operator[](int el) const {
		return data[el];
	}

	T& operator()(int x, int y) {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y) const {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) {
		return data[ch * m_total + y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) const {
		return data[ch * m_ total + y * m_cols + x];
	}

	Iterator begin() {
		return Iterator(this->data.get());
	}

	Iterator cbegin() const {
		return ConstIterator(this->data.get());
	}

	Iterator end() {
		return Iterator(this->data.get() + m_total * m_channels);
	}

	Iterator cend() const {
		return ConstIterator(this->data.get() + m_total * m_channels);
	}

	Iterator begin(int channel) {
		return Iterator(this->data.get() + channel * m_total);
	}

	Iterator end(int channel) {
		return Iterator(this->data.get() + (channel + 1) * m_total);
	}

	ConstIterator cbegin(int channel)const {
		return ConstIterator(this->data.get() + channel * m_total);
	}

	ConstIterator cend(int channel)const {
		return ConstIterator(this->data.get() + (channel + 1) * m_total);
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

	int Total()const { return m_total; }

	int PxCount()const noexcept { return m_pixel_count; }

	int TotalPxCount()const noexcept { return m_total_pixel_count; }

	int TotalImage()const { return m_total_image; }

	int Channels()const { return m_channels; }

	int16_t Bitdepth()const noexcept { return m_bitdepth; }

	T MaxVal()const noexcept { return m_max_val; }


	T Max(int channel_num = 0)const noexcept { return statistics[channel_num].max; }

	T Min(int channel_num = 0)const noexcept { return statistics[channel_num].min; }

	T Median(int channel_num = 0)const noexcept { return statistics[channel_num].median; }

	float Mean(int channel_num = 0)const noexcept { return statistics[channel_num].mean; }

	float StdDev(int channel_num = 0)const noexcept { return statistics[channel_num].stdev; }

	float AvgDev(int channel_num = 0)const noexcept { return statistics[channel_num].avgDev; }

	float MAD(int channel_num = 0)const noexcept { return statistics[channel_num].mad; }

	float nMAD(int channel_num = 0)const noexcept { return 1.4826f * statistics[channel_num].mad; }

	float BWMV(int channel_num = 0)const noexcept { return statistics[channel_num].bwmv; }


	void getRGB(int el, T& R, T& G, T& B)const {
			R = m_red[el];
		G = m_green[el];
		B = m_blue[el];
	}

	void setRGB(int el, T R, T G, T B) {
		m_red[el] = R;
		m_green[el] = G;
		m_blue[el] = B;
	}

	void toRGBFloat(int el, float& R, float& G, float& B)const {
		R = Pixel::toFloat(m_red[el]);
		G = Pixel::toFloat(m_green[el]);
		B = Pixel::toFloat(m_blue[el]);

	}

	void toRGBDouble(int el, double& R, double& G, double& B)const {
		R = Pixel::toDouble(m_red[el]);
		G = Pixel::toDouble(m_green[el]);
		B = Pixel::toDouble(m_blue[el]);
	}

	void fromRGBFloat(int el, float R, float G, float B) {

		Pixel::fromFloat(R, m_red[el]);
		Pixel::fromFloat(G, m_green[el]);
		Pixel::fromFloat(B, m_blue[el]);
	}

	void fromRGBDouble(int el, double R, double G, double B) {
		Pixel::fromDouble(R, m_red[el]);
		Pixel::fromDouble(G, m_green[el]);
		Pixel::fromDouble(B, m_blue[el]);
	}


	void RGBtoGray() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double R, G, B;
			toRGBDouble(el, R, G, B);

			Pixel::fromDouble(ColorSpace::CIEL(R, G, B), data[el]);
		}

		T* temp = (T*)realloc(data.get(), m_total * sizeof(T));
		data.release();
		data.reset(temp);

		m_channels = 1;
		m_total_image = m_total;
	}

	void RGBtoCIELab() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double R, G, B, L, a, b;
			toRGBDouble(el, R, G, B);
			ColorSpace::RGBtoCIELab(R, G, B, L, a, b);
			fromRGBDouble(el, L, a, b);
		}

	}

	void CIELabtoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double L, a, b, R, G, B;
			toRGBDouble(el, L, a, b);
			ColorSpace::CIELabtoRGB(L, a, b, R, G, B);
			fromRGBDouble(el, R, G, B);
		}
	}

	void RGBtoCIELch() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double R, G, B, L, c, h;
			toRGBDouble(el, R, G, B);
			ColorSpace::RGBtoCIELch(R, G, B, L, c, h);
			fromRGBDouble(el, L, c, h);
		}

	}

	void CIELchtoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double L, c, h, R, G, B;
			toRGBDouble(el, L, c, h);
			ColorSpace::CIELchtoRGB(L, c, h, R, G, B);
			fromRGBDouble(el, R, G, B);
		}
	}

	void RGBtoHSI() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double R, G, B, H, S, I;
			toRGBDouble(el, R, G, B);
			ColorSpace::RGBtoHSI(R, G, B, H, S, I);
			fromRGBDouble(el, H, S, I);
		}

	}

	void HSItoRGB() {

		if (m_channels == 1)
			return;

#pragma omp parallel for num_threads(2)
		for (int el = 0; el < m_total; ++el) {
			double H, S, I, R, G, B;
			toRGBDouble(el, H, S, I);
			ColorSpace::HSItoRGB(H, S, I, R, G, B);
			fromRGBDouble(el, R, G, B);
		}
	}


	void CreateWeightMap() {
		weight_map = std::vector<bool>(m_total, true);
	}

	int Weight_At(int x, int y) { return weight_map[y * m_cols + x]; }

	void Set_Weight_At(int x, int y, bool val) { weight_map[y * m_cols + x] = val; }


	void CopyTo(Image& dest) const {
		if (dest.m_rows != m_rows || dest.m_bitdepth != m_bitdepth)
			dest = Image<T>(m_rows, m_cols, m_channels);
		memcpy(dest.data.get(), data.get(), m_total * m_channels * sizeof(T));
	}

	void MoveTo(Image& dest) {
		dest = std::move(*this);
	}

	void CopyToFloat(Image<float>& dest, bool normalize = false) {
		if (!this->Matches(dest))
			dest = Image<float>(m_rows, m_cols, m_channels);

		if (this->is_float())
			memcpy(dest.data.get(), data.get(), m_total_image * sizeof(float));

		else if (normalize)
			for (int el = 0; el < m_total_image; ++el)
				dest[el] = ToFloat(data[el]);
		else
			for (int el = 0; el < m_total_image; ++el)
				dest[el] = data[el];
	}

	void MoveStatsFrom(Image& src) {
		statistics = std::move(src.statistics);
	}


	static bool is_uint8() { return std::is_same<T, uint8_t>::value; }

	static bool is_uint16() { return std::is_same<T, uint16_t>::value; }

	static bool is_float() { return std::is_same<T, float>::value; }


	bool IsInBounds(int x, int y)const {
		return (0 <= y && y < m_rows && 0 <= x && x < m_cols);
	}

	bool IsInBounds(int x, int y, int buffer)const {
		return(buffer <= y && y < m_rows - buffer && buffer <= x && x < m_cols - buffer);
	}

	T MirrorEdgePixel(int x, int y, int ch) {
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


	float ClipPixel(float pixel) noexcept {
		if (pixel > m_max_val)
			return m_max_val;
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
		T max_val = (is_float()) ? 1 : std::numeric_limits<T>::max();

		T max = std::numeric_limits<T>::min();
		T min = std::numeric_limits<T>::max();

		for (T& pixel : *this) {
			if (pixel > max) max = pixel;
			if (pixel < min) min = pixel;
		}

		if (min < 0 || max > max_val)
			for (auto& pixel : *this)
				pixel = (pixel - min) / (max - min);

	}

	void AutoRescale() {
		if (!is_float())
			return;

		T max, min;
		ComputeMinMax(min, max);

		if (max > 1.0f || min < 0.0f)
			Rescale(min, max);
	}

	void Rescale(T a, T b) {
		if (b == a)
			return;

		float dba = 1.0 / (b - a);

		for (T& pixel : *this) {
			if (pixel < a)
				pixel = 0;
			else if (pixel > b)
				pixel = 1;
			else
				pixel = (pixel - a) * dba;
		}

	}

	void Rescale(T a, T b, int ch) {
		if (b == a)
			return;

		float dba = 1.0 / (b - a);

		for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
			if (*pixel < a)
				*pixel = 0;
			else if (*pixel > b)
				*pixel = 1;
			else
				*pixel = (*pixel - a) * dba;
		}
	}

	void Binerize(T threshold) {
		for (auto& pixel : *this)
			pixel = (pixel >= threshold) ? 1 : 0;
	}


public:
	bool IsClippedVal(T pixel)const {
		return (pixel <= 0.0 || m_max_val <= pixel);
	}

	void ComputeStatistics(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch) {
			float max = std::numeric_limits<float>::min();
			float min = std::numeric_limits<float>::max();

			int count = 0;
			double meansum = 0;
			Histogram histogram((*this), ch);

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
				if (clip && IsClippedVal(*pixel)) continue;

				if (*pixel > max)
					max = *pixel;
				if (*pixel < min)
					min = *pixel;

				meansum += *pixel;
				count++;

			}

			statistics[ch].max = max;
			statistics[ch].min = min;
			float median = statistics[ch].median = histogram.Median(clip);
			float mean = statistics[ch].mean = meansum / count;

			histogram.ZeroHistogram();
			double avgDevsum = 0;

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
				if (clip && IsClippedVal(*pixel)) continue;

				float t = abs(*pixel - median);
				histogram[(is_float()) ? t * 65535 : t]++;
				avgDevsum += t;

			}

			statistics[ch].avgDev = avgDevsum / count;

			statistics[ch].mad = histogram.Median();

			double x9mad = 1 / (9 * statistics[ch].mad);
			double sum1 = 0, sum2 = 0;
			double Y, a;
			double d, var = 0;

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {

				if (clip && IsClippedVal(*pixel)) continue;

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
	void ComputeMinMax(T& min, T& max) {

		max = std::numeric_limits<T>::min();
		min = std::numeric_limits<T>::max();

		for (auto pixel : *this) {
			if (pixel > max)
				max = pixel;
			if (pixel < min)
				min = pixel;
		}
	}

public:
	T ComputeMax(int ch, bool clip = false) {

		T max = std::numeric_limits<T>::min();

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel))
				continue;
			if (*pixel > max)  max = *pixel;
		}
		return statistics[ch].max = max;
	}

	void ComputeMax(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeMax(ch, clip);
	}


	T ComputeMin(int ch, bool clip = false) {

		T min = std::numeric_limits<T>::max();

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel))
				continue;
			if (*pixel < min)  min = *pixel;
		}
		return statistics[ch].min = min;
	}

	void ComputeMin(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeMin(ch, clip);
	}


	float ComputeMean(int ch, bool clip = false) const {

		double sum = 0;
		int count = 0;

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel))
				continue;
			sum += *pixel;
			count++;

		}
		return sum / count;
	}

	float ComputeMean(int ch, bool clip = false) {

		double sum = 0;
		int count = 0;

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel))
				continue;
			sum += *pixel;
			count++;

		}
		return statistics[ch].mean = sum / count;
	}

	void ComputeMean(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeMean(ch, clip);
	}


	T ComputeMedian(int ch, bool clip = false) const {
		Histogram hist(*this, ch);
		return hist.Median(clip) / ((is_float()) ? 65535.0 : 1);
	}

	T ComputeMedian(int ch, bool clip = false) {
		Histogram hist(*this, ch);
		return statistics[ch].median = hist.Median(clip) / ((is_float()) ? 65535.0 : 1);
	}

	void ComputeMedian(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeMedian(ch, clip);

	}


	float ComputeStdDev(int ch, float mean, bool clip = false) const {

		double d, var = 0;
		float mean = Mean(ch);
		int count = 0;

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel)) continue;
			d = *pixel - mean;
			var += d * d;
			count++;
		}

		return sqrt(var / count);
	}

	float ComputeStdDev(int ch, bool clip = false) {

		ComputeMean(ch, clip);

		double d, var = 0;
		float mean = Mean(ch);
		int count = 0;

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel)) continue;
			d = *pixel - mean;
			var += d * d;
			count++;
		}

		return statistics[ch].stdev = sqrt(var / count);

	}

	void ComputeStdDev(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeStdDev(ch, clip);
	}


	float ComputeAvgDev(int ch, T median, bool clip = false) const {

		double sum = 0;
		int count = 0;

		for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel)) continue;
			sum += fabs(*pixel - median);
			count++;
		}

		return sum / count;
	}

	float ComputeAvgDev(int ch, bool clip = false) {

		T median = ComputeMedian(ch, clip);

		double sum = 0;
		int count = 0;

		for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
			if (clip && IsClippedVal(*pixel)) continue;
			sum += fabs(*pixel - median);
			count++;
		}
		return statistics[ch].avgDev = (sum / count);
	}

	void ComputeAvgDev(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeAvgDev(ch);

	}


	T ComputeMAD(int ch, T median, bool clip = false) const {

		Histogram histogram((*this), ch, median, clip);
		histogram.Median() / ((is_float()) ? 65535.0 : 1);
	}

	T ComputeMAD(int ch, bool clip = false) {

		T median = ComputeMedian(ch, clip);

		Histogram histogram((*this), ch, median, clip);
		return statistics[ch].mad = histogram.Median() / ((is_float()) ? 65535.0 : 1);
	}

	void ComputeMAD(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeMAD(ch, clip);

	}


	float ComputeBWMV(int ch, bool clip = false) {

		T mad = ComputeMAD(ch, clip);
		T median = Median(ch);

		double x9mad = 1 / (9 * mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		int count = 0;

		for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {

			if (clip && IsClippedVal(*pixel)) continue;

			Y = (*pixel - median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(*pixel - median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));
			count++;
		}
		return statistics[ch].bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));

	}

	void ComputeBWMV(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch)
			ComputeBWMV(ch, clip);
	}

	friend void CreateStatsText(const std::filesystem::path& file_path, Image<T>& img) {

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
	}
};

typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;

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

namespace FileOP {

	void TiffRead(std::filesystem::path file, Image32& img);

	bool FitsRead(const std::filesystem::path file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, const std::filesystem::path& file_path, bool overwrite = true);

	void TiffWrite(Image32& img, std::string filename);
}

