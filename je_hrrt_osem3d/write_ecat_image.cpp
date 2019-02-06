/*-----------------------------------------------------------------
  write_ecat_image.cpp
  Purpose: Write an float image frame in ECAT7 format,
  create the file fi not existing or add the frame to existing file
  Author: Merence Sibomana for the HRRT Users Community
  Creation date: 05-Feb-2010
  Mdofication dates:
  -------------------------------------------------------------------*/

#include	<ctype.h>
#include	<math.h>
#include	<stdio.h>
#include	<string.h>
#include <time.h>

#include	<ecatx/ecat_matrix.hpp>
#include	"interfile_reader.h"
#include	"write_image_header.h"
#define		DIR_SEPARATOR '/'

#define		LINESIZE 1024
#define		IN_DIR_SEPARATOR '/'
static const char *data_types[] = {"Error", "unsigned integer", "signed integer", "float"};
static char line[LINESIZE];

static int data_bytes[] = {0, 1, 2, 4};

static const char *fname(const char *path) 
{
  const char *pos = strrchr(path, IN_DIR_SEPARATOR);
  if (pos!=NULL) pos = pos+1;
  else pos = path;
  return pos;
}

static int write_image(ecat_matrix::MatrixFile *fp, int frame, ecat_matrix::Image_subheader *imh, 
                       float ***image, unsigned char *mask, int recon_type)
{
  int i, j, nvoxels;
  float minval, maxval, scalef;
  MatrixData *matrix=NULL;
  float *fdata=image[0][0];
  short *sdata=NULL, smax=32766;  // not 32767 to avoid rounding problems
  int matnum=0, nblks = 0;

  matrix = (MatrixData*)calloc(1,sizeof(MatrixData));
  matrix->xdim = imh->x_dimension;	
  matrix->ydim = imh->y_dimension;
  matrix->zdim = imh->z_dimension;
  matrix->pixel_size = imh->x_pixel_size;
  matrix->y_size = imh->y_pixel_size;
  matrix->z_size = imh->z_pixel_size;
  imh->data_type = matrix->data_type = SunShort;
  matrix->data_max = imh->image_max*matrix->scale_factor;
  matrix->data_min = imh->image_min*matrix->scale_factor;
  matrix->data_size = matrix->xdim*matrix->ydim*matrix->zdim*sizeof(short);
  nblks = (matrix->data_size + ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
  matrix->data_ptr = (void *)calloc(nblks, ecat_matrix::MatBLKSIZE);
  minval = maxval = fdata[0];
  nvoxels = matrix->xdim*matrix->xdim*matrix->zdim;
  matnum=ecat_matrix::mat_numcod(frame+1, 1, 1, 0, 0);
  if (mask != NULL)
    {  // Use mask to find image extrema and zero hot or cold pixels

      for (i=0, j=0; i<nvoxels; i++)
        {
          if (mask[i]) {
            if (j++ == 0)
              minval=maxval=fdata[i];
            else if (fdata[i]>maxval)
              maxval = fdata[i];
            else if (fdata[i]<minval)
              minval = fdata[i];
          }
        }
      // truncate hot and cold pixels
      for (i=0; i<nvoxels; i++)
        {
          if (!mask[i])
            {
              if (fdata[i]>maxval)
                fdata[i] = maxval;
              else if (fdata[i]<minval)
                fdata[i]=minval;
            }
        }
    }
  else
    {  // find image extrema in central image
      int start = matrix->xdim*matrix->xdim*(matrix->zdim/4); // start at 1/4 of nplanes
      int end = 3*start;                                      // end at 3/4 of nplanes
      minval = maxval = fdata[0];

      for (i=start; i<end; i++)
        {
          if (fdata[i]>maxval)
            maxval = fdata[i];
          else if (fdata[i]<minval)
            minval = fdata[i];
        }
      // Truncate hot and cold pixels
      for (i=0; i<start; i++) {
        if (fdata[i]>maxval)
          fdata[i]=maxval;
        else if (fdata[i]<minval)
          fdata[i]=minval;
      }
      for (i=end; i<nvoxels; i++) {
        if (fdata[i]>maxval)
          fdata[i]=maxval;
        else if (fdata[i]<minval)
          fdata[i]=minval;
      }
    }
    
  printf("frame %d: image_min,image_max = %g,%g\n",frame, minval,maxval);
  if (fabs(minval) < fabs(maxval))
    scalef = (float)(fabs(maxval)/smax);
  else
    scalef = (float)(fabs(minval)/smax);
  sdata = (short*)matrix->data_ptr;
  for (i=0; i<nvoxels; i++)
    sdata[i] = (short)(0.5+fdata[i]/scalef);
  imh->image_max = find_smax(sdata,nvoxels);
  imh->image_min = find_smin(sdata,nvoxels);
  if (recon_type == 0)
    scalef=1.0f; // no quantification
  matrix->scale_factor = imh->scale_factor=scalef;
  matrix->data_max = imh->image_max*scalef;
  matrix->data_min = imh->image_min*scalef;
  printf("frame %d: image_min,image_max = %d,%d, scale factor=%g\n",frame+1,imh->image_min,imh->image_max,scalef);
  matrix->shptr = (void *)calloc(sizeof(ecat_matrix::Image_subheader),1);
  memcpy(matrix->shptr, imh, sizeof(ecat_matrix::Image_subheader));
  return matrix_write(fp,matnum,matrix);
}

static void tm_txt2time(char *txt /*hh:mm:ss*/, struct tm *tm)
{
  txt[2] = txt[5] = txt[8] = '\0'; 
  tm->tm_hour = atoi(&txt[0]);
  tm->tm_min = atoi(&txt[3]);
  tm->tm_sec = atoi(&txt[6]);
}

static void tm_txt2date(char *val, struct tm *tm)
{
  /* expected format: m[m]/d[d]/yyyy  syntax is not checked! */
  char *p1, *p2; // resp. dd and yyyy start 
  if ((p1 = strchr(val,'/')) != NULL) {
    *p1++ = '\0';
    if ((p2 = strchr(p1,'/')) != NULL) {
      *p2++ = '\0';
      tm->tm_mon = atoi(val)-1;
      tm->tm_mday = atoi(p1);
      tm->tm_year = atoi(p2)-1900;
    }
  }
}


static unsigned int ifh_date(char *txt)
{
  struct tm tm;
  memset(&tm,0,sizeof(tm));
  tm_txt2date(txt, &tm);
  tm.tm_isdst = -1; // don't use daylight saving
  return (unsigned)(mktime(&tm));
}

static unsigned int ifh_datetime(char *date_txt, char *time_txt)
{
  struct tm tm;
  memset(&tm,0,sizeof(tm));
  tm_txt2date(date_txt, &tm);
  tm_txt2time(time_txt, &tm);
  tm.tm_isdst=-1;                      /* allows for daylight saving time */
  return (unsigned int)mktime(&tm);
}

int write_ecat_image(float ***image, char * filename, int frame, 
                     ImageHeaderInfo *info, int psf_flag,
                     const char *sw_version, const char *sw_build_id)
{
  float	eps = 0.001f;
  FILE	*f_in=NULL;
  ecat_matrix::MatrixFile *f_out=NULL;
  Main_header mh;
  ecat_matrix::Image_subheader imh;
  char	sinoHeaderName[_MAX_PATH];
  int		sinoHeaderExists;
  char	*p=NULL;
  IFH_Table ifh;
  int ifh_ok=0;
  char date_txt[20], time_txt[20], value[256];


	
  //Check parameters
  if( info->datatype<1	|| info->datatype>3	
      ||	info->nx<=0			|| info->ny<=0		|| info->nz<=0
      ||	info->dx<=eps		|| info->dy<=eps	|| info->dz<=eps ) {
    printf("WriteImageHeader: Parameter out of whack.\n");
    return -1;
  }

  memset(&mh,0,sizeof(mh));
  memset(&imh,0,sizeof(imh));
  strncpy(mh.magic_number, "MATRIX70", 14);
  mh.sw_version = 72;           /* otherwise Vinci might not like it */
  mh.file_type = 7;                /* let's create a Volume 16 image */
  mh.system_type = 328;   
  strncpy(mh.serial_number,"HRRT",
          sizeof(mh.serial_number)-1);
  mh.transm_source_type =  3;                      /* point source */
  mh.distance_scanned =  25.2281f;              /* axial FOV in cm */
  mh.transaxial_fov = 31.2f;
  mh.angular_compression = 3;                           /* unknown */
  mh.calibration_factor = 0;          /* to be set correctly later */
  /* 0=uncalibrated, 1=calibrated */
  imh.num_r_elements = 256;    /* number of bins in cor. sinogramm */
  imh.num_angles = 288;  /* number of angles in corrected sinogram */
  imh.scatter_type = 2;  
  imh.data_type = 6;                                  /* Sun short */
  imh.num_dimensions = 3;
  imh.x_dimension = info->nx;
  imh.y_dimension = info->ny;
  imh.z_dimension = info->nz;
  imh.x_pixel_size = info->dx*0.1f; // cm
  imh.y_pixel_size = info->dy*0.1f; // cm
  imh.z_pixel_size = info->dz*0.1f; // cm
  mh.num_planes = info->nz;
  mh.num_frames = 1;
  mh.plane_separation = info->dz*0.1f; // cm
  mh.bin_size = info->dx*0.1f; // cm

  //determine if sinogram header exists & if so, what it's called

  sinoHeaderExists = 0;

  if( strlen( info->promptfile ) > 0 ){
    strncpy(mh.original_file_name, info->promptfile, 
            sizeof(mh.original_file_name)-1);
    sprintf(sinoHeaderName, "%s.hdr",info->promptfile);
    printf("ImageHeader from prompt sinoHeaderName  %s\n",sinoHeaderName);
    sinoHeaderExists++;
  } else if( strlen( info->truefile ) > 0 ) { 
    strncpy(mh.original_file_name, info->truefile, 
            sizeof(mh.original_file_name)-1);
    sprintf(sinoHeaderName, "%s.hdr",info->truefile);
    printf("ImageHeader: true sinoHeaderName is %s\n",sinoHeaderName);
    sinoHeaderExists++;
  }
  
  if( sinoHeaderExists ) {
    ifh.tags = ifh.data=NULL;
    ifh.size=0;
    switch(interfile_load(sinoHeaderName,&ifh))
      {
      case IFH_FILE_INVALID:                /* Not starting with '!INTERFILE' */
        fprintf(stderr, "%s: is not a valid interfile header\n",sinoHeaderName);
        break;
      case IFH_FILE_OPEN_ERROR:        /* interfile header cold not be opened */
        fprintf(stderr, "%s: Can't open file\n",sinoHeaderName);
        break;
      default: //file loaded
        ifh_ok = 1;
      }
  }


  if (ifh_ok) {
    interfile_find(&ifh, "Patient name", mh.patient_name, sizeof(mh.patient_name));
    interfile_find(&ifh, "Patient ID"  , mh.patient_id  , sizeof(mh.patient_id));
    if (interfile_find(&ifh,"Patient DOB",value,sizeof(date_txt))) {  
      mh.patient_birth_date = ifh_date(date_txt);
    }
    if (interfile_find(&ifh,"Patient sex",value,sizeof(value))) {
      mh.patient_sex[0] = toupper(value[0]); /*'M' Male,'F' Female */
      if ( mh.patient_sex[0] != 'M' &&  mh.patient_sex[0] != 'F' )
        mh.patient_sex[0] = 'U';      // set to Unknown if not valid
    }
    if (interfile_find(&ifh,"!study date (dd:mm:yryr)",value,sizeof(date_txt)) &&
        interfile_find(&ifh,"!study time (hh:mm:ss)",value, sizeof(time_txt)))
      mh.scan_start_time = ifh_datetime(date_txt, time_txt);
    if (interfile_find(&ifh,"Dose type",value,sizeof(value))) {
      strncpy(mh.isotope_code, value, sizeof(mh.isotope_code)-1);
    }
    if (interfile_find(&ifh,"isotope halflife",value,sizeof(value)))
      mh.isotope_halflife = (float)atof(value); 
    if (interfile_find(&ifh,"branching factor",value,sizeof(value)))
      mh.branching_fraction = (float)atof(value); 
    /* energy window */
    if (interfile_find(&ifh,"energy window lower level[1]",value,
                       sizeof(value))) {
      mh.lwr_true_thres = atoi(value); 
    }
    if (interfile_find(&ifh,"energy window upper level[1]",value,
                       sizeof(value))) {
      mh.upr_true_thres = atoi(value); 
    }
    if (interfile_find(&ifh,"Dosage Strength",value,sizeof(value))) {
      mh.dosage = (float)atof(value);
    }
    if (interfile_find(&ifh,"image relative start time",value,
                       sizeof(value))) {
      imh.frame_start_time = atoi(value)* 1000;       /* im ms */
    }
    if (interfile_find(&ifh,"image duration",value,sizeof(value))) {
      imh.frame_duration  = atoi(value)* 1000;       /* im ms */
    }
  }
  if (!psf_flag) sprintf(imh.annotation,"%s OSEM3D W%d", sw_version, info->recontype);
  else sprintf(imh.annotation,"%s OSEM3D-PSF W%d", sw_version,info->recontype);

  /*--------------------*/
  /* Create or Open Main Header */
  printf("Create or Open Main Header \n");
  if (frame==0) {
  	printf("Create Main Header %s\n", filename);
    if ((f_out=matrix_create(filename, ecat_matrix::MatrixFileAccessMode::CREATE_NEW_FILE, &mh)) == NULL) {
      fprintf(stderr,"ERROR: cannot create '%s'\n", filename);
      return 0;
    }
  } else {
	printf("Open Main Header %s\n", filename);
    if ((f_out=matrix_create(filename, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, &mh)) == NULL) {
      fprintf(stderr,"ERROR: cannot open '%s'\n", filename);
      return 0;
    }
  }
  mh_update(f_out);
  write_image(f_out,frame,&imh,image, NULL, info->recontype);
  matrix_close(f_out);
  return 1;
}
