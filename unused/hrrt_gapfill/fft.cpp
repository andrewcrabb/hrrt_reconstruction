#include <cstdio>
#include <cmath>
#include "fft.h"

/*FFT routines for real input functions. Translated from Pascal. January 26, 1994 */

#define	maxn	1024
#define	tmpsize	50

int FFT_initialize(int n, int nu, float* data_cos)
{
    float dpi = 2*F_PI;
    int i,j;
    float angle,incr;


        /*check first that 2**nu = n*/
        i=1;
        for(j=1;j<=nu;++j) i *= 2;
        if(i != n) {
           printf("FFT-initialize : 2**nu should be equal to n\n");
           return 0;
        }
        incr  = dpi/n;
        angle = 0.0;
        if((n % 4) != 0) {

	    printf("FFT-initialize : n should be multiple of 4\n");
            return 0;
        }
        else {
        for(i= 1; i<= (int) (n/4);++i) {
            angle += incr;
            data_cos[i] = cos(angle);
        }
        return 1;
        }
    }

int ibitr(int j, int nu)
    /* bit reversed value of j for n=2**nu */
    {
        int value,j1,i;
        j1=j;
        value=0;
        for(i=1;i<=nu;++i) {
            value = value*2 + j1%2;
            j1 = j1/2;
        }
        return value;
    } /*end ibitr*/

int FFT_real_direct(float x[], int n, int nu, int ntot, int nstep)
    /* written on 85/12/10 to allow the possibility of multidimensional real FFT 
      n : 2**nu, dimension of the current FFT 
      ntot : total dimension of the working array
      nstep : factor multiplying the index for the current FFT */
    {
        int np,kp,ndiv4;
        int i,l,nl,nh,k1,k2,k3,k4,nr,nparg;
        float* data_cos;
        float c,s,a,b;
        int istep,jstep,nlarge,nj,add;          /* for multidimensional FFT */

        // Create array
        if((data_cos = new float[1024]) == 0) 
        {
          printf("Error: memory allocation failed");
  	      return 1;
        }        
    
        /* the values S[l,np,n] are stored in x[n+ibitr(np,nu)] for each value of l.
          S[l,np,n] is the real part of R[l,np,n] when np=0..2**(l-1)
                           imag part of R[l,2**l-np,n] when np=2**(l-1)+1...2**l -1
          When l=0 S[0,0,n] is the real input vector
          When l=nu s[nu,np,0] is the real part of the dft for np<=n/2
                                      imag part of the dft for np> n/2 
          For multidimensional FFT, x(l) is stored in x(jstep*nlarge+l*nstep+istep)
          and every operation is performed for jstep = 0..nj-1 and istep= 0..nstep-1 */
        if(FFT_initialize(n,nu,data_cos) != 1) return 0;
        ndiv4 = n/4;
        nlarge = n*nstep;                              /* for multidimensional FFT */
        nj = ntot/nlarge;                          /* for multidimensional FFT */
        nh=n;
        nl=1;
        for(l=1;l<=nu;++l)
            { 
            /* during each loop over l, nh=2**(nu-l) and nl=2**(l-1) */
            nh=nh/2; /* np=0; */
            for(nr=0;nr<nh;++nr)
                {
                k1=nr+nh;
                for(jstep=0;jstep<nj;++jstep)
                    for(istep=0;istep<nstep;++istep)
                        {
                        add = jstep*nlarge + istep;
                        a=x[nr*nstep+add];
                        b=x[k1*nstep+add];
                        x[nr*nstep+add]=a+b;
                        x[k1*nstep+add]=a-b;
                        }
                }
            np=1; 
            nparg=nh;
            while( (2*np) < nl)
            { 
                kp=ibitr(np,nu);
                /* arg=pi*np/nl; */ 
                c=data_cos[nparg];
                s=data_cos[ndiv4-nparg];
                for(nr=0; nr<nh;++nr)
                    { 
                    k1=nr+kp;
                    k2=ibitr(nl-np,nu)+nr;
                    k3=k1+nh;
                    k4=k2+nh;
                    for(jstep=0;jstep<nj;++jstep)
                        for(istep=0;istep<nstep;++istep)
                            {
                            add = jstep*nlarge + istep;
                            a=c*x[k3*nstep+add]+s*x[k4*nstep+add];
                            b=-s*x[k3*nstep+add]+c*x[k4*nstep+add];
                            x[k3*nstep+add]=-x[k2*nstep+add]+b;
                            x[k4*nstep+add]= x[k2*nstep+add]+b;
                            x[k2*nstep+add]=x[k1*nstep+add]-a;
                            x[k1*nstep+add]=x[k1*nstep+add]+a;
                            }
                    }
                np=np+1;
                nparg=nparg+nh;
            }

            np=nl/2;
            kp=ibitr(np,nu);
            if(np>0)  
                for(nr=0;nr<nh;++nr)
                    for(jstep=0;jstep<nj;++jstep)
                        for(istep=0;istep<nstep;++istep)
                            {
                            add = jstep*nlarge + istep;
                            x[(nr+kp+nh)*nstep+add]=-x[(nr+kp+nh)*nstep+add];
                            }
            nl=nl*2;
        }

    /* shuffling */
        for(i=0;i<n;++i)
            {
            kp=ibitr(i,nu);
            if(kp>i)
                for(jstep=0;jstep<nj;++jstep)
                    for(istep=0;istep<nstep;++istep)
                        {
                        add = jstep*nlarge + istep;
                        a=x[i*nstep+add];
                        x[i*nstep+add]=x[kp*nstep+add];
                        x[kp*nstep+add]=a;
                        }
            }
       delete[] data_cos; 
       return 1;
    } /*FFT_real_direct*/

