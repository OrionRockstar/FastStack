#pragma once
#include "CustomWidgets.h"

class MaskSelectionDialog : Dialog {
	Q_OBJECT

	QWidget* m_image_window = nullptr;

	QLineEdit* m_current_mask;
	ComboBox* m_new_mask;

public:
	MaskSelectionDialog(QWidget* image_window, QMdiArea* parent);

private:
	void apply();
};





class ImageWindowMenu : public QMenu {

	
	QWidget* m_image_window = nullptr;
	QMenu* m_mask = nullptr;
	QMenu* m_zwc_menu = nullptr;

	QButtonGroup* bg;

	std::array<QColor, 8> m_colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow,
										Qt::magenta, Qt::cyan, {255,165,0}, {69,0,128} };

	std::array<QString, 8> m_color_names = { "Red","Green","Blue", "Yellow",
										"Magenta", "Cyan", "Orange", "Purple" };

public:
	ImageWindowMenu(QWidget& image_window, QWidget* parent);

private:
	void removeMask();

	void showMask(bool show = true);

	void invertMask(bool invert = false);

	void enableMask(bool enable = true);

	void setMaskColor(const QColor& color);

	void addMaskMenu();

	void showMaskSelectionDialog();

	void addMaskColorSelection();

	void addZoomWindowColorMenu();
};