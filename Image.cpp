#include "Image.h"
#include "Interpolation.h"
#include "tiffio.h"
#include "cfitsio/fitsio.h"
#include <array>
#include <fstream>


static float ClipZero(float pixel) {
	return (pixel < 0) ? 0 : pixel;
}

static float ClipOne(float pixel) {
	return (pixel > 1) ? 1 : pixel;
}

static float ClipPixel(float pixel) {
	if (pixel > 1)
		return 1;
	else if (pixel < 0)
		return 0;
	return pixel;
}

void CreateStatsText(const std::filesystem::path& file_path, Image32& img) {

	std::ofstream myfile;
	std::filesystem::path temp = file_path;
	myfile.open(temp.replace_extension(".txt"));
	myfile << file_path << "\n";
	if (img.Channels() == 1) {
		myfile << "Max: " << std::fixed << std::setprecision(7) << img.max << "\n";
		myfile << "Min: " << img.min << "\n";
		myfile << "Median: " << img.median << "\n";
		myfile << "Mean: " << img.mean << "\n";
		myfile << "Standard Deviation: " << img.stdev << "\n";
		myfile << "MAD: " << img.mad << "\n";
		myfile << "AvgDev: " << img.avgDev << "\n";
		myfile << "BWMV: " << img.bwmv << "\n";
	}
	myfile << "Homography: " << img.homography(0, 0) << "," << img.homography(0, 1) << "," << img.homography(0, 2)
		<< "," << img.homography(1, 0) << "," << img.homography(1, 1) << "," << img.homography(1, 2) << ",\n";
	myfile.close();
}

bool ReadStatsText(const std::filesystem::path& file_path, Image32& img) {

	std::vector<std::string> stats = { "std::string::npos","Max: ","Min: ", "Median: ","Mean: ","Standard Deviation: ","MAD: ","AvgDev: ","BWMV: ","Homography: " };
	std::filesystem::path temp = file_path;
	std::string line;
	std::ifstream myfile(temp.replace_extension(".txt"));
	int line_counter = 0;
	if (myfile.is_open()) {

		while (std::getline(myfile, line)) {

			if (line.find(stats[line_counter]) != std::string::npos) {

				size_t start = stats[line_counter].length();
				float val = std::stof(line.substr(start, std::string::npos));

				switch (line_counter) {
				case 1:
					img.max = val;
					break;
				case 2:
					img.min = val;
					break;
				case 3:
					img.median = val;
					break;
				case 4:
					img.mean = val;
					break;
				case 5:
					img.stdev = val;
					break;
				case 6:
					img.mad = val;
					break;
				case 7:
					img.avgDev = val;
					break;
				case 8:
					img.bwmv = val;
					break;
				case 9:

					size_t comma = start;
					int i = 0, j = 0;
					while ((comma = line.find(",", comma)) != std::string::npos) {
						img.homography(i, j) = std::stod(line.substr(start, comma - start));
						start = (comma += 1);
						j += 1;
						if (j == 3) { i++, j = 0; }
					}
					break;
				}
			}

			line_counter++;
		}

		myfile.close();
		return true;
	}
	return false;
}

bool StatsTextExists(const std::filesystem::path& file_path) {
	auto temp = file_path;
	return std::filesystem::exists(temp.replace_extension(".txt"));
}

void GetImageStackFromTemp(FileVector& light_files, ImageVector& img_stack) {

	Image32 img;

	img_stack.reserve(light_files.size());

	for (auto file_it : light_files) {
		FileOP::FitsRead("./temp//" + file_it.stem().string() + "temp.fits", img);
		ReadStatsText(file_it.replace_extension(".txt"), img);
		img_stack.emplace_back(std::move(img));
	}

	std::filesystem::remove_all("./temp");
}

void FileOP::TiffRead(std::filesystem::path file, Image32& img) {

	uint32_t imagelength, imagewidth;
	short bd;
	TIFF* tiff = TIFFOpen(file.string().c_str(), "r");
	if (tiff) {
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &imagelength);
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &imagewidth);
		TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bd);
		switch (bd) {
		case 32: {
			img = Image32(imagelength, imagewidth);

			for (uint32_t row = 0; row < imagelength; ++row)
				TIFFReadScanline(tiff, &img[row * imagewidth], row);

			break; }

		case 16: {
			Image16 temp(imagelength, imagewidth);

			for (uint32_t row = 0; row < imagelength; ++row)
				TIFFReadScanline(tiff, &temp[row * imagewidth], row);

			img = Image32(imagelength, imagewidth);

			for (int el = 0; el < img.Total(); ++el)
				img[el] = float(temp[el]) / 65535;


			break; }

		case 8: {
			Image8 temp(imagelength, imagewidth);

			for (uint32_t row = 0; row < imagelength; ++row)
				TIFFReadScanline(tiff, &temp[row * imagewidth], row);

			img = Image32(imagelength, imagewidth);

			for (int el = 0; el < img.Total(); ++el)
				img[el] = float(temp[el]) / 255;

			break; }
		}
		TIFFClose(tiff);
	}
}

