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







class CropFrame : public QWidget {

	const int m_pen_width = 4;
	const int m_pw_2 = m_pen_width / 2;

	QRect m_default_geometry;

	QCursor m_cursor = Qt::CursorShape::OpenHandCursor;

	QRect m_start_rect;
	QPoint m_start_pos;
	bool m_resizing = false;
	bool m_moving = false;

	RectBorder m_current_border = RectBorder::None;
	Frame m_frame;

public:
	CropFrame(const QRect& geometry, QWidget* parent = nullptr);

	QRect viewGeometry()const;

	void resetFrame();

private:
	void enterEvent(QEnterEvent* e);

	void leaveEvent(QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseMoveEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);

	void resizeEvent(QResizeEvent* e);

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
	CropPreview(QWidget* image_window);

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

	void onImageWindowCreated()override;

	void onImageWindowClosed()override;

	void onActivation_imageSelection(int index);

	void resetDialog();

	//void showPreview() {}

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