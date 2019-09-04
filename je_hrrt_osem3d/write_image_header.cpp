/*-----------------------------------------------------------------
What: write_image_header.cpp

Why: This function writes an image header.

Who: Merence Sibomana

When: 3/31/2004

Copyright (C) CPS Innovations 2002-2003-2004 All Rights Reserved.
-------------------------------------------------------------------*/
/*
 Modification history:
 25-SEP-2008: (MS) Added PSF flag
 08-OCT-2008: Add sw_version and sw_build_id (MS)
*/

//mods for pixel sizes & other reasons 6/21/04 jj

// New rule: 6/21/04
// anything incoming is overwritten here.
// if i don't overwrite it, it's forwarded without modification

#include	<stdio.h>
#include	<string.h>

//#include	"Cluster.h"
//#include    "hrrt_osem3d.h"
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

static float scatter_faction(const char *scatter_file)
{
  float scf=0.0f;
  char hdr_file[_MAX_PATH], *p=NULL;
  FILE *fp=NULL;

  strcpy(hdr_file, scatter_file);
  if ((p=strrchr(hdr_file,'.')) != NULL) strcpy(p,".h33");
  if ((fp=fopen(hdr_file,"rt")) != NULL)
  {
    while( fgets(line,LINESIZE,fp) != NULL )
      if (strstr(line,"scatter fraction"))
        if ((p=strstr(line, ":=")) != NULL) 
          if (sscanf(p+2,"%g",&scf) == 1) return scf;
    fclose(fp);
  }
  return 0.0f;
}


