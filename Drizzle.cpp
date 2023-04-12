#include "Drizzle.h"

static float Median(const std::vector<float>& pixstack) {
	std::vector<float> temp;
	temp.reserve(pixstack.size());
	for (const float& pix : pixstack)
		if (pix != -1)
			temp.emplace_back(pix);

	std::nth_element(temp.begin(), temp.begin() + temp.size() / 2, temp.end());

	return temp[temp.size() / 2];
}

static float StandardDeviation(const std::vector<float>& pixstack) {
	float mean = 0;
	for (const float& pixel : pixstack)
		if (pixel != -1)
			mean += pixel;

	mean /= pixstack.size();

	double d;
	double var = 0;
	for (const float& pixel : pixstack)
		if (pixel != -1) {
			d = pixel - mean;
			var += d * d;
		}

	return (float)sqrt(var / pixstack.size());
}

void Drizzle::SigmaClipWM(ImageVector& imgvec, float l_sigma, float u_sigma) {

	std::vector<float> pixelstack(imgvec.size());

	int ch = 0;
	int col = imgvec[0].Cols();

#pragma omp parallel for firstprivate(pixelstack)
	for (int y = 0; y < imgvec[0].Rows(); ++y) {

		for (int x = 0; x < imgvec[0].Cols(); ++x) {

			for (size_t i = 0; i < pixelstack.size(); ++i) {
				double x_s = x * imgvec[i].homography(0, 0) + y * imgvec[i].homography(0, 1) + imgvec[i].homography(0, 2);
				double y_s = x * imgvec[i].homography(1, 0) + y * imgvec[i].homography(1, 1) + imgvec[i].homography(1, 2);

				pixelstack[i] = Interpolator<Image32>::InterpolatePixel(imgvec[i], x_s, y_s, ch);

			}

			float old_stddev = 0;
			for (int iter = 0; iter < 5; iter++) {

				float stddev = StandardDeviation(pixelstack);
				if ((old_stddev - stddev) == 0 || stddev == 0)  break;

				float median = Median(pixelstack);

				float upper = median + u_sigma * stddev;
				float lower = median - l_sigma * stddev;

				for (int i = 0; i < pixelstack.size(); ++i) {
					if (pixelstack[i] < lower || upper < pixelstack[i]) {
						int x_s = floor(x * imgvec[i].homography(0, 0) + y * imgvec[i].homography(0, 1) + imgvec[i].homography(0, 2));
						int y_s = floor(x * imgvec[i].homography(1, 0) + y * imgvec[i].homography(1, 1) + imgvec[i].homography(1, 2));

						if (imgvec[i].IsInBounds(x_s, y_s))
							imgvec[i].Set_Weight_At(x_s, y_s, false);

						pixelstack[i] = -1;
						//else
							//imgvec[i].Set_Weight_At(x, y, false);
					}
				}
				old_stddev = stddev;
			}
		}
	}
}

float Drizzle::DrizzlePix(float inp, float out, float area, float s2, int pix_weight, int out_weight) {
	if (out == 0)
		out_weight = 0;

	float dw = area * pix_weight + out_weight;
	return (dw != 0) ? ((inp * area * pix_weight * s2) + (out * out_weight)) / dw : out; // more correct
}

