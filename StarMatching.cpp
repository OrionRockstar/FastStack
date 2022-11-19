#include "StarMatching.h"
#include <array>

static float Distance(float x1, float y1, float x2, float y2) { return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

static int Median(std::vector<int>& psp) {
    std::vector<int> pspbuf(psp.size());
    std::copy(psp.begin(), psp.end(), pspbuf.begin());

    std::nth_element(pspbuf.begin(), pspbuf.begin()+pspbuf.size()/2, pspbuf.end());
    return pspbuf[pspbuf.size()/ 2];
}

static int StandardDeviation(std::vector<int>& psp) {
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
    return (int)sqrt(var / psp.size());
}

TriangleVector starmatching::TrianglesComputation(const StarVector& starvector) {
    
    uint8_t nstars = (uint8_t)starvector.size();
    int maxtri = int(((nstars * (nstars - 1) * (nstars - 2)) / 6));
    std::array<float, 3> sides;

    TriangleVector trianglevector;
    trianglevector.reserve(maxtri);

    for (uint8_t sa = 0; sa < nstars; ++sa) {
        for (uint8_t sb = sa + 1; sb < nstars; ++sb) {
            float side_ab = Distance(starvector[sa].xc, starvector[sa].yc, starvector[sb].xc, starvector[sb].yc);
            for (uint8_t sc = sb + 1; sc < nstars; ++sc) {
                sides[0] = side_ab;
                sides[1] = Distance(starvector[sa].xc, starvector[sa].yc, starvector[sc].xc, starvector[sc].yc);
                sides[2] = Distance(starvector[sb].xc, starvector[sb].yc, starvector[sc].xc, starvector[sc].yc);

                for (int j = 0; j < 2; j++) {
                    for (int i = 2; i > 0; i--) {
                        if (sides[j] > sides[i])
                            std::swap(sides[i], sides[j]);
                    }
                }

                if (sides[2] != 0)
                    if (sides[1] / sides[2] < .9)
                        trianglevector.emplace_back(sides[1] / sides[2], sides[0] / sides[2],sa,sb,sc);

            }
        }
    }

    std::sort(trianglevector.begin(), trianglevector.end(), Triangle());

    return trianglevector;
}

TVGSPVector starmatching::MatchStars(const TriangleVector& reftri,const TriangleVector& tgttri,int psprow,int pspcol) {

    std::vector<int> psp(psprow*pspcol, 0);

    auto vote = [&psp,pspcol](auto target, auto reference) {return psp[reference*pspcol + target]++; };
    float tol = 0.0002f;

    int lref = 0;

#pragma omp parallel for firstprivate(lref)
    for (int itgt = 0; itgt < tgttri.size(); ++itgt) {
        while (lref < reftri.size() && tgttri[itgt].rx > reftri[lref].rx + tol) lref++;

        int iref = lref;

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

    TVGSPVector tvgspvector;

    int tcount = 0, tgtemp = 0, rtemp = 0, thresh=Median(psp)+StandardDeviation(psp), maxv = thresh + 1;

    newsp:
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
                psp[rtemp * pspcol + tgtemp] = 0;
                goto newsp;
            }
        }

        tvgspvector.push_back({ tgtemp,rtemp });
        psp[rtemp * pspcol + tgtemp] = 0;
        tcount++;
    }

    return tvgspvector;
}