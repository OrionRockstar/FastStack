#pragma once
#include "ProcessDialog.h"
#include "ImageGeometry.h"
#include "ImageWindow.h"


class RotationDialog : public ProcessDialog {

	Rotation m_roatation;

	DoubleLineEdit* m_theta_le;

	QDial* m_dial;
	int m_dial_offset = 180;

	InterpolationComboBox* m_interpolate_cb;

public:
	RotationDialog(QWidget* parent = nullptr);

private:
	void actionDial(int action);

	void dialMoved(int pos);

	void editingFinished_theta();

	void resetDialog();

	void showPreview() {}

	void apply();
};








class FastRotationDialog : public ProcessDialog {

	FastRotation m_fastrotation;

	RadioButton* m_90cw_rb;
	RadioButton* m_90ccw_rb;
	RadioButton* m_180_rb;
	RadioButton* m_hm_rb;
	RadioButton* m_vm_rb;

public:
	FastRotationDialog(QWidget* parent = nullptr);

private:
	void resetDialog();

	void showPreview() {}

	void apply();
};





class IntegerResampleDialog : public ProcessDialog {

	IntegerResample m_ir;

	ComboBox* m_method_combo;

	QButtonGroup* m_type_bg;

	SpinBox* m_factor_sb;

public:
	IntegerResampleDialog(QWidget* parent);

private:
	void resetDialog();

	void showPreview() {}

	void apply();
};






class CropFrame : public QMdiSubWindow {

	int m_pen_width = 3;
	QRect m_default_geometry;

public:
	CropFrame(const QRect& rect, QWidget* parent = nullptr);

	QRect frameRect()const;

	QRect frameGeometry()const;

	void resetFrame();

private:
	void paintEvent(QPaintEvent* event);
};


class NonCropArea : public QWidget {

	const CropFrame* m_cf = nullptr;

public:
	NonCropArea(const CropFrame& cf, QWidget* parent);

private:
	void paintEvent(QPaintEvent* e);
};


template<typename T>
class CropPreview : public PreviewWindow<T> {

	CropFrame* m_cf = nullptr;
	NonCropArea* m_nca = nullptr;

public:
	CropPreview(QWidget* parent);

	QRect cropRect()const;

	void resetFrame();
};



class CropDialog : public ProcessDialog {

	int m_previous_index = 0;
	ComboBox* m_image_sel;
	Crop m_crop;

public:
	CropDialog(QWidget* parent);

private:
	void closeEvent(QCloseEvent* e);

	void onWindowOpen();

	void onWindowClose();

	void onActivation_imageSelection(int index);

	void resetDialog();

	void showPreview() {}

	void apply();
};





class ResizeDialog : public ProcessDialog {

	Resize m_rs;

	IntLineEdit* m_row_le;
	IntLineEdit* m_col_le;

	InterpolationComboBox* m_interpolation_combo;

public:
	ResizeDialog(QWidget* parent);

private:

	void resetDialog();

	void showPreview() {}

	void apply();
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

	void apply();
};