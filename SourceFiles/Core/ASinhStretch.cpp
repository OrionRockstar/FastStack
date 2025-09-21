#include "pch.h"
#include "FastStack.h"
#include "ASinhStretch.h"
#include "Histogram.h"

template<typename T>
void ASinhStretch::computeBlackpoint(const Image<T>& img) {
	Histogram histogram;
	histogram.constructHistogram(img);

	int sum = 0;
	int i = 0;

	while (sum < img.totalPxCount() * .02)
		sum += histogram[i++];

	m_blackpoint = float(i + 1) / (histogram.resolution() - 1);
}
template void ASinhStretch::computeBlackpoint(const Image8&);
template void ASinhStretch::computeBlackpoint(const Image16&);
template void ASinhStretch::computeBlackpoint(const Image32&);

float ASinhStretch::computeBeta() {

	float low = 0,
		high = 10000,
		mid = 5000;

	for (int i = 0; i < 20; i++){
		mid = (low + high) / 2;
		float multiplier_mid = mid / asinh(mid);
		(m_stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}

	return mid;
}

template<typename T, typename P>
static void testThread(int num_threads, Image<T>& img, P&& func) {

	std::vector<std::thread> threads(num_threads);
	int d = (img.end() - img.begin()) / num_threads;
	int r = (img.end() - img.begin()) % num_threads;

	for (int i = 0; i < num_threads; ++i)
		threads[i] = std::thread(func, img.begin() + i * d, img.begin() + (i + 1) * d);

	for (auto& t : threads)
		t.join();
}

static void testThread(int num_threads, int iterations, std::function<void (int,int)> func) {

	std::vector<std::thread> threads(num_threads);

	int step = iterations / num_threads;

	for (int i = 0; i < num_threads; ++i) {
		int r = (i + 1 == num_threads) ? iterations % num_threads : 0;
		threads[i] = std::thread(func, i * step, (i + 1) * step + r);
	}

	for (auto& t : threads)
		t.join();
}

template<typename T>
void ASinhStretch::applyMono(Image<T>& img) {

	float beta = computeBeta();
	float asinhb = asinh(beta);

	float max = img.computeMax(0);

#pragma omp parallel for num_threads(2)
	for (int el = 0; el < img.pxCount(); ++el) {
		float r = (Pixel<float>::toType(img[el]) - blackpoint()) / (max - blackpoint());
		img[el] = Pixel<T>::toType(math::clip((r != 0) ? asinh(r * beta) / asinhb : beta / asinhb));
	}
}
template void ASinhStretch::applyMono(Image8&);
template void ASinhStretch::applyMono(Image16&);
template void ASinhStretch::applyMono(Image32&);

template<typename T>
void ASinhStretch::applyRGB(Image<T>& img) {

	float beta = computeBeta();
	float asinhb = asinh(beta);

	float max = 0;
	for (int i = 0; i < 3; ++i)
		max = math::max(max, Pixel<float>::toType(img.computeMax(i)));

	auto rescale = [max, this](float pixel) { return (pixel - blackpoint()) / (max - blackpoint()); };

	std::array<float, 3> color_space = { 0.333333f, 0.333333f, 0.333333f };
	if (m_srbg) color_space = { 0.222491f, 0.716888f, 0.060621f };

#pragma omp parallel for num_threads(2)
	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			auto color = img.color<float>(x,y);

			float I = rescale(color_space[0] * color.red + color_space[1] * color.green + color_space[2] * color.blue);
			float k = (I != 0) ? asinh(beta * I) / (I * asinhb) : beta / asinhb;

			color.red = math::clip(rescale(color.red) * k);
			color.green = math::clip(rescale(color.green) * k);
			color.blue = math::clip(rescale(color.blue) * k);

			img.setColor<>(x, y, color);
		}
	}
}
template void ASinhStretch::applyRGB(Image8&);
template void ASinhStretch::applyRGB(Image16&);
template void ASinhStretch::applyRGB(Image32&);

template<typename Image>
void ASinhStretch::apply(Image& img) {

	if (img.channels() == 1)
		return applyMono(img);
	else if (img.channels() == 3)
		return applyRGB(img);
}
template void ASinhStretch::apply(Image8&);
template void ASinhStretch::apply(Image16&);
template void ASinhStretch::apply(Image32&);
