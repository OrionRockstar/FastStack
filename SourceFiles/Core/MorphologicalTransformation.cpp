#include "pch.h"
#include"FastStack.h"
#include "MorphologicalTransformation.h"

template<typename T>
Kernel<T>::Kernel(const Image<T>& img, size_t dimension) {

	m_img = &img;
	m_dim = dimension;
	m_radius = (dimension - 1) / 2;
	m_size = m_dim * m_dim;
	m_data = std::make_unique<T[]>(m_size);
}

template<typename T>
Kernel<T>::Kernel(const Kernel<T>& other) {
	
	m_img = other.m_img;
	m_dim = other.m_dim;
	m_radius = other.m_radius;
	m_size = other.m_size;
	m_data = std::make_unique<T[]>(m_size);
	memcpy(&m_data[0], &other.m_data[0], sizeof(T) * m_size);
}

template<typename T>
void Kernel<T>::populate(int y, int ch) {

	for (int j = -m_radius, el = 0; j <= m_radius; ++j)
		for (int i = -m_radius; i <= m_radius; ++i)
			m_data[el++] = m_img->at_mirrored(i, y + j, ch);
}

template<typename T>
void Kernel<T>::update(int x, int y, int ch) {

	int xx = x + m_radius;
	if (xx >= m_img->cols())
		xx = 2 * m_img->cols() - (xx + 1);

	for (int j = 0; j < m_size; j += m_dim)
		for (int i = 0; i < m_dim - 1; ++i)
			m_data[j + i] = m_data[j + i + 1];

	for (int j = -m_radius, el = m_dim - 1; j <= m_radius; ++j, el += m_dim) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= m_img->rows())
			yy = 2 * m_img->rows() - (yy + 1);

		m_data[el] = (*m_img)(xx, yy, ch);
	}
}
template class Kernel<uint8_t>;
template class Kernel<uint16_t>;
template class Kernel<float>;





template<typename T>
MorphologicalKernel<T>::MorphologicalKernel(const Image<T>& img, const MorphologicalTransformation& mt) : Kernel<T>(img, mt.kernelDimension()) {

	m_locations = mt.maskedLocations();
	m_masked_data = std::vector<T>(m_locations.size());
}

template<typename T>
void MorphologicalKernel<T>::populate(int y, int ch) {

	Kernel<T>::populate(y, ch);
	setMaskedData();
}

template<typename T>
void MorphologicalKernel<T>::update(int x, int y, int ch) {

	Kernel<T>::update(x, y, ch);
	setMaskedData();
}

template<typename T>
void MorphologicalKernel<T>::setMaskedData() {

	if (m_masked_data.size() == this->count())
		memcpy(&m_masked_data[0], &(*this)[0], sizeof(T) * this->count());
	else
		for (int el = 0; el < m_masked_data.size(); ++el)
			m_masked_data[el] = (*this)[m_locations[el]];
}

template<typename T>
T MorphologicalKernel<T>::minimum()const {

	T min = std::numeric_limits<T>::max();

	for (int el = 0; el < m_masked_data.size(); ++el)
		min = math::min(min, m_masked_data[el]);

	return min;
}

template<typename T>
T MorphologicalKernel<T>::maximum()const {

	T max = std::numeric_limits<T>::min();

	for (int el = 0; el < m_masked_data.size(); ++el)
		max = math::max(max, m_masked_data[el]);

	return max;
}

template<typename T>
T MorphologicalKernel<T>::selection(int pivot) {

	std::nth_element(m_masked_data.begin(), m_masked_data.begin() + pivot, m_masked_data.end());
	return m_masked_data[pivot];
}

template<typename T>
T MorphologicalKernel<T>::median() {

	return selection(m_masked_data.size() / 2);
}

template<typename T>
T MorphologicalKernel<T>::midpoint()const {

	T min = std::numeric_limits<T>::max();
	T max = std::numeric_limits<T>::min();

	for (int el = 0; el < m_masked_data.size(); ++el) {
		min = math::min(min, m_masked_data[el]);
		max = math::max(max, m_masked_data[el]);
	}

	return (0.5 * min + 0.5 * max);
}



using MT = MorphologicalTransformation;

void MT::resizeKernel(int new_dim) {
	int old_dim = m_kernel_dim;
	m_kernel_dim = new_dim;
	m_kernel_radius = (m_kernel_dim - 1) / 2;
	m_kernel_size = m_kernel_dim * m_kernel_dim;

	std::vector<char> new_mask(m_kernel_size, false);

	int d_dim = (m_kernel_dim - old_dim) / 2;

	if (d_dim > 0)
		for (int y_n = d_dim, y = 0; y < old_dim; ++y, ++y_n)
			for (int x_n = d_dim, x = 0; x < old_dim; ++x, ++x_n)
				new_mask[y_n * m_kernel_dim + x_n] = m_kmask[y * old_dim + x];

	else {
		d_dim *= -1;
		for (int y_n = 0, y = d_dim; y_n < m_kernel_dim; ++y, ++y_n)
			for (int x_n = 0, x = d_dim; x_n < m_kernel_dim; ++x, ++x_n)
				new_mask[y_n * m_kernel_dim + x_n] = m_kmask[y * old_dim + x];
	}

	m_kmask = new_mask;
}

