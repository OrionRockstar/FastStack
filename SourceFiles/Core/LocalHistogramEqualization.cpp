#include "pch.h"
#include "FastStack.h"
#include "LocalHistogramEqualization.h"

using LHE = LocalHistogramEqualization;

LHE::KernelHistogram::KernelHistogram(Histogram::Resolution resolution, int kernel_radius, bool circular) : m_radius(kernel_radius), m_dimension(2 * kernel_radius + 1) {

	m_histogram = Histogram(resolution);
	m_multiplier = m_histogram.resolution() - 1;

	int total = dimension() * dimension();

	if (circular) {

		k_mask.resize(total, false);
		back_pix.resize(dimension());
		front_pix.resize(dimension());

		for (int j = 0; j < dimension(); ++j) {

			int dy2 = j - radius();
			dy2 *= dy2;

			bool new_x = true;

			for (int i = 0; i < dimension(); ++i) {

				int dx = i - radius();
				int loc = j * dimension() + i;

				if (sqrt(dx * dx + dy2) <= radius()) {
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
		back_pix.resize(dimension(), -radius());
		front_pix.resize(dimension(), radius());
		m_count = total;
	}
}

LHE::KernelHistogram::KernelHistogram(const KernelHistogram& kh) {

	m_histogram = kh.m_histogram;
	m_multiplier = m_histogram.resolution() - 1;
	m_count = kh.m_count;

	m_radius = kh.m_radius;
	m_dimension = kh.m_dimension;

	back_pix = kh.back_pix;
	front_pix = kh.front_pix;
	k_mask = kh.k_mask;
}

template<typename T>
void LHE::KernelHistogram::populate(Image<T>& img, int y) {

	m_histogram.fill(0);

	for (int j = -radius(), j_mask = 0; j <= radius(); ++j, j_mask += dimension()) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		for (int i = -radius(), i_m = 0; i <= radius(); ++i, ++i_m) {

			int xx = i;
			if (xx < 0)
				xx = -xx;

			if (k_mask[j_mask + i_m] && img.isInBounds(xx, yy))
				m_histogram[Pixel<float>::toType(img(xx,yy)) * multiplier()]++;
			
		}
	}
}
template void LHE::KernelHistogram::populate(Image8&, int);
template void LHE::KernelHistogram::populate(Image16&, int);
template void LHE::KernelHistogram::populate(Image32&, int);

template<typename T>
void LHE::KernelHistogram::update(Image<T>& img, int x, int y) {

	for (int j = -radius(), s = 0; j <= radius(); ++j, ++s) {

		int yy = y + j;
		if (yy < 0)
			yy = -yy;
		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		//front
		int xx = x + front_pix[s];

		if (xx >= img.cols())
			xx = 2 * img.cols() - (xx + 1);

		if (img.isInBounds(xx, yy))
			m_histogram[Pixel<float>::toType(img(xx, yy)) * multiplier()]++;


		//back
		xx = x + back_pix[s] - 1;

		if (xx < 0)
			xx = -xx;

		if (img.isInBounds(xx,yy))
			m_histogram[Pixel<float>::toType(img(xx, yy)) * multiplier()]--;
	}
}
template void LHE::KernelHistogram::update(Image8&, int, int);
template void LHE::KernelHistogram::update(Image16&, int, int);
template void LHE::KernelHistogram::update(Image32&, int, int);


template<typename T>
void LHE::apply(Image<T>& img) {

	Image<T> temp;

	if (img.channels() == 3) {
		m_ps->emitText("Getting CIE Luminance...");
		img.RGBtoCIELab();
		img.copyTo(temp);
	}
	else
		temp = Image<T>(img);

	std::atomic_uint32_t psum = 0;
	m_ps->emitText("CLAHE...");

	Threads().run([&](uint32_t start, uint32_t end, uint32_t num) {

		KernelHistogram k_hist(histogramResolution(), kernelRadius(), isCircular());
		float original_amount = 1.0 - amount();
		uint32_t m = Histogram::resolutionValue(histogramResolution()) - 1;
		Histogram histogram;

		for (int y = start; y < end; ++y) {

			for (int x = 0; x < img.cols(); ++x) {
				float pixel = img.pixel<float>(x, y);// Pixel<float>::toType(img(x, y));

				if (x == 0)
					k_hist.populate(img, y);
				else
					k_hist.update(img, x, y);

				histogram = k_hist.histogram();
				histogram.clip(math::max<uint32_t>(1, (contrastLimit() * histogram.count()) / m) + 0.5);
				histogram.convertToCDF();
				uint32_t min = histogram.minCount();
				uint32_t max = histogram[m];
				uint32_t cdf = histogram[pixel * m];

				temp(x, y) = Pixel<T>::toType((original_amount * pixel) + (amount() * float(cdf - min) / (max - min)));
			}

			psum++;

			if (num == 0)
				m_ps->emitProgress((psum * 100) / img.rows());
		}
	}, img.rows());

	m_ps->emitProgress(100);

	temp.moveTo(img);

	if (img.channels() == 3) {
		m_ps->emitText("CIE Lab to RGB...");
		img.CIELabtoRGB();
	}

	m_ps->finished();
}
template void LHE::apply(Image8&);
template void LHE::apply(Image16&);
template void LHE::apply(Image32&);
