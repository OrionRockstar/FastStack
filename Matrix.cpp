#include "pch.h"
#include "Matrix.h"

void Matrix::SwapRow(int row_a, int row_b) {
    for (int c = 0; c < m_cols; ++c)
        std::swap((*this)(row_a, c), (*this)(row_b, c));
}

void Matrix::ScaleRow(int row, double val) {
    for (int c = 0; c < m_cols; ++c)
        (*this)(row, c) *= val;
}

Matrix Matrix::ScaleRowTemp(int row, double val) {
    Matrix t(m_cols);

    for (int c = 0; c < m_cols; ++c)
        t[c] = (*this)(row, c) * val;

    return t;
}

void Matrix::SubtractTempFromRow(int row, Matrix& temp) {
    for (int c = 0; c < m_cols; ++c)
        (*this)(row, c) -= temp[c];
}

void Matrix::SubtractRows(int row_a, int row_b) {
    for (int c = 0; c < m_cols; ++c)
        (*this)(row_a, c) -= (*this)(row_b, c);
}

Matrix Matrix::Transpose()const {

    Matrix transpose(m_cols, m_rows);

    for (int r = 0; r < m_rows; ++r)
        for (int c = 0; c < m_cols; ++c)
            transpose(c, r) = (*this)(r, c);

    return transpose;
}

Matrix Matrix::Inverse()const {
    assert(m_rows == m_cols);

    //if (Determinant() == 0)
        //return (*this);

    Matrix aug(m_rows, 2 * m_cols);

    for (int r = 0; r < m_rows; ++r) {
        for (int c = 0; c < m_cols; ++c) {
            aug(r, c) = (*this)(r, c);
            if (r == c)
                aug(r, c + m_cols) = 1;
        }
    }

    for (int row = 0; row < m_rows; ++row) {

        /*if ((*this)(row, row) != 1)
            for (int r = row; r < m_rows; r++)
                if (aug(r, row) == 1)
                    aug.SwapRow(row, r);*/

                    //reduces row to be leading by 1 
        double d = aug(row, row);
        for (int c = 0; c < aug.m_cols; ++c)
            aug(row, c) /= d;

        for (int r = 0; r < m_rows; ++r) {
            if (r == row)
                continue;
            Matrix temp = aug.ScaleRowTemp(row, aug(r, row));
            aug.SubtractTempFromRow(r, temp);
        }
    }

    Matrix inverse(m_rows, m_cols);

    for (int r = 0; r < m_rows; ++r)
        for (int c = m_cols; c < aug.m_cols; ++c)
            inverse(r, c - m_cols) = aug(r, c);

    return inverse;
}

double Matrix::Determinant()const {
    assert(m_rows == m_cols);

    Matrix d(m_rows, m_cols);
    memcpy(&d.data[0], &data[0], m_size * 8);

    for (int row = 0; row < m_rows; ++row) {

        Matrix orig(1, m_cols);
        for (int c = 0; c < m_cols; ++c)
            orig[c] = d(row, c);

        double a = d(row, row);
        for (int c = 0; c < m_cols; ++c)
            d(row, c) /= a;

        for (int r = 0; r < m_rows; ++r) {
            //if (r == row)
                //continue;
            Matrix temp = d.ScaleRowTemp(row, d(r, row));
            d.SubtractTempFromRow(r, temp);
        }

        for (int c = 0; c < m_cols; ++c)
            d(row, c) = orig[c];
    }

    double determinant = d(0, 0);
    for (int i = 1; i < m_rows; ++i)
        determinant *= d(i, i);

    return determinant;
}

void Matrix::MatrixResize(int n_rows, int n_cols) {

    if (m_rows == n_rows && m_cols == n_cols)
        return;

    Matrix other(n_rows, n_cols);

    int rows = (n_rows > m_rows) ? m_rows : n_rows;

    int cols = (n_cols > m_cols) ? m_cols : n_cols;

    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            other.data[r * n_cols + c] = (*this)(r, c);

    *this = std::move(other);

}

Matrix Matrix::Identity()const {
    Matrix I(m_rows, m_cols);
    for (int i = 0; i < I.m_rows; ++i)
        I(i, i) = 1;
    return I;
}

Matrix Matrix::LeastSquares(const Matrix& A, const Matrix& b) {

    return (A.Transpose() * A).Inverse() * (A.Transpose() * b);
}