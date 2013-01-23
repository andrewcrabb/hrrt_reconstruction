/* Author : Merence Sibomana <Sibomana@topo.ucl.ac.be> */
/* created: 25-apr-1996 */
/*
 * int air2ecat
 *
 * Returns:
 *	1 if save was successful
 *	0 if save was unsuccessful
 *
 */
/* Modification history: Merence Sibomana <sibomana@gmail.com> 
   11-DEC-2009: Port to 5.2 and replace XDR by XML 
*/

#include "ecat2air.h"
#include <ecatx/matrix.h>
#ifdef WIN32
#include "io.h"
#define access _access
#define unlink _unlink
#define F_OK 0
#define R_OK 4
#else
#include <unistd.h>
#endif

#ifndef FILENAME_MAX 			/* for Sun OS 4.1 */
# define FILENAME_MAX 256
#endif
static char line[FILENAME_MAX];
static char fname[FILENAME_MAX], hdr_fname[FILENAME_MAX];

static MatrixData* air2matrix(AIR_Pixels ***pixels, struct AIR_Key_info *stats, 
                              MatrixData *orig)
{
	MatrixData *matrix=NULL;
	Image_subheader *imh=NULL;
	AIR_Pixels *image_data;
	int elem_size=2, data_type=SunShort;	/* default */
	int i, nvoxels, nblks;
	u_char *bdata = NULL;
	short *sdata = NULL;
	float a,v;

	if (stats->bits == 8) {data_type = ByteData; elem_size = 1; }
	else if (stats->bits == 1) {	/* BitData not yet implemented */
		data_type = ByteData; elem_size = 1;
	} 
	nvoxels = stats->x_dim*stats->y_dim*stats->z_dim;

/* allocate MatrixData */
	matrix = (MatrixData*)calloc(1,sizeof(MatrixData));
	imh = (Image_subheader*)calloc(1,sizeof(Image_subheader));
	memcpy(matrix,orig,sizeof(MatrixData));
	if (orig->shptr) memcpy(imh,orig->shptr,sizeof(Image_subheader));
	else {
		imh->image_min = (int)(matrix->data_min/matrix->scale_factor);
		imh->image_max = (int)(matrix->data_max/matrix->scale_factor);
	}
	matrix->shptr = (caddr_t)imh;
	if (matrix->data_type == VAX_Ix2) 	/* old integer 2 image format */
		imh->data_type = matrix->data_type = SunShort;
	if (matrix->data_type == IeeeFloat) 	/* Interfile float input */
		imh->data_type = matrix->data_type = SunShort;
  if (matrix->data_type != data_type) {
		fprintf(stderr,"air2matrix : incompatible data types \n");
		free_matrix_data(matrix);
		return NULL;
	}
	imh->x_dimension = matrix->xdim = stats->x_dim;
	imh->x_dimension = matrix->ydim = stats->y_dim;
	imh->z_dimension = matrix->zdim = stats->z_dim;
	imh->x_pixel_size = matrix->pixel_size = (float)stats->x_size*.1f; /* in cm */
	imh->y_pixel_size = matrix->y_size = (float)stats->y_size*.1f; /* in cm */
	imh->z_pixel_size = matrix->z_size = (float)stats->z_size*.1f; /* in cm */
	nblks = (nvoxels*elem_size+511)/512;
	matrix->data_size = nblks*512;
	matrix->data_ptr = (caddr_t)calloc(512,nblks);

	/* fill matrix data */
	image_data = pixels[0][0];
	switch(data_type) {
		case ByteData :
		case BitData :
			a = 255.0f/AIR_CONFIG_MAX_POSS_VALUE;
			bdata = (u_char*)matrix->data_ptr;
			for (i=0; i<nvoxels; i++)
				*bdata++ = (int)(0.5+a*(*image_data++));
			break;
		case SunShort:
			sdata = (short*)matrix->data_ptr;
			for (i=0; i<nvoxels; i++, sdata++) {
				v = *image_data++;
        if (v>32768) *sdata = (short)(v - 32768);
        else *sdata=0; // remove negatives 
			}
			break;
	}
	imh->scale_factor = matrix->scale_factor;
	return matrix;
}

