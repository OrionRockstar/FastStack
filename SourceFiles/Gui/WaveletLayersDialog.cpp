#include "pch.h"
#include "WaveletLayersDialog.h"
#include "FastStack.h"



WaveletLayersDialog::WaveletLayersDialog(Workspace* parent) : ProcessDialog("WaveletLayers", QSize(345, 100), parent, false, true, false) {

	m_layers_sb = new SpinBox(drawArea());
	m_layers_sb->setRange(1, 6);
	m_layers_sb->setValue(m_wavelet.layers());
	m_layers_sb->move(150, 15);
	addLabel(m_layers_sb, new QLabel("Wavelet layers:", drawArea()));
	connect(m_layers_sb, &QSpinBox::valueChanged, this, [this](int val) { m_wavelet.setLayers(val); });

	m_residual_cb = new CheckBox("Residual", drawArea());
	m_residual_cb->move(225, 17);
	connect(m_residual_cb, &QCheckBox::clicked, this, [this](bool v) { m_wavelet.setResidual(v); });

	m_scaling_func_combo = new ComboBox(drawArea());
	m_scaling_func_combo->addItems({ "3x3 Linear Interpolation", "3x3 Small Scale", "5x5 B3 Spline","5x5 Gaussian" });
	m_scaling_func_combo->setCurrentIndex(int(m_wavelet.scalingFuntion()));
	m_scaling_func_combo->move(140, 55);
	addLabel(m_scaling_func_combo, new QLabel("Scaling Function:", drawArea()));

	connect(m_scaling_func_combo, &QComboBox::activated, this, [this](int index) { m_wavelet.setScalingFuntion(Wavelet::ScalingFunction(index)); });

	this->show();
}

void WaveletLayersDialog::resetDialog() {

	m_wavelet = WaveletLayerCreator();

	m_layers_sb->setValue(m_wavelet.layers());
	m_residual_cb->setChecked(false);
	m_scaling_func_combo->setCurrentIndex(int(m_wavelet.scalingFuntion()));
}

void WaveletLayersDialog::apply() {

	if (m_workspace->subWindowList().size() == 0)
		return;

	auto iwptr = imageRecast<>(m_workspace->currentSubWindow()->widget());

	enableSiblings_Subwindows(false);

	std::vector<Image32> wavelet_vector;

	switch (iwptr->type()) {
	case ImageType::UBYTE: {
		wavelet_vector = m_wavelet.generateWaveletLayers(iwptr->source());
		break;
	}
	case ImageType::USHORT: {
		auto iw16 = imageRecast<uint16_t>(iwptr);
		wavelet_vector = m_wavelet.generateWaveletLayers(iw16->source());
		break;
	}
	case ImageType::FLOAT: {
		auto iw32 = imageRecast<float>(iwptr);
		wavelet_vector = m_wavelet.generateWaveletLayers(iw32->source());
		break;
	}
	}

	for (int i = 0; i < wavelet_vector.size(); ++i) {

		std::string name = "WaveletImage" + std::to_string(i);
		int count = 0;

		for (auto sw : m_workspace->subWindowList()) {
			auto ptr = reinterpret_cast<ImageWindow8*>(sw->widget());
			std::string img_name = ptr->name().toStdString();
			if (name == img_name)
				name += "_" + std::to_string(++count);
		}

		ImageWindow32* iw = new ImageWindow32(std::move(wavelet_vector[i]), QString::fromStdString(name), m_workspace);
	}

	enableSiblings_Subwindows(true);
}