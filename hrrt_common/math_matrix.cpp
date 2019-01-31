/*! \class Matrix
    \brief This template implements a 2d matrix type with several algebraic
           operations.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/03/18 added Doxygen style comments
    \date 2004/05/18 improved documentation

    This template class implements a matrix datatype and provides methods to
    calculate algebraic operations on matrices. Be aware that the indices are
    swapped compared to the normal mathematical representation of matrices.
 */

/*! \example example_matrix.cpp
    Here are some examples of how to use the Matrix template class.
 */

#include "math_matrix.hpp"

#include "exception.h"
#include "fastmath.h"

// ---------------------------------------------------------------------------
/*! \brief Initialize the object.

    Initialize the object.
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T>::Matrix() { 
  _rows= 0;
   _columns= 0;
 }

// ---------------------------------------------------------------------------
/*! \brief Create an empty matrix.
    \param[in] icolumns   number of columns in the matrix
    \param[in] irows      number of rows in the matrix

    Create a matrix of the given size and initialize it with zeros.
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T>::Matrix(const unsigned long int icolumns,
                   const unsigned long int irows) { 
  _rows=irows;
   _columns=icolumns;
   if (rows() * columns() > 0) 
    mat.assign(rows() * columns(), 0);
 }

// ---------------------------------------------------------------------------
/*! \brief Create a matrix.
    \param[in] _mat       data for matrix
    \param[in] icolumns   number of columns in the matrix
    \param[in] irows      number of rows in the matrix

    Create a matrix of the given size and copy the given data into it.
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T>::Matrix(T * const _mat,
                   const unsigned long int icolumns,
                   const unsigned long int irows) {
  _rows = irows;
  _columns = icolumns;
  mat.assign(_mat, _mat + rows()*columns());
}

// ---------------------------------------------------------------------------
/*! \brief This is the copy operator for a matrix.
    \param[in] m   source matrix
    \return copy of matrix

    Copy a matrix into this matrix. The original content of this matrix is
    lost.
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator = (const Matrix <T> &m) {
  if (this != &m) {
    mat = m.mat;
    _rows = m.rows();
    _columns = m.columns();
  }
  return (*this);
}

// ---------------------------------------------------------------------------
/*! \brief Add scalar to matrix.
    \param[in] value   scalar value
    \return resulting matrix

     Add a scalar value to this matrix:
     \f[
     \left[\begin{array}{cccc}
      a_{00}+s & a_{10}+s & \cdots & a_{n0}+s\\
      a_{01}+s & a_{11}+s & \cdots & a_{n1}+s\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m}+s & a_{1m}+s & \cdots & a_{nm}+s\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]+s
     \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator += (const T value) { 
  for (unsigned long int r = 0; r < rows(); r++)
    for (unsigned long int c = 0; c < columns(); c++)
     (*this)(c, r) += value;
   return(*this);
 }

// ---------------------------------------------------------------------------
/*! \brief Add matrix to matrix.
    \param[in] m   matrix that will be added
    \exception REC_MATRIX_ADD matrices are of different dimensions
    \return resulting matrix

    Add a matrix to this matrix. Both matrices need to have the same
    dimensions:
    \f[
     \left[\begin{array}{cccc}
      a_{00}+b_{00} & a_{10}+b_{10} & \cdots & a_{n0}+b_{n0}\\
      a_{01}+b_{01} & a_{11}+b_{11} & \cdots & a_{n1}+b_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m}+b_{0m} & a_{1m}+b_{1m} & \cdots & a_{nm}+b_{nm}\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]+
     \left[\begin{array}{cccc}
      b_{00} & b_{10} & \cdots & b_{n0}\\
      b_{01} & b_{11} & \cdots & b_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      b_{0m} & b_{1m} & \cdots & b_{nm}\\
     \end{array}
     \right]
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator += (const Matrix <T> &m) { 
  if ((rows() != m.rows()) || (columns() != m.columns()))
    throw Exception(REC_MATRIX_ADD,
                    "Matrix addition is not allowed because matrices are of different dimensions.");
   for (unsigned long int r = 0; r < rows(); r++)
    for (unsigned long int c = 0; c < columns(); c++)
     (*this)(c, r) += m(c, r);
   return(*this);
 }

// ---------------------------------------------------------------------------
/*! \brief Subtract scalar from matrix.
    \param[in] value   scalar value
    \return resulting matrix

     Subtract a scalar value from this matrix:
     \f[
     \left[\begin{array}{cccc}
      a_{00}-s & a_{10}-s & \cdots & a_{n0}-s\\
      a_{01}-s & a_{11}-s & \cdots & a_{n1}-s\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m}-s & a_{1m}-s & \cdots & a_{nm}-s\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]-s
     \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator -= (const T value) { 
  return((*this) += -value);
 }

// ---------------------------------------------------------------------------
/*! \brief Subtract matrix from matrix.
    \param[in] m   matrix that will be subtracted
    \exception REC_MATRIX_SUB matrices are of different dimensions
    \return resulting matrix

    Subtract a matrix from this matrix. Both matrices need to have the same
    dimensions:
    \f[
     \left[\begin{array}{cccc}
      a_{00}-b_{00} & a_{10}-b_{10} & \cdots & a_{n0}-b_{n0}\\
      a_{01}-b_{01} & a_{11}-b_{11} & \cdots & a_{n1}-b_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m}-b_{0m} & a_{1m}-b_{1m} & \cdots & a_{nm}-b_{nm}\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]-
     \left[\begin{array}{cccc}
      b_{00} & b_{10} & \cdots & b_{n0}\\
      b_{01} & b_{11} & \cdots & b_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      b_{0m} & b_{1m} & \cdots & b_{nm}\\
     \end{array}
     \right]
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator -= (const Matrix <T> &m) { 
  if ((rows() != m.rows()) || (columns() != m.columns()))
    throw Exception(REC_MATRIX_SUB,
                    "Matrix subtraction is not allowed because matrices are of different dimensions.");
   for (unsigned long int r = 0; r < rows(); r++)
    for (unsigned long int c = 0; c < columns(); c++)
     (*this)(c, r)-=m(c, r);
   return(*this);
 }

// ---------------------------------------------------------------------------
/*! \brief Multiply matrix with scalar.
    \param[in] value   scalar value
    \return resulting matrix

     Multiply this matrix with a scalar value:
     \f[
     \left[\begin{array}{cccc}
      a_{00}s & a_{10}s & \cdots & a_{n0}s\\
      a_{01}s & a_{11}s & \cdots & a_{n1}s\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m}s & a_{1m}s & \cdots & a_{nm}s\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m}s & \cdots & a_{nm}\\
     \end{array}
     \right]s
     \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator *= (const T value) { 
  for (unsigned long int r = 0; r < rows(); r++)
    for (unsigned long int c = 0; c < columns(); c++)
     (*this)(c, r)*=value;
   return(*this);
 }

// ---------------------------------------------------------------------------
/*! \brief Multiply matrices.
    \param[in] b   matrix for multiplication
    \exception REC_MATRIX_MULT matrices have wrong sizes
    \return resulting matrix

    Multiply this matrix with a matrix. The number of rows in the matrix
    <I>b</I> must be equal to the number of columns in this matrix:
    \f[A_{m\times p}=A_{n\times m}B_{p\times n}
    \f]
    \f[
     \left[\begin{array}{ccc}
       & \cdots & \\
      \vdots & \sum_{l= 0}^{n-1}a_{li}b_{kl} & \vdots\\
      & \cdots & \\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]
     \left[\begin{array}{cccc}
      b_{00} & b_{10} & \cdots & b_{p0}\\
      b_{01} & b_{11} & \cdots & b_{p1}\\
      \vdots & \vdots & \ddots & \vdots\\
      b_{0n} & b_{1n} & \cdots & b_{pn}\\
     \end{array}
     \right]
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
Matrix <T> & Matrix <T>::operator *= (const Matrix <T> &b) { 
  if (columns() != b.rows())
    throw Exception(REC_MATRIX_MULT,
                    "Matrix multiplication is not allowed because matrices have wrong sizes.");
   std::vector <T> result;

   if (rows()*b.columns() != 0) { 
    result.resize(rows()*b.columns());
      for (unsigned long int r = 0; r < rows(); r++)
       for (unsigned long int c = 0; c < b.columns(); c++) { 
        T *rp;

          rp = &result[r * b.columns() + c];
          *rp= 0;
          for (unsigned long int k= 0; k < columns(); k++)
           *rp+=(*this)(k, r)*b(c, k);
        }
    }
   mat=result;
   _columns=b.columns();
   return(*this);
 }

// ---------------------------------------------------------------------------
/*! \brief Set the value of a matrix element.
    \param[in] c   column of matrix element
    \param[in] r   row of matrix element
    \exception REC_MATRIX_INDEX index is out of range

    Set the value of a matrix element:
    \f[
        a_{cr}=\mbox{value}
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
T & Matrix <T>::operator () (const unsigned long int c,
                             const unsigned long int r)
 { if ((r >= rows()) || (c >= columns()))
    throw Exception(REC_MATRIX_INDEX,
                    "Wrong index in accessing matrix element.");
   return(mat[r*columns()+c]);
 }

// ---------------------------------------------------------------------------
/*! \brief Request the value of a matrix element.
    \param[in] c   column of matrix element
    \param[in] r   row of matrix element
    \exception REC_MATRIX_INDEX index is out of range
    \return value of matrix element

    Request the value of a matrix element:
    \f[
        \mbox{value}=a_{cr}
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
T Matrix <T>::operator () (const unsigned long int c,
                           const unsigned long int r) const
 { if ((r >= rows()) || (c >= columns()))
    throw Exception(REC_MATRIX_INDEX,
                    "Wrong index in accessing matrix element.");
   return(mat[r*columns()+c]);
 }

// ---------------------------------------------------------------------------
/*! \brief Request the number of columns in this matrix.
    \return number of columns in this matrix

    Request the number of columns in this matrix.
 */
