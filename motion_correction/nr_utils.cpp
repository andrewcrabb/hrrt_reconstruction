
#include <stdio.h>
#include "nr_utils.h"
#include "mm_malloc.h"
#include <xmmintrin.h>

unsigned int memoryused=0;

void nrerror( const char error_text[])
{
	fprintf(stderr,"run-time error ... \n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...exiting ...\n");
	exit(1);
}

float	*vector(int nl, int nh)
{
	float *v;
	v = (float *)_mm_malloc( (size_t) ( (nh-nl+1+NR_END)*sizeof(float) ),16 );
	if( !v ) nrerror("allocation failure in vector() ");
	memoryused+=(nh-nl)*sizeof(float);
	////printf("vector memory used %d\n",memoryused/1024);
	return v-nl+NR_END;
}


float	***matrix3d(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	float	***t;

	//printf("allocated %d kbytes %d\n",(nrow*ncol*ndep+NR_END)/256,NR_END);
	t=(float ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(float**) ) ,16);
	if( !t ) nrerror("Allocation failure 1 in matrix3d() ");  
	t += NR_END;
	t -=nrl;

	t[nrl] = (float **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(float *) ) ,16);
	if( !t[nrl] ) nrerror("Allocation failure 2 in matrix3d() ");  
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (float *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(float) ),16 );
	if( !t[nrl][ncl] ) nrerror("Allocation failure 3 in matrix3d() ");  
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(float);

	//printf("float 3d memory used %d\n",memoryused/1024);
	return 	t;

}

float	**matrixfloat( int nrl, int nrh, int ncl, int nch  )
{
	int 	i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
	float	**m;

	m = (float **) _mm_malloc( (size_t) ( (nrow+NR_END) * sizeof(float*)  ),16 );
	if( !m ) nrerror("Allocation failure 1 in matrix() ");
	m += NR_END;
	m -= nrl;

	m[nrl] = (float *) _mm_malloc( (size_t)( (nrow*ncol+NR_END) * sizeof(float) ) ,16);
	if( !m[nrl] ) nrerror("Allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1; i<=nrh; i++) m[i]=m[i-1]+ncol;
	memoryused+=(nrh-nrl)*(nch-ncl)*sizeof(float);

	//printf("float 2d memory used %d\n",memoryused/1024);
	return m;
}


__m128	**matrixm128( int nrl, int nrh, int ncl, int nch  )
{
	int 	i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
	__m128	**m;

	//printf("allocated %d kbytes %d\n",(nrow*ncol+NR_END)/64,NR_END);

	m = (__m128 **) _mm_malloc( (size_t) ( (nrow+NR_END) * sizeof(__m128*)  ),16 );
	if( !m ) nrerror("Allocation failure 1 in matrix() ");
	m += NR_END;
	m -= nrl;

	m[nrl] = (__m128 *) _mm_malloc( (size_t)( (nrow*ncol+NR_END) * sizeof(__m128) ) ,16);
	if( !m[nrl] ) nrerror("Allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1; i<=nrh; i++) m[i]=m[i-1]+ncol;
	memoryused+=(nrh-nrl)*(nch-ncl)*sizeof(__m128);
	//printf("m128 2d memory used %d\n",memoryused/1024);

	return m;
}


short	**matrixshort( int nrl, int nrh, int ncl, int nch  )
{
	int 	i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
	short	**m;

	m = (short**) _mm_malloc( (size_t) ( (nrow+NR_END) * sizeof(short*)  ),16 );
	if( !m ) nrerror("Allocation failure 1 in matrix() ");
	m += NR_END;
	m -= nrl;

	m[nrl] = (short*) _mm_malloc( (size_t)( (nrow*ncol+NR_END) * sizeof(short) ) ,16);
	if( !m[nrl] ) nrerror("Allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1; i<=nrh; i++) m[i]=m[i-1]+ncol;
	memoryused+=(nrh-nrl)*(nch-ncl)*sizeof(short);

	//printf("short 2d memory used %d\n",memoryused/1024);
	return m;
}

float	***matrixfloatptr(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
	float	***m;

	m = (float ***) _mm_malloc( (size_t) ( (nrow+NR_END) * sizeof(float**)  ),16 );
	if( !m ) nrerror("Allocation failure 1 in matrix() ");
	m += NR_END;
	m -= nrl;

	m[nrl] = (float **) _mm_malloc( (size_t)( (nrow*ncol+NR_END) * sizeof(float *) ) ,16);
	if( !m[nrl] ) nrerror("Allocation failure 2 in matrix()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1; i<=nrh; i++) m[i]=m[i-1]+ncol;
	memoryused+=(nrh-nrl)*(nch-ncl)*sizeof(float);

	//printf("float 2d memory used %d\n",memoryused/1024);
	return m;
}

char	***matrix3dchar(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	char	***t;

	//printf("allocated %d kbytes %d\n",(nrow*ncol*ndep+NR_END)/256,NR_END);
	t=(char ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(char**) ) ,16);
	if( !t ) nrerror("Allocation failure 1 in matrix3d() ");  
	t += NR_END;
	t -=nrl;

	t[nrl] = (char **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(char *) ) ,16);
	if( !t[nrl] ) nrerror("Allocation failure 2 in matrix3d() ");  
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (char *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(char) ),16 );
	if( !t[nrl][ncl] ) nrerror("Allocation failure 3 in matrix3d() ");  
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(char);

	//printf("char 3d memory used %d\n",memoryused/1024);
	return 	t;

}
unsigned char ***matrix3duchar(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	unsigned char	***t;

	//printf("allocated %d kbytes %d\n",(nrow*ncol*ndep+NR_END)/256,NR_END);
	t=(unsigned char ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(unsigned char**) ) ,16);
	if( !t ) nrerror("Allocation failure 1 in matrix3d() ");  
	t += NR_END;
	t -=nrl;

	t[nrl] = (unsigned char **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(unsigned char *) ) ,16);
	if( !t[nrl] ) nrerror("Allocation failure 2 in matrix3d() ");  
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (unsigned char *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(unsigned char) ),16 );
	if( !t[nrl][ncl] ) nrerror("Allocation failure 3 in matrix3d() ");  
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(unsigned char);

	//printf("char 3d memory used %d\n",memoryused/1024);
	return 	t;

}

