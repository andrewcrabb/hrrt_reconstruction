#ifndef Volume_h
#define Volume_h

/*
 * static char sccsid[] = "%W% UCL-TOPO %E%";
 *
 *  Author : Merence Sibomana (Sibomana@topo.ucl.ac.be)
 *
 *	  Positron Emission Tomography Laboratory
 *	  Universite Catholique de Louvain
 *	  Ch. du Cyclotron, 2
 *	  B-1348 Louvain-la-Neuve
 *		  Belgium
 *
 *
 *  This program may be used free of charge by the members
 *  of all academic and/or scientific institutions.
 *	   ANY OTHER USE IS PROHIBITED.
 *  It may also be included in any package
 *	  -  provided that this text is not removed from
 *	  the source code and that reference to this original
 *	  work is done ;
 *	  - provided that this package is itself available
 *	  for use, free of charge, to the members of all
 *	  academic and/or scientific institutions.
 *  Nor the author, nor the Universite Catholique de Louvain, in any
 *  way guarantee that this program will fullfill any particular
 *  requirement, nor even that its use will always be harmless.
 *
 *
 */

/*
	Volume units are expressed in mm
*/

#include <sys/types.h>
#include <math.h>
#include <string.h>
#include "ecatx/matrix.h"
#include "ecatx/DICOM.h"
#include "ecatx/interfile.h"
#include "ecatx/matpkg.h"
#include "matrix_resize.h"
#include "VoxelCoord.h"
#include "Modality.h"
#define IMAGENAME_MAX 39
#include <map>
using namespace std;

typedef enum { MEAN_PROJ, MAX_PROJ } ProjectionMode;
typedef enum { VOLUME_CENTER, VOLUME_ORIGIN, ATLAS_ORIGIN} CoordOrigin;
typedef enum { VOXEL_UNIT, METER_UNIT} CoordUnit;

class ColorConverter {
protected:
	int _bytes_per_pixel;
public:
	ColorConverter(int depth=1) { _bytes_per_pixel = depth; }
	int bytes_per_pixel() {return _bytes_per_pixel; }
	virtual MatrixData *load(MatrixFile *file, MatrixData *vheader);
};

class VoIValue {
public:
	const char* name;
	float _average;
	float stdev;
	float max;
	float min;
	float _recovery;
	VoIValue();
};

class Volume {
protected:
	MatrixData *red_data, *green_data, *blue_data; //Color data decomposition
	Main_header main_header;
	MatrixData* _data;
	VoxelCoord _origin;		// transformed origin
	int dyn_max, dyn_min;			   // slices dynamic range (percent)
	float proj_max, proj_min;			   // projection dynamic range (percent)
	int _atlas_origin;		// flag: 1 if atlas origin defined, 0 otherwise
	ImageModality _modality;
	int _data_unit, _user_unit;	
	struct {
    int code;			// histogram code = 100*dyn_max+dyn_min
    float *x, *y;				// histogram data
  } _histogram;
	char _name[IMAGENAME_MAX+1];
	virtual int load(MatrixFile*, int matnum, int segment=0);
	Matrix _transformer;
	void update_origin();	// update transformed update
	MatrixData *rgb_create_data(MatrixData *r,
                                    MatrixData *g ,
                                    MatrixData *b) const;
	 MatrixData *rgb_split_data(MatrixData *src ,
                                    int segment) const;
public :
	enum { NONE=0,
						/* Begin PET units */
		CPS=1, BQ_ML=2, PROSESSED=3, UCI_ML=4,
		UMOLE_100G_MIN=5, MG_100G_MIN=6, ML_100G_MIN=7,
		/* micromole/100g/min, mg/100g/min,  ml/100g/min*/
		SUV_BW=8, SUV_BSA=16, SUV_LEAN=32,
		/* body, surface and lean weighted */
							/* end PET units */
		CM_1=33, HOUNSFIELD=34		/* TX(1/cm) and CT units */
		} Units;

	enum { SAGITTAL=0, CORONAL, AXIAL, OBLIC , UNKNOWN} orientation;
	