int FFT_real_inverse(float x[], int n, int nu, int ntot, int nstep)
{
        int np,kp,ndiv4;
        int i,l,nl,nh,k1,k2,k3,k4,nr,nparg;
        float c,s,a,b, *data_cos;
        int istep,jstep,nlarge,nj,add;          /* for multidimensional FFT */

        // Create array
        if((data_cos = new float[1024]) == 0) 
        {
          printf("Error: memory allocation failed");
  	      return 1;
        }  

        if(FFT_initialize(n,nu, data_cos) != 1) return 0;

        ndiv4 = n/4;
        nlarge = n*nstep;                              /* for multidimensional FFT */
        nj = ntot/nlarge;                          /* for multidimensional FFT */

    /* shuffling */
        for(i=0;i<n;++i)
            {
            kp=ibitr(i,nu);
            if(kp>i)
                for(jstep=0;jstep<nj;++jstep)
                    for(istep=0;istep<nstep;++istep)
                        {
                        add = jstep*nlarge + istep;
                        a=x[i*nstep+add];
                        x[i*nstep+add]=x[kp*nstep+add];
                        x[kp*nstep+add]=a;
                        }
            }

        for(add=0;add<ntot;++add) x[add]=x[add]/n;

    /* the values S[l,np,n] are stored in x[n+ibitr(np,nu)] for each value of l.
      S[l,np,n] is the real part of R[l,np,n] when np=0..2**(l-1)
                       imag part of R[l,2**l-np,n] when np=2**(l-1)+1...2**l -1
      When l=0 S[0,0,n] is the real input vector
      When l=nu s[nu,np,0] is the real part of the dft for np<=n/2
                                  imag part of the dft for np> n/2 */
        nh=1;
        nl=n;
        for(l=nu;l>=1;--l)
            { 
            /* during each loop over l, nh=2**(nu-l) and nl=2**(l-1) */
            nl=nl/2; /* np=0; */
            for(nr=0;nr<nh;++nr)
                {
                k1=nr+nh;
                for(jstep=0;jstep<nj;++jstep)
                    for(istep=0;istep<nstep;++istep)
                        {
                        add = jstep*nlarge + istep;
                        a=x[nr*nstep+add];
                        b=x[k1*nstep+add];
                        x[nr*nstep+add]=(a+b);
                        x[k1*nstep+add]=(a-b);
                        }
                }
                np=1;
                nparg=nh;
                while((2*np) < nl)
                    {
                    kp=ibitr(np,nu);
                    /* arg=pi*np/nl; */ 
                    c=data_cos[nparg];
                    s=data_cos[ndiv4-nparg];
                    for(nr=0;nr<nh;++nr)
                        {
                        k1=nr+kp;
                        k2=ibitr(nl-np,nu)+nr;
                        k3=k1+nh;
                        k4=k2+nh;
                        for(jstep=0;jstep<nj;++jstep)
                            for(istep=0;istep<nstep;++istep)
                                {
                                add = jstep*nlarge + istep;
                                a=x[k1*nstep+add]-x[k2*nstep+add];
                                b=x[k3*nstep+add]+x[k4*nstep+add];
                                x[k1*nstep+add]=x[k1*nstep+add]+x[k2*nstep+add];
                                x[k2*nstep+add]=-x[k3*nstep+add]+x[k4*nstep+add];
                                x[k3*nstep+add]=c*a-s*b; 
                                x[k4*nstep+add]=s*a+c*b;
                                }
                        }
                        np=np+1;
                        nparg=nparg+nh;
                    }
                    np=nl/2;
                    kp=ibitr(np,nu);
                    if(np>0)  
                        for(nr=0;nr<nh;++nr)
                            for(jstep=0;jstep<nj;++jstep)
                                for(istep=0;istep<nstep;++istep)
                                    {
                                    add = jstep*nlarge + istep;
                                    x[(nr+kp+nh)*nstep+add]=-2*x[(nr+kp+nh)*nstep+add] ;
                                    x[(nr+kp)*nstep+add]=2*x[(nr+kp)*nstep+add];
                                    }
                    nh=nh*2;
            }
            
       delete[] data_cos; 

     return 1;
    } /*FFT_real_inverse*/