// ---------------------------------------------------------------------------
template <typename T>
unsigned long int Matrix <T>::columns() const
 { return(_columns);
 }

// ---------------------------------------------------------------------------
/*! \brief Request a pointer to the data of this matrix.
    \return pointer to the data of this matrix

    Request a pointer to the data of this matrix.
 */
// ---------------------------------------------------------------------------
template <typename T>
T *Matrix <T>::data() const
 { if (mat.size() > 0) return((T *)&mat[0]);
   return(NULL);
 }

// ---------------------------------------------------------------------------
/*! \brief Convert this matrix in an identity matrix.
    \exception REC_MATRIX_IDENT matrix is not square

    Convert the matrix in an identity matrix where the elements on the first
    diagonal are 1 and the rest of the matrix is 0. The number of rows and
    columns in this matrix must be equal and the matrix must not be singular:
   \f[
     \left[\begin{array}{cccc}
      1 & 0 & \cdots & 0\\
      0 & 1 & 0 & \vdots\\
      \vdots & 0 & \ddots & 0\\
      0 & \cdots & 0 & 1\\
     \end{array}
     \right]
   \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
void Matrix <T>::identity() { 
  if (rows() != columns())
    throw Exception(REC_MATRIX_IDENT,
                    "Identity undefined because number of rows and columns of "
                    "matrix are different.");
   if (mat.size() != 0) {
    for (unsigned long int r = 0; r < rows(); r++)
     for (unsigned long int c = 0; c < columns(); c++)
      if (r == c) {
        (*this)(c, r) = (T)1;
      } else {
        (*this)(c, r) = (T)0;
      }
     }
 }

// ---------------------------------------------------------------------------
/*! \brief Invert this matrix.
    \exception REC_MATRIX_INVERS matrix is not square
    \exception REC_MATRIX_SINGULAR matrix is singular

    Invert this matrix. The number of rows and columns in this matrix must be
    equal and the matrix must not be singular. A Gauss-Jordan Elimination
    algorithm is used.
    \f[
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]^{-1}
    \f]
*/
// ---------------------------------------------------------------------------
template <typename T>
void Matrix <T>::invert(){ if (rows() != columns())
    throw Exception(REC_MATRIX_INVERS,
                    "Matrix inversion not allowed because number of rows and "
                    "columns of matrix are different.");
  if (mat.size() == 0) return;
  for (unsigned long int i = 0; i < rows(); i++) {
    if (fabsf((*this)(i, i)) <= 1e-16f)
      throw Exception(REC_MATRIX_SINGULAR, "Matrix inversion undefined because matrix is singular.");
    double q = (T)1 / (*this)(i, i);
    (*this)(i, i) = (T)1;
    for (unsigned long int k = 0; k < columns(); k++)  // multiply row i by q
      (*this)(k, i) *= q;
    for (unsigned long int j = 0; j < rows(); j++)
      if (i != j) {
        q = (*this)(i, j);
        (*this)(i, j) = (T)0;
        // subtract q times row i from row j
        for (unsigned long int k = 0; k < columns(); k++)
          (*this)(k, j) -= (*this)(k, i) * q;
      }
  }
}

