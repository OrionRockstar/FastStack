#pragma once
#include"Image.h"

namespace Interpolation {
	float Bilinear(Image32& img, double& x_s, double& y_s);

	float Catmull_Rom_V1(Image32& img, double& x_s, double& y_s);

	float Catmull_Rom_V2(Image32& img, double& x_s, double& y_s);

	float Bicubic_Spline(Image32& img, double& x_s, double& y_s);

	float Bicubic_B_Spline(Image32& img, double& x_s, double& y_s);

}