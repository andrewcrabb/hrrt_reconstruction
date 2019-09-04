/*! \file timedate.cpp
    \brief This file provides functions to handle date and time values.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2005/01/28 initial version
    \date 2005/02/02 added convertToLocalTime() method
 
    This file provides functions to handle date and time values.
 */

#include <ctime>
#include <string>
#include <unistd.h>
#include <sys/time.h>
#include "timedate.h"

/*---------------------------------------------------------------------------*/
/*! \brief Convert time and date into local time and date.
    \param[in,out] td   date/local date
    \param[in,out] tt   time/local time

    Convert time and date into local time and date.
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*! \brief Get current date.
    \return current date

    Get current date.
 */
/*---------------------------------------------------------------------------*/
TIMEDATE::tdate TIMEDATE::currentDate()
 { tdate dt;
   timeval tv;
   tm *t;
                                                            // get current time
   gettimeofday(&tv, NULL);
   t=localtime((time_t *)&tv.tv_sec);
   dt.day=t->tm_mday;
   dt.month=t->tm_mon+1;
   dt.year=t->tm_year+1900;
   return(dt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get current time.
    \param[out] msec   milliseconds
    \return current time

    Get current time.
 */
/*---------------------------------------------------------------------------*/
TIMEDATE::ttime TIMEDATE::currentTime(unsigned short int * const msec)
 { ttime ti;

   tm *t;
   timeval tv;
                                                            // get current time
   gettimeofday(&tv, NULL);
   t=localtime((time_t *)&tv.tv_sec);
   ti.hour=t->tm_hour;
   ti.minute=t->tm_min;
   ti.second=t->tm_sec;
   if (msec != NULL) *msec=tv.tv_usec;
   ti.gmt_offset_h=t->tm_gmtoff/3600;
   ti.gmt_offset_m=(t->tm_gmtoff-ti.gmt_offset_h*3600)/60;
   return(ti);
 }
