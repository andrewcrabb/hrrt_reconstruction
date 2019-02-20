//static char sccsid[] = "%W% UCL-TOPO %E%";
/*
 Author : Sibomana@topo.ucl.ac.be
 Creation date : 14-Nov-2000
 Modification history :
	04-Oct-2001 : Sibomana@topo.ucl.ac.be
					Follow Volume signature changes
  27-Apr-2010: Add to HRRT Users software (sibomana@gmail.com)
               Add -M option for affine transformer from polaris tracking
               Reverse Y before and after applying Polaris transformer due
               to left/right handed differences between polaris and HRRT scanner
*/
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "my_spdlog.hpp"

#include <unistd.h>

#include "matrix_resize.h"
#include "Volume.h"
#include <air_ecat/ecat2air.h>

#define host_data_to_file file_data_to_host
void loading_progress(float, float) {};
int debug_level = 0;

static void usage() {
  LOG_INFO("volume_reslice Build {} {}", __DATE__, __TIME__);
  LOG_ERROR(
	  "usage: volume_reslice -i in_matspec [-A | -C | -S] -o out_matspec \n"
    "       [-t transformer | -M affine_transformer | -a air_transformer] \n"
    "       [-x xdim[,dx]] [-y ydim[,dy]] [-z zdim[,dz]] -Z zoom_factor [-R] [-v]\n");
  LOG_ERROR("\n"
    "  Load volume specified by in_matspec, apply optional transformer and\n"
	  "  save volume in out_matspec in the requested orientation.\n"
	  "  Optionally, use requested volume dimensions and voxelsize.\n");
  LOG_ERROR("\n  Transformer formats:\n"
	  "    -t rz(yaw),rx(pitch),ry(roll),tx,ty,tz\n"
	  "    -M m11,m12,m13,m14,m21,m22,m23,m24,m31,m32,m33,m34\n");
  LOG_ERROR("\n  Orientation: -A(axial=default), -C(coronal), -S(sagittal)\n");
  LOG_ERROR("  Zoom  : applies a zoom factor within [1.0,10.0] range at volume center\n");
  LOG_ERROR("  -R Reverse transformer\n");
  LOG_ERROR("  -v set verbose mode on ( default is off)\n");
  exit(1);
}

static char line[FILENAME_MAX];
static char fname[FILENAME_MAX], hdr_fname[FILENAME_MAX];

static int save_interfile(ecat_matrix::MatrixData *matrix, const char *fname, const char *orig_fname) {
  FILE *fp=NULL, *fpi=NULL;
  float *fdata=NULL;
  short *sdata=NULL;
  int npixels = matrix->xdim*matrix->ydim;

  sdata = (short*)matrix->data_ptr;
  if ((fp=fopen(fname,"wb"))==NULL) 
    LOG_ERROR("error opening file {}", fname);
  if ((fdata=(float*)calloc(npixels, sizeof(float)))==NULL) 
    LOG_ERROR("memory allocation error");
  if (fp!=NULL && fdata!=NULL) {
    for (int plane=0; plane<matrix->zdim; plane++) {
      for (int i=0; i<npixels; i++)
        fdata[i] = sdata[i]*matrix->scale_factor;
      sdata += npixels;
      fwrite(fdata, sizeof(float),npixels,fp);
    }
    fclose(fp);
    sprintf(hdr_fname,"%s.hdr",fname);
    if ((fp=fopen(hdr_fname,"wt")) != NULL)
    {
      sprintf(hdr_fname,"%s.hdr",orig_fname);
      if ((fpi=fopen(hdr_fname,"rt")) != 0) {
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
        fprintf(fp, "scaling factor (mm/pixel) [1] := %g\n", 10*matrix->pixel_size);
        fprintf(fp, "scaling factor (mm/pixel) [2] := %g\n", 10*matrix->pixel_size);
        fprintf(fp, "scaling factor (mm/pixel) [3] := %g\n", 10*matrix->z_size);
      }
    }
    fclose(fp);
    return 1;
  }
  return 0;
}

