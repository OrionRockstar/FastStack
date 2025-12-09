#pragma once
#include "Image.h"
#include "ImageWindow.h"
#include "StarDetector.h"
#include "ImageStackingDialog.h"


class StarAlignment {
	StarDetector m_sd;// = StarDetector(200);

	uint16_t m_maxstars = 200;
	std::vector<ImageWindow8*> m_img_windows;

public:
	StarAlignment() = default;

	void setImages(std::vector<ImageWindow8*> imgs) { m_img_windows = imgs; }

	StarDetector* starDetector() { return &m_sd; }

	void apply();
};



class ImageItem {

	ImageWindow8* m_data = nullptr;
	QString m_text = "";

public:
	ImageItem(const QString& txt, ImageWindow8* data) : m_text(txt), m_data(data) {}

	QString text()const { return m_text; }

	ImageWindow8* data()const { return m_data; }
};



class ImageSelectionDialog : public QDialog {
	Q_OBJECT

	QListWidget* m_img_list = nullptr;

	PushButton* m_ok_pb = nullptr;
public:
	ImageSelectionDialog(QMdiArea& workspace, QWidget* parent);

signals:
	void sendItem(ImageItem item);
};



class StarAlignmentDialog : public ProcessDialog {

	StarAlignment m_sa;

	const int m_button_width = 115;

	ListWidget* m_img_list = nullptr;
	std::vector<ImageWindow8*> m_imgs;

	PushButton* m_add_img_pb = nullptr;
	PushButton* m_remove_item_pb = nullptr;
	PushButton* m_clear_pb = nullptr;

	StarDetectionGroupBox* m_sd_gb = nullptr;
public:
	StarAlignmentDialog(Workspace* parent);

private:
	void addImages();

	void resetDialog();

	void showPreview() {}

	void apply();
};