#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "Maths.h"

template<typename T>
class Kernel {
	const Image<T>* m_img;
	std::unique_ptr<T[]> m_data;

	int m_dim = 0;
	int m_radius = 0;
	int m_size = 0;

public:
	Kernel(const Image<T>& img, size_t dimension);

	Kernel(const Kernel<T>& kernel);

	T& operator[](size_t el) { return m_data[el]; }

	const T& operator[](size_t el)const { return m_data[el]; }

	T& operator()(int x, int y) { return m_data[y * m_dim + x]; }

	const T& operator()(int x, int y)const { return m_data[y * m_dim + x]; }

	int dimension()const { return m_dim; }

	int radius()const { return m_radius; }

	int count()const { return m_size; }

	virtual void populate(int y, int ch);

	virtual void update(int x, int y, int ch);
};



class MorphologicalTransformation;

template<typename T>
class MorphologicalKernel : public Kernel<T> {

	std::vector<uint16_t> m_locations;
	std::vector<T> m_masked_data;

public:
	MorphologicalKernel(const Image<T>& img, const MorphologicalTransformation& mt);

	void populate(int y, int ch)override;

	void update(int x, int y, int ch)override;

private:
	void setMaskedData();

public:
	T minimum()const;

	T maximum()const;

	T selection(int pivot);

	T median();

	T midpoint()const;
};


class MorphologicalTransformation {

public:
	enum class Type {
		erosion,
		dialation,
		opening,
		closing,
		selection,
		median,
		midpoint
	};

private:
	ProgressSignal* m_ps = new ProgressSignal();

	int m_kernel_dim = 3;
	int m_kernel_radius = (m_kernel_dim - 1) / 2;
	int m_kernel_size = m_kernel_dim * m_kernel_dim;

	std::vector<char> m_kmask = std::vector<char>(m_kernel_size, true);

	float m_selection = 0.5;

	Type m_morph_filter = Type::erosion;

	float m_amount = 1.00;
	float m_orig_amount = 1 - m_amount;

public:
	MorphologicalTransformation() = default;

	MorphologicalTransformation(Type filter_type) : m_morph_filter(filter_type) {}

	ProgressSignal* progressSignal()const { return m_ps; }

	int kernelDimension()const { return m_kernel_dim; }

	int kernelSize()const { return m_kernel_size; }

	bool kernelMaskAt(int el) { return m_kmask[el]; }

	Type morphologicalFilter()const { return m_morph_filter; }

	void setMorphologicalFilter(Type filter) { m_morph_filter = filter; }

	void setKernelMaskAt(int el, bool value) {
		m_kmask[el] = value;
	}

	void resizeKernel(int new_dim);

	float selectionPoint()const { return m_selection; }

	void setSelectionPoint(float selection) {
		m_selection = selection;
	}

	float blendAmount()const { return m_amount; }

	void setBlendAmount(float amount) {
		m_amount = amount;
		m_orig_amount = 1.00 - m_amount;
	}

	void setMask_All(bool value);

	void setMask_Circular();

	void setMask_Diamond();

	void invertMask();

	void rotateMask();

	ProgressSignal* signalObject()const  {return m_ps;}

	std::vector<uint16_t> maskedLocations()const;

private:
	int maskCount()const;

	template<typename T>
	T blend(T old_pixel, T new_pixel) {
		return T(m_orig_amount * old_pixel + m_amount * new_pixel);
	}

	template <typename T>
	void erosion(Image<T>&img);

	template <typename T>
	void dialation(Image<T>&img);

	template <typename T>
	void opening(Image<T>&img);

	template <typename T>
	void closing(Image<T>&img);

	template <typename T>
	void selection(Image<T>&img);

	template <typename T>
	void median(Image<T>&img);

	template <typename T>
	void midpoint(Image<T>&img);

	template <typename T>
	void fastMedian3x3(Image<T>& img);

	template <typename T>
	void fastMedian5x5(Image<T>& img);

	template <typename T>
	void fastMedian7x7(Image<T>& img);

	template <typename T>
	void fastMedian9x9(Image<T>& img);

	template <typename T>
	void fastMedian(Image<T>&img, int kernel_dim);

public:
	template <typename T>
	void apply(Image<T>& src);
};