	static ColorConverter *color_converter;
	static int max_byte_value;			// limit in byte_scale methods
    static float min_pixel_size;	   // in mm (default 1mm)
	static int matnum(MatrixFile*, int frame);
  static void apply_zoom(const VoxelCoord &zoom_center, float zoom_f,
    DimensionName orthogonal , float *x, float *y, int count=1);
  static void invert_zoom(const VoxelCoord &zoom_center, float zoom_f,
    DimensionName orthogonal , float *x, float *y, int count=1);
	static float distance(const VoxelCoord& p0, const VoxelCoord& p1);

	Volume(int sx=128, int sy=128, int sz=128);
	Volume(Main_header *mh, MatrixData*, const char* name_str=0);
	Volume(MatrixFile*, int matnum, int segment=0);
									// segment is used for 3D sinograms 
	virtual Volume* clone() const;
	int contains(const VoxelCoord&);
	char* save(MatrixFile*, const unsigned char * cmap=NULL,
		const char* fname=NULL) const;
			// export current range  in InterFile format

	char* unit_name(unsigned);
	int conversion_factor(unsigned unit, float &ret_factor) const;
	char* unit_name() { return unit_name(_user_unit); }
	int convert(float data_value, float &user_value);
	float conversion_factor() const;		// user_unit conversion factor
									//  see inline definition below

	virtual MatrixData* average(float z0, float thickness, float pixel_size,
		int interpolate=1) const;

	const MatrixData& get_slice_header(DimensionName) const;
	MatrixData* get_slice(DimensionName, float x, int interpolate=1) const;
	MatrixData* get_slice(DimensionName, float x, VoxelCoord& area,
	    float pixel_size, int interpolate=1) const;
	MatrixData* get_slice(const VoxelCoord &zoom_center, float zoom_f,
        DimensionName, float x, VoxelCoord& area,
	    float pixel_size, int interpolate=1) const;
 // reslicing acronymes
  MatrixData* transverse(float z, int interpolate=1) const;
  MatrixData* coronal(float y, int interpolate=1) const;
  MatrixData* sagittal(float x, int interpolate=1) const;

	virtual MatrixData* projection(DimensionName, float l, float r,
		float pixel_size, int mode=0) const;

	virtual void reverse(DimensionName);

	void dynamic_range(float min, float max);
	float dynamic_min() const;		// absolute value
	float dynamic_max() const;		// absolute value
	int dynamic_imin() const;       // without scale_factor
	int dynamic_imax() const;       // without scale_factor

	void get_proj_range(float& m, float& M) const {m = proj_min; M = proj_max;}
	int set_proj_range(float, float);

	unsigned modality() const { return _modality; }

	int atlas_origin() const { return _atlas_origin; }
	float origin(DimensionName);
	int origin(DimensionName dimension, float& o1, float& o2) const;
	const char* name() const { return _name; }
	void name(const char* s) { strncpy(_name,s,IMAGENAME_MAX); }
	const MatrixData* data() const { return _data; }
	virtual float voxel_value(const VoxelCoord& p, VoxelCoord& voxel_pos) const;
	int voxel_pos(const VoxelCoord& p, VoxelIndex& voxel_pos) const;
	int valid() const { return _data->data_ptr? 1: 0; }
	int unit_names(map<int, char*> &names);
	int current(DimensionName direction, float x); 		// 1D move
	int current(DimensionName orthogonal, float x, float y); // 2D move
	int current(const VoxelCoord& v); 								// 3D move
	void center(); 								// go to center
	const VoxelCoord& origin() const { return _origin; }
		/* set new origin to current position, update header if SPM image */
	void set_origin(const VoxelCoord& pos, const char* spm_hdr=NULL);
	float x_index(float) const;
	float y_index(float) const;
	float z_index(float) const;
	int data_unit() const { return _data_unit; }
	int set_unit(unsigned user_unit);
	int get_unit() const { return _user_unit; }
	const Main_header &get_main_header() { return main_header; }

