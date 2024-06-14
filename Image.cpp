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
		//ReadStatsText(file_it.replace_extension(".txt"), img);
		img_stack.emplace_back(std::move(img));
	}

	std::filesystem::remove_all("./temp");
}


