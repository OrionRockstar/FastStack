#include "Image.h"
#include "Interpolation.h"
#include "tiffio.h"
#include "cfitsio/fitsio.h"
#include <array>
#include <map>


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

void ImageOP::TiffRead(std::string file, Image32& img) {

	uint32_t imagelength, imagewidth;
	short bd;
	TIFF* tiff = TIFFOpen(file.c_str(), "r");
	if (tiff) {
		TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &imagelength);
		TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &imagewidth);
		TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bd);
		switch (bd) {
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

void ImageOP::FitsRead(std::string file, Image32& img) {

	//Image img;
	fitsfile* fptr;
	int status = 0;
	int bitpix, naxis;
	long naxes[2] = { 1,1 }, fpixel[2] = { 1,1 };//naxes={row,col}

	if (!fits_open_file(&fptr, file.c_str(), READONLY, &status))
	{
		if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status))
		{
			if (naxis > 2 || naxis == 0)
				NULL;
				//std::cout << "Error: only 2D images are supported\n";
			else
			{
				switch (bitpix) {
				case 16: {

					Image16 temp(naxes[1], naxes[0]);

					fits_read_pix(fptr, TUSHORT, fpixel, naxes[0] * naxes[1], NULL, temp.data.get(), NULL, &status);

					img = Image32(naxes[1], naxes[0]);

					for (int el = 0; el < img.Total(); ++el)
						img[el] = float(temp[el]) / 65535;

					break;
				}

				case 8: {
					Image8 temp(naxes[1], naxes[0]);

					fits_read_pix(fptr, TBYTE, fpixel, naxes[0] * naxes[1], NULL, img.data.get(), NULL, &status);

					img = Image32(naxes[1], naxes[0]);

					for (int el = 0; el < img.Total(); ++el)
						img[el] = float(temp[el]) / 255;

					break;
				}
				case -32: {
					Image32 img(naxes[1], naxes[0]);

					fits_read_pix(fptr, TFLOAT, fpixel, naxes[0] * naxes[1], NULL, img.data.get(), NULL, &status);

					break;
				}
				}
			}
		}
		fits_close_file(fptr, &status);
	}

	if (status) fits_report_error(stderr, status); // print any error message 

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
			img(y, x) = (float)temp(pcl::Point(x, y)) / 65535;

}*/

void ImageOP::FitsWrite(Image32& img, std::string filename) {
	fitsfile* fptr;
	int status, exists;
	long  fpixel = 1, naxis = 2;// exposure;
	long naxes[2] = { img.Cols(), img.Rows() };

	status = 0;
	fits_file_exists((filename + ".fits").c_str(), &exists, &status);

	if (exists == 1) {
		fits_open_file(&fptr, (filename + ".fits").c_str(), READWRITE, &status);
		fits_delete_file(fptr, &status);
	}

	fits_create_file(&fptr, (filename + ".fits").c_str(), &status);

	fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status);

	fits_write_img(fptr, TFLOAT, fpixel, img.Total(), img.data.get(), &status);

	fits_close_file(fptr, &status);

	fits_report_error(stderr, status);
	//return(status);
}

void ImageOP::AlignFrame(Image32& img, Eigen::Matrix3d homography, std::function<float(Image32&, double& x_s, double& y_s)> interp_type) {
	Image32 pixels(img.Rows(), img.Cols());
	pixels.homography = img.homography;
#pragma omp parallel for
	for (int y = 0; y < img.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < img.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2);
			double y_s = x * homography(1, 0) + yy + homography(1, 2);

			pixels(y, x) = ClipPixel(interp_type(img, x_s, y_s));

		}
	}

	pixels.ComputeStats();
	img = std::move(pixels);
}

