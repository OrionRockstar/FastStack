#include "StarMatching.h"

using SM = StarMatching;

static double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

static int Median(std::vector<int>& psp) {
    std::vector<int> pspbuf(psp.size());
    std::copy(psp.begin(), psp.end(), pspbuf.begin());

    std::nth_element(pspbuf.begin(), pspbuf.begin()+pspbuf.size()/2, pspbuf.end());
    return pspbuf[pspbuf.size()/ 2];
}

static double StandardDeviation(std::vector<int>& psp) {
    int mean = 0;
    for (int& val : psp)
        mean += val;
    mean /= psp.size();
    int d;
    uint64_t var = 0;
    for (int& val : psp) {
        d = val - mean;
        var += d * d;
    }
    return sqrt(var / psp.size());
}

SM::TriangleVector StarMatching::TrianglesComputation(const StarDetection::StarVector& starvector) {
    int count = 0;
    unsigned char sa, sb, sc;
    double side_ab = 0;
    int mm = (int)starvector.size();
    int maxtri = int(((mm * (mm - 1) * (mm - 2)) / 6));
    std::vector<double>sides(3);

    SM::TriangleVector trianglevector(maxtri);

    for (sa = 0; sa < mm; ++sa) {
        for (sb = sa + 1; sb < mm; ++sb) {
            side_ab = Distance(starvector[sa].xc, starvector[sa].yc, starvector[sb].xc, starvector[sb].yc);
            for (sc = sb + 1; sc < mm; ++sc) {
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
                        trianglevector[count] = { float(sides[1] / sides[2]),float(sides[0] / sides[2]),sa,sb,sc };
                        count++;
                    }
            }
        }
    }

    trianglevector.resize(count);

    std::sort(trianglevector.begin(), trianglevector.end(), Triangle());

    return trianglevector;
}

SM::TVGSPVector SM::MatchStars(const SM::TriangleVector& reftri,const SM::TriangleVector& tgttri,int psprow,int pspcol) {

    std::vector<int> psp(psprow*pspcol);

    auto vote = [&psp,pspcol](auto target, auto reference) {return psp[reference*pspcol + target]++; };
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

    int tcount = 0, tgtemp = 0, rtemp = 0, thresh=Median(psp)+StandardDeviation(psp), maxv = thresh + 1;
    bool spe = false;

    while (tcount<200 && maxv>thresh) {
        maxv = thresh;
        for (int tgt = 0; tgt < pspcol; ++tgt) {
            for (int ref = 0; ref < psprow; ++ref) {
                if (psp[ref * pspcol + tgt] > maxv) {
                    maxv = psp[ref * pspcol + tgt];
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
            psp[rtemp * pspcol + tgtemp] = 0;
        }
        else {
            tvgspvector.push_back({ tgtemp,rtemp });
            psp[rtemp * pspcol + tgtemp] = 0;
            tcount++;

        }
    }
    return tvgspvector;
}