//static char sccsid[] = "%W% UCL-TOPO %E%";
/*
 * Modification History :
 * 5-jan-2000 : Replace ColorData by Color_8 and add Color_24 datatype
 * 16-may-2001 : Histogram
 *              - Use template for different data type support,
 *              - Sort voxel values in a 100 channels histogram and use the
 *              - and set background cut 50% probabilty for normalization
 */


#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "Volume.h"
#include "load_volume_v2.h"
#include "ecatx/analyze.h"
//#include <Lists/intlist.h>
//#include <Lists/charlist.h>
#include <algorithm>
#include <functional>
#include <vector>
#include "IsotropicSlicer.h"

using namespace std;

inline MatrixData* blank(int xdim=64, int ydim=64, int zdim=64) {
	MatrixData* ret = (MatrixData*)calloc(1,sizeof(MatrixData));
    ret->data_type = ByteData;
	ret->pixel_size = ret->y_size = ret->z_size = 2;
	ret->xdim = xdim; ret->ydim = ydim; ret->zdim = zdim;
	ret->x_origin = (float)xdim;
	ret->y_origin = (float)ydim;
	ret->z_origin = (float)zdim;
	ret->data_max = 1.0f;
	ret->scale_factor = 1.0f;
    ret->data_size = xdim*ydim*zdim;
    ret->data_ptr = (caddr_t)calloc(ret->data_size,1);
	return ret;
}

MatrixData *matrix_create(const MatrixData *orig)
{
	MatrixData *copy = (MatrixData*)calloc(1,sizeof(MatrixData));
	copy->data_type = orig->data_type;
	copy->scale_factor = orig->scale_factor;
	copy->xdim = orig->xdim;
	copy->ydim = orig->ydim;
	copy->zdim = orig->zdim;
	copy->pixel_size = orig->pixel_size;
	copy->y_size = orig->y_size;
	copy->z_size = orig->z_size;
	return copy;
}


template <class T> 
void t_histogram(T *src, int nvoxels, float dmin, float dmax, float dv,
				float *yv, int psize)
{
	int i, k;
	T v;
	for (i=0; i<nvoxels; i++) {
		v = *src++;
		if (v > dmin && v<dmax) {
			k = (int)((v-dmin)/dv);
			yv[k] += 1;
		}		
	}
}

static void color_histogram(u_char *rgb, int nvoxels, float *yv, int psize)
{
	int i, k;
	u_char *r=rgb, *g=rgb+nvoxels, *b=rgb+2*nvoxels;
	for (i=0; i<nvoxels; i++)
	{
		k = brightness<u_char>(*r++, *g++, *b++);
		if (k>0 && k<psize) yv[k] += 1;
	}
}

void Volume::apply_zoom(const VoxelCoord &center, float factor,
                      DimensionName orthogonal, float *x, float *y, int count)
{
  int i=0;
  switch(orthogonal) {
  case Dimension_X :
    for (i=0; i<count; i++)
    {
      x[i] = center.y + (x[i]-center.y)*factor;
      y[i] = center.z + (y[i]-center.z)*factor;
    }
    break;
  case Dimension_Y :
    for (i=0; i<count; i++)
    {
      x[i] = center.x + (x[i]-center.x)*factor;
      y[i] = center.z + (y[i]-center.z)*factor;
    }
    break;
  case Dimension_Z :
    for (i=0; i<count; i++)
    {
      x[i] = center.x + (x[i]-center.x)*factor;
      y[i] = center.y + (y[i]-center.y)*factor;
    }
  }
}

void Volume::invert_zoom(const VoxelCoord &center, float factor,
                       DimensionName orthogonal, float *x, float *y, int count)
{
  int i=0;
  switch(orthogonal) {
  case Dimension_X :
    for (i=0; i<count; i++)
    {
      x[i] = center.y + (x[i]-center.y)/factor;
      y[i] = center.z + (y[i]-center.z)/factor;
    }
    break;
  case Dimension_Y :
    for (i=0; i<count; i++)
    {
      x[i] = center.x + (x[i]-center.x)/factor;
      y[i] = center.z + (y[i]-center.z)/factor;
    }
    break;
  case Dimension_Z :
    for (i=0; i<count; i++)
    {
      x[i] = center.x + (x[i]-center.x)/factor;
      y[i] = center.y + (y[i]-center.y)/factor;
    }
  }
}

MatrixData *Volume::rgb_split_data(MatrixData *src , int segment) const
{
    int nvoxels = src->xdim*src->ydim*src->zdim;
	MatrixData *dest = (MatrixData*)calloc(1,sizeof(MatrixData));
	memcpy(dest,src,sizeof(MatrixData));
	dest->data_ptr = src->data_ptr + segment*nvoxels;
	dest->data_type = ByteData;
	return dest;
}
	
MatrixData *Volume::rgb_create_data(MatrixData *r, MatrixData *g ,
                                    MatrixData *b) const
{
	MatrixData *ret = (MatrixData*)calloc(1,sizeof(MatrixData));;
	ret->data_type = Color_24;
	ret->xdim = r->xdim;
	ret->ydim = r->ydim;
	ret->zdim = r->zdim;
	ret->pixel_size = r->pixel_size;
	ret->y_size = r->y_size;
	ret->z_size = r->z_size;
	ret->scale_factor = 1;
    int nvoxels = r->xdim*r->ydim*r->zdim;
	ret->data_ptr = (caddr_t)calloc(3,nvoxels);
    memcpy(ret->data_ptr, r->data_ptr, nvoxels);
    memcpy(ret->data_ptr+nvoxels, g->data_ptr, nvoxels);
    memcpy(ret->data_ptr+2*nvoxels, b->data_ptr, nvoxels);
    return ret;
}

float VoxelCoord::get(DimensionName dimension) const
{
  if (dimension == Dimension_X) return x;
  if (dimension == Dimension_Y) return y;
  return z;
}

float VoxelCoord::get(DimensionName dimension, float &ax, float &ay) const
{
  ax = x; ay = y;
  if (dimension == Dimension_X) { ax = y; ay = z; }
  if (dimension == Dimension_Y) { ay = z; }
  return 1;
}

int VoxelCoord::set(DimensionName dimension, float v)
{
  char bak = undefined;
  undefined=0;
  if (dimension == Dimension_X) { x=v; return 1;}
  if (dimension == Dimension_Y) { y=v; return 1;}
  if (dimension == Dimension_Z) { z=v; return 1;}
  undefined=bak;
  return 0;
}

int VoxelCoord::set(DimensionName dimension, float vx, float vy)
{
  char bak = undefined;
  undefined=0;
  if (dimension == Dimension_X)  { y=vx; z=vy; return 1; }
  if (dimension == Dimension_Y) { x=vx; z=vy; return 1; }
  if (dimension == Dimension_Z) { x=vx; y=vy; return 1; }
  undefined=bak;
  return 0;
}
int  VoxelCoord::equal(const VoxelCoord v) const 
{
	float d = (v.x-x)*(v.x-x) + (v.y-y)*(v.y-y) + (v.z-z)*(v.z-z);
    return (sqrt(d)<0.1);
}

int VoxelIndex::get(DimensionName dimension) const
{
  if (dimension == Dimension_X) return x;
  if (dimension == Dimension_Y) return y;
  return z;
}

