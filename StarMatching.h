#pragma once
#include "StarDetection.h"
#include <omp.h>

class StarMatching
{
public:
    struct Triangle {
        //triangle descriptors
        double rx; //ratio middle/longest side
        double ry; //ratio shortest/longest side
        int star1; //star numbers
        int star2;
        int star3;

        bool operator()(const Triangle &a, const Triangle &b) { return a.rx < b.rx; };
    };

    struct TVGSP {
        //star numbers of top vote getting star pairs
        int tgtstar;
        int refstar;
    };

    typedef std::vector<Triangle> TriangleVector;
    typedef std::vector<TVGSP> TVGSPVector;

    TriangleVector TrianglesComputation(StarDetection::StarVector starvector);

    TVGSPVector MatchStars(TriangleVector reftri,TriangleVector tgttri,int psprow,int pspcol);
};

//double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }