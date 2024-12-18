#pragma once
#include "Star.h"

class StarMatching {

    struct Triangle {
        //triangle descriptors
        float rx = 0; //ratio middle/longest side
        float ry = 0; //ratio shortest/longest side
        uint8_t star1 = 0; //star numbers
        uint8_t star2 = 0;
        uint8_t star3 = 0;

        Triangle(float x, float y, uint8_t s1, uint8_t s2, uint8_t s3) : rx(x), ry(y), star1(s1), star2(s2), star3(s3) {};
        Triangle() = default;
        bool operator()(const Triangle& a, const Triangle& b) { return a.rx < b.rx; };
    };

    typedef std::vector<Triangle> TriangleVector;

    TriangleVector m_reftri;
    TriangleVector m_tgttri;

    TriangleVector computeTriangles(const StarVector& star_vector);

    struct PotentialStarPairs {
        std::vector<uint32_t> data;
        int m_cols = 200;

        PotentialStarPairs(int target_size, int reference_size);

        uint32_t& operator()(int target_star, int reference_star) {
            return data[reference_star * m_cols + target_star];
        }

        uint32_t computeVoteThreshold(float K = 1.0f);
    };

    struct TVGStar {
        //star number of the top vote getting stars
        int tgtstarnum = 0;
        int refstarnum = 0;

        TVGStar(double tgt, double ref) :tgtstarnum(tgt), refstarnum(ref) {};
        TVGStar() = default;
    };

    StarPairVector getMatchedPairsCentroids(const std::vector<Star>& refstarvec, const std::vector<Star>& tgtstarvec, const std::vector<TVGStar>& tvgvec);


public:
    StarPairVector matchStars(const StarVector& refstars, const StarVector& tgtstars);
};
