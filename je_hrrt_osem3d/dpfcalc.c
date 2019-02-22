//#include <stdio.h>
//#include <string.h> 
//#include <stdlib.h> 
//#include <math.h> 
//#include "P39_osem3d.h"
#include "hrrt_osem3d.h"
#include "my_spdlog.hpp"
#include "hrrt_osem_utils.hpp"

/* Updates:
   Nov 30, 07 (AR): maxPET (as well as maxMR) is calculated only once (at third subset) since with further subsets, images get very noisy and max value increases substantially (using mean-value also is not a good idea, since that does not give an idea of the range of values) 

Jun 28, 08 (AR): Perhaps using geometric color scales (and not arithmetic) makes more sense (not implemented)
     * Defined alphaPET (and also MR) such that it is the fraction of maximum PET (MR) value PRIOR to squaring to arrive at psiXS (YS)   
     *  Needed to multiply results by delta_X x delta_Y to make the results intesity-scale independent., so I did (~Line 215).
     * Need to make MRI slightly noisy
     
     * took out sCounts from L217 (1/(2*PI*sCount)) to make this calculation nearly sample-independent in overall scale 
     * Added sigma_fraction_maxPET variable (if this is set to be too small e.g. 0.001, then at edges we get very strong DPF patterns and they force very negative values in recons	 
     * Later, worth considering increasing alphaPET values with iterations since they get increasingly more noisy; or even better, is to somehow model the fact that the images at early iterations have not converged yet, so they're more likely to be biased (which suggests using higher alphaPET values at beginning; so perhaps these two effects effectively cancel each other out)	  

*/

static float delta_Bayes;
float DPF(float r)
{

  //  float exp_pos=exp(r/delta_Bayes);
  //float exp_neg=1/exp_pos;
  //    return ( (exp_pos-exp_neg)/(exp_pos+exp_neg) );

  float exp_2pos=exp(2*r/delta_Bayes);
  return ( (exp_2pos-1)/(exp_2pos+1) ); //Equivalent to above, just faster.
}

