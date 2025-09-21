#include "pch.h"
#include "StarMatching.h"
#include "Maths.h"
#include "FITS.h"


StarMatching::PotentialStarPairs::PotentialStarPairs(int target_size, int reference_size) : tgt_size(target_size), ref_size(reference_size) {
    data = std::vector<uint32_t>(target_size * reference_size);
}

std::vector<StarMatching::TVGStar> StarMatching::PotentialStarPairs::getTopVoteStars()const {

    std::vector<TVGStar> tvgvec;
    int max_star_pair = math::min(ref_size, tgt_size);
    tvgvec.reserve(max_star_pair);

    uint64_t mean = 0;
    for (auto vote : data)
        mean += vote;
    mean /= data.size();

    int d;
    uint64_t var = 0;
    for (auto vote : data) {
        d = vote - mean;
        var += d * d;
    }

    uint32_t thresh = mean + sqrt(var / data.size());

    for (int tgt = 0; tgt < max_star_pair; ++tgt) {
        for (int ref = 0; ref < max_star_pair; ++ref) {
            if ((*this)(tgt, ref) > thresh) {

                bool new_pair = true;
                for (auto& i : tvgvec) {
                    if (i.tgtstarnum == tgt || i.refstarnum == ref) {
                        if ((*this)(i.tgtstarnum, i.refstarnum) > (*this)(tgt, ref)) {
                            i = { tgt,ref };
                        }
                        new_pair = false;
                        break;
                    }
                }
                if (new_pair)
                    tvgvec.emplace_back(tgt, ref);
            }
        }
    }

    tvgvec.shrink_to_fit();
    return std::move(tvgvec);
}


StarMatching::TriangleVector StarMatching::computeTriangles(const StarVector& star_vector)const {

    int nstars = (int)star_vector.size();
    int maxtri = ((nstars * (nstars - 1) * (nstars - 2)) / 6);
    std::array<float, 3>sides;
    TriangleVector tri;// (maxtri);
    tri.reserve(maxtri);

    for (int sa = 0; sa < nstars; ++sa) {
        for (int sb = sa + 1; sb < nstars; ++sb) {
            float side_ab = math::distancef(star_vector[sa].xc, star_vector[sa].yc, star_vector[sb].xc, star_vector[sb].yc);
            for (int sc = sb + 1; sc < nstars; ++sc) {
                sides[0] = side_ab;
                sides[1] = math::distancef(star_vector[sa].xc, star_vector[sa].yc, star_vector[sc].xc, star_vector[sc].yc);
                sides[2] = math::distancef(star_vector[sb].xc, star_vector[sb].yc, star_vector[sc].xc, star_vector[sc].yc);

                //sorts sides, increasing length
                if (sides[0] > sides[1])
                    std::swap(sides[0], sides[1]);
                if (sides[1] > sides[2])
                    std::swap(sides[1], sides[2]);
                if (sides[0] > sides[1])
                    std::swap(sides[0], sides[1]);

                if (sides[2] > 0)
                    if (sides[1] / sides[2] < .9)
                        tri.emplace_back(sides[1] / sides[2], sides[0] / sides[2], sa, sb, sc);

            }
        }
    }

    std::sort(tri.begin(), tri.end(), Triangle());
    return tri;
}

StarPairVector StarMatching::getMatchedPairsCentroids(const std::vector<Star>& refstarvec, const std::vector<Star>& tgtstarvec, const std::vector<TVGStar>& tvgvec)const {

    StarPairVector spv(tvgvec.size());

    for (int i = 0; i < spv.size(); ++i) {
        spv[i].rxc = refstarvec[tvgvec[i].refstarnum].xc;
        spv[i].ryc = refstarvec[tvgvec[i].refstarnum].yc;
        spv[i].txc = tgtstarvec[tvgvec[i].tgtstarnum].xc;
        spv[i].tyc = tgtstarvec[tvgvec[i].tgtstarnum].yc;
    }

    return spv;
}

StarPairVector StarMatching::matchStars(const StarVector& refstars, const StarVector& tgtstars) {

    if (m_reftri.size() == 0)
        m_reftri = computeTriangles(refstars);

    m_tgttri = computeTriangles(tgtstars);
    
    PotentialStarPairs psp(tgtstars.size(), refstars.size());

    float tol = 0.0002f;
    int lref = 0;
#pragma omp parallel for firstprivate(lref) schedule(static)
    for (int itgt = 0; itgt < m_tgttri.size(); ++itgt) {

        while (lref < m_reftri.size() && m_tgttri[itgt].rx > m_reftri[lref].rx + tol) lref++;

        int iref = lref;

        while (iref < m_reftri.size() && m_reftri[iref].rx < m_tgttri[itgt].rx + tol) {
            if (math::distancef(m_tgttri[itgt].rx, m_tgttri[itgt].ry, m_reftri[iref].rx, m_reftri[iref].ry) <= tol) {
                psp(m_tgttri[itgt].star1, m_reftri[iref].star1)++;
                psp(m_tgttri[itgt].star1, m_reftri[iref].star2)++;
                psp(m_tgttri[itgt].star1, m_reftri[iref].star3)++;
                psp(m_tgttri[itgt].star2, m_reftri[iref].star1)++;
                psp(m_tgttri[itgt].star2, m_reftri[iref].star2)++;
                psp(m_tgttri[itgt].star2, m_reftri[iref].star3)++;
                psp(m_tgttri[itgt].star3, m_reftri[iref].star1)++;
                psp(m_tgttri[itgt].star3, m_reftri[iref].star2)++;
                psp(m_tgttri[itgt].star3, m_reftri[iref].star3)++;
            }
            iref++;
        }
    }

    return getMatchedPairsCentroids(refstars, tgtstars, psp.getTopVoteStars());
}
