/*
psf.cpp
Author: Claude Comtat
Creation date: ?
Modification history:
  08-MAY-2008: modified to compile with microsoft visual studio (M. Sibomana) 
*/
#include <stdio.h>
#ifdef WIN32
#include <process.h>
#endif
#include <math.h>
#include "hrrt_osem3d.h"

#ifdef IS_LINUX
#define _alloca alloca
#endif

typedef struct {
	ConvolArg *x,*y,*z;
} ConvolArgXYZ;

/* C. Comtat, PSF */
int make1d_psf_filter(Psf1D *psf)
{
  int   bin,bin0;
  float norm_psf,sigma1,sigma2;
  if (psf->fwhm1 == 0.0) { /* use default value since inki's code cannot decode negative values */
    if (strcmp(psf->model->number,"999") == 0) {
      psf->fwhm1 = 0.9*sqrt(8.0*log(2.0));
      psf->fwhm2 = 2.5*sqrt(8.0*log(2.0));
      psf->ratio = 0.05;
      psf->type = GaussShape;
    } else if (strcmp(psf->model->number,"962") == 0) {
      psf->fwhm1 = 3.25; 
      psf->fwhm2 = 8.00;
      psf->ratio = 1./6.;
      psf->type = GaussShape;
    } else {
      printf(" make1d_psf_filter> !!! Error: no default value for system %0d\n",psf->model->number);
      return -1;
    }  
    printf(" make1d_psf_filter> default PSF parameters for the %s: Gauss 1 fwhm = %0g [mm], Gauss 2 fwhm = %0g [mm], ratio = %0g\n",
           psf->model->model_name,psf->fwhm1,psf->fwhm2,psf->ratio);
  }
  if (psf->fwhm1 > 0.0) {
    if (psf->kernel != NULL) free(psf->kernel);
    if (psf->fwhm2 > 0.0 && psf->ratio > 0.0) {
      psf->kernel_size = 2*((int) (1.5*psf->fwhm2*sqrt(log(10.0)/log(2.))/psf->pixel_size/2.0))+1;
    } else {
      psf->kernel_size = 2*((int) (1.5*psf->fwhm1*sqrt(log(10.0)/log(2.))/psf->pixel_size/2.0))+1;
    }
    printf(" make1d_psf_filter> kernel size is %0d pixels (%0g mm)\n",psf->kernel_size,psf->kernel_size*psf->pixel_size);
    psf->kernel = (float *) calloc(psf->kernel_size,sizeof(float)); 
    if (psf->type == GaussShape) {  
      sigma1 = psf->fwhm1/(sqrt(8.0*log(2.0))*psf->pixel_size);
      sigma2 = psf->fwhm2/(sqrt(8.0*log(2.0))*psf->pixel_size);
    } else return -1; 
    bin0 = (psf->kernel_size-1)/2;
    norm_psf = 0.0;
    for (bin=0;bin<psf->kernel_size;bin++) {
      if (psf->type == GaussShape) {  
        psf->kernel[bin] = exp(-0.5*(pow((double)(bin-bin0)/sigma1,2.0)));
        if (psf->fwhm2 > 0.0 && psf->ratio > 0.0) psf->kernel[bin] += psf->ratio*exp(-0.5*(pow((double)(bin-bin0)/sigma2,(double) 2.0)));
      }
      norm_psf += psf->kernel[bin];
    }  
    for (bin=0;bin<psf->kernel_size;bin++) psf->kernel[bin] /= norm_psf;
    // for (bin=0;bin<psf->kernel_size;bin++)
    // printf("%d\t%f\n",bin,psf->kernel[bin]);

    return 0;
  } else return -1;  
} 

void init_psf1D(float blur_fwhm1,float blur_fwhm2,float blur_ratio,Psf1D *psf)
{
	if(make1d_psf_filter(psf)==-1) crash1("Psf Init Error \n");
}


