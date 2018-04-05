/*! \file matrix.h
    \brief This template implements a 2d matrix type with several algebraic
           operations.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/03/18 added Doxygen style comments
    \date 2004/05/18 improved documentation
 */

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
    unsigned long int _rows,                   /*!< number of rows in matrix */
                      _columns;             /*!< number of columns in matrix */
    std::vector <T> mat;                                /*!< matrix elements */
   public:
    Matrix();                                              // initialize object
                                                      // create an empty matrix
    Matrix(const unsigned long int, const unsigned long int);
                                                             // create a matrix
    Matrix(T * const, const unsigned long int, const unsigned long int);
    Matrix <T> & operator = (const Matrix <T>&);                // '='-operator
    Matrix <T> & operator += (const T);                        // '+='-operator
    Matrix <T> & operator += (const Matrix <T>&);
    Matrix <T> & operator -= (const T);                        // '-='-operator
    Matrix <T> & operator -= (const Matrix <T>&);
    Matrix <T> & operator *= (const T);                        // '*='-operator
    Matrix <T> & operator *= (const Matrix <T>&);
                                                 // set value of matrix element
    T & operator () (const unsigned long int, const unsigned long int);
                                                 // get value of matrix element
    T operator () (const unsigned long int, const unsigned long int) const;
    unsigned long int columns() const;    //request number of columns in matrix
    T *data() const;                          // request pointer to matrix data
    void identity();                                  // create identity matrix
    void invert();                                             // invert matrix
    unsigned long int rows() const;         // request number of rows in matrix
    void transpose();                                       // transpose matrix
 };