ecat_matrix::MatrixData *volume_resize(Volume *volume, int xydim, int nplanes) {
  int interpolate=1;
  ecat_matrix::MatrixData *slice = NULL;
  const ecat_matrix::MatrixData *data = volume->data();
  float xdim = data->xdim*data->pixel_size, zdim=data->zdim*data->z_size;
  float pixel_size = data->xdim*data->pixel_size/xydim;
  float z_size = data->zdim*data->z_size/nplanes;
  VoxelCoord area(xdim,xdim,zdim);
  VoxelCoord zoom_center(0.5f*xdim, 0.5f*xdim, 0.5f*zdim);
  ecat_matrix::MatrixData *cdata = (ecat_matrix::MatrixData*)calloc(1,sizeof(ecat_matrix::MatrixData));
  memcpy(cdata,data,sizeof(ecat_matrix::MatrixData));
  cdata->xdim = cdata->ydim = xydim;
  cdata->zdim = nplanes;
  cdata->data_size = cdata->xdim*cdata->ydim*cdata->zdim;
  if (cdata->data_type == ecat_matrix::MatrixDataType::IeeeFloat) 
    cdata->data_size *= sizeof(float);
  else 
    cdata->data_size *= sizeof(float);
  int nblks = (cdata->data_size + ecat_matrix::MatBLKSIZE - 1)/ecat_matrix::MatBLKSIZE;
  cdata->data_ptr = (void *) calloc(nblks, ecat_matrix::MatBLKSIZE);
  ecat_matrix::Image_subheader *imh = (ecat_matrix::Image_subheader*)calloc(1, ecat_matrix::MatBLKSIZE);
  memcpy(imh, data->shptr, sizeof(ecat_matrix::Image_subheader));
  cdata->shptr = (void *)imh;
  imh->x_dimension = cdata->xdim;
  imh->y_dimension = cdata->ydim;
  imh->z_dimension = cdata->zdim;
  cdata->pixel_size = cdata->y_size = pixel_size;
  cdata->z_size = z_size;
  imh->x_pixel_size = imh->y_pixel_size = pixel_size;
  imh->z_pixel_size = z_size;
  for (int plane=0; plane<cdata->zdim; plane++) {
    slice = volume->get_slice(zoom_center, 1.0, Dimension_Z, plane*z_size, area, z_size, interpolate);
    memcpy( cdata->data_ptr + plane*slice->data_size, slice->data_ptr, slice->data_size);
    free_matrix_data(slice);
  }
  return cdata;
}

inline int mat_decode(const char *s, float *f) {
  return  sscanf(s,"%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
    &f[0],&f[1],&f[2],&f[3],&f[4],&f[5],&f[6],&f[7],&f[8],&f[9],&f[10],&f[11]);
}

