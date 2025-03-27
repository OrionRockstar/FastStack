#include "pch.h"
#include "ImageWindowMenu.h"
#include "ImageWindow.h"

MaskSelectionDialog::MaskSelectionDialog(QWidget* image_window, QMdiArea* parent) : m_image_window(image_window), Dialog(parent) {

    this->resizeDialog(400, 140);
    this->setModal(true);

    m_current_mask = new QLineEdit(drawArea());
    m_current_mask->setReadOnly(true);
    m_current_mask->setText("No Mask");
    m_current_mask->setFixedWidth(250);
    m_current_mask->move(115, 15);
    addLabel(m_current_mask, new QLabel("Current Mask:", drawArea()));

    m_new_mask = new ComboBox(drawArea());
    m_new_mask->addItem("No Mask Selected");
    m_new_mask->setFixedWidth(250);
    m_new_mask->move(75, 55);

    auto iw = imageRecast<>(m_image_window);
    for (auto sw : dynamic_cast<QMdiArea*>(parentWidget())->subWindowList()) {
        auto mask = imageRecast<>(sw->widget());
        if (mask->name() != iw->name() && iw->source().isSameSize(mask->source()) && mask->channels() <= iw->channels())
            m_new_mask->addItem(imageRecast<>(sw->widget())->name());
    }

    if (iw->mask() != nullptr) {
        m_current_mask->setText(iw->mask()->name());
        m_new_mask->setCurrentText(iw->mask()->name());
    }

    PushButton* pb = new PushButton("OK!", drawArea());
    pb->move(160, 95);
    connect(pb, &QPushButton::released, this, [this]() { this->apply(); });

    this->setAttribute(Qt::WA_DeleteOnClose);
    this->show();
}

void MaskSelectionDialog::apply() {

    auto ptr = imageRecast<uint8_t>(m_image_window);

    for (auto sw : dynamic_cast<QMdiArea*>(parentWidget())->subWindowList()) {
        auto iw = imageRecast<>(sw->widget());
        if (iw->name() == m_new_mask->currentText()) {
            m_current_mask->setText(iw->name());
            switch (ptr->type()) {
            case ImageType::UBYTE:
                ptr->setMask(iw);
                break;
            case ImageType::USHORT:
                imageRecast<uint16_t>(ptr)->setMask(iw);
                break;
            case ImageType::FLOAT:
                imageRecast<float>(ptr)->setMask(iw);
                break;
            }
            break;
        }
        else if (m_new_mask->currentIndex() == 0) {
            m_current_mask->setText("No Mask");
            ptr->removeMask();
        }
    }

    this->close();
}


ImageWindowMenu::ImageWindowMenu(QWidget& image_window, QWidget* parent) : m_image_window(&image_window), QMenu(parent) {

    addMaskMenu();
    addZoomWindowColorMenu();
}

void ImageWindowMenu::removeMask() {
    ImageWindow8* iw = imageRecast(m_image_window);
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->removeMask();
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->removeMask();
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->removeMask();
    }
}

void ImageWindowMenu::showMask(bool show) {

    ImageWindow8* iw = imageRecast(m_image_window);
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->showMask(show);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->showMask(show);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->showMask(show);
    }
}

void ImageWindowMenu::invertMask(bool invert) {

    ImageWindow8* iw = imageRecast(m_image_window);
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->invertMask(invert);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->invertMask(invert);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->invertMask(invert);
    }
}

void ImageWindowMenu::enableMask(bool enable) {

    ImageWindow8* iw = imageRecast(m_image_window);
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->enableMask(enable);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->enableMask(enable);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->enableMask(enable);
    }
}

void ImageWindowMenu::setMaskColor(const QColor& color) {

    ImageWindow8* iw = imageRecast(m_image_window);
    switch (iw->type()) {
    case ImageType::UBYTE:
        return iw->setMaskColor(color);
    case ImageType::USHORT:
        return imageRecast<uint16_t>(iw)->setMaskColor(color);
    case ImageType::FLOAT:
        return imageRecast<float>(iw)->setMaskColor(color);
    }
}

void ImageWindowMenu::addMaskMenu() {

    auto iw = imageRecast<>(m_image_window);

    m_mask = new QMenu("Mask", this);
    this->addMenu(m_mask);
    m_mask->addAction(tr("Select Mask"), this, &ImageWindowMenu::showMaskSelectionDialog);

    m_mask->addAction(tr("Remove Mask"), this, &ImageWindowMenu::removeMask);

    QWidgetAction* wa = new QWidgetAction(m_mask);
    wa->setText("Enable Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskEnabled());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::enableMask);

    wa = new QWidgetAction(m_mask);
    wa->setText("Show Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskVisible());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::showMask);

    wa = new QWidgetAction(m_mask);
    wa->setText("Invert Mask");
    wa->setCheckable(true);
    wa->setChecked(iw->maskInverted());
    m_mask->addAction(wa);
    connect(wa, &QWidgetAction::toggled, this, &ImageWindowMenu::invertMask);

    addMaskColorSelection();
}

void ImageWindowMenu::showMaskSelectionDialog() {
    MaskSelectionDialog* m_msd = new MaskSelectionDialog(m_image_window, imageRecast<>(m_image_window)->workspace());
}

void ImageWindowMenu::addMaskColorSelection() {

    QMenu* mask_color_menu = new QMenu("Mask Color", this);
    m_mask->addMenu(mask_color_menu);

    auto iw = imageRecast<>(m_image_window);

    int id = std::find(m_colors.begin(), m_colors.end(), iw->maskColor()) - m_colors.begin();

    for (int i = 0; i < m_color_names.size(); ++i) {
        QWidgetAction* wa = new QWidgetAction(m_mask);
        QPixmap pm(18, 18);
        pm.fill(m_colors[i]);
        wa->setIcon(pm);
        wa->setText(m_color_names[i]);
        wa->setCheckable(true);

        if (i == id)
            wa->setChecked(true);

        mask_color_menu->addAction(wa);
        connect(wa, &QWidgetAction::toggled, this, [this, i]() { this->setMaskColor(m_colors[i]); });
    }
}

void ImageWindowMenu::addZoomWindowColorMenu() {

    m_zwc_menu = new QMenu("ZoomWindow Color", this);
    this->addMenu(m_zwc_menu);

    auto iw = imageRecast<>(m_image_window);

    int id = std::find(m_colors.begin(), m_colors.end(), iw->zoomWindowColor()) - m_colors.begin();

    for (int i = 0; i < m_color_names.size(); ++i) {
        QWidgetAction* wa = new QWidgetAction(m_mask);
        QPixmap pm(18, 18);
        pm.fill(m_colors[i]);
        wa->setIcon(pm);
        wa->setText(m_color_names[i]);
        wa->setCheckable(true);

        if (i == id)
            wa->setChecked(true);

        m_zwc_menu->addAction(wa);
        connect(wa, &QWidgetAction::toggled, this, [this, i]() { 
            auto iw = imageRecast<>(m_image_window);
            iw->setZoomWindowColor(m_colors[i]); });
    }

}