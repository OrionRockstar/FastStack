#include "StarMatching.h"

using SM = StarMatching;

SM::TriangleVector StarMatching::TrianglesComputation(StarDetection::StarVector starvector) {
    int count = 0;
    double side_ab = 0;
    int mm = (int)starvector.size();
    int maxtri = int(((mm * (mm - 1) * (mm - 2)) / 6));
    std::vector<double>sides(3);

    SM::TriangleVector trianglevector(maxtri);

    for (int sa = 0; sa < mm; ++sa) {
        for (int sb = sa + 1; sb < mm; ++sb) {
            side_ab = Distance(starvector[sa].xc, starvector[sa].yc, starvector[sb].xc, starvector[sb].yc);
            for (int sc = sb + 1; sc < mm; ++sc) {
                sides[0] = side_ab;
                sides[1] = Distance(starvector[sa].xc, starvector[sa].yc, starvector[sc].xc, starvector[sc].yc);
                sides[2] = Distance(starvector[sb].xc, starvector[sb].yc, starvector[sc].xc, starvector[sc].yc);

                for (size_t j = 0; j < 2; j++) {
                    for (size_t i = 2; i > 0; i--) {
                        if (sides[j] > sides[i])
                            std::swap(sides[i], sides[j]);
                    }
                }
                if (sides[2] != 0)
                    if (sides[1] / sides[2] < .9) {
                        trianglevector[count] = { sides[1] / sides[2],sides[0] / sides[2],sa,sb,sc };
                        count++;
                    }
            }
        }
    }

    trianglevector.resize(count);

    std::sort(trianglevector.begin(), trianglevector.end(), Triangle());

    return trianglevector;
}

SM::TVGSPVector SM::MatchStars(SM::TriangleVector reftri, SM::TriangleVector tgttri,int psprow,int pspcol) {

    std::vector<unsigned short> psp(psprow*pspcol);

    auto vote = [&psp,pspcol](int target, int reference) {return psp[target * pspcol + reference]++; };
    double tol = 0.0002;

    int lref = 0, iref;

#pragma omp parallel for firstprivate(lref) private(iref)
    for (int itgt = 0; itgt < tgttri.size(); ++itgt) {
        while (lref < reftri.size() && tgttri[itgt].rx > reftri[lref].rx + tol) lref++;

        iref = lref;

        while (iref < reftri.size() && reftri[iref].rx < tgttri[itgt].rx + tol) {
            if (Distance(tgttri[itgt].rx, tgttri[itgt].ry, reftri[iref].rx, reftri[iref].ry) <= tol) {
                vote(tgttri[itgt].star1, reftri[iref].star1);
                vote(tgttri[itgt].star1, reftri[iref].star2);
                vote(tgttri[itgt].star1, reftri[iref].star3);
                vote(tgttri[itgt].star2, reftri[iref].star1);
                vote(tgttri[itgt].star2, reftri[iref].star2);
                vote(tgttri[itgt].star2, reftri[iref].star3);
                vote(tgttri[itgt].star3, reftri[iref].star1);
                vote(tgttri[itgt].star3, reftri[iref].star2);
                vote(tgttri[itgt].star3, reftri[iref].star3);
            }
            iref++;
        }
    }

    SM::TVGSPVector tvgspvector;

    int tcount = 0, tgtemp = 0, rtemp = 0, thresh=Median(&psp[0],psp.size())+StandardDeviation(&psp[0],psp.size()), maxv = thresh + 1;
    bool spe = false;

    while (tcount<pspcol && maxv>thresh) {
        maxv = thresh;
        for (int tgt = 0; tgt < pspcol; ++tgt) {
            for (int ref = 0; ref < pspcol; ++ref) {
                if (psp[tgt * pspcol + ref] > maxv) {
                    maxv = psp[tgt * pspcol + ref];
                    tgtemp = tgt;
                    rtemp = ref;
                }
            }
        }

        for (auto starpair : tvgspvector) {
            if (starpair.tgtstar == tgtemp || starpair.refstar == rtemp) {
                spe = true;
                break;
            }
        }
        if (spe) {
            spe = false;
            psp[tgtemp * pspcol + rtemp] = 0;
        }
        else {
            tvgspvector.push_back({ tgtemp,rtemp });
            psp[tgtemp * pspcol + rtemp] = 0;
            tcount++;

        }
    }
    return tvgspvector;
}