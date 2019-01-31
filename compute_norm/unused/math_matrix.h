// ahc
// This was one of the 6 files named 'matrix.h'
// The version in libconvert/ seems newer, so it replaces this one.
// libconvert/ version uses vector<T> instead of T*, and has operator -=
// It does not have the methods print() and set() defined here.

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

//** file: math_matrix.h
//** author: Frank Kehren

// #pragma once

/*- definitions -------------------------------------------------------------*/

template <typename T> class Matrix
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

