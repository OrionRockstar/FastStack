#include "pch.h"
#include "Drizzle.h"
#include "ImageGeometry.h"
#include "ImageFile.h"
#include "FITS.h"
#include "ImageIntegrationProcess.h"


float Drizzle::addPixel(float inp, float pix_weight, float out, float area) {

	if (m_initial)
		return inp * pix_weight * s2;

	float dw = area * pix_weight + 1;

	return (dw != 0.0f) ? ((inp * area * pix_weight * s2) + (out * 1)) / dw : out;
}

void Drizzle::drizzlePixel(float source_pix, const DoubleImagePoint& dst_pt, Image32& output, float pix_weight) {

	double sx = dst_pt.x() * m_scale_factor;
	double sy = dst_pt.y() * m_scale_factor;

	int x_f = (int)floor(sx);
	int y_f = (int)floor(sy);

	if (x_f < 0 || output.cols() - m_scale_factor <= x_f || y_f < 0 || output.rows() - m_scale_factor <= y_f) return;

	float vx = (1 - (sx - x_f));
	float vy = (1 - (sy - y_f));

	int limity = (1 - vy) + m_new_drop;
	if (vy > m_new_drop) {
		limity = 0;
		vy = m_new_drop;
	}

	int limitx = (1 - vx) + m_new_drop;
	if (vx > m_new_drop) {
		limitx = 0;
		vx = m_new_drop;
	}

	float lx, ly;

	for (int j = 0, el = 0; j <= limity; ++j) {
		if (j == 0)
			ly = vy;
		else if (j == limity)
			ly = (m_new_drop - vy - (j - 1));
		else ly = 1;

		for (int i = 0; i <= limitx; ++i) {
			if (i == 0)
				lx = vx;
			else if (i == limitx)
				lx = (m_new_drop - vx - (i - 1));
			else lx = 1;

			output(x_f + i, y_f + j, dst_pt.channel()) = addPixel(source_pix, pix_weight, output(x_f + i, y_f + j, dst_pt.channel()), lx * ly / m_new_drop_area);
		}
	}
}

void Drizzle::drizzleFrame(const Image32& input, const Matrix& homography, Image32& output) {

	Matrix drizzle_homography = homography.inverse();

	for (uint32_t ch = 0; ch < input.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < input.rows(); ++y) {

			double yx = y * drizzle_homography(0, 1);
			double yy = y * drizzle_homography(1, 1);

			for (int x = 0; x < input.cols(); ++x) {
				double x_s = x * drizzle_homography(0, 0) + yx + drizzle_homography(0, 2) + m_offset;
				double y_s = x * drizzle_homography(1, 0) + yy + drizzle_homography(1, 2) + m_offset;

				drizzlePixel(input(x,y,ch), { x_s,y_s,ch }, output);
			}
		}
	}
}

void Drizzle::drizzleFrame(const Image32& src, const Image8& weight_map, const Matrix& homography, Image32& output) {

	Matrix drizzle_homography = homography.inverse();

	for (uint32_t ch = 0; ch < src.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < src.rows(); ++y) {

			double dyx = y * drizzle_homography(0, 1);
			double dyy = y * drizzle_homography(1, 1);

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);

			for (int x = 0; x < src.cols(); ++x) {
				double x_o = x * drizzle_homography(0, 0) + dyx + drizzle_homography(0, 2) + m_offset;
				double y_o = x * drizzle_homography(1, 0) + dyy + drizzle_homography(1, 2) + m_offset;

				double x_wm = x * homography(0, 0) + yx + homography(0, 2);
				double y_wm = x * homography(1, 0) + yy + homography(1, 2);

				drizzlePixel(src(x, y, ch), { x_o,y_o,ch }, output, Pixel<float>::toType(weight_map.at(x_o, y_o, ch)));
			}
		}
	}
}





Status DrizzleIntegrationProcesss::drizzleImages(Image32& output) {

	if (m_alignment_paths.size() == 0)
		return {false, "Insufficient alignment data available!"};

	std::vector<Matrix> homographies;
	for (auto file : m_alignment_paths)
		homographies.push_back(alignmentDataReader(file));

	ImageCalibrator calibrator = m_calibrator;

	calibrator.setMasterDarkPath(m_dark_path);
	calibrator.setMasterFlatPath(m_flat_path);

	m_iss.emitText("Drizzling " + QString::number(m_light_paths.size()) + " Images...");
	m_iss.emitText("Drop size: " + QString::number(m_drizzle.dropSize()));
	m_iss.emitText("Scale Factor: " + QString::number(m_drizzle.scaleFactor()));

	for (int i = 0; i < m_light_paths.size(); ++i) {

		m_drizzle.setIsInitialFrame(i == 0);

		//m_iss.emitText(m_light_paths[i].string().c_str());

		Image32 src;
		FITS fits;
		fits.open(m_light_paths[i]);
		fits.readAny(src);
		calibrator.calibrateImage(src);

		//need to scale/normalize images before drizzle

		if (i == 0)
			output = Image32(src.rows() * m_drizzle.scaleFactor(), src.cols() * m_drizzle.scaleFactor(), src.channels());

		if (m_weight_paths.size() == m_light_paths.size()) {
			Image8 wm;
			WeightMapImage wmi;
			wmi.open(m_weight_paths[i]);
			wmi.read(wm);

			if (i == 0) 
				m_drizzle.drizzleFrame(src, wm, Matrix(3, 3).identity(), output);
			
			else
				m_drizzle.drizzleFrame(src, wm, homographies[i - 1], output);
		}

		else {
			if (i == 0) 
				m_drizzle.drizzleFrame(src, Matrix(3, 3).identity(), output);
			
			else
				m_drizzle.drizzleFrame(src, homographies[i - 1], output);
		}

		m_iss.emitProgress(((i + 1) * 100) / m_light_paths.size());
	}

	//output.normalize();

	return Status();
}