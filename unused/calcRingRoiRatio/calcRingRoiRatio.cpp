/*******************************************************************************
SYNTAX:

calcRingRoiRatio <interfile image> [<startplane>] [<endplane>]

DESCRIPTION:
makes ROI evaluation on an image of a 20 cm diameter cylindircal phantom.
1. ROI: circular, centered, 8 cm diameter
2. ROI: ring ROI, centered, difference between a circular ROIs with
        18cm and 16 cm diameter
prints the ratio mean(ROI1)/mean(ROI2). The scatter parameters have to be
chosen that this ratio gets 1.

HISTORY/VERSION:
  2005-06-12 (krs) Roman Krais, 
   quick and dirty (version 0)
  2005-06-28 (JJ) version 0.1
   Usage and version printouts added.
  2005-07-25 (krs) version 0.2
   default start and end plane added.
  2005-08-04 (krs) version 0.3
   print also ratios for every plane, not only the average.

VALIDATION: 
version 0:
  checked for FANRT0009_sp9_3D.i plane 66-142: result: ratio = 1,077
  ROI evaluation in Vinci on same image, same plane range: ratio = 1,077
version 0.3.0
  0.3.0: FANRT0020_10_uusi_s9_ANW_128i02.i plane 70-138: ratio = 1.007784
  0.2.0: FANRT0020_10_uusi_s9_ANW_128i02.i plane 70-138: ratio = 1.007789
  Where does the difference come from? Code checked: probably rounding
version 1.0.0 
  Removed Turku librairies and Big Endian (SUN Sparc?) support (MS)

*******************************************************************************/

#define PROGRAM "calcRingRoiRatio 0.3.0  (c) 2005 by Turku PET Centre"

static const char *PROGRAM_U = "calcRingRoiRatio 1.0.0  (c) 2005 by Turku PET Centre\n"
				"                        (c) 2008 by HRRT Users Community";
/******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interfile_reader.h"                            /* reading interfile header */
/******************************************************************************/

void print_usage();
void print_build();

/*******************************************************************************/
static char  imgName[FILENAME_SIZE],hdrName[FILENAME_SIZE];
static char  value[RETURN_MSG_SIZE], errorMessage[ERROR_MSG_SIZE]; 

