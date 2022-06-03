#include "Image.h"

Image::Image(int r, int c, int bitdepth) :rows(r), cols(c), type(bitdepth) {
	switch (bitdepth) {
	case 8:
		Image::data = new unsigned char[r * c];
		break;
	case 16:
		//Image::data16 = new unsigned short[r * c];
		Image::data = new unsigned short[r * c];
		break;
	case 32:
		Image::data = new float[r * c];
		break;
	default:
		Image::data = nullptr;

	}
}

Image::Image(const Image &img) {
	rows = img.rows;
	cols = img.cols;
	type = img.type;
	total = img.total;
	if (data)
		delete[] data;

	switch (type) {
	case 8:
		data = new unsigned char[total];
		memcpy(data, img.data, (size_t)total);
		break;
	case 16:
		data = new unsigned short[total];
		memcpy(data, img.data, (size_t)total * 2);
		break;
	case 32:
		data = new float[total];
		memcpy(data, img.data, (size_t)total * 4);
		break;
	}
}

Image::Image(Image&& img) {
	rows = img.rows;
	cols = img.cols;
	type = img.type;
	total = img.total;
	data = img.data;

	img.data = nullptr;
}

Image Image::DeepCopy() {
	Image temp(Image::rows, Image::cols, Image::type);
	memcpy(temp.data, Image::data, (size_t)Image::total*(Image::type/8));
	return temp;
}

void Image::Release()
{
	if (data) {
		Image::~Image();
		Image::rows = 0,
		Image::cols = 0,
		Image::total = 0,
		Image::type = 0;
	}
}

void Image::Update(int r, int c, int bd) {
	Image::rows = r;
	Image::cols = c;
	Image::total = r * c;
	Image::type = bd;
	switch (bd) {
	case 8:
		Image::data = new unsigned char[r * c];
		break;
	case 16:
		Image::data = new unsigned short[r * c];
		break;
	case 32:
		Image::data = new float[r * c];
		break;
	default:
		Image::data = nullptr;
		break;
	}
}

Image ImageOP::ImRead(char* file) {
	Image img;
	if (std::regex_search(file, std::regex("tif+$"))) {
		uint32_t imagelength, imagewidth;
		short bd;
		TIFF* tiff = TIFFOpen(file, "r");
		if (tiff) {
			TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &imagelength);
			TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &imagewidth);
			TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bd);
			switch (bd) {
			case 16: {
				img.Update(imagelength, imagewidth, bd);
				unsigned short* ptr = (unsigned short*)img.data;

				for (uint32_t row = 0; row < imagelength; ++row)
					TIFFReadScanline(tiff, &ptr[row * imagewidth], row);

				img.Convert<unsigned short, float>(img);

				float* nptr = (float*)img.data;

				for (int el = 0; el < img.total; ++el) nptr[el] /= 65535;

				break; }

			case 8: {
				img.Update(imagelength, imagewidth, bd);
				unsigned char* ptr = (unsigned char*)img.data;

				for (uint32_t row = 0; row < imagelength; ++row)
					TIFFReadScanline(tiff, &ptr[row * imagewidth], row);

				img.Convert<unsigned char, float>(img);

				float* nptr = (float*)img.data;

				for (int el = 0; el < img.total; ++el) nptr[el] /= 255;

				break; }
			}
			TIFFClose;
		}

	}
	else if (std::regex_search(file, std::regex("fits$"))) {
		fitsfile* fptr;
		int status = 0;
		int bitpix, naxis;
		long naxes[2] = { 1,1 }, fpixel[2] = { 1,1 };//naxes={row,col}

		if (!fits_open_file(&fptr, file, READONLY, &status))
		{
			if (!fits_get_img_param(fptr, 2, &bitpix, &naxis, naxes, &status))
			{
				if (naxis > 2 || naxis == 0);
					//std::cout << "Error: only 2D images are supported\n";
				else
				{
					switch (bitpix) {
					case 16: {
						img.Update(naxes[1], naxes[0], bitpix);
						unsigned short* ptr = (unsigned short*)img.data;
						fits_read_pix(fptr, TUSHORT, fpixel, naxes[0] * naxes[1], NULL, ptr, NULL, &status);
						img.Convert<unsigned short, float>(img);

						float* nptr = (float*)img.data;

						for (int el = 0; el < img.total; ++el) nptr[el] /= 65535;

						break;
					}

					case 8: {
						img.Update(naxes[1], naxes[0], bitpix);
						unsigned char* ptr = (unsigned char*)img.data;
						fits_read_pix(fptr, TUSHORT, fpixel, naxes[0] * naxes[1], NULL, ptr, NULL, &status);
						img.Convert<unsigned char, float>(img);

						float* nptr = (float*)img.data;

						for (int el = 0; el < img.total; ++el) nptr[el] /= 255;

						break;
					}
					}
				}
			}
			fits_close_file(fptr, &status);
		}

		if (status) fits_report_error(stderr, status); /* print any error message */
	}
	return img;
}

