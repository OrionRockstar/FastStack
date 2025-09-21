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
        for (T pixel : vector) {
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

inline float duration(std::chrono::steady_clock::time_point start_point) {
    return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - start_point).count();
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
    T x = 0;
    T y = 0;

    PointBase() = default;

    PointBase(T x, T y) : x(x), y(y) {}

    template<typename P>
    PointBase(const PointBase<P>& point) {
        x = point.x;
        y = point.y;
    }

    PointBase(const QPoint& qpoint) {
        x = qpoint.x();
        y = qpoint.y();
    }

    PointBase(const QPointF& qpoint) {
        x = qpoint.x();
        y = qpoint.y();
    }
    
    PointBase& operator*=(int factor) {
        x *= factor;
        y *= factor;
        return *this;
    }

    PointBase& operator*=(float factor) {
        x *= factor;
        y *= factor;
        return *this;
    }

    PointBase& operator*=(double factor) {
        x *= factor;
        y *= factor;
        return *this;
    }

    PointBase& operator/=(double divisor) {
        x /= divisor;
        y /= divisor;
        return *this;
    }

    PointBase& operator+=(const PointBase& point) {
        x += point.x;
        y += point.y;
        return *this;
    }

    PointBase& operator-=(const PointBase& point) {
        x -= point.x;
        y -= point.y;
        return *this;
    }

    PointBase operator*(int factor) {
        return PointBase(x * factor, y * factor);
    }

    PointBase operator*(float factor) {
        return PointBase(x * factor, y * factor);
    }

    PointBase operator*(double factor) {
        return PointBase(x * factor, y * factor);
    }

    PointBase operator/(int divisor) {
        return PointBase(x / divisor, y / divisor);
    }

    PointBase operator/(float divisor) {
        return PointBase(x / divisor, y / divisor);
    }

    PointBase operator/(double divisor) {
        return PointBase(x / divisor, y / divisor);
    }

    friend PointBase operator+(const PointBase& lhs, const PointBase& rhs) {
        return PointBase(lhs.x + rhs.x, lhs.y + rhs.y);
    }

    friend PointBase operator-(const PointBase& lhs, const PointBase& rhs) {
        return PointBase(lhs.x - rhs.x, lhs.y - rhs.y);
    }

    friend std::ostream& operator<<(std::ostream& os, const PointBase& p) {
        return os << p.x << " " << p.y;
    }

    QPoint toQPoint()const { return QPoint(x, y); }

    QPointF toQPointF()const { return QPointF(x, y); }

    /*T x()const { return m_x; }

    T& rx() { return m_x; }

    void setX(T x) { m_x = x; }

    T y()const { return m_y; }

    T& ry() { return m_y; }

    void setY(T y) { m_y = y; }*/

    //bool operator()(PointBase& a, PointBase& b) { return (a.x() < b.x()); }
};
typedef PointBase<int> Point;
typedef PointBase<float> PointF;
typedef PointBase<double> PointD;

template<typename T>
struct ImagePointBase {

private:
    T m_x = 0;
    T m_y = 0;
    uint32_t m_channel = 0;

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
//allow int8_t???
requires std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> || std::is_same_v<T, float> || std::is_same_v<T, double>
struct Color {
    T red = 0;
    T green = 0;
    T blue = 0;

    Color(T red, T green, T blue) : red(red), green(green), blue(blue) {}

    Color() = default;
};
typedef Color<uint8_t> Color8;
typedef Color<uint16_t> Color16;
typedef Color<float> ColorF;
typedef Color<double> ColorD;



struct ColorConversion {
    inline static const uint16_t u8_u16 = 257;
    inline static const float u8_f = 1 / 255.0;
    inline static const float u16_f = 1 / 65535.0;

    inline static const uint8_t f_u8 = 255;

public:
    static Color8 toColor8(const Color8& color) {
        return color;
    }

    static Color8 toColor8(const Color16& color) {
        return Color8(color.red / u8_u16, color.green / u8_u16, color.blue / u8_u16);
    }

    static Color8 toColor8(const ColorF& color) {
        return Color8(color.red * f_u8, color.green * f_u8, color.blue * f_u8);
    }


    static Color16 toColor16(const Color8& color) {
        return Color16(color.red * u8_u16, color.green * u8_u16, color.blue * u8_u16);
    }


    static ColorF toColorF(const Color8& color) {
        return ColorF(color.red * u8_f, color.green * u8_f, color.blue * u8_f);
    }

    static ColorF toColorF(const Color16& color) {
        return ColorF(color.red * u16_f, color.green * u16_f, color.blue * u16_f);
    }

    static ColorF toColorF(const ColorF& color) {
        return color;
    }


};



class Threads {

