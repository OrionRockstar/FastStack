#include "pch.h"
#include "Homography.h"
#include "Maths.h"

std::vector<int> Homography::RNVG(int max_num) {

    std::vector<int> randint;
    randint.reserve(4);

    while (randint.size() < 4) {
    newrand:
        int randnum = rand() % max_num;
        for (int i : randint)
            if (randnum == i)
                goto newrand;

        randint.push_back(randnum);

    }

    return randint;
}

Matrix Homography::computeInitialHomography(const StarPairVector& spv) {

    std::vector<int> randints = RNVG(spv.size());

    Matrix points(8, 8);
    Matrix tgtvec(8, 1);

    for (int i = 0; i < 4; i++) {
        double rx = spv[randints[i]].rxc;
        double ry = spv[randints[i]].ryc;

        double tx = spv[randints[i]].txc;
        double ty = spv[randints[i]].tyc;

        int row = 2 * i;
        points.setRow(row, { rx, ry, 1.0, 0.0, 0.0, 0.0, -rx * tx, -ry * tx});
        tgtvec(row, 0) = tx;
        points.setRow(row + 1, { 0.0, 0.0, 0.0, rx, ry, 1.0, -rx * ty, -ry * ty});
        tgtvec(row + 1, 0) = ty;
    }

    Matrix homovec = Matrix::leastSquares(points, tgtvec);

    return Matrix(3, 3, { homovec[0], homovec[1], homovec[2], homovec[3], homovec[4], homovec[5], homovec[6], homovec[7], 1 });
}

Matrix Homography::computeFinalHomography(const StarPairVector& spv) {

    Matrix points(2 * spv.size(), 8);
    Matrix tgtvec(2 * spv.size());

    for (int i = 0; i < spv.size(); i++) {
        double rx = spv[i].rxc;
        double ry = spv[i].ryc;
        double tx = spv[i].txc;
        double ty = spv[i].tyc;


        int row = 2 * i;
        points.setRow(row, { rx, ry, 1.0, 0.0, 0.0, 0.0, -rx * tx, -ry * tx });
        tgtvec(row, 0) = tx;
        points.setRow(row + 1, { 0.0, 0.0, 0.0, rx, ry, 1.0, -rx * ty, -ry * ty });
        tgtvec(row + 1, 0) = ty;
    }

    Matrix homovec = Matrix::leastSquares(points, tgtvec);

    return Matrix(3, 3, { homovec[0], homovec[1], homovec[2], homovec[3], homovec[4], homovec[5], homovec[6], homovec[7], 1 });

}

Matrix Homography::computeHomography(const StarPairVector& spv) {

    Matrix homography = Matrix(3,3).identity();

    StarPairVector inliers;
    inliers.reserve(spv.size());

    StarPairVector final_inliers;

    int tvgtotal = int(spv.size());
    int maxmatch = 0;

    float tol = 2.0;

    srand(time(NULL));

    for (int iter = 0; iter < 500; iter++) {
        int match = 0;
        inliers.clear();

        homography = computeInitialHomography(spv);

        for (const auto& sp : spv) {
            Matrix ref_pts(3, 1, { sp.rxc, sp.ryc, 1 });
            Matrix pred_pts = homography * ref_pts;
            if (math::distancef(pred_pts[0] / pred_pts[2], pred_pts[1] / pred_pts[2], sp.txc, sp.tyc) <= tol) {
                inliers.emplace_back(sp.rxc, sp.ryc, sp.txc, sp.tyc);
                match++;
            }
        }

        if (match > maxmatch) {
            maxmatch = match;
            final_inliers = inliers;
            if (maxmatch >= 0.98 * tvgtotal) break;
        }
    }

    //std::cout << maxmatch << " " << spv.size() << "\n";
    if (maxmatch < .25 * tvgtotal) {
        homography.fill(std::numeric_limits<double>::quiet_NaN());
        return homography;
    }

    return computeFinalHomography(final_inliers);
}