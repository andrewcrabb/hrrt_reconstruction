#ifndef VoxelCoord_h
#define VoxelCoord_h

#ifndef DIMENSION_NAME	
// DimensionName is also defined in <Graphic/geometry.h>
typedef unsigned DimensionName;
enum { Dimension_X=0, Dimension_Y, Dimension_Z, Dimension_Undefined
 };
#define DIMENSION_NAME 1
#endif

class VoxelIndex {
public :
	short x,y,z;
	VoxelIndex(int vx=0, int vy=0, int vz=0) : x(vx), y(vy), z(vz) {}
	VoxelIndex(int vx, int vy, int vz, DimensionName d);
	int get(DimensionName d) const;
	int get(DimensionName d, int &x, int &y) const;
	int set(DimensionName d, int vz);
	int set(DimensionName d, int vx, int vy);
   	int operator[](DimensionName d) const { return get(d); }
};

class VoxelCoord {
public :
	char undefined;
	float x,y,z;
	VoxelCoord() {x=y=z=0; undefined=1; }
	VoxelCoord(float vx, float vy, float vz) :
		x(vx), y(vy), z(vz), undefined(0) {};
	float get(DimensionName d) const;
	float get(DimensionName d, float &x, float &y) const;
	int set(DimensionName d, float v);
	int set(DimensionName d, float x, float y);
    float operator[](DimensionName d) const { return get(d); }
    int equal(const VoxelCoord v) const;
};

inline VoxelIndex::VoxelIndex(int vx, int vy, int vz, DimensionName d)
{
	set(d,vz);
	set(d,vx,vy);
}

#endif