void ImageOP::DrizzleFrame(Image32& input, Image32& output, float drop) {

	input.homography = Eigen::Inverse(input.homography);
	float oweight = 1;

	float s2 = drop * drop,
		  offset = (1 - drop) / 2,
		  x2drop = 2 * drop,
		  drop_area = x2drop * x2drop;

#pragma omp parallel for
	for (int y = 0; y < input.Rows(); ++y) {

		double yx = y * input.homography(0, 1);
		double yy = y * input.homography(1, 1);

		for (int x = 0; x < input.Cols(); ++x) {
			double x_s = x * input.homography(0, 0) + yx + input.homography(0, 2) + offset;
			double y_s = x * input.homography(1, 0) + yy + input.homography(1, 2) + offset;

			int x_f = (int)floor(2 * x_s);
			int y_f = (int)floor(2 * y_s);

			if (x_f < 0 || x_f >= output.Cols() - 2 || y_f < 0 || y_f >= output.Rows() - 2)
				continue;

			float vx = (1 - ((2 * (x_s)) - x_f));
			float vy = (1 - ((2 * (y_s)) - y_f));

			std::array<float, 9> area{ 0 };

			if (x2drop >= vx && x2drop >= vy)
				area[0] = (vx * vy) / drop_area;
			else if (x2drop < vx && x2drop > vy)
				area[0] = ((x2drop)*vy) / drop_area;
			else if (x2drop > vx && x2drop < vy)
				area[0] = (vx * (x2drop)) / drop_area;
			else
				area[0] = ((x2drop) * (x2drop)) / drop_area;

			if (output(y_f, x_f) == 0)
				output(y_f, x_f) = (input(y, x) * area[0] * s2 + output(y_f, x_f)) / area[0];
			else
				output(y_f, x_f) = (input(y, x) * area[0] * s2 + output(y_f, x_f)) / (area[0] + oweight);

			if (x2drop >= vx + 1 && x2drop > vy) {
				area[1] = vy / drop_area;
				if (output(y_f, x_f + 1) == 0)
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / area[1];
				else
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / (area[1] + oweight);
			}
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop>vy) {
				area[1] = (vy * (x2drop - vx)) / drop_area;
				if (output(y_f, x_f + 1) == 0)
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / area[1];
				else
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / (area[1] + oweight);
			}
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop < vy) {
				area[1] = (x2drop * (x2drop - vx)) / drop_area;
				if (output(y_f, x_f + 1) == 0)
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / area[1];
				else
					output(y_f, x_f + 1) = (input(y, x) * area[1] * s2 + output(y_f, x_f + 1)) / (area[1] + oweight);
			}

			if (x2drop > vx + 1) {
				area[2] = (vy * (x2drop - vx - 1)) / drop_area;
				if (output(y_f, x_f + 2) == 0)
					output(y_f, x_f + 2) = (input(y, x) * area[2] * s2 + output(y_f, x_f + 2)) / area[2];
				else
					output(y_f, x_f + 2) = (input(y, x) * area[2] * s2 + output(y_f, x_f + 2)) / (area[2] + oweight);
			}

			if (x2drop >= vy + 1) {
				area[3] = vx / drop_area;
				if (output(y_f + 1, x_f) == 0)
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / area[3];
				else
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / (area[3] + oweight);
			}
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx) {
				area[3] = (vx * (x2drop - vy)) / drop_area;
				if (output(y_f + 1, x_f) == 0)
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / area[3];
				else
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / (area[3] + oweight);
			}
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop < vx) {
				area[3] = ((x2drop) * (x2drop - vy)) / drop_area;
				if (output(y_f + 1, x_f) == 0)
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / area[3];
				else
					output(y_f + 1, x_f) = (input(y, x) * area[3] * s2 + output(y_f + 1, x_f)) / (area[3] + oweight);
			}

			if (x2drop >= vx + 1 && x2drop >= vy + 1) {
				area[4] = 1 / drop_area;
				if (output(y_f + 1, x_f + 1) == 0)
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / area[4];
				else
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / (area[4] + oweight);
			}
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx + 1) {
				area[4] = (x2drop - vy) / drop_area;
				if (output(y_f + 1, x_f + 1) == 0)
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / area[4];
				else
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / (area[4] + oweight);
			}
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop >= vy + 1) {
				area[4] = (x2drop - vx) / drop_area;
				if (output(y_f + 1, x_f + 1) == 0)
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / area[4];
				else
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / (area[4] + oweight);
			}
			else if ((x2drop < vx + 1 && x2drop >= vx) && (x2drop < vy + 1 && x2drop >= vy)) {
				area[4] = ((x2drop - vx) * (x2drop - vy)) / drop_area;
				if (output(y_f + 1, x_f + 1) == 0)
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / area[4];
				else
					output(y_f + 1, x_f + 1) = (input(y, x) * area[4] * s2 + output(y_f + 1, x_f + 1)) / (area[4] + oweight);
			}

			if (x2drop > vx + 1 && x2drop > vy + 1) {
				area[5] = (x2drop - vx - 1) / drop_area;
				if (output(y_f + 1, x_f + 2) == 0)
					output(y_f + 1, x_f + 2) = (input(y, x) * area[5] * s2 + output(y_f + 1, x_f + 2)) / area[5];
				else
					output(y_f + 1, x_f + 2) = (input(y, x) * area[5] * s2 + output(y_f + 1, x_f + 2)) / (area[5] + oweight);
			}
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop > vx + 1) {
				area[5] = ((x2drop - vy) * (x2drop - vx - 1)) / drop_area;
				if (output(y_f + 1, x_f + 2) == 0)
					output(y_f + 1, x_f + 2) = (input(y, x) * area[5] * s2 + output(y_f + 1, x_f + 2)) / area[5];
				else
					output(y_f + 1, x_f + 2) = (input(y, x) * area[5] * s2 + output(y_f + 1, x_f + 2)) / (area[5] + oweight);
			}

			if (x2drop > vy + 1) {
				area[6] = (vx * (x2drop - vy - 1)) / drop_area;

				if (output(y_f + 2, x_f) == 0)
					output(y_f + 2, x_f) = (input(y, x) * area[6] * s2 + output(y_f + 2, x_f)) / area[6];
				else
					output(y_f + 2, x_f) = (input(y, x) * area[6] * s2 + output(y_f + 2, x_f)) / (area[6] + oweight);
			}

			if (x2drop > vx + 1 && x2drop > vy + 1) {
				area[7] = (x2drop - vy - 1) / drop_area;
				if (output(y_f + 2, x_f + 1) == 0)
					output(y_f + 2, x_f + 1) = (input(y, x) * area[7] * s2 + output(y_f + 2, x_f + 1)) / area[7];
				else
					output(y_f + 2, x_f + 1) = (input(y, x) * area[7] * s2 + output(y_f + 2, x_f + 1)) / (area[7] + oweight);
			}
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop > vy + 1) {
				area[7] = ((x2drop - vy - 1) * (x2drop - vx)) / drop_area;
				if (output(y_f + 2, x_f + 1) == 0)
					output(y_f + 2, x_f + 1) = (input(y, x) * area[7] * s2 + output(y_f + 2, x_f + 1)) / area[7];
				else
					output(y_f + 2, x_f + 1) = (input(y, x) * area[7] * s2 + output(y_f + 2, x_f + 1)) / (area[7] + oweight);
			}

			if (x2drop > vy + 1 && x2drop > vx + 1) {
				area[8] = ((x2drop - vy - 1) * (x2drop - vx - 1)) / drop_area;
				if (output(y_f + 2, x_f + 2) == 0)
					output(y_f + 2, x_f + 2) = (input(y, x) * area[8] * s2 + output(y_f + 2, x_f + 2)) / area[8];
				else
					output(y_f + 2, x_f + 2) = (input(y, x) * area[8] * s2 + output(y_f + 2, x_f + 2)) / (area[8] + oweight);
			}

		}
	}
}

