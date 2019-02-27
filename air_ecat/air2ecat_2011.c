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
#include <ecatx/ecat_matrix.hpp>
#include <unistd.h>
#include "my_spdlog.hpp"

static char line[FILENAME_MAX];
static char fname[FILENAME_MAX], hdr_fname[FILENAME_MAX];

static ecat_matrix::MatrixData* air2matrix(AIR_Pixels ***pixels, struct AIR_Key_info *stats, ecat_matrix::MatrixData *orig) {
  ecat_matrix::MatrixData *matrix=NULL;
  ecat_matrix::Image_subheader *imh=NULL;
  AIR_Pixels *image_data;
  int elem_size=2;
  ecat_matrix::MatrixDataType data_type = ecat_matrix::MatrixDataType::SunShort;	/* default */
  int i, nvoxels, nblks;
  float a,v;

  if (stats->bits == 8) {
    data_type = ecat_matrix::MatrixDataType::ByteData; 
    elem_size = 1;
  }  else if (stats->bits == 1) {
	/* BitData not yet implemented */
    data_type = ecat_matrix::MatrixDataType::ByteData; 
    elem_size = 1;
  } 
  nvoxels = stats->x_dim*stats->y_dim*stats->z_dim;

  /* allocate ecat_matrix::MatrixData */
  matrix = (ecat_matrix::MatrixData*)calloc(1,sizeof(ecat_matrix::MatrixData));
  imh = (ecat_matrix::Image_subheader*)calloc(1,sizeof(ecat_matrix::Image_subheader));
  memcpy(matrix,orig,sizeof(ecat_matrix::MatrixData));
  if (orig->shptr) {
    memcpy(imh,orig->shptr,sizeof(ecat_matrix::Image_subheader));
  } else {
    imh->image_min = (int)(matrix->data_min/matrix->scale_factor);
    imh->image_max = (int)(matrix->data_max/matrix->scale_factor);
  }
  matrix->shptr = (void *)imh;
  if (matrix->data_type == ecat_matrix::MatrixDataType::VAX_Ix2)      /* old integer 2 image format */
    imh->data_type = matrix->data_type = ecat_matrix::MatrixDataType::SunShort;
  if (matrix->data_type == ecat_matrix::MatrixDataType::IeeeFloat)      /* Interfile float input */
    imh->data_type = matrix->data_type = ecat_matrix::MatrixDataType::SunShort;
  if (matrix->data_type != data_type) {
    LOG_ERROR("air2matrix : incompatible data types");
    free_matrix_data(matrix);
    return NULL;
  }
  imh->x_dimension = matrix->xdim = stats->x_dim;
  imh->x_dimension = matrix->ydim = stats->y_dim;
  imh->z_dimension = matrix->zdim = stats->z_dim;
  imh->x_pixel_size = matrix->pixel_size = (float)stats->x_size * 0.1f; /* in cm */
  imh->y_pixel_size = matrix->y_size     = (float)stats->y_size * 0.1f; /* in cm */
  imh->z_pixel_size = matrix->z_size     = (float)stats->z_size * 0.1f; /* in cm */
  nblks = (nvoxels*elem_size+511)/512;
  matrix->data_size = nblks*512;
  matrix->data_ptr = (void *)calloc(512,nblks);

  /* fill matrix data */
  image_data = pixels[0][0];
  switch(data_type) {
  case ecat_matrix::MatrixDataType::ByteData :
  case ecat_matrix::MatrixDataType::BitData :
    a = 255.0f / AIR_CONFIG_MAX_POSS_VALUE;
    unsigned char *bdata = (unsigned char *)matrix->data_ptr;
    for (i=0; i<nvoxels; i++)
      *bdata++ = (int)(0.5+a*(*image_data++));
    break;
  case ecat_matrix::MatrixDataType::SunShort:
    short *sdata = (short*)matrix->data_ptr;
    for (i=0; i<nvoxels; i++, sdata++) {
      v = *image_data++;
      if (v>32768)
        *sdata = (short)(v - 32768);
      else
        *sdata=0; // remove negatives
    }
    break;
  }
  imh->scale_factor = matrix->scale_factor;
  return matrix;
}

