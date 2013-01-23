/*
 Author : Sibomana@gmail.com - HRRT Users community
 Creation date :01-Dec-2009
 Modification history :
 14/jun/2010: Add distance map image
*/

#include <stdio.h>
#include <stdlib.h>
#include <AIR/AIR.h>
#include <ecatx/matrix.h>
#include <ecatx/matpkg.h>
#ifdef WIN32
#include <ecatx/getopt.h>
#else
#include <unistd.h>
#endif
#define MIN_MU_VALUE 0.035f
#define MAX_MU_VALUE 0.5f

static float vicra2hrrt[16] = {
  -0.0243298f, -0.999692f,    0.00485205f, 159.682f,
  -0.999644f,   0.0242749f,  -0.0110728f,  276.756f,
   0.0109516f,	-0.00511973f, -0.999927f,  -142.914f, 
   0.0f, 		     0.0f,      		  0.0f,         1.0f};

inline int mat_decode(const char *s, float *f)
{
  return sscanf(s,"%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g",
    &f[0],&f[1],&f[2],&f[3],&f[4],&f[5],&f[6],&f[7],&f[8],&f[9],&f[10],&f[11]);
}

static void usage(const char *pgm)
{
  fprintf(stderr, "%s Build %s %s \n", pgm, __DATE__, __TIME__);
  fprintf(stderr,
    "usage: %s [-t transf | -a air | -M affine] [-i mu-map -o distance_map]\n", pgm);
  fprintf(stderr,
    "      -t Transformer as rotation and translation: rz(yaw),rx(pitch),ry(roll),tx,ty,tz\n"
    "      -a Transformer as AIR 5.X file\n" 
    "      -M m11,m12,m13,m14,m21,m22,m23,m24,m31,m32,m33,m34 affine transformer:\n" 
    "            assumes Polaris transformer left handed coordinates\n");

//    "      -q Polaris transformer as quaternion and translation: qw,qx,qy,qz,tx,ty,tz\n"
//    "      -r Reference polaris transformer quaternion and translation: qw,qx,qy,qz,tx,ty,tz\n"
//    "      -A polaris to scanner coordinates alignement\n");
  fprintf(stderr,"-v set verbose mode on ( default is off)\n");
  exit(1);
}

static void q2m(float *q, float *t, float *m)
{  
  /*
    matrix = [(qw*qw+qx*qx-qy*qy-qz*qz) 2*(qx*qy-qw*qz)    2*(qx*qz+qw*qy) Tx 
           2*(qy*qx+qw*qz)    (qw*qw-qx*qx+qy*qy-qz*qz) 2*(qy*qz-qw*qx) Ty 
           2*(qz*qx-qw*qy)    2*(qz*qy+qw*qx) (qw*qw-qx*qx-qy*qy+qz*qz) Tz 
             0                     0                    0               1];
 */
  m[0]=(q[0]*q[0]+q[1]*q[1]-q[2]*q[2]-q[3]*q[3]); m[1]= 2*(q[1]*q[2]-q[0]*q[3]); m[2]=2*(q[1]*q[3]+q[0]*q[2]);
  m[4]=2*(q[2]*q[1]+q[0]*q[3]); m[5]=(q[0]*q[0]-q[1]*q[1]+q[2]*q[2]-q[3]*q[3]); m[6]=2*(q[2]*q[3]-q[0]*q[1]);
  m[8]=2*(q[3]*q[1]-q[0]*q[2]); m[9]= 2*(q[3]*q[2]+q[0]*q[1]); m[10]=(q[0]*q[0]-q[1]*q[1]-q[2]*q[2]+q[3]*q[3]);
  m[12]=m[13]=m[14]=0.0f;
  m[3]=t[0]; m[7]=t[1]; m[11]=t[2]; m[15]=1.0;
}

static char line[1024];

static void matrix_float(MatrixData *matrix)
{
	float scalef, *fdata;
	short *sdata;
  int i, np = matrix->xdim*matrix->ydim*matrix->zdim;

	matrix->data_type = IeeeFloat;
	fdata = (float*)calloc(np,sizeof(float));
	sdata = (short*)matrix->data_ptr;
	scalef = matrix->scale_factor;
	matrix->data_ptr = (caddr_t)fdata;
	for (i=0; i<np;i++) fdata[i] = scalef * sdata[i];
	matrix->scale_factor = 1.0;
	free(sdata);
}

