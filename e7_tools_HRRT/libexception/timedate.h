/*! \file timedate.h
    \brief This file provides functions to handle date and time values.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2005/01/28 initial version
    \date 2005/02/02 added convertToLocalTime() method
 */

#ifndef _TIMEDATE_H
#define _TIMEDATE_H

namespace TIMEDATE
{
                                                      /*! structure for date */
  typedef struct { unsigned short int year,                        /*!< year */
                                      month,                      /*!< month */
                                      day;                          /*!< day */
                 } tdate;
                                                       /*!structure for time */
  typedef struct { unsigned short int hour,                        /*!< hour */
                                      minute,                    /*!< minute */
                                      second;                    /*!< second */
                   signed short int gmt_offset_h,      /*!< GMT offset hours */
                                    gmt_offset_m;    /*!< GMT offset minutes */
                 } ttime;
#ifdef WIN32
                              // convert time and date into local time and date
  void convertToLocalTime(TIMEDATE::tdate * const, TIMEDATE::ttime * const);
#endif
  tdate currentDate();                                  // request current date
  ttime currentTime(unsigned short int * const);        // request current time
}

#endif
