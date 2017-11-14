/*! \file timedate.cpp
    \brief This file provides functions to handle date and time values.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2005/01/28 initial version
    \date 2005/02/02 added convertToLocalTime() method
 
    This file provides functions to handle date and time values.
 */

#include <ctime>
#include <string>
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <unistd.h>
#include <sys/time.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif
#include "timedate.h"

/*---------------------------------------------------------------------------*/
/*! \brief Convert time and date into local time and date.
    \param[in,out] td   date/local date
    \param[in,out] tt   time/local time

    Convert time and date into local time and date.
 */
/*---------------------------------------------------------------------------*/
#ifdef WIN32
void TIMEDATE::convertToLocalTime(TIMEDATE::tdate * const td,
                                  TIMEDATE::ttime * const tt)
  { SYSTEMTIME t, tl;

    t.wDay=td->day;
    t.wMonth=td->month;
    t.wYear=td->year;
    t.wHour=tt->hour;
    t.wMinute=tt->minute;
    t.wSecond=tt->second;
    t.wMilliseconds=0;
    SystemTimeToTzSpecificLocalTime(NULL, &t, &tl);
    td->day=tl.wDay;
    td->month=tl.wMonth;
    td->year=tl.wYear;
    tt->hour=tl.wHour;
    tt->minute=tl.wMinute;
    tt->second=tl.wSecond;
    _tzset();
    tt->gmt_offset_h=-_timezone/3600;
    tt->gmt_offset_m=(-_timezone-tt->gmt_offset_h*3600)/60;
  }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Get current date.
    \return current date

    Get current date.
 */
/*---------------------------------------------------------------------------*/
TIMEDATE::tdate TIMEDATE::currentDate()
 { tdate dt;
#ifdef WIN32
   SYSTEMTIME t;
                                                            // get current time
   GetLocalTime(&t);
   dt.day=t.wDay;
   dt.month=t.wMonth;
   dt.year=t.wYear;
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   timeval tv;
   tm *t;
                                                            // get current time
   gettimeofday(&tv, NULL);
   t=localtime((time_t *)&tv.tv_sec);
   dt.day=t->tm_mday;
   dt.month=t->tm_mon+1;
   dt.year=t->tm_year+1900;
#endif
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
#ifdef WIN32
   SYSTEMTIME t;
   std::string vz, gmt_str;
                                                            // get current time
   GetLocalTime(&t);
   ti.hour=t.wHour;
   ti.minute=t.wMinute;
   ti.second=t.wSecond;
   if (msec != NULL) *msec=t.wMilliseconds;
   _tzset();
   ti.gmt_offset_h=-_timezone/3600;
   ti.gmt_offset_m=(-_timezone-ti.gmt_offset_h*3600)/60;
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
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
#endif
   return(ti);
 }