void ImageOP::Resize2x_Bicubic(Image32& img) {
	Image32 temp(img.Rows() * 2, img.Cols() * 2);

#pragma omp parallel for
	for (int y = 0; y < temp.Rows(); ++y) {
		double y_s = 0.5 * y;

		for (int x = 0; x < temp.Cols(); ++x) {
			double x_s = 0.5 * x;

			float val = ClipPixel(Interpolation::Bicubic_Spline(img, x_s, y_s));

		}
	}

	img = std::move(temp);
}

void ImageOP::ImageResize_Bicubic(Image32& img, int new_rows, int new_cols) {
	Image32 temp(new_rows, new_cols);
	double ry = double(img.Rows()) / temp.Rows();
	double rx = double(img.Cols()) / temp.Cols();

#pragma omp parallel for
	for (int y = 0; y < temp.Rows(); ++y) {
		double y_s = y * ry;

		for (int x = 0; x < temp.Cols(); ++x) {
			double x_s = x * rx;

			float val = ClipPixel(Interpolation::Bicubic_Spline(img, x_s, y_s));

		}
	}

	img = std::move(temp);
	img.ComputeStats();
}

void ImageOP::Bin2x(Image32& img) {
	Image32 temp(img.Rows() / 2, img.Cols() / 2);

#pragma omp parallel for
	for (int y = 0; y < temp.Rows(); ++y) {
		int y_s = 2 * y;

		for (int x = 0; x < temp.Cols(); ++x) {
			int x_s = 2 * x;

			temp(y, x) = (img(y_s, x_s) + img(y_s, x_s + 1) + img(y_s + 1, x_s) + img(y_s + 1, x_s + 1)) / 4;

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
			kernel = { img(y - 1, x - 1), img(y - 1, x), img(y - 1, x + 1),
					   img(y , x - 1), img(y, x), img(y, x + 1),
					   img(y - 1, x - 1), img(y + 1, x), img(y + 1, x + 1) };

			imgbuf(y, x) = kernelmedian(kernel);
		}
	}
	img.data = std::move(imgbuf.data);
}

