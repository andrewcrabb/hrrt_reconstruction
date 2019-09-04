/* Copyright 1995 Roger P. Woods, M.D. */
/* Modified 12/16/95 */

/*
 * int write_air16()
 *
 * writes an AIR reslice parameter file
 * adjusts e matrix to cubic voxel default when zooming==0
 * assumes adjustment has already been made when zooming!=0
 *
 * returns:
 *	1 if successful
 *	0 if unsuccessful
 */
/*
 * Modified 31-May-1996 by Merence Sibomana (Sibomana@topo.ucl.ac.be)
 * as xwrite_air16 to use machine independent XDR format.
 */
#include "ecat2air.h"
#include	<unistd.h>

int xmlencode_air16(const char *outfile, int permission, double **e, int zooming,
                 struct AIR_Air16 *air)
{
	FILE 		*fp;
	int		i,j;
	double		pixel_size_s;
	double		f[4][4];
	int error = 0;


	for (j=0;j<4;j++){
		for(i=0;i<4;i++){
			air->e[j][i]=e[j][i];
			f[j][i]=e[j][i];
		}
	}

	if(!zooming){
		pixel_size_s=air->s.x_size;
		if(air->s.y_size<pixel_size_s) pixel_size_s=air->s.y_size;
		if(air->s.z_size<pixel_size_s) pixel_size_s=air->s.z_size;

		if(fabs(air->s.x_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			air->e[0][0]/=(air->s.x_size/pixel_size_s);
			air->e[0][1]/=(air->s.x_size/pixel_size_s);
			air->e[0][2]/=(air->s.x_size/pixel_size_s);
			air->e[0][3]/=(air->s.x_size/pixel_size_s);
		}

		if(fabs(air->s.y_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			air->e[1][0]/=(air->s.y_size/pixel_size_s);
			air->e[1][1]/=(air->s.y_size/pixel_size_s);
			air->e[1][2]/=(air->s.y_size/pixel_size_s);
			air->e[1][3]/=(air->s.y_size/pixel_size_s);
		}

		if(fabs(air->s.z_size-pixel_size_s)>AIR_CONFIG_PIX_SIZE_ERR){
			air->e[2][0]/=(air->s.z_size/pixel_size_s);
			air->e[2][1]/=(air->s.z_size/pixel_size_s);
			air->e[2][2]/=(air->s.z_size/pixel_size_s);
			air->e[2][3]/=(air->s.z_size/pixel_size_s);
		}
	}
	
 /*Open output file if permitted to do so*/
  if(!permission){
    if(access(outfile, F_OK) == 0) {
      printf("file %s exists, no permission to overwrite\n",outfile);
      for (j=0;j<4;j++) {
				for(i=0;i<4;i++) {
          air->e[j][i]=f[j][i];
          e[j][i]=f[j][i];
        }
      }
      return 0;
    }
  }
	if((fp=fopen(outfile,"wb"))==NULL){
		printf("cannot open file %s for output\n",outfile);
		for (j=0;j<4;j++){
			for(i=0;i<4;i++){
				air->e[j][i]=f[j][i];
				e[j][i]=f[j][i];
			}
		}
		return 0;
	}
 
	/*Write out air struct*/
  fprintf(fp,"<AIR_Transformer>\n");
  fprintf(fp,"<reference");
  fprintf(fp," filename=\"%s\"", air->s_file);
  fprintf(fp," dimension=\"%d,%d,%d\"", air->s.x_dim,air->s.y_dim,air->s.z_dim );
  fprintf(fp," voxelsize=\"%g,%g,%g\"", air->s.x_size,air->s.y_size,air->s.z_size);
  fprintf(fp," />\n");
  fprintf(fp,"<reslice>\n");
  fprintf(fp," filename=\"%s\"\n", air->s_file);
  fprintf(fp," dimension=\"%d,%d,%d\"\n", air->s.x_dim,air->s.y_dim,air->s.z_dim );
  fprintf(fp," voxelsize=\"%g,%g,%g\"\n", air->s.x_size,air->s.y_size,air->s.z_size);
  fprintf(fp," matrix=\"");
  for (j=0; j<4; j++) { // row
    for (i=0; i<4; i++) { // column
      if (i==0) {
        if (j==0) fprintf(fp,"%g",air->e[i][j]);
        else  fprintf(fp,"         %g",air->e[i][j]);
      }
      else fprintf(fp,", %g",air->e[i][j]);
    }
    if (j<3) fprintf(fp,"\n");
    else fprintf(fp,"\"\n");
  }
  fprintf(fp," comment=\"%s\"\n", air->comment);
  fprintf(fp,"</reslice>\n");
  fprintf(fp,"</AIR_Transformer>\n");
	fclose(fp);

	for (j=0;j<4;j++){
		for(i=0;i<4;i++){
			air->e[j][i]=f[j][i];
			e[j][i]=f[j][i];
		}
	}
	if (error) return 0;
	return 1;
}