// ---------------------------------------------------------------------------
/*! \brief Request the number of rows in this matrix.
    \return number of rows in this matrix

    Request the number of rows in this matrix.
 */
// ---------------------------------------------------------------------------
template <typename T>
unsigned long int Matrix <T>::rows() const
 { return(_rows);
 }

// ---------------------------------------------------------------------------
/*! \brief Transpose this matrix.

    Transpose this matrix by flipping it along the first diagonal:
    \f[
     \left[\begin{array}{cccc}
      a_{00} & a_{01} & \cdots & a_{0n}\\
      a_{10} & a_{11} & \cdots & a_{1n}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{m0} & a_{m1} & \cdots & a_{mn}\\
     \end{array}
     \right]=
     \left[\begin{array}{cccc}
      a_{00} & a_{10} & \cdots & a_{n0}\\
      a_{01} & a_{11} & \cdots & a_{n1}\\
      \vdots & \vdots & \ddots & \vdots\\
      a_{0m} & a_{1m} & \cdots & a_{nm}\\
     \end{array}
     \right]
    \f]
 */
// ---------------------------------------------------------------------------
template <typename T>
void Matrix <T>::transpose() { 
  if (mat.size() == 0) 
    return;

   std::vector <float> mat_t;
   mat_t.resize(mat.size());
   for (unsigned long int r = 0; r < rows(); r++)
    for (unsigned long int c = 0; c < columns(); c++)
     mat_t[c * rows() + r] = (*this)(c, r);
   mat=mat_t;
   std::swap(_columns, _rows);
 }
