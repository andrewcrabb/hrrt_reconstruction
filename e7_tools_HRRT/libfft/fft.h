/*! \file fft.h
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
 */

#ifndef _FFT_H
#define _FFT_H

#ifdef __ALTIVEC__
#include "vecLib/vecLib.h"
#endif
#include "const.h"

/*- class definitions -------------------------------------------------------*/

class FFT
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_FFT_block(void *);
   friend void *executeThread_rFFT_block(void *);
  protected:
                                         /*! parameters for FFT_block thread */
   typedef struct { FFT *object;                  /*!< pointer to FFT object */
                    float *real,                 /*!< pointer to real values */
                          *imaginary,       /*!< pointer to imaginary values */
                          *buffer;              /*!< pointer to local buffer */
                                        /*! number of sequences to transform */
                    unsigned long int num,
                                      loffs,    /*!< offset to next sequence */
                                      /*! offset to next element in sequence */
                                      offs;
                                                  /*! number of lookup-table */
                    unsigned short int table_number;
                    bool forward;              /*!< forward transformation ? */
#ifdef XEON_HYPERTHREADING_BUG
                    unsigned short int thread_number;  /*!< number of thread */
                                  /*! padding bytes to avoid cache conflicts */
                    char padding[2*CACHE_LINE_SIZE-
                                 2*sizeof(unsigned short int)-sizeof(bool)-
                                 3*sizeof(unsigned long int)-3*sizeof(float *)-
                                 sizeof(FFT *)];
#endif
                  } tFFTthread_params;
                                        /*! parameters for rFFT_block thread */
   typedef struct { FFT *object;                  /*!< pointer to FFT object */
                    float *data,                 /*!< pointer to real values */
                          *buffer;              /*!< pointer to local buffer */
                                        /*! number of sequences to transform */
                    unsigned long int num,
                                      loffs,    /*!< offset to next sequence */
                                      /*! offset to next element in sequence */
                                      offs;
                                                  /*! number of lookup-table */
                    unsigned short int table_number;
                    bool forward,              /*!< forward transformation ? */
                              /*! frequency space representation of type 2 ? */
                         type2;
#ifdef XEON_HYPERTHREADING_BUG
                    unsigned short int thread_number;  /*!< number of thread */
                                  /*! padding bytes to avoid cache conflicts */
                    char padding[2*CACHE_LINE_SIZE-
                                 2*sizeof(unsigned short int)-2*sizeof(bool)-
                                 3*sizeof(unsigned long int)-2*sizeof(float *)-
                                 sizeof(FFT *)];
#endif
                  } trFFTthread_params;
   private:
    static const unsigned short int MAX_RADIX=511;/*!< maximum allowed radix */
#ifndef __ALTIVEC__
                                                   /*! lookup tables for FFT */
    typedef struct {          /*! table for bitreverse shuffling of length n */
                     unsigned long int *revieldn,
                            /*! table for bitreverse shuffling of length n/2 */
                                       *revieldn_2,
         /*! number of pairs to exchange in bitreverse shuffling of length n */
                                       countn,
       /*! number of pairs to exchange in bitreverse shuffling of length n/2 */
                                       countn_2;
                     double *wreal,      /*!< cosinus-table for FFT and iFFT */
                            *wimag;                 /*!< sinus-table for FFT */
                   } ttabs_fourier;
    ttabs_fourier tabs_ft[3];      /*!< lookup tables for up to 3 dimensions */
#endif
    bool tab_alloc[3],                    /*!< memory for tables allocated ? */
         power_of_two[3];         /*!< is transformation over a power of 2 ? */
    unsigned long int n[3];                       /*!< dimensions of dataset */
    unsigned short int dim,                      /*!< dimension of transform */
#ifdef __ALTIVEC__
                       log2_n[3],                        /*!< logarithm of n */
#endif
                                         // radix tables for up to 3 dimensions
                       *sofarRadix[3],    /*!< product of the radices so far */
                       *actualRadix[3], /*!< the radix handled in this stage */
                       *remainRadix[3],/*!< product of the remaining radices */
                       factors[3],                    /*!< number of radices */
                       largestRadix;                      /*!< largest radix */
                             /*! trigonometric factors for different radices */
    float *trig[MAX_RADIX+1];
#ifdef __ALTIVEC__
    FFTSetup fft_setup;                       /*!< lookup tables for Altivec */
#endif
                                              // calculate bitreverse shuffling
    unsigned short int Bits(unsigned long int) const;
#ifndef __ALTIVEC__
                                                    // calculate cosinus-tables
    void exp_table(const unsigned long int, const unsigned short int);
