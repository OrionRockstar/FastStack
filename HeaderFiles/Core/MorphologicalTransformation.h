#pragma once
#include "Image.h"
#include "ProcessDialog.h"
#include "Maths.h"


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
	std::vector<int> m_mask_loc = std::vector<int>(0);

	float m_selection = 0.5;

	Type m_morph_filter = Type::erosion;

	float m_amount = 1.00;
	float m_orig_amount = 1 - m_amount;

	template <typename T>
	struct Kernel2D {
		const MorphologicalTransformation* m_obj;
		const Image<T>* m_img;

		std::unique_ptr<T[]> data;

		int m_dim = 0;
		int m_radius = 0;
		int m_size = 0;

		Kernel2D(const MorphologicalTransformation& m, const Image<T>& img) {

			m_dim = m.m_kernel_dim;
			m_radius = m.m_kernel_radius;

			m_size = m_dim * m_dim;
			data = std::make_unique<T[]>(m_size);

			m_obj = &m;
			m_img = &img;

		}

		Kernel2D(const Kernel2D<T>& other) {
			m_obj = other.m_obj;
			m_img = other.m_img;

			m_dim = other.m_dim;
			m_radius = other.m_radius;
			m_size = other.m_size;

			data = std::make_unique<T[]>(m_size);
			memcpy(data.get(), other.data.get(), m_size * sizeof(T));

		}

		~Kernel2D() {};

		void populate(int y, int ch) {
			for (int j = -m_radius, el = 0; j <= m_radius; ++j)
				for (int i = -m_radius; i <= m_radius; ++i)
					data[el++] = m_img->at_mirrored(i, y + j, ch);

		}

		void update(int x, int y, int ch) {

			int xx = x + m_radius;
			if (xx >= m_img->cols())
				xx = 2 * m_img->cols() - (xx + 1);

			for (int j = 0; j < m_size; j += m_dim) 
				for (int i = 0; i < m_dim - 1; ++i)
					data[j + i] = data[j + i + 1];

			for (int j = -m_radius, el = m_dim - 1; j <= m_radius; ++j, el+=m_dim) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				else if (yy >= m_img->rows())
					yy = 2 * m_img->rows() - (yy + 1);

				data[el] = (*m_img)(xx, yy, ch);

			}

		}

		T minimum()const {
			T min = std::numeric_limits<T>::max();
			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el)
				min = math::min(min, data[m_obj->m_mask_loc[el]]);
			return min;
		}

		T maximum()const {
			T max = std::numeric_limits<T>::min();
			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el)
				max = math::max(max, data[m_obj->m_mask_loc[el]]);
			return max;
		}

		T selection(int pivot)const {
			std::vector<T> k(m_obj->m_mask_loc.size());

			for (int el = 0; el < k.size(); ++el)
				k[el] = data[m_obj->m_mask_loc[el]];

			std::nth_element(&k[0], &k[pivot], &k[k.size()]);

			return k[pivot];
		}

		T median()const {

			std::vector<T> k(m_obj->m_mask_loc.size());

			for (int el = 0; el < k.size(); ++el)
				k[el] = data[m_obj->m_mask_loc[el]];

			int h = k.size() / 2;

			std::nth_element(&k[0], &k[h], &k[k.size()]);

			return k[h];
		}

		T midpoint()const {

			T min = std::numeric_limits<T>::max();
			T max = std::numeric_limits<T>::min();

			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el) {
				min = math::min(min, data[m_obj->m_mask_loc[el]]);
				max = math::max(max, data[m_obj->m_mask_loc[el]]);
			}

			return (0.5 * min + 0.5 * max);
		}

	};

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

private:

	template<typename T>
	T blend(T old_pixel, T new_pixel) {
		return T(m_orig_amount * old_pixel + m_amount * new_pixel);
	}

	void GetMaskedLocations();

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