int VoxelIndex::get(DimensionName dimension, int &ax, int &ay) const
{
  ax = x; ay = y;
  if (dimension == Dimension_X) { ax = y; ay = z; }
  if (dimension == Dimension_Y) { ay = z; }
  return 1;
}

int VoxelIndex::set(DimensionName dimension, int v)
{
  if (dimension == Dimension_X) { x=v; return 1;}
  if (dimension == Dimension_Y) { y=v; return 1;}
  if (dimension == Dimension_Z) { z=v; return 1;}
  return 0;
}

int VoxelIndex::set(DimensionName dimension, int vx, int vy)
{
  if (dimension == Dimension_X)  { y=vx; z=vy; return 1; }
  if (dimension == Dimension_Y) { x=vx; z=vy; return 1; }
  if (dimension == Dimension_Z) { x=vx; y=vy; return 1; }
  return 0;
}

int Volume::max_byte_value = 248; // 248 for image and 8 for overlay
float Volume::min_pixel_size = (float)0.1;  /* mm */

ColorConverter *Volume::color_converter = 0;
static void normalize(float *yv, int size)
{
	int i;
	float totcount=0;
	float bg=0, bg_cut=0;
	vector<float> vec;
    for (i=0; i<size; i++) {
		vec.push_back(yv[i]);
		totcount += yv[i];
	}
	sort(vec.begin(), vec.end(), greater<float>()); // sort in decreasing order
	for (i=0; i<size; i++) {
		bg_cut = vec[i];
		bg += bg_cut;
		if (bg>0.5*totcount) break;
	}
	if (i==size) bg_cut = 0; // no background
	float percent_scale = 100.f/totcount;
	for (i=0; i<size; i++) {
		if (yv[i] > bg_cut) yv[i]= bg_cut*percent_scale;
		else yv[i] = yv[i]*percent_scale;
	}
}
//int unit_names(CharList& names, IntList& ids);

int Volume::unit_names(map<int,char*> &names) {
	unsigned i, convertible = 0;
	float dummy;
	names.clear();
	for (i=1; i <= 8; i = (i<<1)) { /* 1, 2, 4, 8 */
		if (_data_unit == i) convertible = 1;
	}
	if (convertible) {
		for (i=1; i <= 8; i = (i<<1)) {
			if (!conversion_factor(i, dummy)) continue;
			names.insert(make_pair(i,unit_name(i)));
		}
	}
	if (_data_unit == 5 || _data_unit == 6) {
		names.insert(make_pair(5,unit_name(5)));
		names.insert(make_pair(6,unit_name(5)));
	}
	return (int)(names.size());
}

int Volume::set_unit(unsigned unit)
{
	float f;
	if (conversion_factor(unit, f))
		_user_unit = unit;
	else return 0;
	return 1;
}
	
char* Volume::unit_name(unsigned unit)
{
    switch(unit)
    {
        case SUV_BW : return "SUV BW";
        case SUV_BSA : return "SUV BSA";
        case SUV_LEAN : return "SUV LEAN";
		case CM_1 : return "1/cm";
		case HOUNSFIELD : return "HU";
        default:
			if (strlen(main_header.data_units) > 0)
			return main_header.data_units; 
        	//return customDisplayUnits[unit];
    }
}

int Volume::convert(float data_value, float &user_value)
{
    float low=data()->data_min + data()->intercept;
	float high=data()->data_max + data()->intercept;
	// allow up to 2*high
    if (data_value<low || data_value>2*high) return 0;
    switch(_user_unit)
    {
        case SUV_BW:
        case SUV_BSA:
        case SUV_LEAN:
        case HOUNSFIELD:
            user_value = data_value*conversion_factor();
            break;
        default:
            user_value = data_value*100/high;
            break;
    }
    return 1;
}


int Volume::conversion_factor(unsigned unit, float &ret_factor) const
// unit : CPS, BQ_ML, UCI_ML, SUV_BW, SUV_BSA, SUV_LEAN
{
	// We suppose the ecf is in Bq/ml
	float factor = 1.0;
	float bsa_dosage=0.0, lean_dosage=0.0, dose_decay=1.0, lean=0.0;
	int dose_delay = 0;

	float cps_2_bq = main_header.calibration_factor;
	if (cps_2_bq <= 0.0) cps_2_bq = 1.0;
	float bq_2_uci = (float)(1.0/37000.0);
	float cps_2_uci = cps_2_bq*bq_2_uci;
	char sex = *main_header.patient_sex; /* 0:Male 1:Female 2:Unknow */
	float weight = main_header.patient_weight; /* kg */
	float height = main_header.patient_height; /* kg */
	float dosage = main_header.dosage; /* UCi */
	if ((sex==1 || sex=='F') && height > 0.0 && weight > 0.0)
		lean = 1.07f*weight-148.0f*(weight*weight/(height*height));
	if ((sex==0 || sex == 'M') && height > 0.0 && weight > 0.0)
		lean = 1.1f*weight-128.0f*(weight*weight/(height*height));
	if (main_header.dose_start_time > 0)
		dose_delay = main_header.scan_start_time -
			main_header.dose_start_time;
	if (main_header.isotope_halflife > 0 && dose_delay > 0) {
		float lamda = log(2.0f)/main_header.isotope_halflife;
		dose_decay = exp(-lamda*dose_delay);
	}
	dosage = dosage*dose_decay;
	if (weight>0.0 && dosage>0.0 && height>0.0) {
		float bsa = (float)(pow(weight,(float)0.425)*pow(height,(float)0.725)*0.20247);
		bsa_dosage = dosage /bsa;
	}
	if (dosage>0.0 && height>0.0 && lean >0.0)
		lean_dosage = dosage / lean;
	if (weight>0.0 && dosage>0.0) dosage = dosage/weight;
	else dosage = 0.0;

	// Convert Volume units in Bq/ml
	switch(data_unit())
	{
	case 1:	// cps
		factor = cps_2_bq;
		break;
	case 4: // UCi/ml
		factor = 1.0f/bq_2_uci;
		break;
	case 2:	// Bq/ml
		factor = 1.0f;
		break;
	default:
		return 0;
	}
	// Convert Bq/ml to requested units
	switch(unit)
	{
	case CPS:  factor /= cps_2_bq; break;
	case BQ_ML: break;
	case UCI_ML: factor *= bq_2_uci; break;
	case SUV_BW :
		if (dosage>0.0f) factor *= bq_2_uci/dosage;
		else return 0;
		break;
	case SUV_BSA:
		if (bsa_dosage>0.0f) factor *= bq_2_uci/bsa_dosage;
		else return 0;
		break;
	case SUV_LEAN:
		if (lean_dosage>0.0f) factor *= bq_2_uci/lean_dosage;
		else return 0;
		break;
	default:
		return 0;
	}
	ret_factor = factor;
	return 1;
}

Volume::Volume(int sz_x, int sz_y, int sz_z) {
	proj_min = dyn_min = 0;
	proj_max = dyn_max = 100;
	memset(_name,0,IMAGENAME_MAX+1);
    red_data = green_data = blue_data = 0;
	_data = blank(sz_x, sz_y, sz_z);
	_atlas_origin = 0;
	_user_unit = _data_unit = 0;
	_transformer = NULL;
	_histogram.code = 0;
	_modality = UnknownModality;
	_histogram.x = _histogram.y = NULL;
	memset(&main_header, 0, sizeof(Main_header));
}