int spomf(float a[], float b[], float c[], float w[], int nx, int ny, int mode)
    /*phase-only matched filtering of two 2D images (mode = 1)*/
    /*correlation of two 2D images (mode = 0)*/
    /*a and b contain the 2D FFT of the two images */
    /*w contains an apodising window*/
    /*c is the output FFT, which must be inverse FFTed*/
    /*to get the correlation image*/

    {
	float cc,cc1,ss,ss1,cs,cs1,sc,sc1;
	/* below : r : real part, i : imaginary part, 1 : second image */
	/* pp : positive x and y frequency, pm : positive x, negative y*/
	float rpp,rpp1,ipp,ipp1,rpm,rpm1,ipm,ipm1;
	/*product of the two FFT*/
	float qrpp,qrpm,qipp,qipm; 
	/* mod : modulus of the FFT*/
	float modpp,modpp1,modpm,modpm1;
	int x,y;


	/*Calculate the complex product of a and b(conj) */
   for(x=0;x<=(nx/2);++x)
      for(y=0;y<=(ny/2);++y) {
         cc = a[x*ny+y];
         cc1 = b[x*ny+y];
         if((x !=0) && (x != (nx/2))) {
            sc = a[(nx-x)*ny+y];
            sc1 = -b[(nx-x)*ny+y];
         }
         else {
            sc = 0.0;
            sc1 = 0.0;
         }
         if((y !=0) && (y != (ny/2))) {
            cs = a[x*ny + ny-y];
            cs1 = -b[x*ny + ny-y];
         }
         else {
            cs = 0.0;
            cs1 = 0.0;
         }
         if((x !=0) && (x != (nx/2)) && (y !=0) && (y != (ny/2))) {
            ss = a[(nx-x)*ny + ny-y];
            ss1 = b[(nx-x)*ny + ny-y];
         }
         else {
            ss = 0.0;
            ss1 = 0.0;
         }

	 rpp = cc - ss;
         rpm = cc + ss;
         rpp1 = cc1 - ss1;
         rpm1 = cc1 + ss1;
         ipp = cs + sc;
         ipm = -cs + sc;
         ipp1 = cs1 + sc1;
         ipm1 = -cs1 + sc1;
         qrpp = rpp*rpp1 - ipp*ipp1;
         qrpm = rpm*rpm1 - ipm*ipm1;
         qipp = rpp*ipp1 + ipp*rpp1;
         qipm = rpm*ipm1 + ipm*rpm1;
         modpp = sqrt(rpp*rpp + ipp*ipp);
         modpm = sqrt(rpm*rpm + ipm*ipm);
         modpp1 = sqrt(rpp1*rpp1 + ipp1*ipp1);
         modpm1 = sqrt(rpm1*rpm1 + ipm1*ipm1);
         if(mode == 1) {
            qrpp /= (modpp*modpp1);
            qipp /= (modpp*modpp1);
            qrpm /= (modpm*modpm1);
            qipm /= (modpm*modpm1);
         }
         c[x*ny + y] = (qrpp+qrpm)*w[x*ny + y];
         if((x !=0) && (x != (nx/2)))
            c[(nx-x)*ny + y] = (qipp+qipm)*w[x*ny + y];
         if((y !=0) && (y != (ny/2)))
            c[x*ny + ny-y] = (qipp-qipm)*w[x*ny + y];
         if((x !=0) && (x != (nx/2)) && (y !=0) && (y != (ny/2))) 
	/*corrected next line : June 29, 1994*/
            c[(nx-x)*ny + ny-y] = (-qrpp+qrpm)*w[x*ny + y];
   }

/*phase-shift to get the peak at the centre of the image */
for(x=0;x<nx;++x) 
   for(y=0;y<ny;++y) {
      if((x%2) != 0) c[x*ny+y] = - c[x*ny+y];
      if((y%2) != 0) c[x*ny+y] = - c[x*ny+y];
}

return 1;	/* added MS */

}

     