int convolve1d(float *data_in, float *data_smooth, int data_size, Psf1D *psf)
{ 
  int bin,kbin0,kbin,dbin;
  float norm,sum;
  
  if (data_size <= 0) {
    printf(" convolve1d> !!! Error: data size not specified\n");
    return -1;
  } else if (psf->kernel_size < 1) {
    printf(" convolve1d> !!! Error: kernel size not specified\n");
    return -2;
  } else if (psf->kernel == NULL) {
    printf(" convolve1d> !!! Error: no kernel defined\n");
    return -3;
  }      
  kbin0 = (psf->kernel_size-1)/2;
  for (bin=0;bin<data_size;bin++) {
    data_smooth[bin] = 0;
    norm = 0.0;
    for (kbin=0;kbin<psf->kernel_size;kbin++) {
      dbin = kbin - kbin0;
      if ((bin+dbin >= 0) && (bin+dbin) < data_size) {
        data_smooth[bin] += psf->kernel[kbin]*data_in[bin+dbin];
		norm += psf->kernel[kbin];
      }
    }  
    if (norm > 0.0) data_smooth[bin] /= norm;
  }
  return 0;    	 
}        

/*
int blur_image(float ***image, int nthreads, Psf1D *psfx, Psf1D *psfy, Psf1D *psfz)
{
   int	x,y,z;
   static float* datax = NULL;
   static float* datax_sm= NULL;
   static float* datay = NULL;
   static float* datay_sm = NULL;
   static float* dataz = NULL;
   static float* dataz_sm = NULL;
   if (!datax) {	// allocate iif undefined
	datax    = (float *) calloc(x_pixels,sizeof(float));
   	datax_sm = (float *) calloc(x_pixels,sizeof(float));
   	datay    = (float *) calloc(y_pixels,sizeof(float));
   	datay_sm = (float *) calloc(y_pixels,sizeof(float));
   	dataz	 = (float *) calloc(z_pixels,sizeof(float));
   	dataz_sm = (float *) calloc(z_pixels,sizeof(float));
   }
   // X convolve 
   for (z=0;z<z_pixels;z++) 
     for (y=0;y<y_pixels;y++) 
     {  
        for (x=0;x<x_pixels;x++) datax[x] = image[x][y][z];
        convolve1d(datax,datax_sm,x_pixels,psfx);
        for (x=0;x<x_pixels;x++) image[x][y][z] = datax_sm[x];
     }	
    // Y convolve 
    for (z=0;z<z_pixels;z++) 
      for (x=0;x<x_pixels;x++) 
      {
        for (y=0;y<y_pixels;y++) datay[y] = image[x][y][z];
        convolve1d(datay,datay_sm,y_pixels,psfy);
        for (y=0;y<y_pixels;y++) image[x][y][z] = datay_sm[y];
      }	
    // Z convolve 
    for (y=0;y<y_pixels;y++) 
      for (x=0;x<x_pixels;x++) 
      {
        for (z=0;z<z_pixels;z++) dataz[z] = image[x][y][z];
        convolve1d(dataz,dataz_sm,z_pixels,psfz);
        for (z=0;z<z_pixels;z++) image[x][y][z] = dataz_sm[z];
      }	
    return 0;
}
*/