Volume::Volume(Main_header *mh, MatrixData* matrix, const char* name_str) {
	proj_min = dyn_min = 0;
	proj_max = dyn_max = 100;
	memset(_name,0,IMAGENAME_MAX+1);
	if (name_str && strlen(name_str)) 
		strncpy(_name,name_str,IMAGENAME_MAX);
	_user_unit = _data_unit = 0;
	_data = matrix;
	_atlas_origin = 0;
	_transformer = NULL;
	_histogram.code = 0;
	_histogram.x = _histogram.y = NULL;
    red_data = green_data = blue_data = 0;
	if (mh)	{
		float unit_factor=1.0;
		char *magic_number = mh->magic_number;
		int magic_size = sizeof(mh->magic_number);
		memcpy(&main_header, mh, sizeof(Main_header));
		_modality = modality_code(magic_number+magic_size-4);
		switch(main_header.calibration_units) {
		default:
		case Uncalibrated :
			_data_unit = 0; /* none */
			break;
		case Calibrated :
		case Processed :
			_data_unit = max(0,(int)main_header.calibration_units_label);
			_data_unit = min(_data_unit,numDisplayUnits);
		break;
		}
		_user_unit = _data_unit;
		if (_modality == CT)
			_user_unit = HOUNSFIELD;
		else
		{
			if (conversion_factor(SUV_BW, unit_factor)) _user_unit = SUV_BW;
			else if (conversion_factor(BQ_ML, unit_factor)) _user_unit = BQ_ML;
			else if (conversion_factor(CPS, unit_factor)) _user_unit = CPS;
		}
	}
	else {
		memset(&main_header, 0, sizeof(Main_header));
		_modality = UnknownModality;
	}
}

Volume::Volume(MatrixFile* matfile, int matnum, int segment) {
	memcpy(&main_header, matfile->mhptr, sizeof(Main_header));
	memset(_name,0,IMAGENAME_MAX+1);
	const char* fname = strrchr(matfile->fname,'/');
	if (fname != NULL) fname++;
	else fname = matfile->fname;
	strncpy(_name,fname,IMAGENAME_MAX);
	_data = blank();
	_user_unit = _data_unit = 0;
	proj_min = dyn_min = 0;
	proj_max = dyn_max = 100;
	_transformer = NULL;
	_histogram.x = _histogram.y = NULL;
	_modality = UnknownModality;
	red_data = green_data = blue_data = 0;
	try { load(matfile,matnum, segment); }
	catch(...) { throw; }
}
//extern "C" MatrixData *matrix_read_scan(MatrixFile*, int, int, int);

int Volume::matnum(MatrixFile* matfile,int frame)
{
	struct Matval matval;
	int matnum=0;
	MatDirNode *node = matfile->dirlist->first;
	while (node && (matnum==0)) {
		mat_numdoc(node->matnum,&matval);
		if (matval.frame == frame) matnum = node->matnum;
		node = node->next;
	}
	return matnum;
}

MatrixData *ColorConverter::load(MatrixFile *file, MatrixData *vheader)
{
// Creates display pixels volume
	u_char r,g,b;
	u_char bt, bt_max=0;
	MatrixData *volume = (MatrixData*)calloc(1,sizeof(MatrixData));
	memcpy(volume,vheader,sizeof(MatrixData));
	volume->shptr = 0;
	volume->dicom_header = 0;
	volume->dicom_header_size = 0;
	int npixels = volume->xdim*volume->ydim;
	int nvoxels = npixels*volume->zdim;
    if (_bytes_per_pixel < 3)
    {
        volume->data_type = ByteData; 
        volume->data_size = nvoxels;
        volume->data_ptr = (caddr_t)calloc(1,nvoxels);
    }
    else
    {
       volume->data_type = Color_24;
       volume->data_size = nvoxels*sizeof(unsigned);
       volume->data_ptr = (caddr_t)calloc(3,nvoxels);
    }
	u_char *p = (u_char*)volume->data_ptr;
	u_char *p_red = (u_char*)volume->data_ptr;
	u_char *p_green = (u_char*)volume->data_ptr + nvoxels;
	u_char *p_blue = (u_char*)volume->data_ptr + 2*nvoxels;
	for (int plane=1; plane<=volume->zdim; plane++)
	{
		MatrixData *slice = matrix_read_slice(file,vheader, plane,0);
		assert((slice != NULL));
		u_char *p0 = (u_char*)slice->data_ptr;
		for (int i=0; i<npixels; i++)
		{
			r = *p0++;
			g = *p0++;
			b = *p0++;
            bt = brightness<u_char>(r,g,b);
			if (bt_max < bt) bt_max = bt;
            if (_bytes_per_pixel < 3) *p++ = bt;
            else
			{
				*p_red++ = r; 
				*p_green++ = g; 
				*p_blue++ = b; 
			}
		}
		free_matrix_data(slice);
	}
    volume->scale_factor = 1;
	volume->data_max = bt_max;
	return volume;
}

MatrixData *matrix_brightness(const MatrixData *rgb)
{
	u_char *B, *r, *g, *b;
	if (rgb->data_type != Color_24) return 0;
	MatrixData *ret = matrix_create(rgb);
	ret->data_type = ByteData;
	int nvoxels = ret->xdim*ret->ydim*ret->zdim;
	ret->data_ptr = (caddr_t)calloc(1, nvoxels);
	r = (u_char*)rgb->data_ptr; g = r+nvoxels; b = g+nvoxels;
	B = (u_char*)ret->data_ptr;
	for (int i=0; i<nvoxels; i++)
		*B++ = brightness<u_char>(*r++, *g++, *b++);
	return ret;
}

template <class T>
void  matrix_align_4(T *src, MatrixData *data)
{
	int x, y, z, xtrail, ytrail, xdim, ydim;
	int sx = data->xdim, sy = data->ydim, sz = data->zdim;
	T  *dest;
	dest = (T*)data->data_ptr;
	xtrail = sx%4; ytrail = sy%4;		// 4 pixels aligned trailing borders
	if (xtrail==0 && ytrail==0) return;
	xdim = sx-xtrail; ydim = sy-ytrail;	// ignore trailings
	for (z=0; z<sz; z++)
	{
		for (y=0; y<ydim; y++)
		{
			if (dest != src)
			{
				for (x=0; x<xdim; x++)
					*dest++ = *src++;
				src += xtrail;
			}
			else
			{
				dest += xdim;
				src += sx;
			}
		}
		src += ytrail*sx;
	}
	data->xdim = xdim;
	data->ydim = ydim;
}

