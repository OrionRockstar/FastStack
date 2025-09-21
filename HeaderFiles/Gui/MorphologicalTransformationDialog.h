#pragma once

#include "MorphologicalTransformation.h"
#include "ProcessDialog.h"

class MorphologicalKernelScene : public QGraphicsScene {

	MorphologicalTransformation* m_mt;
	QList<QGraphicsItem*> m_items;

	std::array<QPen, 2> m_pens = { QPen(QColor(169,169,169), 0.5), QPen(QColor(0, 0, 0), 0.5) };
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
	Slider* m_selection_slider;

	DoubleLineEdit* m_amount_le;
	Slider* m_amount_slider;

	ComboBox* m_filter_cb;

	ComboBox* m_kerenl_size_cb;

public:
	MorphologicalTransformationDialog(QWidget* parent = nullptr);

private slots:
	void setMask_true();

	void setMask_false();

	void setMask_Circular();

	void setMask_Diamond();

	void invertMask();

	void rotateMask();

private:
	void addKernelPB();

	void addKernelScene();

	void addKernelSizeCombo();

	void addFilterSelectionCombo();

	void addSelectionInputs();

	void addAmountInputs();

	void resetDialog();

	void apply();
};