int main(int argc, char **argv) {
  ecat_matrix::MatrixFile *mptr, *mptr1;
  ecat_matrix::Main_header *proto;
  ecat_matrix::MatrixData *slice;
  ecat_matrix::Image_subheader *imh=NULL;
  float xdim=0, ydim=0, zdim=0, dx=1, dy=1, dz=1, pixel_size=1;
  float tx=0, ty=0, tz=0, rx=0, ry=0, rz=0;
  Matrix tm=NULL; // affine transformer matrix
  char *in_file=NULL, *out_file=NULL, *ext=NULL;
  char *air_transformer=NULL;
  char *x_area_spec=NULL, *y_area_spec=NULL, *z_area_spec=NULL;
  int transformer = 0;
  int c, verbose = 0, in_matnum=0, out_matnum=0;
  int volume_scale=1, interpolate=1;
  int volume_blks, slice_blks;
  DimensionName orientation = Dimension_Z;
  float z=0.0, zoom_factor=1.0; 
  int err_flag=0;
  int y_reverse=0;  // left to right handed coordinates transformation
  int i,j, plane;
  float c_size=1.0f;
  int t_reverse=0;
  int t_flip=0;  // AIR (radiology) convention

  while ((c = getopt (argc, argv, "i:o:x:y:z:t:M:a:Z:ACSRrv")) != EOF) {
    switch (c) {
    case 'i' :
      in_file = strdup(optarg);
      matspec(optarg, in_file, &in_matnum);
      break;
    case 'o' :
      out_file = strdup(optarg);
      matspec(optarg, out_file, &out_matnum);
      break;
    case 'a' :
      air_transformer = strdup(optarg);
      t_flip = 1;
      break;
    case 't' :
      if (sscanf(optarg,"%g,%g,%g,%g,%g,%g", &rz, &rx, &ry, &tx, &ty, &tz) > 0)
        transformer = 1;
      break;
    case 'M' :
      tm = mat_alloc(4,4);
      if (mat_decode(optarg, tm->data) == 12) {
        transformer = 1;
        //mat_print(tm);
        y_reverse = 1; // transformer from Polaris Vicra tracking left handed coord
      } else {
        LOG_ERROR( "Invalid -M argument %s\n", optarg);
        err_flag++;
      }
      break;
    case 'x' :
      x_area_spec = optarg;
      break;
    case 'y' :
      y_area_spec = optarg;
      break;
    case 'z' :
      z_area_spec = optarg;
      break;
    case 'Z' :
      if (sscanf(optarg,"%g", &zoom_factor) != 1) {
        LOG_ERROR( "invalid argument -Z %s\n", optarg);
        err_flag++;
      }
      if (zoom_factor<1.0 || zoom_factor > 10.0) {
        LOG_ERROR( "zoom factor (%g) not in [1.0,10.0] range\n", zoom_factor);
        err_flag++;
      }
      break;
    case 'C' :
      orientation = Dimension_Y;
      break;
    case 'S' :
      orientation = Dimension_X;
      break;
    case 'v' :
      verbose = 1;
    case 'R' :
      t_reverse = 1;
      break;
    case 'r' :
      t_flip = 1;
      break;
    }
  }
  if (err_flag) usage();
 // if (verbose) printf("volume_reslice : build date %s\n", build_date);
  if (in_file == NULL) usage();

  // Open input file
  mptr = matrix_open( in_file, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (!mptr) LOG_EXIT( "can't open file '%s'\n", in_file);
  if ( mptr->dirlist->nmats == 0) LOG_EXIT("no matrix in %s\n",in_file);
  if (in_matnum==0) in_matnum = mptr->dirlist->first->matnum;
    LOG_DEBUG("loading volume");

  // Create Volume Object and load data
#ifdef ORIG_CODE
   Volume *volume = new Volume(mptr, in_matnum);
  const ecat_matrix::MatrixData* data = volume->data();
#else
  ecat_matrix::MatrixData* data = matrix_read(mptr,in_matnum, GENERIC);
  if (data == NULL) LOG_EXIT("Error reading %s\n", in_file);
  if (t_flip)  matrix_flip(data,0,1,1); 		/* AIR radiology convention */
  else  if (y_reverse) matrix_flip(data,0,1,0); /* Vicra */

  LOG_INFO("Volume {} x {} x {} loaded\n", data->xdim, data->ydim, data->zdim);
  data->pixel_size *= 10.0f; // cm to mm
  data->y_size *= 10.0f; // cm to mm
  data->z_size *= 10.0f; // cm to mm
  Volume *volume = new Volume(mptr->mhptr, data);
#endif
  xdim = (float)data->xdim; // #pixels
  ydim = (float)data->ydim;
  zdim = (float)data->zdim; 
  dx = data->pixel_size;
  dy = data->y_size;
  dz = data->z_size;
  
  // specify output image dimension and spacing
  if (x_area_spec) sscanf(x_area_spec,"%g,%g", &xdim, &dx);
  if (y_area_spec) sscanf(y_area_spec,"%g,%g", &ydim, &dy);
  if (z_area_spec) sscanf(z_area_spec,"%g,%g", &zdim, &dz);
  xdim *= dx; ydim *= dy; zdim *= dz;					// #output image size in mm
  VoxelCoord area(xdim,ydim,zdim);

  VoxelCoord zoom_center(0.5f*xdim, 0.5f*ydim, 0.5f*zdim);

  if (transformer)
  {
    if (tm==NULL)  // 6 parameters
    { 
      // Center the image in the output dimension
      tx += 0.5f*(data->xdim*data->pixel_size - xdim);
      ty += 0.5f*(data->ydim*data->y_size - ydim);
      tz += 0.5f*(data->zdim*data->z_size - zdim);
      if (fabs(tx) + fabs(ty) + fabs(tz) >= 1) transformer = 1;
      volume->transform(rx, ry, rz, tx, ty, tz);
    }
    else  // affine matrix
    {  
      if (t_reverse) {
        // mat_print(tm);
        Matrix tcopy = mat_alloc(4,4);
        mat_copy(tcopy, tm);
        mat_invert(tm,tcopy);
        // mat_print(tm);
        mat_free(tcopy);
      }
      c_size = data->pixel_size>data->z_size? data->z_size:data->pixel_size;
      //scale(tm, c_size/data->pixel_size, c_size/data->y_size, c_size/data->z_size);
      // Convert translations to pixels
      if (c_size < data->pixel_size || c_size<data->z_size)
      { // create cubic volume
        ecat_matrix::MatrixData *cdata = (ecat_matrix::MatrixData*)calloc(1,sizeof(ecat_matrix::MatrixData));
        memcpy(cdata,data,sizeof(ecat_matrix::MatrixData));
        cdata->xdim = (int)(xdim/c_size);
        cdata->ydim = (int)(ydim/c_size);
        cdata->zdim = (int)(zdim/c_size);
        int size_factor = (cdata->xdim*cdata->ydim*cdata->zdim)/(data->xdim*data->ydim*data->zdim);
        cdata->data_size = data->data_size * size_factor;
        cdata->data_ptr = (void *) calloc(cdata->data_size, 1);
        imh = (ecat_matrix::Image_subheader*)calloc(1, ecat_matrix::MatBLKSIZE);
        memcpy(imh, data->shptr, sizeof(ecat_matrix::Image_subheader));
        cdata->shptr = (void *)imh;
        imh->x_dimension = cdata->xdim;
        imh->y_dimension = cdata->ydim;
        imh->z_dimension = cdata->zdim;
        cdata->pixel_size = cdata->y_size = cdata->z_size = c_size;
        imh->x_pixel_size = imh->y_pixel_size = imh->z_pixel_size = c_size;
        for (plane=0; plane<cdata->zdim; plane++)
        {
          slice = volume->get_slice(zoom_center, zoom_factor, Dimension_Z, plane*c_size,
								area, c_size, interpolate);
          memcpy( cdata->data_ptr + plane*slice->data_size, slice->data_ptr, slice->data_size);
        }
        data->shptr = NULL;
        delete volume; 
        data = cdata;
        volume = new Volume(mptr->mhptr, data);
      }

      tm->data[3] /= c_size; tm->data[7] /= c_size; tm->data[11] /= c_size;
      volume->transform(tm);
    }
  }
  else if (air_transformer)
  {
    struct AIR_Air16 air_16;
    tm = mat_alloc(4,4);
    AIR_read_air16(air_transformer, &air_16);
    for (j=0;j<tm->nrows;j++)
      for (i=0;i<tm->ncols;i++)
        tm->data[i+j*tm->ncols] = (float)air_16.e[i][j];
    if ( air_16.r.x_dim != volume->data()->xdim)
    {
      ecat_matrix::MatrixData *cdata = volume_resize(volume, air_16.r.x_dim, air_16.r.z_dim);
      data->shptr = NULL;
      delete volume; 
      data = cdata;
      volume = new Volume(mptr->mhptr, data);
    }
    volume->transform(tm);
  }
  
  //if (verbose) 
    mat_print(volume->transformer());
  switch(orientation)
  {
  case Dimension_Z:
    pixel_size = dx<dy? dx:dy;
    break;
  case Dimension_Y:
    pixel_size = dx<dz? dx:dz;
    zdim = ydim;
    dz = dy;
    break;
  case Dimension_X:
    pixel_size = dy<dz? dy:dz;
    zdim = xdim;
    dz = dx;
    break;
  }
  int num_planes = (int)(zdim/dz);

/* get first slice for dimensions */
  if (out_matnum == 0) out_matnum = data->matnum;

  slice = volume->get_slice(zoom_center, zoom_factor, orientation, 0,
								area, pixel_size, interpolate);
  slice_blks = (slice->data_size+ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;

  volume_blks = (slice->data_size*num_planes+ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
  imh = (ecat_matrix::Image_subheader*)calloc(1, ecat_matrix::MatBLKSIZE);
  ecat_matrix::MatrixData *out_data = (ecat_matrix::MatrixData*) calloc(1, sizeof(ecat_matrix::MatrixData));
  memcpy(out_data,slice,sizeof(ecat_matrix::MatrixData));
  out_data->shptr = (void *)imh;
  out_data->dicom_header = NULL;
  out_data->dicom_header_size = 0;
  out_data->data_ptr = (void *)calloc(volume_blks, ecat_matrix::MatBLKSIZE);
  out_data->data_size = volume_blks*ecat_matrix::MatBLKSIZE;
  out_data->pixel_size /= 10.0f;
  out_data->y_size /= 10.0f;
  out_data->z_size /= 10.0f;
  memcpy(imh, data->shptr, sizeof(ecat_matrix::Image_subheader));
  imh->x_pixel_size = imh->y_pixel_size = pixel_size/10; // mm -> cm
  imh->z_pixel_size = dz/10; // mm ->cm
  imh->num_dimensions = 3;
  imh->x_dimension = slice->xdim;
  imh->y_dimension = slice->ydim;
  out_data->zdim = imh->z_dimension = num_planes;
  int plane_size = slice->data_size;
  memcpy(out_data->data_ptr, slice->data_ptr, slice->data_size);
  void * dest = out_data->data_ptr + plane_size;
  for (int plane=1; plane<num_planes; plane++, dest+=plane_size)
  {
    slice = volume->get_slice(zoom_center, zoom_factor, orientation, plane*dz,
      area, pixel_size, interpolate);
    memcpy(dest, slice->data_ptr, plane_size);
    free_matrix_data(slice);

  }
  if (t_flip)  matrix_flip(out_data,0,1,1); 		/* AIR radiology convention */
  else  if (y_reverse) matrix_flip(out_data,0,1,0); /* Vicra */

  if ((ext = strrchr(out_file,'.')) != NULL && strcmp(ext,".v") == 0)  {
    //Format Code to create output in ECAT format
    proto = (ecat_matrix::Main_header*)calloc(1, ecat_matrix::MatBLKSIZE);
    memcpy(proto, mptr->mhptr, sizeof(ecat_matrix::Main_header));
    proto->sw_version = V7;
    proto->file_type = ecat_matrix::DataSetType::PetVolume;
    proto->num_planes = num_planes;
    proto->plane_separation = dz/10;  // mm -> cm
    mptr1 = matrix_create(out_file, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, proto);
    if (mptr1 == NULL) { matrix_perror(out_file); exit(1); }
    matrix_write(mptr1,out_matnum,out_data);
  }  else  {
    save_interfile(out_data, out_file, in_file);
  }
  delete volume;
  free_matrix_data(out_data);
  LOG_DEBUG("done");
  return 0;
}
