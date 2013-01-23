/*! \file stopwatch.h
    \brief This class implements stop watches for time measurements.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/09/09 initial version
 */

#ifndef _STOPWATCH_H
#define _STOPWATCH_H

#include <vector>

/*- class definitions -------------------------------------------------------*/

class StopWatch
 { private:
    std::vector <float> watches;                                /*!< watches */
      // calculate current time (seconds cince 00:00:00 UTC, January 1st, 1970)
    float time() const;
   public:
    StopWatch();                                           // initialize object
    ~StopWatch();                                             // destroy object
    void start();                                        // start new stopwatch
    float stop();                           // stop last stopwatch and get time
 };

#endif
