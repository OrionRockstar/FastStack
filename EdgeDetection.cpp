#include "pch.h"
#include "EdgeDetection.h"
#include "FastStack.h"
#include "RGBColorSpace.h"

template<typename T>
EdgeDetection::Gradient EdgeDetection::Sobel(const Image<T>& img, const ImagePoint& p) {
	float sumx = 0, sumy = 0;

	for (int j = -1, el = 0; j < 2; ++j) {
		for (int i = -1; i < 2; ++i, ++el) {
			float pixel = Pixel<float>::toType(img.At_mirrored(p.x() + i, p.y() + j, p.channel()));
			sumx += pixel * m_sobelx[el];
			sumy += pixel * m_sobely[el];
		}
	}

	return { sqrtf(sumx * sumx + sumy * sumy), atanf(sumy / sumx) };
}

template<typename T>
EdgeDetection::Gradient EdgeDetection::Scharr(const Image<T>& img, const ImagePoint& p) {
	float sumx = 0, sumy = 0;

	for (int j = -1, el = 0; j < 2; ++j) {
		for (int i = -1; i < 2; ++i, ++el) {
			float pixel = Pixel<float>::toType(img.At_mirrored(p.x() + i, p.y() + j, p.channel()));
			sumx += pixel * m_scharrx[el];
			sumy += pixel * m_scharry[el];
		}
	}

	return { sqrtf(sumx * sumx + sumy * sumy), atanf(sumy / sumx) };
}

template<typename T>
EdgeDetection::Gradient EdgeDetection::Roberts_Cross(const Image<T>& img, const ImagePoint& p) {
	float sumx = 0, sumy = 0;

	float pixel = Pixel<float>::toType(img(p.x(), p.y(), p.channel()));
	sumx += pixel * m_robertsx[0];
	sumy += pixel * m_robertsy[0];

	pixel = Pixel<float>::toType(img.At_mirrored(p.x() + 1, p.y(), p.channel()));
	sumx += pixel * m_robertsx[1];
	sumy += pixel * m_robertsy[1];

	pixel = Pixel<float>::toType(img.At_mirrored(p.x(), p.y() + 1, p.channel()));
	sumx += pixel * m_robertsx[2];
	sumy += pixel * m_robertsy[2];

	pixel = Pixel<float>::toType(img.At_mirrored(p.x() + 1, p.y() + 1, p.channel()));
	sumx += pixel * m_robertsx[3];
	sumy += pixel * m_robertsy[3];

	return { sqrtf(sumx * sumx + sumy * sumy), atanf(sumy / sumx) };
}

template<typename T>
EdgeDetection::Gradient EdgeDetection::Prewitt(const Image<T>& img, const ImagePoint& p) {
	float sumx = 0, sumy = 0;

	for (int j = -1, el = 0; j < 2; ++j) {
		for (int i = -1; i < 2; ++i, ++el) {
			float pixel = Pixel<float>::toType(img.At_mirrored(p.x() + i, p.y() + j, p.channel()));
			sumx += pixel * m_prewittx[el];
			sumy += pixel * m_prewitty[el];
		}
	}

	return { sqrtf(sumx * sumx + sumy * sumy), atanf(sumy / sumx) };
}

QString EdgeDetection::getOperator_qstring()const {

	QList<QString> l = { "Sobel_","Scharr_","Roberts_Cross_","Prewitt_" };

	return l[int(m_operator)];
}

template<typename T>
static void RGBtoGray(const Image<T>& src, Image<T>& dst) {
	src.copyTo(dst);
	dst.RGBtoGray();
}

template<typename T>
Image32 EdgeDetection::Apply(const Image<T>& img) {

	auto gradient = [this](const Image<T>& img, const ImagePoint& p) {
		switch (m_operator) {
		case Operator::sobel: 
			return Sobel(img, p);
		
		case Operator::scharr: 
			return Scharr(img, p);
		
		case Operator::roberts_cross: 
			return Roberts_Cross(img, p);
		
		case Operator::prewitt: 
			return Prewitt(img, p);
		
		default:
			return EdgeDetection::Gradient(0,0);
		}
	};

	Image<float> g_mag(img.rows(), img.cols());
	Image<float> g_theta(img.rows(), img.cols());

//compute operator gradients and gradient direction
	if (img.channels() == 3) {
		Image<T> gray;
		img.copyTo(gray);
		gray.RGBtoGray();

#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				Gradient g = gradient(gray, { x,y });
				g_mag(x, y) = g.magnitude;
				g_theta(x, y) = g.theta;
			}
		}
	}

	else {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				Gradient g = gradient(img, { x,y });
				g_mag(x, y) = g.magnitude;
				g_theta(x, y) = g.theta;
			}
		}
	}

	g_mag.normalize();

	float median = g_mag.computeMedian(0, true);

	float minVal = median;
	float maxVal = 3 * median;

	Image8 strong(img.rows(), img.cols());

	//identify sure edge pixels