void  main(int argc, char **argv)
{
  int c=0, transformer=0, verbose=0;
  float tx=0, ty=0, tz=0, rx=0, ry=0, rz=0;
  float qr[4], q[4], qrt[3], qt[3];
  float x1[4], x2[4];
  int xdim=128, ydim=128,zdim=207;
  float pixel_size=2.4375f, z_pixel_size=1.21875f;
  float c_size=1.0f;   // isotropic voxel size used in AIR transformer
  char *air_file = NULL, *minc_file=NULL;
  struct AIR_Air16 air_16;
  char reslice_file[FILENAME_MAX];
  MatrixFile *mptr=NULL;
  MatrixData *matrix=NULL;
  Image_subheader *imh=NULL;
  int matnum=0, frame_start_time=0;
  int i,j,x,y,yrev,z,err_flag=0;
  FILE *fp=NULL;
  Matrix tm=NULL;
  const char *in_file=NULL;  // input mu-map 
  const char *out_file=NULL;  // output distance-map file
  int y_reverse = 0, t_reverse=0;
  memset(reslice_file,0,sizeof(reslice_file));  // initialize to blank
  while ((c = getopt (argc, argv, "i:o:A:t:a:r:q:M:m:Rv")) != EOF) {
    switch (c) {
      case 'i':
        in_file = optarg;
        break;
      case 'o':
        out_file = optarg;
        break;
      case 'a':
        air_file = optarg;
        break;
      case 'm':
        minc_file = optarg;
        break;
      case 't' :
        if (sscanf(optarg,"%g,%g,%g,%g,%g,%g",&rz,&rx,&ry,&tx,&ty,&tz) > 0)
          transformer = 1;
        break;
      case 'q' :
        if (sscanf(optarg,"%g,%g,%g,%g,%g,%g,%g",&q[0],&q[1],&q[2],&q[3],&qt[0],&qt[1],&qt[2]) > 0)
          transformer += 2;
        break;
      case 'r' :
        if (sscanf(optarg,"%g,%g,%g,%g,%g,%g,%g",&qr[0],&qr[1],&qr[2],&qr[3],&qrt[0],&qrt[1],&qrt[2]) > 0)
          transformer += 1;
        break;
     case 'M' :
      tm = mat_alloc(4,4);
      if (mat_decode(optarg, tm->data) == 12) {
        transformer = 1;
        //mat_print(tm);
        y_reverse = 1;
      } else {
        fprintf(stderr, "Invalid -M argument %s\n", optarg);
        err_flag++;
      }
      break;
     case 'R' :
        t_reverse = 1;
     case 'v' :
        verbose = 1;
      break;
    }
  }

  if (!transformer && !air_file && !tm) usage(argv[0]);
  Matrix t = mat_alloc(4,4);
                          // apply Transformer w.r.t AIR convention
  float cx = 0.5f*xdim*pixel_size;
  float cy = 0.5f*ydim*pixel_size;
  float cz = 0.5f*zdim*z_pixel_size;

  x1[0]=cx;  // center
  x1[1]=cy-100.0f; // 10 cm from the center 
  x1[2]= cz; // center
  x1[3] = 1.0f;
  c_size = (pixel_size>z_pixel_size)? z_pixel_size : pixel_size;

  if (air_file)
  {
    AIR_read_air16(air_file, &air_16);
    matspec(air_16.r_file, reslice_file, &matnum);
    if ((mptr=matrix_open(reslice_file,MAT_READ_ONLY, MAT_UNKNOWN_FTYPE)) != NULL)
    {
      if ((matrix=matrix_read( mptr, matnum, MAT_SUB_HEADER)) != NULL)
      {
        imh = (Image_subheader*)matrix->shptr;
        frame_start_time = imh->frame_start_time/1000;
        free_matrix_data(matrix);
      }
      matrix_close(mptr);
    }
    for (j=0;j<t->nrows;j++)
      for (i=0;i<t->ncols;i++)
        t->data[i+j*t->ncols] = (float)air_16.e[i][j];
    pixel_size = (float)air_16.r.x_size;
    z_pixel_size = (float)air_16.r.z_size;
    c_size = (pixel_size>z_pixel_size)? z_pixel_size : pixel_size;
	  scale(t, pixel_size/c_size, pixel_size/c_size, z_pixel_size/c_size);
  }
  else if (transformer==1)
  { // assume mm3 voxels
    c_size = 1.0f;
	  translate(t,-cx,-cy,-cz);
	  rotate(t,0,0,rz);        // apply yaw (+)
	  rotate(t,-rx,0,0);       // apply pitch (-)
	  rotate(t,0,-ry,0);       // apply roll (-)
	  translate(t,cx,cy,cz);
	  translate(t,-tx,ty,tz);	 // reverse x
  }
  /*
  Old Vicra code
  else if (tm != NULL)
  {
    Matrix ref = mat_alloc(4,4);
    Matrix v2s = mat_alloc(4,4); // vicra 2 scanner coordinates alignment
    Matrix v2s_inv = mat_alloc(4,4);
    Matrix tv = mat_alloc(4,4);   // current transformer
    Matrix tv_inv = mat_alloc(4,4); // vicra 2 scanner coordinates alignment
    q2m(qr, qrt, ref->data);
    q2m(q, qt, tv->data);
    mat_invert(tv_inv, tv);
    memcpy(v2s->data, vicra2hrrt, sizeof(vicra2hrrt));
    mat_invert(v2s_inv, v2s);
    // alignment transformer t= v2s_inv.tv_inv.ref.v2s
    matmpy(t, v2s_inv, tv_inv);          // a=v2s_inv.tv_inv
    // mat_print(v2s_inv);
    // mat_print(tv_inv);
    mat_copy(tv,t); matmpy(t, tv, ref);  // a=a.ref   
    //  mat_print(t);
    mat_copy(tv,t); matmpy(t, tv, v2s);  // a=a.v2s   
    //  mat_print(t);
    mat_free(ref);
    mat_free(v2s);
    mat_free(v2s_inv);
    mat_free(tv);
    mat_free(tv_inv);
  } 
  */
  else if (tm != NULL) 
  {
    c_size = 1.0f; // assume mm3 voxels
    memcpy(t->data, tm->data, 12*sizeof(float));
  }
  else if (minc_file != NULL) 
  {
    err_flag=0;
    float *p=NULL;
    if ((fp=fopen(minc_file,"rt")) == NULL) {
      perror(minc_file);
      exit(1);
    }
    for (i=0; i<6; i++) fgets(line, sizeof(line), fp); // skip heading
    for (j=0;j<t->nrows-1 && !err_flag;j++) {
      p = t->data+j*t->ncols;
      if (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line,"%g %g %g %g", p, p+1, p+2, p+3) != 4) err_flag++;
      } else err_flag++;
    }
  }
  if (t_reverse) {
    Matrix tcopy = mat_alloc(4,4);
    mat_copy(tcopy, t);
    mat_invert(t,tcopy);
    mat_free(tcopy);
  }
  if (verbose) mat_print(t);
  mat_apply(t, x1, x2);
  float dx = (x2[0]-x1[0])*c_size;
  float dy = (x2[1]-x1[1])*c_size;
  float dz = (x2[2]-x1[2])*c_size;
  float d = sqrt(dx*dx + dy*dy + dz*dz);
  if (verbose) fprintf(stderr, "Motion point (%g,%g,%g) vector (%4.3g,%4.3g,%4.3g) amplitude %4.3g mm\n",
    x1[0], x1[1], x1[2], dx, dy, dz, d);
  printf("%d %4.3g %4.3g %4.3g %4.3g \n", frame_start_time, dx, dy, dz, d);
  if (in_file == NULL) {
    if (verbose) printf("done\n");
    exit(0);
  }
  if ((mptr=matrix_open(in_file,MAT_READ_ONLY, MAT_UNKNOWN_FTYPE)) == NULL) {
    matrix_perror(in_file);
    exit(1);
  }
  if (mptr->dirlist->nmats==0 || 
    (matrix = matrix_read( mptr, mptr->dirlist->first->matnum, GENERIC)) == NULL) {
    printf("error reading %s image\n",in_file);
    exit(1);
  }
  if (matrix->data_max> MAX_MU_VALUE) {
    printf("Error image data max = %g, mu-map expected with max value %f\n",
      matrix->data_max, MAX_MU_VALUE);
    exit(1);
  }
  if (matrix->data_type != IeeeFloat) matrix_float(matrix);
  float *fdata = (float*)matrix->data_ptr;
  int nvoxels = matrix->xdim*matrix->xdim*matrix->zdim;
  //short *dmap = (short*)calloc(nvoxels, sizeof(short));
  float *dmap = (float*)calloc(nvoxels, sizeof(float));
  i=0;
  pixel_size = matrix->pixel_size*10.0f; // convert to mm
  z_pixel_size = matrix->z_size*10.0f; // convert to mm

  mat_print(t);
  for (z=0; z<matrix->zdim; z++) {
    x1[2] = z*z_pixel_size/c_size; // in cubic pixels
    for (y=0, yrev=matrix->ydim-1; y<matrix->ydim; y++, yrev--) {
      if (y_reverse) x1[1] = yrev*pixel_size/c_size;
      else x1[1] = y*pixel_size/c_size; // in cubic pixels
      for (x=0; x<matrix->xdim; x++, i++) {
        if (fdata[i]>MIN_MU_VALUE) {
          x1[0] = x*pixel_size/c_size; // in cubic pixels
          mat_apply(t, x1, x2);
          dx = (x2[0]-x1[0])*c_size; // in mm
          dy = (x2[1]-x1[1])*c_size;  // in mm
          dz = (x2[2]-x1[2])*c_size;  // in mm
          d = sqrt(dx*dx + dy*dy + dz*dz);
          dmap[i] = d;
        }
      }
    }
  }
  if ((fp=fopen(out_file,"wb")) != NULL) {
    fwrite(dmap, sizeof(float), nvoxels, fp);
    fclose(fp);
  }
  else perror(out_file);
  free_matrix_data(matrix);
  exit(0);
}