// Note that in new Inki code format is image[x][y][z] NOT image[z][x][y]
int calculate_DPF_image(float ***PETimage, float ***MRimage, float ***DPFimage, int MAP_technique)
{
  
  float alphaPET, alphaMR; 	
  /* alphaPET relates PETpsi to PETimage */
  /* alphaMR is the constant assumed for MRimage 
  (will be multiplied by maxMR to keep sense of scale) so keep value low (e.g. ~0.01) */


  int x, y, z, xx, yy, zz, i, j;
  float test_float;

  int Rs, Rsh;  	/* Sample rate for probability density in x y direction*/
  int Rs_z, Rsh_z;  	/* Sample rate (in z direction) for probability density in z direction*/
  int Ns; 	/* Number of samples in x direction */ 
  int Ns_z; 	/* Number of samples in z direction */
  int Ns3;	/* Total number of samples */
  int Np, Nm; 	/* Bin size for PET image and MR image*/  
  int sCount;	/* Number of samples that are NOT ZERO in MR images */

  Np = 14; Nm = 22;
 
  float xbin[Np], ybin[Nm]; 	/* Binned PET and MR image values */ 
  
  static float maxPET, maxMR;		/* Max values in PET and MR images */
  float sigma_fraction_maxPET=0.0442;  /* Used to make sure psiXS is not 0 for very small values */

  static int max_computation_index=0;
  static int max_computation_index_MR=0;
  alphaPET = 0.070711; alphaMR = 0.005;   // alphaMR will be multiplied by maxMR (to have correct sense of scale)  
  /* alphaPET = 0.09; alphaMR = 0.005; */

  float log_pxy[Np][Nm], part_pxy, sumsamp, sumover;
  float psiXS, psiYS, xinS, yinS, psiX, psiY, xPET, yMR;

  float delta_X, delta_Y;

  
  /* NOTE BY ARMAN: CHAGING THIS DOESN'T MAKE A HUGE DIFFERENCE IN SPEED SINCE WE ONLY USE THIS TO COMPUTE P(x,y) AND FOR THE OVERALL RESULT WE COMPUTE DERIVATIVE FOR EVERY VOXEL) */
  Rs = 4;   /* sample 1 voxel in every 4 voxels in x and y directions */
  Rsh = Rs/2;
  Rs_z = 2;   /* sample 1 voxel in every 2 voxels in z direction */
  Rsh_z = Rs_z/2;

  Ns = x_pixels/Rs; 
  Ns_z = z_pixels/Rs_z;  
  Ns3 = Ns*Ns*Ns_z;
  /* find the maximum value in PET and MR images */

  if (max_computation_index==0){  /* We only calculate this once (since image keeps getting noisier and noisier, and redoing everytime disturbs the intensity-bins! */
    LOG_INFO("maxPET: {}", maxPET);
    maxPET = 0; 
    for(x=0; x<x_pixels; x++) {
      	for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) { 
     	  for(z=0; z<z_pixels; z++) {

	    if (PETimage[x][y][z]>maxPET) maxPET = PETimage[x][y][z];
	    
	  } /* for z */
        
        } /* for y */
    }  /* for x */
    
    maxPET = 1.2*maxPET; 
    max_computation_index=0;
  } //if max_computation_index==0
  
 
  
  for(x=0; x<x_pixels; x++) {
    for(y=0; y<y_pixels; y++) {
      for(z=0; z<z_pixels; z++) {
	DPFimage[x][y][z] = 0;   
      }
      }  
  }
    

  
  if (MAP_technique==2){  /* Proposed JE technique */
    /* Bin the PET and MR image values to Np and Nm bins */
    
    if (max_computation_index_MR==0){  /* We only calculate this once (since image keeps getting noisier and noisier, and redoing everytime disturbs the intensity-bins! */
      LOG_INFO("maxMR: {}", maxMR);
      maxMR = 0; 
      for(x=0; x<x_pixels; x++) {
      	for( y=cylwiny[x][0]; y<cylwiny[x][1]; y++ ) { 
     	  for(z=0; z<z_pixels; z++) {
            if (MRimage[x][y][z]>maxMR) maxMR = MRimage[x][y][z];              
	  } /* for z */
          
        } /* for y */
      }  /* for x */
      
      
      max_computation_index_MR=0;
  } //if max_computation_index_MR==0
  


    

    delta_X=maxPET/(Np-1);
    delta_Y=maxMR/(Nm-1);
    for(i=0; i<Np; i++) {
      xbin[i] = delta_X * i;
    }

    // Equal MR sampling
    // for(j=0; j<Nm; j++) {
    //   ybin[j] = delta_Y * j;
    //}

    // Unequal, prespecified MR sampling (substitute to having to use MANY MR samples
     ybin[0] = 0;
    ybin[1] = 10;
    ybin[2] = 1000;
    ybin[3] = 10000;
    ybin[4] = 20000;
    ybin[5] = 30000;
    ybin[6] = 40280;
    ybin[7] = 50000;
    ybin[8] = 69050;
    ybin[9] = 71760;
    ybin[10] = 72980;
    ybin[11] = 74290;
    ybin[12] = 75290;
    ybin[13] = 76150;
    ybin[14] = 77020;
    ybin[15] = 78050;
    ybin[16] = 79770;
    ybin[17] = 80960;
    ybin[18] = 84200;
    ybin[19] = 86370;
    ybin[20] = 92620;
    ybin[21] = 100000;
    
    /* Calculate the number of voxels that are NOT ZERO in the SAMPLED MR image */
    
    sCount = 0;
    for(xx=0; xx<Ns; xx++) {
      for(yy=0; yy<Ns; yy++) {
	for(zz=0; zz<Ns_z; zz++) {
	  yinS = MRimage[xx*Rs+Rsh][yy*Rs+Rsh][zz*Rs_z+Rsh_z];
	  if (yinS != 0) {
	    sCount = sCount +1;
	  }
	}
      }
    }

    LOG_INFO("sCount= {}", sCount);
    
    /* Calculate log_pxy for all binned x and y */
    /* Since intensity 0 values are not considered, start i = 1 and j = 1 */
    /* log_pxy[0][j] and log_pxy[i][0] should be 0 */
   
    for(i=0; i<Np; ++i) {
      for(j=0; j<Nm; ++j) {
	
	sumsamp = 0; 
	log_pxy[i][j] = 0;
	
	/* summation over the samping region */
	
	for(xx=0; xx<Ns; xx++) {
	  for(yy=0; yy<Ns; yy++) {
	    for(zz=0; zz<Ns_z; zz++) {
	      
              yinS = MRimage[xx*Rs+Rsh][yy*Rs+Rsh][zz*Rs_z+Rsh_z];  
	      
              if (yinS != 0) { /* use the MRimage as a mask to ignore 0s when calculating pxy */
		
	        xinS = PETimage[xx*Rs+Rsh][yy*Rs+Rsh][zz*Rs_z+Rsh_z];
		
		

	        /* psiXS = pow(alphaPET*xinS+maxPET*sigma_fraction_maxPET,2); */
		psiXS = pow(xinS*alphaPET, 2) + pow(511*sigma_fraction_maxPET,2); 
	        /* add the last term so that at 0 values, a very narrow psiXS is defined */
	        psiYS = pow(maxMR*alphaMR,2); 	/* assume no dependence on individual MRimage voxels for now */
		
	        sumsamp = sumsamp + 1/sqrt(psiXS *psiYS) * 
		  exp( -0.5*pow((xbin[i]-xinS),2)/psiXS - 0.5*pow((ybin[j]-yinS),2)/psiYS );  


	      } /* if */
	      
	    } /*for zz */
	  } /* for yy */
	} /* for xx */
	



	if (sumsamp!= 0) log_pxy[i][j] = log(1/(2*PI*sCount)*sumsamp);
	LOG_INFO("(log_pxy[{}][{}] = {}, sumsamp: {}) ", i,j,log_pxy[i][j], sumsamp);
      } /* for j */
    } /* for i */
    
    /* Previous calculation of log_pxy was independent of PET voxel */
    /* Now calculate partial derivative of the joint entropy for each PET voxel value */
    
    x=y=z=0;
    for(x=0; x<x_pixels; x++) {
      for( y=cylwiny[x][0]; yy<cylwiny[x][1]; y++ ) { 
        //      for(y=0; y<y_pixels; y++) {
	for(z=0; z<z_pixels; z++) {
	  
	  sumover = 0;
	  
	  for(i=0; i<Np; ++i) {
	    for(j=0; j<Nm; ++j) {
	      
	      yinS = MRimage[x][y][z];  /* note this yinS is the voxel of interest */
              
              if (yinS != 0 ) { /* use the MRimage as a mask to ignore 0s when calculating part_pxy */
	        xinS = PETimage[x][y][z];

		psiXS = pow(xinS*alphaPET,2) + pow(511*sigma_fraction_maxPET,2);		
		/* psiXS = pow(xinS, 2)*alphaPET + maxPET*sigma_fraction_maxPET; */
		/* psiXS = pow(alphaPET*xinS+maxPET*sigma_fraction_maxPET,2);*/
	        /* add the last term so that at 0 values, a very narrow psiXS is defined */
	        /* psiYS = maxMR*alphaMR; */	/* assume no dependence on individual MRimage voxels for now */
		psiYS = pow(maxMR*alphaMR,2);		

		part_pxy = 1/sqrt(psiXS*psiYS) *
		  exp( -0.5*pow((xbin[i]-xinS),2)/psiXS - 0.5*pow((ybin[j]-yinS),2)/psiYS ) *
		  ( (xbin[i]-xinS) * (alphaPET*alphaPET*xbin[i]*xinS + pow(511*sigma_fraction_maxPET,2))/(psiXS*psiXS) -
		    alphaPET*alphaPET*xinS / psiXS );
		  /* ( (xbin[i]-xinS)/psiXS * (xbin[i]/(xinS+30)) - 1/(xinS+30) ); */

	        sumover = sumover - ((1+log_pxy[i][j])*part_pxy);
	      } /* if */
	    } /* for j*/
	  } /* for i */
	  
	  
	  /* took out sCounts from here to make this sample-independent in overall scale */
	  DPFimage[x][y][z] = sumover*1/(2*PI)*delta_X*delta_Y;
	  /* Took this last term (1/(2*PI*sCount)) out of loops to ease computation */
	  
	} /*for z */
	/* }  window */
      } /* for y */
      
      LOG_INFO(" (DPFimage[{}][{}][23] = {}) ", x,y_pixels/2,DPFimage[x][y_pixels/2][23]);
    } /* for x */
  } else if (MAP_technique==1){  /* Conventional technique */   
    
    delta_Bayes=maxPET/20.0;
    LOG_INFO("delta_Bayes: {}", delta_Bayes);
    
    float better_but_expensive_method=1;  // Keep at 1; making it 0 uses faster method but seems to eat away the image over time!

    if (better_but_expensive_method){
      for(xx=1; xx<x_pixels-1; xx++) {
      	for( yy=cylwiny[xx][0]; yy<cylwiny[xx][1]; yy++ ) { 
          //  for ( yy=1; yy<y_pixels-1; yy++ ){ 
          //   for ( xx=1; xx<x_pixels-1; xx++ ){
          //if ( cyl_window[xx][yy] ) {
	  for( zz=1,i=0; zz<z_pixels-1; zz++ ) {
	    DPFimage[xx][yy][zz]= 
	      ((DPF(PETimage[xx][yy][zz] - PETimage[xx][yy][zz-1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy][zz+1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx-1][yy][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx+1][yy][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy-1][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy+1][zz])
		)+sqrt(0.5)*
	       (DPF(PETimage[xx][yy][zz] - PETimage[xx-1][yy][zz-1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy-1][zz-1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx+1][yy][zz-1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy+1][zz-1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx-1][yy-1][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx-1][yy+1][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx+1][yy-1][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx+1][yy+1][zz])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx-1][yy][zz+1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy-1][zz+1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx+1][yy][zz+1])+
		DPF(PETimage[xx][yy][zz] - PETimage[xx][yy+1][zz+1])
		));

	  } /* z loop */
          //} /* cyl_window[x][y] */
        } /* x loop */
      } /* y loop */
    } else {
      // Faster method:
      for(xx=1; xx<x_pixels-1; xx++) {
      	for( yy=cylwiny[xx][0]; yy<cylwiny[xx][1]; yy++ ) { 
	  for( zz=1,i=0; zz<z_pixels-1; zz++ ) {
	    DPFimage[xx][yy][zz]= 
	      DPF( (6*PETimage[xx][yy][zz]
                    - PETimage[xx][yy][zz-1]
                    - PETimage[xx][yy][zz+1]
                    - PETimage[xx-1][yy][zz]
                    - PETimage[xx+1][yy][zz]
                    - PETimage[xx][yy-1][zz]
                    - PETimage[xx][yy+1][zz])
                   +sqrt(0.5)*
                   (12*PETimage[xx][yy][zz]
                    - PETimage[xx-1][yy][zz-1]
                    - PETimage[xx][yy-1][zz-1]
                    - PETimage[xx+1][yy][zz-1]
                    - PETimage[xx][yy+1][zz-1]
                    - PETimage[xx-1][yy-1][zz]
                    - PETimage[xx-1][yy+1][zz]
                    - PETimage[xx+1][yy-1][zz]
                    - PETimage[xx+1][yy+1][zz]
                    - PETimage[xx-1][yy][zz+1]
                    - PETimage[xx][yy-1][zz+1]
                    - PETimage[xx+1][yy][zz+1]
                    - PETimage[xx][yy+1][zz+1]) );

          }
        }
      }
   
    }
    
  }/*  calculate_conventional_DPF_image */

  LOG_INFO(" **FINAL (DPFimage[23][64][64] = {}) ", DPFimage[23][64][64]);
  return 1;
}

