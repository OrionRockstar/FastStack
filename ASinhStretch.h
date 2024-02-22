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










class ASinhStretchDialog : public QDialog {
	Q_OBJECT

	QString m_name = "ASinhStretch";

	ASinhStretch ash;
	QMdiArea* m_workspace;

	Toolbar* m_tb;

	QLabel* m_sf_label;
	QSlider* m_sf_slider;
	QLineEdit* m_sf_le;
	QDoubleValidator* m_sfdv;

	QLabel* m_bp_label;
	QSlider* m_bp_slider;
	QLineEdit* m_bp_le;
	QDoubleValidator* m_bpdv;

	float m_current_bp = 0;
	QSlider* m_fine_tune;

	QCheckBox* m_rgb_cb;

	QPushButton* m_bp_comp;

public:
	ASinhStretchDialog(QWidget* parent = nullptr);

signals:
	void onClose();

private:
	//validate line edits when loosing focus nad repos slider
	//add preview window, only one preview exists for all images/ process instances
	//make it qdialog and make it child of image window//only current sub window
	void closeEvent(QCloseEvent* close) {
		onClose();
		close->accept();
	}

	void keyPressEvent(QKeyEvent* event) {
		//stretch factor
		QString str = m_sf_le->text();
		double val = m_sf_le->text().toDouble();
		int pos = 0;

		if (m_sfdv->validate(str, pos) == QValidator::Intermediate) {
			if (val > m_sfdv->top())
				val = m_sfdv->top();
			else if (val < m_sfdv->bottom())
				val = m_sfdv->bottom();
		}

		str.setNum(val, 10, 2);
		m_sf_le->setText(str);

		if ((m_sf_le->text())[3] == '.')
			m_sf_le->setText(m_sf_le->text().removeLast());

		m_sf_le->returnPressed();

		//black_point
		str = m_bp_le->text();
		pos = 0;
		val = m_bp_le->text().toDouble();


		if (m_bpdv->validate(str, pos) == QValidator::Intermediate) {
			if (val > m_bpdv->top())
				val = m_bpdv->top();
			else if (val < m_bpdv->bottom())
				val = m_bpdv->bottom();
		}

		str.setNum(val, 10, 6);
		m_bp_le->setText(str);

		m_bp_le->returnPressed();
	}

//private slots:

	void actionSlider_sf(int action);

	void repositionSlider_sf();

	void sliderMoved_sf(int value);


	void actionSlider_bp(int action);

	void repositionSlider_bp();

	void sliderMoved_bp(int value);

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