#pragma omp parallel for
	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			float theta = g_theta(x, y);
			float a = g_mag.At(x + 1.5 * cos(theta), y + 1.5 * sin(theta));
			theta += std::_Pi;
			float b = g_mag.At(x + 1.5 * cos(theta), y + 1.5 * sin(theta));
			float g = g_mag(x, y);

			if (g > Max(a, b)) {
				if (g > maxVal)
					strong(x, y) = 1;
				else if (minVal < g && g <= maxVal)
					0;
				else
					g_mag(x, y, 0) = 0;
			}
			else
				g_mag(x, y, 0) = 0;
		}
	}

	auto edgepixel = [&](int x, int y) {
		for (int j = -1; j < 2; ++j) 
			for (int i = -1; i < 2; ++i) 
				if (strong.At(x + i, y + j) == 1)
					return true;
		return false;
	};

	Image32 out(img.rows(), img.cols());

	//identify sure edge pixels cont.
#pragma omp parallel for
	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			float g = g_mag(x, y);

			if (g > maxVal)
				out(x, y) = g;

			else if (minVal < g && g <= maxVal) {
				if (edgepixel(x, y))
					out(x, y) = g;
				else
					out(x, y) = 0;
			}
			else
				out(x, y) = 0;
		}
	}

	return out;
}
template Image32 EdgeDetection::Apply(const Image8&);
template Image32 EdgeDetection::Apply(const Image16&);
template Image32 EdgeDetection::Apply(const Image32&);




EdgeDetectionDialog::EdgeDetectionDialog(QWidget* parent) :ProcessDialog("Canny Edge Detection", QSize(220, 180), FastStack::recast(parent)->workspace(), false) {
	
	connect(this, &ProcessDialog::processDropped, this, &EdgeDetectionDialog::Apply);
	ConnectToolbar(this, &ProcessDialog::CreateDragInstance, &EdgeDetectionDialog::Apply, &EdgeDetectionDialog::showPreview, &EdgeDetectionDialog::resetDialog);

	GroupBox* gb = new GroupBox("Operator", this);
	gb->move(10, 10);
	gb->resize(200, 135);

	QVBoxLayout* layout = new QVBoxLayout;
	//QLabel* l = new QLabel("Operator:", this);
	//l->move(10, 10);

	m_operator_bg = new QButtonGroup(this);

	using EDO = EdgeDetection::Operator;

	RadioButton* rb = new RadioButton("Sobel", this);
	//rb->move(30, 35);
	rb->setChecked(true);
	m_operator_bg->addButton(rb, int(EDO::sobel));
	layout->addWidget(rb);

	rb = new RadioButton("Scharr", this);
	//rb->move(30, 60);
	m_operator_bg->addButton(rb, int(EDO::scharr));
	layout->addWidget(rb);


	rb = new RadioButton("Roberts Cross", this);
	//rb->move(30, 85);
	m_operator_bg->addButton(rb, int(EDO::roberts_cross));
	layout->addWidget(rb);


	rb = new RadioButton("Prewitt", this);
	//rb->move(30, 110);
	m_operator_bg->addButton(rb, int(EDO::prewitt));
	layout->addWidget(rb);
	gb->setLayout(layout);

	connect(m_operator_bg, &QButtonGroup::idClicked, this, [this](int id) { m_ed.setOperator(EdgeDetection::Operator(id)); });

	show();
}

void EdgeDetectionDialog::Apply() {
	using ED = EdgeDetection;

	if (m_workspace->subWindowList().size() == 0)
		return;

	setEnabledAll(false);

	auto iwptr = reinterpret_cast<ImageWindow8*>(m_workspace->currentSubWindow()->widget());

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		Image32 img32 = m_ed.Apply(iwptr->source());
		ImageWindow32* niw = new ImageWindow32(img32, m_ed.getOperator_qstring() + iwptr->name(), m_workspace);
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = reinterpret_cast<ImageWindow16*>(iwptr);
		Image32 img32 = m_ed.Apply(iw16->source());
		ImageWindow32* niw = new ImageWindow32(img32, m_ed.getOperator_qstring() + iw16->name(), m_workspace);
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = reinterpret_cast<ImageWindow32*>(iwptr);
		Image32 img32 = m_ed.Apply(iw32->source());
		ImageWindow32* niw = new ImageWindow32(img32, m_ed.getOperator_qstring() + iw32->name(), m_workspace);
		break;
	}
	}

	setEnabledAll(true);
}