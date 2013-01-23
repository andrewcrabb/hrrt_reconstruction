  /*
   * EcatX software is a set of routines and utility programs
   * to access ECAT(TM) data.
   *
   * Copyright (C) 1996-2008 Merence Sibomana
   *
   * Author: Merence Sibomana <sibomana@gmail.com>
   *
   * ECAT(TM) is a registered trademark of CTI, Inc.
   * This software has been written from documentation on the ECAT
   * data format provided by CTI to customers. CTI or its legal successors
   * should not be held responsible for the accuracy of this software.
   * CTI, hereby disclaims all copyright interest in this software.
   * In no event CTI shall be liable for any claim, or any special indirect or 
   * consequential damage whatsoever resulting from the use of this software.
   *
   * This is a free software; you can redistribute it and/or
   * modify it under the terms of the GNU General Public License
   * as published by the Free Software Foundation; either version 2
   * of the License, or any later version.
   *
   * This software is distributed in the hope that it will be useful,
   * but  WITHOUT ANY WARRANTY; without even the implied warranty of
   * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   * GNU General Public License for more details.

   * You should have received a copy of the GNU General Public License
   * along with this software; if not, write to the Free Software
   * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/*
   Modification History:
   27-JUN-2008: Add sming in the series decription
   22-JAN-2009: Use same series id for all slices
*/

#include "DICOM.h"
#include <stdio.h>
#include <stdlib.h>
#include <ecatx/getopt.h>
#include <ecatx/matrix.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#include <direct.h>
#define stat _stat
#define access _access
#define unlink _unlink
#define F_OK 0
#else
#include <dirent.h>
#include <unistd.h>
#endif

static void usage(const char *pgm) {
  fprintf(stderr,"%s Build %s %s\n", pgm,  __DATE__, __TIME__);
  fprintf(stderr,
    "usage: %s -i matspec [-o|-u] DICOM_dir [-m DICOM_Transfer_dir]\n", pgm);
  fprintf(stderr,
	  "\t-o convert an ECAT file in DICOM files in the specified directory.\n");
  fprintf(stderr,
	  "\t   It is limited to ECAT files converted from DICOM using DICOM2ecat\n");
  fprintf(stderr,
	  "\t-u Use an ECAT file to update DICOM files in the specified directory.\n");
  fprintf(stderr,
	  "\t-m Move updated DICOM files in the specified directory.\n");
  exit(1);
}	
#define Z_MAX 9e+10
#define eps 0.00001
#define MAX_SERIES_DESC_LEN 6
 
typedef struct _Tslice
{
  char fname[256];
  char study_id[68];
  char modality[20];
  char manufacturer[20];
  int serie_id;
  int sequence_id;
  int acq_number;
  int valid_location;
  float location;
  float thickness;
  float rescale_intercept;	/* for CT modality, 0 for others */
  float rescale_slope;		/* for CT modality, 1.0 for others */
  int number;
  short columns;
  short rows;
  short bits_stored;
  short bits_allocated;
} Tslice;

typedef map<int, Tslice> Tslices;

static int compress = 0;
static int reverse = 0;
static int fmri = 0;
int verbose = 0;
static int not_execute = 0;
static MatrixData matrix;
static int is_bigendian=0;
static DICOMMap dcm_map;
static DICOMElem dcm_elem;
static u_char *dcm_buf=0;
static int dcm_buf_size=0;
static int DICOM10_flag=0;

extern int sequence_number;
static int ecat_sw_version = 70;

static int is_dir(const char *path)
{
	struct stat st;
	if (stat(path,&st) < 0) {
		perror(path);
		return 0;
	}
#ifdef unix
	if (S_ISDIR(st.st_mode)) return 0;
	return 1;
#else 
	if (st.st_mode&_S_IFDIR) return 1;
	return 0;
#endif
}

static void DICOM_blank(char *data)
{
  for (char *p = data; *p != '\0'; p++)
    if (*p == '^') *p = ' ';
}

