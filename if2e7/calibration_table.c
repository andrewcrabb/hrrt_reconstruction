/*! \file interfile_reader.c
    \copyright HRRT User Community
    \brief Implement access calibration table.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2008/03/07 initial version
    \date 2010/01/05 bug fix: change time arithmetic to 64bit 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "calibration_table.h"
#include "my_spdlog.hpp"

#define MAX_LINE_LENGTH 1024
#define MAX_TABLE_SIZE 128

typedef struct _CTableEntry {time_t t; char fname[_MAX_PATH];} CTableEntry;
static CTableEntry *ctable=NULL;
static int ctable_size=0;

static int compare_entry(const void *va, const void *vb)
{
  const CTableEntry *a = (const CTableEntry*)va;
  const CTableEntry *b = (const CTableEntry*)vb;
  if (a->t < b->t) return(-1);
  if (a->t > b->t) return 1;
  return 0;
}

/* time_encode(int year, int mon, int day, int hour=0, int min=0, int sec=0)
   Just time encoding to enable sorting, not true time encoding
*/
static time_t time_encode(int year, int mon, int day, 
                          int hour, int min, int sec)
{
  time_t i64 = year; // force 64 bit arithmetic
  time_t t = ((((((((((i64-1900)*366)+(mon-1))*12)+day)*31)+hour)*60)+min)*60)+sec;
  return t;
}
/**
 * Do load open filename in memory table.
 * Returns number of loaded elements.
 */
static int
do_load(const char *dir, const char *fname)
{
  FILE *fp=NULL;
	char buffer[MAX_LINE_LENGTH];
  char *val=NULL;
  int year,mon,day,hour,min,sec, valid=1;
   sprintf(ctable[ctable_size].fname,"%s/%s", dir, fname);

  if ((fp=fopen(ctable[ctable_size].fname,"rb")) == NULL) 
  {
    LOG_ERROR("error opening {}", ctable[ctable_size].fname);
    return 0;
  }

  // initialize stm
	while (fgets(buffer,MAX_LINE_LENGTH,fp))
  {
    if (strstr(buffer,"!study date (dd:mm:yryr") != NULL)
    {
      if ( (val = strstr(buffer,":=")) != NULL)
      {
        if (sscanf(val+2, "%d:%d:%d",&day,&mon,&year) != 3) valid = 0;
      }
    }
    else if (strstr(buffer,"!study time (hh:mm:ss") != NULL)
    {
      if ( (val = strstr(buffer,":=")) != NULL)
      {
        if (sscanf(val+2, "%d:%d:%d",&hour,&min,&sec) != 3) valid=0;
      }
    }
  }
  fclose(fp);
  if (valid) 		
  {
    ctable[ctable_size].t = time_encode(year,mon,day,hour,min,sec);
    ctable_size += 1;
  }
//    ctable.insert(make_pair(time_encode(year,mon,day,hour,min,sec),fname));
	return 1;
}

/*! \brief Load calibration files from specified directory in memory table.
    \param[in] directory      calibration directory
    \return 0 on success  or error code on failure

 * Load calibration files from specified directory in memory table.
 * Returns 0 on success  or error code on failure
 */
int 
calibration_load(char *path)
{
  int i=0;
  DIR *dir;
  char fname[256], *ext=NULL;
  struct dirent *item;

  if ((dir = opendir(path)) == NULL)  {
    LOG_ERROR(path);
    return 0;
  }
  if (ctable==NULL) ctable = (CTableEntry*)calloc(sizeof(CTableEntry), MAX_TABLE_SIZE);
  while ( (item=readdir(dir)) != NULL)  {
    if ((ext=strrchr(item->d_name,'.')) != NULL)
      do_load(path,item->d_name);
  }
   // sort by date
  qsort(ctable, ctable_size, sizeof(CTableEntry), compare_entry);
  return ctable_size;
}
/*! \brief Search specified scan time from memory table
    \param[in] year      scan year
    \param[in] mon        scan month
    \param[in] day        scan day
    \param[in] hour       scan hour
    \param[in] min        scan min
    \param[in] sec        scan sec
    \return calibration filename if found  or NULL otherwise

    Find specified scan date and time from memory table.
    Returns 1 if found or 0 otherwise.
 */
const char *calibration_find(int year, int mon, int day, 
                             int hour, int min, int sec)
{
  int i=0;
  int prev=0;                                    // calibration time
  time_t st = time_encode(year,mon,day,hour,min,sec); // scan time;
  if (ctable_size == 0) return NULL;
  if (st < ctable[prev].t) return NULL;
  for (i=1; i<ctable_size; i++)
  {
    if (st < ctable[i].t) return ctable[prev].fname;
    prev=i;
  }
  return ctable[prev].fname; 
}
 
/*! \brief Clear memory table
    \return 0 on success and error code on failure
    
    Clear memory table allocated in calibration_load.
    Returns 0 if OK or error code otherwise.
 */
int 
calibration_clear()
{
  if (ctable_size > 0) memset(&ctable,0,sizeof(CTableEntry)*ctable_size);
  ctable_size = 0;
  return 0;
}
