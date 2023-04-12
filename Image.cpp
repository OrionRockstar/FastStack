#include "Image.h"
#include "tiffio.h"
#include "cfitsio/fitsio.h"
#include <array>
#include <fstream>


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