static int get_string(int a, int b, char* data, unsigned data_len)
{
  if (DICOM_get_elem(a,b,dcm_map, dcm_elem)) {
    unsigned len = (data_len-1)> dcm_elem.len? dcm_elem.len:(data_len-1);
    memcpy(data, dcm_buf+dcm_elem.offset, data_len);
    if (len>0) data[len] = '\0';
    if (verbose) fprintf(stderr,"Element (%x,%x) %s\n",a,b,data);
    DICOM_blank(data);
    if (verbose) fprintf(stderr,"Element (%x,%x) %s\n",a,b,data);
    return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x) not found\n",a,b);
  return 0;
}

static int set_string(int a, int b, const char* data, unsigned data_len)
{
  if (DICOM_get_elem(a,b,dcm_map, dcm_elem)) {
    char *tt = (char*)dcm_buf+dcm_elem.offset;
    memset(dcm_buf+dcm_elem.offset, 0, dcm_elem.len);
    unsigned len = (data_len > dcm_elem.len) ? dcm_elem.len:data_len;
    if (len>0) memcpy(dcm_buf+dcm_elem.offset, data, len);
    if (verbose) fprintf(stderr,"Element (%x,%x) %s\n",a,b,data);
    return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x) not found\n",a,b);
  return 0;
}

static int get_int32(int a, int b, int* value)
{
  if (DICOM_get_elem(a,b,dcm_map, dcm_elem)) {
     *value = *((int*)dcm_buf+dcm_elem.offset);
     return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x)not found\n",a,b);
  return 0;
}

static int set_int32(int a, int b, int value)
{
  if (DICOM_get_elem(a,b,dcm_map, dcm_elem)) {
    *((int*)dcm_buf+dcm_elem.offset) = value;
     return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x)not found\n",a,b);
  return 0;
}


static int get_int16(int a, int b, short* value)
{
  if (DICOM_get_elem(a,b,dcm_map,dcm_elem)) {
     *value = *((short*)dcm_buf+dcm_elem.offset);
     return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x)not found\n",a,b);
  return 0;
}

static int set_int16(int a, int b, short value)
{
  if (DICOM_get_elem(a,b,dcm_map,dcm_elem)) {
     *((short*)dcm_buf+dcm_elem.offset) = value;
     return 1;
  }
  if (verbose) fprintf(stderr,"Element (%x,%x)not found\n",a,b);
  return 0;
}

/*
 * ELSCINT CT contains mutiples instances of image elements (0x7FE0 0x0010)
 * get_imagedata checks to element size to read the right image
 * it is used instead of get_imagedata8 and get_imagedata16 found in DICOM.c
 */

static int get_imagedata(Tslice* slice, void *data)
{
  unsigned short gr=0, no=0;
  unsigned pixel_bytes,  image_size;
  
  pixel_bytes = (unsigned)(slice->bits_allocated+7)/8;
  image_size = pixel_bytes*slice->columns*slice->rows;
  if (DICOM_get_elem(0x7FE0,0x0010,dcm_map, dcm_elem)) {
    if (dcm_elem.len < image_size) {
      fprintf(stderr,"Truncated data\n");
      return 0;
    }
    memcpy(data, dcm_buf+dcm_elem.offset, image_size);
    return dcm_elem.offset;
  }
  if (verbose) fprintf(stderr,"Image element (0x7FE0,0x0010)not found\n");
  return 0;
}

