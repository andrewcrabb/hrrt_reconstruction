/*! \file listmode_table.h
    \copyright HRRT User Community
    \brief Provide interface to find EM lismode corresponding to tracking.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2010/02/16 initial version
 */
#ifndef LISTMODE_TABLE_H                                                                                                                                                                                                
#define CALIBRATION_H

#include <sys/types.h>
#include <time.h>


typedef struct _CTableEntry {
  time_t t;
  char frame_def[_MAX_PATH];
  char fname[_MAX_PATH];
  float ft;
  int em_flag;
} CTableEntry;

int listmode_table_load(char *dir);
        /*! Find the listmode corresponding to a tracking file */
const CTableEntry *listmode_find(time_t tt, int duration);

        /*! free table memory allocated by listmode_load */
int listmode_table_clear();

/* time_encode(int year, int mon, int day, int hour=0, int min=0, int sec=0)
   Just time encoding to enable sorting, not true time encoding
*/
inline time_t time_encode(int year, int mon, int day, 
                          int hour, int min, int sec)
{
  time_t i64 = year; // force 64 bit arithmetic
  time_t t = ((((((((((i64-1900)*366)+(mon-1))*12)+day)*31)+hour)*60)+min)*60)+sec;
  return t;
}
#endif
