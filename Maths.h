#pragma once

#include <cmath>
#include <algorithm>

inline double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

inline float Distance(float x1, float y1, float x2, float y2) { return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

template<typename T>
T Max(T a, T b) { return (a > b) ? a : b; }

template<typename T>
T Min(T a, T b) { return (a < b) ? a : b; }

template<typename T>
T MedianNC(std::vector<T>& vector) {
    size_t mid = vector.size() / 2;
    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}

template<typename T>
T MADNC(std::vector<T>& vector, float median) {
    size_t mid = vector.size() / 2;
    for (auto& val : vector)
        val = abs(val - median);
    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}