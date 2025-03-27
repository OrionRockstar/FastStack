#include "pch.h"
#include "FastStack.h"
#include "LocalHistogramEqualization.h"


using LHE = LocalHistogramEqualization;


LHE::KernelHistogram::KernelHistogram(Histogram::Resolution resolution, int kernel_radius, bool circular) {

	m_histogram = Histogram(resolution);
	m_multiplier = m_histogram.resolution() - 1;

	m_radius = kernel_radius;
	m_is_circular = circular;
	m_dimension = 2 * kernel_radius + 1;

	int total = m_dimension * m_dimension;
	if (m_is_circular) {

		k_mask.resize(total, false);
		back_pix.resize(m_dimension);
		front_pix.resize(m_dimension);

		for (int j = 0; j < m_dimension; ++j) {

			int dy2 = j - kernel_radius;
			dy2 *= dy2;

			bool new_x = true;

			for (int i = 0; i < m_dimension; ++i) {

				int dx = i - kernel_radius;
				int loc = j * m_dimension + i;

				if (sqrt(dx * dx + dy2) <= kernel_radius) {
					k_mask[loc] = true;
					m_count++;
				}

				if (new_x && k_mask[loc]) {
					back_pix[j] = dx;
					front_pix[j] = dx;
					new_x = false;
				}

				else if (!new_x && k_mask[loc])
					front_pix[j] = dx;
			}
		}

	}

	else {
		k_mask.resize(total, true);
		back_pix.resize(m_dimension, -kernel_radius);
		front_pix.resize(m_dimension, kernel_radius);
		m_count = total;
	}
}

LHE::KernelHistogram::KernelHistogram(const KernelHistogram& kh) {

	m_histogram = kh.m_histogram;
	m_multiplier = m_histogram.resolution() - 1;
	m_count = kh.m_count;

	m_radius = kh.m_radius;
	m_dimension = kh.m_dimension;
	m_is_circular = kh.m_is_circular;

	back_pix = kh.back_pix;
	front_pix = kh.front_pix;
	k_mask = kh.k_mask;
}

template<typename T>
void LHE::KernelHistogram::populate(Image<T>& img, int y) {

	for (int i = 0; i < m_histogram.resolution(); ++i)
		m_histogram[i] = 0;

	for (int j = -m_radius, j_mask = 0; j <= m_radius; ++j, j_mask += m_dimension) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		for (int i = -m_radius, i_m = 0; i <= m_radius; ++i, ++i_m) {

			int xx = i;
			if (xx < 0)
				xx = -xx;

			if (k_mask[j_mask + i_m] && img.isInBounds(xx,yy)) 
				m_histogram[Pixel<float>::toType(img(xx,yy)) * m_multiplier]++;
			
		}
	}
}
template void LHE::KernelHistogram::populate(Image8&, int);
template void LHE::KernelHistogram::populate(Image16&, int);
template void LHE::KernelHistogram::populate(Image32&, int);

template<typename T>
void LHE::KernelHistogram::update(Image<T>& img, int x, int y) {

	for (int j = -m_radius, s = 0; j <= m_radius; ++j, ++s) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		//front
		int xx = x + front_pix[s];

		if (xx >= img.cols())
			xx = 2 * img.cols() - (xx + 1);

		if (img.isInBounds(xx,yy))
			m_histogram[Pixel<float>::toType(img(xx, yy)) * m_multiplier]++;

		//back
		xx = x + back_pix[s] - 1;

		if (xx < 0)
			xx = -xx;

		if (img.isInBounds(xx, yy))
			m_histogram[Pixel<float>::toType(img(xx, yy)) * m_multiplier]--;
	}
}
template void LHE::KernelHistogram::update(Image8&, int, int);
template void LHE::KernelHistogram::update(Image16&, int, int);
template void LHE::KernelHistogram::update(Image32&, int, int);



template<typename T>
void LHE::apply(Image<T>& img) {

	float original_amount = 1.0 - m_amount;

	Image<T> temp;

	if (img.channels() == 3) {
		m_ps.emitText("Getting CIE Luminance...");
		img.RGBtoCIELab();
		img.copyTo(temp);
	}
	else
		temp = Image<T>(img);

	int sum = 0;
	m_ps.emitText("CLAHE...");

	KernelHistogram k_hist(m_hist_res, m_kernel_radius, m_is_circular);

#pragma omp parallel for firstprivate(k_hist)
	for (int y = 0; y < img.rows(); ++y) {

		k_hist.populate(img, y);

		int limit = (m_contrast_limit * k_hist.count()) / (k_hist.histogram().resolution() - 1);

		if (limit == 0)
			limit = 1;

		for (int x = 0; x < img.cols(); ++x) {
			float pixel = Pixel<float>::toType(img(x, y));

			if (x != 0)
				k_hist.update(img, x, y);

			Histogram k_hist_cl(k_hist.histogram());
			k_hist_cl.clip(limit);

			int sum = 0;
			int val = pixel * k_hist.multiplier();

			for (int el = 0; el <= val; ++el)
				sum += k_hist_cl[el];

			int cdf = sum;
			int min = 0;

			for (int el = 0; el < k_hist_cl.resolution(); ++el)
				if (k_hist_cl[el] != 0) { min = k_hist_cl[el]; break; }

			for (int el = val + 1; el < k_hist_cl.resolution(); ++el)
				sum += k_hist_cl[el];

			temp(x, y) = Pixel<T>::toType((original_amount * pixel) + (m_amount * float(cdf - min) / (sum - min)));
		}

#pragma omp atomic
		++sum;

		if (omp_get_thread_num() == 0) 
			m_ps.emitProgress((sum * 100) / img.rows());		
	}

	m_ps.emitProgress(100);

	temp.moveTo(img);

	if (img.channels() == 3) {
		m_ps.emitText("CIE Lab to RGB...");
		img.CIELabtoRGB();
	}
}
template void LHE::apply(Image8&);
template void LHE::apply(Image16&);
template void LHE::apply(Image32&);
