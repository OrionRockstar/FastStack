#pragma once
#include "Image.h"
#include "ProcessDialog.h"


enum class MorphologicalFilter {
	erosion,
	dialation,
	opening,
	closing,
	selection,
	median,
	midpoint
};

class MorphologicalTransformation {

	ProgressSignal* m_ps = new ProgressSignal();

	int m_kernel_dim = 3;
	int m_kernel_radius = (m_kernel_dim - 1) / 2;
	int m_kernel_size = m_kernel_dim * m_kernel_dim;

	std::vector<char> m_kmask = std::vector<char>(m_kernel_size, true);
	std::vector<int> m_mask_loc = std::vector<int>(0);

	float m_selection = 0.5;

	MorphologicalFilter m_morph_filter = MorphologicalFilter::erosion;

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

		void Populate(int y, int ch) {
			for (int j = -m_radius, el = 0; j <= m_radius; ++j)
				for (int i = -m_radius; i <= m_radius; ++i)
					data[el++] = m_img->At_mirrored(i, y + j, ch);

		}

		void Update(int x, int y, int ch) {

			int xx = x + m_radius;
			if (xx >= m_img->Cols())
				xx = 2 * m_img->Cols() - (xx + 1);

			for (int j = 0; j < m_size; j += m_dim) 
				for (int i = 0; i < m_dim - 1; ++i)
					data[j + i] = data[j + i + 1];

			for (int j = -m_radius, el = m_dim - 1; j <= m_radius; ++j, el+=m_dim) {

				int yy = y + j;
				if (yy < 0)
					yy = -yy;
				else if (yy >= m_img->Rows())
					yy = 2 * m_img->Rows() - (yy + 1);

				data[el] = (*m_img)(xx, yy, ch);

			}

		}

		T Minimum()const {
			T min = std::numeric_limits<T>::max();
			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el)
				min = Min(min, data[m_obj->m_mask_loc[el]]);
			return min;
		}

		T Maximum()const {
			T max = std::numeric_limits<T>::min();
			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el)
				max = Max(max, data[m_obj->m_mask_loc[el]]);
			return max;
		}

		T Selection(int pivot)const {
			std::vector<T> k(m_obj->m_mask_loc.size());

			for (int el = 0; el < k.size(); ++el)
				k[el] = data[m_obj->m_mask_loc[el]];

			std::nth_element(&k[0], &k[pivot], &k[k.size()]);

			return k[pivot];
		}

		T Median()const {

			std::vector<T> k(m_obj->m_mask_loc.size());

			for (int el = 0; el < k.size(); ++el)
				k[el] = data[m_obj->m_mask_loc[el]];

			int h = k.size() / 2;

			std::nth_element(&k[0], &k[h], &k[k.size()]);

			return k[h];
		}

		T Midpoint()const {

			T min = std::numeric_limits<T>::max();
			T max = std::numeric_limits<T>::min();

			for (int el = 0; el < m_obj->m_mask_loc.size(); ++el) {
				min = Min(min, data[m_obj->m_mask_loc[el]]);
				max = Max(max, data[m_obj->m_mask_loc[el]]);
			}

			return (0.5 * min + 0.5 * max);
		}

	};

public:

	MorphologicalTransformation() = default;

	MorphologicalTransformation(int kernel_dimension);

	ProgressSignal* progressSignal()const { return m_ps; }

	int KernelDimension()const { return m_kernel_dim; }

	int KernelSize()const { return m_kernel_size; }

	bool KernelMaskAt(int el) { return m_kmask[el]; }

	MorphologicalFilter morphologicalFilter()const { return m_morph_filter; }


	void setKernelMaskAt(int el, bool value) {
		m_kmask[el] = value;
	}

	void resizeKernel(int new_dim);

	float Selection()const { return m_selection; }

	void setSelection(float selection) {
		m_selection = selection;
	}

	float Amount()const { return m_amount; }

	void setAmount(float amount) {
		m_amount = amount;
		m_orig_amount = 1.00 - m_amount;
	}

	void setMorphologicalFilter(MorphologicalFilter filter) { m_morph_filter = filter; }

	void setMask_All(bool value);

	void setMask_Circular();

	void setMask_Diamond();

	void RotateMask();

	void InvertMask();

	ProgressSignal* signalObject()const  {return m_ps;}

private:

	template<typename T>
	T Amount(T old_pixel, T new_pixel) {
		return T(m_orig_amount * old_pixel + m_amount * new_pixel);
	}

	void GetMaskedLocations();

	template <typename T>
	void Erosion(Image<T>&img);

	template <typename T>
	void Dialation(Image<T>&img);

	template <typename T>
	void Opening(Image<T>&img);

	template <typename T>
	void Closing(Image<T>&img);

	template <typename T>
	void Selection(Image<T>&img);

	template <typename T>
	void Median(Image<T>&img);

	template <typename T>
	void Midpoint(Image<T>&img);

	template <typename T>
	void FastMedian3x3(Image<T>& img);

	template <typename T>
	void FastMedian5x5(Image<T>& img);

	template <typename T>
	void FastMedian7x7(Image<T>& img);

	template <typename T>
	void FastMedian9x9(Image<T>& img);

	template <typename T>
	void FastMedian(Image<T>&img, int kernel_dim);

public:
	template <typename T>
	void Apply(Image<T>& src);
};




class MorphologicalKernelScene : public QGraphicsScene {

	MorphologicalTransformation* m_mt;
	QList<QGraphicsItem*> m_items;

	std::array<QPen, 2> m_pens = { QPen(QColor(127,127,127), 0.5), QPen(QColor(0, 0, 0), 0.5) };
	std::array<QBrush, 2> m_brushes = { Qt::transparent, QColor(255,255,255) };

public:
	MorphologicalKernelScene(MorphologicalTransformation& mt, QRect rect);

	void drawElements();

	void recolorElements();

	void mousePressEvent(QGraphicsSceneMouseEvent* event);
};


class MorphologicalTransformationDialog : public ProcessDialog {

	MorphologicalTransformation m_mt;

	QGraphicsView* m_gv;
	MorphologicalKernelScene* m_mks;

	DoubleLineEdit* m_selection_le;
	QSlider* m_selection_slider;

	DoubleLineEdit* m_amount_le;
	QSlider* m_amount_slider;

	QComboBox* m_filter_cb;

	QComboBox* m_kerenl_size_cb;

public:
	MorphologicalTransformationDialog(QWidget* parent = nullptr);

private slots:
	void setNewKernelSize(int index);

	void setMorphologicalFilter(int index);

	void setMask_true();

	void setMask_false();

	void setMask_Circular();

	void setMask_Diamond();

	void invertMask();

	void rotateMask();

	void editingFinished_selection();

	void onActionTriggered_selection(int action);

	void editingFinished_amount();

	void onActionTriggered_amount(int action);

private:
	void AddKernelPB();

	void AddOperationsCombo();

	void AddKernelScene();

	void AddKernelSizeCombo();

	void AddFilterSelectionCombo();

	void AddSelectionInputs();

	void AddAmountInputs();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();
};
