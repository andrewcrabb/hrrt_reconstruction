/*@$
// ELEMENT: Matrix
// DESCRIPTION:
//  This class implements a matrix.
// MEMBER FUNCTIONS:
//  Matrix(unsigned int, unsigned int)
//  Matrix(T *, unsigned int, unsigned int)
//  ~Matrix()
//  Matrix <T> & operator = (const Matrix <T>&)
//  Matrix <T> & operator += (const T)
//  Matrix <T> & operator += (const Matrix <T>&)
//  Matrix <T> & operator *= (const T)
//  Matrix <T> & operator *= (const Matrix <T>&)
//  unsigned int columns()
//  T *data()
//  void dataRemoved()
//  void identity()
//  void invert()
//  void print()
//  unsigned int rows()
//  void set(unsigned int, unsigned int, T)
//  void transpose()
// SEE ALSO:
@$*/

//** file: matrix.h
//** author: Frank Kehren

#ifndef _MATRIX_H
#define _MATRIX_H

/*- definitions -------------------------------------------------------------*/

template <class T> class Matrix
 { protected:
    unsigned int _rows,                             // number of rows in matrix
                 _columns;                       // number of columns in matrix
    T *mat;                                                  // matrix elements
   public:
    Matrix(unsigned int, unsigned int);               // create an empty matrix
    Matrix(T *, unsigned int, unsigned int);                 // create a matrix
    ~Matrix();                                                // destroy matrix
    Matrix <T> & operator = (const Matrix <T>&);                // '='-operator
    Matrix <T> & operator += (const T);                        // '+='-operator
    Matrix <T> & operator += (const Matrix <T>&);
    Matrix <T> & operator *= (const T);                        // '*='-operator
    Matrix <T> & operator *= (const Matrix <T>&);
    unsigned int columns() const;         //request number of columns in matrix
    T *data() const;                          // request pointer to matrix data
    void dataRemoved();                      // matrix data removed from object
    void identity() const;                            // create identity matrix
    void invert() const;                                       // invert matrix
    void print() const;                               // print matrix to stdout
    unsigned int rows() const;              // request number of rows in matrix
                                                 // set value of matrix element
    void set(unsigned int, unsigned int, T) const;
    void transpose();                                       // transpose matrix
 };

#ifndef _MATRIX_CPP
#include "matrix.cpp"
#endif

#endif
