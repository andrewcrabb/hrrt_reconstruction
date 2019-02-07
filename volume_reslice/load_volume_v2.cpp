//static char sccsid[] = "%W% UCL-TOPO %E%"
/*
 * The function
 * ecat_matrix::MatrixData *load_volume(ecat_matrix::MatrixFile*, int frame, int cubic, int interp)
 * loads a frame from a ecat_matrix::MatrixFile. The frames slices may be stored as separate
 * matrices (ECAT V6x files) or as a single volume data.
 * if the cubic flag is non zero, the function returns a volume with cubic
 * voxel.
 * if the interp is set cubic voxels are made by linear interpolation in the
 * z-direction and by nearest voxel pixel otherwise. This flag is not used
 * when the cubic flag is not set.
 * 
 * THE FUNCTION IS WRITTEN AS AN INCLUDE FILE BECAUSE IT MAY CALL A C++
 * FUNCTION "display_message". THIS is the only way I found to call C++
 * code within C code.
 *
 * History :
 *     creation date :  06-DEC-1995 (Sibomana@topo.ucl.ac.be)
 *     28-JUL-1998   :  change function load_volume to load_volume_v2
 *          Remove uncorrectely support of multibed images, add bed argument
 *          to specify the  wanted bed position. When the frame or bed is not
 *          specified (value <0), the first frame or bed position in the file
 *          is loaded.
 *          Add support for non-contiguous and/or overlapping slices
 *          Replace arguments cubic by pixel_size (user pixel size :
 *          0.0 means use image pixel_size, reformate images to specified
 *          value otherwise)
 *
 */
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "ecatx/matrix.h"
#include "ecatx/DICOM.h"
#include "matrix_resize.h"
#include "Modality.h"

#define my_max(a,b) (((a)>(b))?(a):(b))

typedef struct _Tslice
{
    ecat_matrix::MatrixData data;
    float zloc;
} Tslice;

#if defined(__STDC__) || defined(__cplusplus)
typedef int (*qsort_func)(const void *, const void *);
#endif
#if defined(__cplusplus)
extern int debug_level;
extern void loading_progress(float done, float total);
extern void display_message(const char*);
#else
#define display_message(s) 
#endif

static int compare_zloc(const Tslice *a, const Tslice *b) 
{
    if (a->zloc < b->zloc) return(-1);
    else if (a->zloc > b->zloc) return (1);
    else return (0);
}

static int slice_sort(Tslice *slices, int count)
{
	qsort(slices, count, sizeof(Tslice), (qsort_func)compare_zloc);
	return 1;
}

/*
 * matrix_read_v2 uses matrix_read to load image and converts data 
 * from slope,intercept to short integer with scale_factor
 */
