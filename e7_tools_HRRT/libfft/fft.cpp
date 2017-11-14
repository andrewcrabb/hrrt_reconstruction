/*! \class FFT fft.h "fft.h"
    \brief This class implements algorithms for the complex and real-valued FFT.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/02 revieldn only used if sequence length is power of 2
    \date 2003/12/02 sofarRadix, actualRadix, remainRadix, revieldn,
                     revieldn_2, wreal initialized with NULL
    \date 2003/12/02 initialize power_of_two with true
    \date 2003/12/02 add workaround for GNU C++ compiler bug
    \date 2004/02/15 Altivec support
    \date 2004/10/01 use vector operations
    \date 2005/01/23 added Doxygen style comments

    Lookup tables for the bitreverse shuffling and the complex exp-function are
    calculated during initialization. The FFT algorithm is a recursive radix-4
    algorithm. The data is copied into a local buffer before transformation, to
    remove the stride and improve the cache usage. The 2d- and 3d-FFTs are
    calculated in parallel. Each thread calculates one block of data (set of
    rows or set of columns). The number of threads is limited by the
    max_threads constant.
 */

#include <iostream>
#include <algorithm>
#include <limits>
#ifdef XEON_HYPERTHREADING_BUG
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <alloca.h>
#endif
#ifdef WIN32
#include <malloc.h>
#endif
#endif
#include "fastmath.h"
#include "fft.h"
#include "exception.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "thread_wrapper.h"
#include "vecmath.h"

/*- constants ---------------------------------------------------------------*/

                       /*! values of complex exp() function for radix 3 FFTs */
const float c3_1=cosf(2.0f*M_PI/3.0f)-1.0f,
            c3_2=sinf(2.0f*M_PI/3.0f),
                       /*! values of complex exp() function for radix 5 FFTs */
            c5_1=(cosf(2.0f*M_PI/5.0f)+cosf(4.0f*M_PI/5.0f))/2.0f-1.0f,
            c5_2=(cosf(2.0f*M_PI/5.0f)-cosf(4.0f*M_PI/5.0f))/2.0f,
            c5_3=-sinf(2.0f*M_PI/5.0f),
            c5_4=-sinf(2.0f*M_PI/5.0f)-sinf(4.0f*M_PI/5.0f),
            c5_5=sinf(2.0f*M_PI/5.0f)-sinf(4.0f*M_PI/5.0f),
                       /*! values of complex exp() function for radix 8 FFTs */
            c8=1.0f/sqrtf(2.0f);

/*- type declarations -------------------------------------------------------*/

 

