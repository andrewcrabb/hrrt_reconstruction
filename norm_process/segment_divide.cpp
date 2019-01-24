#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gen_delays_lib/gen_delays.h>
#include <gen_delays_lib/geometry_info.h>
#include <gen_delays_lib/lor_sinogram_map.h>

#define AVERAGE
#define GRA_GRB
#define NPROJS 256
#define NVIEWS 288
#define NSINOS 207
// #define NRINGS 104
// #define NMPAIRS 20
#define MAXSEG 45
static float sino1[NPROJS*NVIEWS*NSINOS];
static float sino2[NPROJS*NVIEWS*NSINOS];
static char mp_mask[3][NPROJS*NVIEWS]; 
static unsigned char rebin_dwell[NPROJS*NVIEWS];
static double ratio[MAXSEG][GeometryInfo::NMPAIRS+1];
static  int count[MAXSEG][GeometryInfo::NMPAIRS+1];
static short ordered_mp[] = {0,3,8,13,17,  /* opposite heads */
                               2,7,12,16,   /*opposite head -1 */
                               4,9,14,19,  /*opposite head +1 */
                               1,6,11,15,  /*opposite head -2 */
                               5,10,18,20  /*opposite head +2 */
                             };
static FILE *log_fp=NULL;

static void divide_planes(FILE *fp1, FILE *fp2, int seg, int nplns)
{
  int i=0, sino=0, mp=0;
  for (mp=0; mp<=GeometryInfo::NMPAIRS; mp++) ratio[seg][mp] = count[seg][mp] = 0;
  for (sino=0; sino<nplns; sino++)
  {
    if (fread(sino1, sizeof(float), NPROJS*NVIEWS, fp1)!=NPROJS*NVIEWS)
    {
      printf("Error reading file 1\n");
      exit(1);
    }
    if (fp2!=NULL)
    {
      if (fread(sino2, sizeof(float), NPROJS*NVIEWS, fp2)!=NPROJS*NVIEWS)
      {
        printf("Error reading file 2\n");
        exit(1);
      }
    }
    else
    {   
      for (i=0; i<NPROJS*NVIEWS; i++)
      {
        if (rebin_dwell[i]>0) sino2[i]=1.0f/rebin_dwell[i];
        else sino2[i]=0;
      }
    }

    if ((sino-6) >=0 || (sino+6)<nplns) // ignore 6 border planes
    {
      for (i=0; i<NPROJS*NVIEWS; i++)
      {
        if (sino1[i]>0 && sino1[i]<5 && 
          sino2[i]>0 && sino2[i]<5 ) // ignore outliers
        {
          mp = mp_mask[0][i];
          ratio[seg][mp] += sino1[i]/sino2[i];
          count[seg][mp]++;
        }
      }
    }
  }
  for (mp=1; mp<=GeometryInfo::NMPAIRS; mp++) 
  {
    ratio[seg][0] += ratio[seg][mp]; 
    count[seg][0] += count[seg][mp];
    if (count[seg][mp]>1) ratio[seg][mp] /= count[seg][mp];
  }

  printf("%g", ratio[seg][ordered_mp[1]]);
  for (mp=2; mp<=GeometryInfo::NMPAIRS; mp++)
    printf( ",%g", ratio[seg][ordered_mp[mp]]);
  printf("\n");
}

void init_logging(void) {
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("{}_segment_divide.log", hrrt_util::time_string());
  }
  g_logger = spdlog::basic_logger_mt("HRRT", g_logfile);
}