void ImageOP::AlignFrame(Image& img, Eigen::Matrix3d homography) {
	float* fptr = (float*)img.data;
	int x_n, y_n, temp, yx, yy, size = img.rows * img.cols, disp = img.cols * (int)round(homography(1, 2)) + (int)round(homography(0, 2));
	double theta = atan2(homography(1, 0), homography(0, 0));
	double aff[4] = { cos(theta),sin(theta),-sin(theta),cos(theta) };

	std::vector<float> pixels(size, 0);
	if (fabs(theta) <= M_PI_2) {
#pragma omp parallel for private(x_n,y_n,yx,yy,temp)
		for (int y = 0; y < img.rows; ++y) {
			yx = int(y * aff[1]);
			yy = int(y * aff[3]);
			for (int x = 0; x < img.cols; ++x)
			{
				x_n = int(x * aff[0] + yx);
				y_n = int(x * aff[2] + yy);
				temp = (y_n * img.cols + x_n) - disp;

				if (temp >= size)
					temp -= size;

				else if (temp < 0)
					temp += size;

				pixels[temp] = fptr[y * img.cols + x];
			}
		}
	}

	else {
#pragma omp parallel for private(x_n,y_n,yx,yy,temp)
		for (int y = 0; y < img.rows; ++y) {
			yx = int(y * homography(0, 1));
			yy = int(y * homography(1, 1));
			for (int x = 0; x < img.cols; ++x)
			{
				x_n = int(x * homography(0, 0) + yx);
				y_n = int(x * homography(1, 0) + yy);
				temp = (y_n * img.cols + x_n) + disp;
				if (temp >= size)
					temp -= size;

				else if (temp < 0)
					temp += size;

				pixels[temp] = fptr[y * img.cols + x];
			}
		}
	}
	//memcpy(img.data, &pixels[0], 4 * img.total);
	std::copy(pixels.begin(), pixels.end(), &fptr[0]);
	//delete pixels;
}

void ImageOP::MedianBlur3x3(Image& img) {
	float* iptr = (float*)img.data;

	std::array<float, 9>kernel = { 0 };
	std::vector<float> imgbuf(img.rows * img.cols);

#pragma omp parallel for firstprivate(kernel)
	for (int y = 1; y < img.rows - 1; ++y) {
		for (int x = 1; x < img.cols - 1; ++x) {
			kernel = { iptr[(y - 1) * img.cols + x - 1], iptr[(y - 1) * img.cols + x], iptr[(y - 1) * img.cols + x + 1],
					   iptr[y * img.cols + x - 1], iptr[y * img.cols + x], iptr[y * img.cols + x + 1],
					   iptr[(y + 1) * img.cols + x - 1], iptr[(y + 1) * img.cols + x], iptr[(y + 1) * img.cols + x + 1] };

			for (int r = 0; r < 3; ++r) {
				for (int i = 0; i < 4; ++i) {
					if (kernel[i] > kernel[4])
						std::swap(kernel[i], kernel[4]);
					if (kernel[i + 5] < kernel[4])
						std::swap(kernel[i + 5], kernel[4]);
				}
			}

			imgbuf[y * img.cols + x] = kernel[4];
		}
	}
	memcpy(img.data, &imgbuf[0], img.total * 4);
}

double ImageOP::Median(Image& img) {
	std::vector<float>imgbuf(img.total);
	memcpy(&imgbuf[0], img.data, img.total * 4);
	std::nth_element(&imgbuf[0], &imgbuf[imgbuf.size() / 2], &imgbuf[imgbuf.size()]);
	return (double)imgbuf[imgbuf.size() / 2];
}

double ImageOP::StandardDeviation(Image& img) {
	float* ptr = (float*)img.data;
	float mean = 0;

	for (int el = 0; el < img.total; ++el)
		mean += ptr[el];

	mean /= img.total;
	float var = 0, d;

	for (int el = 0; el < img.total; ++el) {
		d = ptr[el] - mean;
		var += d * d;
	}
	return sqrt(var / img.total);
}

