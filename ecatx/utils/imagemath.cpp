/* @(#)imagemath.c	1.1 4/26/91 */

#ifndef	lint
static char sccsid[]="@(#)imagemath.c	1.1 4/26/91 Copyright 1990 CTI Pet Systems, Inc.";
#endif

/*
 * Feb-1996 :Updated by Sibomana@topo.ucl.ac.be for ECAT 7.0 support
 * Feb 2000: Updated by Michel@topo.ucl.ac.be to add masking operations:
 *	     imagec=c2*imagea if (imageb {gt,lt} c3) when op1={gt,lt}
 * Dec 2000 V2.3 :
 *       Updated by Sibomana@topo.ucl.ac.be to add matnum as argument
 * 02-Nov-2001 V2.4 :
 *       add bin as op2 operation to binarize image
 *
 * 27-Apr-2010: Added to HRRT Users software by sibomana@gmail.com
 */
   

#include "ecat_matrix.hpp"
#include <math.h>
#include <string.h>
#include "my_spdlog.hpp"

static char *version = "imagemath V2.4 02-Nov-2001";
static usage(pgm) 
char *pgm;
{
  printf("%s: Build %s %s\n", pgm, __DATE__, __TIME__);
	fprintf(stderr,"usage: %s imagea imageb imagec c1,c2,c3 op1,op2\n%s%s",pgm,
		 "\twhere op1={+,-,*,/,not,and,gt,lt} and op2={log,exp,sqr,bin}\n",
		 "\timagec=op2(c1+(c2*imagea)op1(c3*imageb)) when op1={+,-,*,/,not,and}\n");
	fprintf(stderr,"and \timagec=c2*imagea if (imageb {gt,lt} c3) when op1={gt,lt}\n");
	fprintf(stderr,"\timageb may be a matrix_spec or a constant value\n");
	fprintf(stderr,"\timageb may be a matrix_spec or a constant value\n");
	fprintf(stderr,"\t%s\n", version);
	exit(1);
}