void ImageOP::TrimHighLow(Image32& img, float high, float low) {

	for (int el = 0; el < img.Total(); ++el) {
		if (img[el] > high)
			img[el] = high;
		else if (img[el] < low)
			img[el] = low;
	}
}

void ImageOP::MaxMin_Normalization(Image32& img, float max, float min) {
	for (float& pixel : img)
		pixel = (pixel - min) / (max - min);
}

void ImageOP::STFImageStretch(Image32& img) {

	if (img.max > 1 || img.min < 0)
		MaxMin_Normalization(img, 1, 0);

	float nMAD=img.nMAD();

	float shadow = img.median - 2.8f * nMAD, midtone = 4 * 1.4826f * (img.median - shadow);

	for (int el = 0; el < img.Total(); ++el) {

		if (img[el] < shadow) { img[el] = shadow; continue; }

		img[el] = (img[el] - shadow) / (1 - shadow);

		if (img[el] == 0 || img[el] == 1)  continue;

		else if (img[el] == midtone)  img[el] = .5;

		else
			img[el] = ((midtone - 1) * img[el]) / (((2 * midtone - 1) * img[el]) - midtone);

	}

}

void ImageOP::ASinhStretch(Image32& img, float stretch_factor) {

	std::map<uint16_t, int> hist;
	for (const auto& pixel : img)
		hist[pixel * 65535] ++;

	int sum = 0;
	uint16_t key;
	int i = 0;
	while (sum < img.Total() * .02) {
		sum += hist[i];
		++i;
	}

	float blackpoint = i / 65535.0;

	float low = 0;
	float high = 10000;
	float mid;
	float beta;

	for (int i = 0; i < 20; i++)
	{
		mid = (low + high) / 2;
		double multiplier_mid = mid / asinh(mid);
		(stretch_factor <= multiplier_mid) ? high = mid : low = mid;
	}
	beta = mid;

	float asinhb = asinh(beta);
	for (auto& pixel : img) {
		float r = (pixel - blackpoint) / (1 - blackpoint);
		if (r != 0)
			pixel = ClipZero(r * asinh(beta * r) / (r * asinhb));
		else pixel = 0;
	}

	ImageOP::MaxMin_Normalization(img, img.Max(), 0);

}

