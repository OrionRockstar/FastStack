#include "pch.h"
#include "StarAlignment.h"
#include "StarMatching.h"
#include "Homography.h"
#include "ImageGeometry.h"
#include "FastStack.h"

void StarAlignment::apply() {

	StarMatching sm;
	HomographyTransformation ht;

	auto resize = [this](Image8* img, int r, int c) {

		Resize rsz;
		rsz.setInterpolation(Interpolator::Type::nearest_neighbor);
		rsz.setNewSize(r, c);

		switch (img->type()) {
		case ImageType::UBYTE:
			return rsz.apply(*img);

		case ImageType::USHORT:
			return rsz.apply(*reinterpret_cast<Image16*>(img));

		case ImageType::FLOAT:
			return rsz.apply(*reinterpret_cast<Image32*>(img));

		default:
			break;
		}
	};


	auto stardetection = [this](const Image8* img) {

		StarVector stars;

		switch (img->type()) {
		case ImageType::UBYTE:
			stars = m_sd.applyStarDetection(*img);
			break;
		case ImageType::USHORT:
			stars = m_sd.applyStarDetection(*reinterpret_cast<const Image16*>(img));
			break;
		case ImageType::FLOAT:
			stars = m_sd.applyStarDetection(*reinterpret_cast<const Image32*>(img));
			break;
		default:
			 m_sd.applyStarDetection(*img);
			 break;
		}

		if (stars.size() > m_maxstars)
			stars.resize(m_maxstars);

		return stars;
	};

	
	StarVector ref_sv;
	const Image8* ref_img;

	for (int i = 0; i < m_img_windows.size(); ++i) {
		Image8* img = &m_img_windows[i]->source();

		if (ref_sv.size() == 0) {
			ref_sv = stardetection(img);
			ref_img = img;
		}

		else {

			if (!img->isSameSize(*ref_img))
				resize(img, ref_img->rows(), ref_img->cols());

			StarVector tgt_sv = stardetection(img);
			StarPairVector spv = sm.matchStars(ref_sv, tgt_sv);
			Matrix h = Homography::computeHomography(spv);

			if (isnan(h(0, 0)))
				continue; //use to pass over bad frame

			ht.setHomography(h);

			switch (img->type()) {

			case ImageType::UBYTE:
				m_img_windows[i]->applyToSource_Geometry(ht, &HomographyTransformation::apply);
				break;

			case ImageType::USHORT: {
				auto iw16 = reinterpret_cast<ImageWindow16*>(m_img_windows[i]);
				iw16->applyToSource_Geometry(ht, &HomographyTransformation::apply);
				break;
			}
			case ImageType::FLOAT: {
				auto iw32 = reinterpret_cast<ImageWindow32*>(m_img_windows[i]);
				iw32->applyToSource_Geometry(ht, &HomographyTransformation::apply);
				break;
			}
			}
		}
	}
}



ImageSelectionDialog::ImageSelectionDialog(QMdiArea& workspace, QWidget* parent) : QDialog(parent) {

	this->setAttribute(Qt::WA_DeleteOnClose);
	this->setWindowTitle("Image Selection");
	this->resize(210, 305);
	this->setModal(true);

	m_img_list = new QListWidget(this);
	m_img_list->resize(180, 250);
	m_img_list->move(15, 15);

	for (auto sw : workspace.subWindowList()) {
		auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
		auto item = new QListWidgetItem(iw->name());
		item->setCheckState(Qt::Checked);
		m_img_list->addItem(item);
	}



	m_ok_pb = new PushButton("OK", this);
	m_ok_pb->move(60, 270);

	auto on_ok = [&,this]() {

		std::vector<Image8*> imgs;

		for (int i = 0; i < workspace.subWindowList().size(); ++i) {
			auto sw = workspace.subWindowList()[i];
			auto iw = reinterpret_cast<ImageWindow8*>(sw->widget());
			if (m_img_list->item(i)->checkState() == Qt::Checked)
				emit sendItem({ m_img_list->item(i)->text(), iw });
		}
		
		this->close();
	};

	connect(m_ok_pb, &PushButton::released, this, on_ok);

	this->show();
}



StarAlignmentDialog::StarAlignmentDialog(QWidget* parent) : ProcessDialog("StarAlignment", QSize(540, 465), FastStack::recast(parent)->workspace(), false, false) {

	connectToolbar(this, &StarAlignmentDialog::apply, &StarAlignmentDialog::showPreview, &StarAlignmentDialog::resetDialog);

	m_img_list = new ListWidget(drawArea());
	m_img_list->resize(365, m_img_list->sizeHint().height());
	m_img_list->move(25, 15);

	m_add_img_pb = new PushButton("Add Images", drawArea());
	m_add_img_pb->move(405, 15);
	m_add_img_pb->setFixedWidth(m_button_width);


	connect(m_add_img_pb, &QPushButton::released, this, &StarAlignmentDialog::addImages);

	m_remove_item_pb = new PushButton("Remove Item", drawArea());
	m_remove_item_pb->move(405, 55);
	m_remove_item_pb->setFixedWidth(m_button_width);

	auto removeimg = [this]() {
		int row = m_img_list->currentRow();
		m_img_list->removeItemWidget(m_img_list->item(row));
		m_imgs.erase(m_imgs.begin() + row);
	};
	connect(m_remove_item_pb, &QPushButton::released, this, removeimg);


	m_clear_pb = new PushButton("Clear List", drawArea());
	m_clear_pb->move(405, 95);
	m_clear_pb->setFixedWidth(m_button_width);

	auto clear = [this]() {
		m_img_list->clear();
		m_imgs.clear();
	};
	connect(m_clear_pb, &QPushButton::released, this, clear);

	m_sd_gb = new StarDetectionGroupBox(*m_sa.starDetector(), drawArea(), true);
	m_sd_gb->move(10, m_img_list->sizeHint().height() + 30);
	this->show();
}

void StarAlignmentDialog::addImages() {

	ImageSelectionDialog* isd = new ImageSelectionDialog(*m_workspace, this);

	auto additem = [this](ImageItem item) {

		for (int row = 0; row < m_img_list->count(); ++row)
			if (m_img_list->item(row)->text() == item.text())
				return;

		m_img_list->addItem(item.text());
		m_imgs.push_back(item.data());
	};

	connect(isd, &ImageSelectionDialog::sendItem, this, additem);
}

void StarAlignmentDialog::resetDialog() {

	m_sa = StarAlignment();
	m_sd_gb->reset();
	m_img_list->clear();
	m_imgs.clear();
}

void StarAlignmentDialog::apply() {

	m_sa.setImages(m_imgs);
	m_sa.apply();
}