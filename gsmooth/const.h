//** file: const.h
//** date: 2002/11/06

//** author: Frank Kehren
//** © CPS Innovations

//** This file contains some constants used at different places in the program.

#ifndef _CONST_H
#define _CONST_H

/*- constants ---------------------------------------------------------------*/

#ifdef M_PI
#undef M_PI
#endif
#ifdef M_PI_2
#undef M_PI_2
#endif

#ifdef XEON_HYPERTHREADING_BUG
                                  // offset between stacks of different threads
    // (multiple of CACHE_LINE_SIZE and larger than the stack used by a thread)
const unsigned long int STACK_OFFSET=1024;
            // size of a cache line on Pentium 4 and Xeon CPUs is 64 Byte; each
            // read fetches a cache sector consisting of two adjacent lines
const unsigned short int CACHE_LINE_SIZE=128;
#endif

const double M_PI=3.1415926535897932384626433832795029,             // 4*atn(1)
             M_PI_2=1.5707963267948966192313216916397514;           // 2*atn(1)

const signed short int CLOCKWISE=1,           // clockwise rotation of detector
                                       // counterclockwise rotation of detector
                       CTR_CLOCKWISE=-1;
const float EPS=1.0e-6f;                                         // small value

#endif
