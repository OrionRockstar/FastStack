#pragma once
#include <Eigen/Dense>
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

template <typename T>
class Image {
private:
	int m_rows = 0;
	int m_cols = 0;
	int m_channels = 0;
	int m_total = m_rows * m_cols;
	int m_total_image = m_total * m_channels;
	uint16_t m_bitdepth = 0;

	std::vector<Stats> statistics;

	inline static int m_max_val = 0;
	inline static int m_multiplier = 0;

	T* m_red = nullptr;
	T* m_green = nullptr;
	T* m_blue = nullptr;

public:
	std::unique_ptr<T[]> data;
	std::vector<bool> weight_map;

	Eigen::Matrix3d homography = Eigen::Matrix3d::Identity();

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

		switch (m_bitdepth) {
		case 32:
			m_max_val = 1.0f;
			m_multiplier = 65535;
			break;
		case 16:
			m_max_val = 65535;
			m_multiplier = 1;
			break;

		case 8:
			m_max_val = 255;
			m_multiplier = 1;
			break;

		default:
			m_max_val = 1.0f;
			m_multiplier = 65535;
			break;
		}
	}

	Image() = default;
	//Image(const Image& img);
	Image(Image&& other) {

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_total = other.m_total;
		m_total_image = other.m_total_image;
		statistics = std::move(other.statistics);

		m_max_val = other.m_max_val;
		m_multiplier = other.m_multiplier;

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;

		homography = other.homography;
		data = std::move(other.data);

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
			m_total = other.m_total;
			m_total_image = other.m_total_image;

			statistics = std::move(other.statistics);

			m_max_val = other.m_max_val;
			m_multiplier = other.m_multiplier;

			m_red = other.m_red;
			m_green = other.m_green;
			m_blue = other.m_blue;

			homography = other.homography;
			data = std::move(other.data);
		}
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


	int Rows()const { return m_rows; }

	int Cols()const { return m_cols; }

	int Total()const { return m_total; }

	int TotalImage()const { return m_total_image; }

	int Channels()const { return m_channels; }

	float Max(int channel_num = 0)const { return statistics[channel_num].max; }

	float Min(int channel_num = 0)const { return statistics[channel_num].min; }

	float Median(int channel_num = 0)const { return statistics[channel_num].median; }

	float Mean(int channel_num = 0)const { return statistics[channel_num].mean; }

	float StdDev(int channel_num = 0)const { return statistics[channel_num].stdev; }

	float AvgDev(int channel_num = 0)const { return statistics[channel_num].avgDev; }

	float MAD(int channel_num = 0)const { return statistics[channel_num].mad; }

	float nMAD(int channel_num = 0)const { return 1.4826f * statistics[channel_num].mad; }

	float BWMV(int channel_num = 0)const { return statistics[channel_num].bwmv; }

	uint16_t Bitdepth()const { return m_bitdepth; }

	T& RedPixel(int x, int y) { return m_red[y * m_cols + x]; }

	T& GreenPixel(int x, int y) { return m_green[y * m_cols + x]; }

	T& BluePixel(int x, int y) { return m_blue[y * m_cols + x]; }

	static float GetLinearVal(float pixel) {
		return (pixel > 0.04045) ? powf((pixel + 0.055f) / 1.055, 2.4f) : (pixel / 12.92);
	}

	static float GetLightness(float Y) {
		return (Y <= 0.008856f) ? Y * 9.033f : (1.16f * powf(Y, 0.333333f)) - 0.16f;
	}

	bool IsSameDim(Image<T>& other) {
		return (m_rows == other.m_rows && m_cols == other.m_cols && m_channels == other.m_channels);
	}

	void RGBtoGray() {
		if (m_channels == 1)
			return;

		//#pragma omp parallel for
		for (int el = 0; el < m_total; ++el) {
			float pix = 0.2126f * GetLinearVal(m_red[el]) + 0.7152f * GetLinearVal(m_green[el]) + 0.0722f * GetLinearVal(m_blue[el]);
			data[el] = GetLightness(pix);
		}
		realloc(data.get(), m_total * sizeof(T));
		m_channels = 1;
	}

	int Weight_At(int x, int y) { return weight_map[y * m_cols + x]; }

	void Set_Weight_At(int x, int y, bool val) { weight_map[y * m_cols + x] = val; }

	void CopyTo(Image& dest) const {
		if (dest.m_rows != m_rows || dest.m_bitdepth != m_bitdepth)
			dest = Image<T>(m_rows, m_cols, m_channels);
		memcpy(dest.data.get(), data.get(), m_total * m_channels * sizeof(T));
	}

	void MoveStatsFrom(Image& src) {
		statistics = std::move(src.statistics);
	}

	bool IsInBounds(int x, int y)const {
		return (0 <= y && y < m_rows && 0 <= x && x < m_cols);
	}

	bool IsXInBounds(int x)const {
		return (0 <= x && x < m_cols);
	}

	bool IsYInBounds(int y)const {
		return (0 <= y && y < m_rows);
	}

	bool IsOutBounds(int x, int y)const {
		return (y < 0 || m_rows <= y || x < 0 || m_cols <= x);
	}

	bool IsXOutBounds(int x)const {
		return (x < 0 || m_cols <= x);
	}

	bool IsYOutBounds(int y)const {
		return (y < 0 || m_rows <= y);
	}

	T ClipPixel(float pixel) {
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

	void Normalize() {
		float max = std::numeric_limits<T>::min();
		float min = std::numeric_limits<T>::max();
		for (auto& pixel : *this) {
			if (pixel > max) max = pixel;
			if (pixel < min) min = pixel;
		}

		float dm = 1.0f / (max - min);

		for (auto& pixel : *this)
			pixel = (pixel - min) * dm;
	}

	std::vector<int> GetHistogram() {

		std::vector<int> histogram(65536);

		for (auto& pixel : *this)
			histogram[pixel * m_multiplier]++;

		return histogram;
	}

	static float HistogramMedian(std::vector<int>& histogram, int sum_count, uint16_t bit_depth) {
		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = sum_count / 2;

		for (int i = 0; i < 65535; ++i) {
			occurrences += histogram[i];
			if (occurrences > medianlength) {
				median1 = i;
				median2 = i;

				break;
			}
			else if (occurrences == medianlength) {
				median1 = i;
				for (int j = i + 1; j <= 65535; ++j) {
					if (histogram[j] > 0) {
						median2 = j;
						break;
					}
				}
				break;
			}
		}

		float med = (median1 + median2) / 2.0;
		if (bit_depth == 32)
			return (med + (float(medianlength - (occurrences - histogram[med])) / histogram[med]));
		return med;

	}

	bool IsClippedVal(T pixel)const {
		return (pixel <= 0.0 || m_max_val <= pixel);
	}

	void ComputeStatistics(bool clip = false) {

		for (int ch = 0; ch < m_channels; ++ch) {
			float max = std::numeric_limits<float>::min();
			float min = std::numeric_limits<float>::max();

			std::vector<int> hist(65536);

			int count = 0;
			double meansum = 0;

			for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {

				if (clip && IsClippedVal(*pixel)) continue;

				if (*pixel > max)
					max = *pixel;
				if (*pixel < min)
					min = *pixel;
				meansum += *pixel;
				hist[*pixel * m_multiplier]++;
				count++;

			}
			statistics[ch].max = max;
			statistics[ch].min = min;
			float median = statistics[ch].median = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;
			float mean = statistics[ch].mean = meansum / count;

			std::fill(hist.begin(), hist.end(), 0);

			double avgDevsum = 0;

			for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {

				if (clip && IsClippedVal(*pixel)) continue;

				float t = fabsf(*pixel - median);
				hist[t * m_multiplier]++;
				avgDevsum += t;

			}

			statistics[ch].avgDev = avgDevsum / count;

			statistics[ch].mad = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;

			double x9mad = 1 / (9 * statistics[ch].mad);
			double sum1 = 0, sum2 = 0;
			double Y, a;
			double d, var = 0;

			for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {

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

	T ComputeMax(bool clip = false) {

		T r_max = std::numeric_limits<T>::min();

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				T max = std::numeric_limits<T>::min();

				for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;

					if (*pixel > max)  max = *pixel;
				}
				statistics[ch].max = max;

				if (max > r_max)
					r_max = max;
			}
			return r_max;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			T max = std::numeric_limits<T>::min();

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel)
				if (*pixel > max)  max = *pixel;

			statistics[ch].max = max;

			if (max > r_max)
				r_max = max;
		}

		return r_max;
	}

	T ComputeMin(bool clip = false) {

		T r_min = std::numeric_limits<T>::max();

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				T min = std::numeric_limits<T>::max();

				for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;

					if (*pixel < min)  min = *pixel;
				}
				statistics[ch].min = min;

				if (min < r_min)
					r_min = min;
			}
			return r_min;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			T min = std::numeric_limits<T>::max();

			for (auto pixel = cbegin(ch); pixel != cend(ch); ++pixel)
				if (*pixel < min)  min = *pixel;

			statistics[ch].min = min;

			if (min < r_min)
				r_min = min;
		}

		return r_min;
	}

	void ComputeMean(bool clip = false) {

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				int count = 0;
				double sum = 0;
				for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;

					sum += *pixel;
					count++;
				}
				statistics[ch].mean = sum / count;
			}
			return;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			double sum = 0;
			for (auto pixel = begin(ch); pixel != end(ch); ++pixel)
				sum += *pixel;

			statistics[ch].mean = sum / m_total;
		}
	}

	float ComputeMean(int channel_num, bool clip = false) {

		if (clip) {
			int count = 0;
			double sum = 0;
			for (auto pixel = begin(channel_num); pixel != end(channel_num); ++pixel) {
				if (IsClippedVal(*pixel)) continue;

				sum += *pixel;
				count++;
			}
			return statistics[channel_num].mean = sum / count;

		}

		double sum = 0;
		for (auto pixel = begin(channel_num); pixel != end(channel_num); ++pixel)
			sum += *pixel;

		return statistics[channel_num].mean = sum / m_total;
	}

	void ComputeMedian(bool clip = false) {

		std::vector<int> histogram(65536);

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				int count = 0;
				for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;
					histogram[*pixel * m_multiplier]++;
					count++;
				}
				statistics[ch].median = HistogramMedian(histogram, count, m_bitdepth) / m_multiplier;
			}
			return;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			for (auto pixel = begin(ch); pixel != end(ch); ++pixel)
				histogram[*pixel * m_multiplier]++;

			statistics[ch].median = HistogramMedian(histogram, m_total, m_bitdepth) / m_multiplier;
		}
	}

	float ComputeAverageMedian() {
		float val = 0;
		for (auto& stat : statistics)
			val += stat.median;
		return val / m_channels;
	}

	void ComputeStdDev(bool clip = false) {
		ComputeMean(clip);

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				double d, var = 0;
				int count = 0;
				for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;
					d = *pixel - Mean(ch);
					var += d * d;
					count++;
				}
				statistics[ch].stdev = sqrt(var / count);
			}
			return;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			double d, var = 0;
			for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
				d = *pixel - Mean(ch);
				var += d * d;
			}
			statistics[ch].stdev = sqrt(var / m_total);
		}
	}

	void ComputeAvgDev(bool clip = false) {

		ComputeMedian(clip);

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				double sum = 0;
				int count = 0;
				for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {

					if (IsClippedVal(*pixel)) continue;

					sum += fabs(*pixel - Median(ch));
					count++;
				}
				statistics[ch].avgDev = (sum / count);
			}
			return;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			double sum = 0;

			for (auto pixel = begin(ch); pixel != end(ch); ++pixel)
				sum += fabs(*pixel - Median(ch));

			statistics[ch].avgDev = (sum / m_total);
		}
	}

	void ComputeMAD(bool clip = false) {

		ComputeMedian(clip);

		std::vector<int> histogram(65536);

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				int count = 0;
				for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
					if (IsClippedVal(*pixel)) continue;
					histogram[fabsf(*pixel - Median(ch)) * m_multiplier]++;
					count++;
				}

				statistics[ch].mad = HistogramMedian(histogram, count, m_bitdepth) / m_multiplier;
			}
			return;
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			for (auto pixel = begin(ch); pixel != end(ch); ++pixel)
				histogram[fabsf(*pixel - Median(ch)) * m_multiplier]++;

			statistics[ch].mad = HistogramMedian(histogram, m_total, m_bitdepth) / m_multiplier;
		}
	}

	float ComputeAverageMAD() {
		float val = 0;
		for (auto& stat : statistics)
			val += stat.mad;
		return val / m_channels;
	}

	void ComputeBWMV() {

		double x9mad = 1 / (9 * ComputeMAD());
		double sum1 = 0, sum2 = 0;
		double Y, a;
		for (int ch = 0; ch < m_channels; ++ch) {
			for (auto pixel = begin(ch); pixel != end(ch); ++pixel) {
				Y = (*pixel - Median(ch)) * x9mad;

				(abs(Y) < 1) ? a = 1 : a = 0;

				Y *= Y;

				sum1 += (a * pow(*pixel - Median(ch), 2) * pow(1 - Y, 4));
				sum2 += (a * (1 - Y) * (1 - 5 * Y));
			}
			statistics[ch].bwmv = sqrt((m_total * sum1) / (abs(sum2) * abs(sum2)));
		}
	}

	T ComputeMedianABS(int channel_num) {

		std::vector<int> histogram(65536);

		for (auto pixel = begin(channel_num); pixel != end(channel_num); ++pixel)
			histogram[fabsf(*pixel) * m_multiplier]++;

		return HistogramMedian(histogram, m_total, m_bitdepth) / m_multiplier;

	}

	float ComputeMAD_MedABS(int channel_num) {

		T Med = ComputeMedianABS(channel_num);

		std::vector<int> histogram(65536);

		for (auto pixel = begin(channel_num); pixel != end(channel_num); ++pixel)
			histogram[fabsf(*pixel - Med) * m_multiplier]++;

		return HistogramMedian(histogram, m_total, m_bitdepth) / m_multiplier;

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

enum class ScaleEstimator {
	median,
	avgdev,
	mad,
	bwmv,
	none
};

bool StatsTextExists(const std::filesystem::path& file_path);

void GetImageStackFromTemp(FileVector& light_files, ImageVector& img_stack);

namespace FileOP {

	void TiffRead(std::filesystem::path file, Image32& img);

	bool FitsRead(const std::filesystem::path& file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, const std::filesystem::path& file_path, bool overwrite = true);

	void TiffWrite(Image32& img, std::string filename);
}

