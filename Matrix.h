#pragma once

#pragma once
#include <vector>
#include <iostream>
#include <assert.h>

class Matrix {

    int m_rows = 0;
    int m_cols = 0;
    int m_size = 0;

public:
    std::vector<double> data;

    Matrix(int rows, int cols = 1) :m_rows(rows), m_cols(cols), m_size(rows* cols) {
        data.resize(rows * cols);
    }

    Matrix(int rows, int cols, std::initializer_list<double> list) :m_rows(rows), m_cols(cols), m_size(rows* cols) {
        assert(m_size == list.size());
        data = list;
    }

    Matrix() = default;

    Matrix(const Matrix& other) {
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        m_size = other.m_size;
        data = other.data;
        //memcpy(&data[0], &other.data[0], m_size * 8);
    }

    Matrix(Matrix&& other) {
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        m_size = other.m_size;
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
        m_size = other.m_size;

        return *this;
    }

    Matrix& operator=(const Matrix&& other) {
        data = std::move(other.data);
        m_rows = other.m_rows;
        m_cols = other.m_cols;
        m_size = other.m_size;

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
        assert(m_size == list.size());
        data = list;
    }


    const int Rows()const { return m_rows; }

    const int Cols()const { return m_cols; }

    const int Size()const { return m_size; }

    void Print() {
        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c)
                std::cout << data[r * m_cols + c] << " ";
            std::cout << "\n";
        }
    }


    template<typename T>
    void Fill(T value) {
        for (auto& el : data)
            el = value;
    }

    void ModifyRow(int row, int col, std::initializer_list<double> list) {
        assert(list.size() <= m_cols - col);

        for (auto& i : list)
            data[row * m_cols + col++] = i;
    }

    template<typename T>
    void ModifyRow(int row, int col, std::initializer_list<T> list) {
        assert(list.size() <= m_cols - col);

        for (auto& i : list)
            data[row * m_cols + col++] = i;
    }


    void ModifyVector(int start, std::initializer_list<double> list) {
        assert(m_cols == 1 || m_rows == 1 && start + list.size() <= m_size);

        for (auto i : list)
            data[start++] = i;
    }

    template<typename T>
    void ModifyVector(int start, std::initializer_list<T> list) {
        assert(m_cols == 1 || m_rows == 1 && start + list.size() <= m_size);

        for (auto i : list)
            data[start++] = i;
    }

private:
    void ScaleAndSubstractRow(int rowa, int rowb, double scale);

    Matrix MatrixToAugument()const;

    static Matrix GetMatrixFromAugument(const Matrix& aug);

public:
    Matrix Transpose()const;

    Matrix Inverse()const;

    double Determinant()const;

    void MatrixResize(int n_rows, int n_cols = 1);

    Matrix Identity()const;

    static Matrix LeastSquares(const Matrix& A, const Matrix& b);
};