short	***matrix3dshort(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	short	***t;

	//printf("allocated %d kbytes\n",nrow*ncol*ndep/256);
	t=(short ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(short**) ) ,16);
	if( !t ) nrerror("Allocation failure 1 in matrix3d() ");  
	t += NR_END;
	t -=nrl;

	t[nrl] = (short **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(short *) ) ,16);
	if( !t[nrl] ) nrerror("Allocation failure 2 in matrix3d() ");  
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (short *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(short) ),16 );
	if( !t[nrl][ncl] ) nrerror("Allocation failure 3 in matrix3d() ");  
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(short);

	//printf("shrot 3d memory used %d\n",memoryused/1024);
	return 	t;

}

__m128	***matrix3dm128(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	__m128	***t;

	//printf("allocated %d kbytes\n",(nrow*ncol*ndep+NR_END)/64);

	t=(__m128 ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(__m128**) ) ,16);
	if( !t ) nrerror("Allocation failure 1 in matrix3dm128() ");  
	t += NR_END;
	t -=nrl;

	t[nrl] = (__m128 **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(__m128 *) ) ,16);
	if( !t[nrl] ) nrerror("Allocation failure 2 in matrix3dm128() ");  
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (__m128 *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(__m128) ),16 );
	if( !t[nrl][ncl] ) nrerror("Allocation failure 3 in matrix3dm128() ");  
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(__m128);

	//printf("m128 3d memory used %d\n",memoryused/1024);
	return 	t;
}
__m128	***matrix3dm128_check(int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	int 	i,j,nrow=nrh-nrl+1, ncol=nch-ncl+1, ndep=ndh-ndl+1;
	__m128	***t;

	//printf("allocated %d kbytes\n",(nrow*ncol*ndep+NR_END)/64);

	t=(__m128 ***) _mm_malloc((size_t) ( (nrow+NR_END)*sizeof(__m128**) ) ,16);
	if( !t )return NULL;  
	t += NR_END;
	t -=nrl;

	t[nrl] = (__m128 **) _mm_malloc( (size_t) ( (nrow*ncol+NR_END)*sizeof(__m128 *) ) ,16);
	if( !t[nrl] ) {
		//		_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
		//		_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
		_mm_free( (FREE_ARG) (t+nrl-NR_END) );
		return NULL;
	}
	t[nrl] += NR_END;
	t[nrl] -=ncl;

	t[nrl][ncl] = (__m128 *) _mm_malloc( (size_t) ( (nrow*ncol*ndep+NR_END)*sizeof(__m128) ),16 );
	if( !t[nrl][ncl] ){
		_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
		_mm_free( (FREE_ARG) (t+nrl-NR_END) );
		return NULL;
	}
	t[nrl][ncl] += NR_END;
	t[nrl][ncl] -=ndl;

	for(j=ncl+1; j<=nch; j++) t[nrl][j] = t[nrl][j-1]+ndep;
	for(i=nrl+1; i<=nrh; i++) 
	{  t[i]=t[i-1]+ncol;
	t[i][ncl]=t[i-1][ncl]+ncol*ndep;
	for(j=ncl+1;j<=nch;j++)  t[i][j]=t[i][j-1]+ndep;
	}
	memoryused+=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(__m128);

	//printf("m128 3d memory used %d\n",memoryused/1024);
	return 	t;

}