/*- local functions ---------------------------------------------------------*/

 // The thread API is a C-API so C linkage and calling conventions have to be
 // used, when creating a thread. To use a method as thread, a helper function
 // in C is needed, that calls the method.

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to execute set of FFTs.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated.

    Start thread to execute set of FFTs.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_FFT_block(void *param)
 { try
   { FFT::tFFTthread_params *tp;

     tp=(FFT::tFFTthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
      tp->object->FFT_block_thread(tp->real, tp->imaginary, tp->buffer,
                                   tp->num, tp->loffs, tp->offs,
                                   tp->forward, tp->table_number);
      return(NULL);
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to execute set of rFFTs.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated.

    Start thread to execute set of rFFTs.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_rFFT_block(void *param)
 { try
   { FFT::trFFTthread_params *tp;

     tp=(FFT::trFFTthread_params *)param;
#ifdef XEON_HYPERTHREADING_BUG
      // allocate some padding memory on the stack in front of the thread-stack
      // to avoid cache conflicts while accessing local variables
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     alloca(tp->thread_number*STACK_OFFSET);
#endif
#ifdef WIN32
     _alloca(tp->thread_number*STACK_OFFSET);
#endif
#endif
     tp->object->rFFT_block_thread(tp->data, tp->buffer, tp->num, tp->loffs,
                                   tp->offs, tp->forward, tp->table_number,
                                   tp->type2);
     return(NULL);
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Constructor for 1D FFT.
    \param[in] _nx   number of values to transform
    \exception REC_FFT_DIM_TOO_SMALL dimension of matrix for FFT is too small
    \exception REC_FFT_DIM_NOT_EVEN dimension of matrix for FFT is not an even
                                    number

    Constructor for 1D FFT. Initialize all required lookup tables.
 */
/*---------------------------------------------------------------------------*/
FFT::FFT(const unsigned long int _nx)
 { unsigned short int i;

   for (i=0; i < 3; i++)
    { tab_alloc[i]=false;
      sofarRadix[i]=NULL;
      actualRadix[i]=NULL;
      remainRadix[i]=NULL;
      power_of_two[i]=true;
#ifndef __ALTIVEC__
      tabs_ft[i].revieldn=NULL;
      tabs_ft[i].revieldn_2=NULL;
      tabs_ft[i].wreal=NULL;
#endif
    }
   for (i=0; i <= MAX_RADIX; i++)
    trig[i]=NULL;
   n[0]=_nx;
   if (n[0] <  8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("x");
   if (n[0] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("x");
   InitTablesFT_1D();
   dim=1;
#ifdef __ALTIVEC__
   if (power_of_two[0])
    { log2_n[0]=Bits(n[0]);
      fft_setup=create_fftsetup(log2_n[0], FFT_RADIX2);
    }
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Constructor for 2D FFT.
    \param[in] _nx   number of values to transform in x-direction
    \param[in] _ny   number of values to transform in y-direction
    \exception REC_FFT_DIM_TOO_SMALL dimension of matrix for FFT is too small
    \exception REC_FFT_DIM_NOT_EVEN dimension of matrix for FFT is not an even
                                    number

    Constructor for 2D FFT. Initialize all required lookup tables.
 */
/*---------------------------------------------------------------------------*/
FFT::FFT(const unsigned long int _nx, const unsigned long int _ny)
 { unsigned short int i;

   for (i=0; i < 3; i++)
    { tab_alloc[i]=false;
      sofarRadix[i]=NULL;
      actualRadix[i]=NULL;
      remainRadix[i]=NULL;
      power_of_two[i]=true;
#ifndef __ALTIVEC__
      tabs_ft[i].revieldn=NULL;
      tabs_ft[i].revieldn_2=NULL;
      tabs_ft[i].wreal=NULL;
#endif
    }
   for (i=0; i <= MAX_RADIX; i++)
    trig[i]=NULL;
   n[0]=_nx;
   if (n[0] < 8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("x");
   if (n[0] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("x");
   n[1]=_ny;
   if (n[1] < 8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("y");
   if (n[1] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("y");
   InitTablesFT_2D();
   dim=2;
#ifdef __ALTIVEC__
   unsigned short int log2n=0;

   if (power_of_two[0]) { log2_n[0]=Bits(n[0]);
                          log2n=log2_n[0];
                        }
   if (power_of_two[1]) { log2_n[1]=Bits(n[1]);
                          log2n=std::max(log2n, log2_n[1]);
                        }
   if (log2n > 0) fft_setup=create_fftsetup(log2n, FFT_RADIX2);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Constructor for 3D FFT.
    \param[in] _nx   number of values to transform in x-direction
    \param[in] _ny   number of values to transform in y-direction
    \param[in] _nz   number of values to transform in z-direction
    \exception REC_FFT_DIM_TOO_SMALL dimension of matrix for FFT is too small
    \exception REC_FFT_DIM_NOT_EVEN dimension of matrix for FFT is not an even
                                    number

    Constructor for 3D FFT. Initialize all required lookup tables.
 */
/*---------------------------------------------------------------------------*/
FFT::FFT(const unsigned long int _nx, const unsigned long int _ny,
         const unsigned long int _nz)
 { unsigned short int i;

   for (i=0; i < 3; i++)
    { tab_alloc[i]=false;
      sofarRadix[i]=NULL;
      actualRadix[i]=NULL;
      remainRadix[i]=NULL;
      power_of_two[i]=true;
#ifndef __ALTIVEC__
      tabs_ft[i].revieldn=NULL;
      tabs_ft[i].revieldn_2=NULL;
      tabs_ft[i].wreal=NULL;
#endif
    }
   for (i=0; i <= MAX_RADIX; i++)
    trig[i]=NULL;
   n[0]=_nx;
   if (n[0] < 8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("x");
   if (n[0] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("x");
   n[1]=_ny;
   if (n[1] < 8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("y");
   if (n[1] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("y");
   n[2]=_nz;
   if (n[2] < 8)
    throw Exception(REC_FFT_DIM_TOO_SMALL,
                    "#1-dimension of matrix for FFT is too small.").arg("z");
   if (n[2] % 2)
    throw Exception(REC_FFT_DIM_NOT_EVEN,
                    "#1-dimension of matrix for FFT is not an even "
                    "number.").arg("z");
   InitTablesFT_3D();
   dim=3;
#ifdef __ALTIVEC__
   unsigned short int log2n=0;

   if (power_of_two[0]) { log2_n[0]=Bits(n[0]);
                          log2n=log2_n[0];
                        }
   if (power_of_two[1]) { log2_n[1]=Bits(n[1]);
                          log2n=std::max(log2n, log2_n[1]);
                        }
   if (power_of_two[2]) { log2_n[2]=Bits(n[2]);
                          log2n=std::max(log2n, log2_n[2]);
                        }
   if (log2n > 0) fft_setup=create_fftsetup(log2n, FFT_RADIX2);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destructor.

    Destructor. Delete lookup tables.
 */
/*---------------------------------------------------------------------------*/
FFT::~FFT()
 { unsigned short int i;

   for (i=0; i < 3; i++)
    if (tab_alloc[i])
     if (power_of_two[i])
      {
#ifndef __ALTIVEC__
        if (tabs_ft[i].revieldn != NULL) delete[] tabs_ft[i].revieldn;
        if (tabs_ft[i].revieldn_2 != NULL) delete[] tabs_ft[i].revieldn_2;
        if (tabs_ft[i].wreal != NULL) delete[] tabs_ft[i].wreal;
#endif
      }
      else { if (sofarRadix[i] != NULL) delete[] sofarRadix[i];
             if (actualRadix[i] != NULL) delete[] actualRadix[i];
             if (remainRadix[i] != NULL) delete[] remainRadix[i];
           }
   for (i=0; i <= MAX_RADIX; i++)
    if (trig[i] != NULL) delete[] trig[i];
#ifdef __ALTIVEC__
   if (power_of_two[0] ||
       (power_of_two[1] && (dim > 1)) ||
       (power_of_two[2] && (dim > 2))) destroy_fftsetup(fft_setup);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Determine which power of 2 a number is.
    \param[in] n   number
    \return power of 2

    Determine which power of 2 a number is.
 */
/*---------------------------------------------------------------------------*/
unsigned short int FFT::Bits(unsigned long int n) const
 { unsigned short int b=0;

   while (n > 1) { b++;
                   n>>=1;
                 }
   return(b);
 }

#ifndef __ALTIVEC__
/*---------------------------------------------------------------------------*/
/*! \brief Create cosinus-tables for transform.
    \param[in] n              number of values to transform
    \param[in] table_number   number of table

    Create cosinus-tables for transform.
 */
/*---------------------------------------------------------------------------*/
void FFT::exp_table(const unsigned long int n,
                    const unsigned short int table_number)
 { tabs_ft[table_number].wreal=NULL;
   try
   { tabs_ft[table_number].wreal=new double[3*n/4];      // table for real part
                                                    // table for imaginary part
     tabs_ft[table_number].wimag=tabs_ft[table_number].wreal+n/4;
     for (unsigned long int x=0; x < 3*n/4; x++)
      { double v;

        v=2.0*M_PI*(double)x/(double)n;
        tabs_ft[table_number].wreal[x]=cos(v);
      }
   }
   catch (...)
    { if (tabs_ft[table_number].wreal != NULL)
       { delete[] tabs_ft[table_number].wreal;
         tabs_ft[table_number].wreal=NULL;
       }
      throw;
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Factorize value into 2,3,4,5,8,10 and bigger factors.
    \param[in]  n       value to factorize
    \param[out] nFact   number of factors
    \param[out] fact    sorted array of factors (smallest first)
    \exception REC_FFT_RADIX_TOO_LARGE can't calculate FFT with given radix

    Factorize value into 2,3,4,5,8,10 and bigger factors.
 */
/*---------------------------------------------------------------------------*/
void FFT::factorize(unsigned long int n, unsigned short int * const nFact,
                    unsigned short int * const fact)
 { unsigned short int j, i;

   n/=2;                                     // take care that factor 2 remains
   { unsigned short int radices[6];
                                                // extract factors 2,3,4,5,8,10
     radices[0]=10;
     radices[1]=8;
     radices[2]=5;
     radices[3]=4;
     radices[4]=3;
     radices[5]=2;
     j=0;
     i=0;
     while ((n > 1) && (i < 6))
      if ((n % radices[i]) == 0)
       { n/=radices[i];
         fact[j++]=radices[i];
       }
       else i++;
   }
                                                        // convert 2*8 into 4*4
   if ((fact[j-1] == 2) && (j > 1))
    { for (signed short int i=j-2; i >= 0 ; i--)
       if (fact[i] == 8) { fact[j-1]=4;
                           fact[i]=4;
                           break;
                         }
    }
   if (n > 1)                      // find remaining factors (not 2,3,4,5,8,10)
    {
      for (unsigned short int k=2; k < sqrtf(n)+1; k++)
       while ((n % k) == 0)
        { n/=k;
          fact[j++]=k;
        }
      if (n > 1) fact[j++]=n;
    }
   fact[j++]=2;                                                 // add factor 2
                          // mirror sequence of factors (smallest factor first)
   for (i=0; i < j/2; i++)
    std::swap(fact[i], fact[j-i-1]);
   *nFact=j;
                                        // store trigonometric factors in table
   for (i=0;  i < j; i++)
    if (fact[i] > MAX_RADIX)
     throw Exception(REC_FFT_RADIX_TOO_LARGE,
                     "Can't calculate FFT with radix #1.").arg(fact[i]);
     else if (trig[fact[i]] == NULL)
           if ((fact[i] > 5) && (fact[i] != 8) && (fact[i] != 10))
            trig[fact[i]]=initTrig(fact[i]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 2.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 2:
    \f[
        r'_0=r_0+r_1
    \f]
    \f[
        r'_1=r_0-r_1
    \f]
    \f[
        i'_0=i_0+i_1
    \f]
    \f[
        i'_1=i_0-i_1
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_2(float * const rbuff, float * const ibuff) const
 { float tmp;

   tmp=rbuff[0]+rbuff[1];
   rbuff[1]=rbuff[0]-rbuff[1];
   rbuff[0]=tmp;
   tmp=ibuff[0]+ibuff[1];
   ibuff[1]=ibuff[0]-ibuff[1];
   ibuff[0]=tmp;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 3.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 3:
    \f[
        r'_0=r_0+r_1+r_2
    \f]
    \f[
        r'_1=r_0+c_{3_1}(r_1+r_2)+c_{3_2}(i_1-i_2)
    \f]
    \f[
        r'_2=r_0+c_{3_1}(r_1+r_2)-c_{3_2}(i_1-i_2)
    \f]
    \f[
        i'_0=i_0+i_1+i_2
    \f]
    \f[
        i'_1=i_0+c_{3_1}(i_1+i_2)+c_{3_2}(r_2-r_1)
    \f]
    \f[
        i'_2=i_0+c_{3_1}(i_1+i_2)-c_{3_2}(r_2-r_1)
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_3(float * const rbuff, float * const ibuff) const
 { float t1, t2, t3, t4, t5, t6;

   t1=rbuff[1]+rbuff[2];
   t5=c3_2*(rbuff[2]-rbuff[1]);
   t4=ibuff[1]+ibuff[2];
   t2=c3_2*(ibuff[1]-ibuff[2]);
   rbuff[0]+=t1;
   ibuff[0]+=t4;
   t3=rbuff[0]+t1*c3_1;
   t6=ibuff[0]+t4*c3_1;
   rbuff[1]=t3+t2;
   rbuff[2]=t3-t2;
   ibuff[1]=t6+t5;
   ibuff[2]=t6-t5;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 4.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 4:
    \f[
        r'_0=r_0+r_1+r_2+r_3
    \f]
    \f[
        r'_1=r_0-r_2+i_1-i_3
    \f]
    \f[
        r'_2=r_0-r_1+r_2-r_3
    \f]
    \f[
        r'_3=r_0-r_2-i_1+i_3
    \f]
    \f[
        i'_0=i_0+i_1+i_2+i_3
    \f]
    \f[
        i'_1=i_0-i_2+r_3-r_1
    \f]
    \f[
        i'_2=i_0-i_1+i_2-i_3
    \f]
    \f[
        i'_3=i_0-i_2-r_3+r_1
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_4(float * const rbuff, float * const ibuff) const
 { float t1, t2, t3, t4, t5, t6, t7, t8;

   t1=rbuff[0]+rbuff[2];
   t3=rbuff[0]-rbuff[2];
   t2=rbuff[1]+rbuff[3];
   t8=rbuff[3]-rbuff[1];
   t5=ibuff[0]+ibuff[2];
   t7=ibuff[0]-ibuff[2];
   t6=ibuff[1]+ibuff[3];
   t4=ibuff[1]-ibuff[3];
   rbuff[0]=t1+t2;
   rbuff[1]=t3+t4;
   rbuff[2]=t1-t2;
   rbuff[3]=t3-t4;
   ibuff[0]=t5+t6;
   ibuff[1]=t7+t8;
   ibuff[2]=t5-t6;
   ibuff[3]=t7-t8;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 5.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 5:
    \f[
        r'_0=r_0+r_1+r_2+r_3+r_4
    \f]
    \f[
        r'_1=r_1+r_4+c_{5_2}(r_1-r_2-r_3+r_4)-c_{5_3}(i_1-i_2+i_3-i_4)+
             c_{5_4}(i_3-i_2)
    \f]
    \f[
        r'_2=r-1+r_4-c_{5_2}(r_1-r_2-r_3+r_4)-c_{5_3}(i_1-i_2+i_3-i_4)-
             c_{5_5}(i_1-i_4)
    \f]
    \f[
        r'_3=r-1+r_4-c_{5_2}(r_1-r_2-r_3+r_4)+c_{5_3}(i_1-i_2+i_3-i_4)+
             c_{5_5}(i_1-i_4)
    \f]
    \f[
        r'_4=r_1+r_4+c_{5_2}(r_1-r_2-r_3+r_4)+c_{5_3}(i_1-i_2+i_3-i_4)-
             c_{5_4}(i_3-i_2)
    \f]
    \f[
        i'_0=i_0+i_1+i_2+i_3+i_4
    \f]
    \f[
        i'_1=i_1+i_4+c_{5_2}(i_1-i_2-i_3+i_4)+c_{5_3}(r_1-r_2+r_3-r_4)-
             c_{5_4}(r_3-r_2)
    \f]
    \f[
        i'_2=i_1+i_4-c_{5_2}(i_1-i_2-i_3+i_4)+c_{5_3}(r_1-r_2+r_3-r_4)+
             c_{5_5}(r_1-r_4)
    \f]
    \f[
        i'_3=i_1+i_4-c_{5_2}(i_1-i_2-i_3+i_4)-c_{5_3}(r_1-r_2+r_3-r_4)-
             c_{5_5}(r_1-r_4)
    \f]
    \f[
        i'_4=i_1+i_4+c_{5_2}(i_1-i_2-i_3+i_4)-c_{5_3}(r_1-r_2+r_3-r_4)+
             c_{5_4}(r_3-r_2)
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_5(float * const rbuff, float * const ibuff) const
 { float t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16,
         t17, t18, t19, t20;

   t1=rbuff[1]+rbuff[4];
   t3=rbuff[1]-rbuff[4];
   t2=rbuff[2]+rbuff[3];
   t4=rbuff[3]-rbuff[2];
   t6=ibuff[1]+ibuff[4];
   t8=ibuff[1]-ibuff[4];
   t7=ibuff[2]+ibuff[3];
   t9=ibuff[3]-ibuff[2];
   t5=t1+t2;
   t10=t6+t7;
   rbuff[0]+=t5;
   ibuff[0]+=t10;

   t11=c5_1*t5;
   t16=c5_1*t10;
   t12=c5_2*(t1-t2);
   t17=c5_2*(t6-t7);
   t13=-c5_3*(t8+t9);
   t18=c5_3*(t3+t4);
   t14=-c5_4*t9;
   t19=c5_4*t4;
   t15=-c5_5*t8;
   t20=c5_5*t3;

   t3=t13-t14;
   t5=t13+t15;
   t8=t18-t19;
   t10=t18+t20;
   t1=rbuff[0]+t11;
   t2=t1+t12;
   t4=t1-t12;
   t6=ibuff[0]+t16;
   t7=t6+t17;
   t9=t6-t17;

   rbuff[1]=t2+t3;
   rbuff[2]=t4+t5;
   rbuff[3]=t4-t5;
   rbuff[4]=t2-t3;
   ibuff[1]=t7+t8;
   ibuff[2]=t9+t10;
   ibuff[3]=t9-t10;
   ibuff[4]=t7-t8;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 8.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 8:
    \f[
       (r',i')=fft_4({r_1,r_3,r_5,r_7},{i_1,i_3,i_5,i_7})
    \f]
    \f[
       (r'',i'')=fft_4({r_0,r_2,r_4,r_6},{i_0,i_2,i_4,i_6})
    \f]
    \f[
        r'''_0=r''_0+i'_0
    \f]
    \f[
        r'''_1=r''_1+c_8(r'_1+i'_1)
    \f]
    \f[
        r'''_2=r''_2+i'_2
    \f]
    \f[
        r'''_3=r''_3+c_8(i'_3-r'_3)
    \f]
    \f[
        r'''_4=r''_0-i'_0
    \f]
    \f[
        r'''_5=r''_1-c_8(r'_1+i'_1)
    \f]
    \f[
        r'''_6=r''_2-i'_2
    \f]
    \f[
        r'''_7=r''_3-c_8(i'_3-r'_3)
    \f]
    \f[
        i'''_0=i''_0+i'_0
    \f]
    \f[
        i'''_1=i''_1+c_8(i'_1-r'_1)
    \f]
    \f[
        i'''_2=i''_2-r'_2
    \f]
    \f[
        i'''_3=i''_3-c_8(r'_3+i'_3)
    \f]
    \f[
        i'''_4=i''_0-i'_0
    \f]
    \f[
        i'''_5=i''_1-c_8(i'_1-r'_1)
    \f]
    \f[
        i'''_6=i''_2+r'_2
    \f]
    \f[
        i'''_7=i''_3+c_8(r'_3+i'_3)
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_8(float * const rbuff, float * const ibuff) const
 { float t, tmp_wre[4], tmp_wim[4];

   tmp_wre[0]=rbuff[1];
   tmp_wre[1]=rbuff[3];
   tmp_wre[2]=rbuff[5];
   tmp_wre[3]=rbuff[7];
   tmp_wim[0]=ibuff[1];
   tmp_wim[1]=ibuff[3];
   tmp_wim[2]=ibuff[5];
   tmp_wim[3]=ibuff[7];
   fft_4(tmp_wre, tmp_wim);

   t=c8*(tmp_wre[1]+tmp_wim[1]);
   tmp_wim[1]=c8*(tmp_wim[1]-tmp_wre[1]);
   tmp_wre[1]=t;
   t=tmp_wim[2];
   tmp_wim[2]=-tmp_wre[2];
   tmp_wre[2]=t;
   t=c8*(tmp_wim[3]-tmp_wre[3]);
   tmp_wim[3]=-c8*(tmp_wre[3]+tmp_wim[3]);
   tmp_wre[3]=t;

   rbuff[1]=rbuff[2];
   rbuff[2]=rbuff[4];
   rbuff[3]=rbuff[6];

   ibuff[1]=ibuff[2];
   ibuff[2]=ibuff[4];
   ibuff[3]=ibuff[6];
   fft_4(rbuff, ibuff);

   memcpy(rbuff+4, rbuff, 4*sizeof(float));

   rbuff[0]+=tmp_wre[0];
   rbuff[4]-=tmp_wre[0];
   rbuff[1]+=tmp_wre[1];
   rbuff[5]-=tmp_wre[1];
   rbuff[2]+=tmp_wre[2];
   rbuff[6]-=tmp_wre[2];
   rbuff[3]+=tmp_wre[3];
   rbuff[7]-=tmp_wre[3];

   memcpy(ibuff+4, ibuff, 4*sizeof(float));

   ibuff[0]+=tmp_wim[0];
   ibuff[4]-=tmp_wim[0];
   ibuff[1]+=tmp_wim[1];
   ibuff[5]-=tmp_wim[1];
   ibuff[2]+=tmp_wim[2];
   ibuff[6]-=tmp_wim[2];
   ibuff[3]+=tmp_wim[3];
   ibuff[7]-=tmp_wim[3];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for length 10.
    \param[in,out] rbuff   real part of sequence
    \param[in,out] ibuff   imaginary part of sequence

    Calculate FFT for length 10:
    \f[
       (r',i')=fft_5({r_5,r_7,r_9,r_1,r_3},{i_5,i_7,i_9,i_1,i_3})
    \f]
    \f[
       (r'',i'')=fft_5({r_0,r_2,r_4,r_6,r_8},{i_0,i_2,i_4,i_6,i_8})
    \f]
    \f[
        r'''_0=r''_0+r'_0
    \f]
    \f[
        r'''_1=r''_1-r'_1
    \f]
    \f[
        r'''_2=r''_2+r'_2
    \f]
    \f[
        r'''_3=r''_3-r'_3
    \f]
    \f[
        r'''_4=r''_4+r'_4
    \f]
    \f[
        r'''_5=r''_0-r'_0
    \f]
    \f[
        r'''_6=r''_1+r'_1
    \f]
    \f[
        r'''_7=r''_2-r'_2
    \f]
    \f[
        r'''_8=r''_3+r'_3
    \f]
    \f[
        r'''_9=r''_4-r'_4
    \f]
    \f[
        i'''_0=i''_0+i'_0
    \f]
    \f[
        i'''_1=i''_1-i'_1
    \f]
    \f[
        i'''_2=i''_2+i'_2
    \f]
    \f[
        i'''_3=i''_3-i'_3
    \f]
    \f[
        i'''_4=i''_4+i'_4
    \f]
    \f[
        i'''_5=i''_0-i'_0
    \f]
    \f[
        i'''_6=i''_1+i'_1
    \f]
    \f[
        i'''_7=i''_2-i'_2
    \f]
    \f[
        i'''_8=i''_3+i'_3
    \f]
    \f[
        i'''_9=i''_4-i'_4
    \f]
 */
/*---------------------------------------------------------------------------*/
inline void FFT::fft_10(float * const rbuff, float * const ibuff) const
 { float tmp_wre[5], tmp_wim[5];

   tmp_wre[0]=rbuff[5];
   tmp_wre[1]=rbuff[7];
   tmp_wre[2]=rbuff[9];
   tmp_wre[3]=rbuff[1];
   tmp_wre[4]=rbuff[3];

   tmp_wim[0]=ibuff[5];
   tmp_wim[1]=ibuff[7];
   tmp_wim[2]=ibuff[9];
   tmp_wim[3]=ibuff[1];
   tmp_wim[4]=ibuff[3];
   fft_5(tmp_wre, tmp_wim);

   rbuff[0]=rbuff[0];
   rbuff[1]=rbuff[2];
   rbuff[2]=rbuff[4];
   rbuff[3]=rbuff[6];
   rbuff[4]=rbuff[8];

   ibuff[0]=ibuff[0];
   ibuff[1]=ibuff[2];
   ibuff[2]=ibuff[4];
   ibuff[3]=ibuff[6];
   ibuff[4]=ibuff[8];
   fft_5(rbuff, ibuff);

   memcpy(rbuff+5, rbuff, 5*sizeof(float));

   rbuff[0]+=tmp_wre[0];
   rbuff[5]-=tmp_wre[0];
   rbuff[6]+=tmp_wre[1];
   rbuff[1]-=tmp_wre[1];
   rbuff[2]+=tmp_wre[2];
   rbuff[7]-=tmp_wre[2];
   rbuff[8]+=tmp_wre[3];
   rbuff[3]-=tmp_wre[3];
   rbuff[4]+=tmp_wre[4];
   rbuff[9]-=tmp_wre[4];

   memcpy(ibuff+5, ibuff, 5*sizeof(float));

   ibuff[0]+=tmp_wim[0];
   ibuff[5]-=tmp_wim[0];
   ibuff[6]+=tmp_wim[1];
   ibuff[1]-=tmp_wim[1];
   ibuff[2]+=tmp_wim[2];
   ibuff[7]-=tmp_wim[2];
   ibuff[8]+=tmp_wim[3];
   ibuff[3]-=tmp_wim[3];
   ibuff[4]+=tmp_wim[4];
   ibuff[9]-=tmp_wim[4];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for radix other than 2,3,4,5,8,10.
    \param[in]     radix    radix of FFT
    \param[in]     trigRe   trigonometric factors
    \param[in,out] rbuff    real part of sequence
    \param[in,out] ibuff    imaginary part of sequence

    Calculate FFT for radix other than 2,3,4,5,8,10.
 */
/*---------------------------------------------------------------------------*/
void FFT::fft_odd(const unsigned short int radix, float * const trigRe,
                  float * const rbuff, float * const ibuff) const
 { std::vector <float> tmp_vre;
   unsigned short int j, max, rj;
   float *trigIm, *tmp_vim, *tmp_wre, *tmp_wim;;

   trigIm=trigRe+radix;
   max=(radix+1)/2;
   tmp_vre.resize(max*4);
   tmp_vim=&tmp_vre[0]+max;
   tmp_wre=&tmp_vim[0]+max;
   tmp_wim=&tmp_wre[0]+max;
   for (rj=radix-1,
        j=1; j < max; j++,
        rj--)
    { float t1, t2;

      t1=rbuff[j];
      t2=rbuff[rj];
      tmp_vre[j]=t1+t2;
      tmp_wre[j]=t1-t2;
      t1=ibuff[j];
      t2=ibuff[rj];
      tmp_vim[j]=t1-t2;
      tmp_wim[j]=t1+t2;
    }
   for (rj=radix-1,
        j=1; j < max; j++,
        rj--)
    { unsigned short int k;

      rbuff[j]=rbuff[0];
      ibuff[j]=ibuff[0];
      rbuff[rj]=rbuff[0];
      ibuff[rj]=ibuff[0];
      k=j;
      for (unsigned short int i=1; i < max; i++)
       { float rere, reim, imre, imim;

         rere=trigRe[k]*tmp_vre[i];
         reim=trigRe[k]*tmp_wim[i];
         imim=trigIm[k]*tmp_vim[i];
         imre=trigIm[k]*tmp_wre[i];
         rbuff[rj]+=rere+imim;
         rbuff[j]+=rere-imim;
         ibuff[rj]+=reim-imre;
         ibuff[j]+=reim+imre;
         k+=j;
         if (k >= radix) k-=radix;
       }
    }
   for (j=1; j < max; j++)
    rbuff[0]+=tmp_vre[j];
   for (j=1; j < max; j++)
    ibuff[0]+=tmp_wim[j];
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D-FFT (in-place).
    \param[in,out] real        pointer to real part of dataset
    \param[in,out] imaginary   pointer to imaginary part of dataset
    \param[in]     forward     transformation into frequency domain ?

    1D-FFT of a real and imaginary vector (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_1D(float * const real, float * const imaginary,
                 const bool forward) const
 { std::vector <float> buffer;

   buffer.resize(n[0]*2);
   four(real, imaginary, &buffer[0], 1, forward, 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D-FFT for several sequences (in-place).
    \param[in,out] real          pointer to real part of dataset
    \param[in,out] imaginary     pointer to imaginary part of dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     num           number of 1D-FFTs
    \param[in]     loffs         offset to next 1D-FFT
    \param[in]     offs          offset to next element
    \param[in]     max_threads   maximum number of threads to use

    1D-FFT for several real and imaginary vectors (in-place). This method is
    multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_1D_block(float * const real, float * const imaginary,
                       const bool forward, const unsigned long int num,
                       const unsigned long int loffs,
                       const unsigned long int offs,
                       const unsigned short int max_threads)
 { std::vector <float> buffer;

   buffer.resize(n[0]*2*(unsigned long int)max_threads);
   FFT_block(real, imaginary, &buffer[0], num, loffs, offs, forward, 0,
             max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D-FFT of complex data (in-place).
    \param[in] cplx      pointer to complex dataset (r0,i0,r1,i1,...)
    \param[in] forward   transformation into frequency domain ?
    \param[in] offs      offset to next complex element

    1D-FFT of complex data (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_1D_cplx(float * const cplx, const bool forward,
                      const unsigned long int offs) const
 { std::vector <float> buffer;

   buffer.resize(n[0]*2);
   four(cplx, cplx+1, &buffer[0], 2*offs, forward, 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 2D-FFT (in-place).
    \param[in,out] real          pointer to real part of dataset
    \param[in,out] imaginary     pointer to imaginary part of dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     max_threads   maximum number of threads to use
    \excception REC_FFT_INIT FFT object is not initialized for FFT of this
                             dimension

    2D-FFT of a real and imaginary vector (in-place). First the rows are
    transformed, then the columns. This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_2D(float * const real, float * const imaginary,
                 const bool forward, const unsigned short int max_threads)
 { if (dim < 2)
    throw Exception(REC_FFT_INIT,
                    "FFT object is not initialized for #1D-FFT.").arg("2");
   std::vector <float> buffer;                // buffer for data without stride

   buffer.resize(std::max(n[0], n[1])*2*(unsigned long int)max_threads);
                                                      // transformation of rows
   FFT_block(real, imaginary, &buffer[0], n[1], n[0], 1, forward, 0,
             max_threads);
                                                   // transformation of columns
   FFT_block(real, imaginary, &buffer[0], n[0], 1, n[0], forward, 1,
             max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 2D-FFT of complex data (in-place).
    \param[in,out] cplx          pointer to complex dataset (r0,i0,r1,i1,...)
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     max_threads   maximum number of threads to use
    \excception REC_FFT_INIT FFT object is not initialized for FFT of this
                             dimension

    2D-FFT of complex data (in-place). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_2D_cplx(float * const cplx, const bool forward,
                      const unsigned short int max_threads)
 { if (dim < 2)
    throw Exception(REC_FFT_INIT,
                    "FFT object is not initialized for #1D-FFT.").arg("2");
   std::vector <float> buffer;
 
   buffer.resize(std::max(n[0], n[1])*2*(unsigned long int)max_threads);
                                                      // transformation of rows
   FFT_block(cplx, cplx+1, &buffer[0], n[1], n[0]*2, 2, forward, 0,
             max_threads);
                                                   // transformation of columns
   FFT_block(cplx, cplx+1, &buffer[0], n[0], 2, n[0]*2, forward, 1,
             max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 3D-FFT (in-place).
    \param[in,out] real          pointer to real part of dataset
    \param[in,out] imaginary     pointer to imaginary part of dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     max_threads   maximum number of threads to use
    \excception REC_FFT_INIT FFT object is not initialized for FFT of this
                             dimension

    3D-FFT of a real and imaginary vector (in-place). First the rows are
    transformed, then the columns. This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_3D(float * const real, float * const imaginary,
                 const bool forward, const unsigned short int max_threads)
 { if (dim < 3)
    throw Exception(REC_FFT_INIT,
                    "FFT object is not initialized for #1D-FFT.").arg("3");
   std::vector <float> buffer;
   unsigned long int nxy;

   buffer.resize(std::max(std::max(n[0], n[1]), n[2])*2*
                 (unsigned long int)max_threads);
   nxy=n[0]*n[1];
   for (unsigned long int z=0; z < n[2]; z++)
    {                                                 // transformation of rows
      FFT_block(&real[z*nxy], &imaginary[z*nxy], &buffer[0], n[1], n[0], 1,
                forward, 0, max_threads);
                                                   // transformation of columns
      FFT_block(&real[z*nxy], &imaginary[z*nxy], &buffer[0], n[0], 1, n[0],
                forward, 1, max_threads);
    }
                                               // transformation in z direction
   FFT_block(real, imaginary, &buffer[0], nxy, 1, nxy, forward, 2,
             max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D-FFT of data block (in-place).
    \param[in,out] real           pointer to real part of dataset
    \param[in,out] imaginary      pointer to imaginary part of dataset
    \param[in]     buffer         buffer for data without stride
    \param[in]     num            number of 1D-FFTs
    \param[in]     loffs          offset to next 1D-FFT
    \param[in]     offs           offset to next element
    \param[in]     forward        transformation into frequency domain ?
    \param[in]     table_number   number of exp- and bitshuffling-table
    \param[in]     max_threads    maximum number of threads to use

    1D-FFT of data block (in-place). This method is multi-threaded. Every
    thread processes a series of vectors.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_block(float * const real, float * const imaginary,
                    float * const buffer, unsigned long int num,
                    const unsigned long int loffs,
                    const unsigned long int offs, const bool forward,
                    const unsigned short int table_number,
                    const unsigned short int max_threads)
 { try
   { unsigned short int threads=0, t;
     void *thread_result;
     std::vector <bool> thread_running;
     std::vector <tthread_id> tid;

     try
     { threads=(unsigned short int)std::min((unsigned long int)max_threads,
                                            num);
       if (threads == 1)
        { FFT_block_thread(real, imaginary, buffer, num, loffs, offs, forward,
                           table_number);
          return;
        }
       std::vector <tFFTthread_params> tp;
       unsigned short int i;
       float *rp, *ip;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (rp=real,
            ip=imaginary,
            t=threads,
            i=0; i < threads; i++,
            t--)
        { tp[i].object=this;
          tp[i].real=rp;
          tp[i].imaginary=ip;
          tp[i].buffer=&buffer[(unsigned long int)i*n[table_number]*2];
          tp[i].num=num/t;
          tp[i].loffs=loffs;
          tp[i].offs=offs;
          tp[i].forward=forward;
          tp[i].table_number=table_number;
#ifdef XEON_HYPERTHREADING_BUG
          tp[i].thread_number=i;
#endif
          thread_running[i]=true;
          ThreadCreate(&tid[i], &executeThread_FFT_block, &tp[i]);
          num-=tp[i].num;
          rp+=(unsigned long int)tp[i].num*loffs;
          ip+=(unsigned long int)tp[i].num*loffs;
        }
       for (i=0; i < threads; i++)
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
     }
     catch (...)
      { thread_result=NULL;
        for (t=0; t < thread_running.size(); t++)
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
                        // if we catch exceptions from the terminating threads,
                        // we store only the first one and ignore the others
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
                    // if the threads have thrown exceptions we throw the first
                    // one of them, not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D-FFT of data block (in-place).
    \param[in,out] real           pointer to real part of dataset
    \param[in,out] imaginary      pointer to imaginary part of dataset
    \param[in]     buffer         buffer for data without stride
    \param[in]     num            number of 1D-FFTs
    \param[in]     loffs          offset to next 1D-FFT
    \param[in]     offs           offset to next element
    \param[in]     forward        transformation into frequency domain ?
    \param[in]     table_number   number of exp- and bitshuffling-table

    1D-FFT of data block (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_block_thread(float * const real, float * const imaginary,
                           float * const buffer, const unsigned long int num,
                           const unsigned long int loffs,
                           const unsigned long int offs, const bool forward,
                           const unsigned short int table_number) const
 { for (unsigned long int i=0; i < num; i++)
    four(&real[i*loffs], &imaginary[i*loffs], buffer, offs, forward,
         table_number);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert symmetric complex FT into real valued FT.
    \param[in,out] real        pointer to real part of dataset
                       (input: r0, r1, r2, r3, r4, r3, r2, r1)
                       (output: r0, r4, r1, i1, r2, i2, r3, i3)
    \param[in] imaginary   pointer to imaginary part of dataset
                           0, i1, i2, i3, 0, -i3, -i2, -i1

    Convert symmetric complex FT into real valued FT.
 */
/*---------------------------------------------------------------------------*/
void FFT::FFT_rFFT_1D(float * const real, float * const imaginary) const
 { float tmp;

   tmp=real[n[0]/2];
   vecCopySplCplxIntlCplx(&real[n[0]/4], &imaginary[n[0]/4], &real[n[0]/2],
                          n[0]/4);
   for (unsigned short int i=n[0]/2-2; i > 1; i-=2)
    { real[i]=real[i/2];
      real[i+1]=imaginary[i/2];
    }
   real[1]=tmp;
 }

/*---------------------------------------------------------------------------*/
/*! \brief 1D Fast Fourier Transform (in-place).
    \param[in,out] real           pointer to real part of dataset
    \param[in,out] imaginary      pointer to imaginary part of dataset
    \param[in]     rbuff          pointer to buffer for dataset
    \param[in]     offs           offset to next element
    \param[in]     forward        transformation into frequency domain ?
    \param[in]     table_numzber   number of exp- and bitshuffling-table

    1D Fast Fourier Transform (in-place). The data is copied into a buffer to
    remove the stride, then a recursive radix-4 FFT is calculated and finally
    the data is copied back into its original location.
 */
/*---------------------------------------------------------------------------*/
void FFT::four(float * const real, float * const imaginary, float *rbuff,
               const unsigned long int offs, const bool forward,
               const unsigned short int table_number) const
 { float *ibuff;
   unsigned long int i, *bsp;
   unsigned short int nval;

   nval=n[table_number];
   ibuff=rbuff+nval;
                                      // copy data into buffer to remove stride
   if (offs == 1) { memcpy(rbuff, real, nval*sizeof(float));
                    memcpy(ibuff, imaginary, nval*sizeof(float));
                  }
    else for (i=0; i < nval; i++)
          { rbuff[i]=real[i*offs];
            ibuff[i]=imaginary[i*offs];
          }
   if (!forward) std::swap(rbuff, ibuff);
   if (power_of_two[table_number])
    {
#ifdef __ALTIVEC__
      DSPSplitComplex s;//, alti_tmp;

//    alti_tmp.realp=rbuff;
//    alti_tmp.imagp=rbuff+n;
      s.realp=rbuff;
      s.imagp=ibuff;
      fft_zip(fft_setup, &s, 1, log2_n[table_number], kFFTDirection_Forward);
//    fft_zipt(fft_setup, &s, 1, &alti_tmp, log2n, kFFTDirection_Forward);
#else
                                   // level 0 of the FFT (Bitreverse Shuffling)
                                       // exchange-pairs from permutation-table
      bsp=tabs_ft[table_number].revieldn;
      for (i=0; i < tabs_ft[table_number].countn; i++)
       std::swap(rbuff[bsp[i*2]], rbuff[bsp[i*2+1]]);
      for (i=0; i < tabs_ft[table_number].countn; i++)
       std::swap(ibuff[bsp[i*2]], ibuff[bsp[i*2+1]]);
                                                        // higher levels of FFT
      four_recursive(rbuff, ibuff, tabs_ft[table_number].wreal,
                     tabs_ft[table_number].wimag, nval, 1);
#endif
    }
    else { unsigned short int radix;
                                   // level 0 of the FFT (Bitreverse Shuffling)
           permute(nval, factors[table_number], actualRadix[table_number],
                   remainRadix[table_number], rbuff, ibuff);
           radix=actualRadix[table_number][0];
           twiddleTransf(radix, remainRadix[table_number][1], trig[radix],
                         rbuff, ibuff);
           for (unsigned short int f=1; f < factors[table_number]; f++)
            { radix=actualRadix[table_number][f];
              twiddleTransf(sofarRadix[table_number][f], radix,
                            remainRadix[table_number][f+1], trig[radix],
                            rbuff, ibuff);
            }
         }
   if (!forward)
    { std::swap(rbuff, ibuff);
      vecMulScalar(rbuff, 1.0f/(float)nval, rbuff, nval*2); // scale inverse FT
    }
                           // mirror sequence and copy back into original place
   if (offs == 1) { real[0]=rbuff[0];
                    for (i=1; i < nval; i++)
                     real[i]=rbuff[nval-i];
                    imaginary[0]=ibuff[0];
                    for (i=1; i < nval; i++)
                     imaginary[i]=ibuff[nval-i];
                  }
    else { real[0]=rbuff[0];
           for (i=1; i < nval; i++)
            real[i*offs]=rbuff[nval-i];
           imaginary[0]=ibuff[0];
           for (i=1; i < nval; i++)
            imaginary[i*offs]=ibuff[nval-i];
         }
 }

#ifndef __ALTIVEC__
/*---------------------------------------------------------------------------*/
/*! \brief Recursive 1D-FFT (level 1 to log2(N)-1).
    \param[in,out] rbuff   shuffled real part of dataset
    \param[in,out] ibuff   shuffled imaginary part of dataset
    \param[in]     wreal   real part of exp-table
    \param[in]     wimag   imaginary part of exp-table
    \param[in]     n       number of values to transform
    \param[in]     incr    increment for exp-table

    Recursive 1D-FFT (level 1 to log2(N)-1). FFTs of length 4 are calculated
    with explicit code (radix 4). Longer FFTs are calculated by recursively
    calculating two FFTs of half the length and then calculating the last
    level with explicit code again. The recursivity is used to improve cache
    locality. This method works only for FFTs that have a length of power of 2.
 */
/*---------------------------------------------------------------------------*/
void FFT::four_recursive(float * const rbuff, float * const ibuff,
                         const double * const wreal,
                         const double * const wimag, const unsigned long int n,
                         const unsigned long int incr) const
 {                            // levels 1 and 2 of the FFT -> radix 4 algorithm
   if (n == 4)
    { float t1, t2, t3, t4, t5, t6, t7, t8;

      t1=rbuff[0]+rbuff[1];
      t5=rbuff[0]-rbuff[1];
      t2=ibuff[0]+ibuff[1];
      t6=ibuff[0]-ibuff[1];
      t3=rbuff[2]+rbuff[3];
      t4=ibuff[2]+ibuff[3];
      t7=rbuff[2]-rbuff[3];
      t8=ibuff[2]-ibuff[3];
      rbuff[0]=t1+t3;
      rbuff[2]=t1-t3;
      ibuff[0]=t2+t4;
      ibuff[2]=t2-t4;
      rbuff[1]=t5+t8;
      rbuff[3]=t5-t8;
      ibuff[1]=t6-t7;
      ibuff[3]=t6+t7;
      return;
    }
   unsigned long int n_2;
                                            // levels 4 to log2(N)-1 of the FFT
   n_2=n/2;
   four_recursive(rbuff, ibuff, wreal, wimag, n_2, incr*2);
   four_recursive(rbuff+n_2, ibuff+n_2, wreal, wimag, n_2, incr*2);
   float *data2r, *data2i;
   unsigned long int i, u;
                                                    // level log2(N) of the FFT
   data2r=rbuff+n_2;
   data2i=ibuff+n_2;
   { float temp;
                                 // butterfly for u=0 is trivial -> exp()=(1,0)
     temp=data2r[0];
     data2r[0]=rbuff[0]-temp;
     rbuff[0]+=temp;
     temp=data2i[0];
     data2i[0]=ibuff[0]-temp;
     ibuff[0]+=temp;
   }
   for (i=incr,
        u=1; u < n/4; u++,                  // calculate all parts of butterfly
        i+=incr)
    { float tempr, tempi;
                                                               // temp=w^n*f(b)
      tempr=(float)(wreal[i]*(double)data2r[u]-wimag[i]*(double)data2i[u]);
      tempi=(float)(wreal[i]*(double)data2i[u]+wimag[i]*(double)data2r[u]);
      data2r[u]=rbuff[u]-tempr;                               // F(b)=f(a)-temp
      data2i[u]=ibuff[u]-tempi;
      rbuff[u]+=tempr;                                        // F(a)=f(a)+temp
      ibuff[u]+=tempi;
    }
          // butterfly for u=n/4, i=n/4*incr is trivial -> exp()=(0,-1) for FFT
          //                                           and exp()=(0,1) for iFFT
   { float tempr, tempi;

     tempr=-wimag[i]*data2i[u];
     tempi=wimag[i]*data2r[u];
     data2r[u]=rbuff[u]-tempr;
     rbuff[u]+=tempr;
     data2i[u]=ibuff[u]-tempi;
     ibuff[u]+=tempi;
   }
   for (i+=incr,
        u++; u < n_2; u++,                  // calculate all parts of butterfly
        i+=incr)
    { float tempr, tempi;
                                                               // temp=w^n*f(b)
      tempr=(float)(wreal[i]*(double)data2r[u]-wimag[i]*(double)data2i[u]);
      tempi=(float)(wreal[i]*(double)data2i[u]+wimag[i]*(double)data2r[u]);
      data2r[u]=rbuff[u]-tempr;                               // F(b)=f(a)-temp
      data2i[u]=ibuff[u]-tempi;
      rbuff[u]+=tempr;                                        // F(a)=f(a)+temp
      ibuff[u]+=tempi;
    }
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Create tables for the FFT algorithms.
    \param[in] n              number of values to transform
    \param[in] table_number   number of exp- and bitshuffling-table

    Create tables for the FFT algorithms.
 */
/*---------------------------------------------------------------------------*/
void FFT::InitTablesFT(const unsigned long int n,
                       const unsigned short int table_number)
 { try
   { tab_alloc[table_number]=true;       // memory for tables will be allocated
     power_of_two[table_number]=((unsigned long int)(1 << Bits(n)) == n);
     if (!power_of_two[table_number])                // is n not a power of 2 ?
      { unsigned short int nf;

        nf=numFactors(n);
        factors[table_number]=nf;
#ifdef __GNUG__
          // the GNU compilers 3.2.3 and 3.3.2 produce bugs if this is not here
        std::cerr << "";
#endif
                                                                // radix tables
        sofarRadix[table_number]=new unsigned short int[nf];
        actualRadix[table_number]=new unsigned short int[nf];
        remainRadix[table_number]=new unsigned short int[nf+1];
        transTableSetup(sofarRadix[table_number], actualRadix[table_number],
                        remainRadix[table_number], n);
        for (unsigned short int f=0; f < factors[table_number]; f++)
         if (actualRadix[table_number][f] > largestRadix)
          largestRadix=actualRadix[table_number][f];
      }
#ifndef __ALTIVEC__
      else {                                      // bitreverse-shuffling-table
             tabs_ft[table_number].countn=
                                (n-(1<<(unsigned short int)((Bits(n)+1)/2)))/2;
             tabs_ft[table_number].revieldn=
                         new unsigned long int[tabs_ft[table_number].countn*2];
             permtable(tabs_ft[table_number].revieldn, n);
             tabs_ft[table_number].countn_2=(n/2-(1<<(unsigned short int)
                                                     ((Bits(n/2)+1)/2)))/2;
             tabs_ft[table_number].revieldn_2=
                       new unsigned long int[tabs_ft[table_number].countn_2*2];
             permtable(tabs_ft[table_number].revieldn_2, n/2);
             exp_table(n, table_number);                           // exp-table
           }
#endif
   }
   catch (...)
    {
#ifndef __ALTIVEC__
      if (tabs_ft[table_number].revieldn != NULL)
       { delete[] tabs_ft[table_number].revieldn;
         tabs_ft[table_number].revieldn=NULL;
       }
      if (tabs_ft[table_number].revieldn_2 != NULL)
       { delete[] tabs_ft[table_number].revieldn_2;
         tabs_ft[table_number].revieldn_2=NULL;
       }
#endif
      if (sofarRadix[table_number] != NULL)
       { delete[] sofarRadix[table_number];
         sofarRadix[table_number]=NULL;
       }
      if (actualRadix[table_number] != NULL)
       { delete[] actualRadix[table_number];
         actualRadix[table_number]=NULL;
       }
      if (remainRadix[table_number] != NULL)
       { delete[] remainRadix[table_number];
         remainRadix[table_number]=NULL;
       }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create tables for the 1D-FFT.

    Create tables for the 1D-FFT.
 */
/*---------------------------------------------------------------------------*/
void FFT::InitTablesFT_1D()
 { for (unsigned short int i=0; i < 3; i++) {
#ifndef __ALTIVEC__
                                              tabs_ft[i].revieldn=NULL;
                                              tabs_ft[i].revieldn_2=NULL;
                                              tabs_ft[i].wreal=
                                              tabs_ft[i].wimag=NULL;
#endif
                                              sofarRadix[i]=NULL;
                                              actualRadix[i]=NULL;
                                              remainRadix[i]=NULL;
                                            }
   largestRadix=1;
   tab_alloc[1]=
   tab_alloc[2]=false;
   InitTablesFT(n[0], 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create tables for the 2D-FFT.

    Create tables for the 2D-FFT.
 */
/*---------------------------------------------------------------------------*/
void FFT::InitTablesFT_2D()
 { InitTablesFT_1D();
   tab_alloc[2]=false;
   if (n[1] == n[0])
    { power_of_two[1]=power_of_two[0];
      if (power_of_two[1])
       {
#ifndef __ALTIVEC__
         tabs_ft[1].countn=tabs_ft[0].countn;
         tabs_ft[1].revieldn=tabs_ft[0].revieldn;
         tabs_ft[1].revieldn_2=tabs_ft[0].revieldn_2;
         tabs_ft[1].countn_2=tabs_ft[0].countn_2;
         tabs_ft[1].wreal=tabs_ft[0].wreal;
         tabs_ft[1].wimag=tabs_ft[0].wimag;
#endif
         sofarRadix[1]=sofarRadix[0];
         actualRadix[1]=actualRadix[0];
         remainRadix[1]=remainRadix[0];
       }
       else { sofarRadix[1]=sofarRadix[0];
              actualRadix[1]=actualRadix[0];
              remainRadix[1]=remainRadix[0];
              factors[1]=factors[0];
            }
      tab_alloc[1]=false;
      return;
    }
   InitTablesFT(n[1], 1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create tables for the 3D-FFT.

    Create tables for the 3D-FFT.
 */
/*---------------------------------------------------------------------------*/
void FFT::InitTablesFT_3D()
 { InitTablesFT_2D();
   if (n[2] == n[0])
    { power_of_two[2]=power_of_two[0];
      if (power_of_two[2])
       {
#ifndef __ALTIVEC__
         tabs_ft[2].countn=tabs_ft[0].countn;
         tabs_ft[2].revieldn=tabs_ft[0].revieldn;
         tabs_ft[2].revieldn_2=tabs_ft[0].revieldn_2;
         tabs_ft[2].countn_2=tabs_ft[0].countn_2;
         tabs_ft[2].wreal=tabs_ft[0].wreal;
         tabs_ft[2].wimag=tabs_ft[0].wimag;
#endif
         sofarRadix[2]=sofarRadix[0];
         actualRadix[2]=actualRadix[0];
         remainRadix[2]=remainRadix[0];
       }
       else { sofarRadix[2]=sofarRadix[0];
              actualRadix[2]=actualRadix[0];
              remainRadix[2]=remainRadix[0];
              factors[2]=factors[0];
            }
      tab_alloc[2]=false;
      return;
    }
   if (n[2] == n[1])
    { power_of_two[2]=power_of_two[1];
      if (power_of_two[2])
       {
#ifndef __ALTIVEC__
         tabs_ft[2].countn=tabs_ft[1].countn;
         tabs_ft[2].revieldn=tabs_ft[1].revieldn;
         tabs_ft[2].revieldn_2=tabs_ft[1].revieldn_2;
         tabs_ft[2].countn_2=tabs_ft[1].countn_2;
         tabs_ft[2].wreal=tabs_ft[1].wreal;
         tabs_ft[2].wimag=tabs_ft[1].wimag;
#endif
         sofarRadix[2]=sofarRadix[1];
         actualRadix[2]=actualRadix[1];
         remainRadix[2]=remainRadix[1];
       }
       else { sofarRadix[2]=sofarRadix[1];
              actualRadix[2]=actualRadix[1];
              remainRadix[2]=remainRadix[1];
              factors[2]=factors[1];
            }
      tab_alloc[2]=false;
      return;
    }
   InitTablesFT(n[2], 2);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize sine/cosine table for given radix.
    \param[in] radix   radix for which table is calculated
    \return trigonometric factors

    Initialize sine/cosine table for given radix.
 */
/*---------------------------------------------------------------------------*/
float *FFT::initTrig(const unsigned short int radix) const
 { float *trigRe=NULL;

   try
   { float w, xre, xim, *trigIm, a, b;

     trigRe=new float[radix*2];
     trigIm=trigRe+radix;
     w=2.0f*M_PI/(float)radix;
     trigRe[0]=1.0f;
     trigIm[0]=0.0f;
     xre=cosf(w);
     xim=-sinf(w);
     trigRe[1]=xre;
     trigIm[1]=xim;
     a=xre;
     b=xim;
     for (unsigned short int i=2; i < radix; i++)
      { trigRe[i]=xre*a-xim*b;
        trigIm[i]=xim*a+xre*b;
        a=trigRe[i];
        b=trigIm[i];
      }
     return(trigRe);
   }
   catch (...)
    { if (trigRe != NULL) delete[] trigRe;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Determine number of factors, if value is factorized into 2,3,4,5,8,
           10 and bigger factors.
    \param[in] n   value to factorize

    Determine number of factors, if value is factorized into 2,3,4,5,8,10 and
    bigger factors.
 */
/*---------------------------------------------------------------------------*/
unsigned short int FFT::numFactors(unsigned long int n) const
 { unsigned short int j;

   n/=2;
   { unsigned short int radices[6], i;
                                                // extract factors 2,3,4,5,8,10
     radices[0]=10;
     radices[1]=8;
     radices[2]=5;
     radices[3]=4;
     radices[4]=3;
     radices[5]=2;
     j=0;
     i=0;
     while ((n > 1) && (i < 6))
      if ((n % radices[i]) == 0)
       { n/=radices[i];
         j++;
       }
       else i++;
   }
   if (n > 1)                      // find remaining factors (not 2,3,4,5,8,10)
    { for (unsigned short int k=2; k < sqrtf(n)+1; k++)
       while ((n % k) == 0)
        { n/=k;
          j++;
        }
      if (n > 1) j++;
    }
   return(j+1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate permutation of sequence.
    \param[in] n        number of values to transform
    \param[in] nFact    number of radices
    \param[in] fact     radices
    \param[in] remain   product of the remaining radices in given stage
    \param[in] xRe      real part of sequence
    \param[in] xIm      imaginary part of sequence

    Calculate permutation of sequence.
 */
/*---------------------------------------------------------------------------*/
void FFT::permute(const unsigned long int n, const unsigned short int nFact,
                  unsigned short int * const fact,
                  unsigned short int * const remain, float * const xRe,
                  float * const xIm) const
 { std::vector <unsigned short int> count;
   std::vector <float> yRe, yIm;
   unsigned short int k=0;
 
   yRe.resize(n);
   yIm.resize(n);
   count.assign(nFact, 0);
   for (unsigned short int i=0; i < n-1; i++)
    { unsigned short int j=0;

      yRe[i]=xRe[k];
      yIm[i]=xIm[k];
      j=0;
      k+=remain[1];
      count[0]++;
      while (count[j] >= fact[j])
       { count[j]=0;
         k-=remain[j]-remain[j+2];
         count[++j]++;
       }
    }
   memcpy(xRe, &yRe[0], (n-1)*sizeof(float));
   memcpy(xIm, &yIm[0], (n-1)*sizeof(float));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create permutation-table for bitreverse shuffling.
    \param[in] revield   table
    \param[in] n         number of values to transform

    Create permutation-table for bitreverse shuffling.
 */
/*---------------------------------------------------------------------------*/
void FFT::permtable(unsigned long int *revield, const unsigned long int n)
 { unsigned short int bits;

   bits=Bits(n);
   for (unsigned long int i=1; i < n; i++)
    { unsigned long int j;

      j=i;                                           // mirror bits from m -> j
      j=((j>> 1) & 0x55555555) | ((j<< 1) & 0xaaaaaaaa);
      j=((j>> 2) & 0x33333333) | ((j<< 2) & 0xcccccccc);
      j=((j>> 4) & 0x0f0f0f0f) | ((j<< 4) & 0xf0f0f0f0);
      j=((j>> 8) & 0x00ff00ff) | ((j<< 8) & 0xff00ff00);
      j=((j>>16) & 0x0000ffff) | ((j<<16) & 0xffff0000);
      j>>=32-bits;
      if (j > i) { *(revield++)=j;
                   *(revield++)=i;
                 }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 1D-FFT (in-place).
    \param[in,out] real      pointer to dataset
                   (space domain:     r0, r1, r2, r3, r4, r5, r6, r7)
                   (frequency domain: r0, r4, r1, i1, r2, i2, r3, i3)
    \param[in] forward   transformation into frequency domain ?

    Real-valued 1D-FFT (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_1D(float * const real, const bool forward) const
 { std::vector <float> buffer;

   buffer.resize(n[0]);
   rfour(real, &buffer[0], 1, forward, 0, false);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 1D-FFT for several sequences (in-place).
    \param[in,out] real          pointer for dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     num           number of sequences to transform
    \param[in]     max_threads   maximum number of threads to use

    Real-valued 1D-FFT for several sequences (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_1D_block(float * const real, const bool forward,
                        const unsigned long int num,
                        const unsigned short int max_threads)
 { std::vector <float> buffer;

   buffer.resize(n[0]*(unsigned long int)max_threads);
   rFFT_block(real, &buffer[0], num, n[0], 1, forward, 0, false, max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 2D-FFT (in-place, type 2).
    \param[in,out] data          pointer to dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     max_threads   maximum number of threads to use

    Real-valued 2D-FFT (in-place, type 2). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_2D(float * const data, const bool forward,
                  const unsigned short int max_threads)
 { std::vector <float> buffer;

   buffer.resize(std::max(n[0], n[1])*2*(unsigned long int)max_threads);
   if (forward)                         // transformation into frequency domain
    {                                               // transform in x-direction
      rFFT_block(data, &buffer[0], n[1], n[0], 1, forward, 0, true,
                 max_threads);
                                               // transformation in y-direction
      rfour(&data[0], &buffer[0], n[0], forward, 1, true);
      rfour(&data[n[0]/2], &buffer[0], n[0], forward, 1, true);
      for (unsigned long int x=1; x < n[0]/2; x++)
       four(&data[x], &data[n[0]-x], &buffer[0], n[0], forward, 1);
    }
    else {                                  // transformation into image domain
                                               // transformation in y-direction
           rfour(&data[0], &buffer[0], n[0], forward, 1, true);
           rfour(&data[n[0]/2], &buffer[0], n[0], forward, 1, true);
           for (unsigned long int x=1; x < n[0]/2; x++)
            four(&data[x], &data[n[0]-x], &buffer[0], n[0], forward, 1);
                                               // transformation in x-direction
           rFFT_block(data, &buffer[0], n[1], n[0], 1, forward, 0, true,
                      max_threads);
         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 3D-FFT (in-place).
    \param[in,out] data          pointer to dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     first         number of first slice to transform
                                 (only for iFFT)
    \param[in]     last          number of last slice+1 to transform
                                 (only for iFFT)
    \param[in]     max_threads   maximum number of threads to use

    Real-valued 3D-FFT (in-place). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_3D(float * const data, const bool forward,
                  const unsigned long int first, const unsigned long int last,
                  const unsigned short int max_threads)
 { std::vector <float> buffer;
   float *dp;
   unsigned long int z;

   buffer.resize(std::max(std::max(n[0], n[1]), n[2])*2*
                 (unsigned long int)max_threads);
#if 1
                                  // this is not an FFT; the correction step to
                                  // convert this into an FFT is in sino2d_3d
                                                    // transform in z-direction
   rFFT_block(data, &buffer[0], n[0]*n[1], 1, n[0]*n[1], forward, 2, true,
              max_threads);
   for (dp=data+first*n[0]*n[1],
        z=first; z < last; z++,                         // transform all slices
        dp+=n[0]*n[1])
    {                                               // transform in x-direction
      rFFT_block(dp, &buffer[0], n[1], n[0], 1, forward, 0, true, max_threads);
                                                    // transform in y-direction
      rFFT_block(dp, &buffer[0], n[0], 1, n[0], forward, 1, true, max_threads);
    }
#endif
#if 0
   if (forward)                         // transformation into frequency domain
    { for (dp=data,
           z=0; z < n[2]; z++,                          // transform all slices
           dp+=n[0]*n[1])
       {                                            // transform in x-direction
         rFFT_block(dp, &buffer[0], n[1], n[0], 1, forward, 0, true,
                    max_threads);
                                                    // transform in y-direction
         rfour(&dp[0], &buffer[0], n[0], forward, 1, true);
         rfour(&dp[n[0]/2], &buffer[0], n[0], forward, 1, true);
         for (unsigned long int x=1; x < n[0]/2; x++)
          four(&dp[x], &dp[n[0]-x], &buffer[0], n[0], forward, 1);
       }
                                                    // transform in z-direction
      rfour(&data[0], &buffer[0], n[0]*n[1], forward, 2, true);
      rfour(&data[n[1]/2*n[0]], &buffer[0], n[0]*n[1], forward, 2, true);
      rfour(&data[n[0]/2], &buffer[0], n[0]*n[1], forward, 2, true);
      rfour(&data[n[1]/2*n[0]+n[0]/2], &buffer[0], n[0]*n[1], forward, 2,
            true);
      for (unsigned long int y=1; y < n[1]/2; y++)
       { four(&data[y*n[0]], &data[(n[1]-y)*n[0]], &buffer[0], n[0]*n[1],
              forward, 2);
         four(&data[y*n[0]+n[0]/2], &data[(n[1]-y)*n[0]+n[0]/2], &buffer[0],
              n[0]*n[1], forward, 2);
       }
      for (unsigned long int y=0; y < n[1]; y++)
       for (unsigned long int x=1; x < n[0]/2; x++)
        four(&data[y*n[0]+x], &data[y*n[0]+n[0]-x], &buffer[0], n[0]*n[1],
             forward, 2);
    }
    else {                                  // transformation into image domain
                                                    // transform in z-direction
           rfour(&data[0], &buffer[0], n[0]*n[1], forward, 2, true);
           rfour(&data[n[1]/2*n[0]], &buffer[0], n[0]*n[1], forward, 2, true);
           rfour(&data[n[0]/2], &buffer[0], n[0]*n[1], forward, 2, true);
           rfour(&data[n[1]/2*n[0]+n[0]/2], &buffer[0], n[0]*n[1], forward, 2,
                 true);
           for (unsigned long int y=1; y < n[1]/2; y++)
            { four(&data[y*n[0]], &data[(n[1]-y)*n[0]], &buffer[0], n[0]*n[1],
                   forward, 2);
              four(&data[y*n[0]+n[0]/2], &data[(n[1]-y)*n[0]+n[0]/2],
                   &buffer[0], n[0]*n[1], forward, 2);
            }
           for (unsigned long int y=0; y < n[1]; y++)
            for (unsigned long int x=1; x < n[0]/2; x++)
             four(&data[y*n[0]+x], &data[y*n[0]+n[0]-x], &buffer[0], n[0]*n[1],
                  forward, 2);
           for (dp=data+first*n[0]*n[1],
                z=first; z < last; z++,                 // transform all slices
                dp+=n[0]*n[1])
            {                                       // transform in y-direction
              rfour(&dp[0], &buffer[0], n[0], forward, 1, true);
              rfour(&dp[n[0]/2], &buffer[0], n[0], forward, 1, true);
              for (unsigned long int x=1; x < n[0]/2; x++)
               four(&dp[x], &dp[n[0]-x], &buffer[0], n[0], forward, 1);
                                                    // transform in x-direction
              rFFT_block(dp, &buffer[0], n[1], n[0], 1, forward, 0, true,
                         max_threads);
            }
         }
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 3D-FFT (in-place).
    \param[in,out] data          pointer to dataset
    \param[in]     forward       transformation into frequency domain ?
    \param[in]     max_threads   maximum number of threads to use

    Real-valued 3D-FFT (in-place). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_3D(float * const data, const bool forward,
                  const unsigned short int max_threads)
 { rFFT_3D(data, forward, 0, n[2], max_threads);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 1D-FFT of data block (in-place).
    \param[in,out] data      pointer to dataset
              (space domain:             r0, r1, r2, r3,  r4,  r5,  r6,  r7)
              (frequency domain: type 1: r0, r4, r1, i1,  r2,  i2,  r3,  i3)
              (                  type 2: r0, r1, r2, r3,  r4. -i3, -i2, -i1)
    \param[in] buffer         buffer for data without stride
    \param[in] num            number of 1D-FFTs
    \param[in] loffs          offset to next 1D-FFT
    \param[in] offs           offset to next element
    \param[in] forward        transformation into frequency domain ?
    \param[in] table_number   number of exp- and bitshuffling-table
    \param[in] form_b         representation in frequency space of type 2 ?
    \param[in] max_threads    maximum number of threads to use

    Real-valued 1D-FFT of data block (in-place). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_block(float * const data, float * const buffer,
                     unsigned long int num, const unsigned long int loffs,
                     const unsigned long int offs, const bool forward,
                     const unsigned short int table_number, const bool form_b,
                     const unsigned short int max_threads)
 { try
   { std::vector <tthread_id> tid;
     std::vector <bool> thread_running;
     unsigned short int threads=0, t;
     void *thread_result;

     try
     { threads=(unsigned short int)std::min((unsigned long int)max_threads,
                                            num);
       if (threads == 1)
        { rFFT_block_thread(data, buffer, num, loffs, offs, forward,
                            table_number, form_b);
          return;
        }
       std::vector <trFFTthread_params> tp;
       unsigned short int i;
       float *dp;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (dp=data,
            t=threads,
            i=0; i < threads; i++,
            t--)
        { tp[i].object=this;
          tp[i].data=dp;
          tp[i].buffer=&buffer[(unsigned long int)i*n[table_number]];
          tp[i].num=num/t;
          tp[i].loffs=loffs;
          tp[i].offs=offs;
          tp[i].forward=forward;
          tp[i].table_number=table_number;
          tp[i].type2=form_b;
#ifdef XEON_HYPERTHREADING_BUG
          tp[i].thread_number=i;
#endif
          thread_running[i]=true;
          ThreadCreate(&tid[i], &executeThread_rFFT_block, &tp[i]);
          num-=tp[i].num;
          dp+=(unsigned long int)tp[i].num*loffs;
        }
       for (i=0; i < threads; i++)
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
     }
     catch (...)
      { thread_result=NULL;
        for (t=0; t < thread_running.size(); t++)
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
                       // if we catch exceptions from the terminating threads,
                       // we store only the first one and ignore the others
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
                    // if the threads have thrown exceptions we throw the first
                    // one of them, not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 1D-FFT of data block (in-place).
    \param[in,out] data      pointer to dataset
                 (space domain:             r0, r1, r2, r3,  r4,  r5,  r6,  r7)
                 (frequency domain: type 1: r0, r4, r1, i1,  r2,  i2,  r3,  i3)
                 (                  type 2: r0, r1, r2, r3,  r4. -i3, -i2, -i1)
    \param[in] buff           buffer for data without stride
    \param[in] num            number of 1D-FFTs
    \param[in] loffs          offset to next 1D-FFT
    \param[in] offs           offset to next element
    \param[in] forward        transformation into frequency domain ?
    \param[in] table_number   number of exp- and bitshuffling-table
    \param[in] form_b         representation in frequency space of type 2 ?

    Real-valued 1D-FFT of data block (in-place). This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_block_thread(float * const data, float * const buff,
                            const unsigned long int num,
                            const unsigned long int loffs,
                            const unsigned long int offs, const bool forward,
                            const unsigned short int table_number,
                            const bool form_b) const
 { for (unsigned long int i=0; i < num; i++)
    rfour(&data[i*loffs], buff, offs, forward, table_number, form_b);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert real valued FT into symmetric complex FT.
    \param[in,out] real        pointer to real part of dataset
                               (input: r0, r4, r1, i1, r2, i2, r3, i3)
                               (output: r0, r1, r2, r3, r4, r3, r2, r1)
    \param[in]     imaginary   pointer to imaginary part of dataset
                               (output: 0, i1, i2, i3, 0, -i3, -i2, -i1)

    Convert real valued FT into symmetric complex FT.
 */
/*---------------------------------------------------------------------------*/
void FFT::rFFT_FFT_1D(float * const real, float * const imaginary) const
 { float tmp;

   imaginary[0]=0.0f;
   tmp=real[1];
   vecCopyIntlCplxSplCplx(&real[2], &real[1], &imaginary[1], n[0]/2-1);
   real[n[0]/2]=tmp;
   imaginary[n[0]/2]=0.0f;
   for (unsigned short int i=n[0]/2+1; i < n[0]; i++)
    { real[i]=real[n[0]-i];
      imaginary[i]=-imaginary[n[0]-i];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Real-valued 1D Fast Fourier Transform (in-place).
    \param[in,out] data      pointer to dataset
                 (space domain:             r0, r1, r2, r3,  r4,  r5,  r6,  r7)
                 (frequency domain: type 1: r0, r4, r1, i1,  r2,  i2,  r3,  i3)
                 (                  type 2: r0, r1, r2, r3,  r4. -i3, -i2, -i1)
    \param[in]     rbuff          pointer to buffer for dataset
    \param[in]     offs           offset to next element
    \param[in]     forward        transformation into frequency domain ?
    \param[in]     table_number   number of exp- and bitshuffling-table
    \param[in]     form_b         representation in frequency space of type 2 ?

    Real-valued 1D Fast Fourier Transform (in-place). This method works for
    FFTs of any length.
 */
/*---------------------------------------------------------------------------*/
void FFT::rfour(float * const data, float *rbuff, const unsigned long int offs,
                const bool forward, const unsigned short int table_number,
                const bool form_b) const
 { unsigned long int n_2, i, nval;
   float *ibuff, h1;

   nval=n[table_number];
   n_2=nval/2;
   ibuff=rbuff+n_2;
                        // copy data into buffer to remove stride
                        // and split sequence into two sequences of half length
   if (form_b && !forward)
    { for (i=0; i < n_2; i++) rbuff[i]=data[i*offs];
      ibuff[0]=data[n_2*offs];
      for (i=1; i < n_2; i++) ibuff[n_2-i]=-data[(n_2+i)*offs];
    }
    else for (i=0; i < n_2; i++)
          { rbuff[i]=data[i*2*offs];
            ibuff[i]=data[(i*2+1)*offs];
          }
   if (power_of_two[table_number])          // length of sequence is power of 2
    {
#ifdef __ALTIVEC__
      DSPSplitComplex s;

      s.realp=rbuff;
      s.imagp=ibuff;
      if (forward)
       fft_zrip(fft_setup, &s, 1, log2_n[table_number], kFFTDirection_Forward);
       else fft_zrip(fft_setup, &s, 1, log2_n[table_number],
                     kFFTDirection_Inverse);
      if (!forward)                                        // scale inverse FFT
       vecMulScalar(rbuff, 1.0f/(float)n_2, rbuff, nval);
                      // copy data back into original place
                      // and combine sequences into one sequence of full length
      if (form_b && forward)
       { for (i=0; i < n_2; i++) data[i*offs]=rbuff[i]/2.0f;
         data[n_2*offs]=ibuff[0]/2.0f;
         for (i=1; i < n_2; i++) data[(n_2+i)*offs]=ibuff[n_2-i]/2.0f;
       }
       else if (forward)
             { data[0]=rbuff[0]/2.0f;
               data[offs]=ibuff[0]/2.0f;
               for (i=1; i < n_2; i++)
                { data[2*i*offs]=rbuff[i]/2.0f;
                  data[(2*i+1)*offs]=-ibuff[i]/2.0f;
                }
             }
             else { data[0]=rbuff[0]/2.0f;
                    data[offs]=ibuff[n_2-1]/2.0f;
                    for (i=1; i < n_2; i++)
                     { data[2*i*offs]=rbuff[n_2-i]/2.0f;
                       data[(2*i+1)*offs]=ibuff[n_2-i-1]/2.0f;
                     }
                  }
#else
      double *wrs, *wis;
      unsigned long int *bsp;

      wrs=tabs_ft[table_number].wreal;
      wis=tabs_ft[table_number].wimag;
      if (forward)
       {        // level 0 of the FFT (Bitreverse Shuffling) for both sequences
                                       // exchange-pairs from permutation table
         bsp=tabs_ft[table_number].revieldn_2;
         for (i=0; i < tabs_ft[table_number].countn_2; i++)
          std::swap(rbuff[bsp[i*2]], rbuff[bsp[i*2+1]]);
         for (i=0; i < tabs_ft[table_number].countn_2; i++)
          std::swap(ibuff[bsp[i*2]], ibuff[bsp[i*2+1]]);
                                  // one sequence is real part, other imaginary
         four_recursive(rbuff, ibuff, wrs, wis, n_2, 2);
                                          // combine FFTs into rFFT (element 0)
         h1=rbuff[0];
         rbuff[0]=h1+ibuff[0];
         ibuff[0]=h1-ibuff[0];
       }
                                  // combine FFTs into rFFT (elements 1 to n/4)
                                  // or split rFFT into FFTs
      for (i=1; i < nval/4; i++)
       { float tmp, h2r, h2i;

         h2r=ibuff[i]+ibuff[n_2-i];
         h2i=rbuff[i]-rbuff[n_2-i];

         tmp=wrs[i]*h2r+wis[i]*h2i;
         h1=rbuff[i]+rbuff[n_2-i];
         rbuff[i]=0.5f*(h1+tmp);
         rbuff[n_2-i]=0.5f*(h1-tmp);

         tmp=wrs[i]*h2i-wis[i]*h2r;
         h1=ibuff[i]-ibuff[n_2-i];
         ibuff[i]=-0.5f*(h1-tmp);
         ibuff[n_2-i]=0.5f*(h1+tmp);
       }

      if (!forward)
       {                                    // split rFFT into FFTs (element 0)
         h1=rbuff[0];
         rbuff[0]=0.5f*(h1+ibuff[0]);
         ibuff[0]=0.5f*(h1-ibuff[0]);
                // level 0 of the FFT (Bitreverse Shuffling) for both sequences
                                       // exchange-pairs from permutation table
         bsp=tabs_ft[table_number].revieldn_2;
         for (i=0; i < tabs_ft[table_number].countn_2; i++)
          std::swap(rbuff[bsp[i*2]], rbuff[bsp[i*2+1]]);
         for (i=0; i < tabs_ft[table_number].countn_2; i++)
          std::swap(ibuff[bsp[i*2]], ibuff[bsp[i*2+1]]);
                                  // one sequence is real part, other imaginary
         four_recursive(ibuff, rbuff, wrs, wis, n_2, 2);
       }
      if (!forward)
       vecMulScalar(rbuff, 1.0f/(float)n_2, rbuff, nval);  // scale inverse FFT
                      // copy data back into original place
                      // and combine sequences into one sequence of full length
      if (form_b && forward)
       { for (i=0; i < n_2; i++) data[i*offs]=rbuff[i];
         data[n_2*offs]=ibuff[0];
         for (i=1; i < n_2; i++) data[(n_2+i)*offs]=-ibuff[n_2-i];
       }
       else for (i=0; i < n_2; i++)
             { data[2*i*offs]=rbuff[i];
               data[(2*i+1)*offs]=ibuff[i];
             }
#endif
      return;
    }
   unsigned long int loop_end;
                                        // length of sequence is not power of 2
   if (forward)
    { unsigned short int radix;
                // level 0 of the FFT (Bitreverse Shuffling) for both sequences
                                       // exchange-pairs from permutation table
      permute(n_2, factors[table_number]-1, actualRadix[table_number]+1,
              remainRadix[table_number]+1, rbuff, ibuff);
                                  // one sequence is real part, other imaginary
      radix=actualRadix[table_number][1];
      twiddleTransf(radix, remainRadix[table_number][2], trig[radix],
                    rbuff, ibuff);
      for (unsigned short int f=1; f < factors[table_number]-1; f++)
       { radix=actualRadix[table_number][f+1];
         twiddleTransf(sofarRadix[table_number][f+1]/2, radix,
                       remainRadix[table_number][f+2], trig[radix], rbuff,
                       ibuff);
       }
                                          // combine FFTs into rFFT (element 0)
      h1=rbuff[0];
      rbuff[0]=h1+ibuff[0];
      ibuff[0]=h1-ibuff[0];
    }
                                  // combine FFTs into rFFT (elements 1 to n/4)
                                  // or split rFFT into FFTs
   if ((nval % 4) == 0) loop_end=nval/4;
    else loop_end=nval/4+1;
   for (i=1; i < loop_end; i++)
    { float tmp, h2r, h2i, cosw, sinw;

      h2r=ibuff[n_2-i]+ibuff[i];
      h2i=rbuff[n_2-i]-rbuff[i];
      tmp=2.0f*M_PI*(float)i/(float)nval;
      cosw=cosf(tmp);
      sinw=sinf(tmp);

      tmp=cosw*h2r+sinw*h2i;
      h1=rbuff[n_2-i]+rbuff[i];
      rbuff[i]=0.5f*(h1+tmp);
      rbuff[n_2-i]=0.5f*(h1-tmp);

      tmp=cosw*h2i-sinw*h2r;
      h1=ibuff[n_2-i]-ibuff[i];
      ibuff[i]=0.5f*(h1-tmp);
      ibuff[n_2-i]=-0.5f*(h1+tmp);
    }

   if (!forward)
    { unsigned short int radix;
                                            // split rFFT into FFTs (element 0)
      h1=rbuff[0];
      rbuff[0]=0.5f*(h1+ibuff[0]);
      ibuff[0]=0.5f*(h1-ibuff[0]);
                // level 0 of the FFT (Bitreverse Shuffling) for both sequences
                                       // exchange-pairs from permutation table
      permute(n_2, factors[table_number]-1, actualRadix[table_number]+1,
              remainRadix[table_number]+1, rbuff, ibuff);
                                  // one sequence is real part, other imaginary
      radix=actualRadix[table_number][1];
      twiddleTransf(radix, remainRadix[table_number][2], trig[radix],
                    rbuff, ibuff);
      for (unsigned short int f=1; f < factors[table_number]-1; f++)
       { radix=actualRadix[table_number][f+1];
         twiddleTransf(sofarRadix[table_number][f+1]/2, radix,
                       remainRadix[table_number][f+2], trig[radix], rbuff,
                       ibuff);
       }
      vecMulScalar(rbuff, 1.0f/(float)n_2, rbuff, nval);   // scale inverse FFT
    }
                      // copy data back into original place
                      // and combine sequences into one sequence of full length
   if (form_b && forward)
    { for (i=0; i < n_2; i++) data[i*offs]=rbuff[i];
      data[n_2*offs]=ibuff[0];
      for (i=1; i < n_2; i++) data[(n_2+i)*offs]=-ibuff[n_2-i];
    }
    else { data[0]=rbuff[0];
           data[offs]=ibuff[0];
           if (forward)
            for (i=1; i < n_2; i++)
             { data[2*i*offs]=rbuff[i];
               data[(2*i+1)*offs]=ibuff[i];
             }
             else for (i=1; i < n_2; i++)
                   { data[2*i*offs]=rbuff[n_2-i];
                     data[(2*i+1)*offs]=ibuff[n_2-i];
                   }
          }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate parameters that control stages.
    \param[in,out] sofar    product of the radices so far
    \param[in,out] actual   the radix handled in this stage
    \param[in,out] remain   product of the remaining radices
    \param[in]     n        number of values to transform

    Calculate parameters that control stages.
 */
/*---------------------------------------------------------------------------*/
void FFT::transTableSetup(unsigned short int * const sofar,
                          unsigned short int * const actual,
                          unsigned short int * const remain,
                          const unsigned long int n)
 { unsigned short int nFact;

   factorize(n, &nFact, actual);                                 // factorize n
   remain[0]=n;
   sofar[0]=1;
   remain[1]=n/actual[0];
   for (unsigned short int i=1; i < nFact; i++)
    { sofar[i]=sofar[i-1]*actual[i-1];
      remain[i+1]=remain[i]/actual[i];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for given radix (in-place).
    \param[in]     radix         the radix handled in this stage
    \param[in]     remainRadix   product of the remaining radices
    \param[in]     trigRe        trigonometric factors
    \param[in,out] rbuff         real part of sequence
    \param[in,out] ibuff         imaginary part of sequence

    Calculate FFT for given radix (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::twiddleTransf(const unsigned short int radix,
                        const unsigned short int remainRadix,
                        float * const trigRe, float *rbuff, float *ibuff) const
 { unsigned short int groupNo;

   switch (radix)
    { case 2:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_2(rbuff, ibuff);
       break;
      case 3:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_3(rbuff, ibuff);
       break;
      case 4:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_4(rbuff, ibuff);
       break;
      case 5:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_5(rbuff, ibuff);
       break;
      case 8:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_8(rbuff, ibuff);
       break;
      case 10:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_10(rbuff, ibuff);
       break;
      default:
       for (groupNo=0; groupNo < remainRadix; groupNo++,
            rbuff+=radix,
            ibuff+=radix)
        fft_odd(radix, trigRe, rbuff, ibuff);
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate FFT for given radix (in-place).
    \param[in]     sofarRadix    product of the radices so far
    \param[in]     radix         the radix handled in this stage
    \param[in]     remainRadix   product of the remaining radices
    \param[in]     trigRe        trigonometric factors
    \param[in,out] rbuff         real part of sequence
    \param[in,out] ibuff         imaginary part of sequence

    Calculate FFT for given radix (in-place).
 */
/*---------------------------------------------------------------------------*/
void FFT::twiddleTransf(const unsigned short int sofarRadix,
                        const unsigned short int radix,
                        const unsigned short int remainRadix,
                        float * const trigRe, float * const rbuff,
                        float * const ibuff) const
 { std::vector <float> twiddleRe;
   float *twiddleIm=NULL, *zRe=NULL, *zIm=NULL, cosw, sinw, tw_re=1.0f,
         tw_im=0.0f;

   twiddleRe.resize(radix*4);
   twiddleIm=&twiddleRe[radix];
   zRe=twiddleIm+radix;
   zIm=zRe+radix;
   { float omega;

     omega=2.0f*M_PI/(float)(sofarRadix*radix);
     cosw=cosf(omega);
     sinw=-sinf(omega);
   }
   for (unsigned short int dataNo=0; dataNo < sofarRadix; dataNo++)
    { unsigned short int groupOffset;
      float gem, a, b;

      groupOffset=dataNo;
      twiddleRe[0]=1.0f;
      twiddleIm[0]=0.0f;
      twiddleRe[1]=tw_re;
      twiddleIm[1]=tw_im;
      a=tw_re;
      b=tw_im;
      for (unsigned short int twNo=2; twNo < radix; twNo++)
       { twiddleRe[twNo]=tw_re*a-tw_im*b;
         twiddleIm[twNo]=tw_im*a+tw_re*b;
         a=twiddleRe[twNo];
         b=twiddleIm[twNo];
       }
      gem=cosw*tw_re-sinw*tw_im;
      tw_im=sinw*tw_re+cosw*tw_im;
      tw_re=gem;
      for (unsigned short int groupNo=0; groupNo < remainRadix; groupNo++)
       { unsigned short int blockNo, adr;

         if (dataNo > 0)
          { zRe[0]=rbuff[groupOffset];
            zIm[0]=ibuff[groupOffset];
            for (adr=groupOffset+sofarRadix,
                 blockNo=1; blockNo < radix; blockNo++,
                 adr+=sofarRadix)
             { zRe[blockNo]=twiddleRe[blockNo]*rbuff[adr]-
                            twiddleIm[blockNo]*ibuff[adr];
               zIm[blockNo]=twiddleRe[blockNo]*ibuff[adr]+
                            twiddleIm[blockNo]*rbuff[adr];
             }
          }
          else for (adr=groupOffset,
                    blockNo=0; blockNo < radix; blockNo++,
                    adr+=sofarRadix)
                { zRe[blockNo]=rbuff[adr];
                  zIm[blockNo]=ibuff[adr];
                }
         switch(radix)
          { case  2: fft_2(zRe, zIm);
                     break;
            case  3: fft_3(zRe, zIm);
                     break;
            case  4: fft_4(zRe, zIm);
                     break;
            case  5: fft_5(zRe, zIm);
                     break;
            case  8: fft_8(zRe, zIm);
                     break;
            case 10: fft_10(zRe, zIm);
                     break;
            default: fft_odd(radix, trigRe, zRe, zIm);
                     break;
          }
         for (adr=groupOffset,
              blockNo=0; blockNo < radix; blockNo++,
              adr+=sofarRadix)
          { rbuff[adr]=zRe[blockNo];
            ibuff[adr]=zIm[blockNo];
          }
         groupOffset+=sofarRadix*radix;
       }
    }
 }
