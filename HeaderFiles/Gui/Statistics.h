#pragma once
#include "Image.h"
#include "Histogram.h"
#include "CustomWidgets.h"
#include <QtDataVisualization/Q3DSurface>
#include <QtDataVisualization/Q3DScatter>
#include "HistogramView.h"
#include "StarDetector.h"

class PSFTable : public QTableWidget {
	Q_OBJECT
private:
	static const int m_columns = 13;

public:
	PSFTable(QWidget* parent = nullptr);

signals:
	void psfSelected(const PSF* psf);

public:
	void selectPSF(const PSF* psf);

	void addRow(const PSF& psf);

	bool event(QEvent* e);

	void mousePressEvent(QMouseEvent* e);

	void mouseReleaseEvent(QMouseEvent* e);
};





class PSFUtilityDialog : public Dialog {
	Q_OBJECT

private:
	StarDetector m_sd;
	const Image8* m_img;

	QLabel* m_stars_detected = nullptr;
	QLabel* m_average_FWHM = nullptr;

	PSFVector m_psf_vector;
	PSFTable* m_psf_table = nullptr;
	const int columns = 13;

	DoubleInput* m_sigmaK_input = nullptr;
	DoubleInput* m_roundness_input = nullptr;
	ComboBox* m_psf_combo = nullptr;
	DoubleSpinBox* m_beta_sb = nullptr;

	PushButton* m_run_psf_pb = nullptr;
	PushButton* m_clear_pb = nullptr; //clears psf_list/table
	PushButton* m_reset_pb = nullptr; //resets star_detector and respective controls

public:
	PSFUtilityDialog(QWidget* parent = nullptr);

	template<typename T>
	PSFUtilityDialog(const Image<T>& img, const QString& name, QWidget* parent = nullptr);

signals:
	void onDetection(PSFVector* psf_detector);

	void psfSelected(const PSF* psf);

	void psfCleared();

public:
	void selectPSF(const PSF* psf);

private:
	void addStarDetctionInputs();

	void addButtons();

	void runPSFDetection();

	void closeEvent(QCloseEvent* e);
};



struct Statistics {

	enum class BitDepth {
		_float = 1,
		_8bit = 255,
		_10bit = 1023,
		_12bit = 4095,
		_14bit = 16383,
		_16bit = 65535
	};

	ImageType type;

	uint32_t pixel_count;
	float mean = 0.0;
	float median = 0;
	float stdDev = 0.0;
	float avgDev = 0.0;
	float MAD = 0;
	//float sqrtBWMV = 0.0;
	float min = 0;
	float max = 1;

	typedef std::vector<Statistics> StatsVector;

private:
	Statistics& operator*=(BitDepth bitdepth) {
		mean *= int(bitdepth);
		median *= int(bitdepth);
		stdDev *= int(bitdepth);
		avgDev *= int(bitdepth);
		MAD *= int(bitdepth);
		min *= int(bitdepth);
		max *= int(bitdepth);

		return *this;
	}

public:
	template<typename T>
	static StatsVector computeStatistics(const Image<T>& img, bool clip = false);

private:
	template<typename T>
	void normalizedFromType(Statistics& s)const {

		s.mean = mean / Pixel<T>::max();
		s.median = Pixel<float>::toType(T(median));
		s.stdDev = stdDev / Pixel<T>::max();
		s.avgDev = avgDev / Pixel<T>::max();
		s.MAD = Pixel<float>::toType(T(MAD));
		s.min = Pixel<float>::toType(T(min));
		s.max = Pixel<float>::toType(T(max));
	}

public:
	Statistics toBitDepth(BitDepth bitdepth)const;
};





class StatisticsDialog : public Dialog {
	Q_OBJECT

	class Label : public QLabel {

	public:
		Label(const QString& txt, QWidget* parent = nullptr) : QLabel(txt, parent) {
			this->setAlignment(Qt::AlignCenter);
		}
	};

	const Statistics::StatsVector* m_stats_vector = nullptr;

	ComboBox* m_bit_depth_combo = nullptr;

	CheckBox* m_clip_cb;

	QTableWidget* m_stats_table;
	
	const QStringList m_stat_labels = { "Px Count","Mean","Median","StdDev","AvgDev","MAD","Minimum","Maximum","","" };

public:
	StatisticsDialog(const QString& img_name, const Statistics::StatsVector& statsvector, int precision, QWidget* parent);

	void updateStats(const Statistics::StatsVector& statsvector);

	bool isChecked()const { return m_clip_cb->isChecked(); }
signals:
	void clipped(bool clip);

private:
	void addStatsTable();

	void addBitDepthCombo();
};







class HistogramDialog : public Dialog {
	Q_OBJECT

private:
	const Image8* m_img = nullptr;
	HistogramView* m_hist_view = nullptr;
	CheckBox* m_clip_cb = nullptr;
	ComboBox* m_resolution_combo = nullptr;
	ComboBox* m_gstyle_combo = nullptr;
	QLabel* m_hist_data = nullptr;

public:
	template<typename T>
	HistogramDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

	HistogramView* histogramView()const { return m_hist_view; }

private:
	void addResolutionCombo();

	void addGraphStyleCombo();

	void resizeEvent(QResizeEvent* e);
};






class SideWidget : public QWidget {
	Q_OBJECT

	enum class State : uint8_t {
		expanded,
		collapsed
	};
	
	//Dialog* m_dialog;
	State m_state = State::collapsed;
	const int m_collapsed_width = 25;
	const int m_expanded_width = 150;

	PushButton* m_resize_pb = nullptr;

public:
	SideWidget(QWidget* parent);

signals:
	void sizeChanged();

private:
	void onPress();
};


class Image3DDialog : public QWidget {
	Q_OBJECT

	Q3DSurface* m_graph = nullptr;
	QWidget* m_container = nullptr;
	
	QLinearGradient m_gradient = defaultGradient();
	QSurfaceDataProxy* m_img_proxy = nullptr;
	QSurface3DSeries* m_img_series = nullptr;
	int m_width = 0;

	SideWidget* m_side_widget = nullptr;

	const float darkRedPos = 1.0f;
	const float redPos = 0.8f;
	const float yellowPos = 0.6f;
	const float greenPos = 0.4f;
	const float darkGreenPos = 0.2f;

public:
	template<typename T>
	Image3DDialog(const QString& img_name, const Image<T>& img, QWidget* parent = nullptr);

signals:
	void windowClosed();

private:
	QLinearGradient defaultGradient();

	template<typename T>
	int computeBinFactor(const Image<T>& img);

	template<typename T>
	Image<T> binImage(const Image<T>& img);

	template<typename T>
	float lumValue(const Image<T>& img, const Point& p);

	template<typename T>
	void fillImageProxy(const Image<T>& img);

};