static int add_slice(const char * fname, int serie_id, Tslices &slices)
{
  char str[256];
  Tslice slice;
  float x_location, y_location;

  if (!DICOM_open(fname, dcm_buf, dcm_buf_size,dcm_map,DICOM10_flag)) return 0;
  memset(&slice,0,sizeof(slice));
  get_string(0x0008, 0x0060, slice.modality, sizeof(slice.modality));
  get_string(0x0008, 0x0070, slice.manufacturer, sizeof(slice.manufacturer));
  get_string(0x0020, 0x000D, slice.study_id, sizeof(slice.study_id));
  if (get_string(0x0020, 0x0011, str, sizeof(str)))
    sscanf(str,"%d",&slice.serie_id);
  if (serie_id>=0 && serie_id!=slice.serie_id)
  {	/* unrequested serie */
    return 0;
  }
  get_int16(0x0028, 0x0100, &slice.bits_allocated);
  if (slice.bits_allocated<0 || slice.bits_allocated > 16)
  {
    if (verbose)
      fprintf(stderr,
              "%s Error:unsupported number of bits per pixel %d\n",
              fname, slice.bits_allocated);
    return 0;
  }
  get_int16(0x0028, 0x0101, &slice.bits_stored);
  if (slice.bits_stored<0 || slice.bits_stored > 16)
  {
    if (verbose)
      fprintf(stderr,
              "%s Error:unsupported number of bits per pixel %d\n",
              fname, slice.bits_stored);
    return 0;
  }
  slice.rescale_slope = 1.0;
  /* fprintf(stderr,"modality %s\n",slice.modality); */
  if (strncmp(slice.modality,"CT",2)==0)
  {
    if (get_string(0x0028, 0x1052, str, sizeof(str)))
      sscanf(str, "%g", &slice.rescale_intercept);
    if (get_string(0x0028, 0x1053, str, sizeof(str)))
      sscanf(str, "%g", &slice.rescale_slope);
    /* fprintf(stderr,"rescale_intercept %g rescale_slope %g\n"
              ,slice.rescale_intercept,slice.rescale_slope); */
  }
  if (get_string(0x0020, 0x0012, str, sizeof(str))) 
    sscanf(str,"%d",&slice.acq_number);
  get_string(0x0020, 0x0013, str, sizeof(str));
  sscanf(str, "%d", &slice.number);
  get_string(0x0018, 0x0050, str, sizeof(str));
  if (sscanf(str, "%g", &slice.thickness) != 1) 
    if (verbose)
      fprintf(stderr,
              "%s Warning:can't find slice thickness\n", fname);
  if (get_string(0x0020, 0x1041, str, sizeof(str)) &&
    sscanf(str, "%g", &slice.location) == 1)
  {
    slice.valid_location = 1; 
    if (reverse) slice.location *= -1;
  }
  else
  {  /* Use "0x0020, 0x0032" for Philips */
    if (get_string(0x0020, 0x0032, str, sizeof(str)) &&
      sscanf(str, "%g\\%g\\%g", &x_location, &y_location, &slice.location) == 3)
    {
      slice.valid_location = 1;
      if (reverse) slice.location *= -1;
    }
    else
    {
      if (verbose)
        fprintf(stderr,
                 "%s Warning:can't find slice location (%s)\n", fname,str);
    }
  }
  slice.sequence_id = sequence_number;
  strcpy(slice.fname, fname);
  slices.insert(make_pair(slice.number,slice));
  return 1;
}

#ifdef unix
static int split_dir(const char *path, int serie_id, TSlices &slices) {
  DIR *dir;
  char fname[256];
  struct dirent *item;
  
  if (!is_dir(path)) {
    return add_slice(path, serie_id, slices);
  }
  dir = opendir(path);
  if (dir==NULL) {
    perror(path);
    return 0;
  }
  while ( (item=readdir(dir)) != NULL) {
    sprintf(fname, "%s/%s", path, item->d_name);
    if (!is_dir(fname)) {
      num_slices += add_slice(fname, serie_id);
    }
  }
}
#endif
#ifdef WIN32
#include <io.h>
#include <direct.h>
#include <time.h>

