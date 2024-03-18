#pragma once
#include "Image.h"
#include "ImageWindow.h"
#include "Toolbar.h"

class ASinhStretch {
	float m_stretch_factor = 1.0;
	float m_blackpoint = 0.0;
	bool m_srbg = true;

	float ComputeBeta();

	template<typename T>
	void ApplyMono(Image<T>& img);

	template<typename T>
	void ApplyRGB(Image<T>& img);

public:
	ASinhStretch() = default;

	float StretchFactor()const;

	void setStretchFactor(float stretch_factor);

	float Blackpoint()const;

	void setBlackpoint(float blackpoint);

	void setsRGB(bool srgb) { m_srbg = srgb; }

	template<typename Image>
	void ComputeBlackpoint(Image& img);

	template<typename Image>
	void Apply(Image& img);
};










class ASinhStretchDialog : public ProcessDialog {
	Q_OBJECT

	ASinhStretch ash;

	QLabel* m_sf_label;
	QSlider* m_sf_slider;
	DoubleLineEdit* m_sf_le;

	QLabel* m_bp_label;
	QSlider* m_bp_slider;
	DoubleLineEdit* m_bp_le;

	float m_current_bp = 0;
	QSlider* m_fine_tune;

	QCheckBox* m_rgb_cb;

	QPushButton* m_bp_comp;

public:
	ASinhStretchDialog(QWidget* parent = nullptr);

/*signals:
	void onClose();

private:

	void closeEvent(QCloseEvent* close) {
		for (auto sw : m_workspace->subWindowList()) {
			auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget())->rtp;
			QString str = ptr->windowTitle();
			str.remove(m_name);
			ptr->setWindowTitle(str);
		}
		onClose();
		close->accept();
	}*/

//private slots:
private:
	void actionSlider_sf(int action);

	void repositionSlider_sf();

	void sliderMoved_sf(int value);


	void actionSlider_bp(int action);

	void repositionSlider_bp();

	void sliderMoved_bp(int value);


	void actionSlider_ft(int action);

	void sliderPressed_ft();

	void sliderMoved_ft(int value);

	void sliderReleased_ft();

private:
	void AddStretchFactorInputs();

	void AddBlackpointInputs();

	void AddFinetuneInputs();

	void computeBlackpoint();

	void resetDialog();

	void showPreview();

	void Apply();

	void ApplytoPreview();

};