bool FileOP::FitsRead(const std::filesystem::path& file, Image32& img) {

	fitsfile* fptr;
	int status = 0;
	int bitpix, naxis;
	long naxes[3] = { 1,1,1 }, fpixel[3] = { 1,1,1 };//naxes={row,col}

	if (!fits_open_file(&fptr, file.string().c_str(), READONLY, &status))
	{
		if (!fits_get_img_param(fptr, 3, &bitpix, &naxis, naxes, &status))
		{
			if (naxis > 3 || naxis == 0) 0;
				//std::cout << "Error: only 2D images are supported\n";
			else
			{
				img = Image32(naxes[1], naxes[0], naxes[2]);

				fits_read_pix(fptr, TFLOAT, fpixel, naxes[0] * naxes[1] * naxes[2], NULL, img.data.get(), NULL, &status);

				switch (bitpix) {
				case 16: {

					for (float& pix : img)
						pix /= 65535.0f;

					break;
				}

				case 8: {

					for (float& pix : img)
						pix /= 255.0f;

					break;
				}
				case -32: {
					
					break;
				}
				}
			}
		}
		fits_close_file(fptr, &status);
	}

	if (status) { fits_report_error(stderr, status); return false; } // print any error message 

	return true;
}

/*void ImageOP::XISFRead(std::string file, Image32& img) {
	pcl::UInt16Image temp;
	pcl::XISFReader xisf;

	xisf.Open(file.c_str());
	//(int)xisf.ImageOptions().bitsPerSample
	xisf.ReadImage(temp);
	xisf.Close();

	Image32 img(temp.Height(), temp.Width());

	for (int y = 0; y < img.Rows(); ++y)
		for (int x = 0; x < img.Cols(); ++x)
			img(x, y) = (float)temp(pcl::Point(x, y)) / 65535;

}*/

void FileOP::FitsWrite(Image32& img, const std::filesystem::path& file_path, bool overwrite) {
	fitsfile* fptr;
	int status, exists;
	long  fpixel = 1, naxis = 3;// exposure;
	long naxes[3] = { img.Cols(), img.Rows(), img.Channels() };
	std::string name = file_path.string();

	status = 0;
	if (overwrite) {
		fits_file_exists((name + ".fits").c_str(), &exists, &status);

		if (exists == 1) {
			fits_open_file(&fptr, (name + ".fits").c_str(), READWRITE, &status);
			fits_delete_file(fptr, &status);
		}
	}
	fits_create_file(&fptr, (name + ".fits").c_str(), &status);

	fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status);

	fits_write_img(fptr, TFLOAT, fpixel, img.Total() * img.Channels(), img.data.get(), &status);

	fits_close_file(fptr, &status);

	fits_report_error(stderr, status);
	//return(status);
}

void ImageOP::AlignFrame(Image32& img, Eigen::Matrix3d homography, std::function<float(Image32&, double& x_s, double& y_s, int& ch)> interp_type) {
	
	Image32 temp(img.Rows(), img.Cols());
	int ch = 0;

	temp.homography = img.homography;

#pragma omp parallel for
	for (int y = 0; y < img.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < img.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2);
			double y_s = x * homography(1, 0) + yy + homography(1, 2);

			temp(x, y) = ClipPixel(interp_type(img, x_s, y_s, ch));

		}
	}

	temp.ComputeStats();
	img = std::move(temp);
}

static float DrizzlePix(float& inp, float& out, float& area, const float& s2, int& pix_weight, int& out_weight) {
	float dw = area * pix_weight + out_weight;
	return (dw != 0) ? ((inp * area * pix_weight) + (out * out_weight)) / dw : out;
}

