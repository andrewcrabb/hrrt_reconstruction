/*! \file calibration_table.h
    \copyright HRRT User Community
    \brief Provide interface to access Calibrations table.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2008/10/06 initial version
 */
#ifndef CALIBRATION_H                                                                                                                                                                                                
#define CALIBRATION_H

#include <sys/types.h>
#include <time.h>

        /*! Load filename in the table */
int calibration_load(char* directory);

        /*! Find the calibration corresponding to a scan time */
const char *calibration_find(int year, int mon, int day, 
                             int hour, int min, int sec);

        /*! free table memory allocated by calibration_load */
int calibration_clear();

#endif