static int split_dir(const char *path, int serie_id, Tslices &slices)
{
  struct _finddata_t c_file;
  int hFile=0;
  char cwd[_MAX_PATH];
  char full_path[_MAX_PATH];
  char fname[_MAX_PATH];
  int num_slices=0;

	if (!is_dir(path)) {
		return add_slice(path, serie_id, slices);
	}
	if (_getcwd(cwd, _MAX_PATH) == NULL) return 0;
	if (_chdir(path) == -1) return 0;
	_getcwd(full_path, _MAX_PATH);
    /* Find first file in current directory */
  if( (hFile = _findfirst( "*", &c_file )) == -1 ) return 0;
	if ((c_file.attrib & _A_SUBDIR) == 0) { // normal file
		sprintf(fname, "%s\\%s", full_path, c_file.name);
		num_slices += add_slice(fname, serie_id, slices);
	}
	/* Find the rest of the .c files */
	while( _findnext(hFile, &c_file ) == 0 )
	{
		if ((c_file.attrib & _A_SUBDIR) == 0) { // normal file
			sprintf(fname, "%s\\%s", full_path, c_file.name);
			num_slices += add_slice(fname, serie_id, slices);
		}
	}
	_findclose(hFile);
	_chdir(cwd);
	return num_slices;
}
#endif

static const char *def_study_descr = "HRRT";
static const char *def_series_descr = "3D";