static ecat_matrix::MatrixData *matrix_read_v2(ecat_matrix::MatrixFile *file, int matnum, ecat_matrix::MatrixDataType type, int plane)
{
	char buf[80];
	int i, npixels, b;
	float intercept=0.0f, slope=1.0f, f=0.0f;
	ecat_matrix::Main_header *mh = file->mhptr;
	char *magic_number = mh->magic_number;
	int magic_size = sizeof(mh->magic_number);
	ecat_matrix::MatrixData *matrix = matrix_read(file, matnum, type);
	if (matrix==NULL || matrix->dicom_header == NULL) return matrix;
	if (dicom_scan_string(0x0020, 0x0032, buf, matrix->dicom_header,
		matrix->dicom_header_size)) {
		sscanf(buf, "%g\\%g\\%g", 
			&matrix->x_origin, &matrix->y_origin, &matrix->z_origin);
	}
    if (dicom_scan_string(0x0028, 0x1052, buf, matrix->dicom_header,
          	matrix->dicom_header_size)) sscanf(buf, "%g", &intercept); 
   	if (dicom_scan_string(0x0028, 0x1053, buf, matrix->dicom_header,
		matrix->dicom_header_size))	sscanf(buf, "%g", &slope);
    ecat_matrix::Image_subheader *imh = (ecat_matrix::Image_subheader*)matrix->shptr;
	matrix->scale_factor *= slope;
	imh->scale_factor = matrix->scale_factor;
	b = (int)(intercept/slope);
	if (b != 0 && matrix->data_type!=ecat_matrix::MatrixDataType::ByteData)
	{
		npixels = matrix->xdim*matrix->ydim;
		short *data = (short*)matrix->data_ptr;
		if (data != NULL)
			for (i=0; i<npixels; i++) data[i] += b;
		matrix->data_min += intercept;
		matrix->data_max += intercept;
		imh->image_min = (int)(matrix->data_min/matrix->scale_factor);
		imh->image_max = (int)(matrix->data_max/matrix->scale_factor);
	}
	if (plane != 1) return matrix;
	//
	// Get once (plane == 1) PET main header information from dicom header
	if (dicom_scan_string(0x0008, 0x0060, buf, matrix->dicom_header,
		matrix->dicom_header_size))
	{
		if (modality_code(buf) == PET && mh->calibration_units==ecat_matrix::CalibrationStatus::Uncalibrated)
		{
			if (dicom_scan_string(0x0028, 0x1054, buf, matrix->dicom_header,
				matrix->dicom_header_size)) {
				if (strncasecmp(buf,"BQML",4) == 0) mh->calibration_units = ecat_matrix::CalibrationStatus::Calibrated;
			}
			if (dicom_scan_string(0x0018, 0x1072, buf, matrix->dicom_header,
				matrix->dicom_header_size)) {
				// Radiopharmaceutical start time in sec.microsec
				if (sscanf(buf,"%g", &f) == 1) {
					if (f>0.0f) mh->dose_start_time = (unsigned)f;
				}
			}
			if (dicom_scan_string(0x0018, 0x1074, buf, matrix->dicom_header,
				matrix->dicom_header_size)) {
				// Radionuclide total dose
				// 1 milli-curie = 37 mega-bequerel (MBq)
				// Both MBq and and Bq are used in the same DICOM element as dose units
				if (sscanf(buf,"%g", &f) == 1) {
					if (f>10e6) f *= 10e-6f; // assume Bq/cc, convert to MBq/cc
					if (f>0.0f && f<10e6f) {
						// convert dosage from MBq to milli-curies
						mh->dosage = f/37.0f;
					}
				}
			}
			if (dicom_scan_string(0x0018, 0x1074, buf, matrix->dicom_header,
				matrix->dicom_header_size)) {
				// Isotope halftime in seconds
				if (sscanf(buf,"%g", &f) == 1) mh->isotope_halflife = f;
			}
		}
	}
	return matrix;
}
	
