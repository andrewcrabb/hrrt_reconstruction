/* @(#)matcopy.c  1.4 7/10/92 */
#ifndef lint
static char sccsid[]="(#)matcopy.c 1.4 7/10/92 Copyright 1990 CTI Pet Systems, Inc.";
#endif

/* 09-Nov-1995 : modified by sibomana@topo.ucl.ac.be */

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ecat_matrix.hpp"
#include "my_spdlog.hpp"
extern ecat_matrix::MatrixData *matrix_read_scan();

static void usage();
static int copy_scan(ecat_matrix::MatrixFile *mptr1, int matnum, ecat_matrix::MatrixFile *mptr2, int o_matnum, int storage_order);

static void usage() {
  LOG_ERROR(
    "usage: matcopy -i matspec -o matspec [-V version -v] [-s storage_order]]\n");
  LOG_ERROR("version is either 70 or 6 (default = 70)\n");
  LOG_ERROR("-s storage_order (0 or 1); valid only for sinograms and attenuations\n");
  LOG_ERROR("-v set verbose mode on ( default is off)\n");
  exit(1);
}

static int verbose=0;

static int copy_scan(ecat_matrix::MatrixFile *mptr1, int matnum, ecat_matrix::MatrixFile *mptr2, int o_matnum, int storage_order) {
  ecat_matrix::MatrixData *matrix;
  ecat_matrix::MatDir matdir, o_matdir;
  ecat_matrix::Scan3D_subheader *sh=NULL, *o_sh=NULL;
  ecat_matrix::Attn_subheader  *ah=NULL, *o_ah=NULL;
  void *blk, *sino, *planar, *dest;
  int keep_order = 1;
  int i, view, plane;
  int blkno, file_pos, view_pos;
  int nblks, line_size, num_views, num_planes;

  if (matrix_find(mptr1, matnum, &matdir) == -1) return 0;
  nblks = matdir.endblk-matdir.strtblk+1;
  if (matrix_find(mptr2, o_matnum, &o_matdir) == -1)  {
    blkno = mat_enter(mptr2->fptr, mptr2->mhptr, o_matnum, nblks) ;
    o_matdir.matnum = o_matnum;
    o_matdir.strtblk = blkno;
    o_matdir.endblk =  blkno + nblks - 1 ;
    insert_mdir(&o_matdir, mptr2->dirlist) ;
  }
  matrix = matrix_read(mptr1,matnum,MatrixData::DataType::MAT_SUB_HEADER);
  blk = (void *)malloc(MatBLKSIZE);
  switch (mptr1->mhptr->file_type) {
  case MatrixData::DataSetType::Float3dSinogram :
  case MatrixData::DataSetType::Short3dSinogram :
    sh = (ecat_matrix::Scan3D_subheader*)matrix->shptr;
    o_sh = (ecat_matrix::Scan3D_subheader*)calloc(2,MatBLKSIZE);
    memcpy(o_sh,sh,sizeof(ecat_matrix::Scan3D_subheader));
    if (storage_order>=0) o_sh->storage_order = storage_order;
    if (o_sh->storage_order != sh->storage_order) keep_order = 0;
    if (mptr1->mhptr->file_type == Float3dSinogram)
      line_size = sh->num_r_elements*sizeof(float);
    else line_size = sh->num_r_elements*sizeof(short);
    num_views = sh->num_angles;
    num_planes = sh->num_z_elements[0];
    mat_write_Scan3D_subheader(mptr2->fptr,mptr2->mhptr, o_matdir.strtblk,
      o_sh);
    nblks -= 2;
    file_pos = (o_matdir.strtblk+1)*MatBLKSIZE;
    break;
  case MatrixData::DataSetType::AttenCor :
    ah = (ecat_matrix::Attn_subheader*)matrix->shptr;
    o_ah = (ecat_matrix::Attn_subheader*)calloc(1,MatBLKSIZE);
    memcpy(o_ah,ah,sizeof(ecat_matrix::Attn_subheader));
    if (storage_order>=0) o_ah->storage_order = storage_order;
    if (o_ah->storage_order != ah->storage_order) keep_order = 0;
    line_size = ah->num_r_elements*sizeof(float);
    num_views = ah->num_angles;
    num_planes = ah->z_elements[0];
    mat_write_attn_subheader(mptr2->fptr,mptr2->mhptr, o_matdir.strtblk,
      o_ah);
    nblks -= 1;
    file_pos = o_matdir.strtblk*MatBLKSIZE;
    break;
  default:
    return 0;
  }
  if (keep_order) {
    for (i=0; i<nblks;i++) {
      if (fread(blk,MatBLKSIZE,1,mptr1->fptr) != 1) {
        LOG_EXIT(mptr1->fname);
      }
      if (fwrite(blk,MatBLKSIZE,1,mptr2->fptr) != 1) {
          LOG_EXIT(mptr2->fname);
        }
    }
  }  else { /* keep_order */
    if (storage_order == 1) {   /* view mode to sino mode */
      LOG_DEBUG("view mode to sino mode");
      sino = (void *)malloc(line_size*num_views);
      file_pos = ftell(mptr1->fptr);
      for (plane=0;plane<num_planes;plane++) {
        dest = sino;
        for (view=0; view<num_views; view++) {
          view_pos = file_pos + view*num_planes*line_size +
            plane*line_size;
          if ((fseek(mptr1->fptr,view_pos,0) == -1) ||
            fread(dest,line_size,1,mptr1->fptr) != 1) {
            LOG_EXIT(mptr1->fname);
          }
          dest += line_size;
        }
        if (fwrite(sino,line_size,num_views,mptr2->fptr) != num_views) {
          LOG_EXIT(mptr2->fname);
        }
      }
    } else {        /* storage_order */
      LOG_DEBUG("sino mode to view mode\n");
      planar = (void *)malloc(line_size*num_planes);
      file_pos = ftell(mptr1->fptr);
      for (view=0; view<num_views; view++) {
        dest = planar;
        for (plane=0;plane<num_planes;plane++) {
        view_pos = file_pos + plane*num_views*line_size + view*line_size;
          if ((fseek(mptr1->fptr,view_pos,0) == -1) ||
            fread(dest,1,line_size,mptr1->fptr) != line_size) {
            LOG_EXIT(mptr1->fname);
          }
          dest += line_size;
        }
        if (fwrite(planar,line_size,num_planes,mptr2->fptr)!=num_planes) {
          LOG_EXIT(mptr2->fname);
        }
      }
    } /* storage_order */
  } /* keep_order */
  if (mptr2->mhptr->sw_version == V7) mh_update(mptr2);
  return 1;
}
  
