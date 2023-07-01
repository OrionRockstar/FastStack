#include "StarMatching.h"
#include "Maths.h"
#include <array>
#include "omp.h"

StarMatching::TriangleVector StarMatching::TriangleComputation(const StarVector& star_vector)
{
    int nstars = (int)star_vector.size();
    int maxtri = ((nstars * (nstars - 1) * (nstars - 2)) / 6);
    std::array<float, 3>sides;
    TriangleVector tri;// (maxtri);
    tri.reserve(maxtri);

    for (int sa = 0; sa < nstars; ++sa) {
        for (int sb = sa + 1; sb < nstars; ++sb) {
            float side_ab = Distance(star_vector[sa].xc, star_vector[sa].yc, star_vector[sb].xc, star_vector[sb].yc);
            for (int sc = sb + 1; sc < nstars; ++sc) {
                sides[0] = side_ab;
                sides[1] = Distance(star_vector[sa].xc, star_vector[sa].yc, star_vector[sc].xc, star_vector[sc].yc);
                sides[2] = Distance(star_vector[sb].xc, star_vector[sb].yc, star_vector[sc].xc, star_vector[sc].yc);

                for (int j = 0; j < 2; j++) {
                    for (int i = 2; i > 0; i--) {
                        if (sides[j] > sides[i])
                            std::swap(sides[i], sides[j]);
                    }
                }

                if (sides[2] != 0)
                    if (sides[1] / sides[2] < .9)
                        tri.emplace_back(sides[1] / sides[2], sides[0] / sides[2], sa, sb, sc);

            }
        }
    }

    std::sort(tri.begin(), tri.end(), Triangle());

    return tri;
}

StarPairVector StarMatching::GetMatchedPairsCentroids(const std::vector<Star>& refstarvec, const std::vector<Star>& tgtstarvec, const std::vector<TVGStar>& tvgvec) {

    StarPairVector spv(tvgvec.size());

    for (int i = 0; i < spv.size(); ++i) {
        spv[i].rxc = refstarvec[tvgvec[i].refstarnum].xc;
        spv[i].ryc = refstarvec[tvgvec[i].refstarnum].yc;
        spv[i].txc = tgtstarvec[tvgvec[i].tgtstarnum].xc;
        spv[i].tyc = tgtstarvec[tvgvec[i].tgtstarnum].yc;
    }

    return spv;
}

StarPairVector StarMatching::MatchStars(const std::vector<Triangle>& reftri, StarVector& refstars, StarVector& tgtstars) {

    TriangleVector tgttri = TriangleComputation(tgtstars);

    PotentialStarPairs psp;

    float tol = 0.0002f;
    int lref = 0;
#pragma omp parallel for firstprivate(lref) schedule(static,tgttri.size()/omp_get_max_threads())
    for (int itgt = 0; itgt < tgttri.size(); ++itgt) {

        while (lref < reftri.size() && tgttri[itgt].rx > reftri[lref].rx + tol) lref++;

        int iref = lref;

        while (iref < reftri.size() && reftri[iref].rx < tgttri[itgt].rx + tol) {
            if (Distance(tgttri[itgt].rx, tgttri[itgt].ry, reftri[iref].rx, reftri[iref].ry) <= tol) {
                psp.AddVote(tgttri[itgt].star1, reftri[iref].star1);
                psp.AddVote(tgttri[itgt].star1, reftri[iref].star2);
                psp.AddVote(tgttri[itgt].star1, reftri[iref].star3);
                psp.AddVote(tgttri[itgt].star2, reftri[iref].star1);
                psp.AddVote(tgttri[itgt].star2, reftri[iref].star2);
                psp.AddVote(tgttri[itgt].star2, reftri[iref].star3);
                psp.AddVote(tgttri[itgt].star3, reftri[iref].star1);
                psp.AddVote(tgttri[itgt].star3, reftri[iref].star2);
                psp.AddVote(tgttri[itgt].star3, reftri[iref].star3);
            }
            iref++;
        }
    }

    std::vector<TVGStar> tvgvec;
    int max_star_pair = 200;
    tvgvec.reserve(max_star_pair);

    int thresh = psp.Threshold();
    //move to own function???
    for (int iter = 0; iter <= max_star_pair; ++iter) {

        int maxv = thresh;
        int tgtemp, rtemp;

        for (int tgt = 0; tgt < max_star_pair; ++tgt) {
            for (int ref = 0; ref < max_star_pair; ++ref) {
                if (psp(tgt, ref) > maxv) {
                    maxv = psp(tgt, ref);
                    tgtemp = tgt;
                    rtemp = ref;
                }
            }
        }

        bool new_star_pair = true;

        for (auto& i : tvgvec)
            if (i.tgtstarnum == tgtemp || i.refstarnum == rtemp) {
                psp(tgtemp, rtemp) = 0;
                new_star_pair = false;
                break;
            }

        if (new_star_pair) {
            tvgvec.emplace_back(tgtemp, rtemp);
            psp(tgtemp, rtemp) = 0;
        }

    }

    return GetMatchedPairsCentroids(refstars, tgtstars, tvgvec);

}