int blur_image(float ***image, int nthreads, Psf1D *psfx, Psf1D *psfy, Psf1D *psfz)
{
#ifdef IS_WIN32
    HANDLE threads[MAXTHREADS];
#else
    pthread_t threads[MAXTHREADS];
#endif
    ConvolArg convolarg[MAXTHREADS];

   int x,y,z;
   int start,end,step;
   int thr;
   unsigned threadID;	
   int i;
   // X convolve 
   //convolx(image,thr,0,z_size,psfx);
//   start=0;
//   step = z_pixels/nthreads;
//   end = step;
//   for (thr = 0; thr < nthreads; thr++) {
//     if(thr==nthreads-1) end=z_pixels;
//     convolarg[thr].ima   = (float ***)image;
//     convolarg[thr].thr   = thr;
//     convolarg[thr].start = start;
//     convolarg[thr].end = end;
//     convolarg[thr].psf = (Psf1D *)psfx; 
////     printf(" thread %d , start %d, end %d\n",thr,start,end);
//     start += step;
//     end += step;
//     #ifdef IS_WIN32
//     threads[thr] = (HANDLE)_beginthreadex( NULL, 0, &pt_convolx, &convolarg[thr], 0, &threadID );
//     #else
//     pthread_create(&threads[thr],NULL,pt_convolx, &convolarg[thr]);
//     #endif
//   } // All threads started...now we wait for them to complete
//   for (thr = 0; thr < nthreads; thr++) Wait_thread(threads[thr]);
//   // Y convolve 
//   //convoly(image,thr,0,z_size,psfy);
//   start=0;
//   step = z_pixels/nthreads;
//   end = step;
//   for (thr = 0; thr < nthreads; thr++) {
//     if(thr==nthreads-1) end=z_pixels;
//     convolarg[thr].ima   = (float ***)image;
//     convolarg[thr].thr   = thr;
//     convolarg[thr].start = start;
//     convolarg[thr].end = end;
//     convolarg[thr].psf = (Psf1D *) psfy; 
//     start += step;
//     end += step;
//     #ifdef IS_WIN32
//     threads[thr] = (HANDLE)_beginthreadex( NULL, 0, &pt_convoly, &convolarg[thr], 0, &threadID );
//     #else
//     pthread_create(&threads[thr],NULL,pt_convoly, &convolarg[thr]);
//     #endif
//   } // All threads started...now we wait for them to complete
//   for (thr = 0; thr < nthreads; thr++) Wait_thread(threads[thr]);
//   // Z convolve 
//   // convolz(image,thr,0,y_size,psfz);
//   start=0;
//   step = y_pixels/nthreads;
//   end = step;
//   for (thr = 0; thr < nthreads; thr++) {
//     if(thr==nthreads-1) end=y_pixels;
//     convolarg[thr].ima   = (float ***)image;
//     convolarg[thr].thr   = thr;
//     convolarg[thr].start = start;
//     convolarg[thr].end = end;
//     convolarg[thr].psf = (Psf1D *) psfz; 
//     start += step;
//     end += step;
//     #ifdef IS_WIN32
//     threads[thr] = (HANDLE)_beginthreadex( NULL, 0, &pt_convolz, &convolarg[thr], 0, &threadID );
//     #else
//     pthread_create(&threads[thr],NULL,pt_convolz, &convolarg[thr]);
//     #endif
//   } // All threads started...now we wait for them to complete
//   for (thr = 0; thr < nthreads; thr++) Wait_thread(threads[thr]);
   return 0;
}

FUNCPTR convolvex(void *arg)
{
	ConvolArg *parg=(ConvolArg *) arg ;

	__m128 *mptr1,*mptr2,*mptr3;
	__m128 coef;
	float norm;
	int fsize,fsize2;
	float *filter;
	__m128 *mfilterx;
	int mask,mask2,x,y,z;
	float ***out;
	float ***image;

	mptr1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	
	// X convolve
	fsize=parg->psf->kernel_size;
	filter=parg->psf->kernel;
	mfilterx=(__m128 *) _alloca(fsize*sizeof(__m128));
	fsize2 = (fsize-1)/2;
	
	out=parg->out;
	image=parg->ima;

	for(mask=0;mask<fsize;mask++) mfilterx[mask]=_mm_set_ps1(filter[mask]);
	for(x=parg->start;x<parg->end;x++){
		if(cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			mptr3=(__m128 *) out[x][y];
			memset(mptr1,0,z_pixels_simd*sizeof(__m128));
			norm=0;
			for(mask=-fsize2;mask<fsize2;mask++){
				coef=mfilterx[mask+fsize2];
				mask2=x+mask;
				if(mask2<cylwiny[y][0] || mask2>=cylwiny[y][1])	continue;
				else norm+=filter[mask+fsize2];
				mptr2=(__m128 *) image[mask2][y];
				for(z=0;z<z_pixels_simd;z++){
					mptr1[z]=_mm_add_ps(mptr1[z],_mm_mul_ps(mptr2[z],coef));
				}
			}
			if(norm!=1 && norm!=0){
				coef=_mm_set_ps1(1/norm);
				for(z=0;z<z_pixels_simd;z++){
					mptr3[z]=_mm_mul_ps(mptr1[z],coef);
				}
			} else	memcpy(mptr3,mptr1,z_pixels_simd*sizeof(__m128));
		}
	}
	return (FUNCPTR) 1;
}