static void  matrix_align_color_4(MatrixData *data)
{
	int x, y, z, xtrail, ytrail, xdim, ydim;
	int sx = data->xdim, sy = data->ydim, sz = data->zdim;
	u_char *src, *dest;
	src = (u_char*)data->data_ptr;
	dest = (u_char*)data->data_ptr;
	xtrail = sx%4; ytrail = sy%4;		// 4 pixels aligned trailing borders
	if (xtrail==0 && ytrail==0) return;
	xdim = sx-xtrail; ydim = sy-ytrail;	// ignore trailings
	for (z=0; z<sz; z++)
	{
		for (y=0; y<ydim; y++)
		{
			if (dest != src)
			{
				for (x=0; x<xdim; x++)
				{
					*dest++ = *src++;		// r
					*dest++ = *src++;		// g
					*dest++ = *src++;		// b
				}
				src += 3*xtrail;
			}
			else
			{
				dest += 3*xdim;
				src += 3*sx;
			}
		}
		src += 3*ytrail*sx;
	}
	data->xdim = xdim;
	data->ydim = ydim;
}

int Volume::load(MatrixFile* matfile,int matnum, int segment) {
	MatrixData *new_data=NULL;
	int _min, _max;
	char **ifh = matfile->interfile_header;
	float unit_factor=1.0;
	float pixel_size = 0;	// load using image pixel size 
	int interp = 1;			// get nearest neighbour when pixel size is changed
                            // on loading

	MatrixData *new_header = matrix_read(matfile,matnum,MAT_SUB_HEADER);
	if (new_header == NULL) return 0;
// attention new_header->pixel_size in cm
	if (new_header->pixel_size < min_pixel_size/10)
		pixel_size = min_pixel_size/10;
	memcpy(&main_header,matfile->mhptr, sizeof(Main_header));
	if (main_header.file_type == Short3dSinogram || 
		main_header.file_type == Float3dSinogram ||
		main_header.file_type == AttenCor)
		new_data = matrix_read_scan(matfile,matnum,GENERIC,segment);
	else if (new_header->data_type == Color_24) {
		if (!color_converter) color_converter = new ColorConverter();
		new_data = color_converter->load(matfile, new_header);
	} else try {
		 new_data = load_volume_v2(matfile, matnum, pixel_size, interp);
	} catch(...) { throw; }
// Decode modality from magic number
    char *magic_number = main_header.magic_number;
    int modality_pos = sizeof(main_header.magic_number)-4;
	if (strncmp(magic_number+modality_pos,"CT",2) == 0) _modality = CT;
	if (strncmp(magic_number+modality_pos,"MR",2) == 0) _modality = MR;
	if (strncmp(magic_number+modality_pos,"NM",2) == 0) _modality = NM;
	if (strncmp(magic_number+modality_pos,"PET",3) == 0) _modality = PET;
	free_matrix_data(new_header);
	if (new_data == NULL) return 0;
	free_matrix_data(_data); _data = new_data;
// set lentgh units from cm to mm
	_data->pixel_size *= 10; _data->y_size *= 10;
	if (main_header.file_type != PetProjection &&
		main_header.file_type != ByteProjection) _data->z_size *= 10; 
//  else z_size in degrees
	_data->x_origin *= 10; _data->y_origin *= 10; _data->z_origin *= 10;
	_origin.x = _data->x_origin;
	_origin.y = _data->y_origin;
	_origin.z = _data->z_origin;

// check that voxel dimensions are specified */
	if (_data->pixel_size == 0) _data->pixel_size = 1.0;
	if (_data->y_size == 0) _data->y_size = _data->pixel_size;
	if (_data->z_size == 0) _data->z_size = _data->pixel_size;

	if (_modality == CT)
	{
		// ignore hot/cold  points
		if (_data->data_max > 2000) _data->data_max = 2000; 
		if (_data->data_min < -1000) _data->data_min = -1000; 
	}

	switch(main_header.calibration_units) {
		default:
		case Uncalibrated :
			_data_unit = 0; /* none */
			break;
		case Calibrated :
		case Processed :
			_data_unit = max(0,(int)main_header.calibration_units_label);
			_data_unit = min(_data_unit,numDisplayUnits);
		break;
	}
	_user_unit = _data_unit;
	if (_modality == CT)
		_user_unit = HOUNSFIELD;
	else
	{
		if (conversion_factor(SUV_BW, unit_factor)) _user_unit = SUV_BW;
		else if (conversion_factor(BQ_ML, unit_factor)) _user_unit = BQ_ML;
			else if (conversion_factor(CPS, unit_factor)) _user_unit = CPS;
	}

	if (ifh && ifh[DISPLAY_RANGE] &&
		sscanf(ifh[DISPLAY_RANGE],"%d %d",&_min,&_max) == 2) {
		int image_max = (int)(_data->data_max/_data->scale_factor);
		dyn_min = _min*100/image_max;
		dyn_max = _max*100/image_max;
	}
	if (ifh && ifh[ATLAS_ORIGIN_1]) _atlas_origin = 1; // atlas origin defined
	else _atlas_origin = 0;
	if (ifh && ifh[TRANSFORMER]) transform(ifh[TRANSFORMER]);
	_histogram.code = 0;		// invalid current histogram;

	if (_data->data_type == Color_24) {
		if (red_data) free(red_data);
		if (green_data) free(green_data);
		if (blue_data) free(blue_data);
		red_data = rgb_split_data(_data, 0);
		green_data = rgb_split_data(_data, 1);
		blue_data = rgb_split_data(_data, 2);
	}
	return 1;
}

void  Volume::update_origin() {
	float x0 = _data->x_origin, y0 = _data->y_origin, z0 = _data->z_origin;
	if (_transformer) {
		float *tm = _transformer->data;
		_origin.x = tm[0]*x0 + tm[1]*y0 + tm[2]*z0 + tm[3];
		_origin.y = tm[4]*x0 + tm[5]*y0 + tm[6]*z0 + tm[7];
		_origin.z = tm[8]*x0 + tm[9]*y0 + tm[10]*z0 + tm[11];
	} else {
		_origin.x = x0;
		_origin.y = y0;
		_origin.z = z0;
	}
}

void Volume::set_origin(const VoxelCoord& pos, const char* spm_hdr)
// set new origin
{
	FILE *fp;
	struct dsr hdr;
	short spm_origin[3];
	_data->x_origin = pos.x;
	_data->y_origin = pos.y;
	_data->z_origin = pos.z;
	update_origin();
if (spm_hdr) fprintf(stderr, "set origin %s\n",spm_hdr);
else fprintf(stderr, "set origin\n");
	if (spm_hdr && (fp=fopen(spm_hdr,"r")) &&
		fread(&hdr,sizeof(struct dsr),1,fp) == 1) {
		fclose(fp);
		spm_origin[0] = (int)(0.5+_data->x_origin/_data->pixel_size);
		spm_origin[1] = (int)(0.5+_data->y_origin/_data->y_size);
		spm_origin[2] = (int)(0.5+_data->z_origin/_data->z_size);
		spm_origin[0] = _data->xdim - spm_origin[0] -1;
		spm_origin[1] = _data->ydim - spm_origin[1] - 1;
		spm_origin[2] = _data->zdim - spm_origin[2] - 1;
		SWAB((char*)spm_origin,hdr.hist.originator,6); // convert to bigendian order
		if ((fp=fopen(spm_hdr,"w")) != NULL) {
			fwrite(&hdr,sizeof(struct dsr),1,fp);
			fclose(fp);
		}
	}
}

