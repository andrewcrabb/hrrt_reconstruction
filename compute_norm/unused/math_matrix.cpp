//** file: matrix.cpp
//** author: Frank Kehren

#include <iostream>
#include "math_matrix.h"
#include <string.h>

//#include "exception.h"

// using namespace std;

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Matrix: create an empty matrix                                            */
/*  iolumns   number of columns in matrix                                    */
/*  iows      number of rows in matrix                                       */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T>::Matrix(unsigned int icolumns, unsigned int irows)
 { _rows=irows;
   _columns=icolumns;
   mat=new T[_rows*_columns];
   memset(mat, 0, _rows*_columns*sizeof(T));
 }

/*---------------------------------------------------------------------------*/
/* Matrix: create a matrix                                                   */
/*  _mat       data for matrix                                               */
/*  icolumns   number of columns in matrix                                   */
/*  irows      number of rows in matrix                                      */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T>::Matrix(T *_mat, unsigned int icolumns, unsigned int irows)
 { _rows=irows;
   _columns=icolumns;
   mat=_mat;
 }

/*---------------------------------------------------------------------------*/
/* ~Matrix: destroy matrix                                                   */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T>::~Matrix()
 { if (mat != NULL) delete[] mat;
 }

/*---------------------------------------------------------------------------*/
/* =: copy operator                                                          */
/*  m   original matrix                                                      */
/* return: copy of matrix                                                    */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T> & Matrix <T>::operator = (const Matrix <T> &m)
 { if (this != &m)
    { if (m.mat != NULL)
       { if (_rows*_columns != m._rows*m._columns)
          { if (mat != NULL) delete[] mat;
            mat=new T[m._rows*m._columns];
          }
         memcpy(mat, m.mat, m._rows*m._columns*sizeof(T));
       }
       else if (mat != NULL) { delete[] mat;
                               mat=NULL;
                             }
      _rows=m._rows;
      _columns=m._columns;
    }
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/* +=: add scalar value to matrix (t+=s)                                     */
/*  value   scalar value                                                     */
/* return: sum                                                               */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T> & Matrix <T>::operator += (const T value)
 { T *m1;

   m1=mat;
   for (unsigned short int r=0; r < _rows; r++)
    for (unsigned short int c=0; c < _columns; c++)
     *m1+++=value;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/* +=: add matrix to matrix (t+=m)                                           */
/*  m   second matrix                                                        */
/* return: sum                                                               */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T> & Matrix <T>::operator += (const Matrix <T> &m)
 { if ((_rows != m._rows) || (_columns != m._columns))
 //   throw Exception(ERR_MATRIX, "Matrices have different sizes.");
    throw("Matrices have different sizes.");
   T *m1, *m2;

   m1=mat;
   m2=m.mat;
   for (unsigned short int r=0; r < _rows; r++)
    for (unsigned short int c=0; c < _columns; c++)
     *m1+++=*m2++;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/* *=: multiply matrix with scalar (t*=s)                                    */
/*  value   scalar value                                                     */
/* return: product                                                           */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T> & Matrix <T>::operator *= (const T value)
 { T *m1;

   m1=mat;
   for (unsigned short int r=0; r < _rows; r++)
    for (unsigned short int c=0; c < _columns; c++)
     *m1++*=value;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/* *=: multiply matrix with matrix (t*=m)                                    */
/*  m   second matrix                                                        */
/* return: product                                                           */
/*---------------------------------------------------------------------------*/
template <typename T>
Matrix <T> & Matrix <T>::operator *= (const Matrix <T> &m)
 { if (_columns != m._rows)
//    throw Exception(ERR_MATRIX, "Matrices have wrong sizes.");
    throw ("Matrices have wrong sizes.");
   T *result;

   if (_rows*m._columns != 0)
    { result=new T[_rows*m._columns];
      for (unsigned short int r=0; r < _rows; r++)
       for (unsigned short int c=0; c < m._columns; c++)
        { T *rp;

          rp=result+r*m._columns+c;
          *rp=0;
          for (unsigned short int k=0; k < _columns; k++)
           *rp+=*(mat+r*_columns+k)**(m.mat+k*m._columns+c);
        }
    }
    else result=NULL;
   if (mat != NULL) delete[] mat;
   mat=result;
   _columns=m._columns;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/* columns: request number of columns in matrix                              */
/* return: number of columns in matrix                                       */
/*---------------------------------------------------------------------------*/
template <typename T>
unsigned int Matrix <T>::columns() const
 { return(_columns);
 }

/*---------------------------------------------------------------------------*/
/* data: request pointer to matrix data                                      */
/* return: pointer to matrix data                                            */
/*---------------------------------------------------------------------------*/
template <typename T>
T *Matrix <T>::data() const
 { return(mat);
 }

/*---------------------------------------------------------------------------*/
/* dataRemoved: matrix data removed from object                              */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::dataRemoved()
 { mat=NULL;
   _rows=0;
   _columns=0;
 }

/*---------------------------------------------------------------------------*/
/* identity: create identity matrix                                          */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::identity() const
 { 
	//if (_rows != _columns)
	//	throw Exception(ERR_MATRIX,"Number of rows and columns in matrix is different.");
   if (mat != NULL)
    for (unsigned int i=0; i < _rows; i++)
     mat[i*_columns+i]=1;
 }

/*---------------------------------------------------------------------------*/
/* invert: invert square matrix                                              */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::invert() const
 { //if (_rows != _columns)
    //throw Exception(ERR_MATRIX,
    //                "Number of rows and columns in matrix is different.");
   if (mat == NULL) return;
   for (unsigned int i=0; i < _rows; i++)
    { //if (fabs(mat[i*_columns+i]) <= 1e-16) 
      // throw Exception(ERR_MATRIX, "Matrix is singular.");
      float q;

      q=1.0f/mat[i*_columns+i];
      mat[i*_columns+i]=1.0;
      for (unsigned int k=0; k < _columns; k++)          // multiply row i by q
       mat[i*_columns+k]*=q;
      for (unsigned int j=0; j < _rows; j++)
       if (i != j) { q=mat[j*_columns+i];
                     mat[j*_columns+i]=0.0;
                                           // subtract q times row i from row j
                     for (unsigned int k=0; k < _columns; k++)
                      mat[j*_columns+k]-=mat[i*_columns+k]*q;
                   }
    }
 }

/*---------------------------------------------------------------------------*/
/* print: print matrix content to stdout                                     */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::print() const
 { if (mat == NULL) return;
   T *mp;

   mp=mat;
   for (unsigned int r=0; r < _rows; r++)
    { for (unsigned int c=0; c < _columns; c++)
       std::cout << *mp++ << " ";
      std::cout << std::endl;
    }
   std::cout << std::endl;
 }

/*---------------------------------------------------------------------------*/
/* rows: request number of rows in matrix                                    */
/* return: number of rows in matrix                                          */
/*---------------------------------------------------------------------------*/
template <typename T>
unsigned int Matrix <T>::rows() const
 { return(_rows);
 }

/*---------------------------------------------------------------------------*/
/* set: set value of matrix element                                          */
/*  c       column of matrix element                                         */
/*  r       row of matrix element                                            */
/*  value   value of matrix element                                          */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::set(unsigned int c, unsigned int r, T value) const
 { //if ((r >= _rows) || (c >= _columns))
   // throw Exception(ERR_MATRIX, "Wrong index in accessing matrix element.");
   if (mat == NULL) return;
   *(mat+r*_columns+c)=value;
 }

/*---------------------------------------------------------------------------*/
/* transpose: transpose matrix                                               */
/*---------------------------------------------------------------------------*/
template <typename T>
void Matrix <T>::transpose()
 { if (mat == NULL) return;
   T *mat_t;

   mat_t=new T[_columns*_rows];
   for (unsigned int r=0; r < _rows; r++)
    for (unsigned int c=0; c < _columns; c++)
     mat_t[c*_rows+r]=mat[r*_columns+c];
   delete[] mat;
   mat=mat_t;
   { unsigned int tmp;

     tmp=_columns;
     _columns=_rows;
     _rows=tmp;
   }
 }
