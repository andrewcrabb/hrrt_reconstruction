#include <xmmintrin.h>
#include "hrrt_osem3d.h"

void mulprojection(float ***prj1,float ***prj2,int theta) {
	int v,yr,xr,yr2;
	__m128 *ptr1,*ptr2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){
			ptr1=(__m128 *) prj1[yr2][v];
			ptr2=(__m128 *) prj2[yr][v];
			for(xr=0;xr<xr_pixels/4;xr++){
				ptr1[xr]=_mm_mul_ps(ptr1[xr],ptr2[xr]);
			}
		}
	}
}

/* 
divideprojection not a simd operation because done only when denum not 0 
it is added to get a complete set 
*/
void divideprojection(float ***prj1,float ***prj2,int theta) {
	int v,yr,xr,yr2;
	float *ptr1,*ptr2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){
			ptr1=prj1[yr2][v];
			ptr2=prj2[yr][v];
			for(xr=0;xr<xr_pixels;xr++){
				if (ptr2[xr] > 0) ptr1[xr] /= ptr2[xr];
        else ptr2[xr] = 0.0f;
			}
		}
	}
}

void addprojection(float ***prj1,float ***prj2,int theta) {
	int v,yr,xr,yr2;
	__m128 *ptr1,*ptr2;    
	//float *ptrf1,*ptrf2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){
			ptr1=(__m128 *) prj1[yr2][v];
			ptr2=(__m128 *) prj2[yr][v];
			//ptrf1=(float *)ptr1;
			//ptrf2=(float *)ptr2;
			for(xr=0;xr<xr_pixels/4;xr++){
				ptr1[xr]=_mm_add_ps(ptr1[xr],ptr2[xr]);
			}
		}
	}
}

void subprojection(float ***prj1,float ***prj2,int theta) {
	int v,yr,xr,yr2;
	__m128 *ptr1,*ptr2;    
	//float *ptrf1,*ptrf2;

	yr2=segment_offset[theta];
	for( yr=0; yr<=yr_top[theta]-yr_bottom[theta]; yr++,yr2++){
		for(v=0;v<views;v++){
			ptr1=(__m128 *) prj1[yr2][v];
			ptr2=(__m128 *) prj2[yr][v];
			//ptrf1=(float *)ptr1;
			//ptrf2=(float *)ptr2;
			for(xr=0;xr<xr_pixels/4;xr++){
				ptr1[xr]=_mm_sub_ps(ptr1[xr],ptr2[xr]);
			}
		}
	}
}
void setnegative2zero(float ***largeprj)
{
	int v,yr2,xr;
	__m128 *ptr,coef1;
	float *float_ptr;
	coef1=_mm_set_ps(0.0,0.0,0.0,0.0);
	for(v=0;v<views/2;v++){
		for(yr2=0;yr2<segmentsize*2;yr2++){
			ptr=(__m128 *) largeprj[v][yr2];
			float_ptr=largeprj[v][yr2];
			for(xr=0;xr<xr_pixels/4;xr++){
				ptr[xr]=_mm_max_ps(ptr[xr],coef1);
			}
			for(xr=0;xr<xr_pixels;xr++){
				if(float_ptr[xr]<0) printf("error setnegative2zero\n");
			}
		}
	}
}
