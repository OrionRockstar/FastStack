#include "RGBColorSpace.h"
#include "Image.h"

#pragma once
class SCNR {

public:
	enum class Colors : uint8_t {
		red,
		green,
		blue
	};

	enum class Method : uint8_t {
		maximum_mask,
		additive_mask,
		average_neutral,
		maximum_neutral
	};

private:
	float m_amount = 1.0;
	Colors m_remove_color = Colors::green;
	Method m_protection_method = Method::average_neutral;
	bool m_preserve_lightness = true;

	float maskProtection(float color, float v)const {
		return color * (1 - amount())* (1 - v) + v * color;
	}

	void removeRed(Color<float>& color)const;

	void removeGreen(Color<float>& color)const;

	void removeBlue(Color<float>& color)const;

	void removeColor(Color<float>& color)const;

public:
	Colors removeColor()const { return m_remove_color; }

	void setRemoveColor(Colors color) { m_remove_color = color; }

	Method protectionMethod()const { return m_protection_method; }

	void setProtectionMethod(Method method) { m_protection_method = method; }

	float amount()const { return m_amount; }

	void setAmount(float amount) { m_amount = amount; }

	bool preserveLightness()const { return m_preserve_lightness; }
	
	void setPreserveLightness(bool preserve) { m_preserve_lightness = preserve; }

	template <typename T>
	void apply(Image<T>& img);
};