static int  update_dicom_slice(Main_header *mh, Image_subheader *imh, Tslice &slice,
                               const char *transfer_dir, const char *prefix,
                               const char *series_descr)
{
  struct tm *tm;
  char str[40];
  FILE *fp=NULL;
  int count=0;
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char new_fname[_MAX_FNAME];
  char new_full_path[_MAX_PATH];
  char ext[_MAX_EXT];
  char series_id[8];

  time_t t = (time_t) mh->scan_start_time;
  float x_location, y_location, z_location;

  /* Make sure to use a new buffer to get the correct file size */
  if (dcm_buf_size>0) 
  {
    free(dcm_buf); dcm_buf=NULL;
    dcm_buf_size = 0;
  }
  if (!DICOM_open(slice.fname, dcm_buf, dcm_buf_size,dcm_map,DICOM10_flag)) return 0;
  tm = localtime(&t);
			/* set Study date and time */
  if (tm != NULL)
  {
       /* Date format YYYYMMDD */
    sprintf(str,"%d",tm->tm_year+1900);
    if (tm->tm_mon<9) sprintf(str+4,"0%d",tm->tm_mon+1);
    else sprintf(str+4,"%d",tm->tm_mon+1);
    if (tm->tm_mday<10) sprintf(str+6,"0%d",tm->tm_mday);
    else sprintf(str+6,"%d",tm->tm_mday);
    set_string(0x0008, 0x0020, str,strlen(str)); /* study date */
    set_string(0x0008, 0x0021, str,strlen(str)); /* series date */
    set_string(0x0008, 0x0022, str,strlen(str)); /* acquisition date */
    set_string(0x0008, 0x0023, str,strlen(str)); /* image date */

			/* time format hhmmss.ffff */
    if (tm->tm_hour<10) sprintf(str,"0%d",tm->tm_hour);
    else sprintf(str,"%d",tm->tm_hour);
    if (tm->tm_min<10) sprintf(str+2,"0%d",tm->tm_min);
    else sprintf(str+2,"%d",tm->tm_min);
    if (tm->tm_sec<10) sprintf(str+4,"0%d",tm->tm_sec);
    else sprintf(str+4,"%d",tm->tm_sec);
    strcat(str,".0000");
    set_string(0x0008, 0x0030, str, strlen(str));  /*study time */
    set_string(0x0008, 0x0031, str, strlen(str));  /*series time */
    set_string(0x0008, 0x0032, str, strlen(str));  /*acquisition time */
    set_string(0x0008, 0x0033, str, strlen(str));  /*image time */
  }

  set_string(0x0008, 0x0080, mh->facility_name,strlen(mh->facility_name));
  set_string(0x0008, 0x0090, mh->physician_name, strlen(mh->physician_name));
  strncpy(mh->physician_name, str, 31);
  if (strlen(mh->study_description) > 0)
    set_string(0x0008, 0x1030, mh->study_description, strlen(mh->study_description));
  else set_string(0x0008, 0x1030, def_study_descr, strlen(def_study_descr));

  // Set series description
  set_string(0x0008, 0x103e, series_descr, strlen(series_descr));
    // Set series id
  sprintf(series_id,"%d",slice.serie_id);
  set_string(0x0020, 0x0011,series_id, strlen(series_id));

  set_string(0x0008 ,0x1090, "HRRT", 4);	/* ID Manufacturer Model Name */
  set_string(0x0008, 0x0060, "PT", 2);    /* Modality PET */
  set_string(0x0010, 0x0010, mh->patient_name, strlen(mh->patient_name));
  set_string(0x0010, 0x0020, mh->patient_id, strlen(mh->patient_id));
		  /* PAT Patient Birthdate */
  t = (time_t)mh->patient_birth_date;
  tm = localtime(&t);
  if (tm != NULL)
  {
    sprintf(str,"%d",tm->tm_year+1900);
    if (tm->tm_mon<9) sprintf(str+4,"0%d",tm->tm_mon+1);
    else sprintf(str+4,"%d",tm->tm_mon+1);
    if (tm->tm_mday<10) sprintf(str+6,"0%d",tm->tm_mday);
    else sprintf(str+6,"%d",tm->tm_mday);
    set_string(0x0010, 0x0030, str, strlen(str)); 
  }

  set_string(0x0010, 0x0040, mh->patient_sex, 1);
  sprintf(str,"%f", mh->patient_age );
  set_string(0x0010, 0x1010, str, strlen(str));
  sprintf(str,"%f", mh->patient_weight );
  set_string(0x0010, 0x1030, str, strlen(str));

  set_string(0x0018, 0x0015, "Brain", 5);   /* Body Part */
  set_string(0x0018, 0x1020, "HRRT U1.0", 9);   /* Software revisions */
  set_string(0x0018, 0x1030, "PET Brain", 9);   /* Protocol Part */
  set_string(0x0032, 0x1060, "PET Brain", 9);   /* Procedure description */
  set_string(0x0054, 0x1000, "PET Brain", 9);   /* Procedure description */

  x_location = -0.5f*imh->x_dimension*imh->x_pixel_size*10.0f;
  y_location = -0.5f*imh->y_dimension*imh->y_pixel_size*10.0f;
  z_location =  slice.number*imh->z_pixel_size*10.0f;
  sprintf(str,"%g", z_location);
  set_string(0x0020, 0x1041, str,sizeof(str));         /* slice location */
  sprintf(str, "%g\\%g\\%g", x_location, y_location, z_location);
  set_string(0x0020, 0x0032, str, sizeof(str));       /* image location */

  _splitpath(slice.fname, drive, dir, fname, ext);
  if (strstr(fname, "IM_00") == fname) {
    sprintf(new_fname, "%s.%s.IMA", prefix, fname);
  } else strcpy(new_fname, fname);
  if (transfer_dir != NULL)
  {
    sprintf(new_full_path,"%s\\%s%s",transfer_dir, new_fname, ext);
    if ((fp=fopen(new_full_path,"wb")) == NULL) {
      perror(new_full_path);
      return 0;
    }
    count = fwrite(dcm_buf,1,dcm_buf_size,fp);
    if (count != dcm_buf_size) {
      if (count<0) perror(new_full_path);
      else fprintf(stderr,"Write fail : only %d of %d  bytes\n",
		   count, dcm_buf_size);
    }
    fclose(fp);
    unlink(slice.fname);
    strncpy(slice.fname, new_full_path, sizeof(slice.fname)-1);
  }
  else
  {
    if ((fp=fopen(slice.fname,"wb")) == NULL) {
      perror(slice.fname);
      return 0;
    }
    count = fwrite(dcm_buf,1,dcm_buf_size,fp);
    if (count != dcm_buf_size) {
      if (count<0) perror(slice.fname);
      else fprintf(stderr,"Write fail : only %d of %d  bytes\n",
		   count, dcm_buf_size);
    }
    fclose(fp);
    
    // Rename file name to create unique file names if necessary
    if (strstr(fname, "IM_00") == fname) {
      sprintf(new_fname, "%s.%s.IMA", prefix, fname);
      _makepath(new_full_path,drive, dir, new_fname, ext);
      rename(slice.fname, new_full_path);
      strncpy(slice.fname, new_full_path, sizeof(slice.fname)-1);
    }
  }
  return 1;
}

