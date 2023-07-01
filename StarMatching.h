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

    TriangleVector TriangleComputation(const StarVector& star_vector);

    struct PotentialStarPairs {
        std::vector<uint32_t> data;

        PotentialStarPairs() {
            data = std::vector<uint32_t>(40'000);
        }

        ~PotentialStarPairs() {};

        uint32_t& operator[](int el) {
            return data[el];
        }

        uint32_t& operator()(int target_star, int reference_star) {
            return data[reference_star * 200 + target_star];
        }

        void AddVote(int target_star, int reference_star) {
            (*this)(target_star, reference_star)++;
        }

        uint32_t Threshold(float K = 1) {
            uint64_t mean = 0;
            for (auto vote : data)
                mean += vote;

            mean /= 40'000;

            int d;
            uint64_t var = 0;
            for (auto vote : data) {
                d = vote - mean;
                var += d * d;
            }

            return mean + K * sqrt(var / 40'000);
        }
    };

    struct TVGStar {
        //star number of the top vote getting stars
        int tgtstarnum = 0;
        int refstarnum = 0;

        TVGStar(double tgt, double ref) :tgtstarnum(tgt), refstarnum(ref) {};
        TVGStar() = default;
    };

    StarPairVector GetMatchedPairsCentroids(const std::vector<Star>& refstarvec, const std::vector<Star>& tgtstarvec, const std::vector<TVGStar>& tvgvec);


public:

    StarPairVector MatchStars(const std::vector<Triangle>& reftri, StarVector& refstars, StarVector& tgtstars);

    StarPairVector MatchStars(const StarVector& refstars, const StarVector& tgtstars);
};