    uint32_t m_thread_count = std::thread::hardware_concurrency();

public:
    Threads(uint32_t thread_count = std::thread::hardware_concurrency()) : m_thread_count(thread_count) {}

    template<class Func, class... Args>
    static void runThread(Func&& func, Args&&... args) {

        auto thread = std::thread(func, args...);
        thread.detach();
    }

    void run(std::function<void(uint32_t start, uint32_t end)> func, uint32_t size) {

        if (size == 0)
            return;

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<std::thread> threads;

        for (int i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            threads.emplace_back(func, start, math::min(start + chunk_size, size));
        }

        for (auto& th : threads)
            th.join();
    }

    void run(std::function<void(uint32_t start, uint32_t end, uint32_t thread_num)> func, uint32_t size) {

        if (size == 0)
            return;

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<std::thread> threads;

        for (uint32_t i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            threads.emplace_back(func, start, math::min(start + chunk_size, size), i);
        }

        for (auto& th : threads)
            th.join();
    }
};


//get rid of event loop
class QThreads : QObject{

    uint32_t m_thread_count = std::thread::hardware_concurrency();
    //std::mutex m;
public:
    QThreads(uint32_t thread_count = std::thread::hardware_concurrency()) : m_thread_count(thread_count) {}

    template<class Func, class... Args>
    static void runThread(Func&& func, Args&&... args) {

       // QEventLoop loop;
        auto thread = QThread::create(func, args...);
        //connect(thread, &QThread::finished, [&]() { loop.quit(); });
        thread->start();
        //loop.exec();
    }

    void run(std::function<void(uint32_t start, uint32_t end)> func, uint32_t size) {

        if (size == 0)
            return;

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<QThread*> threads;

        //std::atomic_uint32_t finished_counter;
        //QEventLoop loop;

        /*auto onFinished = [&, thread_count]() {
            if (++finished_counter >= thread_count)
                loop.quit();
        };*/

        for (uint32_t i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            auto t = threads.emplace_back(QThread::create(func, start, math::min(start + chunk_size, size)));
            //connect(t, &QThread::finished, onFinished);
            t->start();
        }

        //loop.exec();
    }

    void run(std::function<void(uint32_t start, uint32_t end, uint32_t thread_num)> func, uint32_t size) {

        if (size == 0)
            return;

        //std::lock_guard<std::mutex> lg(m);

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<QThread*> threads;

        //std::atomic_uint32_t finished_counter;
        //QEventLoop loop;

        /*auto onFinished = [&, thread_count]() {
            if (++finished_counter >= thread_count)
                loop.quit();
        };*/

        for (uint32_t i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            auto t = threads.emplace_back(QThread::create(func, start, math::min(start + chunk_size, size), i));
            //connect(t, &QThread::finished, onFinished);
            t->start();
        }

        //loop.exec();
    }
};



class QEventThreads : QObject {

    uint32_t m_thread_count = std::thread::hardware_concurrency();
    //std::mutex m;
public:
    QEventThreads(uint32_t thread_count = std::thread::hardware_concurrency()) : m_thread_count(thread_count) {}

    template<class Func, class... Args>
    static void runThread(Func&& func, Args&&... args) {

        QEventLoop loop;
        auto thread = QThread::create(func, args...);
        connect(thread, &QThread::finished, [&]() { loop.quit(); });
        thread->start();
        loop.exec();
    }

    void run(std::function<void(uint32_t start, uint32_t end)> func, uint32_t size) {

        if (size == 0)
            return;

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<QThread*> threads;

        std::atomic_uint32_t finished_counter;
        QEventLoop loop;

        auto onFinished = [&, thread_count]() {
            if (++finished_counter >= thread_count)
                loop.quit();
        };

        for (uint32_t i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            auto t = threads.emplace_back(QThread::create(func, start, math::min(start + chunk_size, size)));
            connect(t, &QThread::finished, onFinished);
            t->start();
        }

        loop.exec();
    }

    void run(std::function<void(uint32_t start, uint32_t end, uint32_t thread_num)> func, uint32_t size) {

        if (size == 0)
            return;

        //std::lock_guard<std::mutex> lg(m);

        uint32_t thread_count = math::min<uint32_t>(size, m_thread_count);
        uint32_t chunk_size = (size > m_thread_count) ? (size / m_thread_count) + 1 : 1;

        std::vector<QThread*> threads;

        std::atomic_uint32_t finished_counter;
        QEventLoop loop;

        auto onFinished = [&, thread_count]() {
            if (++finished_counter >= thread_count)
                loop.quit();
        };

        for (uint32_t i = 0; i < thread_count; ++i) {
            uint32_t start = i * chunk_size;
            auto t = threads.emplace_back(QThread::create(func, start, math::min(start + chunk_size, size), i));
            connect(t, &QThread::finished, onFinished);
            t->start();
        }

        loop.exec();
    }
};