void Drizzle::DrizzleFrame(Image32& input, Image32& output, float drop) {

	Eigen::Matrix3d homography = Eigen::Inverse(input.homography);
	int oweight = 1;// (homography == Eigen::Matrix3d::Identity()) ? 0 : 1;

	float s2 = drop * drop,
		offset = (1 - drop) / 2,
		x2drop = 2 * drop,
		drop_area = x2drop * x2drop;

#pragma omp parallel for
	for (int y = 0; y < input.Rows(); ++y) {

		double yx = y * homography(0, 1);
		double yy = y * homography(1, 1);

		for (int x = 0; x < input.Cols(); ++x) {
			double x_s = x * homography(0, 0) + yx + homography(0, 2) + offset;
			double y_s = x * homography(1, 0) + yy + homography(1, 2) + offset;

			int x_f = (int)floor(2 * x_s);
			int y_f = (int)floor(2 * y_s);

			if (x_f < 0 || x_f >= output.Cols() - 2 || y_f < 0 || y_f >= output.Rows() - 2) continue;

			float vx = (1 - ((2 * (x_s)) - x_f));
			float vy = (1 - ((2 * (y_s)) - y_f));

			std::array<float, 9> area = { 0 };

			if (x2drop >= vx && x2drop >= vy)
				area[0] = (vx * vy) / drop_area;
			else if (x2drop < vx && x2drop > vy)
				area[0] = ((x2drop)*vy) / drop_area;
			else if (x2drop > vx && x2drop < vy)
				area[0] = (vx * (x2drop)) / drop_area;
			else
				area[0] = ((x2drop) * (x2drop)) / drop_area;

			if (x2drop >= vx + 1 && x2drop > vy)
				area[1] = vy / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop>vy)
				area[1] = (vy * (x2drop - vx)) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop < vy)
				area[1] = (x2drop * (x2drop - vx)) / drop_area;

			if (x2drop > vx + 1)
				area[2] = (vy * (x2drop - vx - 1)) / drop_area;

			if (x2drop >= vy + 1)
				area[3] = vx / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx)
				area[3] = (vx * (x2drop - vy)) / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop < vx)
				area[3] = ((x2drop) * (x2drop - vy)) / drop_area;

			if (x2drop >= vx + 1 && x2drop >= vy + 1)
				area[4] = 1 / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop >= vx + 1)
				area[4] = (x2drop - vy) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop >= vy + 1)
				area[4] = (x2drop - vx) / drop_area;
			else if ((x2drop < vx + 1 && x2drop >= vx) && (x2drop < vy + 1 && x2drop >= vy))
				area[4] = ((x2drop - vx) * (x2drop - vy)) / drop_area;

			if (x2drop > vx + 1 && x2drop > vy + 1)
				area[5] = (x2drop - vx - 1) / drop_area;
			else if (x2drop < vy + 1 && x2drop >= vy && x2drop > vx + 1)
				area[5] = ((x2drop - vy) * (x2drop - vx - 1)) / drop_area;

			if (x2drop > vy + 1)
				area[6] = (vx * (x2drop - vy - 1)) / drop_area;

			if (x2drop > vx + 1 && x2drop > vy + 1)
				area[7] = (x2drop - vy - 1) / drop_area;
			else if (x2drop < vx + 1 && x2drop >= vx && x2drop > vy + 1)
				area[7] = ((x2drop - vy - 1) * (x2drop - vx)) / drop_area;

			if (x2drop > vy + 1 && x2drop > vx + 1)
				area[8] = ((x2drop - vy - 1) * (x2drop - vx - 1)) / drop_area;

			int pix_weight = input.Weight_At(x, y);
			float pixel = input(x, y);

			output(x_f, y_f) = DrizzlePix(pixel, output(x_f, y_f), area[0], s2, pix_weight, oweight);
			output(x_f + 1, y_f) = DrizzlePix(pixel, output(x_f + 1, y_f), area[1], s2, pix_weight, oweight);
			output(x_f + 2, y_f) = DrizzlePix(pixel, output(x_f + 2, y_f), area[2], s2, pix_weight, oweight);
			output(x_f, y_f + 1) = DrizzlePix(pixel, output(x_f, y_f + 1), area[3], s2, pix_weight, oweight);
			output(x_f + 1, y_f + 1) = DrizzlePix(pixel, output(x_f + 1, y_f + 1), area[4], s2, pix_weight, oweight);
			output(x_f + 2, y_f + 1) = DrizzlePix(pixel, output(x_f + 2, y_f + 1), area[5], s2, pix_weight, oweight);
			output(x_f, y_f + 2) = DrizzlePix(pixel, output(x_f, y_f + 2), area[6], s2, pix_weight, oweight);
			output(x_f + 1, y_f + 2) = DrizzlePix(pixel, output(x_f + 1, y_f + 2), area[7], s2, pix_weight, oweight);
			output(x_f + 2, y_f + 2) = DrizzlePix(pixel, output(x_f + 2, y_f + 2), area[8], s2, pix_weight, oweight);
		}
	}

}

void Drizzle::DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_est, bool reject) {
	ImageVector img_stack;

	GetImageStackFromTemp(light_files, img_stack);

	ImageOP::ScaleImageStack(img_stack, scale_est);

	if (reject)
		SigmaClipWM(img_stack, 4, 3);

	output = Image32(img_stack[0].Rows() * 2, img_stack[0].Cols() * 2);
	output.FillZero();

	for (auto& img : img_stack)
		DrizzleFrame(img, output, drop_size);

	output.ComputeStatistics();
}
