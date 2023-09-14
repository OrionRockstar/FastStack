#pragma once
#include "pch.h"

inline std::chrono::steady_clock::time_point GetTimePoint() {
    return std::chrono::high_resolution_clock().now();
}

inline void DisplayTimeDuration(std::chrono::steady_clock::time_point start_point) {
    float dt = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock().now() - start_point).count();
    std::cout << ((dt > 1000) ? dt / 1000 : dt)
        << ((dt > 1000) ? "s" : "ms") << "\n";
}

inline double Distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

inline float Distance(float x1, float y1, float x2, float y2) { return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

template<typename T>
T Max(T a, T b) { return (a > b) ? a : b; }

template<typename T>
T Min(T a, T b) { return (a < b) ? a : b; }

inline int Clip(int val, int low = 0, int high = 10) {
    if (val > high)
        return high;
    else if (val < low)
        return low;
    return val;
}

inline float Clip(float val, float low = 0.0, float high = 1.0) {
    if (val > high)
        return high;
    else if (val < low)
        return low;
    return val;
}

inline double Clip(double val, double low = 0.0, double high = 1.0) {
    if (val > high)
        return high;
    else if (val < low)
        return low;
    return val;
}

template <typename A, typename T>
A Mean(const std::vector<T>& vector) {

    A mean = 0;
    for (const T& val : vector)
        mean += val;

    return mean / vector.size();
}

template <typename A, typename T>
A StandardDeviation(const std::vector<T>& vector) {

    A mean = Mean(vector);

    double d;
    double var = 0;
    for (const float& pixel : vector) {
        d = pixel - mean;
        var += d * d;
    }

    return (A)sqrt(var / vector.size());
}

template<typename T>
T Median_copy(const std::vector<T>& vector) {
    std::vector<T> copy(vector.size());
    memcpy(&copy[0], &vector[0], vector.size() * sizeof(T));

    size_t mid = copy.size() / 2;
    std::nth_element(copy.begin(), copy.begin() + mid, copy.end());
    return copy[mid];
}

template<typename T>
T Median_nocopy(std::vector<T>& vector) {
    size_t mid = vector.size() / 2;
    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}

template<typename T>
T MAD_copy(const std::vector<T>& vector, float median) {
    std::vector<T> copy(vector.size());
    memcpy(&copy[0], &vector[0], vector.size() * sizeof(T));

    size_t mid = copy.size() / 2;
    for (auto& val : copy)
        val = abs(val - median);

    std::nth_element(copy.begin(), copy.begin() + mid, copy.end());
    return copy[mid];
}

template<typename T>
T MAD_nocopy(std::vector<T>& vector, float median) {
    size_t mid = vector.size() / 2;

    for (auto& val : vector)
        val = abs(val - median);

    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}

template<typename T>
struct Point {
    T x = 0;
    T y = 0;

    Point() = default;
    Point(T x, T y) : x(x), y(y) {}

    bool operator()(Point& a, Point& b) { return (a.x < b.x); }
};
typedef Point<int> Pointi;
typedef Point<float> Pointf;
typedef Point<double> Pointd;