void main(int argc, char ** argv)
{
  int i=0, j=0, sino=0;
  int rd=GeometryInfo::MAX_RINGDIFF, span=0, seg=0, nseg=0;
  int ntotal_3, ntotal_9, nplns=0, nplns0=207;
	struct _stat st1, st2;
  FILE *fp1=NULL, *fp2=NULL;
  // const char *rebinner_lut_file=NULL;
  int *m_segzoffset=NULL;
  int mp, al, bl, ax, axx, bx, bxx, idx;
  float d_theta=1.0, plane_sep=1.21875f, crystal_radius=234.5f;

  nseg = (2*rd+1)/3;
	ntotal_3 = nseg*nplns0 - (nseg-1)*(3*(nseg-1)/2+1);
	nseg = (2*rd+1)/9;
	ntotal_9 = nseg*nplns0 - (nseg-1)*(9*(nseg-1)/2+1);
  int N=15;
  double a=6.5;
  for (double x=-7; x<=7; x++)
  {
    double x1 = a*(x/(N/2));
    double g = exp(-(x1 *x1)/2);
    printf("gaussian(%g)=%g\n",x,g);
  }
  // if ((rebinner_lut_file=hrrt_rebinner_lut_path())==NULL)
  // {
  //   fprintf(stdout,"Rebinner LUT file not found\n");
  //   exit(1);
  // }
  lor_sinogram::init_lut_sol(m_segzoffset);
  memset(rebin_dwell,0,NPROJS*NVIEWS);
  for (mp=1; mp<=GeometryInfo::NMPAIRS; mp++)  {
    for (al=0;al<GeometryInfo::NDOIS;al++)    { 
    //layer
      for (ax=0;ax<GeometryInfo::NXCRYS;ax++)      { 
        axx=ax+GeometryInfo::NXCRYS*al; 
        for (bl=0;bl<GeometryInfo::NDOIS;bl++)        {
          for (bx=0;bx<GeometryInfo::NXCRYS;bx++)          {
            bxx=bx+GeometryInfo::NXCRYS*bl;
            if ((idx=lor_sinogram::solution_[mp][axx][bxx].nsino) >=0 ) {
              mp_mask[0][idx] = mp;
              mp_mask[1][idx] = GeometryInfo::HRRT_MPAIRS[mp][0];
              mp_mask[2][idx] = GeometryInfo::HRRT_MPAIRS[mp][1];
              rebin_dwell[idx] += 1;
            }
          }
        }
      }
    }
  }
  if ((fp1=fopen("mp_mask.s","wb")) != NULL)  {
    fwrite(mp_mask[0], sizeof(char),NPROJS*NVIEWS, fp1);
    fwrite(mp_mask[1], sizeof(char),NPROJS*NVIEWS, fp1);
    fwrite(mp_mask[2], sizeof(char),NPROJS*NVIEWS, fp1);
    fwrite(rebin_dwell, sizeof(char),NPROJS*NVIEWS, fp1);
    fclose(fp1);
  }

  if (argc > 1)	{
    if (_stat(argv[1],&st1) == -1)    {
      perror(argv[1]);
      exit(1);
    }
    size_t data_size = st1.st_size;
    if (data_size== (size_t)(NPROJS*NVIEWS*ntotal_3*sizeof(float))) 
      span = 3;
    else if (data_size == (size_t)(NPROJS*NVIEWS*ntotal_9*sizeof(float))) 
      span = 9;
    else     {
      printf("%s size= %d : not span3 or span9 size\n", argv[1], data_size);
      exit(1);
    }
    if (argc>2)    {
      if (_stat(argv[2],&st2) == -1)      {
        perror(argv[2]);
        exit(1);
      }
      if (st1.st_size != st2.st_size)      {
        printf("%s and %s sizes are different\n", argv[1], argv[2]);
        exit(1);
      }
    }
		if ((fp1 = fopen(argv[1],"rb")) == NULL)     {
      perror(argv[1]);
      exit(1);
    }
		if (argc>2)    {
      if ((fp2 = fopen(argv[2],"rb")) == NULL)      {
        perror(argv[2]);
        exit(1);
      }
    }
	  nseg = (2*rd+1)/span;

    d_theta = (float)(atan(span*plane_sep/crystal_radius) * 180/M_PI);

    log_fp = fopen("segment_divide_log.txt","wt");
    fprintf(log_fp, "segment, theta, mp%d", ordered_mp[1]);
    for (mp=2; mp<=GeometryInfo::NMPAIRS; mp++)
      fprintf(log_fp, ",mp%d", ordered_mp[mp]);
    fprintf(log_fp,"\n");

    printf("segment 0 planes %d\n", nplns0);
    divide_planes(fp1, fp2, nseg/2, nplns0);

		for (seg=1; (2*seg)<nseg;  seg++)		{
      nplns = 2*GeometryInfo::NRINGS - span*(2*seg -1) -2;  
      printf("segment +/- %d planes %d\n", seg, nplns);
      divide_planes(fp1, fp2, nseg/2 -seg, nplns); // -
      divide_planes(fp1, fp2, nseg/2 +seg, nplns); // +
    }
    fclose(fp1);
    if (fp2!=NULL) fclose(fp2);
    for (i=0; i<nseg; i++)    {
      int omp; // ordered mp
      seg = i -nseg/2;
      omp = ordered_mp[1];
      fprintf(log_fp,"%d, %g, %g", seg, seg*d_theta,
        span==3? ratio[i][omp]/3:ratio[i][omp]);
      for (mp=2; mp<=GeometryInfo::NMPAIRS; mp++)
      {
        omp = ordered_mp[mp];
        fprintf(log_fp, ", %g", span==3? ratio[i][omp]/3:ratio[i][omp]);
      }
      fprintf(log_fp, "\n");
    }
    fclose(log_fp);
    // compute all mp average
    for (i=0; i<nseg; i++)    {
      ratio[i][0] /= count[i][0];
    }
    seg=0; //segment 0
    double seg_ratio = ratio[nseg/2][0];
    printf("%d, %g, %g\n", seg, seg*d_theta,span==3? seg_ratio/3:seg_ratio);
    for (seg=1; (2*seg)<nseg;  seg++)     {
      seg_ratio = (ratio[nseg/2-seg][0]+ratio[nseg/2+seg][0])/2;
      printf("%d, %g, %g\n", seg, seg*d_theta,span==3? seg_ratio/3:seg_ratio);
    }
	}
	else 
    printf("usage: %s norm1_file norm2_file\n", argv[0]);
}