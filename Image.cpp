#include "Image.h"

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

void ImageOP::AlignFrame_Bilinear(Image32& img, Eigen::Matrix3d homography) {
	double x_s, y_s, yx, yy, r1, r2;
	int x_f, y_f;

	std::vector<float> pixels(img.Total(), 0);

#pragma omp parallel for private(x_s, y_s, yx, yy, x_f, y_f, r1, r2)
	for (int y = 0; y < img.Rows(); ++y) {
		yx = y * homography(0, 1);
		yy = y * homography(1, 1);

		for (int x = 0; x < img.Cols(); ++x) {
			x_s = x * homography(0, 0) + yx + homography(0, 2);
			y_s = x * homography(1, 0) + yy + homography(1, 2);

			if (x_s < 1 || x_s >= img.Cols() - 1 || y_s < 1 || y_s >= img.Rows() - 1) continue;

			x_f = (int)floor(x_s);
			y_f = (int)floor(y_s);

			r1 = img(y_f, x_f) * (x_f + 1 - x_s) + img(y_f, x_s + 1) * (x_s - x_f);
			r2 = img(y_f + 1, x_f) * (x_f + 1 - x_s) + img(y_f + 1, x_f + 1) * (x_s - x_f);

			pixels[y * img.Cols() + x] = float(r1 * (y_f + 1 - y_s) + r2 * (y_s - y_f));
		}
	}

	std::copy(pixels.begin(), pixels.end(), img.data.get());

}

void ImageOP::AlignFrame_Bicubic(Image32& img, Eigen::Matrix3d homography) {
	double x_s, y_s, yx, yy;
	int x_f, y_f;
	float val;
	std::vector<float> pixels(img.Total(), 0);

	double a, b, c, d, dx, dy;
	double px[4] = { 0,0,0,0 };

#pragma omp parallel for private(x_s, y_s, yx, yy, x_f, dx, y_f, dy, a, b, c, d, px, val)
	for (int y = 0; y < img.Rows(); ++y) {
		yx = y * homography(0, 1);
		yy = y * homography(1, 1);

		for (int x = 0; x < img.Cols(); ++x) {
			x_s = x * homography(0, 0) + yx + homography(0, 2);
			y_s = x * homography(1, 0) + yy + homography(1, 2);

			if (x_s < 1 || x_s >= img.Cols() - 2 || y_s < 1 || y_s >= img.Rows() - 2) continue;

			x_f = (int)floor(x_s);
			dx = x_s - x_f;
			y_f = (int)floor(y_s);
			dy = y_s - y_f;

			for (int i = -1; i < 3; ++i) {
				a = -.5f * img(y_f + i, x_f - 1) + 1.5f * img(y_f + i, x_f) - 1.5f * img(y_f + i, x_f + 1) + .5f * img(y_f + i, x_f + 2);
				b = img(y_f + i, x_f - 1) - 2.5 * img(y_f + i, x_f) + 2 * img(y_f + i, x_f + 1) - .5 * img(y_f + i, x_f + 2);
				c = -.5 * img(y_f + i, x_f - 1) + .5 * img(y_f + i, x_f + 1);
				d = img(y_f + i, x_f);
				px[i + 1] = (a * dx * dx * dx) + (b * dx * dx) + (c * dx) + d;
			}

			a = -.5 * px[0] + 1.5 * px[1] - 1.5 * px[2] + .5 * px[3];
			b = px[0] - 2.5 * px[1] + 2 * px[2] - .5 * px[3];
			c = -.5 * px[0] + .5 * px[2];
			d = px[1];
			val = float((a * dy * dy * dy) + (b * dy * dy) + (c * dy) + d);
			if (val > 1)
				pixels[y * img.Cols() + x] = 1;
			else if (val < 0)
				pixels[y * img.Cols() + x] = 0;
			else
				pixels[y * img.Cols() + x] = val;

			//pixels[y * img.cols + x] = float((a * dy * dy * dy) + (b * dy * dy) + (c * dy) + d);
		}
	}
	//will need to clamp/trim data
	std::copy(pixels.begin(), pixels.end(), img.data.get());
}

void ImageOP::MedianBlur3x3(Image32& img) {
	//float* iptr = (float*)img.data;

	std::array<float, 9>kernel = { 0 };
	std::vector<float> imgbuf(img.Total());

#pragma omp parallel for firstprivate(kernel)
	for (int y = 1; y < img.Rows() - 1; ++y) {
		for (int x = 1; x < img.Cols() - 1; ++x) {
			kernel = { img(y - 1, x - 1), img(y - 1, x), img(y - 1, x + 1),
					   img(y , x - 1), img(y, x), img(y, x + 1),
					   img(y - 1, x - 1), img(y + 1, x), img(y + 1, x + 1) };

			for (int r = 0; r < 3; ++r) {
				for (int i = 0; i < 4; ++i) {
					if (kernel[i] > kernel[4])
						std::swap(kernel[i], kernel[4]);
					if (kernel[i + 5] < kernel[4])
						std::swap(kernel[i + 5], kernel[4]);
				}
			}

			imgbuf[y * img.Cols() + x] = kernel[4];
		}
	}
	memcpy(img.data.get(), &imgbuf[0], img.Total() * 4);
}

void ImageOP::TrimHighLow(Image32& img, float high, float low) {

	for (int el = 0; el < img.Total(); ++el) {
		if (img[el] > high)
			img[el] = high;
		else if (img[el] < low)
			img[el] = low;
	}
}

void ImageOP::STFImageStretch(Image32& img) {

	float median, nMAD;
	img.nMAD(median, nMAD);

	float shadow = median - 2.8f * nMAD, midtone = 4 * 1.4826f * (median - shadow);

	for (int el = 0; el < img.Total(); ++el) {

		if (img[el] < shadow) { img[el] = shadow; continue; }

		img[el] = (img[el] - shadow) / (1 - shadow);

		if (img[el] == 0 || img[el] == 1)  continue;

		else if (img[el] == midtone)  img[el] = .5;

		else
			img[el] = ((midtone - 1) * img[el]) / (((2 * midtone - 1) * img[el]) - midtone);

	}

}