static void DrizzleFrame(Image32& input, Image32& output, float drop) {

	Eigen::Matrix3d homography = Eigen::Inverse(input.homography);
	int oweight = (homography == Eigen::Matrix3d::Identity()) ? 0 : 1;

	int pix_weight = 1;

	float s2 = drop * drop,
		offset = (1 - drop) / 2,
		x2drop = 2 * drop,
		drop_area = x2drop * x2drop;

#pragma omp parallel for
	for (int y = 0; y < input.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < input.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2) + offset;
			double y_s = x * homography(1, 0) + yy + homography(1, 2) + offset;

			int x_f = (int)floor(2 * x_s);
			int y_f = (int)floor(2 * y_s);

			if (x_f < 0 || x_f >= output.Cols() - 2 || y_f < 0 || y_f >= output.Rows() - 2)
				continue;

			float vx = (1 - ((2 * (x_s)) - x_f));
			float vy = (1 - ((2 * (y_s)) - y_f));

			std::array<float, 9> area = { 0 };

			if (x2drop >= vx && x2drop >= vy)
				area[0] = (vx * vy) / drop_area;
			else if (x2drop < vx && x2drop > vy)
				area[0] = ((x2drop)*vy) / drop_area;
			else if (x2drop > vx && x2drop < vy)
				area[0] = (vx * (x2drop)) / drop_area;
			else
				area[0] = ((x2drop) * (x2drop)) / drop_area;

			if (x2drop >= vx + 1 && x2drop > vy)
				area[1] = vy / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop>vy)
				area[1] = (vy * (x2drop - vx)) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop < vy)
				area[1] = (x2drop * (x2drop - vx)) / drop_area;

			if (x2drop > vx + 1)
				area[2] = (vy * (x2drop - vx - 1)) / drop_area;

			if (x2drop >= vy + 1)
				area[3] = vx / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx)
				area[3] = (vx * (x2drop - vy)) / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop < vx)
				area[3] = ((x2drop) * (x2drop - vy)) / drop_area;

			if (x2drop >= vx + 1 && x2drop >= vy + 1)
				area[4] = 1 / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx + 1)
				area[4] = (x2drop - vy) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop >= vy + 1)
				area[4] = (x2drop - vx) / drop_area;
			else if ((x2drop < vx + 1 && x2drop >= vx) && (x2drop < vy + 1 && x2drop >= vy))
				area[4] = ((x2drop - vx) * (x2drop - vy)) / drop_area;

			if (x2drop > vx + 1 && x2drop > vy + 1)
				area[5] = (x2drop - vx - 1) / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop > vx + 1)
				area[5] = ((x2drop - vy) * (x2drop - vx - 1)) / drop_area;

			if (x2drop > vy + 1)
				area[6] = (vx * (x2drop - vy - 1)) / drop_area;

			if (x2drop > vx + 1 && x2drop > vy + 1)
				area[7] = (x2drop - vy - 1) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop > vy + 1)
				area[7] = ((x2drop - vy - 1) * (x2drop - vx)) / drop_area;

			if (x2drop > vy + 1 && x2drop > vx + 1)
				area[8] = ((x2drop - vy - 1) * (x2drop - vx - 1)) / drop_area;

			output(x_f, y_f) = DrizzlePix(input(x, y), output(x_f, y_f), area[0], s2, pix_weight, oweight);
			output(x_f + 1, y_f) = DrizzlePix(input(x, y), output(x_f + 1, y_f), area[1], s2, pix_weight, oweight);
			output(x_f + 2, y_f) = DrizzlePix(input(x, y), output(x_f + 2, y_f), area[2], s2, pix_weight, oweight);
			output(x_f, y_f + 1) = DrizzlePix(input(x, y), output(x_f, y_f + 1), area[3], s2, pix_weight, oweight);
			output(x_f + 1, y_f + 1) = DrizzlePix(input(x, y), output(x_f + 1, y_f + 1), area[4], s2, pix_weight, oweight);
			output(x_f + 2, y_f + 1) = DrizzlePix(input(x, y), output(x_f + 2, y_f + 1), area[5], s2, pix_weight, oweight);
			output(x_f, y_f + 2) = DrizzlePix(input(x, y), output(x_f, y_f + 2), area[6], s2, pix_weight, oweight);
			output(x_f + 1, y_f + 2) = DrizzlePix(input(x, y), output(x_f + 1, y_f + 2), area[7], s2, pix_weight, oweight);
			output(x_f + 2, y_f + 2) = DrizzlePix(input(x, y), output(x_f + 2, y_f + 2), area[8], s2, pix_weight, oweight);

		}
	}

}

