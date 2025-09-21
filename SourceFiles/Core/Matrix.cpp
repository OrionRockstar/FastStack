#include "pch.h"
#include "Matrix.h"

void Matrix::scale_SubstractRow(int row_a, int row_b, double scale_b) {
    double* ptra = &(*this)(row_a, 0);
    double* ptrb = &(*this)(row_b, 0);
    for (int i = 0; i < m_cols; ++i)
        ptra[i] -= (scale_b * ptrb[i]);
}

Matrix Matrix::augumentMatrix()const {

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

Matrix Matrix::getMatrixFromAugument(const Matrix& aug) {

    Matrix matrix(aug.rows(), aug.cols() / 2);

    for (int r = 0; r < matrix.cols(); ++r) {
        const double* aptr = &aug(r, matrix.cols());
        double* mptr = &matrix(r, 0);
        for (int c = 0; c < matrix.cols(); ++c)
            mptr[c] = aptr[c];
    }

    return matrix;
}


Matrix Matrix::transpose()const {

    Matrix transpose(cols(), rows());

    for (int r = 0; r < rows(); ++r)
        for (int c = 0; c < cols(); ++c)
            transpose(c, r) = (*this)(r, c);

    return transpose;
}

Matrix Matrix::inverse()const {
    if (rows() != cols())
        return *this;

    Matrix aug = this->augumentMatrix();

    for (int row = 0; row < rows(); ++row) {

        double d = aug(row, row);
        for (int c = row; c < aug.cols(); ++c)
            aug(row, c) /= d;

        for (int r = 0; r < rows(); ++r)
            if (r != row)
                aug.scale_SubstractRow(r, row, aug(r, row));

    }

    return getMatrixFromAugument(aug);
}

double Matrix::determinant()const {
    if (rows() != cols())
        return 0.0;

    Matrix d(rows(), cols());
    memcpy(&d.data[0], &data[0], data.size() * 8);

    for (int row = 0; row < rows(); ++row) {

        Matrix orig(1, cols());
        for (int c = row; c < cols(); ++c)
            orig[c] = d(row, c);

        double a = d(row, row);
        for (int c = row; c < cols(); ++c)
            d(row, c) /= a;

        for (int r = row + 1; r < rows(); ++r)
            d.scale_SubstractRow(r, row, d(r, row));

        for (int c = row; c < cols(); ++c)
            d(row, c) = orig[c];
    }

    double determinant = d(0, 0);
    for (int i = 1; i < rows(); ++i)
        determinant *= d(i, i);

    return determinant;
}

void Matrix::resize(int nrows, int ncols) {

    if (rows() == nrows && cols() == ncols)
        return;

    Matrix other(nrows, ncols);

    int mr = (nrows > rows()) ? rows() : nrows;

    int mc = (ncols > cols()) ? cols() : ncols;

    for (int r = 0; r < mr; ++r)
        for (int c = 0; c < mc; ++c)
            other(r, c) = (*this)(r, c);

    *this = std::move(other);
}

Matrix Matrix::identity()const {
    Matrix I(rows(), cols());
    for (int i = 0; i < I.rows(); ++i)
        I(i, i) = 1;
    return I;
}

Matrix Matrix::leastSquares(const Matrix& A, const Matrix& b) {

    Matrix A_trans = A.transpose();
    return (A_trans * A).inverse() * (A_trans * b);
}