MatrixData* Volume::average(float abs_z, float thickness,
	float pixel_size, int interpolate) const
{
  float z = z_index(abs_z);
  switch(_data->data_type)
    {
    case ByteData:
      {
        IsotropicSlicer<u_char> slicer1(_data, _transformer, interpolate);
        return slicer1.average(z, thickness, pixel_size);
      }
    case SunShort:
    case VAX_Ix2:
      {
        IsotropicSlicer<short> slicer2(_data, _transformer, interpolate);
        return slicer2.average(z, thickness, pixel_size);
      }
    case UShort_BE:
    case UShort_LE:
      {
        IsotropicSlicer<unsigned short> slicer2(_data, _transformer, interpolate);
        return slicer2.average(z, thickness, pixel_size);
      }
    case IeeeFloat:
      {
        IsotropicSlicer<float> slicer4(_data, _transformer, interpolate);
        return slicer4.average(z, thickness, pixel_size);
      }
   default:
	throw("bad data type");
   }
}

Volume* Volume::clone() const 
{
  float voxel_size = _data->pixel_size;
  int sx = (int)(0.5 + _data->xdim*_data->pixel_size/voxel_size);
  int sy = (int)(0.5 + _data->ydim*_data->y_size/voxel_size);
  int sz = (int)(0.5 + _data->zdim*_data->z_size/voxel_size);
  return new Volume(sx,sy,sz);
}

const MatrixData &Volume::get_slice_header(DimensionName orthogonal) const
{
  static MatrixData *sheader=NULL;
  if (sheader == 0) sheader = (MatrixData*)calloc(1,sizeof(MatrixData));
  float sx = _data->xdim*_data->pixel_size;
  float sy = _data->ydim*_data->y_size;
  float sz = _data->zdim*_data->z_size;
  sheader->zdim = 1;
  switch(orthogonal) {
  case Dimension_X :
    sheader->xdim = _data->ydim;  sheader->pixel_size = _data->y_size;
    sheader->ydim = _data->zdim;  sheader->y_size = _data->z_size;
    sheader->z_size = _data->pixel_size;
    break;
  case Dimension_Y :
    sheader->xdim = _data->xdim;  sheader->pixel_size = _data->pixel_size;
    sheader->ydim = _data->zdim;  sheader->y_size = _data->z_size;
    sheader->z_size = _data->y_size;
    break;
  case Dimension_Z :
    sheader->xdim = _data->xdim;  sheader->pixel_size = _data->pixel_size;
    sheader->ydim = _data->ydim;  sheader->y_size = _data->y_size;
    sheader->z_size = _data->z_size;
  }
  return *sheader;
}

MatrixData* Volume::get_slice(DimensionName dimension, float pos,
                 int interpolate) const
{
  VoxelCoord area(_data->xdim*_data->pixel_size, _data->ydim*_data->y_size,
                  _data->zdim*_data->z_size);
  float pix_size = min(min(_data->pixel_size,_data->y_size), _data->z_size);
  return get_slice(dimension, pos, area, pix_size, interpolate);
}

