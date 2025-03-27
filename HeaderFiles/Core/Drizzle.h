#pragma once
#include "Image.h"
#include "Interpolator.h"
#include "Maths.h"
#include "ImageFile.h"

//#include "ImageStacking.h"
#include "ImageCalibration.h"


class WeightMapImage : public ImageFile {
#pragma pack(push, 1)
	struct Header {
		const char signature[3] = { 'W','M','I' };
		uint32_t file_size = 0; //bytes
		const uint16_t data_offset = 30;
		uint32_t rows = 0;
		uint32_t cols = 0;
		uint16_t channels = 0;
		uint32_t pixels_per_channel = 0;
		uint32_t total_pixels = 0;
		const int16_t bitdepth = 8;
		uint8_t compression = 1;
	};
#pragma pack(pop)

	struct RLE {
		uint8_t count = 0;
		uint8_t value = 0;
	};

	bool m_compression = true;

	void writeHeader(const Image8& src, bool compression = true);

public:

	WeightMapImage() : ImageFile(Type::WMI) {}
	//bool isWeightMapImage();

	void open(std::filesystem::path path)override;

	void create(std::filesystem::path path)override;

	void read(Image8& dst);

	void write(const Image8& src, bool compression = true);

};





class Drizzle {

	float m_drop = 0.9;
	uint8_t m_scale_factor = 2;

	float m_offset = (1 - m_drop) / 2;
	float m_new_drop = m_scale_factor * m_drop;
	float m_new_drop_area = m_new_drop * m_new_drop;
	float s2 = m_drop * m_drop;

	//float m_out_weight = 1.0f;
	bool m_initial = true;

public:
	Drizzle() = default;

	float dropSize()const { return m_drop; }
	
	void setDropSize(float drop) { 
		m_drop = drop; 
	    m_offset = (1 - m_drop) / 2;
		m_new_drop = m_scale_factor * m_drop;
		m_new_drop_area = m_new_drop * m_new_drop;
		s2 = m_drop * m_drop;
	}

	uint8_t scaleFactor()const { return m_scale_factor; }

	void setScaleFactor(uint8_t scale_factor) {
		m_scale_factor = scale_factor; 
		m_offset = m_scale_factor * (1 - m_drop) / 2;
		m_new_drop = m_scale_factor * m_drop;
		m_new_drop_area = m_new_drop * m_new_drop;
	}

	//void setOutputWeight(float weight) { m_out_weight = weight; }
	void setIsInitialFrame(bool v) { m_initial = v; }

private:
	float addPixel(float inp, float pix_weight, float out, float area);

	void drizzlePixel(float source_pix, const DoubleImagePoint& dst_pt, Image32& output, float _pix_weight = 1.0f);

public:
	void drizzleFrame(const Image32& src, const Matrix& homography, Image32& output);

	void drizzleFrame(const Image32& src, const Image8& weight_map, const Matrix& homography, Image32& output);
};






class DrizzleIntegrationProcesss {

	ImageStackingSignal m_iss;

	Drizzle m_drizzle;
	ImageCalibrator m_calibrator;

	FileVector m_light_paths;
	FileVector m_alignment_paths;
	FileVector m_weight_paths;

	std::filesystem::path m_dark_path;
	std::filesystem::path m_flat_path;

public:
	DrizzleIntegrationProcesss() = default;

	Drizzle& drizzle() { return m_drizzle; }

	ImageCalibrator& imageCalibrator() { return m_calibrator; }

	const ImageStackingSignal* imageStackingSignal()const { return &m_iss; }

	void setLightPaths(const FileVector& light_paths) { m_light_paths = light_paths; }

	void setAlignmentPaths(const FileVector& alignment_paths) { m_alignment_paths = alignment_paths; }

	void setWeightPaths(const FileVector& weight_paths) { m_weight_paths = weight_paths; }

	Status drizzleImages(Image32& output);
};