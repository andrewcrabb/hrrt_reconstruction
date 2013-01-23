/*! \file vecmath.cpp
    \brief This module provides functions for vector operations.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/01 initial version
    \date 2004/04/29 added vecAddScalar function
    \date 2004/09/25 added functions for complex vectors

    This module provides functions for vector operations.

    \todo Add code for convolutions to be used in image_filter, add code for
    complex vector operations to be used in fft.
 */

#include <string.h>
#ifdef __ALTIVEC__
#include "Accelerate/Accelerate.h"
#include "vecLib/vecLib.h"
#endif
#include "fastmath.h"
#define _VECMATH_CPP
#include "vecmath.h"

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Add two vectors.
    \param[in]     a     first vector
    \param[in]     b     second vector
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Add two vectors:
    \f[
        c_i=a_i+b_i \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecAdd(const float * const a, const float * const b, float * const c,
            const unsigned long int len)
 {
#ifdef __ALTIVEC__
    vadd(a, 1, b, 1, c, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    c[i]=a[i]+b[i];
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add two complex vectors.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     br    real part of second vector
    \param[in]     bi    imaginary part of second vector
    \param[in,out] cr    real part of result vector
    \param[in,out] ci    imaginary part of result vector
    \param[in]     len   length of vectors

    Add two complex vectors:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})+(b_{r_j};b_{i_j})=
        (a_{r_j}+b_{r_j};a_{i_j}+b_{i_j})
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecAddComplex(const float * const ar, const float * const ai,
                   const float * const br, const float * const bi,
                   float * const cr, float * const ci,
                   const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, b, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   b.realp=&br[0];
   b.imagp=&bi[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   zvadd(&a, 1, &b, 1, &c, 1, len);
#else
   vecAdd(ar, br, cr, len);
   vecAdd(ai, bi, ci, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add complex and real vector.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     b     second vector
    \param[in,out] cr    real part of result vector
    \param[in,out] ci    imaginary part of result vector
    \param[in]     len   length of vectors

    Add complex and real vector:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})+b_j=(a_{r_j}+b_j;a_{i_j})
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecAddComplexReal(const float * const ar, const float * const ai,
                       const float * const b, float * const cr,
                       float * const ci, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   zrvadd(&a, 1, b, 1, &c, 1, len);
#else
   vecAdd(ar, b, cr, len);
   memcpy(ci, ai, len*sizeof(float));
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add and multiply vectors.
    \param[in]     a     first vector
    \param[in]     b     second vector
    \param[in]     c     third vector
    \param[in,out] d     result vector
    \param[in]     len   length of vectors

    Add and multiply vectors:
    \f[
        d_i=(a_i+b_i)c_i \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecAddMult(const float * const a, const float * const b,
                const float * const c, float * const d,
                const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vam(a, 1, b, 1, c, 1, d, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    d[i]=(a[i]+b[i])*c[i];
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add scalar to vector.
    \param[in]     a     vector
    \param[in]     b     scalar
    \param[in,out] c     result vector
    \param[in]     len   length of vector

    Add scalar to vector:
    \f[
        c_i=a_i+b \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecAddScalar(const float * const a, const float b, float * const c,
                  const unsigned long int len)
 { for (unsigned long int i=0; i < len; i++)
    c[i]=a[i]+b;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convolute a signal with a filter.
    \param[in]     signal          signal
    \param[in]     signal_stride   offset between neighbouring data elements
    \param[in]     signal_len      length of signal
    \param[in]     filter          filtet
    \param[in]     filter_len      length of filter (odd number)
    \param[in,out] result          filtered signal
 
    Convolute a signal with a filter, following the equation
    \f[
       t_i=\frac{\sum\limits_{j=0}^{l-1}s_{i+j-\frac{l-1}{2}}f_j}
                {\sum\limits_{j=0}^{l-1}f_j} \qquad \forall \;\; 0\le i<k
    \f]
    where \f$s\f$ is the input signal of length \f$k\f$, \f$t\f$ the output
    signal and \f$f\f$ the filter of length \f$l\f$. The edges of the signal
    are extended to make filtering at the borders possible. The resulting
    signal will have the same length and stride as the input signal. The
    convolution can also be done in-place.
 */
/*---------------------------------------------------------------------------*/
void vecConvolution(const float * const signal,
                    const unsigned long int signal_stride,
                    const unsigned long int signal_len,
                    const float * const filter,
                    const unsigned long int filter_len, float * const result)
 { if ((filter_len & 0x1) == 0) return;
   unsigned long int i;
   float b=0.0f;
 
   b=1.0f/(float)vecScalarAdd(filter, filter_len);
#ifdef __ALTIVEC__
   vImage_Buffer src, dest;

   src.data=signal;
   src.height=signal_len;
   src.width=1;
   src.rowBytes=signal_stride*sizeof(float);
   dest.data=result;
   dest.height=signal_len;
   dest.width=1;
   dest.rowBytes=src.rowBytes;
   vImageConvolve_PlanarF(&src, &dest, NULL, 0, 0, filter, 1, filter_len,
                          0, kvImageEdgeExtend);
   vsmul(result, signal_stride, &b, result, signal_stride, signal_len);
#else
   float *buffer=NULL;
 
   try
   { unsigned long int fh;
                                                  // allocate buffer with edges
     buffer=new float[signal_len+filter_len-1];
     fh=filter_len/2;
     for (i=0; i < signal_len; i++)
      buffer[fh+i]=signal[i*signal_stride];
                                              // extend edges with border pixel
     for (i=0; i < fh; i++)
      buffer[i]=buffer[fh];
     for (i=0; i < fh; i++)
      buffer[signal_len+fh+i]=buffer[signal_len+fh-1];
                                                       // calculate convolution
     for (i=0; i < signal_len; i++)
      result[i*signal_stride]=vecDotProd(&buffer[i], filter, filter_len)*b;
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy interleaved complex vector into split complex vector.
    \param[in]     a     interleaved complex vector
    \param[in,out] br    real part of complex vector
    \param[in,out] bi    imaginary part of complex vector
    \param[in]     len   length of vector

    Copy interleaved complex vector into split complex vector.
 */
/*---------------------------------------------------------------------------*/
void vecCopyIntlCplxSplCplx(const float * const a, float * const br,
                            float * const bi, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex b;

   b.realp=&br[0];
   b.imagp=&bi[0];
   ctoz((DSPComplex *)a, 1, &b, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    { br[i]=a[2*i];
      bi[i]=a[2*i+1];
    }
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy split complex vector into interleaved complex vector.
    \param[in]     ar    real part of complex vector
    \param[in]     ai    imaginary part of complex vector
    \param[in,out] b     interleaved complex vector
    \param[in]     len   length of vector

    Copy split complex vector into interleaved complex vector.
 */
/*---------------------------------------------------------------------------*/
void vecCopySplCplxIntlCplx(const float * const ar, const float * const ai,
                            float * const b, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   ztoc(&a, 1, (DSPComplex *)b, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    { b[2*i]=ar[i];
      b[2*i+1]=ai[i];
    }
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate absolute value of complex vector.
    \param[in]     re    real part of complex vector
    \param[in]     im    imaginary part of complex vector
    \param[in,out] a     result vector
    \param[in]     len   length of vectors

    Calculate absolute value of complex vector:
    \f[
        a_i=\sqrt{re^2_i+im^2_i} \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecCplxAbs(const float * const re, const float * const im,
                float * const a, const unsigned long int len)
 { for (unsigned long int i=0; i < len; i++)
    a[i]=sqrtf(re[i]*re[i]+im[i]*im[i]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate complex vector conjugate, multiply and add.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     br    real part of second vector
    \param[in]     bi    imaginary part of second vector
    \param[in]     cr    real part of second vector
    \param[in]     ci    imaginary part of second vector
    \param[in,out] dr    real part of result
    \param[in,out] di    imaginary part of result
    \param[in]     len   length of vectors

    Calculate complex vector conjugate, multiply and add:
    \f[
        (d_{r_j};d_{i_j})=(a_{r_j}b_{r_j}+a_{i_j}b_{i_j}+c_{r_j};
                           a_{r_j}b_{i_j}-a_{i_j}b_{r_j}+c_{i_j})
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecCplxConjugateMultiplyAdd(
                                const float * const ar, const float * const ai,
                                const float * const br, const float * const bi,
                                const float * const cr, const float * const ci,
                                float * const dr, float * const di,
                                const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, b, c, d;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   b.realp=&br[0];
   b.imagp=&bi[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   d.realp=&dr[0];
   d.imagp=&di[0];
   zvcma(&a, 1, &b, 1, &c, 1, &d, 1, len);
#else
   vecMulComplex(ar, ai, br, bi, dr, di, len, true);
   vecAdd(dr, cr, dr, len);
   vecAdd(di, ci, di, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Divide two vectors.
    \param[in]     a     first vector
    \param[in]     b     second vector
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Divide two vectors:
    \f[
        c_i=\left\{
        \begin{array}{r@{\quad:\quad}l}
        \frac{a_i}{b_i} & b_i \neq 0\\
        a_i & b_i=0\\
        \end{array}\right.
        \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecDiv(const float * const a, const float * const b, float * const c,
            const unsigned long int len)
 { for (unsigned long int i=0; i < len; i++)
    if (b[i] != 0.0f) c[i]=a[i]/b[i];
     else c[i]=a[i];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate dot product.
    \param[in] a     first vector
    \param[in] b     second vector
    \param[in] len   length of vectors
    \return resulting scalar

    Calculate dot product (inner product):
    \f[
        c=\sum_{i=0}^{len-1}a_ib_i
    \f]
 */
/*---------------------------------------------------------------------------*/
float vecDotProd(const float * const a, const float * const b,
                 const unsigned long int len)
 { float c;
#ifdef __ALTIVEC__
   dotpr(a, 1, b, 1, &c, len);
#else
   c=0.0f;
   for (unsigned long int i=0; i < len; i++)
    c+=a[i]*b[i];
#endif
   return(c);
 }
 
/*---------------------------------------------------------------------------*/
/*! \brief Calculate complex dot product.
    \param[in]     ar          real part of first vector
    \param[in]     ai          imaginary part of first vector
    \param[in]     br          real part of second vector
    \param[in]     bi          imaginary part of second vector
    \param[in,out] cr          real part of result
    \param[in,out] ci          imaginary part of result
    \param[in]     len         length of vectors
    \param[in]     conjugate   conjugate dot product ?

    Calculate complex dot product:
    \f[
        (c_r;c_i)=\sum_{j=0}^{len-1}(a_{r_j}b_{r_j}-a_{i_j}b_{i_j};
                                     a_{r_j}b_{i_j}+a_{i_j}b_{r_j})
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecDotProdComplex(const float * const ar, const float * const ai,
                       const float * const br, const float * const bi,
                       float * const cr, float * const ci,
                       const unsigned long int len, const bool conjugate)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, b, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   b.realp=&br[0];
   b.imagp=&bi[0];
   c.realp=cr;
   c.imagp=ci;
   if (conjugate) zidotpr(&a, 1, &b, 1, &c, len);
    else zdotpr(&a, 1, &b, 1, &c, len);
#else
   if (conjugate)
    { *cr=vecDotProd(ar, br, len)+vecDotProd(ai, bi, len);
      *ci=vecDotProd(ar, bi, len)-vecDotProd(ai, br, len);
      return;
    }
   *cr=vecDotProd(ar, br, len)-vecDotProd(ai, bi, len);
   *ci=vecDotProd(ar, bi, len)+vecDotProd(ai, br, len);
#endif
 }
 
/*---------------------------------------------------------------------------*/
/*! \brief Calculate dot product between complex and real vector.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     b     second vector
    \param[in,out] cr    real part of result
    \param[in,out] ci    imaginary part of result
    \param[in]     len   length of vectors

    Calculate dot product between complex and real vector:
    \f[
        (c_r;c_i)=\sum_{j=0}^{len-1}(a_{r_j}b_j;a_{i_j}b_j)
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecDotProdComplexReal(const float * const ar, const float * const ai,
                           const float * const b, float * const cr,
                           float * const ci, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   c.realp=cr;
   c.imagp=ci;
   zrdotpr(&a, 1, b, 1, &c, len);
#else
   *cr=vecDotProd(ar, b, len);
   *ci=vecDotProd(ai, b, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply two vectors.
    \param[in]     a     first vector
    \param[in]     b     second vector
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Multiply two vectors (Hadamard product):
    \f[
        c_i=a_ib_i \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecMul(const float * const a, const float * const b, float * const c,
            const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vmul(a, 1, b, 1, c, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    c[i]=a[i]*b[i];
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply two complex vectors.
    \param[in]     ar          real part of first vector
    \param[in]     ai          imaginary part of first vector
    \param[in]     br          real part of second vector
    \param[in]     bi          imaginary part of second vector
    \param[in,out] cr          real part of result vector
    \param[in,out] ci          imaginary part of result vector
    \param[in]     len         length of vectors
    \param[in]     conjugate   multiply with conjugate values ?

    Multiply two complex vectors:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})(b_{r_j};b_{i_j})=\left\{
        \begin{array}{r@{\quad:\quad}l}
        (a_{r_j}b_{r_j}-a_{i_j}b_{i_j};
         a_{r_j}b_{i_j}+a_{i_j}b_{r_j}) & \mbox{normal multiplication}\\
        (a_{r_j}b_{r_j}+a_{i_j}b_{i_j};
         a_{r_j}b_{i_j}-a_{i_j}b_{r_j}) & \mbox{multiplication with conjugate}
        \end{array}\right.
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecMulComplex(const float * const ar, const float * const ai,
                   const float * const br, const float * const bi,
                   float * const cr, float * const ci,
                   const unsigned long int len, const bool conjugate)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, b, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   b.realp=&br[0];
   b.imagp=&bi[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   if (conjugate) zvmul(&a, 1, &b, 1, &c, 1, len, -1);
    else zvmul(&a, 1, &b, 1, &c, 1, len, 1);
#else
   unsigned long int i;

   if (conjugate)
    { for (i=0; i < len; i++)
       cr[i]=ar[i]*br[i]+ai[i]*bi[i];
      for (i=0; i < len; i++)
       ci[i]=ar[i]*bi[i]-ai[i]*br[i];
      return;
    }
   for (i=0; i < len; i++)
    cr[i]=ar[i]*br[i]-ai[i]*bi[i];
   for (i=0; i < len; i++)
    ci[i]=ar[i]*bi[i]+ai[i]*br[i];
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply complex and real vector.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     b     second vector
    \param[in,out] cr    real part of result vector
    \param[in,out] ci    imaginary part of result vector
    \param[in]     len   length of vectors

    Multiply complex and real vector:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})b_j=(a_{r_j}b_j;a_{i_j}b_j)
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecMulComplexReal(const float * const ar, const float * const ai,
                       const float * const b, float * const cr,
                       float * const ci, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   zrvmul(&a, 1, b, 1, &c, 1, len);
#else
   vecMul(ar, b, cr, len);
   vecMul(ai, b, ci, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply two vector with scalars and add them.
    \param[in]     a     first vector
    \param[in]     b     first scalar
    \param[in]     c     second vector
    \param[in]     d     second scalar
    \param[in,out] e     result vector
    \param[in]     len   length of vectors

    Multiply two vector with scalars and add them:
    \f[
       e_i=a_ib+c_id \qquad \forall 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecMulAdd(const float * const a, const float b, const float * const c,
               const float d, float * const e, const unsigned long int len)
 { vecMulScalar(a, b, e, len);
   vecMulScalarAdd(c, d, e, len);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply vector with scalar.
    \param[in]     a     vector
    \param[in]     b     scalar
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Multiply vector with scalar:
    \f[
        c_i=a_ib \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecMulScalar(const float * const a, const float b, float * const c,
                  const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vsmul(a, 1, &b, c, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    c[i]=a[i]*b;
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Multiply vector with scalar and add to result.
    \param[in]     a     vector
    \param[in]     b     scalar
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Multiply vector with scalar and add to result:
    \f[
        c_i=c_i+a_ib \qquad \forall \;\; 0\le i<len
    \f]
    The values are added to the existing result vector.
 */
/*---------------------------------------------------------------------------*/
void vecMulScalarAdd(const float * const a, const float b, float * const c,
                     const unsigned long int len)
 { for (unsigned long int i=0; i < len; i++)
    c[i]+=a[i]*b;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Add vector elements to scalar.
    \param[in] a     vector to be added
    \param[in] len   length of vectors
    \return resulting scalar

    Add vector elements to scalar:
    \f[
       b=\sum_{i=0}^{len-1}a_i
    \f]
    The values are added to the existing result.
 */
/*---------------------------------------------------------------------------*/
double vecScalarAdd(const float * const a, const unsigned long int len)
 { double b=0.0;

   for (unsigned long int i=0; i < len; i++)
    b+=(double)a[i];
   return(b);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate scaled sum of vector elements.
    \param[in] a     vector
    \param[in] b     scalar
    \param[in] len   length of vectors
    \return resulting scalar

    Calculate the scaled sum of the vector elements:
    \f[
        c=b\sum_{i=0}^{len-1}a_i
    \f]
 */
/*---------------------------------------------------------------------------*/
float vecScaledSum(const float * const a, const float b,
                   const unsigned long int len)
 { return((float)vecScalarAdd(a, len)*b);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate signed square of vector elements.
    \param[in]     a     vector
    \param[in,out] b     result vector
    \param[in]     len   length of vectors

    Calculate signed square of vector elements:
    \f[
       b_i=a_i|a_i| \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecSignedSquare(const float * const a, float * const b,
                     const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vssq(a, 1, b, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    b[i]=a[i]*fabsf(a[i]);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate square of vector elements.
    \param[in]     a     vector
    \param[in,out] b     result vector
    \param[in]     len   length of vectors

    Calculate square of vector elements:
    \f[
       b_i=a_i^2 \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecSquare(const float * const a, float * const b,
               const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vsq(a, 1, b, 1, len);
#else
   vecMul(a, a, b, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Subtract two vectors.
    \param[in]     a     vector from which to subtract
    \param[in]     b     vector to be subtracted
    \param[in,out] c     result vector
    \param[in]     len   length of vectors

    Subtract two vectors:
    \f[
        c_i=a_i-b_i \qquad \forall \;\; 0\le i<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecSub(const float * const a, const float * const b, float * const c,
            const unsigned long int len)
 {
#ifdef __ALTIVEC__
   vsub(a, 1, b, 1, c, 1, len);
#else
   for (unsigned long int i=0; i < len; i++)
    c[i]=a[i]-b[i];
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Subtract two complex vectors.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     br    real part of second vector
    \param[in]     bi    imaginary part of second vector
    \param[in,out] cr    real part of result vector
    \param[in,out] ci    imaginary part of result vector
    \param[in]     len   length of vectors

    Subtract two complex vectors:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})-(b_{r_j};b_{i_j})=
        (a_{r_j}-b_{r_j};a_{i_j}-b_{i_j})
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecSubComplex(const float * const ar, const float * const ai,
                   const float * const br, const float * const bi,
                   float * const cr, float * const ci,
                   const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, b, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   b.realp=&br[0];
   b.imagp=&bi[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   zvsub(&a, 1, &b, 1, &c, 1, len);
#else
   vecSub(ar, br, cr, len);
   vecSub(ai, bi, ci, len);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Subtract real from complex vector.
    \param[in]     ar    real part of first vector
    \param[in]     ai    imaginary part of first vector
    \param[in]     b     second vector
    \param[in,out] cr    real part of result vector
    \param[in,out] ci    imaginary part of result vector
    \param[in]     len   length of vectors

    Subtract real from complex vector:
    \f[
        (c_{r_j};c_{i_j})=(a_{r_j};a_{i_j})-b_j=(a_{r_j}-b_j;a_{i_j})
        \qquad \forall \;\; 0\le j<len
    \f]
 */
/*---------------------------------------------------------------------------*/
void vecSubComplexReal(const float * const ar, const float * const ai,
                       const float * const b, float * const cr,
                       float * const ci, const unsigned long int len)
 {
#ifdef __ALTIVEC__
   DSPSplitComplex a, c;
 
   a.realp=&ar[0];
   a.imagp=&ai[0];
   c.realp=&cr[0];
   c.imagp=&ci[0];
   zrvsub(&a, 1, b, 1, &c, 1, len);
#else
   vecSub(ar, b, cr, len);
   memcpy(ci, ai, len*sizeof(float));
#endif
 }