static int ecat2DICOM(MatrixData *matrix, char *out_dir, const char *prefix, 
					  int frame, int plane)
{
  char fname[FILENAME_MAX];
  FILE *fp=NULL;
  Image_subheader *imh=NULL;
  int nblks, data_size, header_size;
  caddr_t header, buf=NULL;
  unsigned char *p;
  u_short g = 0x7fe0, no = 0x0010;
  char image_id[80];

  imh = (Image_subheader*)matrix->shptr;
  if (strncmp(imh->annotation,"DICOM",6) != 0) return 0;
  if (access(out_dir,F_OK) < 0) 
    crash("First create %s directory\n",out_dir);
  
  sprintf(image_id,"%d.%d",frame, plane);
  sprintf(fname,"%s/%s_%s",out_dir, prefix, image_id);
  if ((fp = fopen(fname,W_MODE)) == NULL)
  {
    perror(fname);
    return 0;
  }
  data_size = matrix->xdim*matrix->ydim;
  switch (matrix->data_type)
  {
  default :
    crash("unsupported matrix type : %d", matrix->data_type);
    break;
  case ByteData :
    break;
  case SunShort :
  case VAX_Ix2 :
    data_size *= 2;
  }
  
  nblks = (data_size+511)/512;
  header = matrix->data_ptr + nblks*512;
  header_size = matrix->data_size - nblks*512;
  p = (u_char*)(header + header_size -1);
  for (; header_size>0; header_size--, p--)
    if (*p == 0xff) break;
  if (header_size == 0) crash("no DICOM header found\n");
  header_size--;
  fwrite(header, 1, header_size, fp);
  if (ntohs(1) == 1 && matrix->data_type!=ByteData)
  {
    buf = (caddr_t)malloc(data_size);
    swab(matrix->data_ptr, buf, data_size);
    fwrite(matrix->data_ptr, data_size, 1, fp);
    free(buf);
  } else fwrite(matrix->data_ptr, data_size, 1, fp);
  fclose(fp);
  return 1;
}

static int updateDICOM(Main_header *mh, Image_subheader *imh, char *in_out_dir, 
                       const char *transfer_dir, const char *prefix,
                       unsigned series_id)
{
  Tslices slices;
  char series_descr[20], txt[20];
  const char *file_descr=NULL;
  Tslices::iterator i;

  split_dir(in_out_dir,-1, slices);

          // locate "EM_3D" or "EM_3D_Xmm" or "TX" from filename
  if ((file_descr = strrchr(prefix, '.')) != NULL)
  {
    file_descr++;
    while (!isalpha(*file_descr) && *file_descr!='\0') file_descr++;
    if (*file_descr == '\0') file_descr = NULL;
  }
  if (file_descr != NULL)
  {
    if (strlen(file_descr) > MAX_SERIES_DESC_LEN)
      file_descr += strlen(file_descr) - MAX_SERIES_DESC_LEN;
    strcpy(series_descr, file_descr);
  }
  else strcpy(series_descr, def_series_descr);
  printf("                        Series Description: %s\n", series_descr);
  printf("Enter new Series Description (max %d char): ", MAX_SERIES_DESC_LEN);
  fgets(txt, sizeof(txt)-1, stdin);
  if (strlen(txt)>0) strcpy(series_descr, txt);

  for (i=slices.begin(); i != slices.end(); i++)
    printf("%s %d %g\n", i->second.fname, i->second.number, i->second.location);
  if (slices.size() != imh->z_dimension)
  {
    crash("Error: ECAT number of slices(%d) and number of dicom files(%d)"
      "are different\n", imh->z_dimension, slices.size());
  }
  for (i=slices.begin(); i != slices.end(); i++)
  {
    i->second.serie_id = series_id; // same series id for all slices
    update_dicom_slice(mh, imh, i->second, transfer_dir, prefix, series_descr);
  }
  return (int)(slices.size());
}