void	free_vector(float *v, int nl, int nh)
{
	_mm_free( (FREE_ARG) (v+nl-NR_END) );
	memoryused-=(nh-nl)*sizeof(float);
	//printf("vector free memory used %d\n",memoryused/1024);
}

void	free_matrix(float **m, int nrl, int nrh, int ncl, int nch)
{
	_mm_free( (FREE_ARG) (m[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (m+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*sizeof(float);
	//printf("float 2d free memory used %d\n",memoryused/1024);
}

void	free_matrixm128(float **ptr, int nrl, int nrh, int ncl, int nch)
{
	__m128 **m=(__m128**)ptr;
	_mm_free( (FREE_ARG) (m[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (m+nrl-NR_END) );

	memoryused-=(nrh-nrl)*(nch-ncl)*sizeof(__m128);
	//printf("m128 2d free memory used %d\n",memoryused/1024);
}

void	free_matrix3d(float ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
	_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (t+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(float);
	//printf("float 3d free memory used %d\n",memoryused/1024);

}
void	free_matrix3dchar(char ***t, int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
	_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (t+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(char);
	//printf("char 3d free memory used %d\n",memoryused/1024);

}
void	free_matrix3dm128(float ***ptr, int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	__m128 ***t=(__m128 ***)ptr;
	_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
	_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (t+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(__m128);
	//printf("m128 3d free memory used %d\n",memoryused/1024);
}
void	free_matrix3dshortm128(short ***ptr, int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	__m128 ***t=(__m128 ***)ptr;
	_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
	_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (t+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(__m128);
	//printf("m128 3d free memory used %d\n",memoryused/1024);
}
void	free_matrix3dshort(short ***ptr, int nrl, int nrh, int ncl, int nch, int ndl, int ndh)
{
	short ***t=(short ***)ptr;
	_mm_free( (FREE_ARG) (t[nrl][ncl]+ndl-NR_END) );
	_mm_free( (FREE_ARG) (t[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (t+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*(ndh-ndl)*sizeof(short);
	//printf("m128 3d free memory used %d\n",memoryused/1024);
}

short int  **matrix_i( int nrl, int nrh, int ncl, int nch  )
{
	int 	i, nrow=nrh-nrl+1, ncol=nch-ncl+1;
	short int	**m;

	m = (short int **) _mm_malloc( (size_t) ( (nrow+NR_END) * sizeof(short int *)  ),16 );
	if( !m ) nrerror("Allocation failure 1 in matrix_i() ");
	m += NR_END;
	m -= nrl;

	m[nrl] = (short int *) _mm_malloc( (size_t)( (nrow*ncol+NR_END) * sizeof(short int) ) ,16);
	if( !m[nrl] ) nrerror("Allocation failure 2 in matrix_i()");
	m[nrl] += NR_END;
	m[nrl] -= ncl;

	for(i=nrl+1; i<=nrh; i++) m[i]=m[i-1]+ncol;

	memoryused+=(nrh-nrl)*(nch-ncl)*sizeof(short int);
	//printf("short int 2d memory used %d %d %d %d %d\n",memoryused/1024,nrl,nrh,ncl,nch);
	return m;
}

void	free_matrix_i(short int **m, int nrl, int nrh, int ncl, int nch)
{
	_mm_free( (FREE_ARG) (m[nrl]+ncl-NR_END) );
	_mm_free( (FREE_ARG) (m+nrl-NR_END) );
	memoryused-=(nrh-nrl)*(nch-ncl)*sizeof(short int);
	//printf("short free memory used %d\n",memoryused/1024);
}
