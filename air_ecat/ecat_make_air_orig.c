/* 
 *This program creates  a .air file from input arguments		
 */

#include "ecat2air.h"
#include <ecatx/getopt.h>
#include <ecatx/matpkg.h>
#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static void usage(const char *pgm) {
  printf("\n%s Build %s %s\n",pgm,__DATE__,__TIME__);
  fprintf(stderr, "usage: %s -s standard_file -r reslice_file -i air_file | -t yaw(z_rot),pitch(x_rot),roll(y_rot),tx,ty,tz"
    " -o air_file [-N]\n", pgm);
	fprintf(stderr, "        Transformer can be specified with -i existing .air file or -t with rotation and translation\n");
	fprintf(stderr, "       -N when transformer was generated in neurological convention (SPM95)\n");
	fprintf(stderr, "        translations are expressed in mm\n");
	fprintf(stderr, "        rotations are expressed in degrees\n");
	fprintf(stderr, "        transformer data are provided by ecat_alignlinear\n");
	exit(1);
}


main(int argc, char **argv)
{
	struct AIR_Air16		air1, air2;
	int c, convention=0;
	char out_air_file[256], fname[256], *in_air_file=NULL;
	int matnum=0, ret=0;
	MatrixFile *mptr;
	MatrixData *matrix;
	float pixel_size;
	float tx,ty,tz,rx,ry,rz, tflag=0;
	double par[6];	/*pitch,roll,yaw,p,q,r*/
				/* pitch,roll,yaw are expressed in radians */
				/* p,q,r are respectively tx,ty,tz expressed in half-pixels */
	double      **e;
	double      ***de;
	double      ****ee;
	int permission=1;
	int zooming = 0;		/*flags whether files will be interpolated to cubic voxels*/
  Matrix mat;
  int i,j;


	memset(&air1,0,sizeof(air1));
	memset(&air2,0,sizeof(air2));
	memset(out_air_file,0,128);
  while ((c = getopt (argc, argv, "s:r:t:i:o:N")) != EOF) {
	switch(c) {
		case 'N' :
			convention = 1;			/* neurological convention */
			break;
		case 'i' :
      in_air_file=optarg;
			break;
		case 's' :
			if (strlen(optarg) > 127) {
				fprintf(stderr,"%s : filename too long\n",optarg);
				exit(1);
			}
			strcpy(air1.s_file,optarg);
			break;
		case 'r':
			if (strlen(optarg) > 127) {
				fprintf(stderr,"%s : filename too long %s\n",argv[0],optarg);
				exit(1);
			}
			strcpy(air1.r_file,optarg);
			break;
		case 'o' :
			if (strlen(optarg) > 127) {
				fprintf(stderr,"%s : filename too long %s\n",argv[0],optarg);
				exit(1);
			}
			strcpy(out_air_file,optarg);
			break;
		case 't' :
			if (sscanf(optarg,"%g,%g,%g,%g,%g,%g",&rz,&rx,&ry,&tx,&ty,&tz)!=6)
			{
				fprintf(stderr,"%s : invalid transformer %s\n",argv[0],optarg);
				exit(1);
			}
			tflag++;
			break;
		}
	}
	if (out_air_file[0]==0 || air1.s_file[0]==0 || air1.r_file[0]==0 ) usage(argv[0]);
  if (!tflag && !in_air_file) usage(argv[0]);
	ret = matspec(air1.s_file,fname,&matnum);
	mptr = matrix_open(fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
	if (mptr==NULL) crash("%s : can't open %s\n",argv[0],air1.s_file);
	if (ret == 0)	/* no matrix specified, use first */
			matnum = mptr->dirlist->first->matnum;
	matrix = matrix_read(mptr,matnum,MAT_SUB_HEADER);
	if (matrix == NULL) crash("%s : can't read image header\n",argv[0]);
	sprintf(air1.comment,"make_air -t %g,%g,%g,%g,%g,%g",rz,rx,ry,tx,ty,tz);
	if (convention==1) {
		strcat(air1.comment, " -N");
		tx = -tx;
		rx = -rx;
	}
	if (matrix->zdim == 1)					/* volume stored slice per matrix */
		matrix->zdim = mptr->mhptr->num_planes;
	air1.s.x_dim = matrix->xdim;
	air1.s.y_dim = matrix->ydim;
	air1.s.z_dim = matrix->zdim;
	air1.s.x_size = matrix->pixel_size*10;
	air1.s.y_size = matrix->y_size*10;
	air1.s.z_size = matrix->z_size*10;
	free_matrix_data(matrix);
	matrix_close(mptr);
	matnum = 0;
	ret = matspec(air1.r_file,fname,&matnum);
	mptr = matrix_open(fname, MAT_READ_ONLY, MAT_UNKNOWN_FTYPE);
	if (mptr==NULL) crash("%s : can't open %s\n",argv[0],air1.r_file);
	if (ret == 0)	/* no matrix specified, use first */
			matnum = mptr->dirlist->first->matnum;
	matrix = matrix_read(mptr,matnum,MAT_SUB_HEADER);
	if (matrix == NULL) crash("%s : can't read image header\n",argv[0]);
	if (matrix->zdim == 1)					/* volume stored slice per matrix */
		matrix->zdim = mptr->mhptr->num_planes;
  air1.r.x_dim = matrix->xdim;
  air1.r.y_dim = matrix->ydim;
  air1.r.z_dim = matrix->zdim;
  air1.r.x_size = matrix->pixel_size*10;
  air1.r.y_size = matrix->y_size*10;
  air1.r.z_size = matrix->z_size*10;

   	  /* Allocate matrices; see align.c AIR source code for explanation */
  e=AIR_matrix2(4,4);
  de=AIR_matrix3(4,4,6);
  ee=AIR_matrix4(6,6,4,4);

  /* build parameters array */
  pixel_size = (float)(min(air1.r.x_size,air1.r.y_size));
  pixel_size = (float)(min(pixel_size,air1.r.z_size));
  if (in_air_file==NULL)
  {
	  par[0] = rx*M_PI/180;		/* degrees to radians */
	  par[1] = ry*M_PI/180;
	  par[2] = rz*M_PI/180;
	  par[3] = 2*tx/pixel_size;
	  par[4] = 2*ty/pixel_size;
	  par[5] = 2*tz/pixel_size;

	  /*Convert parameters into AIR transformation matrix*/
	  AIR_uv3D6(par,e,de,ee,&air1.s,&air1.r,zooming);

    /* XML format delayed
	  xmlencode_air16(air_file,permission,e,zooming,&air1);
	  xmldecode_air16(air_file);
    */
    AIR_write_air16(out_air_file,permission,e,zooming,&air1);
  }
  else
  {
    mat = mat_alloc(4,4);
    AIR_read_air16(in_air_file,&air2);
    memcpy(air2.r_file, air1.r_file, sizeof(air2.r_file));
    memcpy(air2.s_file, air1.s_file, sizeof(air2.s_file));
	  pixel_size = (float)(min(air1.r.x_size,air1.r.y_size));
	  pixel_size = (float)(min(pixel_size,air1.r.z_size));
    for (i=0; i<4; i++) {
      for (j=0; j<4; j++) mat->data[j*4+i] = (float)air2.e[i][j];
    }
    scale(mat, (float)air2.r.x_size/pixel_size, (float)air2.r.y_size/pixel_size, (float)air2.r.z_size/pixel_size);
    mat_print(mat);
    for (i=0; i<4; i++) {
      for (j=0; j<4; j++) e[i][j] = air2.e[i][j] =  mat->data[j*4+i];
    }
    memcpy(&air2.r, &air1.r, sizeof(air2.r));
    memcpy(&air2.s, &air1.s, sizeof(air2.s));
    AIR_write_air16(out_air_file,permission,e,zooming,&air2);
  }
}
