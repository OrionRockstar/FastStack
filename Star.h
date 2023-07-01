#pragma once
#include <vector>

struct Star {
    // star descriptor
    double xc = 0; // x-coord of center
    double yc = 0; // y-coord of center
    double radius = 0;
    double luminance = 0; //appoximated absolute magnitude of star(luminance)
    Star(double x, double y, double r) :xc(x), yc(y), radius(r) {};
    Star() = default;
    ~Star() {};

    bool operator()(Star& a, Star& b) { return (a.luminance < b.luminance); }
};

typedef std::vector<Star> StarVector;

struct StarPair {
    double rxc = 0; // reference
    double ryc = 0;
    double txc = 0; //target
    double tyc = 0;

    StarPair(double rxc, double ryc, double txc, double tyc) :rxc(rxc), ryc(ryc), txc(txc), tyc(tyc) {};
    StarPair() = default;
};

typedef std::vector<StarPair> StarPairVector;