void ImageOP::DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_est) {
	ImageVector img_stack;
	GetImageStackFromTemp(light_files, img_stack);
	//ImageOP::ScaleImageStack(img_stack, scale_est);

	output = Image32(img_stack[0].Rows() * 2, img_stack[0].Cols() * 2);

	for (auto& img : img_stack)
		DrizzleFrame(img, output, drop_size);

	output.ComputeStats();
}

void ImageOP::RotateImage(Image32& img, float theta, Interp_func interp_type) {

	theta *= (M_PI / 180);
	float s = sin(theta);
	float c = cos(theta);
	Image32 temp(fabs(img.Rows() * c) + fabs(img.Cols() * s), fabs(img.Cols() * c) + fabs(img.Rows() * s), img.Channels());

	float hc = temp.Cols() / 2;
	float hr = temp.Rows() / 2;

	float offsetx = hc - (temp.Cols() - img.Cols()) / 2;
	float offsety = hr - (temp.Rows() - img.Rows()) / 2;
	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {

			double yx = (y - hr) * s;
			double yy = (y - hr) * c;

			for (int x = 0; x < temp.Cols(); ++x) {

				double x_s = ((x - hc) * c - yx) + offsetx;
				double y_s = ((x - hc) * s + yy) + offsety;

				temp(x, y, ch) = ClipPixel(interp_type(img, x_s, y_s, ch));

			}
		}
	}

	img = std::move(temp);
	img.ComputeStats(true);

}

static void FastRotate_90CW(Image32& img) {
	Image32 temp(img.Cols(), img.Rows());
	temp.CopyStatsFrom(img);

	int hc = temp.Cols() / 2;
	int hr = temp.Rows() / 2;

	int offsetx = hc - (temp.Cols() - img.Cols()) / 2 - hr;
	int offsety = hr - (temp.Rows() - img.Rows()) / 2 + hc - 1;

	for (int y = 0; y < temp.Rows(); ++y) {
		int x_s = y - offsetx;
		for (int x = 0; x < temp.Cols(); ++x) {
			int y_s = offsety - x;

			temp(x, y) = img(x_s, y_s);

		}
	}
	img = std::move(temp);
}

static void FastRotate_90CCW(Image32& img) {
	Image32 temp(img.Cols(), img.Rows());
	temp.CopyStatsFrom(img);

	int hr = temp.Rows() / 2;
	int hc = temp.Cols() / 2;

	int offsetx = hc - (temp.Cols() - img.Cols()) / 2 + hr;
	int offsety = hr - (temp.Rows() - img.Rows()) / 2 - hc;

	for (int y = 0; y < temp.Rows(); ++y) {
		int x_s = offsetx - y;
		for (int x = 0; x < temp.Cols(); ++x) {
			int y_s = x + offsety;

			temp(x, y) = img(x_s, y_s);

		}
	}

	img = std::move(temp);
}

inline static void Swap(float& a, float& b) {
	float c = a; a = b; b = c;
}

static void FastRotate_180(Image32& img) {
	int r = img.Rows() - 1;
	int c = img.Cols() - 1;
	for (int y = 0; y < img.Rows() / 2; ++y) {
		int y_s = r - y;
		for (int x = 0, x_s = c; x < img.Cols(), x_s >= 0; ++x, --x_s) {
			//int x_s = c - x;
			std::swap(img(x, y), img(x_s, y_s));
		}
	}
}

static void FastRotate_HorizontalMirror(Image32& img) {
	int c = img.Cols() - 1;
	int hc = img.Cols() / 2;
	for (int y = 0; y < img.Rows(); ++y) {
		for (int x = 0, x_s = c; x < hc, x_s >= hc; ++x, --x_s) {
			//int x_s = c - x;
			std::swap(img(x, y), img(x_s, y));
		}
	}
}

static void FastRotate_VerticalMirror(Image32& img) {
	int r = img.Rows() - 1;
	for (int y = 0; y < img.Rows() / 2; ++y) {
		int y_s = r - y;
		for (int x = 0; x < img.Cols(); ++x)
			std::swap(img(x, y), img(x, y_s));
	}
}

void ImageOP::FastRotation(Image32& img, FastRotate type) {
	switch (type) {
	case FastRotate::rotate90CW:
		FastRotate_90CW(img);
		break;
	case FastRotate::rotate90CCW:
		FastRotate_90CCW(img);
		break;
	case FastRotate::rotate180:
		FastRotate_180(img);
		break;
	case FastRotate::horizontalmirror:
		FastRotate_HorizontalMirror(img);
		break;
	case FastRotate::verticalmirror:
		FastRotate_VerticalMirror(img);
		break;
		//default:
			//break;
	}
}

void ImageOP::Crop(Image32& img, int top, int bottom, int left, int right) {

	Image32 temp(img.Rows() - (top + bottom), img.Cols() - (left + right), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch)
		for (int y = 0; y < temp.Rows(); ++y)
			for (int x = 0; x < temp.Cols(); ++x)
				temp(x, y, ch) = img(x + left, y + top, ch);

	img = std::move(temp);
	img.ComputeStats();
}

void ImageOP::Resize2x_Bicubic(Image32& img) {
	Image32 temp(img.Rows() * 2, img.Cols() * 2);
	int ch = 0;

#pragma omp parallel for
	for (int y = 0; y < temp.Rows(); ++y) {
		double y_s = 0.5 * y;

		for (int x = 0; x < temp.Cols(); ++x) {
			double x_s = 0.5 * x;

			temp(x, y) = ClipPixel(Interpolation::Bicubic_Spline(img, x_s, y_s, ch));

		}
	}

	img = std::move(temp);
}

void ImageOP::ImageResize_Bicubic(Image32& img, int new_rows, int new_cols) {

	Image32 temp(new_rows, new_cols, img.Channels());

	double ry = double(img.Rows()) / temp.Rows();
	double rx = double(img.Cols()) / temp.Cols();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.Cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = ClipPixel(Interpolation::Bicubic_Spline(img, x_s, y_s, ch));

			}
		}
	}
	img = std::move(temp);
	if (img.Channels() == 3)
		img.AverageRGBStats();
	else
		img.ComputeStats();
}

void ImageOP::Bin2x(Image32& img) {
	Image32 temp(img.Rows() / 2, img.Cols() / 2);

#pragma omp parallel for
	for (int y = 0; y < temp.Rows(); ++y) {
		int y_s = 2 * y;

		for (int x = 0; x < temp.Cols(); ++x) {
			int x_s = 2 * x;

			temp(x, y) = (img(x_s, y_s) + img(x_s + 1, y_s) + img(x_s, y_s + 1) + img(x_s + 1, y_s + 1)) / 4;

		}
	}
	img = std::move(temp);
	img.ComputeStats();
}

inline static float kernelmedian(std::array<float, 9>& kernel) {
	for (int r = 0; r < 3; ++r) {
		if (kernel[0] > kernel[4])
			std::swap(kernel[0], kernel[4]);
		if (kernel[5] < kernel[4])
			std::swap(kernel[5], kernel[4]);

		if (kernel[1] > kernel[4])
			std::swap(kernel[1], kernel[4]);
		if (kernel[6] < kernel[4])
			std::swap(kernel[6], kernel[4]);


		if (kernel[2] > kernel[4])
			std::swap(kernel[2], kernel[4]);
		if (kernel[7] < kernel[4])
			std::swap(kernel[7], kernel[4]);

		if (kernel[3] > kernel[4])
			std::swap(kernel[3], kernel[4]);
		if (kernel[8] < kernel[4])
			std::swap(kernel[8], kernel[4]);
	}
	return kernel[4];
}

void ImageOP::MedianBlur3x3(Image32& img) {
	Image32 imgbuf(img.Rows(), img.Cols());
	std::array<float, 9>kernel = { 0 };

#pragma omp parallel for firstprivate(kernel)
	for (int y = 1; y < img.Rows() - 1; ++y) {
		for (int x = 1; x < img.Cols() - 1; ++x) {

			kernel = { img(x - 1, y - 1), img(x, y - 1), img(x + 1, y - 1),
					   img(x - 1, y), img(x, y), img(x + 1, y),
					   img(x - 1, y + 1), img(x, y + 1), img(x + 1, y + 1) };

			imgbuf(x, y) = kernelmedian(kernel);
		}
	}
	img.data = std::move(imgbuf.data);
}

