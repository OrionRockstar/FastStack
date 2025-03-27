#include "pch.h"
#include "Binerize.h"
#include "FastStack.h"

template<typename T>
void Binerize::apply(Image<T>& img) {

	for (int ch = 0; ch < img.channels(); ++ch)
		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = (*pixel > m_threshold[ch]) ? Pixel<T>::max() : 0;
}
template void Binerize::apply(Image8&);
template void Binerize::apply(Image16&);
template void Binerize::apply(Image32&);