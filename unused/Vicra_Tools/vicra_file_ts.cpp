/*
   vicra_file_ts.c
   Purpose         : Time stamp vicra tracking file with creation time
                     in millseconds on windows systems.
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 24-sep-2009
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#define LINE_SIZE 1024

static char line[LINE_SIZE];
int file_ts(char *in_file)
{
  char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
  char out_file[MAX_PATH];
  FILE *fpi=NULL, *fpo=NULL;

  FILETIME creation_t, modif_t, access_t, lt;
  SYSTEMTIME st;
  HANDLE h;

  h = CreateFile (in_file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	if (h != INVALID_HANDLE_VALUE) 
  {
    if (GetFileTime(h, &creation_t, &access_t, &modif_t)!= NULL)
    {
      FileTimeToLocalFileTime(&creation_t, &lt);
      if (FileTimeToSystemTime(&lt, &st))
      {
        printf("%04d/%02d/%02d %02d:%02d:%02d.%03d\n", 
          st.wYear,st.wMonth,st.wDay,
          st.wHour,st.wMinute,st.wSecond, st.wMilliseconds);
        CloseHandle(h);
        if ((fpi=fopen(in_file,"rt")) != NULL) {
          fgets(line, sizeof(line), fpi);
          if (strncmp(line, "Tools", 5) != 0) {
            fprintf(stderr,"%s not a valid vicra non time stamped file\n", in_file);
            fclose(fpi);
            return 0;
          }
          _splitpath(in_file, drive, dir, fname, ext);
          sprintf(out_file,"%s%s%s_TS%s", drive, dir, fname, ext);
          if ((fpo=fopen(out_file,"wt")) != NULL) {
            fprintf(fpo,"Creation time := %02d:%02d:%04d, %02d:%02d:%02d.%03d\n", 
              st.wDay,st.wMonth,st.wYear,st.wHour,st.wMinute,st.wSecond, 
              st.wMilliseconds);
            fputs(line,fpo);
            while (fgets(line, sizeof(line), fpi) != NULL)  fputs(line,fpo);
            fclose(fpo);
          }
          fclose(fpi);
        }
        CloseHandle(h);
        return 1;
      }  else fprintf(stderr,"Error converting creation time\n");
    } else fprintf(stderr, "%s: Error GetFileTime()\n", in_file);
    CloseHandle(h);
  } else   fprintf(stderr, "Error opening %s\n", in_file);
  return 0;
}

int main(int argc, char ** argv)
{
  for (int i=1; i<argc; i++) file_ts(argv[i]);
  return 1;
}