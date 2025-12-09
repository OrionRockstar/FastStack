#pragma once
#include "StarDetector.h"
#include "ImageStacking.h"
#include "ImageCalibration.h"

class TempFolder {
	std::filesystem::path m_temp_path = std::filesystem::temp_directory_path();

	FileVector m_fv;
public:
	TempFolder() {

		m_temp_path.append("FastStackTemp");
		if (std::filesystem::exists(m_temp_path))
			std::filesystem::remove_all(m_temp_path);

		std::filesystem::create_directory(m_temp_path);
	}

	TempFolder(const std::string& forlder_name) {

		m_temp_path.append(forlder_name);

		if (std::filesystem::exists(m_temp_path))
			std::filesystem::remove_all(m_temp_path);

		std::filesystem::create_directory(m_temp_path);
	}

	~TempFolder() {

		if (std::filesystem::exists(m_temp_path))
			std::filesystem::remove_all(m_temp_path);

	}

	std::filesystem::path folderPath() const { return m_temp_path; }

	const FileVector& filePaths()const { return m_fv; }

	template<typename T>
	void writeTempFits(const Image<T>& src, std::filesystem::path file_path);
};


static void alignmentDataWriter(const std::filesystem::path& file_path, const Matrix& homography = Matrix(3, 3, { 1,0,0,0,1,0,0,0,1 })) {

	auto path = file_path;
	path = path.parent_path().append("AlignmentData");

	if (!std::filesystem::exists(path))
		std::filesystem::create_directory(path);

	path.append(file_path.filename().replace_extension("info").string());

	if (std::filesystem::exists(path))
		return;

	std::fstream stream(path, std::ios::out);

	stream << file_path.string() << "\n\n";

	stream << "H0: " << homography(0, 0) << "\n";
	stream << "H1: " << homography(0, 1) << "\n";
	stream << "H2: " << homography(0, 2) << "\n";

	stream << "H3: " << homography(1, 0) << "\n";
	stream << "H4: " << homography(1, 1) << "\n";
	stream << "H5: " << homography(1, 2) << "\n";

	stream << "H6: " << homography(2, 0) << "\n";
	stream << "H7: " << homography(2, 1) << "\n";
	stream << "H8: " << homography(2, 2) << "\n";

	stream.close();
}

static Matrix alignmentDataReader(const std::filesystem::path& file_path) {

	std::fstream stream(file_path, std::ios::in);

	Matrix homography(3, 3);

	std::string line;
	int line_no = 0;
	while (std::getline(stream, line)) {
		if (1 < line_no && line_no < 11)
			homography[line_no - 2] = std::stod(line.substr(4));
		line_no++;
	}
	stream.close();

	return homography;
}

static std::filesystem::path alignmentLightPath(const std::filesystem::path& alignment_file_path) {

	std::fstream stream(alignment_file_path, std::ios::in);
	std::string line;
	std::getline(stream, line);
	return line;
}





struct ImageStackingFiles {

	std::filesystem::path light;
	std::filesystem::path alignment;
	std::filesystem::path weight_map;

	ImageStackingFiles(const std::filesystem::path& light_path) : light(light_path) {}
};


class ImageIntegrationProcess {

	ImageStackingSignal m_iss;

	StarDetector m_sd;

	uint16_t m_maxstars = 200;

	ImageStacking m_is;

	std::vector<ImageStackingFiles> m_paths;

	ImageCalibrator m_ic;

	bool m_generate_weight_maps = false;

	bool isLightsSameSize();

public:
	uint16_t maxStars()const { return m_maxstars; }

	void setMaxStars(uint16_t num_stars) { m_maxstars = num_stars; }

	void setPaths(const PathVector& light_paths, const PathVector& alignment_paths) {

		m_paths.clear();
		m_paths.reserve(light_paths.size());

		for (auto path : light_paths)
			m_paths.emplace_back(path);

		for (auto path : alignment_paths) {
			auto light = alignmentLightPath(path);
			for (auto& p : m_paths) {
				if (p.light == light) {
					p.alignment = path;
					break;
				}
			}
		}
	}

	bool generateWeightMaps()const { return m_generate_weight_maps; }

	void setGenerateWeightMaps(bool generate = false) { m_generate_weight_maps = generate; }

	ImageCalibrator& imageCalibrator() { return m_ic; }

	StarDetector& starDetector() { return m_sd; }

	ImageStacking& imageStacker() { return m_is; }

	const ImageStackingSignal* imageStackingSignal()const { return &m_iss; }

	Status integrateImages(Image32& out);

};

