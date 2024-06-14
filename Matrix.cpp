#include "pch.h"
#include "Matrix.h"

void Matrix::ScaleAndSubstractRow(int rowa, int rowb, double scale) {
    double* ptra = &(*this)(rowa, 0);
    double* ptrb = &(*this)(rowb, 0);
    for (int i = 0; i < m_cols; ++i)
        ptra[i] -= (scale * ptrb[i]);
}

Matrix Matrix::MatrixToAugument()const {

    Matrix aug(m_rows, 2 * m_cols);

    for (int r = 0; r < m_rows; ++r) {
        const double* mptr = &(*this)(r, 0);
        double* aptr = &aug(r, 0);
        for (int c = 0; c < m_cols; ++c) {
            aptr[c] = mptr[c];

            if (r == c)
                aug(r, c + m_cols) = 1;
        }
    }

    return aug;
}

Matrix Matrix::GetMatrixFromAugument(const Matrix& aug) {

    Matrix matrix(aug.Rows(), aug.Cols() / 2);

    for (int r = 0; r < matrix.Cols(); ++r) {
        const double* aptr = &aug(r, matrix.Cols());
        double* mptr = &matrix(r, 0);
        for (int c = 0; c < matrix.Cols(); ++c)
            mptr[c] = aptr[c];
    }

    return matrix;
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

    Matrix aug = this->MatrixToAugument();

    for (int row = 0; row < m_rows; ++row) {

        double d = aug(row, row);
        for (int c = row; c < aug.Cols(); ++c)
            aug(row, c) /= d;

        for (int r = 0; r < m_rows; ++r)
            if (r != row)
                aug.ScaleAndSubstractRow(r, row, aug(r, row));

    }

    Matrix inverse = GetMatrixFromAugument(aug);

    return inverse;
}

double Matrix::Determinant()const {
    assert(m_rows == m_cols);

    Matrix d(m_rows, m_cols);
    memcpy(&d.data[0], &data[0], m_count * 8);

    for (int row = 0; row < m_rows; ++row) {

        Matrix orig(1, m_cols);
        for (int c = row; c < m_cols; ++c)
            orig[c] = d(row, c);

        double a = d(row, row);
        for (int c = row; c < m_cols; ++c)
            d(row, c) /= a;

        for (int r = row + 1; r < m_rows; ++r)
            d.ScaleAndSubstractRow(r, row, d(r, row));

        for (int c = row; c < m_cols; ++c)
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