/*Multi-dimensional FFT routines for real input functions and arbitrary 
  dimension multiple of 4. Uses FFT_real_direct for the largest power of 
  two, and a slow DFT for the remaining factor. Efficient only if this 
  factor is small. March, 1995 Michel Defrise */

int analyse_dimension(int n, int* nl, int* nul, int* ns)
//int n;		/*input dimension */
//int *nl;	/*largest power of two <= n */
//int *nul;	/*nl = 2**nul */
//int *ns;	/*remaining factor : n=nl*ns */
{
int k,m;

k = n;
m = 0;
while( (k % 2) == 0) {
  k /= 2;
  m += 1;
}
*nl = n/k;
*nul = m;
*ns = k;
return 1;

}

int FFTa_initialize(int n, float* coslut)
{
float dpi = 2*F_PI;
int i;

   if(n > maxn)  {
     printf("FFTa-initialize : maximum dimension is %d\n",maxn);
     return 0;
   }
   /*only for the use of the same lookup table for cos and sin must
     n be multiple of 4 */
   if((n%4) != 0) {
     printf("FFTa-initialize : dimension must be multiple of 4");
     return 0;
   }
   for(i= 0; i< n;++i) coslut[i] = cos(dpi*i/n);
   
   return 0;
 }


int swapa(float x[],int n, int nl, int ns, int way, int ntot, int nstep)
/*way = 1 for direct DFT, -1 for inverse */
/* n dimensional FFT, x(l) is stored in x(jstep*nlarge+l*nstep+istep) */
{
int pd,pm,k;
int nlarge,nj;
int istep,jstep,add;
float tmp[maxn];

nlarge = n*nstep;                              
nj = ntot/nlarge;                          

for(jstep=0;jstep<nj;++jstep)
  for(istep=0;istep<nstep;++istep) {

add = jstep*nlarge + istep;
for(k=0;k<n;++k) tmp[k] = x[k*nstep+add];
if(way == 1) {
   for(pd=0;pd<ns;++pd)
     for(pm=0;pm<nl;++pm) x[(pd*nl + pm)*nstep+add] = tmp[pd + pm*ns];
}
else if(way == -1) {
   for(pd=0;pd<ns;++pd)
     for(pm=0;pm<nl;++pm) x[(pd + pm*ns)*nstep+add] = tmp[pd*nl + pm];
}
else {
   printf("Wrong value of variable way in swapa\n");
   return 0;
}
}/*multi-dimensional loops*/

return 0;
} /*end swapa */


