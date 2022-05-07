#include "CoordinateTransformation.h"
//#include <eigen3/Eigen/Dense
using CT = CoordinateTransformation;
using SD = StarDetection;
using SM = StarMatching;

std::vector<int> CT::RandomPoints(int maxnum) {
    int randnum = 0;
    std::vector<int> randint;
    bool newnum = false;
    while (randint.size() < 4) {
        randnum = rand() % maxnum;
        for (int i : randint) {
            if (randnum == i) {
                newnum = true;
                break;
            }
        }
        if (newnum) {
            newnum = false;
            continue;
        }
        else {
            randint.push_back(randnum);
        }
    }
    return randint;
}

Eigen::Matrix3d CT::Homography(SD::StarVector refstarvector, SD::StarVector tgtstarvector, SM::TVGSPVector tvgspvector, std::vector<int> randompoints) {
    
    CT::Matrix8d matrix(8, 8);
    CT::E_Vector8d tgtmat;
    CT::E_Vector8d homovec = CT::E_Vector8d::Zero();
    Eigen::Matrix3d homography = Eigen::Matrix3d::Ones();

    double rx1 = refstarvector[tvgspvector[randompoints[0]].refstar].xc,
        rx2 = refstarvector[tvgspvector[randompoints[1]].refstar].xc,
        rx3 = refstarvector[tvgspvector[randompoints[2]].refstar].xc,
        rx4 = refstarvector[tvgspvector[randompoints[3]].refstar].xc,
        ry1 = refstarvector[tvgspvector[randompoints[0]].refstar].yc,
        ry2 = refstarvector[tvgspvector[randompoints[1]].refstar].yc,
        ry3 = refstarvector[tvgspvector[randompoints[2]].refstar].yc,
        ry4 = refstarvector[tvgspvector[randompoints[3]].refstar].yc,
        tx1 = tgtstarvector[tvgspvector[randompoints[0]].tgtstar].xc,
        tx2 = tgtstarvector[tvgspvector[randompoints[1]].tgtstar].xc,
        tx3 = tgtstarvector[tvgspvector[randompoints[2]].tgtstar].xc,
        tx4 = tgtstarvector[tvgspvector[randompoints[3]].tgtstar].xc,
        ty1 = tgtstarvector[tvgspvector[randompoints[0]].tgtstar].yc,
        ty2 = tgtstarvector[tvgspvector[randompoints[1]].tgtstar].yc,
        ty3 = tgtstarvector[tvgspvector[randompoints[2]].tgtstar].yc,
        ty4 = tgtstarvector[tvgspvector[randompoints[3]].tgtstar].yc;

    matrix << rx1, ry1, 1, 0, 0, 0, -rx1 * tx1, -ry1 * tx1,
        0, 0, 0, rx1, ry1, 1, -rx1 * ty1, -ry1 * ty1,
        rx2, ry2, 1, 0, 0, 0, -rx2 * tx2, -ry2 * tx2,
        0, 0, 0, rx2, ry2, 1, -rx2 * ty2, -ry2 * ty2,
        rx3, ry3, 1, 0, 0, 0, -rx3 * tx3, -ry3 * tx3,
        0, 0, 0, rx3, ry3, 1, -rx3 * ty3, -ry3 * ty3,
        rx4, ry4, 1, 0, 0, 0, -rx4 * tx4, -ry4 * tx4,
        0, 0, 0, rx4, ry4, 1, -rx4 * ty4, -ry4 * ty4;

    tgtmat << tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4;
    homovec = Eigen::Inverse(Eigen::Transpose(matrix) * matrix) * (Eigen::Transpose(matrix) * tgtmat);
    homography << homovec(0), homovec(1), homovec(2), homovec(3), homovec(4), homovec(5), homovec(6), homovec(7);
    return homography;
}

Eigen::Matrix3d CT::FinalHomography(CT::InlierVector final_ref_inlier, CT::InlierVector final_tgt_inlier) {

    Eigen::Matrix <double, Eigen::Dynamic, 8> matrix(2 * final_ref_inlier.size(), 8);
    Eigen::Matrix <double, Eigen::Dynamic, 1> tgtmat(2 * final_ref_inlier.size(), 1);
    CT::E_Vector8d homovec = CT::E_Vector8d::Zero();
    Eigen::Matrix3d homography = Eigen::Matrix3d::Ones();
    double rx, ry, tx, ty;

    for (int i = 0; i < final_ref_inlier.size(); i++) {
        rx = final_ref_inlier[i].xc;
        ry = final_ref_inlier[i].yc;
        tx = final_tgt_inlier[i].xc;
        ty = final_tgt_inlier[i].yc;
        matrix.block(2 * i, 0, 2, 8) <<
            rx, ry, 1, 0, 0, 0, -rx * tx, -ry * tx,
            0, 0, 0, rx, ry, 1, -rx * ty, -ry * ty;
        tgtmat.block(2 * i, 0, 2, 1) << tx, ty;
    }

    homovec = Eigen::Inverse((Eigen::Transpose(matrix) * matrix)) * (Eigen::Transpose(matrix) * tgtmat);
    homography << homovec(0), homovec(1), homovec(2), homovec(3), homovec(4), homovec(5), homovec(6), homovec(7);

    return homography;
}

Eigen::Matrix3d CT::RANSAC(SD::StarVector refstarvector, SD::StarVector tgtstarvector, SM::TVGSPVector tvgspvector){
    std::vector<int>randints;
    Eigen::Matrix3d homography;
    Eigen::Matrix3d finalhomography;

    CT::InlierVector ref_inlier, final_ref_inlier, tgt_inlier, final_tgt_inlier;

    Eigen::Vector3d pred_pts;
    Eigen::Vector3d ref_pts = Eigen::Vector3d::Ones();

    int match = 0,
        tvgsptotal = int(tvgspvector.size()),
        maxmatch = 0;
    double tol = 2;
    srand(time(NULL));

    for (int iter = 0; iter < 150; iter++) {
        match = 0;
        ref_inlier.clear();
        tgt_inlier.clear();
        randints = CT::RandomPoints(tvgsptotal);

        homography = Homography(refstarvector, tgtstarvector, tvgspvector, randints);

        for (auto starnum : tvgspvector) {
            ref_pts << refstarvector[starnum.refstar].xc, refstarvector[starnum.refstar].yc;
            pred_pts = homography * ref_pts;
            if (Distance(pred_pts(0) / pred_pts(2), pred_pts(1) / pred_pts(2), tgtstarvector[starnum.tgtstar].xc, tgtstarvector[starnum.tgtstar].yc) <= tol) {
                ref_inlier.push_back({ refstarvector[starnum.refstar].xc,refstarvector[starnum.refstar].yc });
                tgt_inlier.push_back({ tgtstarvector[starnum.tgtstar].xc,tgtstarvector[starnum.tgtstar].yc });
                match++;
            }
        }
        if (match > maxmatch) {
            maxmatch = match;
            final_ref_inlier = ref_inlier;
            final_tgt_inlier = tgt_inlier;
            if (maxmatch >= .98 * tvgsptotal) { break; }
        }
    }

    if (maxmatch < .25*tvgsptotal) 
        return Eigen::Matrix3d::Constant(std::numeric_limits<double>::quiet_NaN());

    finalhomography = FinalHomography(final_ref_inlier, final_tgt_inlier);

    return finalhomography;
}