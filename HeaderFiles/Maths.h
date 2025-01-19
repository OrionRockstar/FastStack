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

    template<typename T>
    double mean(const std::vector<T>& vector) {

        double sum = 0;
        for (T val : vector)
            sum += val;

        return sum / vector.size();
    }

    template <typename T>
    double standardDeviation(const std::vector<T>& vector) {

        double mean = math::mean<T>(vector);

        double d;
        double var = 0;
        for (const float& pixel : vector) {
            d = pixel - mean;
            var += d * d;
        }

        return sqrt(var / vector.size());
    }

    template<typename T>
    T median(std::vector<T>& vector, bool inplace = true) {

        if (inplace) {
            size_t mid = vector.size() / 2;
            std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
            return vector[mid];
        }

        std::vector<T> copy(vector.size());
        memcpy(&copy[0], &vector[0], vector.size() * sizeof(T));

        size_t mid = copy.size() / 2;
        std::nth_element(copy.begin(), copy.begin() + mid, copy.end());
        return copy[mid];
    }

    template<typename T>
    double avgDev(const std::vector<T>& vector, T median) {

        double sum = 0;

        for (auto val : vector)
            sum += abs(double(val) - median);

        return sum / vector.size();
    }

    template<typename T>
    T mad(std::vector<T>& vector, T median, bool inplace = true) {

        if (inplace) {
            size_t mid = vector.size() / 2;

            for (auto& val : vector)
                val = abs(double(val) - median);

            std::nth_element(vector.begin(), vector.begin() + mid, vector.end());
            return vector[mid];
        }

        std::vector<T> copy(vector.size());
        memcpy(&copy[0], &vector[0], vector.size() * sizeof(T));

        size_t mid = copy.size() / 2;
        for (auto& val : copy)
            val = abs(double(val) - median);

        std::nth_element(copy.begin(), copy.begin() + mid, copy.end());
        return copy[mid];
    }
}

inline std::chrono::steady_clock::time_point getTimePoint() {
    return std::chrono::high_resolution_clock().now();
}

inline void displayTimeDuration(std::chrono::steady_clock::time_point start_point) {
    float dt = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock().now() - start_point).count();
    std::cout << ((dt > 1000) ? dt / 1000 : dt)<< ((dt > 1000) ? "s" : "ms") << "\n";
}

inline float byteswap_float(const float inp) {

    unsigned int buff = *(unsigned int*)&inp;
    buff = _byteswap_ulong(buff);

    return *(float*)&buff;
}

static int NUM_THREADS(int num) {
    return (num > omp_get_max_threads()) ? omp_get_max_threads() : num;
}

template<typename T>
struct PointBase {

private:
    T m_x = 0;
    T m_y = 0;

public:
    PointBase() = default;

    PointBase(T x, T y) : m_x(x), m_y(y) {}
    
    T x()const { return m_x; }

    T& rx() { return m_x; }

    void setX(T x) { m_x = x; }

    T y()const { return m_y; }

    T& ry() { return m_y; }

    void setY(T y) { m_y = y; }

    bool operator()(PointBase& a, PointBase& b) { return (a.x() < b.x()); }
};
typedef PointBase<int> Point;
typedef PointBase<float> PointF;
typedef PointBase<double> PointD;

template<typename T>
struct ImagePointBase {

private:
    T m_x = 0;
    T m_y = 0;
    uint32_t m_channel = 1;

public:
    ImagePointBase() = default;

    ImagePointBase(T x, T y) : m_x(x), m_y(y) {}

    ImagePointBase(T x, T y, uint32_t ch) : m_x(x), m_y(y), m_channel(ch) {}

    T x()const { return m_x; }

    T& rx() { return m_x; }

    void setX(T x) { m_x = x; }

    T y()const { return m_y; }

    T& ry() { return m_y; }

    void setY(T y) { m_y = y; }

    uint32_t channel()const { return m_channel; }

    bool operator()(ImagePointBase& a, ImagePointBase& b) { return (a.x() < b.x()); }
};

typedef ImagePointBase<int> ImagePoint;
typedef ImagePointBase<double> DoubleImagePoint;


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

};

