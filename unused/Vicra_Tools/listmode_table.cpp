/*! \file listmode_table.cpp
    \copyright HRRT User Community
    \brief Implement access calibration table.
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2008/03/07 initial version
    \date 2010/01/05 bug fix: change time arithmetic to 64bit 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "listmode_table.h"
#ifdef WIN32
#include <direct.h>
#include <io.h>
#define chdir _chdir
#define getcwd _getcwd
#define strdup _strdup
#else
#include <dirent.h>
#include <unistd.h>
#define _MAX_PATH 256
#endif

#define MAX_LINE_LENGTH 1024
#define MAX_TABLE_SIZE 128

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

/**
 * Do load open filename in memory table.
 * Returns number of loaded elements.
 */
static int
do_load(const char *dir, const char *fname)
{
  FILE *fp=NULL;
	char buffer[MAX_LINE_LENGTH];
	char path[FILENAME_MAX];
  char *val=NULL;
  int year,mon,day,hour,min,sec, valid=1;
  float fsec=-1.0f;
#ifdef WIN32
   sprintf(path,"%s\\%s", dir, fname);
#else
   sprintf(path,"%s/%s", dir, fname);
#endif
   ctable[ctable_size].em_flag=0;

  if ((fp=fopen(path,"rb")) == NULL) 
  {
    printf("error opening %s\n", path);
    return 0;
  }

  // initialize stm
	while (fgets(buffer,MAX_LINE_LENGTH,fp))
  {
    if (strstr(buffer,"!study date (dd:mm:yryr") != NULL) {
      if ( (val = strstr(buffer,":=")) != NULL) {
        if (sscanf(val+2, "%d:%d:%d",&day,&mon,&year) != 3) valid = 0;
      }
    } else if (strstr(buffer,"!study time (hh:mm:ss.sss") != NULL) {
      if ( (val = strstr(buffer,":=")) != NULL)  {
        if (sscanf(val+2, "%d:%d:%f",&hour,&min,&fsec) != 3) valid=0;
        if (valid) sec = (int)sec;
      }
    } else if (strstr(buffer,"!study time (hh:mm:ss") != NULL) {
      if ( (val = strstr(buffer,":=")) != NULL) {
        if (sscanf(val+2, "%d:%d:%d",&hour,&min,&sec) != 3) valid=0;
        if (valid) fsec = (float)sec;
      }
    } else if (strstr(buffer,"!PET data type") != NULL) {
      if ( (val = strstr(buffer,":=")) != NULL)
        if (strstr(val+2,"emission") != NULL) ctable[ctable_size].em_flag=1;
    } else if (strstr(buffer,"Frame definition") != NULL) {
      if ( (val = strstr(buffer,":=")) != NULL)
        strcpy(ctable[ctable_size].frame_def, val+2);
    }
  }
  fclose(fp);
  if (valid) 		
  {
    strcpy(ctable[ctable_size].fname, fname);
    ctable[ctable_size].t = time_encode(year,mon,day,hour,min,sec);
    ctable[ctable_size].ft = fsec;
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
#ifdef WIN32
int 
listmode_table_load(char *dir)
{
  int i=0;
  char *cwd=NULL;
  struct _finddata_t c_file;
  long  hFile;

  if (ctable==NULL) ctable = (CTableEntry*)calloc(sizeof(CTableEntry), MAX_TABLE_SIZE);
  // save working directory
  if ( (cwd=getcwd(NULL,0)) == NULL) 
  {
    perror("calibration_load::getcwd error");
    return 0;
  }

  // change to requested directory
  if (chdir(dir) != 0)
  {
    perror(dir);
    free(cwd);
    return 0;
  }

  if( (hFile = _findfirst( "*l64.hdr", &c_file )) != -1)
  do
  {
    do_load(dir, c_file.name);
  }  while( _findnext(hFile, &c_file ) == 0 );
  _findclose(hFile);

  // restore workong directory
  if (chdir(cwd) != 0) perror(cwd);

   // sort by date
    qsort(ctable, ctable_size, sizeof(CTableEntry), compare_entry);

  return ctable_size;
}
#else
int 
listmode_table_load(char *path)
{
  int i=0;
  DIR *dir;
  char fname[256], *ext=NULL;
  struct dirent *item;

  if ((dir = opendir(path)) == NULL)
  {
    perror(path);
    return 0;
  }
  if (ctable==NULL) ctable = (CTableEntry*)calloc(sizeof(CTableEntry), MAX_TABLE_SIZE);
  while ( (item=readdir(dir)) != NULL)
  {
    if ((ext=strrchr(item->d_name,'.')) != NULL)
      do_load(path,item->d_name);
  }
   // sort by date
  qsort(ctable, ctable_size, sizeof(CTableEntry), compare_entry);
  return ctable_size;
}
#endif
/*! \brief Search specified scan time from memory table
    \param[in] year      scan year
    \param[in] mon        scan month
    \param[in] day        scan day
    \param[in] hour       scan hour
    \param[in] min        scan min
    \param[in] sec        scan sec
    \return emission start time wrt to tracking time if found
    \or -1 otherwise

    Find specified scan date and time from memory table.
    Returns relative time if found or 0 otherwise.
 */
const CTableEntry *listmode_find(time_t tt, int duration)
{
  int i=0;
  int prev=0;                                    // calibration time
  if (ctable_size == 0) return NULL;
  for (i=0; i<ctable_size; i++)
  {
    if (!ctable[i].em_flag) continue;
    if (ctable[i].t>tt && ctable[i].t<tt+duration) //scan time in tracking period
      return &ctable[i];
    prev=i;
  }
  return NULL; 
}
 
/*! \brief Clear memory table
    \return 0 on success and error code on failure
    
    Clear memory table allocated in calibration_load.
    Returns 0 if OK or error code otherwise.
 */
int 
listmode_table_clear()
{
  if (ctable_size > 0) memset(&ctable,0,sizeof(CTableEntry)*ctable_size);
  ctable_size = 0;
  return 0;
}
