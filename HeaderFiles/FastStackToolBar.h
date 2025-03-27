#pragma once
#include <qtoolbar.h>
#include "Maths.h"
#include "Image.h"

class ImageInformationLabel : public QLabel {

	const QString m_rowString = "rows: ";

	const QString m_colString = "cols: ";

	const QString m_channelString = "Channels: ";

	const QString m_bitdepthString = "Bitdepth: ";

public:
	ImageInformationLabel(QWidget* parent);

	void displayText(const QMdiSubWindow* window);
};

class PixelValueLabel : public QLabel {

	const QString m_xStr = "x: ";

	const QString m_yStr = "y: ";

	const QString m_KStr = "K: ";

	const QString m_rStr = "R: ";

	const QString m_gStr = "G: ";

	const QString m_bStr = "B: ";

	template<typename T>
	QString rgb_k(const Image<T>* img, const QPointF& p) {
		QString txt = "";

		if (img->channels() == 1)
			txt += m_KStr + QString::number(Pixel<float>::toType((*img)(p.x(), p.y())), 'f', 4) + "   ";

		else if (img->channels() == 3) {
			txt += m_rStr + QString::number(Pixel<float>::toType((*img)(p.x(), p.y(), 0)), 'f', 4) + "   ";
			txt += m_gStr + QString::number(Pixel<float>::toType((*img)(p.x(), p.y(), 1)), 'f', 4) + "   ";
			txt += m_bStr + QString::number(Pixel<float>::toType((*img)(p.x(), p.y(), 2)), 'f', 4) + "   ";
		}

		return txt;
	}
	
	void addPixelValue(QString& txt, const Image8* img, const QPointF& p);

public:
	PixelValueLabel(QWidget* parent);

	void displayText(const Image8* img, const QPointF& p);

	void displayPreviewText(const Image8* img, const QPointF& p, float factor, const QPointF offset = QPointF(0,0));
};


class FastStackToolBar:public QToolBar {

	//QMdiArea* m_workspace;

	ImageInformationLabel* m_img_info = nullptr;

	PixelValueLabel* m_pix_val = nullptr;
public:
	FastStackToolBar(QWidget* parent);

	ImageInformationLabel* imageInformationLabel()const { return m_img_info; }

	PixelValueLabel* pixelValueLabel()const { return m_pix_val; }
};