FUNCPTR convolvey(void *arg)
{
	ConvolArg *parg=(ConvolArg *) arg ;

	__m128 *mptr1,*mptr2,*mptr3;
	__m128 coef;
	float norm;
	int fsize,fsize2;
	float *filter;
	__m128 *mfilterx;
	int mask,mask2,x,y,z;
	float ***out;
	float ***image;

	mptr1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	
	// Y convolve
	fsize=parg->psf->kernel_size;
	filter=parg->psf->kernel;
	mfilterx=(__m128 *) _alloca(fsize*sizeof(__m128));
	fsize2 = (fsize-1)/2;
	
	out=parg->out;
	image=parg->ima;

	for(mask=0;mask<fsize;mask++) mfilterx[mask]=_mm_set_ps1(filter[mask]);
	for(x=parg->start;x<parg->end;x++){
		if(cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			mptr3=(__m128 *) out[x][y];
			memset(mptr1,0,z_pixels_simd*sizeof(__m128));
			norm=0;
			for(mask=-fsize2;mask<fsize2;mask++){
				coef=mfilterx[mask+fsize2];
				mask2=y+mask;
				if(mask2<cylwiny[x][0] || mask2>=cylwiny[x][1])	continue;
				else norm+=filter[mask+fsize2];
				mptr2=(__m128 *) image[x][mask2];
				for(z=0;z<z_pixels_simd;z++){
					mptr1[z]=_mm_add_ps(mptr1[z],_mm_mul_ps(mptr2[z],coef));
				}
			}
			if(norm!=1 && norm!=0){
				coef=_mm_set_ps1(1/norm);
				for(z=0;z<z_pixels_simd;z++){
					mptr3[z]=_mm_mul_ps(mptr1[z],coef);
				}
			} else	memcpy(mptr3,mptr1,z_pixels_simd*sizeof(__m128));
		}
	}
	return (FUNCPTR) 1;
}