void ImageOP::B3WaveletTransform(Image32& img, ImageVector& wavelet_vector, int scale_num) {

	wavelet_vector.reserve(scale_num);

	Image32 source(img.Rows(), img.Cols());
	img.CopyTo(source);
	ImageOP::MedianBlur3x3(source);

	Image32 convolved(img.Rows(), img.Cols());

	std::array<float, 5> kernel = { 0.0625f, 0.25f, 0.375f, 0.25f, 0.0625f }; // use actual values instead of array

	for (int i = 0; i < scale_num; ++i) {

		int _2i = pow(2, i);
		int _x2i = 2 * _2i;
		Image32 wavelet(img.Rows(), img.Cols());

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {

				float sum = 0;

				sum += source.IsInBounds(x - _x2i, y) ? source(x - _x2i, y) * kernel[0] : 0;
				sum += source.IsInBounds(x - _2i, y) ? source(x - _2i, y) * kernel[1] : 0;
				sum += source(x, y) * kernel[2];
				sum += source.IsInBounds(x + _2i, y) ? source(x + _2i, y) * kernel[3] : 0;
				sum += source.IsInBounds(x + _x2i, y) ? source(x + _x2i, y) * kernel[4] : 0;

				wavelet(x, y) = sum;
			}

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {

				float sum = 0;

				sum += wavelet.IsInBounds(x, y - _x2i) ? wavelet(x, y - _x2i) * kernel[0] : 0;
				sum += wavelet.IsInBounds(x, y - _2i) ? wavelet(x, y - _2i) * kernel[1] : 0;
				sum += wavelet(x, y) * kernel[2];
				sum += wavelet.IsInBounds(x, y + _2i) ? wavelet(x, y + _2i) * kernel[3] : 0;
				sum += wavelet.IsInBounds(x, y + _x2i) ? wavelet(x, y + _x2i) * kernel[4] : 0;

				convolved(x, y) = sum;
			}

		for (int el = 0; el < img.Total(); ++el) {
			wavelet[el] = (source[el] - convolved[el]);
			source[el] = convolved[el];
		}

		//wavelet.AvgDev_clipped();
		wavelet.AvgDev(true);
		wavelet_vector.emplace_back(std::move(wavelet));
	}

}

static void TrinerizeImage(Image32& input, Image8& output, float thresh) {

	for (int el = 0; el < input.Total(); ++el)
		output[el] = (input[el] >= thresh) ? 1 : 0;

	for (int y = 1; y < output.Rows() - 1; ++y) {
		for (int x = 1; x < output.Cols() - 1; ++x) {
			if (output(x, y) == 1)
				if (output(x - 1, y) != 0 && output(x + 1, y) != 0 && output(x, y - 1) != 0 && output(x, y + 1) != 0) output(x, y) = 2;
		}
	}

}

void ImageOP::B3WaveletTransformTrinerized(Image32& img, Image8Vector& wavelet_vector, float thresh, int scale_num) {

	wavelet_vector.reserve(scale_num);

	Image32 source(img.Rows(), img.Cols());
	img.CopyTo(source);
	ImageOP::MedianBlur3x3(source);

	Image32 convolved(img.Rows(), img.Cols());
	Image32 wavelet(img.Rows(), img.Cols());

	for (int i = 0; i < scale_num; ++i) {

		int _2i = pow(2, i);
		int _x2i = 2 * _2i;

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {

				float sum = 0;

				sum += source.IsInBounds(x - _x2i, y) ? source(x - _x2i, y) * 0.0625f : 0;
				sum += source.IsInBounds(x - _2i, y) ? source(x - _2i, y) * 0.25f : 0;
				sum += source(x, y) * 0.375f;
				sum += source.IsInBounds(x + _2i, y) ? source(x + _2i, y) * 0.25f : 0;
				sum += source.IsInBounds(x + _x2i, y) ? source(x + _x2i, y) * 0.0625f : 0;

				wavelet(x, y) = sum;
			}

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {

				float sum = 0;

				sum += wavelet.IsInBounds(x, y - _x2i) ? wavelet(x, y - _x2i) * 0.0625 : 0;
				sum += wavelet.IsInBounds(x, y - _2i) ? wavelet(x, y - _2i) * 0.25f : 0;
				sum += wavelet(x, y) * 0.375f;
				sum += wavelet.IsInBounds(x, y + _2i) ? wavelet(x, y + _2i) * 0.25f : 0;
				sum += wavelet.IsInBounds(x, y + _x2i) ? wavelet(x, y + _x2i) * 0.0625f : 0;

				convolved(x, y) = sum;
			}

		for (int el = 0; el < img.Total(); ++el) {
			wavelet[el] = (source[el] - convolved[el]);
			source[el] = convolved[el];
		}

		wavelet.AvgDev(true);
		Image8 tri_wavelet(img.Rows(), img.Cols());
		TrinerizeImage(wavelet, tri_wavelet, wavelet.median + thresh * (wavelet.avgDev / 0.6745));
		wavelet_vector.emplace_back(std::move(tri_wavelet));
	}

}

