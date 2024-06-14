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

        randint.emplace_back(randnum);

    }

    return randint;
}

void Homography::InitialHomography(const StarPairVector& spv) {

    std::vector<int> randints = RNVG(spv.size());

    Matrix points(8, 8);
    Matrix tgtvec(8, 1);

    for (int i = 0; i < 4; i++) {
        double rx = spv[randints[i]].rxc;
        double ry = spv[randints[i]].ryc;
        double tx = spv[randints[i]].txc;
        double ty = spv[randints[i]].tyc;

        points.ModifyRow(2 * i, 0, { rx, ry, 1, 0, 0, 0, -rx * tx, -ry * tx });
        points.ModifyRow(2 * i + 1, 0, { 0, 0, 0, rx, ry, 1, -rx * ty, -ry * ty });
        tgtvec.ModifyVector(2 * i, { tx,ty });
    }

    Matrix homovec = Matrix::LeastSquares(points, tgtvec);

    m_homography = { homovec[0], homovec[1], homovec[2], homovec[3], homovec[4], homovec[5], homovec[6], homovec[7], 1 };
    //return Matrix(3, 3, { homovec[0], homovec[1], homovec[2], homovec[3], homovec[4], homovec[5], homovec[6], homovec[7], 1 });
}

void Homography::FinalHomography(const StarPairVector& spv) {

    Matrix points(2 * spv.size(), 8);
    Matrix tgtvec(2 * spv.size());

    for (int i = 0; i < spv.size(); i++) {
        double rx = spv[i].rxc;
        double ry = spv[i].ryc;
        double tx = spv[i].txc;
        double ty = spv[i].tyc;

        points.ModifyRow(2 * i, 0, { rx, ry, 1, 0, 0, 0, -rx * tx, -ry * tx });
        points.ModifyRow(2 * i + 1, 0, { 0, 0, 0, rx, ry, 1, -rx * ty, -ry * ty });
        tgtvec.ModifyVector(2 * i, { tx,ty });
    }

    Matrix homovec = Matrix::LeastSquares(points, tgtvec);

    m_homography = { homovec[0], homovec[1], homovec[2], homovec[3], homovec[4], homovec[5], homovec[6], homovec[7], 1 };

}

Matrix Homography::ComputeHomography(const StarPairVector& spv) {

    StarPairVector inliers;
    inliers.reserve(spv.size());

    StarPairVector final_inliers;

    int tvgtotal = int(spv.size()),
        maxmatch = 0;
    //le& slider 1.0-5.0
    double tol = 2;

    srand(time(NULL));

    for (int iter = 0; iter < 250; iter++) {
        int match = 0;
        inliers.clear();

        InitialHomography(spv);

        for (const auto& sp : spv) {
            Matrix ref_pts(3, 1, { sp.rxc, sp.ryc, 1 });
            Matrix pred_pts = m_homography * ref_pts;
            if (Distance(pred_pts[0] / pred_pts[2], pred_pts[1] / pred_pts[2], sp.txc, sp.tyc) <= tol) {
                inliers.emplace_back(sp.rxc, sp.ryc, sp.txc, sp.tyc);
                match++;
            }
        }

        if (match > maxmatch) {
            maxmatch = match;
            final_inliers = inliers;
            if (maxmatch >= .98 * tvgtotal) break;
        }
    }

    if (maxmatch < .25 * tvgtotal) {
        m_homography.Fill(std::numeric_limits<double>::quiet_NaN());
        return m_homography;
    }

    FinalHomography(final_inliers);

    return m_homography;
}