void MT::setMask_All(bool value) {
	for (char& val : m_kmask)
		val = value;
}

void MT::setMask_Circular() {

	for (char& value : m_kmask)
		value = false;

	for (int j = 0; j < m_kernel_dim; ++j) {
		for (int i = 0; i < m_kernel_dim; ++i) {
			int dx = i - m_kernel_radius;
			int dy = j - m_kernel_radius;
			if (sqrt(dx * dx + dy * dy) <= m_kernel_radius + 0.5) {
				m_kmask[j * m_kernel_dim + i] = true;
			}
		}
	}
}

void MT::setMask_Diamond() {

	for (char& value : m_kmask)
		value = false;

	int mid = m_kernel_dim / 2;

	for (int j = 0; j < m_kernel_dim; ++j) {

		int x_start = abs(mid - j);
		int x_end = (j > mid) ? m_kernel_dim - x_start - 1 : mid + j;

		for (int i = x_start; i <= x_end; ++i) {
			m_kmask[j * m_kernel_dim + i] = true;
		}
	}
}

void MT::invertMask() {
	for (auto& val : m_kmask)
		val = (val) ? false : true;
}

void MT::rotateMask() {

	std::vector<char> temp(m_kmask.size());

	int s = m_kernel_dim - 1;

	for (int y = 0; y < m_kernel_dim; ++y) {

		for (int x = 0, x_s = m_kernel_dim - 1; x < m_kernel_dim; ++x) {
			temp[y * m_kernel_dim + x] = m_kmask[(s - x) * m_kernel_dim + y];

		}
	}

	m_kmask = temp;
}

int MT::maskCount()const {

	int c = 0;
	for (auto v : m_kmask)
		if (v)
			c++;

	return c;
}

std::vector<uint16_t> MT::maskedLocations()const {

	int count = 0; //number of true values
	for (auto v : m_kmask)
		if (v) count++;

	std::vector<uint16_t> locations(count);

	for (int j = 0, el = 0; j < m_kernel_dim; ++j)
		for (int i = 0; i < m_kernel_dim; ++i)
			if (m_kmask[j * m_kernel_dim + i])
				locations[el++] = j * m_kernel_dim + i; //location of true values in the kernel

	return locations;
}

template<typename T>
void MT::erosion(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Erosion...");
	int sum = 0, total = img.channels() * img.rows();

	MorphologicalKernel<T> kernel(img, *this);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.minimum());

			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);

}
template void MT::erosion(Image8&);
template void MT::erosion(Image16&);
template void MT::erosion(Image32&);

template<typename T>
void MT::dialation(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Dialation...");
	int sum = 0, total = img.channels() * img.rows();

	MorphologicalKernel<T> kernel(img, *this);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.maximum());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::dialation(Image8&);
template void MT::dialation(Image16&);
template void MT::dialation(Image32&);

template <typename T>
void MT::opening(Image<T>& img) {

	dialation(img);
	erosion(img);
}
template void MT::opening(Image8&);
template void MT::opening(Image16&);
template void MT::opening(Image32&);

template <typename T>
void MT::closing(Image<T>& img) {

	erosion(img);
	dialation(img);
}
template void MT::closing(Image8&);
template void MT::closing(Image16&);
template void MT::closing(Image32&);

