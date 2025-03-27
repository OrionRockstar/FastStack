#pragma once
#include "Matrix.h"
#include "Star.h"

class Homography {

	//random number vector generator
	static std::vector<int> RNVG(int max_num);

	static Matrix computeInitialHomography(const StarPairVector& spv);

	static Matrix computeFinalHomography(const StarPairVector& spv);

public:
	static Matrix computeHomography(const StarPairVector& starpairs);
};