StarPairVector StarMatching::MatchStars(const StarVector& refstars, const StarVector& tgtstars) {

    if (m_reftri.size() == 0)
        m_reftri = TriangleComputation(refstars);

    m_tgttri = TriangleComputation(tgtstars);

    PotentialStarPairs psp;

    float tol = 0.0002f;
    int lref = 0;
#pragma omp parallel for firstprivate(lref) schedule(static,m_tgttri.size()/omp_get_max_threads())
    for (int itgt = 0; itgt < m_tgttri.size(); ++itgt) {

        while (lref < m_reftri.size() && m_tgttri[itgt].rx > m_reftri[lref].rx + tol) lref++;

        int iref = lref;

        while (iref < m_reftri.size() && m_reftri[iref].rx < m_tgttri[itgt].rx + tol) {
            if (Distance(m_tgttri[itgt].rx, m_tgttri[itgt].ry, m_reftri[iref].rx, m_reftri[iref].ry) <= tol) {
                psp.AddVote(m_tgttri[itgt].star1, m_reftri[iref].star1);
                psp.AddVote(m_tgttri[itgt].star1, m_reftri[iref].star2);
                psp.AddVote(m_tgttri[itgt].star1, m_reftri[iref].star3);
                psp.AddVote(m_tgttri[itgt].star2, m_reftri[iref].star1);
                psp.AddVote(m_tgttri[itgt].star2, m_reftri[iref].star2);
                psp.AddVote(m_tgttri[itgt].star2, m_reftri[iref].star3);
                psp.AddVote(m_tgttri[itgt].star3, m_reftri[iref].star1);
                psp.AddVote(m_tgttri[itgt].star3, m_reftri[iref].star2);
                psp.AddVote(m_tgttri[itgt].star3, m_reftri[iref].star3);
            }
            iref++;
        }
    }

    std::vector<TVGStar> tvgvec;
    int max_star_pair = 200;
    tvgvec.reserve(max_star_pair);

    int thresh = psp.Threshold();
    //move to own function???
    for (int iter = 0; iter <= max_star_pair; ++iter) {

        int maxv = thresh;
        int tgtemp, rtemp;

        for (int tgt = 0; tgt < max_star_pair; ++tgt) {
            for (int ref = 0; ref < max_star_pair; ++ref) {
                if (psp(tgt, ref) > maxv) {
                    maxv = psp(tgt, ref);
                    tgtemp = tgt;
                    rtemp = ref;
                }
            }
        }

        bool new_star_pair = true;

        for (auto& i : tvgvec)
            if (i.tgtstarnum == tgtemp || i.refstarnum == rtemp) {
                psp(tgtemp, rtemp) = 0;
                new_star_pair = false;
                break;
            }

        if (new_star_pair) {
            tvgvec.emplace_back(tgtemp, rtemp);
            psp(tgtemp, rtemp) = 0;
        }

    }

    return GetMatchedPairsCentroids(refstars, tgtstars, tvgvec);

}
