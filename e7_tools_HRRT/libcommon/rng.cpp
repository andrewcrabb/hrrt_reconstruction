/*! \class RNG rng.h "rng.h"
    \brief This class is used to calculate random numbers.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/30 initial version

    This code is from the book:
    Numerical Recipes, The Art or Scientific Computing, 2nd ed., W. H. Press,
    S. A. Teukolsky, W. T. Vetterling, B. P. Flannery, 2002
 */

#include <cmath>
#include "rng.h"
#include "const.h"
#include "fastmath.h"

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

/*---------------------------------------------------------------------------*/
/*! \brief Initialize random number generator.
    \param[in] seed   seed for random number generator

    Initialize random number generator. The seed needs to be a negative
    integer.
 */
/*---------------------------------------------------------------------------*/
RNG::RNG(const signed long int seed)
 { idum=seed;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate logarithm of the gamma function.
    \param[in] z   argument for the gamma function
    \return value of the gamma function

    Calculate the logarithm of the gamma function for \f$z>0\f$:
    \f[
       \ln(\Gamma(z))=\ln\left(\int_0^{\infty} t^{z-1}e^{-t}dt\right)
    \f]
    The algorithm uses the Lanczos approximation.
 */
/*---------------------------------------------------------------------------*/
float RNG::gammln(const float z) const
 { double x, y, tmp, ser;
   static double cof[6]={ 76.18009172947146, -86.50532032941677,
                          24.01409824083091, -1.231739572450155,
                          0.1208650973866179e-2, -0.5395239384953e-5 };

   y=x=z;
   tmp=x+5.5;
   tmp-=(x+0.5)*log(tmp);
   ser=1.000000000190015;
   for (unsigned short int j=0; j <= 5; j++)
    ser+=cof[j]/++y;
   return(-tmp+log(2.5066282746310005*ser/x));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get a Poisson distributed random number.
    \return random number

    Get a Poisson distributed random number. The function returns as a
    floating-point number an integer value that is a random deviate drawn from
    a Poisson distribution of mean \em xm, using \em uniformDev() as a source
    of uniform random deviates.
    This code is equivalent to the \em poidev() function from \em Numerical
    \em Recipes.
 */
/*---------------------------------------------------------------------------*/
float RNG::poissonDev(const float xm)
 { static float sq, alxm, g, oldm=-1.0f;
   float em, t, y;

   if (xm < 12.0f)                                       // use direct method ?
    { if (xm != oldm) { oldm=xm;
                        g=expf(-xm);   // if xm is new, compute the exponential
                      }
         // instead of adding exponential deviates it is equivalent to multiply
         // uniform deviates. We never actually have to take the log, merely
         // compare to the pre-computed exponential g.
      em=-1.0f;
      t=1.0f;
      do { ++em;
           t*=uniformDev();
         } while (t > g);
    }
    else {                                              // use rejection method
                                      // if xm has changed since the last call,
           if (xm != oldm)                            // precompute some values
            { oldm=xm;
              sq=sqrtf(2.0f*xm);
              alxm=logf(xm);
              g=xm*alxm-gammln(xm+1.0f);
            }
           do { do {    // y is a deviate from a Lorentzian comparison function
                     y=tan(M_PI*uniformDev());
                     em=sq*y+xm;                 // em is y, shifted and scaled
                   } while (em < 0.0f);               // reject if in regime of
                                                      // zero probability
                em=floor(em);     // the trick for integer-valued distributions
                   // the ratio of the desired distribution to the comparision
                   // function; we accept or reject by comparing it to another
                   // uniform deviate. The factor 0.9 is chosen so that t never
                   // exceeds 1.
                t=0.9f*(1.0f+y*y)*expf(em*alxm-gammln(em+1.0f)-g);
              } while (uniformDev() > t);
         }
   return(em);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get a uniformly distributed random number.
    \return random number \f$\in(0,1)\f$

    Get a uniformly distributed random number. This is a "minimal" random
    number generator of Park and Miller with Bays-Durham shuffle and added
    safeguards. The numbers generated are in the range \f$0<r<1\f$.
    This code is equivalent to the \em ran1() function from \em Numerical
    \em Recipes.
 */
/*---------------------------------------------------------------------------*/
float RNG::uniformDev()
 { signed long int j, k;
   static signed long int iy=0, iv[NTAB];
   float temp;

   if ((idum <= 0) || !iy)                                      // initialize ?
     { if (-idum < 1) idum=1;                                 // prevent idum=0
       else idum=-idum;
      for (j=NTAB+7; j >= 0; j--)  // load the shuffle table (after 8 warm-ups)
       { k=idum/IQ;
         idum=IA*(idum-k*IQ)-IR*k;
         if (idum < 0) idum+=IM;
         if (j < NTAB) iv[j]=idum;
       }
      iy=iv[0];
    }
   k=idum/IQ;                               // start here when not initializing
           // compute idum=(IA*idum) % IM without overflows by Schrage's method
   idum=IA*(idum-k*IQ)-IR*k;
   if (idum < 0) idum+=IM;

   j=iy/NDIV;                                 // will be in the range 0..NTAB-1
   iy=iv[j];                              // output previously stored value and
   iv[j]=idum;                                      // refill the shuffle table
                                  // because users don't expect endpoint values
   if ((temp=AM*iy) > RNMX) return(RNMX);
   return(temp);
 }
