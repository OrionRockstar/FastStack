#pragma once
#include "Image.h"

enum class Component {
	red,
	green,
	blue,
	rgb_k
};

class HistogramTransformation {

	struct HistogramCurve {

		float m_shadow = 0;
		float m_midtone = 0.5;
		float m_highlights = 1.0;

		float m1 = m_midtone - 1;
		float m2 = 2 * m_midtone - 1;
		float dv = 1.0 / (m_highlights - m_shadow);

		std::vector<uint16_t> m_lut;

		HistogramCurve(float shadow, float midtone, float highlight) : m_shadow(shadow), m_midtone(midtone), m_highlights(highlight) {}

		HistogramCurve() = default;

		bool IsIdentity() { return (m_shadow == 0 && m_midtone == 0.5 && m_highlights == 1.0); }

		void ModifyShadow(float shadow) {
			m_shadow = shadow;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		void ModifyMidtone(float midtone) {
			m_midtone = midtone;
			m1 = m_midtone - 1;
			m2 = 2 * m_midtone - 1;
		}

		void ModifyHighlight(float hightlight) {
			m_highlights = hightlight;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float MTF(float pixel) {

			if (pixel <= 0.0f) return 0;

			else if (pixel >= 1) return 1;

			else if (pixel == m_midtone)  return 0.5;

			return (m1 * pixel) / ((m2 * pixel) - m_midtone);

		}

		float TransformPixel(float pixel) {
			pixel = (pixel - m_shadow) * dv;

			return MTF(pixel);
		}


		void Generate16Bit_LUT() {
			m_lut.resize(65536);
			for (int el = 0; el < 65536; ++el)
				m_lut[el] = TransformPixel(el / 65535.0f) * 65535;
		}

		void Generate8Bit_LUT() {
			m_lut.resize(256);
			for (int el = 0; el < 256; ++el)
				m_lut[el] = TransformPixel(el / 255.0f) * 255;
		}

		template <typename Image>
		void ApplyChannel(Image& img, int ch);
	};
public:
	HistogramCurve Red;
	HistogramCurve Green;
	HistogramCurve Blue;
	HistogramCurve RGB_K;

public:
	HistogramTransformation() = default;

	HistogramTransformation(Component component, float shadow, float midtone, float highlight);

	void ModifyShadow(Component component, float shadow);

	void ModifyMidtone(Component component, float midtone);

	void ModifyHighlight(Component component, float hightlight);

	template<typename T>
	void ComputeSTFCurve(Image<T>& img);

	template<typename T>
	void STFStretch(Image<T>& img);

	template<typename Image>
	void Apply(Image& img);

};

class HistogramTransformationWidget : public QDialog {
	Q_OBJECT

	HistogramTransformation ht;

public:
	HistogramTransformationWidget(QWidget* parent = nullptr) :QDialog(parent) {

		this->setGeometry(400, 400, 300, 300);
		this->setAttribute(Qt::WA_DeleteOnClose);
		//this->setFixedSize(300, 300);
			//this->setSizePolicy(QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Maximum)
		this->show();

	}

};