void ImageOP::ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator type) {

	float rse, cse;
	switch (type) {
	case ScaleEstimator::median:
		rse = ref.median;
		cse = tgt.median;
		break;
	case ScaleEstimator::avgdev:
		rse = ref.avgDev;
		cse = ref.avgDev;
		break;
	case ScaleEstimator::mad:
		rse = ref.mad;
		cse = tgt.mad;
		break;
	case ScaleEstimator::bwmv:
		rse = ref.bwmv;
		cse = tgt.bwmv;
		break;
	case ScaleEstimator::none:
		return;
	}

	float k = rse / cse;
	for (auto& pixel : tgt)
		pixel = ClipPixel(k * (pixel - tgt.median) + ref.median);

}

void ImageOP::ScaleImageStack(ImageVector& img_stack, ScaleEstimator type) {

	for (auto iter = img_stack.begin() + 1; iter != img_stack.end(); ++iter)
		ImageOP::ScaleImage(*img_stack.begin(), *iter, type);

}

void ImageOP::MaxMin_Normalization(Image32& img, float max, float min) {
	float dm = 1.0f / (max - min);

	for (float& pixel : img)
		pixel = (pixel - min) * dm;
}

void ImageOP::STFImageStretch(Image32& img) {

	float nMAD = 1.4826f * img.mad;

	float shadow = img.median - 2.8f * nMAD, midtone = 4.5 * (img.median - shadow);

	float m1 = midtone - 1, m2 = (2 * midtone) - 1;

	for (auto& pixel : img) {

		pixel = (pixel - shadow) / (1.0f - shadow);

		if (pixel <= 0.0f) pixel = 0.0f;

		else if (pixel == 1.0f) pixel = 1.0f;

		else if (pixel == midtone)  pixel = 0.5f;

		else
			pixel = (m1 * pixel) / ((m2 * pixel) - midtone);

	}

}

void ImageOP::ASinhStretch(Image32& img, float stretch_factor) {

	std::vector<int> hist(65536);

	for (const auto& pixel : img)
		hist[pixel * 65535]++;

	int sum = 0;
	int i = 0;

	while (sum < img.Total() * .02) {
		sum += hist[i];
		++i;
	}

	float blackpoint = i / 65535.0;

	float low = 0,
		high = 10000,
		mid;

	for (int i = 0; i < 20; i++){
		mid = (low + high) / 2;
		double multiplier_mid = mid / asinh(mid);
		(stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}

	float beta = mid;
	float asinhb = asinh(beta);

	if (img.Channels() == 1) {
		float max = img.Max();
		for (auto& pixel : img) {
			float r = (pixel - blackpoint) / (max - blackpoint);
			//float k = (r != 0) ? asinh(beta * r) / (r * asinhb) : 0;
			//pixel = ClipPixel(r * k);

			pixel = ClipPixel((r != 0) ? asinh(r * beta) / asinhb : 0);
		}
	}

	bool srbg = false;
	if (img.Channels() == 3) {

		float max = img.Max();
		std::array<float, 3> color = { 0.333333f, 0.333333f, 0.333333f };
		if (srbg) color = { 0.222491f, 0.716888f, 0.060621f };

		for (int y = 0; y < img.Rows(); ++y)
			for (int x = 0; x < img.Cols(); ++x) {
				float I = ((color[0] * img.RedPixel(x, y) + color[1] * img.GreenPixel(x, y) + color[2] * img.BluePixel(x, y)) - blackpoint) / (max - blackpoint);
				float k = (I != 0) ? ClipZero(asinh(beta * I) / (I * asinhb)) : 0;

				img.RedPixel(x, y) = ClipPixel(((img.RedPixel(x, y) - blackpoint) / (max - blackpoint)) * k);
				img.GreenPixel(x, y) = ClipPixel(((img.GreenPixel(x, y) - blackpoint) / (max - blackpoint)) * k);
				img.BluePixel(x, y) = ClipPixel(((img.BluePixel(x, y) - blackpoint) / (max - blackpoint)) * k);
			}
	}

}