double ImageOP::StandardDeviation256(Image& img) {
	float* ptr = (float*)img.data;
	__m256 mean = _mm256_setzero_ps();

	for (int el = 0; el < img.total / 8; ++el, ptr += 8)
		mean = _mm256_add_ps(mean, _mm256_load_ps(ptr));

	mean = _mm256_div_ps(mean, _mm256_set1_ps(img.total));
	float* mp = (float*)&mean;
	float m = 0;
	for (int el = 0; el < 8; ++el, ++mp)
		m += *mp;

	mean = _mm256_set1_ps(m);
	__m256 var = _mm256_setzero_ps(), d;
	ptr -= img.total;

	for (int el = 0; el < img.total / 8; ++el, ptr += 8) {
		d = _mm256_sub_ps(_mm256_load_ps(ptr), mean);
		var = _mm256_add_ps(var, _mm256_mul_ps(d, d));
	}

	float* vp = (float*)&var;
	float v = 0;
	for (int el = 0; el < 8; ++el, ++vp)
		v += *vp;
	return sqrt(v / img.total);
}

void ImageOP::nMAD(Image& img, double& median, double& nMAD) {
	std::vector<float> imgbuf(img.total);
	float* bptr = &imgbuf[0];
	memcpy(&imgbuf[0], img.data, img.total * 4);
	std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
	median = imgbuf[imgbuf.size() / 2];
	for (int i = 0; i < (int)imgbuf.size(); ++i)
		imgbuf[i] = fabs(imgbuf[i] - median);
	std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
	nMAD = 1.4826 * imgbuf[imgbuf.size() / 2];
}

void ImageOP::AvgAbsDev(Image& img, double& median, double& abs_dev) {
	std::vector<float> buf(img.total);
	memcpy(&buf[0], img.data, img.total * 4);
	std::nth_element(buf.begin(), buf.begin() + img.total / 2, buf.end());
	median = buf[img.total / 2];
	double sum = 0;
	for (int el = 0; el < img.total; ++el) {
		sum += fabs(buf[el] - median);
	}
	abs_dev = sum / img.total;
}

void ImageOP::STFImageStretch(Image& img) {
	float* ptr = (float*)img.data;

	double median, nMAD;
	ImageOP::nMAD(img, median, nMAD);

	float shadow = median - 2.8 * nMAD, midtone = 4 * 1.4826 * (median - shadow);

	for (int el = 0; el < (img.rows * img.cols); ++el) {

		if (ptr[el] < shadow) { ptr[el] = 0; continue; }

		ptr[el] = (ptr[el] - shadow) / (1 - shadow);

		if (ptr[el] == 0 || ptr[el] == 1)  continue;

		else if (ptr[el] == midtone)  ptr[el] = .5;

		else
			ptr[el] = ((midtone - 1) * ptr[el]) / (((2 * midtone - 1) * ptr[el]) - midtone);

	}

}

void ImageOP::STFImageStretch256(Image& img) {
	float* ptr = (float*)img.data;

	double median, nMAD;
	ImageOP::nMAD(img, median, nMAD);

	double shadow = median - 2.8 * nMAD, midtone = 4 * 1.4826 * (median - shadow);

	__m256 _sh, one, pixels, midtone_mask, _mt, _midp, _pm, zero, two;
	_mt = _mm256_set1_ps(midtone);
	_midp = _mm256_set1_ps(0.5);
	one = _mm256_set1_ps(1);
	zero = _mm256_setzero_ps();
	_sh = _mm256_set1_ps(shadow);
	two = _mm256_set1_ps(2);

	for (int el = 0; el < (img.rows * img.cols) / 8; ++el, ptr += 8) {
		pixels = _mm256_div_ps(_mm256_sub_ps(_mm256_load_ps(ptr), _sh), _mm256_sub_ps(one, _sh));

		_pm = _mm256_cmp_ps(pixels, zero, _CMP_LT_OQ);
		pixels = _mm256_add_ps(_mm256_sub_ps(pixels, _mm256_and_ps(pixels, _pm)), _mm256_and_ps(_mm256_set1_ps(0.005), _pm));

		midtone_mask = _mm256_cmp_ps(pixels, _mt, _CMP_EQ_OQ);
		pixels = _mm256_add_ps(_mm256_sub_ps(pixels, _mm256_and_ps(_mt, midtone_mask)), _mm256_and_ps(_midp, midtone_mask));

		_pm = _mm256_and_ps(_mm256_cmp_ps(pixels, one, _CMP_NEQ_OQ), _mm256_cmp_ps(pixels, zero, _CMP_NEQ_OQ));
		_pm = _mm256_and_ps(_pm, _mm256_cmp_ps(pixels, _midp, _CMP_NEQ_OQ));
		pixels = _mm256_and_ps(pixels, _pm);

		pixels = _mm256_div_ps(_mm256_mul_ps(_mm256_sub_ps(_mt, one), pixels), _mm256_fmsub_ps(_mm256_fmsub_ps(two, _mt, one), pixels, _mt));

		_mm256_store_ps(ptr, pixels);

	}

}