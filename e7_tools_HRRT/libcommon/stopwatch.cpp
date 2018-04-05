/*! \class StopWatch stopwatch.h "stopwatch.h"
    \brief This class implements stop watches for time measurements.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/09/09 initial version

    This class implements stop watches for time measurements. Time is measured
    in milliseconds for Windows and microseconds for Unix. The watches are
    stored on a stack; the method start() puts a new watch on the stack and
    the method stop() gets the time from the last watch on the stack.
 */

#include <ctime>
#include <sys/time.h>
#include "stopwatch.h"
#include "exception.h"

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
StopWatch::StopWatch()
 { watches.clear();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.
    \exception REC_STOPWATCH_RUN stop watch is still running

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
StopWatch::~StopWatch()
 { if (watches.size() > 0)
  // ahc 11/27/17 Destructors should not throw.
  // Since C++ 2011 this generates warning:
  // warning: throw will always call terminate() [-Wterminate]
    // throw Exception(REC_STOPWATCH_RUN, "Stop watch is still running.");
  std::cerr << "ERROR: " << watches.size() << " Stopwatches still exist!" << std::endl;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Start new stop watch.

    Start new stop watch and put it on the stack.
 */
/*---------------------------------------------------------------------------*/
void StopWatch::start()
 { 
   // std::cerr << "StopWatch::start(" << time() << ")\n";
   watches.push_back(time());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Stop last stop watch.
    \return measured time in seconds
    \exceptions REC_STOPWATCH_INIT stop watch was not started

    Stop last stop watch and return measured time in seconds. The watch is
    removed from the stack.
 */
/*---------------------------------------------------------------------------*/
float StopWatch::stop()
 { 
   // std::cerr << "StopWatch::stop(" << time() << "): " << watches.size() << " items\n";
   if (watches.size() > 0) {
     float t;

     t=time()-watches.back();
     watches.pop_back();
     if (t < -1.0f) 
       return(t+86400.0f);
     else 
       if (t < 0.0f) 
	 return(0.0f);
     return(t);
   }
   throw Exception(REC_STOPWATCH_INIT, "Stop watch was not started.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request current time.
    \return Current time in seconds.

    Request current time in seconds. Accuracy is in milliseconds for Windows
    and microseconds for Unix.
 */
/*---------------------------------------------------------------------------*/
float StopWatch::time() const {
   timeval tv;
   tm *t;
                                                            // get current time
   gettimeofday(&tv, NULL);
   t=localtime((time_t *)&tv.tv_sec);
   return(float(t->tm_hour)*3600.0f+float(t->tm_min)*60.0f+
          float(t->tm_sec)+float(tv.tv_usec)/1000000.0f);

 }
