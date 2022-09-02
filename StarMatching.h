#pragma once
#include "StarDetection.h"

struct Triangle {
    //triangle descriptors
    float rx=0; //ratio middle/longest side
    float ry=0; //ratio shortest/longest side
    uint8_t star1=0; //star numbers
    uint8_t star2=0;
    uint8_t star3=0;

    Triangle(float x, float y, uint8_t s1, uint8_t s2, uint8_t s3) : rx(x), ry(y), star1(s1), star2(s2), star3(s3) {};
    Triangle() = default;

    bool operator()(const Triangle &a, const Triangle &b) { return a.rx < b.rx; };
};

struct TVGSP {
    //star numbers of top vote getting star pairs
    int tgtstar;
    int refstar;
};

typedef std::vector<Triangle> TriangleVector;
typedef std::vector<TVGSP> TVGSPVector;
namespace starmatching {
    TriangleVector TrianglesComputation(const StarVector& starvector);

    TVGSPVector MatchStars(const TriangleVector& reftri, const TriangleVector& tgttri, int psprow, int pspcol);
}