MatrixData* Volume::get_slice(
                 DimensionName dimension, float pos,
                 VoxelCoord &area, float pixel_size, int interpolate) const
{
  MatrixData *ret = NULL;
  Matrix t = mat_alloc(4,4);
  if (_transformer) mat_copy(t,_transformer);
  if (!area.undefined)
  {
/*
   float tx =  0.5*(area.x - _data->xdim*_data->pixel_size);
    float ty =  0.5*(area.y - _data->ydim*_data->y_size);
    float tz =  0.5*(area.z - _data->zdim*_data->z_size);
    // center in requested area
    if ((fabs(tx) + fabs(ty) + fabs(tz)) > 0.5)
	  {
	  	TRACE2("x_offset,y_offset=%g,%g\n",t->data[3], t->data[7]);
      scale(t, _data->pixel_size, _data->y_size, _data->z_size);
      translate(t, -tx, -ty, -tz);
      scale(t, 1.0/_data->pixel_size, 1.0/_data->y_size, 1.0/_data->z_size);
	 }
*/
  }
  switch(_data->data_type)
    {
    case ByteData:
    case Color_8:
      {
        IsotropicSlicer<u_char> slicer1(_data, t, interpolate);
        ret =  slicer1.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
	  break;
    case SunShort:
    case VAX_Ix2:
      {
        IsotropicSlicer<short> slicer2(_data, t, interpolate);
        ret = slicer2.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
      break;
    case UShort_BE:
    case UShort_LE:
      {
        IsotropicSlicer<unsigned short> slicer2(_data, t, interpolate);
        ret = slicer2.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
      break;
    case Color_24:
      {
        MatrixData *red_slice, *green_slice, *blue_slice, *rgb;
        IsotropicSlicer<u_char> r_slicer(red_data, t, interpolate);
        IsotropicSlicer<u_char> g_slicer(green_data,t,interpolate);
        IsotropicSlicer<u_char> b_slicer(blue_data, t,interpolate);
		red_slice = r_slicer.slice(dimension, pos, area.x, area.y,area.z,pixel_size);
		green_slice = g_slicer.slice(dimension, pos,area.x,area.y,area.z,pixel_size);
		blue_slice = b_slicer.slice(dimension, pos, area.x, area.y,area.z,pixel_size);
		rgb = rgb_create_data(red_slice,green_slice,blue_slice);
		free_matrix_data(red_slice);
		free_matrix_data(green_slice);
		free_matrix_data(blue_slice);
        ret = rgb;
      }
      break;
    case IeeeFloat:
      {
        IsotropicSlicer<float> slicer4(_data, t, interpolate);
        ret = slicer4.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
      break;
   default:
    mat_free(t);
	throw("bad data type");
   }
   mat_free(t);
   return ret;
}

MatrixData* Volume::get_slice(
                 const VoxelCoord &zcenter, float zfactor,
                 DimensionName dimension, float pos,
                 VoxelCoord &area, float pixel_size, int interpolate) const
{
  MatrixData *ret = NULL;
  Matrix t = mat_alloc(4,4);
  if (_transformer) mat_copy(t,_transformer);
  if (!area.undefined)
  {
if (fabs(area.x-area.y)) {
printf("RRRRRRRRRRRRRRRr transverse area = (%g,%g)\n",area.x, area.y);
}
    float tx =  0.5*(area.x - _data->xdim*_data->pixel_size);
    float ty =  0.5*(area.y - _data->ydim*_data->y_size);
    float tz =  0.5*(area.z - _data->zdim*_data->z_size);
    // center in requested area
    if ((fabs(tx) + fabs(ty) + fabs(tz)) > 0.5)
	{
      scale(t, _data->pixel_size, _data->y_size, _data->z_size);
      translate(t, -tx, -ty, -tz);
      scale(t, 1.0/_data->pixel_size, 1.0/_data->y_size, 1.0/_data->z_size);
	}
  }
  if (zfactor > 1)
  {
	float *tm = t->data;
    float xi = x_index(zcenter.x);
    float yi = y_index(zcenter.y);
    float zi = z_index(zcenter.z);
    float zx = tm[0]*xi + tm[1]*yi + tm[2]*zi + tm[3];
    float zy = tm[4]*xi + tm[5]*yi + tm[6]*zi + tm[7];
    float zz = tm[8]*xi + tm[9]*yi + tm[10]*zi + tm[11];
    ::translate(t,-zx,-zy,-zz);
    scale(t, 1.0/zfactor, 1.0/zfactor, 1.0/zfactor);
    // restore origin
    ::translate(t,zx,zy,zz);
    switch(dimension)
    {
    case Dimension_X : pos = zcenter.x + (pos-zcenter.x)*zfactor; break;
    case Dimension_Y : pos = zcenter.y + (pos-zcenter.y)*zfactor; break;
    case Dimension_Z : pos = zcenter.z + (pos-zcenter.z)*zfactor; break;
    }
  }
  switch(_data->data_type)
    {
    case ByteData:
    case Color_8:
      {
        IsotropicSlicer<u_char> slicer1(_data, t, interpolate);
        ret =  slicer1.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
	  break;
    case SunShort:
    case VAX_Ix2:
      {
        IsotropicSlicer<short> slicer2(_data, t, interpolate);
        ret = slicer2.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
      break;
    case UShort_BE:
    case UShort_LE:
		{
			IsotropicSlicer<unsigned short> slicer2(_data, t, interpolate);
			ret = slicer2.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
		}
		break;
    case Color_24:
      {
        MatrixData *red_slice, *green_slice, *blue_slice, *rgb;
        IsotropicSlicer<u_char> r_slicer(red_data, t, interpolate);
        IsotropicSlicer<u_char> g_slicer(green_data,t,interpolate);
        IsotropicSlicer<u_char> b_slicer(blue_data, t,interpolate);
		red_slice = r_slicer.slice(dimension, pos, area.x, area.y,area.z,pixel_size);
		green_slice = g_slicer.slice(dimension, pos,area.x,area.y,area.z,pixel_size);
		blue_slice = b_slicer.slice(dimension, pos, area.x, area.y,area.z,pixel_size);
		rgb = rgb_create_data(red_slice,green_slice,blue_slice);
		free_matrix_data(red_slice);
		free_matrix_data(green_slice);
		free_matrix_data(blue_slice);
        ret = rgb;
      }
      break;
    case IeeeFloat:
      {
        IsotropicSlicer<float> slicer4(_data, t, interpolate);
        ret = slicer4.slice(dimension, pos, area.x, area.y, area.z, pixel_size);
      }
      break;
   default:
    mat_free(t);
	throw("bad data type");
   }
   mat_free(t);
   return ret;
}

MatrixData* Volume::projection(DimensionName dimension, float abs_l,
                               float abs_h, float pixel_size, int mode) const
{
  int interpolate = 1;
  float l = x_index(abs_l), h = x_index(abs_h);
  if (dimension == Dimension_Y) { l = y_index(abs_l); h = y_index(abs_h); }
  else if (dimension == Dimension_Z) { l = z_index(abs_l); h = z_index(abs_h); }
  switch(_data->data_type)
    {
    case ByteData:
      {
        IsotropicSlicer<u_char> slicer1(_data, _transformer, interpolate);
        return slicer1.projection(dimension, l, h, pixel_size, mode);
      }
    case SunShort:
    case VAX_Ix2:
      {
        IsotropicSlicer<short> slicer2(_data, _transformer, interpolate);
        return slicer2.projection(dimension, l, h, pixel_size, mode);
      }
    case UShort_BE:
    case UShort_LE:
      {
        IsotropicSlicer<unsigned short> slicer2(_data, _transformer, interpolate);
        return slicer2.projection(dimension, l, h, pixel_size, mode);
      }
    case IeeeFloat:
      {
        IsotropicSlicer<float> slicer4(_data, _transformer, interpolate);
        return slicer4.projection(dimension, l, h, pixel_size, mode);
      }
   default:
	throw("bad data type");
   }
}

template <class T>
void matrix_reverse( T *src, MatrixData *vol, DimensionName dimension)
{
	int i1,i2;			// swap indexes i1, i2;
	int x,y,z, sx = vol->xdim, sy = vol->ydim, sz = vol->zdim;
	int size = sx*sy, el_size=sizeof(T);
	T *plane=NULL, *tmp=NULL;
	switch(dimension)
    {
		case Dimension_Z :
			tmp = new T[size];
			for (i1=0,i2=sz-1; i1<i2; i1++, i2--) {
				memcpy(tmp, src+size*i1,size*el_size);
				memcpy(src+size*i1, src+size*i2, size*el_size);
				memcpy(src+size*i2,tmp,size*el_size);
			}
			break;
		case Dimension_X :
			tmp = new T[sx];
			for (z=0;z<sz;z++) {
				plane = src + size*z;			// current plane
				for (y=0; y<sy; y++) {
					T* p0 = plane + sx*y;		// current line
					T* p1 = tmp+sx;			
					for (x=0; x<sx; x++) *p1-- = *p0++; // invert line in tmp
					memcpy(plane + sx*y, tmp, sx*el_size);	// copy tmp in line
				}
			}
			break;
		case Dimension_Y :
			tmp = new T[sx];
			for (z=0;z<sz;z++) {
				plane = src + size*z;			// current plane
				for (i1=0,i2=sy-1; i1<i2; i1++, i2--) {
					memcpy(tmp,plane+sx*i1,sx*el_size);
					memcpy(plane+sx*i1,plane+sx*i2,sx*el_size);
					memcpy(plane+sx*i2,tmp,sx*el_size);
				}
			}
			break;
		}
		delete tmp;
}

void Volume::reverse(DimensionName dimension)
{
  if (!valid()) return;
  switch(_data->data_type)
    {
    case ByteData:
    case Color_8:
        matrix_reverse<u_char>((u_char*)_data->data_ptr,_data, dimension);
        break;
    case SunShort:
    case VAX_Ix2:
        matrix_reverse<short>((short*)_data->data_ptr,_data, dimension);
        break;
    case UShort_BE:
    case UShort_LE:
        matrix_reverse<unsigned short>((unsigned short*)_data->data_ptr,_data, dimension);
        break;
    case Color_24:
        matrix_reverse<unsigned>((unsigned*)_data->data_ptr,_data, dimension);
        break;
    case IeeeFloat:
        matrix_reverse<float>((float*)_data->data_ptr,_data, dimension);
        break;
    }
}

int Volume::voxel_pos(const VoxelCoord& pos, VoxelIndex& voxel_pos) const {
	if (!valid())  return 0;
	float data_min = _data->data_min, scale_factor = _data->scale_factor;
	int xi = round(x_index(pos.x) - 0.5), sx= _data->xdim;  //use mid pixel  xi+0.5 
	int yi = round(y_index(pos.y) - 0.5), sy = _data->ydim; //use mid pixel  yi+0.5
	int zi = round(z_index(pos.z) - 0.5), sz = _data->zdim; //use mid pixel  zi+0.5
	// use mid pixel for xi, yi, zi
	if (_transformer) {
		float *tm = _transformer->data;
		float x0 = x_index(pos.x), y0 = y_index(pos.y), z0 = z_index(pos.z);
		float x1 = tm[0]*x0 + tm[1]*y0 + tm[2]*z0 + tm[3];
		float y1 = tm[4]*x0 + tm[5]*y0 + tm[6]*z0 + tm[7];
		float z1 = tm[8]*x0 + tm[9]*y0 + tm[10]*z0 + tm[11];
		xi = round(x1 - 0.5); yi = round(y1 - 0.5); zi = round(z1 - 0.5);
	}
	if (xi<0 || xi>=_data->xdim || yi<0 || yi>=_data->ydim ||
		zi<0 || zi >= _data->zdim) return 0;
	voxel_pos.x = xi;
	voxel_pos.y = yi;
	voxel_pos.z = zi;
	return 1;
}


float Volume::voxel_value(const VoxelCoord& pos, VoxelCoord& voxel_pos) const {
	if (!valid())  return 0.0;
	float data_min = _data->data_min, scale_factor = _data->scale_factor;
	int xi = round(x_index(pos.x) - 0.5), sx= _data->xdim;  //use mid pixel  xi+0.5 
	int yi = round(y_index(pos.y) - 0.5), sy = _data->ydim; //use mid pixel  yi+0.5
	int zi = round(z_index(pos.z) - 0.5), sz = _data->zdim; //use mid pixel  zi+0.5
	// use mid pixel for xi, yi, zi
	if (_transformer) {
		float *tm = _transformer->data;
		float x0 = x_index(pos.x), y0 = y_index(pos.y), z0 = z_index(pos.z);
		float x1 = tm[0]*x0 + tm[1]*y0 + tm[2]*z0 + tm[3];
		float y1 = tm[4]*x0 + tm[5]*y0 + tm[6]*z0 + tm[7];
		float z1 = tm[8]*x0 + tm[9]*y0 + tm[10]*z0 + tm[11];
		xi = round(x1 - 0.5); yi = round(y1 - 0.5); zi = round(z1 - 0.5);
	}
	if (xi<0 || xi>=_data->xdim || yi<0 || yi>=_data->ydim ||
		zi<0 || zi >= _data->zdim) return 0.0;
	u_char *bp = (u_char*)_data->data_ptr + sx*sy*zi;
	short* sp = (short*)_data->data_ptr + sx*sy*zi;
	unsigned short* usp = (unsigned short*)_data->data_ptr + sx*sy*zi;
	float *fp = (float*)_data->data_ptr + sx*sy*zi;
	int nvoxels = sx*sy*sz;
	voxel_pos.x = xi*_data->pixel_size;
	voxel_pos.y = yi*_data->y_size;
	voxel_pos.z = zi*_data->z_size;
	voxel_pos.undefined = 0;
	switch(_data->data_type)
	{
	case ByteData:
	case Color_8:
		return (data_min + scale_factor*bp[yi*sx+xi]);
	case SunShort:
	case VAX_Ix2:
		return  (scale_factor*sp[yi*sx+xi]);
	case UShort_BE:
	case UShort_LE:
		return  (_data->intercept + scale_factor*usp[yi*sx+xi]);
	case Color_24:
		return ((bp[2*nvoxels+yi*sx+xi]<<16) +
				(bp[nvoxels+yi*sx+xi] << 8) +
				bp[yi*sx+xi]);
//		return 1;
	case IeeeFloat:
		return  (scale_factor*fp[yi*sx+xi]);
	default:
		throw("bad data type");
	}
}

int Volume::transform(float r, float t1, float t2, DimensionName dimension) {
	switch (dimension) {
	 case Dimension_X :
		return transform(r,0,0,0,t1,t2);
	 case Dimension_Y :
		return transform(0,r,0,t1,0,t2);
	 case Dimension_Z :
		return transform(0,0,r,t1,t2,0);
	}
	return 0;
}

int Volume::transformer(Matrix m) {
	if (_transformer) {
		mat_free(_transformer);
		_transformer = NULL;
	}
	if (m != NULL) {
		if (m->ncols==4 && m->nrows==4) transform(m);
		else return 0;
	} 
	return 1;
}

int Volume::transform(float rx, float ry, float rz, float tx,
float ty, float tz) {
	float cx = 0.5*_data->xdim*_data->pixel_size;
	float cy = 0.5*_data->ydim*_data->y_size;
	float cz = 0.5*_data->zdim*_data->z_size;
	Matrix t = mat_alloc(4,4);
        
                             // apply Transformer w.r.t AIR convention
	scale(t, _data->pixel_size,_data->y_size, _data->z_size);
	translate(t,-cx,-cy,-cz);
	rotate(t,0,0,rz);        // apply yaw (+)
	rotate(t,-rx,0,0);       // apply pitch (-)
	rotate(t,0,-ry,0);       // apply roll (-)
	translate(t,cx,cy,cz);
	translate(t,-tx,ty,tz);	 // reverse x
	scale(t, 1/_data->pixel_size, 1/_data->y_size, 1/_data->z_size);

//	TRACE2("x_offset,y_offset=%g,%g\n",t->data[3], t->data[7]);

	transform(t);
	mat_free(t);
	return 1;
}

char*  Volume::save(MatrixFile* matfile, const u_char* rgb,
const char* _fname) const{
	FILE *fp=NULL;
	char *fname = NULL, *ext = NULL;
	const char* str = _fname;
	struct MatDir matdir;
	char data_offset[20];
	const u_char *p = NULL;
	int i, j, error_flag=0;

	assert(_data->data_type != Color_24);
	char **ifh = matfile->interfile_header;
	if (ifh == NULL) { // CTI PetVolume 
		// read matnum sub_header
		// check check if volume and header dimensions are same
		// reject if dimensions not comaptible
		// get volume offset
		// sprintf(data_offset,offset);
		ifh = (char**)calloc(END_OF_INTERFILE+1,sizeof(char*));
		ifh[VERSION_OF_KEYS] = "3.3";
		ifh[NAME_OF_DATA_FILE] = matfile->fname;
		MatrixData *header = matrix_read(matfile,_data->matnum,
			MAT_SUB_HEADER);
		if (header == NULL || header->xdim != _data->xdim ||
			header->ydim != _data->ydim || header->zdim != _data->zdim)
			error_flag++;
		free_matrix_data(header);
		if (!error_flag) {
			matrix_find(matfile,_data->matnum,&matdir);
			if (matfile->mhptr->file_type == Short3dSinogram ||
				matfile->mhptr->file_type == Float3dSinogram) 
				sprintf(data_offset,"%d",(matdir.strtblk+1)*512);
			else  sprintf(data_offset,"%d",(matdir.strtblk)*512);
			ifh[DATA_OFFSET_IN_BYTES] = data_offset;
		} else {
			free(ifh);
			return NULL;
		}
		ifh[IMAGE_MODALITY] = "PET";
	}
	
	if (str==NULL || strlen(str) == 0) str = matfile->fname;
	fname = (char*)malloc(strlen(str)+4);
	strcpy(fname,str);
	if ( (ext = strrchr(fname,'.')) != NULL) strcpy(ext+1,"h33");
	else strcat(fname,".h33");
	if ((fp=fopen(fname,"w")) ==  NULL) {
		perror(fname);
		if (matfile->interfile_header == NULL) free(ifh);
		return NULL;
	}
	fprintf(fp,"!INTERFILE \n");
	for (InterfileItem* item=used_keys; item->key!=END_OF_INTERFILE; item++) {
		if (ifh[item->key] != 0 &&  item->key!= COLORTAB && 
		item->key != TRANSFORMER )
				fprintf(fp,"%s := %s\n", item->value,ifh[item->key]);
	}
	if (matfile->interfile_header == NULL)
		fprintf (fp, "conversion program := Volume::save\n");
	if (ifh[NUMBER_FORMAT] == 0) {
		switch(_data->data_type) {
		case ByteData :
		case Color_8 :
			fprintf (fp, "number format  := unsigned integer\n");
			fprintf (fp, "number of bytes per pixel  := 1\n");
			break;
		case SunShort:
			fprintf (fp, "number format  := signed integer\n");
			fprintf (fp, "number of bytes per pixel  := 2\n");
			break;
		case UShort_BE:
			fprintf (fp, "number format  := unsigned integer\n");
			fprintf (fp, "number of bytes per pixel  := 2\n");
			break;
		case IeeeFloat :
			fprintf (fp, "number format  := short float\n");
			fprintf (fp, "number of bytes per pixel  := 4\n");
			break;
		case UShort_LE:
			fprintf (fp, "imagedata byte order := little endian\n");
			fprintf (fp, "number format  := unsigned integer\n");
			fprintf (fp, "number of bytes per pixel  := 2\n");
		default:
		case VAX_Ix2:
			fprintf (fp, "imagedata byte order := little endian\n");
			fprintf (fp, "number format  := signed integer\n");
			fprintf (fp, "number of bytes per pixel  := 2\n");
		}
	}
	if (ifh[NUMBER_OF_DIMENSIONS] == 0) {
		fprintf (fp, "number of dimensions   := 3\n");
		fprintf (fp, "matrix size [1]	:= %d\n", _data->xdim);
		fprintf (fp, "matrix size [2]	:= %d\n", _data->ydim);
		fprintf (fp, "matrix size [3]	:= %d\n", _data->zdim);
	}
	if (ifh[SCALE_FACTOR_1] == NULL) {
		fprintf (fp,"scaling factor (mm/pixel) [1]  := %f\n",_data->pixel_size);
		fprintf (fp,"scaling factor (mm/pixel) [2]  := %f\n", _data->y_size);
		fprintf (fp, "scaling factor (mm/pixel) [3]  := %f\n", _data->z_size);
	}
	int d_min = (int)((_data->data_max*dyn_min)/(_data->scale_factor*100));
	int d_max = (int)((_data->data_max*dyn_max)/(_data->scale_factor*100));
	fprintf(fp,";%%display range := %d %d\n", d_min, d_max);
	if (ifh[QUANTIFICATION_UNITS] == NULL)
		 fprintf(fp,";%%quantification units := %g\n", _data->scale_factor);
	if (_transformer != NULL) {
		fprintf(fp,";%%transformer := <\n");
		for (j=0; j<4; j++) {
			fprintf(fp, ";");
			for (i=0; i<4; i++)
				fprintf(fp,"%13.6g ",_transformer->data[i+j*4]);
			fprintf(fp, "\n");
		}
		fprintf(fp,";>\n");
	}
	if ((p=rgb) != NULL) {
		fprintf (fp, ";%%colortab := <\n");
		for (i=0; i<max_byte_value; i++, p += 3)
			fprintf (fp, ";%d %d %d\n",p[0],p[1],p[2]);
		fprintf (fp, ";>\n");
	}
	fprintf(fp,"!END OF INTERFILE \n");
	fclose(fp);
	if (matfile->interfile_header == NULL) free(ifh);
	return fname;
}
int Volume::transform(const char* ts) {
	int i,j, k, error_flag=0;
	Matrix m = mat_alloc(4,4);
	char *dup = strdup(ts);
	char *words[16];

	for (j=0; j<4 && !error_flag; j++) {
		for (i=0; i<4 && !error_flag; i++) {
			k = i+j*4;
			if (k==0) words[k] = strtok(dup, " \n");
			else words[k] = strtok(NULL, " \n");
			if (words[k]==NULL) error_flag++;
		}
	}

	for (k=0; k<16 && !error_flag; k++) 
		if (sscanf(words[k], "%g",&m->data[k]) != 1) error_flag++;
	free(dup);
	if (!error_flag) transform(m);
	mat_free(m);
	if (!error_flag) return 1;
	return 0;
}

int Volume::transform(Matrix m) {
	if (m && (m->ncols != 4 || m->nrows != 4)) return 0;
//	TRACE2("x_offset,y_offset=%g,%g\n",m->data[3], m->data[7]);
	if (_transformer) matmpy(_transformer,m,_transformer);
	else {
		_transformer = mat_alloc(4,4);
		memcpy(_transformer->data,m->data,16*sizeof(float));
	}
	return 1;
}

int Volume::histogram(float *& xv, float *& yv, int &psize) {
	if (!valid()) return 0;
	int i=0, nvoxels=_data->xdim*_data->ydim*_data->zdim;
	float d_min = _data->data_min/_data->scale_factor;
	float d_max = _data->data_max/_data->scale_factor;
    float d_step = 0.01*max(fabs(d_max), fabs(d_min));
	psize = (int)(0.5 + (d_max-d_min)/d_step + 1);
    if (_data->data_type == Color_24) { // use brightness [0 256] step 2
		psize=128; d_step = 2;
	}
	if (_histogram.x == NULL) {		// new histogram
		xv = _histogram.x = new float[psize];
		yv = _histogram.y = (float*)calloc(psize,sizeof(float));
		_histogram.code = 0;
	} else {
		xv = _histogram.x;
		yv = _histogram.y;
		return psize;
	}
	xv[0] = d_min/d_step;
	for (i=1; i<psize; i++) xv[i] = xv[i-1]+1;
	switch(_data->data_type)
	{	
	case ByteData:
	case Color_8:
		t_histogram<u_char>((u_char*)_data->data_ptr, nvoxels, d_min, d_max,
			d_step, yv, psize);
		break;
	case SunShort:
	case VAX_Ix2:
		t_histogram<short>((short*)_data->data_ptr,nvoxels, d_min, d_max,
			d_step, yv, psize);
		break;
	case UShort_BE:
	case UShort_LE:
		t_histogram<unsigned short>((unsigned short*)_data->data_ptr,nvoxels, d_min, d_max,
			d_step, yv, psize);
		break;
	case IeeeFloat:
		t_histogram<float>((float*)_data->data_ptr, nvoxels, d_min, d_max,
			d_step, yv, psize);
		break;
	case Color_24:
		color_histogram((u_char*)_data->data_ptr, nvoxels, yv, psize);
		break;
	default:
		throw("Volume::histogram : bad datatype");
	}
	normalize(yv,psize);
	return psize;
}

int Volume::set_proj_range(float m, float M)
{
	if ((M-m)>0 && (M-m)<200)
	{
		proj_min = m;
		proj_max = M;
		return 1;
	}
	return 0;
}

Volume::~Volume() {
	free_matrix_data(_data);
// !!! don't use free_matrix_data : red_data, green_data, blue_data
// points to freed _data area
	if (red_data) free(red_data);
	if (green_data) free(green_data);
	if (blue_data) free(blue_data);
}