void main(int argc, char **argv)
{
  MatDirNode *node;
  MatrixFile *mptr;
  MatrixData *matrix;
  struct Matval mat;
  char in_fname[FILENAME_MAX];
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char prefix[_MAX_FNAME];
  char ext[_MAX_EXT];
  int ftype, frame = -1, matnum=0, c=0;
  vector<int> matnums;
  char *in_spec=NULL, *out_dir=NULL, *update_dir=NULL;
  char *dest_dir=NULL;     // move dicom files to dest_dir
  time_t t;
  struct tm *tm;
  unsigned series_id;
  extern char *optarg;

  while ((c = getopt (argc, argv, "i:o:u:m:v")) != EOF)
  {
    switch (c)
    {
    case 'i' :
      in_spec = optarg;
      break;
    case 'o' :
      out_dir    = optarg;
      break;
    case 'u' :
      update_dir    = optarg;
      break;
    case 'm' :
      dest_dir    = optarg;
      break;
    case 'v' :
      verbose = 1;
    }
  }
  if (in_spec==NULL ||(out_dir==NULL&&update_dir==NULL)) usage(argv[0]);
  matspec( in_spec, in_fname, &matnum);
  mptr = matrix_open(in_fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
  if (mptr == NULL)
  {
    matrix_perror(in_fname);
    exit(0);
  }
  ftype = mptr->mhptr->file_type;
  if (ftype != PetImage && ftype != PetVolume)
  {
    crash("%s : is not a PetImage file\n",in_fname);
  }

  if (matnum != 0) matnums.push_back(matnum);
  else
  {
    node = mptr->dirlist->first;
    while (node) 
    {
      matnums.push_back(node->matnum);
      node = node->next;
    }
  }

  for (unsigned i=0; i<matnums.size(); i++)
  {
    mat_numdoc(matnums[i], &mat);
    matrix = matrix_read(mptr,matnums[i], UnknownMatDataType);
    if (matrix == NULL)
    {
      crash("%d,%d,%d,%d,%d not found\n",
        mat.frame, mat.plane, mat.gate, mat.data, mat.bed);
    }
    else
    {
      printf("processing %d,%d,%d,%d,%d \n",  mat.frame,
        mat.plane, mat.gate, mat.data, mat.bed);
    }
	_splitpath(in_fname, drive, dir, prefix, ext);
    time(&t);
    tm = localtime(&t);
    // Try making series id unique in case of different reconstructions
    // or filtering using time minutes and seconds
    series_id = tm->tm_sec*60 + tm->tm_sec+1; // make sure it is positive
    if (update_dir != NULL)
    {
      if (!updateDICOM(mptr->mhptr,(Image_subheader*)matrix->shptr,update_dir,
                       dest_dir, prefix, series_id))
        crash("%s,%d,%d has not been generated by %s\n",
        in_fname, mat.frame, mat.plane, argv[0]);
      // delete directory if files moved to dest dir
      if (dest_dir != NULL) unlink(update_dir);
    }
    else
    {
      if (!ecat2DICOM(matrix, out_dir, prefix, mat.frame, mat.plane))
        crash("%s,%d,%d has not been generated by %s\n",
        in_fname, mat.frame, mat.plane, argv[0]);
    }
  }
  matrix_close(mptr);
}