main( argc, argv)
  int argc;
  char **argv;
{
	ecat_matrix::MatrixFile *file1=NULL, *file2=NULL, *file3=NULL;
	ecat_matrix::MatrixData *image1=NULL, *image2=NULL, *image3=NULL;
	ecat_matrix::MatrixData *slice1=NULL, *slice2=NULL;
	ecat_matrix::Main_header *mh3=NULL;
	ecat_matrix::Image_subheader *imh;
	float *imagea, *imageb, *imagec;
	float valb;
	short int *sdata;
	unsigned char  *bdata;
	float maxval, minval, scalef;
	int i, matnuma, matnumb, matnumc, plane, nblks;
	char op1 = ' ', op2=' ',  *ptr, fname[256];
	float c1=0,c2=1,c3=1;
	int npixels, nvoxels;
	int segment = 0;

	if (argc<6) usage(argv[0]);
/* get imagea */
	if (!matspec( argv[1], fname, &matnuma)) matnuma = ecat_matrix::mat_numcod(1,1,1,0,0);
	file1 = matrix_open( fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
	if (!file1)
	  LOG_EXIT( "Can't open file '{}'", fname);
	image1 = matrix_read(file1,matnuma,ecat_matrix::MatrixDataType::MAT_SUB_HEADER);
	if (!image1) LOG_EXIT( "Image '{}' not found\n", argv[1]);
	switch(file1->mhptr->file_type) {
	case ecat_matrix::DataSetType::InterfileImage :
	case ecat_matrix::DataSetType::PetImage :
	case ecat_matrix::DataSetType::PetVolume :
	case ecat_matrix::DataSetType::ByteImage :
	case ecat_matrix::DataSetType::ByteVolume :
		break;
	default :
		LOG_EXIT("input is not a Image nor Volume\n");
		break;
	}
	if (!matspec( argv[2], fname, &matnumb)) matnumb = ecat_matrix::mat_numcod(1,1,1,0,0);
	file2 = matrix_open( fname, ecat_matrix::MatrixFileAccessMode::READ_ONLY, ecat_matrix::MatrixFileType_64::UNKNOWN_FTYPE);
	if (!file2) {		/* check constant value argument */
	  if (sscanf(argv[2],"%g",&valb) != 1)
			LOG_EXIT("Can't open file '{}'", fname);
	}
	if (file2) {
		image2 = matrix_read(file2,matnumb,ecat_matrix::MatrixDataType::MAT_SUB_HEADER);
		if (!image2) LOG_EXIT( "image '{}' not found", argv[2]);
		if (image1->xdim != image2->xdim || image1->ydim != image2->ydim ||
			image1->zdim != image2->zdim)
			LOG_EXIT("{} and {} are not compatible",argv[1], argv[2]);
	}
	npixels = image1->xdim*image1->ydim;
	nvoxels = npixels*image1->zdim;

/* get imagec specification and write header */
	if (!matspec( argv[3], fname, &matnumc)) matnumc = ecat_matrix::mat_numcod(1,1,1,0,0);
	mh3 = (ecat_matrix::Main_header*)calloc(1, sizeof(ecat_matrix::Main_header));
	memcpy(mh3, file1->mhptr, sizeof(ecat_matrix::Main_header));
	mh3->file_type = ecat_matrix::DataSetType::PetVolume;
	file3 = matrix_create( fname, ecat_matrix::MatrixFileAccessMode::OPEN_EXISTING, mh3);
	if (!file3) 
		LOG_EXIT( "Can't open file '{}'", fname);
	image3 = (ecat_matrix::MatrixData*)calloc(1, sizeof(ecat_matrix::MatrixData));
	memcpy(image3, image1, sizeof(ecat_matrix::MatrixData));
	imh = (ecat_matrix::Image_subheader*)calloc(1,sizeof(ecat_matrix::Image_subheader));
	memcpy(imh,image1->shptr,sizeof(ecat_matrix::Image_subheader));
	imagec = (float*)malloc(nvoxels*sizeof(float));
	imagea = (float*)malloc(npixels*sizeof(float));
	imageb = (float*)malloc(npixels*sizeof(float));

/* Decode Operators and Coefficients */
	sscanf( argv[4], "%f,%f,%f", &c1, &c2, &c3);
	ptr = strtok(argv[5],",");
	op1 = *ptr;
	if ((ptr=strtok(NULL,"'")) != NULL) {
		if (!strcmp( ptr, "log")) op2 = 'l';
		else if (!strcmp( ptr, "exp")) op2 = 'e';
			else if (!strcmp( ptr, "sqr")) op2 = 's';
				else if (!strcmp( ptr, "bin")) op2 = 'b';
	}

/* compute and write data */
	if (!file2) {
		for (i=0; i<npixels; i++) imageb[i] = valb;
	}
	for (plane=0; plane< image1->zdim; plane++) {
		slice1 = matrix_read_slice(file1,image1,plane,segment);
		if (slice1->data_type==ecat_matrix::MatrixDataType::SunShort || slice1->data_type==ecat_matrix::MatrixDataType::VAX_Ix2) {
			sdata = (short*)slice1->data_ptr;
			for (i=0; i<npixels; i++)
	  			imagea[i] = slice1->scale_factor*sdata[i];
		} else {	/* assume byte data */
			bdata = (unsigned char *)slice1->data_ptr;
			for (i=0; i<npixels; i++)
				imagea[i] = slice1->scale_factor*bdata[i];
		}
		free_matrix_data(slice1);

		if (file2) {
			slice2 = matrix_read_slice(file2,image2,plane,segment);
			if (slice2->data_type==ecat_matrix::MatrixDataType::SunShort || slice2->data_type==ecat_matrix::MatrixDataType::VAX_Ix2) {
				sdata = (short*)slice2->data_ptr;
				for (i=0; i<npixels; i++)
	  				imageb[i] = slice2->scale_factor*sdata[i];
			} else {    /* assume byte data */
				bdata = (unsigned char *)slice2->data_ptr;
				for (i=0; i<npixels; i++)
					imageb[i] = slice2->scale_factor*bdata[i];
			}
			free_matrix_data(slice2);
		}

		switch( op1)
		{
	  	case '+':
			for (i=0; i<npixels; i++)
			  imagea[i] = c1+c2*imagea[i]+c3*imageb[i];
			break;
	  	case '-':
			for (i=0; i<npixels; i++)
			  imagea[i] = c1+c2*imagea[i]-c3*imageb[i];
			break;
	  	case '*':
			for (i=0; i<npixels; i++)
			  imagea[i] = c1+c2*imagea[i]*c3*imageb[i];
			break;
	  	case '/':
			for (i=0; i<npixels; i++)
			  imagea[i] = c1+(imageb[i]==0.0f) ? 0.0f :
				c2*imagea[i]/(c3*imageb[i]);
			break;
		case 'a' :		/* c2*imagea if (imageb!=0) */
			for (i=0; i<npixels; i++)
			imagea[i] = (imageb[i]==0.0f)? 0.0f : c2*imagea[i];
			break;
		case 'n' :	/* c2*imagea if (imageb==0) */
			for (i=0; i<npixels; i++)
			imagea[i] = (imageb[i]==0.0f)? c2*imagea[i] : 0.0f;
			break;
		case 'g' :	/* c2*imagea if (imageb>c3) */
			for (i=0; i<npixels; i++)
			imagea[i] = (imageb[i]>c3)? c2*imagea[i] : 0.0f;
			break;
		case 'l' :	/* c2*imagea if (imageb<c3) */
			for (i=0; i<npixels; i++)
			imagea[i] = (imageb[i]< c3)? c2*imagea[i] : 0.0f;
			break;

	  	default:
			LOG_EXIT("Illegal operator {}: chose from {+,-,*,/,and,not,gt,lt}",op1);
		}
		switch (op2) {
		case 'b' :
	  		for (i=0; i<npixels; i++)
	    		imagea[i] = (imagea[i] > 0.0f) ? 1.0f : 0.0f;
			break;
		case 'l' :
	  		for (i=0; i<npixels; i++)
	    		imagea[i] = (imagea[i] < 0.0f) ? 0.0f : logf(imagea[i]);
			break;
		case 'e' :
			for (i=0; i<npixels; i++)
				imagea[i] = expf(imagea[i]);
			break;
		case 's' :
	  		for (i=0; i<npixels; i++)
	    		imagea[i] = (imagea[i] < 0.0f) ? 0.0f : sqrtf(imagea[i]);
			break;
	/*	default	:  no operation */
		}
		memcpy(imagec+npixels*plane, imagea, npixels*sizeof(float));
	}
/* scale the output image */

	minval = maxval = imagec[0];
	for (i=0; i<nvoxels; i++)
	{
	  if (imagec[i]>maxval) maxval = imagec[i];
	  if (imagec[i]<minval) minval = imagec[i];
	}

	if (image1->data_type!=ecat_matrix::MatrixDataType::ByteData || image2->data_type!=ecat_matrix::MatrixDataType::ByteData || minval<0)
	{
		image3->shptr = (void *)imh;
		nblks = (nvoxels*sizeof(short)+511)/512;
		image3->data_size = nblks*512;
		image3->data_type = imh->data_type = ecat_matrix::MatrixDataType::SunShort;
		if (fabs(minval) < fabsf(maxval)) scalef = fabsf(maxval)/32767.0f;
		else scalef = fabsf(minval)/32767.0f;
		sdata = (short*)imagec;				/* reuse huge float array */
		for (i=0; i<nvoxels; i++)
	  		sdata[i] = (short)(0.5f+imagec[i]/scalef);
	} else {
		image3->shptr = (void *)imh;
		image3->data_type = imh->data_type = ecat_matrix::MatrixDataType::ByteData;
		nblks = (nvoxels*sizeof(unsigned char )+511)/512;
		image3->data_size = nblks*512;
		scalef = fabsf(maxval)/255;
		bdata = (unsigned char *)imagec;				/* reuse huge float array */
		for (i=0; i<nvoxels; i++)
			bdata[i] = (unsigned char )(0.5+imagec[i]/scalef);
	}
	image3->data_ptr = (void *)imagec;
	image3->scale_factor = imh->scale_factor = scalef;
	imh->image_max = (short)((0.5f+maxval)/scalef);
	imh->image_min = (short)((0.5f+minval)/scalef);
	strncpy( imh->annotation, version, 40);
	matrix_write(file3, matnumc, image3);
	matrix_close(file1);
	if (file2) matrix_close(file2);
	matrix_close(file3);
}
