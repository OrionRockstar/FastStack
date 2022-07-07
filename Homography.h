#pragma once
#include "StarDetection.h"
#include "StarMatching.h"
#include <eigen3/Eigen/Dense>

//class Homography
//{
//public:
struct InlierStar {
	//center of stars that are considered matched inliers
	double xc;
	double yc;
};

typedef std::vector<InlierStar> InlierVector;
typedef Eigen::Matrix <double, 8, 1> E_Vector8d;
typedef Eigen::Matrix <double, 8, 8> Matrix8d;
	
namespace homography {
	std::vector<int> RandomPoints(int maxnum); //maxnum=tvgspvector.size()

	Eigen::Matrix3d ComputeHomography(const StarVector& refstarvector, const StarVector& tgtstarvector, const TVGSPVector& tvgspvector, const std::vector<int> randompoints);

	Eigen::Matrix3d ComputeFinalHomography(const InlierVector& final_ref_inlier, const InlierVector& final_tgt_inlier);

	Eigen::Matrix3d RANSAC(const StarVector& refstarvector, const StarVector& tgtstarvector, const TVGSPVector& tvgspvector);
}
//};