int FFTa_real_direct(float a[], int n, int ntot, int nstep)
/* multi-dimensional real fft for arbitrary dimension */
{


int nl,ns,nul;
int i,r,k,l;
int nlarge,nj;
int istep,jstep,add;
float tmp1[tmpsize];
float tmp2[tmpsize];
float c1,c2,s1,s2, *coslut;
int n4;

nlarge = n*nstep;                              
nj = ntot/nlarge;  
n4=n/4;
i = analyse_dimension(n,&nl,&nul,&ns);
if(ns > tmpsize) {
  printf("Use larger tmp matrix in FFTa_real_direct\n");
  return -1;
}

FFT_real_direct(a,nl,nul,ntot,(ns*nstep)); /*use FFT for the largest power of 2 */
if( ns == 1)  return 1; /*dimension is a power of two */

        // Create array
        if((coslut = new float[maxn]) == 0) 
        {
          printf("Error: memory allocation failed");
  	      return 1;
        }  

 i = FFTa_initialize(n, coslut);


/* apply the DFT of dimension ns */

if((ns%2) == 0) {
   printf("FFTa : ns must be odd\n");
   delete[] coslut;
   return 0;
}

for(jstep=0;jstep<nj;++jstep)
  for(istep=0;istep<nstep;++istep) {

add = jstep*nlarge + istep;


/*k== 0*/
for(r=0;r<ns;++r) {
   tmp1[r] = a[r*nstep+add];
   a[r*nstep+add] = 0;
}
for(r=0;r<ns;++r) a[0*nstep+add] += tmp1[r];
for(l=1;l <= (ns-1)/2; ++l) {
   for(r=0;r<ns;++r) a[l*nstep+add]    += coslut[(r*l*nl)%n]*tmp1[r];
   for(r=0;r<ns;++r) a[(ns-l)*nstep+add] -= (-coslut[(n4+r*l*nl)%n])*tmp1[r];
}

for(k=1; k < nl/2;++k) {
   for(r=0;r<ns;++r) {
      tmp1[r] = a[(r + k*ns)*nstep+add];
      tmp2[r] = a[(r + (nl-k)*ns)*nstep+add];
      a[(r + k*ns)*nstep+add] = 0;
      a[(r + (nl-k)*ns)*nstep+add] = 0;
   }
   for(l=0;l <= (ns-1)/2;++l) {
      for(r=0;r<ns;++r) {
         c1 = coslut[(r*(k+l*nl))%n];
         s1 = -coslut[(n4+r*(k+l*nl))%n];
         a[(l + k*ns)*nstep+add] += (c1*tmp1[r] + s1*tmp2[r]);
         a[((ns-l-1) + (nl-k)*ns)*nstep+add] += (-s1*tmp1[r] + c1*tmp2[r]);
      }
      if(l < (ns-1)/2) {
         for(r=0;r<ns;++r) {
           c2 = coslut[(r*(nl-k+l*nl))%n];
           s2 = -coslut[(n4+r*(nl-k+l*nl))%n];
           a[(l + (nl-k)*ns)*nstep+add] += (c2*tmp1[r] - s2*tmp2[r]);
           a[((ns-l-1) + k*ns)*nstep+add] += (-s2*tmp1[r] - c2*tmp2[r]);
         }
      }
   }

} /*for k*/

k = nl/2;
for(r=0;r<ns;++r) {
   tmp1[r] = a[(r + n/2)*nstep+add];
   a[(r + n/2)*nstep+add] = 0;
}
for(l=0;l <= (ns-1)/2;++l) {
   for(r=0;r<ns;++r) {
      c1 = coslut[(r*(k+l*nl))%n];
      s1 = -coslut[(n4+r*(k+l*nl))%n];
      a[(l + k*ns)*nstep+add] += c1*tmp1[r];
      if(l < (ns-1)/2) 
         a[((ns-l-1) + (nl-k)*ns)*nstep+add] -= s1*tmp1[r];
   }
}
}/*end multi-dimensional loops*/

swapa(a,n,nl,ns,1,ntot,nstep);

  delete[] coslut;

  return 0;
} /*end FFTa_real_direct */


    int FFTa_real_inverse(float a[], int n, int ntot, int nstep)
