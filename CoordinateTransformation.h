#pragma once
#include "StarDetection.h"
#include "StarMatching.h"
#include <eigen3/Eigen/Dense>

class CoordinateTransformation
{
public:
	struct InlierStar {
		//center of stars that are considered matched inliers
		double xc;
		double yc;
	};

	typedef std::vector<InlierStar> InlierVector;
	typedef Eigen::Matrix <double, 8, 1> E_Vector8d;
	typedef Eigen::Matrix <double, 8, 8> Matrix8d;
	

	std::vector<int> RandomPoints(int maxnum); //maxnum=tvgspvector.size()

	Eigen::Matrix3d Homography(const StarDetection::StarVector &refstarvector,const StarDetection::StarVector &tgtstarvector,const StarMatching::TVGSPVector &tvgspvector,const std::vector<int> randompoints);

	Eigen::Matrix3d FinalHomography(const InlierVector &final_ref_inlier,const InlierVector &final_tgt_inlier);

	Eigen::Matrix3d RANSAC(const StarDetection::StarVector &refstarvector ,const StarDetection::StarVector &tgtstarvector,const StarMatching::TVGSPVector &tvgspvector);
};