FUNCPTR convolvez(void *arg)
{
	ConvolArg *parg=(ConvolArg *) arg ;

	__m128 *mptr1,*mptr2,*mptr3;
	__m128 coef;
	float norm;
	int fsize,fsize2;
	float *filter;
	__m128 *mfilterx;
	int mask,mask2,x,y,z;
	float ***out;
	float ***image;
	float *normz;
	float *ptr1;
	float *ptr2;
	float fvalue;
	int zstart,zend;
	int z2;

	mptr1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	normz=(float  *) _alloca(z_pixels_simd*sizeof(__m128));
	// X convolve
	fsize=psfz->kernel_size;
	filter=psfz->kernel;
	fsize2 = (fsize-1)/2;

	out=parg->out;
	image=parg->ima;

	for(z=0;z<z_pixels_simd*4;z++){
		normz[z]=0;
	}
	for(mask=-fsize2;mask<fsize2;mask++){
		zstart=z_min-mask;
		if(zstart<0) zstart=0;
		zend=z_max-mask;
		if(zend>z_pixels) zend=z_pixels;
		z2=mask+zstart;
		for(z=0;z<z_pixels_simd*4;z++){
			mask2=z+mask;
			if(mask2<z_min || mask2>=z_max){
				continue;
			}
			else normz[z]+=filter[mask+fsize2];
		}
	}

	for(z=0;z<z_pixels_simd*4;z++){
		if(normz[z]>0) normz[z]=1/normz[z];
	}

	ptr1=(float *) mptr1;
	mptr2=(__m128 *) normz;
	for(x=parg->start;x<parg->end;x++){
		if(cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			ptr2=(float *) image[x][y];
			mptr3=(__m128 *) out[x][y];
			memset(ptr1,0,z_pixels_simd*sizeof(__m128));
			
			for(mask=-fsize2;mask<fsize2;mask++){
				zstart=z_min-mask;
				if(zstart<0) zstart=0;
				zend=z_max-mask;
				if(zend>z_pixels) zend=z_pixels;
				z2=mask+zstart;
				fvalue=filter[mask+fsize2];
				for(z=zstart;z<zend;z++,z2++){
					ptr1[z]+=ptr2[z2]*fvalue;
				}
			}
			for(z=0;z<z_pixels_simd;z++){
				mptr3[z]=_mm_mul_ps(mptr1[z],mptr2[z]);
			}
		}
	}
	return (FUNCPTR) 1;
}

FUNCPTR convolveyz(void *arg)
{
	ConvolArgXYZ *parg=(ConvolArgXYZ *) arg ;
	ConvolArg *pargx,*pargy,*pargz;

	__m128 *mptr1,*mptr2,*mptr3,*mnormz;
	__m128 coef;
	float norm;
	int fsize,fsize2;
	int fsizez,fsizez2;
	float *filterz;
	float *filter;
	__m128 *mfilterx;
	int mask,mask2,x,y,z;
	float ***out;
	float ***image;
	
	float *normz;
	float *ptr1;
	float *ptr2;
	float *ptr3;
	float fvalue;
	int zstart,zend;
	int z2;

	pargx=parg->x;
	pargy=parg->y;
	pargz=parg->z;

	mptr1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	normz=(float  *) _alloca(z_pixels_simd*sizeof(__m128));
	// Y convolve
	fsize=pargy->psf->kernel_size;
	filter=pargy->psf->kernel;
	mfilterx=(__m128 *) _alloca(fsize*sizeof(__m128));
	fsize2 = (fsize-1)/2;

	fsizez=pargz->psf->kernel_size;
	filterz=pargz->psf->kernel;
	fsizez2 = (fsize-1)/2;


	out=pargy->out;
	image=pargy->ima;
	ptr1=(float *) mptr1;
	mnormz=(__m128 *) normz;

	memset(normz,0,z_pixels_simd*sizeof(__m128));

	for(mask=-fsizez2;mask<fsizez2;mask++){
		zstart=z_min-mask;
		if(zstart<0) zstart=0;
		zend=z_max-mask;
		if(zend>z_pixels) zend=z_pixels;
		z2=mask+zstart;
		for(z=0;z<z_pixels_simd*4;z++){
			mask2=z+mask;
			if(mask2<z_min || mask2>=z_max){
				continue;
			}
			else normz[z]+=filterz[mask+fsize2];
		}
	}

	for(z=0;z<z_pixels_simd*4;z++){
		if(normz[z]>0) normz[z]=1/normz[z];
	}

	for(mask=0;mask<fsize;mask++) mfilterx[mask]=_mm_set_ps1(filter[mask]);

	for(x=pargy->start;x<pargy->end;x++){
		if(cylwiny[x][1]==0) continue;
		for(y=cylwiny[x][0];y<cylwiny[x][1];y++){
			mptr3=(__m128 *) out[x][y];
			memset(mptr1,0,z_pixels_simd*sizeof(__m128));
			norm=0;
			for(mask=-fsize2;mask<fsize2;mask++){
				coef=mfilterx[mask+fsize2];
				mask2=y+mask;
				if(mask2<cylwiny[x][0] || mask2>=cylwiny[x][1])	continue;
				else norm+=filter[mask+fsize2];
				mptr2=(__m128 *) image[x][mask2];
				for(z=0;z<z_pixels_simd;z++){
					mptr1[z]=_mm_add_ps(mptr1[z],_mm_mul_ps(mptr2[z],coef));
				}
			}
			
			memset(mptr3,0,z_pixels_simd*sizeof(__m128));
			ptr3=(float *)mptr3;

			if(norm!=0){
				for(mask=-fsize2;mask<fsize2;mask++){
					zstart=z_min-mask;
					if(zstart<0) zstart=0;
					zend=z_max-mask;
					if(zend>z_pixels) zend=z_pixels;
					z2=mask+zstart;
					fvalue=filter[mask+fsize2];
					for(z=zstart;z<zend;z++,z2++){
						ptr3[z]+=ptr1[z2]*fvalue;
					}
				}
				coef=_mm_set_ps1(1/norm);

				for(z=0;z<z_pixels_simd;z++){
					mptr3[z]=_mm_mul_ps(mptr3[z],_mm_mul_ps(mnormz[z],coef));
				}
			} 
		}
	}
	return (FUNCPTR) 1;
}