#endif
                                                             // factorize value
    void factorize(unsigned long int, unsigned short int * const,
                   unsigned short int * const);
                                                  // calculate fft for length 2
    void fft_2(float * const, float * const) const;
                                                  // calculate fft for length 3
    void fft_3(float * const, float * const) const;
                                                  // calculate fft for length 4
    void fft_4(float * const, float * const) const;
                                                  // calculate fft for length 5
    void fft_5(float * const, float * const) const;
                                                  // calculate fft for length 8
    void fft_8(float * const, float * const) const;
                                                 // calculate fft for length 10
    void fft_10(float * const, float * const) const;
                            // calculate fft for length other than 2,3,4,5,8,10
    void fft_odd(const unsigned short int, float * const, float *,
                 float *) const;
                                     // 1D-FFT of a data block (multi-threaded)
    void FFT_block(float * const, float * const, float * const,
                   unsigned long int, const unsigned long int,
                   const unsigned long int, const bool,
                   const unsigned short int, const unsigned short int);
                                                      // 1D-FFT of a data block
    void FFT_block_thread(float * const, float * const, float * const,
                          const unsigned long int, const unsigned long int,
                          const unsigned long int, const bool,
                          const unsigned short int) const;
                                                                      // 1D-FFT
    void four(float * const, float * const, float *, const unsigned long int,
              const bool, const unsigned short int) const;
#ifndef __ALTIVEC__
                                                            // recursive 1d-FFT
    void four_recursive(float * const, float * const, const double * const,
                        const double * const, const unsigned long int,
                        const unsigned long int) const;
#endif
                                                           // initialize tables
    void InitTablesFT(const unsigned long int, const unsigned short int);
    void InitTablesFT_1D();
    void InitTablesFT_2D();
    void InitTablesFT_3D();
                                      // init sine/cosine table for given radix
    float *initTrig(const unsigned short int) const;
                                // determine number of factors in factorization
    unsigned short int numFactors(unsigned long int) const;
                        // permutation-table for bitreverse shuffling (n = 2^x)
    void permtable(unsigned long int *, const unsigned long int);
                             // permutation for bitreverse shuffling (n != 2^x)
    void permute(const unsigned long int, const unsigned short int,
                 unsigned short int * const, unsigned short int * const,
                 float * const, float * const) const;
                         // real-valued 1D-FFT of a data block (multi-threaded)
    void rFFT_block(float * const, float * const, unsigned long int,
                    const unsigned long int, const unsigned long int,
                    const bool, const unsigned short int, const bool,
                    const unsigned short int);
                                          // real-valued 1D-FFT of a data block
    void rFFT_block_thread(float * const, float * const,
                           const unsigned long int, const unsigned long int,
                           const unsigned long int, const bool,
                           const unsigned short int, const bool) const;
                                                          // real-valued 1D-FFT
    void rfour(float * const, float *, const unsigned long int, const bool,
               const unsigned short int, const bool) const;
                             // calculate tables with factorization information
    void transTableSetup(unsigned short int * const,
                         unsigned short int * const,
                         unsigned short int * const, const unsigned long int);
                                               // calculate FFT for given radix
    void twiddleTransf(const unsigned short int, const unsigned short int,
                       float * const, float *, float *) const;
    void twiddleTransf(const unsigned short int, const unsigned short int,
                       const unsigned short int, float * const, float * const,
                       float * const) const;
   public:
    FFT(const unsigned long int);                               // constructors
    FFT(const unsigned long int, const unsigned long int);
    FFT(const unsigned long int, const unsigned long int,
        const unsigned long int);
    ~FFT();                                                       // destructor
    void FFT_1D(float * const, float * const, const bool) const;      // 1D-FFT
                                                 // 1D-FFT of several sequences
    void FFT_1D_block(float * const, float * const, const bool,
                      const unsigned long int, const unsigned long int,
                      const unsigned long int, const unsigned short int);
                                                    // 1D-FFT on complex values
    void FFT_1D_cplx(float * const, const bool, const unsigned long int) const;
                                                                      // 2D-FFT
    void FFT_2D(float * const, float * const, const bool,
                const unsigned short int);
                                                    // 2D-FFT on complex values
    void FFT_2D_cplx(float * const, const bool, const unsigned short int);
                                                                      // 3D-FFT
    void FFT_3D(float * const, float * const, const bool,
                const unsigned short int);
                            // convert symmetric complex FT into real valued FT
    void FFT_rFFT_1D(float * const, float * const) const;
    void rFFT_1D(float * const, const bool) const;        // real-valued 1D-FFT
                                     // real-valued 1D-FFT of several sequences
    void rFFT_1D_block(float * const, const bool, const unsigned long int,
                       const unsigned short int);
                                                          // real-valued 2D-FFT
    void rFFT_2D(float * const, const bool, const unsigned short int);
                                                          // real-valued 3D-FFT
    void rFFT_3D(float * const, const bool, const unsigned long int,
                 const unsigned long int, const unsigned short int);
    void rFFT_3D(float * const, const bool, const unsigned short int);
                                      // convert real valued FT into complex FT
    void rFFT_FFT_1D(float * const, float * const) const;
 };

#endif
