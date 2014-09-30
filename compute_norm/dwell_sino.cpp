#include <stdlib.h>
#include <stdio.h>
#include "norm_globals.h"
#define DEFAULT_DWELL_FNAME "dwellc.n"
#ifdef WIN32
#include <windows.h>
#define SEPARATOR '\\'
#else
#define SEPARATOR '/'
#endif

int dwell_sino(float *dwell, int npixels)
{
  char filename[FILENAME_MAX];
  int use_reg=0;

#ifdef WIN32
  HKEY hkey;                  // registry key
  DWORD dwSize, dwType;
  LPCSTR key = "SOFTWARE\\CPS\\HRRT";
  LPCSTR subkey = "DWELL_SINO";
  use_reg = 1;
  if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, key , 0, NULL,
		     REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkey,
		     NULL) != ERROR_SUCCESS)
    use_reg = 0;
  if (use_reg && RegQueryValueEx(hkey, subkey, NULL, &dwType, NULL, &dwSize) != ERROR_SUCCESS)
    use_reg = 0;
  if (dwType != REG_SZ) use_reg = 0;
  if (use_reg &&RegQueryValueEx(hkey, subkey, NULL, NULL, (unsigned char*)filename,
                                &dwSize) != ERROR_SUCCESS) use_reg=0;
#endif
	
  if (!use_reg) 
    {
      char *envp = getenv("GMINI");
      if (envp != NULL )
	{
	  sprintf(filename, "%s%c%s", envp, SEPARATOR, DEFAULT_DWELL_FNAME);
	  use_reg = 0;
	}
      else
	{
	  printf("GMINI environment variable not found\n");
	  return 0;
	}
    }
  FILE *fp = fopen(filename,"rb");
  if (fp == NULL) 
    {
      perror(filename);
      return 0;
    }
  size_t count = fread(dwell,sizeof(float), npixels, fp);
  fclose(fp);
  if (count != (size_t)npixels)
    {
      printf("%s: Error reading file \n",filename);
      return 0;
    }
  if (qc_flag) 
    {
      float mindw = dwell[0], maxdw = dwell[0];
      for (int i=1; i<(int)count; i++)
	{
	  if (mindw>dwell[i]) mindw=dwell[i];
	  else if (maxdw<dwell[i]) maxdw = dwell[i];
	}
      printf("Dwell_range :%g %g\n",mindw,maxdw);
    }
  return 1;
}