void convolve3d(float ***image,float ***out,float ***buf)
{
	int x,y,z;
	int fsize;
	int fsize2;
	int s,e;
	float *filter;
	__m128 *mptr1;
	__m128 *mptr2;
	__m128 *mptr3;
	__m128 coef;
	__m128 *mfilterx;
	__m128 *mfiltery;
	float *ptr1;
	float *ptr2;
	float norm;
	float *normz;
	int start[MAXTHREADS];
	int end[MAXTHREADS];

	ConvolArg arg[MAXTHREADS];
	ConvolArg argx[MAXTHREADS];
	ConvolArg argy[MAXTHREADS];
	ConvolArg argz[MAXTHREADS];
	ConvolArgXYZ argxyz[MAXTHREADS];

	int thr;
	THREAD_TYPE handle[MAXTHREADS];
	unsigned id;

	mptr1=(__m128 *) _alloca(z_pixels_simd*sizeof(__m128));
	normz=(float  *) _alloca(z_pixels_simd*sizeof(__m128));
	
	s=e=0;
	for(thr=0;thr<nthreads;thr++){
		s+=start_x__per_thread[thr];
		e+=start_x__per_thread[thr+1];
		start[thr]=s;
		end[thr]=e;
		if(thr==nthreads-1) end[thr]=x_pixels;
	}

	for(thr=0;thr<nthreads;thr++){
		arg[thr].ima=image;
		arg[thr].out=buf;
		arg[thr].psf=psfx;
		arg[thr].start=start[thr];
		arg[thr].end=end[thr];
		START_THREAD(handle[thr],convolvex,arg[thr],id);
	}
	for(thr=0;thr<nthreads;thr++) Wait_thread(handle[thr]);

	for(thr=0;thr<nthreads;thr++){
		argy[thr].ima=buf;
		argy[thr].out=out;
		argy[thr].psf=psfy;
		argy[thr].start=start[thr];
		argy[thr].end=end[thr];

		argz[thr]=argy[thr];
		argz[thr].psf=psfz;
		argxyz[thr].y=&argy[thr];
		argxyz[thr].z=&argz[thr];

		START_THREAD(handle[thr],convolveyz,argxyz[thr],id);
	}
	for(thr=0;thr<nthreads;thr++) Wait_thread(handle[thr]);

	//for(thr=0;thr<nthreads;thr++){
	//	arg[thr].ima=buf;
	//	arg[thr].out=out;
	//	arg[thr].psf=psfy;
	//	arg[thr].start=thr*x_pixels/nthreads;
	//	arg[thr].end=(thr+1)*x_pixels/nthreads;
	//	if(thr==nthreads-1) arg[thr].end=x_pixels;
	//	START_THREAD(handle[thr],convolvey,arg[thr],id);
	//}
	//for(thr=0;thr<nthreads;thr++) Wait_thread(handle[thr]);

	//for(thr=0;thr<nthreads;thr++){
	//	arg[thr].ima=out;
	//	arg[thr].out=out;
	//	arg[thr].psf=psfz;
	//	arg[thr].start=thr*x_pixels/nthreads;
	//	arg[thr].end=(thr+1)*x_pixels/nthreads;
	//	if(thr==nthreads-1) arg[thr].end=x_pixels;
	//	START_THREAD(handle[thr],convolvez,arg[thr],id);
	//}
	//for(thr=0;thr<nthreads;thr++) Wait_thread(handle[thr]);

}