/* Multi-dimensional real fft for arbitrary dimension */
{

int nl,ns,nul;
int i,r,k,l;
int nlarge,nj;
int istep,jstep,add;
float tmp1[tmpsize];
float tmp2[tmpsize];
float c1,s1, *coslut;
int n4;

nlarge = n*nstep;                              
nj = ntot/nlarge;  
n4 = n/4;
i = analyse_dimension(n,&nl,&nul,&ns);
if(ns > tmpsize) {
  printf("Use larger tmp matrix in FFTa_real_direct\n");
  return -1;
}


if( ns == 1) {  /*dimension is a power of two */
   FFT_real_inverse(a,nl,nul,ntot,(ns*nstep));
   return 1;
}
/*inverse swap*/

        // Create array
        if((coslut = new float[maxn]) == 0) 
        {
          printf("Error: memory allocation failed");
  	      return 1;
        }  

   i = FFTa_initialize(n, coslut);

swapa(a,n,nl,ns,-1,ntot,nstep);

/* apply the inverse DFT of dimension ns */

for(jstep=0;jstep<nj;++jstep)
  for(istep=0;istep<nstep;++istep) {

add = jstep*nlarge + istep;


/*k=0*/
for(r=0;r<ns;++r) tmp1[r] = a[r*nstep+add];
for(r=0;r<ns;++r) {
  a[r*nstep+add] = tmp1[0];
  for(l=1;l <= (ns-1)/2;++l) {
      c1 = coslut[(r*l*nl)%n];
      s1 = -coslut[(n4+r*l*nl)%n];
      a[r*nstep+add] += (c1*tmp1[l] - s1*tmp1[ns-l]);
  }
  for(l= (ns+1)/2;l < ns;++l) {
      c1 = coslut[(r*l*nl)%n];
      s1 = -coslut[(n4+r*l*nl)%n];
      a[r*nstep+add] += (c1*tmp1[ns-l] + s1*tmp1[l]);
  }
}

for(k=1; k < nl/2;++k) {
   for(r=0;r<ns;++r) tmp1[r] = a[(r + k*ns)*nstep+add];
   for(r=0;r<ns;++r) tmp2[r] = a[(r + (nl-k)*ns)*nstep+add];
   for(r=0;r<ns;++r) {
     a[(r + k*ns)*nstep+add] = 0;
     a[(r + (nl-k)*ns)*nstep+add] = 0;
     for(l=0;l <= (ns-1)/2;++l) {
         c1 = coslut[(r*(k+l*nl))%n];
         s1 = -coslut[(n4+r*(k+l*nl))%n];
         a[(r + k*ns)*nstep+add] += (c1*tmp1[l] - s1*tmp2[ns-l-1]);
         a[(r + (nl-k)*ns)*nstep+add] += (s1*tmp1[l] + c1*tmp2[ns-l-1]);
     }
     for(l= (ns+1)/2;l < ns;++l) {
         c1 = coslut[(r*(k+l*nl))%n];
         s1 = -coslut[(n4+r*(k+l*nl))%n];
         a[(r + k*ns)*nstep+add] += (c1*tmp2[ns-l-1] + s1*tmp1[l]);
         a[(r + (nl-k)*ns)*nstep+add] += (s1*tmp2[ns-l-1] - c1*tmp1[l]);
     }

   }

} /*for k*/

k = nl/2;
for(r=0;r<ns;++r) tmp1[r] = a[(r + n/2)*nstep+add];
for(r=0;r<ns;++r) {
  a[(r + k*ns)*nstep+add] = 0;
  for(l=0;l <= (ns-1)/2;++l) {
      c1 = coslut[(r*(k+l*nl))%n];
      s1 = - coslut[(n4+r*(k+l*nl))%n];      
      a[(r + k*ns)*nstep+add] += (c1*tmp1[l] - s1*tmp1[ns-l-1]);
  }
  for(l= (ns+1)/2;l < ns;++l) {
      c1 = coslut[(r*(k+l*nl))%n];
      s1 = -coslut[(n4+r*(k+l*nl))%n];
      a[(r + k*ns)*nstep+add] += (c1*tmp1[ns-l-1] + s1*tmp1[l]);
  }
}

} /*end multi-dimensional loops*/

for (k=0;k<ntot;++k) a[k] /= ns;

FFT_real_inverse(a,nl,nul,ntot,(ns*nstep)); /*use FFT for the largest power of 2 */

  delete[] coslut;

  return 0;
} /*end FFTa_real_inverse */