int air2ecat(AIR_Pixels ***pixels, struct AIR_Key_info *stats, const char *specs, 
             int permission, const char *comment, const char *orig_specs)
{
	Main_header mh;
	char orig_fname[FILENAME_MAX], *base, *ext;
	MatrixFile* file;
  FILE *fp=NULL, *fpi=NULL;
	MatrixData *orig=NULL, *matrix=NULL;
	MatrixData *slice=NULL;
	Image_subheader *imh = NULL;
	int sw_version,  i_matnum=0, o_matnum=0;
	int cubic=0, interpolate=0;
  int i=0, npixels=0, plane=0;
	char* ecat_version;

	matspec(orig_specs,orig_fname,&i_matnum);
	file = matrix_open(orig_fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
	if (i_matnum == 0) {		/* use first */
		if (file->dirlist->nmats) i_matnum = file->dirlist->first->matnum;
    else {
      printf("%s : no matrix found\n", orig_specs);
      return 0;
    }
	}
	orig = matrix_read(file,i_matnum,MAT_SUB_HEADER);
	if (orig != NULL && orig->zdim == 1) {	/* slice mode */
		struct Matval val;
		mat_numdoc(i_matnum, &val);
		free_matrix_data(orig);
		orig = matrix_read(file,val.frame, GENERIC);
	}
  if (orig == NULL) {
    printf("error reading matrix %s\n", orig_specs);
    return 0;
  }
  if (orig->data_type == IeeeFloat) { // load matrix and convert to short
		free_matrix_data(orig);
		orig = matrix_read(file,i_matnum, GENERIC);
  }
	if (!matspec(specs,fname,&o_matnum)) o_matnum = i_matnum;
	if ((base = strrchr(fname,'/')) == NULL) base = fname;
	else base++;
	if ((ext = strrchr(base,'.')) == NULL) ext = base+strlen(base);

	memcpy(&mh,file->mhptr,sizeof(Main_header));
	matrix_close(file);
		
	if (!permission && access(fname,F_OK)==0) {
		printf("file %s exists, no permission to overwrite\n",fname);
		return 0;
	}
	matrix = air2matrix(pixels, stats,orig);
  if (matrix == NULL) {
    printf("Error converting AIR pixel data to ECAT matrix\n");
    return 0;
  }
	matrix_flip(matrix,0,1,1);   /* radiolgy convention */
	free_matrix_data(orig);
	if ((ecat_version=getenv("ECAT_VERSION")) != NULL) {
#ifdef VERBOSE
 		fprintf(stderr,"current ecat version : %s\n",ecat_version);
#endif
		if (sscanf(ecat_version,"%d",&sw_version)==1 && sw_version>0)
			mh.sw_version = sw_version;
	}
#ifdef VERBOSE
	fprintf(stderr,"sw version %d\n",mh.sw_version);
#endif
	if (strlen(ext) == 0) strcpy(ext,".v"); // ECAT7 default output format
	if (strncmp(ext,".i",2) == 0)
  { // write Interfile format
    float *fdata=NULL;
    short *sdata=NULL;
    npixels = matrix->xdim*matrix->ydim;
    sdata = (short*)matrix->data_ptr;
    if ((fp=fopen(fname,"wb"))==NULL) 
      printf("%s: %d: error opening file %s",__FILE__,__LINE__, fname);
    if ((fdata=(float*)calloc(npixels, sizeof(float)))==NULL) 
      printf("%s: %d: memory allocation error",__FILE__,__LINE__);
    if (fp!=NULL && fdata!=NULL) {
      for (plane=0; plane<matrix->zdim; plane++) {
        for (i=0; i<npixels; i++)
          fdata[i] = sdata[i]*matrix->scale_factor;
        sdata += npixels;
        fwrite(fdata, sizeof(float),npixels,fp);
      }
      fclose(fp);
      sprintf(hdr_fname,"%s.hdr",fname);
      strcat(orig_fname,".hdr");
      if ((fp=fopen(hdr_fname,"wt")) != NULL)
      {
        if ((fpi=fopen(orig_fname,"rt")) != 0) {
          while (fgets(line,sizeof(line),fpi) != NULL) {
            if (strstr(line,"name of data file :=") != NULL)
              fprintf(fp,"name of data file := %s\n", fname);
            else fprintf(fp,"%s", line);
          }
          fclose(fpi);
        } else {
          fprintf(fp, "!INTERFILE\n");
          fprintf(fp, "name of data file := %s\n", fname);
          fprintf(fp, "image data byte order := LITTLEENDIAN\n");
          fprintf(fp, "number of dimensions := 3\n");
          fprintf(fp, "matrix size [1] := %d\n", matrix->xdim);
          fprintf(fp, "matrix size [2] := %d\n", matrix->ydim);
          fprintf(fp, "matrix size [3] := %d\n", matrix->zdim);
          fprintf(fp, "number format := float\n");
          fprintf(fp, "data offset in bytes := 0\n");
          fprintf(fp, "scaling factor (mm/pixel) [1] := %g\n", matrix->pixel_size*10.0f);
          fprintf(fp, "scaling factor (mm/pixel) [2] := %g\n", matrix->pixel_size*10.0f);
          fprintf(fp, "scaling factor (mm/pixel) [3] := %g\n", matrix->z_size*10.0f);
        }
      }
    }
    free_matrix_data(matrix);
    if (fp!=NULL && fdata!=NULL) 
    {
      fclose(fp);
      return 1;
    }
    return 0;
  }
  // else  write ECAT format
  mh.file_type = PetVolume;
  mh.num_planes = matrix->zdim;
  mh.plane_separation = matrix->z_size;
  if ((file=matrix_create(fname,MAT_OPEN_EXISTING, &mh)) == NULL) {
    matrix_perror(fname);
    free_matrix_data(matrix);
    return 0;
  }
  imh = (Image_subheader*)matrix->shptr;
  strncpy(imh->annotation,comment,sizeof(imh->annotation)-1);
  matrix_write(file,o_matnum,matrix);
  free_matrix_data(matrix);
  matrix_close(file);
  return 0;
}


float ecat_AIR_open_header(const char *mat_spec, struct AIR_Fptrs *fp, struct AIR_Key_info *stats, int *flag)

{
	char fname[FILENAME_MAX];
	MatrixFile* file;
	MatrixData *hdr=NULL;
  int matnum=0;

  fp->errcode = 0;
	matspec(mat_spec,fname,&matnum);
	file = matrix_open(fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
  if (file==NULL) {
    fprintf(stderr, "%s : error opening file\n", fname); 
    fp->errcode = 1;
    return 0.0f;
  }
	if (matnum == 0) {		/* use first */
		if (file->dirlist->nmats) matnum = file->dirlist->first->matnum;
    else { 
      fp->errcode = 1;
      fprintf(stderr, "%s : no matrix found\n", mat_spec); 
    }
	}
	if (fp->errcode == 0)
  {
    if ((hdr = matrix_read(file,matnum,MAT_SUB_HEADER)) == NULL) {
      fprintf(stderr, "%s : error opening matrix\n", mat_spec); 
      fp->errcode = 1;
      return 0.0f;
    }
    if (hdr->data_min == hdr->data_max) {
      // read data to find extrema
      hdr = matrix_read(file,matnum,GENERIC);
      if (fabs(hdr->data_max) > fabs(hdr->data_min)) {
        hdr->scale_factor = (float)(fabs(hdr->data_max)/32767);
      } else if (fabs(hdr->data_min)>0) {
        hdr->scale_factor = (float)(fabs(hdr->data_min)/32767);
      }
    }
    stats->bits = 16;
    stats->x_dim = stats->y_dim = hdr->xdim;
    stats->x_size = stats->y_size = hdr->pixel_size;
    stats->z_dim = hdr->zdim;
    stats->z_size = hdr->z_size;
    flag[0] = (int)(hdr->data_min/hdr->scale_factor);
    if (flag[0]==0) flag[0]=-1; // force type 2 data
    flag[1] = (int)(hdr->data_max/hdr->scale_factor);
    flag[2] = 0;
	  free_matrix_data(hdr);
  }
	matrix_close(file);
	if (fp->errcode == 0) return 1.0f;
  else return 0.0f;
}

void AIR_close_header(struct AIR_Fptrs *fp)
{
  fp->errcode = 0;
}

AIR_Error ecat_AIR_load_probr(const char *specs, const AIR_Boolean decompressable_read)

{
	char fname[FILENAME_MAX];
	MatrixFile* file;
	MatrixData  *hdr=NULL;
	int matnum=0;
  int error=0;

	matspec(specs,fname,&matnum);
	file = matrix_open(fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
  if (file==NULL) return 0;
	if (matnum == 0) {		/* use first */
		if (file->dirlist->nmats) matnum = file->dirlist->first->matnum;
    else {
      printf("%s : no matrix found\n", specs);
      error++;
    }
	}
	if (file != NULL) {
    if ((hdr = matrix_read(file,matnum,MAT_SUB_HEADER))==NULL) error++;
    else free_matrix_data(hdr);
  } else matrix_close(file);
  return error;
}
