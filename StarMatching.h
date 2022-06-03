#pragma once
#include "StarDetection.h"
//#include <omp.h>

class StarMatching
{
public:
    struct Triangle {
        //triangle descriptors
        float rx; //ratio middle/longest side
        float ry; //ratio shortest/longest side
        unsigned char star1; //star numbers
        unsigned char star2;
        unsigned char star3;

        bool operator()(const Triangle &a, const Triangle &b) { return a.rx < b.rx; };
    };

    struct TVGSP {
        //star numbers of top vote getting star pairs
        int tgtstar;
        int refstar;
    };

    typedef std::vector<Triangle> TriangleVector;
    typedef std::vector<TVGSP> TVGSPVector;

    TriangleVector TrianglesComputation(const StarDetection::StarVector &starvector);

    TVGSPVector MatchStars(const TriangleVector &reftri,const TriangleVector &tgttri,int psprow,int pspcol);
};