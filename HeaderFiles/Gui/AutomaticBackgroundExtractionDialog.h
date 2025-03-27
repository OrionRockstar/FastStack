#pragma once

#include "AutomaticBackgroundExtraction.h"
#include "ProcessDialog.h"
#include "ImageWindow.h"


class AutomaticBackgroundExtractionDialog : public ProcessDialog {
    Q_OBJECT

    AutomaticBackgroundExtraction m_abe;

    IntLineEdit* m_sample_radius_le = nullptr;
    Slider* m_sample_radius_slider = nullptr;
    IntLineEdit* m_sample_seperation_le = nullptr;
    Slider* m_sample_seperation_slider = nullptr;

    DoubleLineEdit* m_sigma_low_le = nullptr;
    Slider* m_sigma_low_slider = nullptr;
    DoubleLineEdit* m_sigma_high_le = nullptr;
    Slider* m_sigma_high_slider = nullptr;

    SpinBox* m_poly_degree_sb = nullptr;

    ComboBox* m_correction_combo = nullptr;

    PushButton* m_apply_to_preview_pb = nullptr;
    const QString m_apply_to_preview = "Apply to Preview";

public:

    AutomaticBackgroundExtractionDialog(QWidget* parent = nullptr);

private:

    void addSampleGeneration();

    void addSampleRejection();

    void addOther();

    void resetDialog();

    void showPreview();

    void apply();

    void applytoPreview();
};