/*! \file rng.h
    \brief This class is used to calculate random numbers.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/04/30 initial version
 */

#ifndef _RNG_H
#define _RNG_H

/*- class definition --------------------------------------------------------*/

class RNG
 { private:
    signed long int idum;             /*!< seed for random number generators */
    float gammln(const float) const;         // logarithm of the gamma function
   public:
    RNG(const signed long int);                                // create object
    float poissonDev(const float);     // get poisson distributed random number
    float uniformDev();              // get uniformly distributed random number
 };

#endif
