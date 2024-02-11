#include "pch.h"
#include "Image.h"

bool StatsTextExists(const std::filesystem::path& file_path) {
	auto temp = file_path;
	return std::filesystem::exists(temp.replace_extension(".txt"));
}

void GetImageStackFromTemp(FileVector& light_files, ImageVector& img_stack) {

	Image32 img;

	img_stack.reserve(light_files.size());

	for (auto file_it : light_files) {
		//FileOP::FitsRead("./temp//" + file_it.stem().string() + "temp.fits", img);
		ReadStatsText(file_it.replace_extension(".txt"), img);
		img_stack.emplace_back(std::move(img));
	}

	std::filesystem::remove_all("./temp");
}

/*bool FileOP::FitsRead(const std::filesystem::path file, Image32& img) {

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
}*/

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