int main( argc, argv)
  int argc;
  char **argv;
{
  ecat_matrix::MatrixFile *mptr1, *mptr2;
  ecat_matrix::MatrixData *matrix, *slice;
  ecat_matrix::Main_header proto;
  ecat_matrix::Image_subheader* imagesub;
  ecat_matrix::MatDirNode *node=NULL;
  char *mk, fname[256];
  int i, j, specs[5];
  char *in_spec=NULL, *out_spec=NULL;
  int c, version=V7, matnum=0, o_matnum=0;
  int plane,  npixels, slice_blks, slice_matnum;
  int elem_size=2, offset = 0;
  int storage_order = -1;
  short *sdata;
  unsigned char  *bdata;
  int *matnums=NULL, nmats=0;
  ecat_matrix::MatVal mat;
  extern char *optarg;

  while ((c = getopt (argc, argv, "i:o:V:s:v")) != EOF) {
    switch (c) {
    case 'i' :
      in_spec = optarg;
            break;
    case 'o' :
      out_spec  = optarg;
            break;
    case 'V' :
      sscanf(optarg,"%d",&version);
            break;
    case 's' :
      if (sscanf(optarg,"%d",&storage_order) != 1 ||
      (storage_order!=0 && storage_order!=1)) usage();
            break;
    case 'v' :
      verbose = 1;
      break;
    }
  }
  
  if (in_spec == NULL || out_spec==NULL) usage();
  for (i=0; i<5; i++) specs[i] = 0;
  strcpy(fname,strtok(in_spec,","));
  mk = strtok(NULL,",");
  for (i=0; i<5; i++) {
    if (mk!=NULL) {
      if (*mk == '*') specs[i] = -1;
      else specs[i] = atoi(mk);
      mk = strtok(NULL,",");
    }
  }
  mptr1 = matrix_open( fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
  if (!mptr1) 
    LOG_EXIT( "can't open file {}", argv[0], fname);
  if ( mptr1->dirlist->nmats == 0) 
    LOG_EXIT("no matrix in {}",fname);
  matnums = (int*)calloc(sizeof(int),mptr1->dirlist->nmats);
  node = mptr1->dirlist->first;
  nmats = 0;
  if (specs[0] == 0) {  /* no matrix specified, use all matrices */ 
    while(node != NULL) {
      matnums[nmats++] = node->matnum;
      node = node->next;
    }
  } else {        /* build specified matnums */
    while(node != NULL) {
      matnum = node->matnum;
      node = node->next;
      mat_numdoc(matnum, &mat);
      if (specs[0]>=0 && specs[0] != mat.frame) continue;
      if (specs[1]>=0 && specs[1] != mat.plane) continue;
      if (specs[2]>=0 && specs[2] != mat.gate) continue;
      if (specs[3]>=0 && specs[3] != mat.data) continue;
      if (specs[4]>=0 && specs[4] != mat.bed) continue;
      matnums[nmats++] = matnum;
    }
  }
  if (nmats == 0) 
    LOG_EXIT( "{}: matrix not found{}", in_spec);
  matspec( out_spec, fname, &o_matnum);
  memcpy(&proto,mptr1->mhptr,sizeof(ecat_matrix::Main_header));
  proto.sw_version = version;
  if (version < V7) {
    if (proto.file_type != MatrixData::DataSetType::PetImage && proto.file_type != MatrixData::DataSetType::ByteVolume &&
    proto.file_type != MatrixData::DataSetType::PetVolume && proto.file_type != MatrixData::DataSetType::ByteImage &&
    proto.file_type != InterfileImage)
      LOG_EXIT ("version 6 : only images are supported");
    proto.file_type = MatrixData::DataSetType::PetImage;
  } else {
    if (proto.file_type == InterfileImage) {
      matrix = matrix_read( mptr1, matnums[0], MatrixData::DataType::MAT_SUB_HEADER);
      if (matrix->data_type == MatrixData::DataSetType::ByteData) 
        proto.file_type = MatrixData::DataSetType::ByteVolume;
      else 
        proto.file_type = MatrixData::DataSetType::PetVolume;
      free_matrix_data(matrix);
    }
  }
  if (proto.sw_version != mptr1->mhptr->sw_version) {
    LOG_DEBUG("converting version %d to version %d\n",
        mptr1->mhptr->sw_version, proto.sw_version); 
  } else {
    LOG_DEBUG("input/output version : %d\n",proto.sw_version);
  }
  mptr2 = matrix_create( fname, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, &proto);
  if (!mptr2) LOG_EXIT( "%s: can't open file '%s'\n", argv[0], fname);
  
  for (i=0; i<nmats; i++) {
    if (nmats > 1 || o_matnum == 0) o_matnum = matnums[i];
    if (verbose) {
          mat_numdoc(matnums[i], &mat);
      LOG_ERROR("input matrix : %s,%d,%d,%d,%d,%d\n",
        mptr1->fname, mat.frame,mat.plane,mat.gate,mat.data,mat.bed);
          mat_numdoc(o_matnum, &mat);
      LOG_ERROR("output matrix : %s,%d,%d,%d,%d,%d\n",
        mptr2->fname, mat.frame,mat.plane,mat.gate,mat.data,mat.bed);
    }
    if (mptr1->mhptr->file_type==Short3dSinogram ||
      mptr1->mhptr->file_type==Float3dSinogram ||
      mptr1->mhptr->file_type==AttenCor)
      copy_scan( mptr1,matnums[i], mptr2,o_matnum,storage_order);
    else {
      matrix = matrix_read( mptr1,matnums[i], GENERIC);
      if (matrix != NULL) matrix_write( mptr2, o_matnum, matrix);
    }
  }
  matrix_close( mptr1);
  matrix_close( mptr2);
  return(0);
}
