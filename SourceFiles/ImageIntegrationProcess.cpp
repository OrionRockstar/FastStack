#include "pch.h"
#include "ImageIntegrationProcess.h"
#include "StarMatching.h"
#include "Homography.h"
#include "ImageGeometry.h"
#include "FITS.h"
#include "TIFF.h"
#include "FastStack.h"

void TempFolder::writeTempFits(const Image32& src, std::filesystem::path file_path) {
	FITS fits;
	auto path = folderPath().append(file_path.stem().concat("_temp").string());
	fits.create(path);
	fits.write(src, ImageType::FLOAT);
	m_fv.push_back(path += ".fits");
}


bool ImageIntegrationProcess::isLightsSameSize() {
	int ref_rows = 1;
	int ref_cols = 1;
	int ref_channels = 1;

	for (auto file_it = m_light_paths.begin(); file_it != m_light_paths.end(); ++file_it) {

		auto file = *file_it;

		std::unique_ptr<ImageFile>imagefile;

		if (FITS::isFITS(file)) {
			imagefile = std::make_unique<FITS>();
			dynamic_cast<FITS*>(imagefile.get())->open(file);
		}

		else if (TIFF::isTIFF(file)) {
			imagefile = std::make_unique<TIFF>();
			dynamic_cast<TIFF*>(imagefile.get())->open(file);
		}

		if (file_it == m_light_paths.begin()) {
			ref_rows = imagefile->rows();
			ref_cols = imagefile->cols();
			ref_channels = imagefile->channels();
		}

		else {
			if (imagefile->rows() != ref_rows)
				return false;
			if (imagefile->cols() != ref_cols)
				return false;
			if (imagefile->channels() != ref_channels)
				return false;
		}
	}
	return true;
}



Status ImageIntegrationProcess::integrateImages(Image32& output) {

	if (m_light_paths.size() < 2)
		return {false,"Insufficient light frames."};

	if (!isLightsSameSize())
		return { false, "Light frames are not the same size." };

	TempFolder temp;

	StarVector ref_sv;

	ImageCalibrator calibrator = m_ic;

	StarMatching sm;
	HomographyTransformation homography_trans;

	for (auto file_it = m_light_paths.begin(); file_it != m_light_paths.end(); ++file_it) {
		
		auto file = *file_it;
		m_iss.emitText(file.filename().string().c_str());

		if (FITS::isFITS(file)) {
			FITS fits;
			fits.open(*file_it);
			fits.readAny(output);
		}

		else if (TIFF::isTIFF(file)) {
			TIFF tiff;
			tiff.open(file);
			tiff.readAny(output);
		}

		calibrator.calibrateImage(output);

		if (m_alignment_paths.size() != m_light_paths.size() - 1) {

			if (file_it == m_light_paths.begin()) {
				ref_sv = m_sd.applyStarDetection(output);
				m_iss.emitPSFData(ref_sv.size(), m_sd.meanPSF());
				ref_sv.shrink_to_size(m_maxstars);
			}

			else {
				StarVector tgt_sv = m_sd.applyStarDetection(output);
				m_iss.emitPSFData(tgt_sv.size(), m_sd.meanPSF());
				tgt_sv.shrink_to_size(m_maxstars);

				StarPairVector spv = sm.matchStars(ref_sv, tgt_sv);

				Matrix h = Homography::computeHomography(spv);
				m_iss.emitMatrix(h);

				if (isnan(h(0, 0))) {
					file_it = m_light_paths.erase(file_it);
					continue; //use to pass over bad frame
				}

				homography_trans.setHomography(h);
				homography_trans.apply(output);

				alignmentDataWriter(file, h);
			}
		}

		else {
			if (file_it != m_light_paths.begin()) {
				Matrix h = alignmentDataReader(m_alignment_paths[file_it - m_light_paths.begin() - 1]);
				m_iss.emitMatrix(h);
				homography_trans.setHomography(h);
				homography_trans.apply(output);
			}
		}
		temp.writeTempFits(output, file);
	}

	if (m_generate_weight_maps)
		return ImageStackingWeightMap(m_is).stackImages(temp.filePaths(), output, m_light_paths[0].parent_path());

	else
		return m_is.stackImages(temp.filePaths(), output);
}