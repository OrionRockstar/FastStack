#pragma once
#include "Matrix.h"
#include "Star.h"

class Homography
{
	Matrix m_homography = Matrix(3, 3);

	//random number vector generator
	std::vector<int> RNVG(int max_num);

	void InitialHomography(const StarPairVector& spv);

	void FinalHomography(const StarPairVector& spv);

public:
	Matrix ComputeHomography(const StarPairVector& starpairs);
};
