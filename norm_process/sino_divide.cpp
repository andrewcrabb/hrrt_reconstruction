#include <stdio.h>
#include <math.h>
#include <string.h>
#define AVERAGE
//#define GRA_GRB
#define NPROJS 256
#define NVIEWS 288
#define NSINOS 207
#define SKIP 6
static float sino1[NPROJS*NVIEWS];
static float sino2[NPROJS*NVIEWS];
static float sino3[NPROJS*NVIEWS];
void main(int argc, char ** argv)
{
  int i=0, sino=0;
	if (argc == 4)
	{
		FILE *fp1 = fopen(argv[1],"rb");
		FILE *fp2 = fopen(argv[2],"rb");
		FILE *fp3 = fopen(argv[3],"wb");
		if (fp1 != NULL && fp2 != NULL && fp3 != NULL)
		{
      while (fread(sino1,  sizeof(float), NPROJS*NVIEWS, fp1)==NPROJS*NVIEWS &&
        fread(sino2, sizeof(float), NPROJS*NVIEWS, fp2)==NPROJS*NVIEWS)
      {
        if (++sino <= SKIP) continue;
#ifdef AVERAGE
        for (i=0; i<NPROJS*NVIEWS; i++)
        {
          if (sino1[i]*sino2[i] != 0.0f) sino3[i] += sino1[i]/sino2[i];
        }
#else
        for (i=0; i<NPROJS*NVIEWS; i++)
        {
          if (sino1[i]*sino2[i] != 0.0f) sino3[i] = sino1[i]/sino2[i];
          else sino3[i]=0.0f;
        }
        fwrite(sino3, sizeof(float), NPROJS*NVIEWS, fp3);
#endif
        if (sino == NSINOS-SKIP) break;
			}
#ifdef AVERAGE
       for (i=0; i<NPROJS*NVIEWS; i++) sino3[i] /= (NSINOS-2*SKIP);
#ifdef GRA_GRB
       for (i=0; i<NPROJS*NVIEWS; i++)
         if (sino3[i>0]) sino3[i] = sqrt(sino3[i]);
#endif
       fwrite(sino3, sizeof(float), NPROJS*NVIEWS, fp3);
#endif
      fclose(fp1);
			fclose(fp2);
			fclose(fp3);
		}
		else printf("error opening file\n");
	}
	else printf("usage: %s num_file denum_file out_file\n", argv[0]);
}