	int transform(const char* ts);		// 4x4 matrix string
	int transform(float rx, float ry, float rz,
		float tx, float ty, float tz);
	int transform(float r, float t1, float t2, DimensionName orthogonal);
	virtual int transform(Matrix m);

	int transformer(Matrix m);
	const Matrix transformer() { return _transformer; }
	int histogram(float*& xv, float*& yv, int& size);
	~Volume();
};

inline void Volume::dynamic_range(float min, float max) {
	dyn_min = (int)(min/_data->data_max*100);
	dyn_max = (int)(max/_data->data_max*100);
}
inline float Volume::dynamic_min() const {
	return _data->data_max*dyn_min/100;
}
inline float Volume::dynamic_max() const {
	return _data->data_max*dyn_max/100;
}
inline int Volume::dynamic_imin() const {
	float f = _data->data_max*dyn_min/100;
	return (int)(f/_data->scale_factor);
}
inline int Volume::dynamic_imax() const {
	float f = _data->data_max*dyn_max/100;
	return (int)(f/_data->scale_factor);
}

inline MatrixData* Volume::transverse(float z, int interpolate) const
{
  return get_slice(Dimension_Z, z, interpolate);
}
inline MatrixData* Volume::coronal(float y, int interpolate) const
{
  return get_slice(Dimension_Y, y, interpolate);
}
inline MatrixData* Volume::sagittal(float x, int interpolate) const
{
  return get_slice(Dimension_X, x, interpolate);
}

inline float Volume::distance(const VoxelCoord& p0, const VoxelCoord& p1) {
	float dx = p1.x-p0.x, dy = p1.y-p0.y, dz = p1.z-p0.z;
	double d = 0.1*(int)(10*sqrt(dx*dx + dy*dy + dz*dz));
	return (float)d;
}

inline const char* dimension_name(DimensionName dim) {
	const char* ret = "undefined";
	if (dim == Dimension_X) ret = "sagittal";
	if (dim==Dimension_Y) ret = "coronal";
	if (dim==Dimension_Z) ret = "transverse";
	return ret;
}

inline VoIValue::VoIValue() {
	name = "no name"; 
	_average = stdev = min = max = _recovery = 0;
}

inline float Volume::origin(DimensionName dimension) {
	float ret = _origin.x;
	if (dimension == Dimension_Y) ret = _origin.y;
	else if (dimension == Dimension_Z) ret = _origin.z;
	return ret;
}

inline float Volume::x_index(float xc) const {
	float x = 0;
//	if (xc>0) {
		x = xc/_data->pixel_size;
//		if (x > _data->xdim-1) x = _data->xdim-1;
//	}
	return x;
}

inline float Volume::y_index(float yc) const {
	float y = 0;
//	if (yc>0) {
		 y = yc/_data->y_size;
//		if (y > _data->ydim-1) y = _data->ydim-1;
//	}
	return y;
}

inline float Volume::z_index(float zc) const {
	float z = 0;
//	if (zc > 0) {
		z = zc/_data->z_size;
//		if (z > _data->zdim-1) z = _data->zdim-1;
//	}
	return z;
}

inline float Volume::conversion_factor() const {
	float user_factor = 1.0;
	conversion_factor(_user_unit, user_factor);
	return user_factor;
}

inline int interpolability(const MatrixFile* file) {
	int ret = 1;
	char **ifh = NULL;
	if (file) ifh = file->interfile_header;
	if (ifh && ifh[INTERPOLABILITY]) sscanf(ifh[INTERPOLABILITY],"%d",&ret);
	return ret;
}

template <class T> T brightness(T r, T g, T b)
{
	T bt = r>g? r : g;
	return bt>b? bt : b;
}

inline float mm_round(float v) {		/* round to mm */
	return (float)floor(v + 0.5);
}

inline float mm3_round(float v) {		/* round to mm3 */
	return (float)(0.001*floor(1000*v + 0.5));
}



MatrixData *matrix_brightness(const MatrixData* rgb);
extern MatrixData *matrix_create(const MatrixData* orig);

#endif
