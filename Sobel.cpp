#include "pch.h"
#include "Sobel.h"
#include "FastStack.h"

template<typename T>
Image32 Sobel::Apply(const Image<T>&img) {

	Image<float> temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				float sumx = 0, sumy = 0;

				for (int j = -1, el = 0; j < 2; ++j) {
					for (int i = -1; i < 2; ++i, ++el) {
						float pixel = Pixel<float>::toType(img.At_mirrored(x + i, y + j, ch));
						sumx += pixel * m_sx[el];
						sumy += pixel * m_sy[el];
					}
				}
				temp(x, y, ch) = sqrtf(sumx * sumx + sumy * sumy);

			}
		}
	}

	//temp.MoveTo(img);
	temp.Normalize();
	return temp;
}
template Image32 Sobel::Apply(const Image8&);
template Image32 Sobel::Apply(const Image16&);
template Image32 Sobel::Apply(const Image32&);


SobelDialog::SobelDialog(QWidget* parent) :ProcessDialog("Sobel Edge Detection", QSize(250, 40), *reinterpret_cast<FastStack*>(parent)->workspace(), parent, false) {

	connect(this, &ProcessDialog::processDropped, this, &SobelDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &SobelDialog::Apply, &SobelDialog::showPreview, &SobelDialog::resetDialog);

	show();
}

void SobelDialog::Apply() {
	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->Source().Bitdepth()) {
	case 8: {
		Image32 img32 = m_sobel.Apply(iwptr->Source());
		ImageWindow32* niw = new ImageWindow32(img32, "Sobel_" + iwptr->ImageName(), m_workspace);
		break;
	}
	case 16: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		Image32 img32 = m_sobel.Apply(iw16->Source());
		ImageWindow32* niw = new ImageWindow32(img32, "Sobel_" + iw16->ImageName(), m_workspace);		
		break;
	}
	case -32: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		Image32 img32 = m_sobel.Apply(iw32->Source());
		ImageWindow32* niw = new ImageWindow32(img32, "Sobel_" + iw32->ImageName(), m_workspace);
		break;
	}
	}

	setEnabledAll(true);
}