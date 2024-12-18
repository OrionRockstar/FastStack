#pragma once
#include "pch.h"

namespace math {

    template<typename T = double>
    inline T clip(T value, T low = 0.0, T high = 1.0) {
        if (value > high)
            return high;
        else if (value < low)
            return low;
        return value;
    }

    template<typename T>
    T min(T a, T b) { return (a < b) ? a : b; }

    template<typename T>
    T max(T a, T b) { return (a > b) ? a : b; }

    inline double distance(double x1, double y1, double x2, double y2) { return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }

    inline float distancef(float x1, float y1, float x2, float y2) { return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)); }
}

inline std::chrono::steady_clock::time_point GetTimePoint() {
    return std::chrono::high_resolution_clock().now();
}

inline void DisplayTimeDuration(std::chrono::steady_clock::time_point start_point) {
    float dt = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock().now() - start_point).count();
    std::cout << ((dt > 1000) ? dt / 1000 : dt)<< ((dt > 1000) ? "s" : "ms") << "\n";
}

inline float byteswap_float(const float inp) {

    unsigned int buff = *(unsigned int*)&inp;
    buff = _byteswap_ulong(buff);

    return *(float*)&buff;
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

template <typename T>
double Mean(const std::vector<T>& vector) {

    double mean = 0;
    for (const T& val : vector)
        mean += val;

    return mean / vector.size();
}

template <typename T>
double StandardDeviation(const std::vector<T>& vector) {

    double mean = Mean(vector);

    double d;
    double var = 0;
    for (const float& pixel : vector) {
        d = pixel - mean;
        var += d * d;
    }

    return sqrt(var / vector.size());
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
T MAD_copy(const std::vector<T>& vector, T median) {
    std::vector<T> copy(vector.size());
    memcpy(&copy[0], &vector[0], vector.size() * sizeof(T));

    size_t mid = copy.size() / 2;
    for (auto& val : copy)
        val = abs(val - median);

    std::nth_element(copy.begin(), copy.begin() + mid, copy.end());
    return copy[mid];
}

template<typename T>
T MAD_nocopy(std::vector<T>& vector, T median) {
    size_t mid = vector.size() / 2;

    for (auto& val : vector)
        val = abs(val - median);

    std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
    return vector[mid];
}

template<typename T>
T AvgDev(const std::vector<T>& vector, T median) {

    double sum = 0;

    for (auto val : vector) 
        sum += abs(val - median);
    
    return sum / vector.size();
}

static int NUM_THREADS(int num) {
    return (num > omp_get_max_threads()) ? omp_get_max_threads() : num;
}

template<typename T = int>
struct Point {

private:
    T m_x = 0;
    T m_y = 0;

public:
    Point() = default;

    Point(T x, T y) : m_x(x), m_y(y) {}
    
    T x()const { return m_x; }

    T& rx() { return m_x; }

    void setX(T x) { m_x = x; }

    T y()const { return m_y; }

    T& ry() { return m_y; }

    void setY(T y) { m_y = y; }

    bool operator()(Point& a, Point& b) { return (a.x() < b.x()); }
};
typedef Point<float> PointF;
typedef Point<double> PointD;

// remove channel support?!
struct ImagePoint {

private:
    int m_x = 0;
    int m_y = 0;
    uint32_t m_ch = 0;

public:
    ImagePoint() = default;

    ImagePoint(int x, int y) : m_x(x), m_y(y) {}

    ImagePoint(int x, int y, uint32_t ch) : m_x(x), m_y(y), m_ch(ch) {}

    int x()const { return m_x; }

    int& rx() { return m_x; }

    void setX(int x) { m_x = x; }

    int y()const { return m_y; }

    int& ry() { return m_y; }

    void setY(int y) { m_y = y; }

    uint32_t channel()const { return m_ch; }

    bool operator()(ImagePoint& a, ImagePoint& b) { return (a.x() < b.x()); }
};

struct DoubleImagePoint {

private:
    double m_x = 0;
    double m_y = 0;
    uint32_t m_ch = 0;

public:
    DoubleImagePoint() = default;

    DoubleImagePoint(double x, double y) : m_x(x), m_y(y) {}

    DoubleImagePoint(double x, double y, uint32_t ch) : m_x(x), m_y(y), m_ch(ch) {}

    double x()const { return m_x; }

    double& rx() { return m_x; }

    void setX(double x) { m_x = x; }

    double y()const { return m_y; }

    double& ry() { return m_y; }

    void setY(double y) { m_y = y; }

    uint32_t channel()const { return m_ch; }

    bool operator()(DoubleImagePoint& a, DoubleImagePoint& b) { return (a.x() < b.x()); }
};



//template<typename T = float>
class ColorF {
    float m_red = 0;
    float m_green = 0;
    float m_blue = 0;

public:
    ColorF(float red, float green, float blue) : m_red(red), m_green(green), m_blue(blue) {}

    ColorF(float k) : m_red(k) {}

    float k()const { return m_red; }

    void setK(float k) { m_red = k; }

    float red()const { return m_red; }

    void setRed(float red) { m_red = red; }

    float green()const { return m_green; }

    float blue()const { return m_blue; }
};

template<typename T>
class Color {
    T m_red = 0;
    T m_green = 0;
    T m_blue = 0;

public:
    Color(T red, T green, T blue) : m_red(red), m_green(green), m_blue(blue) {}

    Color(T k) : m_red(k) {}

    Color() = default;

    T k()const { return m_red; }

    void setK(float k) { m_red = k; }

    T& rRed() { return m_red; }

    T red()const { return m_red; }

    void setRed(float red) { m_red = red; }

    T& rGreen() { return m_green; }

    T green()const { return m_green; }

    void setGreen(float green) { m_green = green; }

    T& rBlue() { return m_blue; }

    T blue()const { return m_blue; }

    void setBlue(float blue) { m_blue = blue; }

    T maxColor()const { return Max(m_red, Max(m_green, m_blue)); }

    T minColor()const { return Min(m_red, Min(m_green, m_blue)); }

};
