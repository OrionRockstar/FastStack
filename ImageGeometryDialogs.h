#pragma once
#include "ProcessDialog.h"
#include "ImageGeometry.h"
#include "ImageWindow.h"


class RotationDialog : public ProcessDialog {

	Rotation m_roatation;

	DoubleLineEdit* m_theta_le;

	QDial* m_dial;
	int m_dial_offset = 180;

	QComboBox* m_interpolate_cb;

public:
	RotationDialog(QWidget* parent = nullptr);

private:
	void actionDial(int action);

	void dialMoved(int pos);

	void editingFinished_theta();

	void resetDialog();

	void showPreview() {}

	void Apply();
};








class FastRotationDialog : public ProcessDialog {

	FastRotation m_fastrotation;

	QRadioButton* m_90cw_rb;
	QRadioButton* m_90ccw_rb;
	QRadioButton* m_180_rb;
	QRadioButton* m_hm_rb;
	QRadioButton* m_vm_rb;

public:
	FastRotationDialog(QWidget* parent = nullptr);

private:
	void resetDialog();

	void showPreview() {}

	void Apply();
};





class IntegerResampleDialog : public ProcessDialog {

	IntegerResample m_ir;

	QComboBox* m_method_combo;

	QButtonGroup* m_type_bg;

	QSpinBox* m_factor_sb;

public:
	IntegerResampleDialog(QWidget* parent);

private:
	void resetDialog();

	void showPreview() {}

	void Apply();
};






class CropDialog : public ProcessDialog {

	template<typename T>
	class CropPreview : public PreviewWindow<T> {

		CropDialog* m_cd;
		QMdiSubWindow* m_rb;

	public:
		CropPreview(CropDialog* cd, QWidget* parent);

		QRect cropRect()const;

		void closeEvent(QCloseEvent* e) {
			PreviewWindow<T>::closeEvent(e);
			m_cd->m_image_sel->setCurrentIndex(0);
		}
	};


	QComboBox* m_image_sel;
	Crop m_crop;

public:
	CropDialog(QWidget* parent);

private:
	void onWindowOpen();

	void onWindowClose();

	void onActivation_imageSelection(int index);

	void resetDialog();

	void showPreview() {}

	void Apply();
};








class HomographyTransformationDialog :public ProcessDialog {

	HomographyTransformation m_transformation;
	QGridLayout* m_layout;
	std::array<DoubleLineEdit*, 9> m_le_array;

public:
	HomographyTransformationDialog(QWidget* parent = nullptr);

private:
	void onEditingFinished();

	void resetDialog();

	void showPreview() {}

	void Apply();
};