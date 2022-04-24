#pragma once
#include "StarDetection.h"
#include "StarMatching.h"
#include <Eigen/Dense>

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

	void RanomPoints();
	void Homography();
	void FinalHomography();
	void RANSAC();
};

