//static char sccsid[] = "%W% UCL-TOPO %E%";
/*
 Author : Sibomana@gmail.com
 Creation date :01-Dec-2009
 Modification history :
*/

#include <stdio.h>
#include <stdlib.h>
// #include <AIR/AIR.h>
// Now include directly from AIR src dir, path from CMake file
#include "AIR.h"
#include <ecatx/ecat_matrix.hpp>
#include <ecatx/matpkg.h>
#include <unistd.h>

static float vicra2hrrt[16] = {
  -0.0243298f, -0.999692f,    0.00485205f, 159.682f,
  -0.999644f,   0.0242749f,  -0.0110728f,  276.756f,
   0.0109516f,	-0.00511973f, -0.999927f,  -142.914f, 
   0.0f, 		     0.0f,      		  0.0f,         1.0f};

static void usage(const char *pgm)
{
  LOG_ERROR("%s Build %s %s \n", pgm, __DATE__, __TIME__);
  fprintf(stderr,
    "usage: %s [-t transformer | -a air_file | -q qw,qx,qy,qz,tx,ty,tz] | -M minc_file\n", pgm);
  fprintf(stderr,
    "      -t Transformer as rotation and translation: rz(yaw),rx(pitch),ry(roll),tx,ty,tz\n"
		"      -l label file (use pixel value=2 for computing displacement)\n"
    "      -a Transformer as AIR 5.X file\n" 
    "      -q Transformer as quaternion and translation: qw,qx,qy,qz,tx,ty,tz\n");
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

int  main(int argc, char **argv)
{
  int c=0, transformer=0, verbose=0;
  float tx=0, ty=0, tz=0, rx=0, ry=0, rz=0;
  float qr[4], q[4], qrt[3], qt[3];
  float x1[4], x2[4];
  int xdim=256, ydim=256, zdim=207;
  float pixel_size=1.21875f, z_pixel_size=1.21875f;
  float c_size=1.0f;   // isotropic voxel size used in AIR transformer
  char *air_file = NULL, *minc_file=NULL;
	char *label_file=NULL;
	unsigned char *ldata=NULL;  // label data;
  struct AIR_Air16 air_16;
  char reslice_file[FILENAME_MAX];
  ecat_matrix::MatrixFile *mptr=NULL;
  ecat_matrix::MatrixData *matrix=NULL;
  ecat_matrix::Image_subheader *imh=NULL;
  int matnum=0, frame_start_time=0;
  int i,j;
  FILE *fp=NULL;
	double dx, dy, dz, d;


  memset(reslice_file,0,sizeof(reslice_file));  // initialize to blank
	while ((c = getopt (argc, argv, "t:a:r:q:M:l:v")) != EOF) {
    switch (c) {
      case 'a':
        air_file = optarg;
        break;
      case 'M':
        minc_file = optarg;
        break;
      case 'l':
        label_file = optarg;
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
      case 'v' :
        verbose = 1;
      break;
    }
  }

  if (!transformer && !air_file && !minc_file) usage(argv[0]);
  Matrix t = mat_alloc(4,4);
                          // apply Transformer w.r.t AIR convention
  float cx = 0.5f*xdim*pixel_size;
  float cy = 0.5f*ydim*pixel_size;
  float cz = 0.5f*zdim*z_pixel_size;

  x1[0]=cx;  // center
  x1[1]=cy+100.0f; // 10 cm from the center 
  x1[2]= cz; // center
  x1[3] = 1.0f;
  c_size = (pixel_size>z_pixel_size)? z_pixel_size : pixel_size;

  if (air_file)
  {
    AIR_read_air16(air_file, &air_16);
    matspec(air_16.r_file, reslice_file, &matnum);
    if ((mptr=matrix_open(reslice_file,ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE)) != NULL)
    {
      if ((matrix=matrix_read( mptr, matnum, ecat_matrix::MatrixDataType::MAT_SUB_HEADER)) != NULL)
      {
        imh = (ecat_matrix::Image_subheader*)matrix->shptr;
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
  {
	  scale(t,pixel_size,pixel_size, z_pixel_size);
	  translate(t,-cx,-cy,-cz);
	  rotate(t,0,0,rz);        // apply yaw (+)
	  rotate(t,-rx,0,0);       // apply pitch (-)
	  rotate(t,0,-ry,0);       // apply roll (-)
	  translate(t,cx,cy,cz);
	  translate(t,-tx,ty,tz);	 // reverse x
	  scale(t, 1.0f/pixel_size, 1.0f/pixel_size, 1.0f/z_pixel_size);
  }
  else if (transformer==3)
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
      mat_print(v2s_inv);
      mat_print(tv_inv);
    mat_copy(tv,t); matmpy(t, tv, ref);  // a=a.ref   
      mat_print(t);
    mat_copy(tv,t); matmpy(t, tv, v2s);  // a=a.v2s   
      mat_print(t);
    mat_free(ref);
    mat_free(v2s);
    mat_free(v2s_inv);
    mat_free(tv);
    mat_free(tv_inv);
  } 
  else if (minc_file) 
  {
    int err_flag=0;
    float *p=NULL;
    if ((fp=fopen(minc_file,"rt")) == NULL) {
      perror(minc_file);
      return 1;
    }
    for (i=0; i<6; i++) fgets(line, sizeof(line), fp); // skip heading
    for (j=0;j<t->nrows-1 && !err_flag;j++) {
      p = t->data+j*t->ncols;
      if (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line,"%g %g %g %g", p, p+1, p+2, p+3) != 4) err_flag++;
      } else err_flag++;
    }
  }
  if (verbose) mat_print(t);
	int nvoxels = xdim*ydim*zdim;
	if (label_file != NULL) {
		if ((fp=fopen(label_file,"rb")) != NULL) {
			ldata = (unsigned char*)calloc(nvoxels, sizeof(unsigned char));
			if (fread(ldata, sizeof(unsigned char), nvoxels, fp) != nvoxels) {
				printf("Error reading %s\n", label_file);
				free(ldata);
				ldata=NULL;
			}
			fclose(fp);
		} else perror(label_file);
	}
	if (ldata != NULL) {
		i=j=0;
		d=dx=dy=dz= 0;
		for (int z=0; z<zdim; z++) {
			for (int y=0; y<ydim; y++) {
				for (int x=0; x<xdim; x++, i++)
				{
					if (ldata[i] == 2) {
						x1[0] = x*pixel_size - cx; 
						x1[1] = y*pixel_size - cy;
						x1[2] = z*z_pixel_size - cz;
						mat_apply(t, x1, x2);
						float ldx = (x2[0]-x1[0])*c_size;
						float ldy = (x2[1]-x1[1])*c_size;
						float ldz = (x2[2]-x1[2])*c_size;
						dx += sqrt(ldx*ldx);
						dy += sqrt(ldy*ldy);
						dz += sqrt(ldz*ldz);
						d += sqrt(ldx*ldx + ldy*ldy + ldz*ldz);
						j++;
					}
				}
			}
		}
		dx /= j;	dy /= j; dz /= j;
		d /= j;
	}
	else 
	{ 
		mat_apply(t, x1, x2);
		dx = (x2[0]-x1[0])*c_size;
		dy = (x2[1]-x1[1])*c_size;
		dz = (x2[2]-x1[2])*c_size;
		d = sqrt(dx*dx + dy*dy + dz*dz);
	}
  if (verbose) LOG_ERROR("Motion point (%g,%g,%g) vector (%4.3g,%4.3g,%4.3g) amplitude %4.3g mm\n",
    x1[0], x1[1], x1[2], dx, dy, dz, d);
  printf("%d %4.3g %4.3g %4.3g %4.3g \n", frame_start_time, dx, dy, dz, d);
  if (verbose) printf("done\n");
  return 0;
}