static ecat_matrix::MatrixData *load_slices(/*CProgressCtrl *progress_ctrl,*/ ecat_matrix::MatrixFile *matrix_file,
							   Tslice *slice, int nslices, float pixel_size, int interp)
{
	int i, j, k;
	ecat_matrix::MatrixData *s1, *s2;
	ecat_matrix::MatrixData *volume;
	void * vdata=NULL;
	short *vp=NULL, *p1=NULL, *p2=NULL;
	unsigned char  *b_vp=NULL, *b_p1=NULL, *b_p2=NULL;
	float *f_vp=NULL, *f_p1=NULL, *f_p2=NULL;
	int xdim, ydim, zdim, npixels, nvoxels,nblks;
	char cbufr[256];
	float  z_size,zsep,zloc, w1, w2,w;
	float *scalef=NULL;		/* slice scale factors */
	float data_min, data_max;
	int z_reform = 0;
	int foo=0;  /* segment argument in matrix_read_slice for 3D scans */
	int align = 0;		/* not align xdim on 4 byte boundaries */
	float eps = 0.01F; /* 0.01 cm = 0.1 mm */

	slice_sort(slice, nslices);
/* set z pixel size to lowest */
	if (nslices > 1) {
    	z_size = slice[1].zloc - slice[0].zloc;
       	if (z_size < eps) throw("First 2 slices with same z location");
    }
	else z_size = slice[0].data.z_size;
    if (z_size < eps) throw("Invalid slice thickness");
	for (i=1; i< nslices; i++)
	{
		zsep = slice[i].zloc - slice[i-1].zloc;
		/* We don't use slice thickness since slices may not be contiguous */
        if (zsep < 2*eps) continue;  /* assume thickness transition */
		if (z_size > zsep+eps) {
		 	if (debug_level > 0)
				fprintf(stderr, "at slice %d : zsep = %g\n", i, zsep);
			z_size = zsep;
		}
	}

/* set z_reform if slice recomputing needed */
	zloc = slice[0].zloc;
	for (i=0; i< nslices; i++, zloc += z_size)
		if (fabs(zloc-slice[i].zloc) > eps) {
			if (debug_level > 0)
				fprintf(stderr,
						"at slice %d : %g-%g > %g ==> Z REFORM\n",
						i, 10*zloc, 10*slice[i].zloc, 10*eps);
			z_reform = 1;
		}
	
	zdim = (int)(0.5+(slice[nslices-1].zloc - slice[0].zloc)/z_size) + 1;
	volume = matrix_read_v2(matrix_file,slice[0].data.matnum,GENERIC,1);
	if (volume->zdim > 1) zdim = volume->zdim;

/* if user requested pixel size compute new image size */
	if (pixel_size>0.005 &&
         (pixel_size>volume->pixel_size || pixel_size>volume->y_size) ) {
		xdim = (int)(0.5+volume->xdim*volume->pixel_size/pixel_size);
		ydim = (int)(0.5+volume->ydim*volume->y_size/pixel_size);
	} else {
		xdim = volume->xdim;
		ydim = volume->ydim;
	}
	scalef = (float*)calloc(zdim,sizeof(float));
	npixels = xdim*ydim;
	nvoxels = npixels*zdim;
	switch (volume->data_type) {
	case ecat_matrix::MatrixDataType::ByteData : 
		nblks = (nvoxels+ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
		vdata = (void *)calloc(nblks,ecat_matrix::MatBLKSIZE);
		b_vp = (unsigned char *)vdata;
		break;
	case ecat_matrix::MatrixDataType::VAX_Ix2:
	case ecat_matrix::MatrixDataType::SunShort:
		nblks = (nvoxels*sizeof(short)+ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
		vdata = (void *)calloc(nblks,ecat_matrix::MatBLKSIZE);
		vp = (short*)vdata;
		break;
	case ecat_matrix::MatrixDataType::IeeeFloat:
		nblks = (nvoxels*sizeof(float)+ecat_matrix::MatBLKSIZE-1)/ecat_matrix::MatBLKSIZE;
		vdata = (void *)calloc(nblks,ecat_matrix::MatBLKSIZE);
		f_vp = (float*)vdata;
		break;
	default:
		fprintf(stderr,"unsupported data type : %d\n",volume->data_type);
    exit(1);
	}
	if (vdata==NULL) {
		sprintf(cbufr, "malloc failure for %d blocks...volume too large",nblks);
		display_message(cbufr);
		free_matrix_data(volume);
		return NULL;
	}

	if (!z_reform)
	{
	/* load slice data and compute extrema */
		for (i=0; i<zdim; i++)
    		{
			if (volume->zdim == 1)
				s1 = matrix_read_v2(matrix_file,slice[i].data.matnum,GENERIC, i+1);
			else s1 = matrix_read_slice(matrix_file, volume, i, foo);
			if (xdim != s1->xdim) 
				matrix_resize(s1,pixel_size,interp, align);
			scalef[i] = s1->scale_factor;
			switch (volume->data_type) {
			case ecat_matrix::MatrixDataType::ByteData : 
				b_p1 = (unsigned char *) s1->data_ptr;
				data_min = scalef[i]*find_bmin(b_p1,npixels);
				data_max = scalef[i]*find_bmax(b_p1,npixels);
				memcpy(b_vp,b_p1,npixels);
				b_vp += npixels;
				break;
			case ecat_matrix::MatrixDataType::VAX_Ix2:
			case ecat_matrix::MatrixDataType::SunShort:
				p1 = (short*)s1->data_ptr;
				data_min = scalef[i]*find_smin(p1,npixels);
				data_max = scalef[i]*find_smax(p1,npixels);
				memcpy(vp,p1,npixels*sizeof(short));
				vp += npixels;
				break;
			case ecat_matrix::MatrixDataType::IeeeFloat:
				f_p1 = (float*)s1->data_ptr;
				data_min = scalef[i]*find_fmin(f_p1,npixels);
				data_max = scalef[i]*find_fmax(f_p1,npixels);
				memcpy(f_vp,f_p1,npixels*sizeof(float));
				f_vp += npixels;
			}

/* update volume extrema */
			if (i==0) {
				volume->data_min = data_min;
				volume->data_max = data_max;
			} else {
				if (volume->data_min > data_min) volume->data_min = data_min;
				if (volume->data_max < data_max) volume->data_max = data_max;
			}
			free_matrix_data(s1);
//			if (progress_ctrl) progress_ctrl->StepIt();
    	}
	}
	else
	{
 /* z_reform flag : recompute slices */
		float *fp, *fdata;
		j = 1;
		if (volume->zdim == 1) {
			s1 = matrix_read_v2(matrix_file, slice[0].data.matnum, GENERIC, 1);
			s2 = matrix_read_v2(matrix_file, slice[1].data.matnum, GENERIC, 2);
		} else { /* reform volume stored image */
			s1 = matrix_read_slice(matrix_file, volume, 1, foo);
			s2 = matrix_read_slice(matrix_file, volume, 2, foo);
		}
		if (xdim != s1->xdim)  {
			matrix_resize(s1,pixel_size,interp, align);
			if (s2!=NULL) matrix_resize(s2,pixel_size,interp, align);
		}
		fdata = (float*)calloc(npixels,sizeof(float));
		for (i=0; i<zdim; i++)
		{
			zloc = slice[0].zloc + i*z_size;
			sprintf( cbufr, "Computing slice %d...(%0.2f cm)", i+1,zloc);
			display_message(cbufr);
//			if (progress_ctrl) progress_ctrl->StepIt();
			while (zloc > slice[j].zloc)
			{
				free_matrix_data(s1);
				s1 = s2;
				if (j < nslices-1) {
					if (volume->zdim == 1) s2 = matrix_read_v2(matrix_file,
						slice[++j].data.matnum, GENERIC, j);
					else s2 = matrix_read_slice(matrix_file, volume, ++j, foo);
					if (xdim != s2->xdim)
						matrix_resize(s2,pixel_size,interp, align);
				} else {
					s2 = NULL; /*	plane overflow */
					break;
				}
			}
/*
 * post condition : needed slice i position is between file slices j-1 and j
 * slice i position   start(i)=zloc-0.5*z_size,
 *                    end(i)=zloc+0.5*z_size
 * slice j-1 position start(j-1)=zloc(j-1)-0.5*thick(j-1),
 *                    end(j-1)=zloc(j-1)+0.5*thick(j-1)
 * slice j position   start(j)=zloc(j)-0.5*thick(j), end=zloc(j)+0.5*thick(j)
 * w1 = (zloc(j)-zloc)/(zloc(j)-zloc(j-1));
 * w2 = (zloc-zloc(j-1))/((zloc(j)-zloc(j-1));
 *
 */
			if (s2 != NULL) 
			{
				w1 = (slice[j].zloc - zloc)/(slice[j].zloc-slice[j-1].zloc);
				w1 = 0.01F*(int)(0.5 + w1*100);  /* round to .001*/
			}
			else w1 = 1.0F;
			w2 = 1.0F - w1;
#ifdef DEBUG
			printf("slice %d : z %g, thick %g = ",
				i, zloc, z_size);
			if (w1 > 0.0) printf("%g x slice %d ( z %g, thick %g ) + ",
				w1, j, slice[j-1].zloc, slice[j-1].data.z_size);
			if (w2 > 0.0) printf("%g x slice %d (z %g, thick %g)\n",
				w2, j+1, slice[j].zloc, slice[j].data.z_size);
#endif
			switch (volume->data_type) {
			case ecat_matrix::MatrixDataType::VAX_Ix2:
			case ecat_matrix::MatrixDataType::SunShort:
				if (w1>0.0) {
					p1 = (short*)s1->data_ptr;
					w1 *= s1->scale_factor;			/* pre-multiply w1 */
					for (k=0, fp = fdata; k<npixels; k++, fp++)
						*fp = w1 * (*p1++);
					w1 /= s1->scale_factor;			/* retrieve w1 */
				} else memset(fdata,0,npixels*sizeof(float)); 
				if (w2>0.0) {
					p1 = (short*)s2->data_ptr;
					w2 *= s2->scale_factor;            /* pre-multiply w2 */
					for (k=0, fp = fdata; k<npixels; k++, fp++)
						*fp += w2 * (*p1++);
					w2 /= s2->scale_factor;			/* retrieve w2 */
				}
				break;
			case ecat_matrix::MatrixDataType::ByteData :
				if (w1>0.0) {
					b_p1 = (unsigned char *)s1->data_ptr;
					w1 *= s1->scale_factor;			/* pre-multiply w1 */
					for (k=0, fp=fdata; k<npixels; k++, fp++)
						*fp = w1 * (*b_p1++);
					w1 /= s1->scale_factor;			/* retrieve w1 */
				} else memset(fdata,0,npixels*sizeof(float)); 
				if (w2>0.0) {
					b_p1 = (unsigned char *)s2->data_ptr;
					w2 *= s2->scale_factor;            /* pre-multiply w2 */
					for (k=0, fp = fdata; k<npixels; k++, fp++)
						*fp += w2 * (*b_p1++);
					w2 /= s2->scale_factor;			/* retrieve w2 */
				}
				break;
			case ecat_matrix::MatrixDataType::IeeeFloat :
				if (w1>0.0) {
					f_p1 = (float*)s1->data_ptr;
					w1 *= s1->scale_factor;			/* pre-multiply w1 */
					for (k=0, fp=fdata; k<npixels; k++, fp++)
						*fp = w1 * (*f_p1++);
					w1 /= s1->scale_factor;			/* retrieve w1 */
				} else memset(fdata,0,npixels*sizeof(float)); 
				if (w2>0.0) {
					b_p1 = (unsigned char *)s2->data_ptr;
					w2 *= s2->scale_factor;            /* pre-multiply w2 */
					for (k=0, fp = fdata; k<npixels; k++, fp++)
						*fp += w2 * (*f_p1++);
					w2 /= s2->scale_factor;			/* retrieve w2 */
				}
				break;
			}
		
/* compute slice scale factor and save short in volume */
			data_min = find_fmin(fdata,npixels);
			data_max = find_fmax(fdata,npixels);
			if (s2 != NULL) 
				scalef[i] = my_max(s1->scale_factor,s2->scale_factor);
			else scalef[i] = s1->scale_factor;
			w = 1.0F/scalef[i];	/* use inverse : multiply speed vs divide */
			switch (volume->data_type) {
			case ecat_matrix::MatrixDataType::VAX_Ix2:
			case ecat_matrix::MatrixDataType::SunShort:
				for (k=0, fp=fdata; k<npixels; k++, fp++)
					*vp++ = (int)((*fp)*w);
				break;
			case ecat_matrix::MatrixDataType::ByteData:
				for (k=0, fp=fdata; k<npixels; k++, fp++)
					*b_vp++ = (int)((*fp)*w);
				break;
			case ecat_matrix::MatrixDataType::IeeeFloat:
				for (k=0, fp=fdata; k<npixels; k++, fp++)
					*f_vp++ = (float)((*fp)*w);
				break;
			}

/* update volume extrema */
			if (i==0) {
				volume->data_min = data_min;
				volume->data_max = data_max;
			} else {
				if (volume->data_min > data_min) volume->data_min = data_min;
				if (volume->data_max < data_max) volume->data_max = data_max;
			}
		}
		free(fdata);
	}
/*
 * compute volume scale factor and rescale volume data
 */
	volume->scale_factor = scalef[0];
	for (i=1; i<zdim; i++)
		volume->scale_factor = my_max(volume->scale_factor,scalef[i]);
	vp = (short*)vdata;
	b_vp = (unsigned char *)vdata;
	f_vp = (float*)vdata;
	for (i=0; i<zdim; i++)
	{
		w = scalef[i]/volume->scale_factor;
		if (fabs(1-w) < 0.001) continue;
		switch (volume->data_type) {
		case ecat_matrix::MatrixDataType::VAX_Ix2:
		case ecat_matrix::MatrixDataType::SunShort:
			for (k=0; k<npixels; k++, vp++) *vp = (int)(w*(*vp));
			break;
		case ecat_matrix::MatrixDataType::ByteData:
			for (k=0; k<npixels; k++, b_vp++)
				*b_vp = (int)(w*(*b_vp));
			break;
		case ecat_matrix::MatrixDataType::IeeeFloat:
			for (k=0; k<npixels; k++, f_vp++)
				*f_vp = w*(*f_vp);
		break;
		default:
			fprintf(stderr,"unsupported data type : %d\n",volume->data_type);
      exit(1);
		}
	}
	volume->data_ptr = vdata;
	volume->data_size = nblks*ecat_matrix::MatBLKSIZE;
	if (xdim != volume->xdim) {
		volume->xdim = xdim;
		volume->ydim = ydim;
		volume->pixel_size = volume->y_size = pixel_size;
	}
	volume->zdim = zdim;
	volume->z_size = z_size;
	return volume;
}

ecat_matrix::MatrixData *load_volume_v2(/*CProgressCtrl *progress_ctrl,*/ ecat_matrix::MatrixFile *matrix_file,
						   int matnum, float pixel_size, int interp)
{
/*
 * Pre-condition : file_type =  PetImage|ecat_matrix::DataSetType::PetVolume|ecat_matrix::DataSetType::ByteVolume|InterfileImage
 */
	int i=0, ret=0;
	ecat_matrix::MatrixData *mat;
	float zsep, zmin, zmax, maxval;
	ecat_matrix::Main_header *mh;
	ecat_matrix::Image_subheader *imh = NULL;
	int nmats, plane, nslices=0;
	ecat_matrix::MatDirNode *entry;
	ecat_matrix::MatVal matval;
	Tslice *slice;
	ecat_matrix::MatrixData *volume;
	char buf[80];
	int frame=-1; /* frame -1 ==> load first frame in the file */
	int bed=-1;		/* bed -1 ==> load first bed in the file */

	mh = matrix_file->mhptr;
	if (mh->file_type != PetImage && mh->file_type!=ecat_matrix::DataSetType::PetVolume &&
		mh->file_type!=ecat_matrix::DataSetType::ByteVolume && mh->file_type!=InterfileImage &&
		mh->file_type!=ByteProjection && mh->file_type!=PetProjection) {
		fprintf(stderr,"unsupported file type %d\n",mh->file_type); 
		return NULL;
	}

	nmats = matrix_file->dirlist->nmats;
	entry = matrix_file->dirlist->first;
	maxval = 0.0;
	slice = (Tslice*)calloc(MAX_SLICES, sizeof(Tslice));
    if (matnum > 0)
    {
	   mat_numdoc( matnum, &matval);
       frame = matval.frame;
       bed = matval.bed;
    }
	for (i=0; i<nmats; i++,entry=entry->next)
	{
		matnum = entry->matnum;
		mat_numdoc( matnum, &matval);
		if (frame < 0) frame = matval.frame;
		if (bed < 0) bed = matval.bed;
		plane = matval.plane;
		if (matval.frame != frame || matval.bed != bed) continue;
		mat = matrix_read_v2( matrix_file, matnum, ecat_matrix::MatrixDataType::MAT_SUB_HEADER, plane);
		if (mat != NULL) {
/* if this slice has a different pixel size,
 * assume that the first loaded was the scout image
 */			if (nslices>0 &&
				 fabs(mat->pixel_size-slice[nslices-1].data.pixel_size)>0.0001) {
				if (nslices==1)	nslices--;	/* remove first slice */
				else {						/* reject this slice */
					free_matrix_data(mat);
					continue;
				}
			}
			imh = (ecat_matrix::Image_subheader*)mat->shptr;
			memcpy(&slice[nslices].data,mat, sizeof(ecat_matrix::MatrixData));
			slice[nslices].zloc = plane*mat->z_size;
			if (mat->dicom_header != NULL)
			{
				float x_location, y_location;
				if (dicom_scan_string(0x0020, 0x1041,buf,mat->dicom_header,
          			mat->dicom_header_size) &&
					sscanf(buf, "%g", &slice[nslices].zloc) == 1)
				{
					slice[nslices].zloc /= 10; /* mm to cm */
					slice[nslices].zloc *= -1 ; /*reverse orientation */
				}
				else if (dicom_scan_string(0x0020, 0x0032,buf,
					mat->dicom_header,mat->dicom_header_size) &&
					sscanf(buf, "%g\\%g\\%g", &x_location, &y_location,
					 &slice[nslices].zloc) == 3)
				{
					 	slice[nslices].zloc /= 10; /* mm to cm */
						slice[nslices].zloc *= -1 ; /*reverse orientation */
				}
			}

#ifdef DEBUG
			printf("slice %d : z location %g, thickness %g\n",
				nslices+1, slice[nslices].zloc, imh->z_pixel_size);
#endif
			free_matrix_data(mat);
			nslices++;
		} else {
			matrix_perror(matrix_file->fname);
			free_matrix_data(mat);
			free(slice);
			return NULL;
		}
	}
	
	if (nslices == 0) {
		fprintf( stderr, "ERROR...No slices selected\n");
		free(slice);
		return NULL;
	}

	zmin = zmax = slice[0].zloc;
	for (i=1; i< nslices; i++) {
		if (zmin>slice[i].zloc) zmin = slice[i].zloc;
		else if (zmax<slice[i].zloc) zmax = slice[i].zloc;
	}
	if ( (zmax-zmin) < 1.0) {
		/* z location not specified, use main header plane separation if
		common thickness */
		zsep = slice[0].data.z_size;
		for (i=0; i< nslices; i++) {
			if (slice[i].data.z_size != zsep) {
				fprintf( stderr, "R...slices z locations not specified for ");
				fprintf( stderr, "variable thickness slices\n");
				free(slice);	
				return NULL;
			}
			mat_numdoc( slice[i].data.matnum, &matval);
			slice[i].zloc = zsep*matval.plane;
		}
	}

//	if (progress_ctrl) progress_ctrl->SetRange(1,nslices);
//	if (progress_ctrl) progress_ctrl->SetStep(1);

	if (pixel_size>0.0005 || nslices > 1) {
/* reformate to requested pixel size or slice mode images */
		volume = load_slices(/*progress_ctrl, */matrix_file,slice,nslices,
			pixel_size, interp);
	} else {
/* use also load_slices to show loading progess
		return matrix_read(matrix_file, slice[0].data.matnum,GENERIC);
*/
		volume = load_slices(/*progress_ctrl,*/ matrix_file,slice,nslices,
			pixel_size, interp);
	}
	free(slice);
	if (volume == NULL) return NULL;

	if (volume->data_type == ecat_matrix::MatrixDataType::ByteData) volume->mat_type = ecat_matrix::DataSetType::ByteVolume;
	else volume->mat_type = ecat_matrix::DataSetType::PetVolume;
	volume->x_origin += 0.5F*(10.0f*volume->xdim*volume->pixel_size-1); // in mm
	volume->y_origin += 0.5F*(10.0f*volume->ydim*volume->y_size-1); // in mm
	volume->z_origin += 0.5F*(10.0f*volume->zdim*volume->z_size-1); // in mm
	volume->matnum = ecat_matrix::mat_numcod(frame, 1, 1, 0, bed);
	imh = (ecat_matrix::Image_subheader*)volume->shptr;
	imh->num_dimensions = 3;
	imh->scale_factor = volume->scale_factor;
	imh->x_dimension = volume->xdim;
	imh->y_dimension = volume->ydim;
	imh->z_dimension = volume->zdim;
	imh->x_pixel_size = volume->pixel_size;
	imh->y_pixel_size = volume->y_size;
	imh->z_pixel_size = volume->z_size;
	return volume;
}
