#include <stdlib.h>
#include <stdio.h>
#include "norm_globals.h"
#include "my_spdlog.hpp"
#define DEFAULT_DWELL_FNAME "dwellc.n"
#define SEPARATOR '/'


int dwell_sino(float *dwell, int npixels) {
  char filename[FILENAME_MAX];

  char *envp = getenv("GMINI");
  if (envp != NULL ) {
    sprintf(filename, "%s%c%s", envp, SEPARATOR, DEFAULT_DWELL_FNAME);
    use_reg = 0;
  } else {
    LOG_ERROR("GMINI environment variable not found\n");
    return 0;
  }

  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) {
    LOG_ERROR(filename);
    return 0;
  }
  size_t count = fread(dwell, sizeof(float), npixels, fp);
  fclose(fp);
  if (count != (size_t)npixels) {
    LOG_ERROR("%s: Error reading file \n", filename);
    return 0;
  }
  if (qc_flag) {
    float mindw = dwell[0], maxdw = dwell[0];
    for (int i = 1; i < (int)count; i++)
    {
      if (mindw > dwell[i]) mindw = dwell[i];
      else if (maxdw < dwell[i]) maxdw = dwell[i];
    }
    LOG_INFO("Dwell_range : {} {}", mindw, maxdw);
  }
  return 1;
}
