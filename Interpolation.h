#pragma once
#include"Image.h"

namespace Interpolation {
	float Bilinear(Image32& img, double& x_s, double& y_s, int& channel);

	float Catmull_Rom(Image32& img, double& x_s, double& y_s, int& channel);

	float Bicubic_Spline(Image32& img, double& x_s, double& y_s, int& channel);

	float Bicubic_B_Spline(Image32& img, double& x_s, double& y_s, int& channel);
}