int convolx(float ***image, int thr, int start, int end, Psf1D *psf)
{
     int x,y,z;
     static float* vdatax[MAXTHREADS];  // asume they are null since static
     static float* vdatax_sm[MAXTHREADS]; 
// alternative - MS
// static float** vdatax=NULL;
// if(!vdatax) calloc(MAXTHREADS,sizeof(float*);
     if (!vdatax[thr]) {	// allocate iif undefined
	vdatax[thr]    = (float *) calloc(x_pixels,sizeof(float));
   	vdatax_sm[thr] = (float *) calloc(x_pixels,sizeof(float));
        printf(" vectors allocated for thread %d\n",thr);
     }
     float *datax = vdatax[thr]; 
     float *datax_sm = vdatax_sm[thr];
     for (z=start;z<end;z++) for (y=0;y<y_pixels;y++) {  
        for (x=0;x<x_pixels;x++) datax[x] = image[x][y][z];
        convolve1d(datax,datax_sm,x_pixels,psf);
        for (x=0;x<x_pixels;x++) image[x][y][z] = datax_sm[x];
     }
     return 0;
}

int convoly(float ***image, int thr, int start,int end, Psf1D *psf)
{
     int x,y,z;
     static float* vdatay[MAXTHREADS];
     static float* vdatay_sm[MAXTHREADS];
     if (!vdatay[thr]) {	// allocate iif undefined
	vdatay[thr]    = (float *) calloc(y_pixels,sizeof(float));
   	vdatay_sm[thr] = (float *) calloc(y_pixels,sizeof(float));
     }
     float *datay = vdatay[thr]; 
     float *datay_sm = vdatay_sm[thr];
     for (z=start;z<end;z++) for (x=0;x<x_pixels;x++) {
        for (y=0;y<y_pixels;y++) datay[y] = image[x][y][z];
        convolve1d(datay,datay_sm,y_pixels,psf);
        for (y=0;y<y_pixels;y++) image[x][y][z] = datay_sm[y];
     }	
     return 0;
}

int convolz(float ***image, int thr, int start, int end, Psf1D *psf)
{
   int x,y,z;
   static float* vdataz[MAXTHREADS];
   static float* vdataz_sm[MAXTHREADS];
   if (!vdataz[thr]) {	// allocate iif undefined
   	vdataz[thr] = (float *) calloc(z_pixels,sizeof(float));
   	vdataz_sm[thr] = (float *) calloc(z_pixels,sizeof(float));
   }
   float *dataz = vdataz[thr]; 
   float *dataz_sm = vdataz_sm[thr];
   for (y=start;y<end;y++) for (x=0;x<x_pixels;x++) {
        for (z=0;z<z_pixels;z++) dataz[z] = image[x][y][z];
        convolve1d(dataz,dataz_sm,z_pixels,psf);
        for (z=0;z<z_pixels;z++) image[x][y][z] = dataz_sm[z];
   }	
   return 0;
}
