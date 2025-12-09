#pragma once
#include "FITS.h"
#include "Maths.h"
#include "Star.h"
#include "ImageFileReader.h"


class ImageStackingSignal : public QObject {
	Q_OBJECT
public:
	ImageStackingSignal(QObject* parent = nullptr) {}
	~ImageStackingSignal() {}

public slots:

signals:
	void emitText(const QString& txt);

	void emitPSFData(uint16_t size, const PSF& psf);

	void emitMatrix(const Matrix& m);

	void emitProgress(int progress);
};


class ImageStacking {

public:
	enum class Integration {
		average,
		median,
		min,
		max
	};

	enum class Normalization {
		none,
		additive,
		multiplicative,
		additive_scaling,
		multiplicative_scaling,
	};

	enum class Rejection {
		none,
		sigma_clip,
		winsorized_sigma_clip,
		percintile_clip
	};

protected:
	typedef std::vector<float> Pixelstack;

	struct PixelRows {

	protected:
		ImageStacking* m_is;

	private:
		Pixelstack m_pixels;
		int m_width = 0; //size of row
		int m_num_imgs = 0; //number of rows/images
		int m_size = 0;

		int m_max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

	public:
		PixelRows(int num_imgs, int width, ImageStacking& is);

		float& operator() (int x, int img_num) { return m_pixels[img_num * m_width + x]; }

		int count()const { return m_num_imgs; }

		void fill(const ImagePoint& start_point);

		void fillPixelStack(std::vector<float>& pixelstack, int x, int ch);
	};

	ImageStackingSignal m_iss;

	std::vector<std::unique_ptr<ImageFile>> m_imgfile_vector;

	FileVector m_file_paths;


	std::vector<std::array<float, 3>> m_le; //location estimator
	std::vector<std::array<float, 3>> m_sf; //scale factors

	Integration m_integration = Integration::average;

	Normalization m_normalization = Normalization::none;

	Rejection m_rejection = Rejection::none;

	float m_sigma_low = 4.0;
	float m_sigma_high = 3.0;

	float m_perc_low = 0.1f;
	float m_perc_high = 0.9f;

public:
	ImageStacking() = default;

	ImageStacking(const ImageStacking& other) {
	
		m_file_paths = other.m_file_paths;

		m_le = other.m_le;
		m_sf = other.m_sf;

		m_integration = other.m_integration;
		m_normalization = other.m_normalization;
		m_rejection = other.m_rejection;

		m_sigma_low = other.m_sigma_low;
		m_sigma_high = other.m_sigma_high;

		m_perc_low = other.m_perc_low;
		m_perc_high = other.m_perc_high;
	}

	ImageStackingSignal* imageStackingSignal() { return &m_iss; }

	const ImageStackingSignal* imageStackingSignal()const { return &m_iss; }

	void setRejectionMethod(Rejection method) { m_rejection = method; }

	Normalization normalization()const { return m_normalization; }

	void setNormalation(Normalization normalization) { m_normalization = normalization; }

	Integration integrationMethod()const { return m_integration; }

	void setIntegrationMethod(Integration method) { m_integration = method; }

	float sigmaLow()const { return m_sigma_low; }

	void setSigmaLow(float sigma_low) { m_sigma_low = sigma_low; }

	float sigmaHigh()const { return m_sigma_high; }

	void setSigmaHigh(float sigma_high) { m_sigma_high = sigma_high; }

private:
	float mean(const std::vector<float>& pixelstack);

	float standardDeviation(const std::vector<float>& pixelstack);

	float median(std::vector<float>& pixelstack);

	float min(const std::vector<float>& pixelstack);

	float max(const std::vector<float>& pixelstack);

	void sigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void winsorizedSigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void percintileClipping(std::vector<float>& pixelstack, float p_low, float p_high);

	//new method for weight map
	void pixelRejection(std::vector<float>& pixelstack);

	float pixelIntegration(std::vector<float>& pixelstack);

protected:
	void computeScaleEstimators();

	void openFiles();

	bool isFilesSameDimenisions();

	void closeFiles();

public:

	Status stackImages(const FileVector& paths, Image32& output);

};


//migrate to be apart of imagestacking?!
class ImageStackingWeightMap : public ImageStacking {

	struct Pixel_t {
		float value = 0;
		uint32_t img_num = 0;

		bool operator()(Pixel_t& a, Pixel_t& b) { return (a.value < b.value); }
	};

	typedef std::vector<Pixel_t> Pixelstack_t;

	std::vector<Image8> m_weight_maps;

	struct PixelRows_t: public PixelRows {
		PixelRows_t(int num_imgs, int cols, ImageStackingWeightMap& iswm) : PixelRows(num_imgs, cols, iswm) {}

		void fillPixelStack(Pixelstack_t& pixelstack, int x, int ch);
	};

	ImageStackingSignal* m_issp;

public:
	//ImageStackingWeightMap() : ImageStacking() {};

	ImageStackingWeightMap(const ImageStacking& is) : ImageStacking(is) {
		m_issp = const_cast<ImageStackingSignal*>(is.imageStackingSignal());
	}

private:
	float mean(const Pixelstack_t& pixelstack);

	float standardDeviation(const Pixelstack_t& pixelstack);

	float median(Pixelstack_t& pixelstack);

	float min(const Pixelstack_t& pixelstack);

	float max(const Pixelstack_t& pixelstack);


	void sigmaClip(const ImagePoint& point, Pixelstack_t& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void winsorizedSigmaClip(const ImagePoint& point, Pixelstack_t& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void pixelRejection(const ImagePoint& point, Pixelstack_t& pixelstack);

	float pixelIntegration(Pixelstack_t& pixelstack);

	void writeWeightMaps(std::filesystem::path parent_directory);

public:
	Status stackImages(const FileVector& paths, Image32& output, std::filesystem::path wm_parent_directory);
};

