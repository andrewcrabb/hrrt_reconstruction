/*! \file math_matrix.hpp
    \brief This template implements a 2d matrix type with several algebraic
           operations.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/03/18 added Doxygen style comments
    \date 2004/05/18 improved documentation
 */

// ahc 1/29/19
// This was one of the 6 files named "matrix.h", some for maths and some for ECAT
// It seems to be the most advanced math one, so it's retained and moved to its own library in hrrt_common

#pragma once

#include <iostream>
#include <vector>

/*- template definitions ----------------------------------------------------*/

template <typename T> class Matrix
 {
 /*! \fn friend std::ostream & operator << (std::ostream &os, Matrix <T>& a)
     \brief Print matrix content into stream.
     \param os   stream for output
     \param a    matrix to be printed
     \return stream with matrix content

     Print the content of a matrix into a stream.
  */
   friend std::ostream & operator << (std::ostream &os, Matrix <T>& a)
    { if (a.data() == NULL) return(os);
      for (unsigned long int r=0; r < a.rows(); r++)
       { for (unsigned long int c=0; c < a.columns(); c++)
          os << a(c, r) << " ";
         os << "\n";
       }
      return(os);
    }
   protected:
    unsigned long int _rows, _columns;
    std::vector <T> mat;
   public:
    Matrix();
    Matrix(const unsigned long int, const unsigned long int);
    Matrix(T * const, const unsigned long int, const unsigned long int);
    Matrix <T> & operator = (const Matrix <T>&);
    Matrix <T> & operator += (const T);
    Matrix <T> & operator += (const Matrix <T>&);
    Matrix <T> & operator -= (const T);
    Matrix <T> & operator -= (const Matrix <T>&);
    Matrix <T> & operator *= (const T);
    Matrix <T> & operator *= (const Matrix <T>&);
    T & operator () (const unsigned long int, const unsigned long int);  // get value of matrix element
    T operator () (const unsigned long int, const unsigned long int) const;
    unsigned long int columns() const;
    T *data() const;                          // request pointer to matrix data
    void identity();
    void invert();
    unsigned long int rows() const;
    void transpose();
 };