template <typename T>
void MT::selection(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Selection...");
	int sum = 0, total = img.channels() * img.rows();

	int pivot = (m_selection == 1.0) ? maskCount() - 1 : maskCount() * m_selection;

	MorphologicalKernel<T> kernel(img, *this);

	for (int ch = 0; ch < img.channels(); ++ch) {

#pragma omp parallel for firstprivate(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.selection(pivot));
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::selection(Image8&);
template void MT::selection(Image16&);
template void MT::selection(Image32&);

template <typename T>
void MT::median(Image<T>& img) {

	if (maskCount() == m_kmask.size() && m_kernel_dim <= 9)
		return fastMedian(img, m_kernel_dim);

	Image<T> temp(img);

	m_ps->emitText("Median...");
	int sum = 0, total = img.channels() * img.rows();

	MorphologicalKernel<T> kernel(img, *this);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.median());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::median(Image8&);
template void MT::median(Image16&);
template void MT::median(Image32&);

template<typename T>
void MT::midpoint(Image<T>& img) {

	Image<T> temp(img);

	m_ps->emitText("Selection...");
	int sum = 0, total = img.channels() * img.rows();

	MorphologicalKernel<T> kernel(img, *this);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				temp(x, y, ch) = blend(img(x, y, ch), kernel.midpoint());
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::midpoint(Image8&);
template void MT::midpoint(Image16&);
template void MT::midpoint(Image32&);


template <typename T>
void MT::fastMedian3x3(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 9>kernel{ 0 };

				kernel[0] = img.at_mirrored(x - 1, y - 1, ch);
				kernel[1] = img.at_mirrored(x, y - 1, ch);
				kernel[2] = img.at_mirrored(x + 1, y - 1, ch);

				kernel[3] = img.at_mirrored(x - 1, y, ch);
				kernel[4] = img(x, y, ch);
				kernel[5] = img.at_mirrored(x + 1, y, ch);

				kernel[6] = img.at_mirrored(x - 1, y + 1, ch);
				kernel[7] = img.at_mirrored(x, y + 1, ch);
				kernel[8] = img.at_mirrored(x + 1, y + 1, ch);

				for (int r = 0; r < 3; ++r) {
					for (int i = 0, j = 5; i < 4; ++i, ++j) {
						if (kernel[i] > kernel[4])
							std::swap(kernel[i], kernel[4]);
						if (kernel[j] < kernel[4])
							std::swap(kernel[j], kernel[4]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[4]);
			}
		}
	}

	temp.moveTo(img);
}
template void MT::fastMedian3x3(Image8&);
template void MT::fastMedian3x3(Image16&);
template void MT::fastMedian3x3(Image32&);

template <typename T>
void MT::fastMedian5x5(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 25>kernel{ 0 };

				for (int j = -2, el = 0; j <= 2; ++j)
					for (int i = -2; i <= 2; ++i)
						kernel[el++] = img.at_mirrored(x + i, y + j, ch);

				for (int r = 0; r < 5; ++r)
					for (int i = 0, j = 13; i < 12; ++i, ++j) {
						if (kernel[i] > kernel[12])
							std::swap(kernel[i], kernel[12]);
						if (kernel[j] < kernel[12])
							std::swap(kernel[j], kernel[12]);
					}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[12]);
			}
		}
	}
	temp.moveTo(img);
}
template void MT::fastMedian5x5(Image8&);
template void MT::fastMedian5x5(Image16&);
template void MT::fastMedian5x5(Image32&);

template <typename T>
void MT::fastMedian7x7(Image<T>& img) {

	Image<T> temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				std::array<T, 49> kernel;

				for (int j = -3, el = 0; j <= 3; ++j)
					for (int i = -3; i <= 3; ++i)
						kernel[el++] = img.at_mirrored(x + i, y + j, ch);

				for (int r = 0; r < 7; ++r) {
					for (int i = 0, j = 25; i < 24; ++i, ++j) {
						if (kernel[i] > kernel[24])
							std::swap(kernel[i], kernel[24]);
						if (kernel[j] < kernel[24])
							std::swap(kernel[j], kernel[24]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), kernel[24]);
			}
		}
	}

	temp.moveTo(img);
}
template void MT::fastMedian7x7(Image8&);
template void MT::fastMedian7x7(Image16&);
template void MT::fastMedian7x7(Image32&);

template <typename T>
void MT::fastMedian9x9(Image<T>& img) {

	Image<T> temp(img);

	int sum = 0, total = img.channels() * img.rows();

	Kernel<T> kernel(img, kernelDimension());
	std::vector<T> k(kernel.count());

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel, k)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				memcpy(&k[0], &kernel[0], k.size() * sizeof(T));

				for (int r = 0; r < 9; ++r) {
					for (int i = 0, j = 41; i < 40; ++i, ++j) {
						if (k[i] > k[40])
							std::swap(k[i], k[40]);
						if (k[j] < k[40])
							std::swap(k[j], k[40]);
					}
				}

				temp(x, y, ch) = blend(img(x, y, ch), k[40]);
			}

#pragma omp atomic
			++sum;

			if (omp_get_thread_num() == 0)
				m_ps->emitProgress((sum * 100) / total);
		}
	}

	m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void MT::fastMedian9x9(Image8&);
template void MT::fastMedian9x9(Image16&);
template void MT::fastMedian9x9(Image32&);

template <typename T>
void MT::fastMedian(Image<T>& img, int kernel_dim) {
	if (kernel_dim > 9 || kernel_dim % 2 == 0)
		return;

	m_ps->emitText("Fast Median...");

	switch (kernel_dim) {
	case 3:
		return fastMedian3x3(img);
	case 5:
		return fastMedian5x5(img);
	case 7:
		return fastMedian7x7(img);
	case 9:
		return fastMedian9x9(img);
	}
}
template void MT::fastMedian(Image8&, int);
template void MT::fastMedian(Image16&, int);
template void MT::fastMedian(Image32&, int);

template <typename T>
void MT::apply(Image<T>& src) {

	//GetMaskedLocations();
	if (maskCount() == 0)
		return src.fillZero();

	switch (m_morph_filter) {
	case Type::erosion:
		return erosion(src);
	case Type::dialation:
		return dialation(src);
	case Type::opening:
		return opening(src);
	case Type::closing:
		return closing(src);
	case Type::selection:
		return selection(src);
	case Type::median:
		return median(src);
	case Type::midpoint:
		return midpoint(src);
	default:
		return erosion(src);
	}
}
template void MT::apply(Image8&);
template void MT::apply(Image16&);
template void MT::apply(Image32&);