int air2ecat(AIR_Pixels ***pixels, struct AIR_Key_info *stats, const char *specs, int permission, const char *comment, const char *orig_specs) {
  ecat_matrix::Main_header mh;
  char orig_fname[FILENAME_MAX], *base, *ext;
  ecat_matrix::MatrixFile* file;
  FILE *fp=NULL, *fpi=NULL;
  ecat_matrix::MatrixData *orig=NULL, *matrix=NULL;
  ecat_matrix::MatrixData *slice=NULL;
  ecat_matrix::Image_subheader *imh = NULL;
  int sw_version,  i_matnum=0, o_matnum=0;
  int cubic=0, interpolate=0;
  int i=0, npixels=0, plane=0;
  char* ecat_version;

  matspec(orig_specs,orig_fname,&i_matnum);
  file = matrix_open(orig_fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (i_matnum == 0) {
    /* use first */
    if (file->dirlist->nmats) {
      i_matnum = file->dirlist->first->matnum;
    } else {
      LOG_ERROR("{} : no matrix found\n", orig_specs);
      return 0;
    }
  }
  orig = matrix_read(file,i_matnum,ecat_matrix::MatrixDataType::MAT_SUB_HEADER);
  if (orig != NULL && orig->zdim == 1) {
    /* slice mode */
    ecat_matrix::MatVal val;
    mat_numdoc(i_matnum, &val);
    free_matrix_data(orig);
    orig = matrix_read(file,val.frame, GENERIC);
  }
  if (orig == NULL) {
    LOG_ERROR("error reading matrix {}", orig_specs);
    return 0;
  }
  if (orig->data_type == ecat_matrix::MatrixDataType::IeeeFloat) {
    // load matrix and convert to short
    free_matrix_data(orig);
    orig = matrix_read(file,i_matnum, GENERIC);
  }
  if (!matspec(specs,fname,&o_matnum))
    o_matnum = i_matnum;
  if ((base = strrchr(fname,'/')) == NULL)
    base = fname;
  else
    base++;
  if ((ext = strrchr(base,'.')) == NULL)
    ext = base+strlen(base);

  memcpy(&mh,file->mhptr,sizeof(ecat_matrix::Main_header));
  matrix_close(file);
		
  if (!permission && access(fname,F_OK)==0) {
    LOG_ERROR("file {} exists, no permission to overwrite",fname);
    return 0;
  }
  matrix = air2matrix(pixels, stats,orig);
  if (matrix == NULL) {
    LOG_ERROR("Error converting AIR pixel data to ECAT matrix");
    return 0;
  }
  matrix_flip(matrix,0,1,1);   /* radiolgy convention */
  free_matrix_data(orig);
  if ((ecat_version=getenv("ECAT_VERSION")) != NULL) {
      LOG_DEBUG("current ecat version : {}" ,ecat_version);
    if (sscanf(ecat_version,"%d",&sw_version)==1 && sw_version>0)
      mh.sw_version = sw_version;
  }
    LOG_DEBUG("sw version %d\n",mh.sw_version);

  if (strlen(ext) == 0)
    strcpy(ext,".v"); // ECAT7 default output format
  if (strncmp(ext,".i",2) == 0)    {
    // write Interfile format
    float *fdata=NULL;
    short *sdata=NULL;
    npixels = matrix->xdim*matrix->ydim;
    sdata = (short*)matrix->data_ptr;
    if ((fp=fopen(fname,"wb"))==NULL) 
      LOG_ERROR("error opening file {}", fname);
    if ((fdata=(float*)calloc(npixels, sizeof(float)))==NULL) 
      LOG_EXIT("memory allocation error");
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
      if ((fp=fopen(hdr_fname,"wt")) != NULL)          {
        if ((fpi=fopen(orig_fname,"rt")) != 0) {
          while (fgets(line,sizeof(line),fpi) != NULL) {
            if (strstr(line,"name of data file :=") != NULL)
              fprintf(fp,"name of data file := %s\n", fname);
            else
              fprintf(fp,"%s", line);
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
    if (fp!=NULL && fdata!=NULL)         {
      fclose(fp);
      return 1;
    }
    return 0;
  }
  // else  write ECAT format
  mh.file_type = ecat_matrix::DataSetType::PetVolume;
  mh.num_planes = matrix->zdim;
  mh.plane_separation = matrix->z_size;
  if ((file=matrix_create(fname,ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, &mh)) == NULL) {
    LOG_ERROR(fname);
    free_matrix_data(matrix);
    return 0;
  }
  imh = (ecat_matrix::Image_subheader*)matrix->shptr;
  strncpy(imh->annotation,comment,sizeof(imh->annotation)-1);
  matrix_write(file,o_matnum,matrix);
  free_matrix_data(matrix);
  matrix_close(file);
  return 0;
}


float ecat_AIR_open_header(const char *mat_spec, struct AIR_Fptrs *fp, struct AIR_Key_info *stats, int *flag) {
  char fname[FILENAME_MAX];
  ecat_matrix::MatrixFile* file;
  ecat_matrix::MatrixData *hdr=NULL;
  int matnum=0;

  fp->errcode = 0;
  matspec(mat_spec,fname,&matnum);
  file = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (file==NULL) {
    LOG_ERROR( "{} : error opening file", fname); 
    fp->errcode = 1;
    return 0.0f;
  }
  if (matnum == 0) {		/* use first */
    if (file->dirlist->nmats) matnum = file->dirlist->first->matnum;
    else { 
      fp->errcode = 1;
      LOG_ERROR( "{} : no matrix found", mat_spec); 
    }
  }
  if (fp->errcode == 0)    {
      if ((hdr = matrix_read(file,matnum,ecat_matrix::MatrixDataType::MAT_SUB_HEADER)) == NULL) {
        LOG_ERROR( "{} : error opening matrix", mat_spec); 
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

void AIR_close_header(struct AIR_Fptrs *fp) {
  fp->errcode = 0;
}

AIR_Error ecat_AIR_load_probr(const char *specs, const AIR_Boolean decompressable_read) {
  char fname[FILENAME_MAX];
  ecat_matrix::MatrixFile* file;
  ecat_matrix::MatrixData  *hdr=NULL;
  int matnum=0;
  int error=0;

  matspec(specs,fname,&matnum);
  file = matrix_open(fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (file==NULL) return 0;
  if (matnum == 0) {		/* use first */
    if (file->dirlist->nmats) matnum = file->dirlist->first->matnum;
    else {
      LOG_ERROR("{} : no matrix found", specs);
      error++;
    }
  }
  if (file != NULL) {
    if ((hdr = matrix_read(file,matnum,ecat_matrix::MatrixDataType::MAT_SUB_HEADER))==NULL) error++;
    else free_matrix_data(hdr);
  } else matrix_close(file);
  return error;
}
