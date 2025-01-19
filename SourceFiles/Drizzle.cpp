#include "pch.h"
#include "Drizzle.h"
#include "ImageGeometry.h"
#include "FITS.h"
#include "ImageIntegrationProcess.h"
//#include ""

void WeightMapImage::writeHeader(const Image8& src, bool compression) {

	Header h;

	h.rows = src.rows();
	h.cols = src.cols();
	h.channels = src.channels();
	h.pixels_per_channel = src.pxCount();
	h.total_pixels = src.totalPxCount();
	h.compression = compression;

	m_stream.write((char*)&h, sizeof(h));
}


/*bool WeightMapImage::isWeightMapImage() {
	m_stream.seekg(0);
	char sig[3];
	m_stream.read(sig, 3);
	m_stream.seekg(0);
	//for (int i = 0; i < 3; ++i)
		//if (sig[i] != m_header.signature[i])
			//return false;

	return true;
}*/


void WeightMapImage::open(std::filesystem::path path) {

	ImageFile::open(path);

	Header h;
	m_stream.read((char*)&h, sizeof(h));

	m_rows = h.rows;
	m_cols = h.cols;
	m_channels = h.channels;
	m_px_count = h.pixels_per_channel;

	m_compression = h.compression;
}

void WeightMapImage::create(std::filesystem::path path) {

	path += ".wmi";

	ImageFile::create(path);
}

void WeightMapImage::read(Image8& dst) {

	dst = Image8(rows(), cols(), channels());

	if (!m_compression) {
		m_stream.read((char*)dst.data.get(), dst.totalPxCount());
		return close();
	}

	for (int x = 0, y = 0, ch = 0;;) {
		RLE run;

		m_stream.read((char*)&run, 2);

		//end of line
		if (run.count == 0 && run.value == 0)
			x = 0, y++;

		//end of file
		else if (run.count == 0 && run.value == 1)
			break;

		if (y == dst.rows())
			x = 0, y = 0, ch++;
		
		for (int i = 0; i < run.count; ++i)
				dst(x++, y, ch) = run.value;
	}

	close();
}

void WeightMapImage::write(const Image8& src, bool compression) {

	writeHeader(src, compression);

	if (!compression) {
		m_stream.write((char*)src.data.get(), src.totalPxCount());
		RLE eof(0, 1);
		m_stream.write((char*)&eof, 2);
		return close();
	}

	uint32_t file_size = 0;

	std::vector<RLE> compress;
	compress.reserve(src.cols());
	RLE eol = RLE(0, 0); // end of line
	RLE eof = RLE(0, 1); // end of file

	for (int ch = 0; ch < src.channels(); ++ch) {
		for (int y = 0; y < src.rows(); ++y) {

			compress.emplace_back(RLE(1, src(0, y)));

			for (int x = 1, xb = 0; x < src.cols(); ++x) {

				uint8_t val = src(x, y);

				if (src(x, y) == compress[xb].value) {
					if (compress[xb].count == 255)
						goto newcount;
					else
						compress[xb].count++;
				}

				else {
				newcount:
					compress.emplace_back(RLE(1, src(x, y)));
					xb++;
				}

			}

			m_stream.write((char*)compress.data(), compress.size() * sizeof(RLE));
			m_stream.write((char*)&eol, sizeof(RLE));
			file_size += (compress.size() * sizeof(RLE));
			compress.clear();
		}
	}

	m_stream.write((char*)&eof, 2);

	m_stream.seekp(3);
	m_stream.write((char*)&file_size, 4);

	close();
}





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

				drizzlePixel(src(x, y, ch), { x_o,y_o,ch }, output, Pixel<float>::toType(weight_map.at(x_wm, y_wm, ch)));
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