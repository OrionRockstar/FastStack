#pragma once
#include <vector>
#include <iostream>
#include <assert.h>

class Matrix {

    int m_rows = 0;
    int m_cols = 0;
    int m_count = 0;

    std::vector<double> data;

public:
    Matrix(int rows, int cols = 1) :m_rows(rows), m_cols(cols), m_count(rows*cols) {
        data.resize(rows * cols, 0);
    }

    constexpr Matrix(int rows, int cols, std::initializer_list<double> list) :m_rows(rows), m_cols(cols) {
        data.resize(rows * cols);
        int size = (list.size() < data.size()) ? list.size() : data.size();
        for (int i = 0; i < size; ++i)
            data[i] = *(list.begin() + i);
    }

    Matrix() = default;

    Matrix(const Matrix& other) {
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        data = other.data;
    }

    Matrix(Matrix&& other)noexcept {
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        data = std::move(other.data);
    }


    double& operator[](int el) { return data[el]; }

    const double& operator[](int el)const { return data[el]; }

    double& operator()(int row, int col) {
        return data[row * m_cols + col];
    }

    const double& operator()(int row, int col)const {
        return data[row * m_cols + col];
    }

    Matrix& operator=(const Matrix& other) {
        data = other.data;
        m_rows = other.m_rows;
        m_cols = other.m_cols;

        return *this;
    }

    Matrix& operator=(const Matrix&& other)noexcept {
        data = other.data;
        m_rows = other.m_rows;
        m_cols = other.m_cols;

        return *this;
    }

    Matrix operator*(const Matrix& other) {
        assert(m_cols == other.m_rows);
        Matrix out(m_rows, other.m_cols);

        for (int r = 0; r < out.m_rows; ++r)
            for (int i = 0; i < m_cols; ++i)
                for (int c = 0; c < out.m_cols; ++c)
                    out(r, c) += (*this)(r, i) * other(i, c);

        return out;
    }

    Matrix& operator*=(const Matrix& other) {
        assert(m_cols == other.m_rows);
        Matrix out(m_rows, other.m_cols);

        for (int r = 0; r < out.m_rows; ++r)
            for (int i = 0; i < m_cols; ++i)
                for (int c = 0; c < out.m_cols; ++c)
                    out(r, c) += (*this)(r, i) * other(i, c);

        *this = std::move(out);
        return *this;
    }

    friend Matrix operator*(const Matrix& lhs, double val) {
        Matrix out(lhs.m_rows, lhs.m_cols);
        for (int el = 0; el < out.data.size(); ++el)
            out[el] = val * lhs[el];
        return out;
    }

    friend Matrix operator*(double val, const Matrix& rhs) {
        Matrix out(rhs.m_rows, rhs.m_cols);
        for (int el = 0; el < out.data.size(); ++el)
            out[el] = val * rhs[el];
        return out;
    }

    Matrix& operator*=(double val) {
        for (auto& v : data)
            v *= val;
        return *this;
    }

    Matrix operator+(const Matrix& other) {
        assert(m_rows == other.m_rows && m_cols == other.m_cols);
        Matrix out(m_rows, m_cols);
        for (int r = 0; r < m_rows; ++r)
            for (int c = 0; c < m_cols; ++c)
                out(r, c) = (*this)(r, c) + other(r, c);

        return out;
    }

    Matrix& operator+=(const Matrix& other) {
        assert(m_rows == other.m_rows && m_cols == other.m_cols);
        for (int r = 0; r < m_rows; ++r)
            for (int c = 0; c < m_cols; ++c)
                (*this)(r, c) = (*this)(r, c) + other(r, c);

        return *this;
    }

    Matrix operator-(const Matrix& other) {
        assert(m_rows == other.m_rows && m_cols == other.m_cols);
        Matrix out(m_rows, m_cols);
        for (int r = 0; r < m_rows; ++r)
            for (int c = 0; c < m_cols; ++c)
                out(r, c) = (*this)(r, c) - other(r, c);

        return out;
    }

    Matrix& operator-=(const Matrix& other) {
        assert(m_rows == other.m_rows && m_cols == other.m_cols);
        for (int r = 0; r < m_rows; ++r)
            for (int c = 0; c < m_cols; ++c)
                (*this)(r, c) = (*this)(r, c) - other(r, c);

        return *this;
    }

    void operator=(std::initializer_list<double> list) {
        assert(data.size() == list.size());
        data = list;
    }


    int rows()const { return m_rows; }

    int cols()const { return m_cols; }

    std::array<int, 2> size()const { return { m_rows, m_cols }; }

    bool isSize(int rows, int cols)const {
        return (rows == m_rows && cols == m_cols);
    }

    void print()const {
        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c)
                std::cout << data[r * m_cols + c] << " ";
            std::cout << "\n";
        }
    }


    template<typename T>
    void fill(T value) {
        for (auto& el : data)
            el = value;
    }

    template<typename T = double>
    void setRow(int row, const std::initializer_list<T>& list) {

        int size = (list.size() < cols()) ? list.size() : cols();

        for (int i = 0; i < size; ++i)
            (*this)(row, i) = *(list.begin() + i);
    }

private:
    void scale_SubstractRow(int row_a, int row_b, double scale_b);

    Matrix augumentMatrix()const;

    static Matrix getMatrixFromAugument(const Matrix& aug);

public:
    Matrix transpose()const;

    Matrix inverse()const;

    double determinant()const;

    void resize(int nrows, int ncols = 1);

    Matrix identity()const;

    static Matrix leastSquares(const Matrix& A, const Matrix& b);
};