int write_image_header(ImageHeaderInfo *info, int psf_flag,
                       const char *sw_version, const char *sw_build_id,
                       int sino_flag)
{
	float	eps = 0.001f;
	FILE	*f_in=NULL, *f_out=NULL;
	char	sinoHeaderName[_MAX_PATH];
	int		sinoHeaderExists;
	char	ImgHeaderName[_MAX_PATH];
	char	*p=NULL;

	#define _MAX_DRIVE 0
	#define _MAX_DIR 256
	#define _MAX_FNAME 256
	#define _MAX_EXT 256

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char ImageName[_MAX_FNAME];
	char ext[_MAX_EXT];

	//split output image file name into component parts

	// !sv we actually only need to separate ImageName and ext for the following code
	// this is a dirty hack
	char *lastslash = strrchr ( info->imagefile,  '/' );
	if ( lastslash ) {
	  strcpy ( ImageName, lastslash + 1 );
	}
	else {
	  strcpy ( ImageName, info->imagefile );
	}  
	char *lastdot = strrchr( ImageName, '.' );
	if ( lastdot ) { 
	  strcpy ( ext, lastdot );  // includes dot
	  *lastdot = '\0'; // effectively deleting the extension from ImageName
	}
	else {
	  strcpy ( ext, "" );
	}

	//Check image file name, create header name
	if( strlen(info->imagefile) == 0 ) {
		printf("Error! WriteImageHeader: Null File Name\n");
		return -1;
	}
	else printf("WriteImageHeader: image '%s'\n",info->imagefile);

	sprintf( ImgHeaderName, "%s.hdr",info->imagefile );

	//Check image file
	if((f_in = fopen(info->imagefile,"rb")) == NULL ){
		printf("WriteImageHeader: No such image file '%s'\n",info->imagefile);
		return -1;
	}
	else {
		printf("WriteImageHeader: Image file '%s' found!\n",info->imagefile);
		fclose(f_in);
		f_in = NULL;
	}

  if (!sino_flag)
  {
  	//Check parameters
	  if( info->datatype<1	|| info->datatype>3	
		  ||	info->nx<=0			|| info->ny<=0		|| info->nz<=0
		  ||	info->dx<=eps		|| info->dy<=eps	|| info->dz<=eps )
	  {
		  printf("WriteImageHeader: Parameter out of whack.\n");
		  return -1;
	  }
  }

	//determine if sinogram header exists & if so, what it's called

	sinoHeaderExists = 0;

	if( strlen( info->truefile ) > 0 ){
		sprintf(sinoHeaderName, "%s.hdr",info->truefile  );
		printf("WriteImageHeader: true sinoHeaderName is %s\n",sinoHeaderName);
		sinoHeaderExists++;
	}
	else
		if( strlen( info->promptfile ) > 0 ){
			sprintf(sinoHeaderName, "%s.hdr",info->promptfile  );
			printf("WriteImageHeader: prompt sinoHeaderName is %s\n",sinoHeaderName);
			sinoHeaderExists++;
		}

		//replace directory separator

		for (p=sinoHeaderName; *p!='\0'; p++) 
			if (*p == IN_DIR_SEPARATOR) *p = DIR_SEPARATOR;

		for (p=ImgHeaderName; *p!='\0'; p++) 
			if (*p == IN_DIR_SEPARATOR) *p = DIR_SEPARATOR;


		//open image header

		if((f_out=fopen(ImgHeaderName,"wt")) == NULL){
			printf("WriteImageHeader: Cannot open image header file '%s'\n",ImgHeaderName);
			return -4;
		} 
		else {
			printf("WriteImageHeader: File '%s' open ok!\n",ImgHeaderName);
		}

		if( sinoHeaderExists ){
			if((f_in = fopen(sinoHeaderName,"r")) == NULL){
				printf("WriteImageHeader: Cannot open sino header file '%s'\n",sinoHeaderName);
			}
		}

		/*--------------------*/
		/* Write Image Header */

		//this part is always written

		fprintf( f_out,"!INTERFILE\n");
		fprintf( f_out,"name of data file := %s%s\n", ImageName, ext);
		if (sino_flag)
    {
      fprintf( f_out,"data format := sinogram\n");
      fprintf(f_out,"sinogram data type := corrected\n");
    }
    else  fprintf( f_out,"data format := image\n");
		fprintf( f_out,"number format := %s\n", data_types[info->datatype]);
		fprintf( f_out,"number of bytes per pixel := %d\n", data_bytes[info->datatype]);

		if (!sino_flag)
    {
      fprintf( f_out,"number of dimensions := 3\n");
		  fprintf( f_out,"matrix size [1] := %d\n", info->nx);		
		  fprintf( f_out,"matrix size [2] := %d\n", info->ny);		
		  fprintf( f_out,"matrix size [3] := %d\n", info->nz);

		  fprintf( f_out,"scaling factor (mm/pixel) [1] := %f\n", info->dx);
		  fprintf( f_out,"scaling factor (mm/pixel) [2] := %f\n", info->dy);
		  fprintf( f_out,"scaling factor (mm/pixel) [3] := %f\n", info->dz);
  
      switch (info->recontype) {
			case 0:	if (!psf_flag)
                fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-UW\n");
              else fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-UW-PSF\n");
              break;
			case 1:	if (!psf_flag)
                fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-AW\n");
              else fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-AW-PSF\n");
              break;
			case 2:	if (!psf_flag)
                fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-ANW\n");	
              else fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-ANW-PSF\n");
              break;
			case 3:	if (!psf_flag)
                fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-OP\n");
              else fprintf( f_out, "reconstruction method := inki-2006-may-OSEM3D-OP-PSF\n");
              break;
      }
      fprintf( f_out, "reconstruction version := %s\n", sw_version);
      fprintf( f_out, "reconstruction build ID := %s\n", sw_build_id);
    }
    else
    {

      fprintf( f_out, "sinogram correction version := %s\n", sw_version);
      fprintf( f_out, "sinogram correction build ID := %s\n", sw_build_id);
    }

    if( strlen( info->truefile ) > 0 )
			fprintf( f_out,"true file used := %s\n", fname(info->truefile));

		if( strlen( info->promptfile ) > 0 ) 
			fprintf( f_out,"prompt file used := %s\n", fname(info->promptfile));

		if( strlen( info->delayedfile ) > 0 )
			fprintf( f_out,"delayed file used := %s\n", fname(info->delayedfile));

		if( strlen( info->normfile ) > 0 )
			fprintf( f_out,"norm file used := %s\n", fname(info->normfile));

		if( strlen( info->attenfile ) > 0 )
			fprintf( f_out,"atten file used := %s\n", fname(info->attenfile));

		if( strlen( info->scatterfile ) > 0 )
    { float scf;
			fprintf( f_out,"scatter file used := %s\n", fname(info->scatterfile));
      if ((scf=scatter_faction(info->scatterfile)) > 0)
        fprintf( f_out,"scatter fraction := %g\n", scf);
    }

		if (!sino_flag) 
    {
      fprintf( f_out,"number of iterations := %d\n", info->iterations);
	  	fprintf( f_out,"number of subsets := %d\n", info->subsets);
    }

		//	fprintf( f_out,"SMICC-OSEM3D build := %s\n", info->BuildTime);
		//	fprintf( f_out,"SMICC-Server build := %s\n", BuildTime());

		// this part is written only if there's a sinogram header on the input side

		if( f_in != NULL ){

			while( fgets(line,LINESIZE,f_in) != NULL ){

				//ignore anything we've already written
				if(!strncmp(line,"!INTERFILE", 10     )) continue;
				if(!strncmp(line,"!Interfile", 10     )) continue;
				if(!strncmp(line,"!interfile", 10     )) continue;

				if( strstr(line,"name of data file"    )) continue;
				if( strstr(line,"data format"          )) continue;
				if( strstr(line,"number format"        )) continue;
				if( strstr(line,"number of bytes per"  )) continue;
        if (sino_flag)
        {
          if( strstr(line,"frame :=" )) continue; 
          if( strstr(line,"sinogram data type" )) continue; 
          if( strstr(line,"name of true data file" )) continue; 
        }
        else
        {
				  if( strstr(line,"number of dimensions" )) continue;
				  if( strstr(line,"matrix size"          )) continue;
				  if( strstr(line,"scaling factor"       )) continue;
        }
				if( strstr(line,"reconstruction method")) continue;
				if( strstr(line,"true file used"       )) continue;
				if( strstr(line,"prompt file used"     )) continue;
				if( strstr(line,"delayed file used"    )) continue;
				if( strstr(line,"norm file used"       )) continue;
				if( strstr(line,"atten file used"      )) continue;
				if( strstr(line,"scatter file used"    )) continue;
				if( strstr(line,"number of iterations" )) continue;
				if( strstr(line,"number of subsets"    )) continue;
				if( strstr(line,"CPSCC-OSEM3D build"   )) continue;
				if( strstr(line,"CPSCC-Server build"   )) continue;

				//if line not ignored, simply copy it forward

				fprintf( f_out, "%s", line);
			}

		}

		fclose(f_out);

		if( f_in != NULL) fclose(f_in);

		printf("WriteImageHeader: image '%s' complete\n",info->imagefile);

		return 0;
}