int main(int argc, char *argv[])
{
  short int x,y,z; //,is_little=little_endian();
  int       dimx, dimy, dimz;
  int       startplane,endplane;
  int       ROI1numpix=0, ROI2numpix=0, planeROI1numpix=0, planeROI2numpix=0;
  int       ROIradius1, ROIradius2a, ROIradius2b;
  long int  j,k;
  float     planesum, xsum, ysum, pixelSize;
  float     x0, y0, ROI1sum=0, ROI2sum=0, planeROI1sum=0, planeROI2sum=0;
  float     *img;
  FILE      *image;
  IFH_Table ifh;
                                            /* process command line arguments */
  if (argc < 2 || argc > 4) {
    print_usage();
    return 1;
  }
  strcpy(imgName,argv[1]);
  if (argc < 3) 
    startplane=70;
   else 
    startplane=atoi(argv[2]);
  if (argc < 4) 
    endplane=138;
   else 
    endplane=atoi(argv[3]);

  ifh.tags = ifh.data=NULL;
  ifh.size=0;
  printf("reading image header\n");
  strcpy(hdrName,imgName); strcat(hdrName,".hdr"); 
  switch(interfile_load(hdrName,&ifh))
  {
      case IFH_FILE_INVALID:                /* Not starting with '!INTERFILE' */
          fprintf(stderr, "%s: is not a valid interfile header\n",hdrName);
          return 1;

      case IFH_FILE_OPEN_ERROR:        /* interfile header cold not be opened */
          fprintf(stderr, "%s: Can't open file\n",hdrName);
          return 1;

      // default: file loaded
  }

                                                     /* read interfile header */
  interfile_find(&ifh,"scaling factor (mm/pixel) [1]",value,sizeof(value));
  pixelSize = atof(value);
  printf("  pixel size =  %f mm\n",pixelSize);
  interfile_find(&ifh,"matrix size [1]",value,sizeof(value));
  dimx = atoi(value);
  printf("  dimx =        %d\n",dimx);
  interfile_find(&ifh,"matrix size [2]",value,sizeof(value));
  dimy = atoi(value);
  printf("  dimy =        %d\n",dimy);
  interfile_find(&ifh,"matrix size [3]",value,sizeof(value));
  dimz = atoi(value);
  printf("  dimz =        %d\n",dimz);
                                              /* Calculate ROIradius in pixel */
  ROIradius1  = (int)  80/2 / pixelSize;
  ROIradius2a = (int) 180/2 / pixelSize;
  ROIradius2b = (int) 160/2 / pixelSize;
                                          /* allocate memory for image matrix */
  img = (float *) malloc(dimx*dimy*dimz*sizeof(float));
                                                         /* read image matrix */
  if ((image = fopen(imgName,"r"))==NULL) {
    fprintf(stderr, "ERROR opening image %s.\n",imgName);
    return 1;
  }
  fread(&img[0],sizeof (float),dimx*dimy*dimz,image);
  fclose(image);
/*
  if (is_little == 0) {
        printf("swapping bytes\n");
	for (j=0; j<dimx*dimy*dimz; j++) 
          swawbip(&img[j], sizeof(float));
      }
*/
                                                            /* ROI evaluation */
  printf("Ratio innerROI/outerROI plane by plane (first plane is plane 1, not 0):\n\n");
  printf("plane ROI_ratio   ROI_centre(x0/y0)\n");
  printf("---------------------------------------\n");
  j=dimx*dimy*(startplane-1);
  k=dimx*dimy*(startplane-1);
                                                   /* evaluate plane by plane */
  for (z=startplane-1; z<endplane; z++) {
    planesum=0;
    xsum=0;
    ysum=0;
    planeROI1sum=0.0;
    planeROI1numpix=0;
    planeROI2sum=0.0;
    planeROI2numpix=0;
                                                         /* centre of gravity */
    for (y=0; y<dimy; y++) {
      for (x=0; x<dimx; x++) {
        planesum += img[j];
        xsum += x*img[j];
        ysum += y*img[j];
        j ++;         
      }
    }
    x0 = (float) xsum / (float) planesum;
    y0 = (float) ysum / (float) planesum;
                                                      /* "draw" ROIs on plane */
    for (y=0; y<dimy; y++) {
      for (x=0; x<dimx; x++) {
        if (sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0)) <= ROIradius1) {
          planeROI1numpix ++;
          planeROI1sum += img[k];
	}
        if (sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0)) <= ROIradius2a && sqrt((x-x0)*(x-x0)+(y-y0)*(y-y0)) >= ROIradius2b) {
          planeROI2numpix ++;
          planeROI2sum += img[k];
	}
        k ++;
      }
    }
                                                              /* plane output */
    printf("%3d   %f    (%f/%f)\n",z+1,(planeROI1sum/planeROI1numpix)/(planeROI2sum/planeROI2numpix),x0,y0);
    ROI1numpix += planeROI1numpix;
    ROI1sum    += planeROI1sum;
    ROI2numpix += planeROI2numpix;
    ROI2sum    += planeROI2sum;
  }
  printf("---------------------------------------\n");
  free(img);
                                                              /* final output */
  printf("\n");
  printf("Ratio innerROI/outerROI (avg plane %d-%d) = %f\n",startplane,endplane,(ROI1sum/ROI1numpix)/(ROI2sum/ROI2numpix));
  exit(0);
}

void print_usage(){

  printf("\n"
       "  %s\n"
       "  Build %s %s\n\n"
       "  Syntax: calcRingRoiRatio <interfile image> [<startplane>] [<endplane>]\n"
       "\n"
       "  Makes ROI evaluation on an image of a 20 cm diameter cylindrical phantom.\n"
       "  1. ROI: circular, centered, 8 cm diameter\n"
       "  2. ROI: ring ROI, centered, difference between a circular ROIs with\n"
       "     18cm and 16 cm diameter\n"
       "  prints the ratio mean(ROI1)/mean(ROI2).\n"
       "  (has to be 1.0 for uniformity phantoms)\n"
       "\n"
       "  Defaults: startplane=70  endplane=138\n"
       "            (the first plane is plane 1, not 0)\n"
       "\n",
       PROGRAM_U, __DATE__, __TIME__);

  fflush(stdout);
}

/******************************************************************************/
/*                        print_build()                                       */
/******************************************************************************/
void print_build(){

  printf("%s\n",PROGRAM_U);
  printf("\n Build %s %s\n\n",__DATE__,__TIME__);
  printf("***   Libraries   ***\n");
}

