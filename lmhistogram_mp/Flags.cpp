#include <string.h>
#ifdef WIN32
#define strcasecmp _stricmp
#endif

int flagset(char *flag, int *argc, char **argv, bool case_sensitive)
{
   int i, c;

   for (i = 1; i < *argc; i++)
   {
      if (case_sensitive)  c = strcmp(flag,argv[i]);
      else c = strcasecmp(flag,argv[i]);
      if (c==0)
      {
         for (;argv[i+1]; i++)
         {
            argv[i] = argv[i+1];
         }
         argv[i] = argv[i+1]; // null terminate list
         *argc = *argc - 1;
         return (1);
      }
   }

   return (0);
}

int flagval(char *flag, int *argc, char **argv, char *result, bool case_sensitive)
{
   int i, c=0;

   for (i = 1; (i+1) < *argc; i++)
   {
      if (case_sensitive)  c = strcmp(flag,argv[i]);
      else c = strcasecmp(flag,argv[i]);
      if (c==0)
      {
         strcpy(result,argv[i+1]);
         for (;argv[i+2]; i++)
         {
            argv[i] = argv[i+2];
         }
         argv[i] = argv[i+2];  // null terminate list
         *argc = *argc - 2;
         return (1);
      }
   }

   return (0);
}

