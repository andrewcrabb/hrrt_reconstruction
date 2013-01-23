/******************************************************************************

   interfile.h   Turku PET Centre



DATE:
  2005-04-05 (krs) (Roman Krais) 
  2008-02-21: version 2.0 (Merence Sibomana)
              Replace vector arguments by pointer
              Replace hard code string sizes by macro defined values

******************************************************************************/
#ifndef _INTERFILE_H_
#define _INTERFILE_H_
#ifdef WIN32
#define FILENAME_SIZE MAX_PATH
#endif
#define RETURN_MSG_SIZE 256
#define ERROR_MSG_SIZE 300

/*****************************************************************************/
extern int interfile_read(char *headerName, char *searchWord, char *returnValue,
                          char *errorMessage);
/*****************************************************************************/

#endif
