#ifndef HEADER_H_
#define HEADER_H_

typedef struct {
  int nelems;
  int nangles;
  int nplanes;
  float pixelsize;
  char* prompt;
  char* delayed;
  char* norm;
  char* atten;
  char* scatter;
  int gapfill;
  char* correctedname;
} HeaderInfo;

int constructheader(char* inheader, char* outheader, HeaderInfo header);

#endif /*HEADER_H_*/
