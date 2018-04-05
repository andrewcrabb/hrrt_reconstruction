/*! \class Tx_PB_3D 
    \brief This class implements the calculation of a u-map from blank and
           transmission.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana  - HRRT users community (sibomana@gmail.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2003/11/17 initial version
    \date 2008/06/16 Add scatter correction for HRRT using 
     Charles Watson method "Design and Performance of a Single Photon
     Transmission Measurement for the ECAT ART, IEEE Proceedings 1988"
     Equation (7):ln(ACF)=a+b*ln(slab/tx) ==> ln(ACF)=a+b*ln(bl/tx)
     ==> p = a+b*ln(bl/tx) in MAP-TR method
     \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

 */

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include "e7_tools_const.h"
#include "Tx_PB_3D.h"
#include "gm.h"
#include "logging.h"
#include <string.h>

/*****************************************************************************
                           Declarations of
                    Concrete  Tx_PB_Image class

*****************************************************************************/
template <typename T>
Tx_PB_3D <T>::Tx_PB_3D(const int size, const int size_z, const int nang,
                       const int bins, const float bwid): 
 PB_3D(size, size_z, nang, bins, bwid)     
 { blank=NULL;
   tx=NULL;
   norm=NULL;
   try
   { blank=new T[psize];
     tx=new T[psize];
     norm=new float[psize];
   }
   catch (...)
    { if (blank != NULL) delete[] blank;
      if (tx != NULL) delete[] tx;
      if (norm != NULL) delete[] norm;
      throw;
    }
 }

template <typename T>
Tx_PB_3D <T>::~Tx_PB_3D()
 { if (blank != NULL) delete[] blank;
   if (tx != NULL) delete[] tx;
   if (norm != NULL) delete[] norm;
 }

template <typename T>
void Tx_PB_3D <T>::calculate_proj_norm() const
 { float eps=15.0f;

   for (int i=0; i < psize; i++)
    { if (blank[i] < 1) blank[i]=1;
      if (tx[i] < 1) tx[i]=1;
      proj[i]=(float)log((float)blank[i]/(float)tx[i]);
      if (proj[i] < 0.0f) proj[i]=0.0f;
      if (proj[i] > eps)
       norm[i]=proj[i]/(1.0f/(float)tx[i]+1.0f/(float)blank[i]);
       else norm[i]=eps/(1.0f/(float)tx[i]+1.0f/(float)blank[i]);
    }
 }

template <typename T>
float Tx_PB_3D <T>::der_der_U(const int, const float * const) const
 { return(0.0f);
 }

template <typename T>
void Tx_PB_3D <T>::IP_grad(float * const grprior, float * const g2prior,
                           const float mu_img, const int mu_number,
                           const float * const mean, const float * const std,
                           const float * const gauss_intersect,
                           const float * const mu_interval) const
 { if (mu_img <= mean[0])
    { IP_grad_compute(grprior, g2prior, mu_img, mean[0], std[0], false);
      return;
    }
   if (mu_img >= mean[mu_number-1])
    { IP_grad_compute(grprior, g2prior, mu_img, mean[mu_number-1],
                      std[mu_number-1], false);
      return;
    }
   for (int i=0; i < mu_number-1; i++)
    if ((mu_img >= mean[i]) && (mu_img < mean[i+1]))
     { if (mu_img < mu_interval[2*i])
        { IP_grad_compute(grprior, g2prior, mu_img, mean[i], std[i], false);
          return;
        }
       if ((mu_img >= mu_interval[2*i]) && (mu_img < gauss_intersect[i]))
        { IP_grad_compute(grprior, g2prior, mu_img, gauss_intersect[i], std[i],
                          true);
          return;
        }
       if ((mu_img >= gauss_intersect[i]) && (mu_img < mu_interval[2*i+1]))
        { IP_grad_compute(grprior, g2prior, mu_img, gauss_intersect[i],
                          std[i+1], true);
          return;
        }
       if (mu_img >= mu_interval[2*i+1])
        { IP_grad_compute(grprior, g2prior, mu_img, mean[i+1], std[i+1],
                          false);
          return;
        }
     }
 }

template <typename T>
void Tx_PB_3D <T>::IP_grad_compute(float * const grprior,
                                   float * const g2prior,
                                   const float mu_img, const float mean,
                                   const float std, const bool invert) const
 { float std2;

   std2=std*std;
   *grprior=(mu_img-mean)/std2;
   *g2prior=1.0f/std2;
   if (invert) *grprior=-*grprior;
 }

template <typename T>
float Tx_PB_3D <T>::IP_prior(const float mu_img, const int mu_number,
                             const float * const mean, const float * const std,
                             const float * const gauss_intersect,
                             const float * const mu_interval) const
 { if (mu_img <= mean[0])
    return(IP_prior_compute(mu_img, mean[0], std[0], false));
   if (mu_img >= mean[mu_number-1])
    return(IP_prior_compute(mu_img, mean[mu_number-1], std[mu_number-1],
           false));
   for (int i=0; i < mu_number-1; i++)
    if ((mu_img >= mean[i]) && (mu_img < mean[i+1]))
     { if(mu_img < mu_interval[2*i])
        return(IP_prior_compute(mu_img, mean[i], std[i], false));
       if ((mu_img >= mu_interval[2*i]) && (mu_img < gauss_intersect[i]))
        return(IP_prior_compute(mu_img, gauss_intersect[i], std[i], true));
       if ((mu_img >= gauss_intersect[i]) && (mu_img < mu_interval[2*i+1]))
        return(IP_prior_compute(mu_img, gauss_intersect[i], std[i+1], true));
       if (mu_img >= mu_interval[2*i+1])
        return(IP_prior_compute(mu_img, mean[i+1], std[i+1], false));
     }
   return(0.0f);
 }

template <typename T>
float Tx_PB_3D <T>::IP_prior_compute(const float mu_img, const float mean,
                                     const float std, const bool invert) const
 { float prior;

   prior=(mu_img-mean)*(mu_img-mean)/(2.0f*std*std);
   if (invert) return(-prior);
   return(prior);
 }

// So far this code contains redundancy - backprojection of tx data is
// performed for each iteration for given subset
// It should be stored for each subset, if there is enough memory
template <typename T>
void Tx_PB_3D <T>::MAPTR(const int niter, const int subs, const int rotation,
                         const float pixel_size, const float alpha, 
                         const unsigned short int mu_number,
                         const float * const _mu_mean,
                         const float * const _std,
                         const float * const _gauss_intersect,
                         const float scaling_factor,
                         std::vector <unsigned long int> *histo,
                         const int div_twopi,
                         const float angle_shift, const float beta, 
                         const float beta_ip, const int niter_ip,
                         const bool initial, const bool autoscale,
                         const float autoscalevalue,
                         const unsigned short int loglevel)
 { float *mu_interval=NULL, *p=NULL, *back_up=NULL, *back_down=NULL,
         *gauss_intersect=NULL, *mu_mean=NULL, *std=NULL;
   int **subsets=NULL;

   try
   { int i, j, m, ms, iter, subset_size;
     float ang, sinphi, cosphi, grprior_ip=0, g2prior_ip=0, sample=0.0f;
     unsigned short int water_peak=0;
     float a_exp=1.0f, b=1.0f; 
     float peak=0.0f;
     float sc_thres = 0.5f;
     bool tx_scatter_flag=false;

/*
 p = a+b*ln(bl/tx) => ln(bl/tx)=(p-a)/b => bl/tx=exp((p-a)/b)
 => tx = bl/exp((p-a)/b) = bl*exp(-(p-a)/b)=bl*exp(a/b)*exp(-p/b)
 where a is hrrt_tx_scatter_a and b is hrrt_tx_scatter_b
*/
     gauss_intersect=new float[mu_number-1];
     memcpy(gauss_intersect, _gauss_intersect, sizeof(float)*(mu_number-1));
     mu_mean=new float[mu_number];
     memcpy(mu_mean, _mu_mean, sizeof(float)*mu_number);
     std=new float[mu_number];
     memcpy(std, _std, sizeof(float)*mu_number);

     if ((GM::gm()->modelNumber()=="328") ||(GM::gm()->modelNumber()=="3282"))
     { // apply scatter factor for HRRT
       if (fabs(hrrt_tx_scatter_b-1.0f)>EPS) tx_scatter_flag=true;
       if (tx_scatter_flag)
       {
         a_exp = (float)exp(hrrt_tx_scatter_a/hrrt_tx_scatter_b);
         b = hrrt_tx_scatter_b;
         Logging::flog()->logMsg("HRRT scatter factors=(#1,#2), threshold=#3", 
           loglevel)->arg(hrrt_tx_scatter_a)->arg(hrrt_tx_scatter_b)->arg(sc_thres);
       }
     }
     for (i=0; i < mu_number; i++)
      { mu_mean[i]*=pixel_size;
        std[i]*=pixel_size;
      }
     mu_interval=new float[2*(mu_number-1)];
                                             // Intersection of Gaussian priors
     for (i=0; i < mu_number-1; i++)
      gauss_intersect[i]*=pixel_size;
     for (i=0; i < mu_number-1; i++)
      { mu_interval[2*i]=(mu_mean[i]+gauss_intersect[i])/2.0f;
        mu_interval[2*i+1]=(mu_mean[i+1]+gauss_intersect[i])/2.0f;
      }
     p=new float[p_nang_offset];   
     back_up=new float[isize] ;
     back_down=new float[isize];
     if (!initial) memset(img, 0, isize*sizeof(float));
     subset_size=nang/subs;
     subsets=new int *[subs];
     for (ms=0; ms < subs; ms++)
      subsets[ms]=new int[subset_size];
     generate_subsets(subsets, nang, subset_size);
     for (iter=0; iter < niter; iter++)
      { Logging::flog()->logMsg("iteration: #1", loglevel)->arg(iter+1);
        if (((iter == niter_ip) && (beta_ip > 0.0f)) ||
            ((iter == niter-1) && (beta_ip == 0.0f)))
         { unsigned short int lwr, upr;
           unsigned long int i;

           histo[0].assign(512, 0);
           sample=0.5f/(float)512;
           for (i=0; i < (unsigned long int)isize; i++)
            { signed short int j;

              j=(signed short int)(img[i]*scaling_factor/(pixel_size*sample));
              if (j < 0) histo[0][0]++;
               else if (j > 512-1) histo[0][512-1]++;
                     else histo[0][j]++;
            }
           if (autoscale)                       // find water peak in histogram
            { lwr=(unsigned short int)(0.075f*scaling_factor/sample);
              upr=(unsigned short int)(0.12f*scaling_factor/sample);
              water_peak=lwr;
              for (i=lwr+1; i <= upr; i++)
               if (histo[0][i] > histo[0][water_peak]) water_peak=i;
              peak=water_peak*sample/(scaling_factor*autoscalevalue);
              Logging::flog()->logMsg("sample #1 scaling_factor #2 autoscale #3",
                                      loglevel)->arg(sample)->arg(scaling_factor)->arg(autoscalevalue);
              Logging::flog()->logMsg("water peak in histogram at #1 1/cm",
                                      loglevel)->arg(peak);
              for (i=0; i < mu_number; i++)
               mu_mean[i]*=peak;
              for (i=0; i < (unsigned long int)(mu_number-1); i++)
               gauss_intersect[i]*=peak;
              for (i=0; i < (unsigned long int)(2*(mu_number-1)); i++)
               mu_interval[i]*=peak;
            }
         }
        for (ms=0; ms < subs; ms++)
         { memset(back_up, 0, isize*sizeof(float));
           memset(back_down, 0, isize*sizeof(float));
           for (j=0; j < subset_size; j++ )
            { int offset;

              m=subsets[ms][j];
              offset=m*p_nang_offset;
              ang=(float)(m*rotation*2)*M_PI/(float)(div_twopi*nang)+
                  angle_shift;
              sinphi=sin(ang);
              cosphi=cos(ang);
                                     // Backprojection of Tx data:
                                     // this is redundancy, however save memory
              for (i=0; i < p_nang_offset; i++) 
               p[i]=tx[offset+i];
              blf(p, m, sinphi, cosphi, back_up);
                                               // Projection of attenuation map
              plf(p, m, sinphi, cosphi);
                    // Backprojection of model: attenuation weighted blank data

              for (i=0; i < p_nang_offset; i++)
              {
               if (p[i]<sc_thres)
                 p[i] = blank[offset+i]*exp(-p[i]);
               else  
                 p[i] = a_exp*blank[offset+i]*exp(-p[i]/b);
              }

              blf(p, m, sinphi, cosphi, back_down);
            }
                                                                // update image
           for (i=0; i < isize; i++)
            { if ((beta_ip != 0.0f) && (iter >= niter_ip))
               IP_grad(&grprior_ip, &g2prior_ip, img[i], mu_number, mu_mean,
                       std, gauss_intersect, mu_interval);
              img[i]+=alpha*(back_down[i]-back_up[i]-
                             (beta == 0.0f ? 0.0f : beta*der_U(i, img+i))- 
                             ((beta_ip == 0.0f) &&
                              (iter >= niter_ip) ? 0.0f : beta_ip*grprior_ip))/
                            (msize*back_down[i]+
                             (beta == 0.0f ? 0.0f : beta*der_der_U(i, img+i))+
                             (beta_ip == 0.0f ? 0.0f : beta_ip*g2prior_ip));
                                                       // positivity constraint
              if (img[i] < 0.0f) img[i]=0.0f;
            }
           max_min_ML_EM();
         }
      }
     delete[] mu_interval;
     mu_interval=NULL;
     for (ms=0; ms < subs; ms++)
      { delete[] subsets[ms];
        subsets[ms]=NULL;
      }
     delete[] subsets;
     subsets=NULL;
     delete[] p;
     p=NULL;
     delete[] back_up;
     back_up=NULL;
     delete[] back_down;
     back_down=NULL;
     delete[] gauss_intersect;
     gauss_intersect=NULL;
     delete[] mu_mean;
     mu_mean=NULL;
     delete[] std;
     std=NULL;

     if (autoscale)
     {
       if (tx_scatter_flag) // scatter correction :absolute scaling
       {
         peak = 0.086f;
         Logging::flog()->logMsg("Scale expected water peak #1 1/cm to 0.096",
           loglevel)->arg(peak);
       }
       else 
       {
         peak = water_peak*sample;
         Logging::flog()->logMsg("Scale water peak #1 1/cm to 0.096",
           loglevel)->arg(water_peak);
       }
      for (i=0; i < isize; i++)
       img[i]*=0.096f/peak;
     }

     histo[1].assign(512, 0);
     sample=0.5f/(float)512;
     for (unsigned long int k=0; k < (unsigned long int)isize; k++)
      { signed short int j;

        j=(signed short int)(img[k]*scaling_factor/(pixel_size*sample));
        if (j < 0) histo[1][0]++;
         else if (j > 512-1) histo[1][512-1]++;
               else histo[1][j]++;
      }
   }
   catch (...)
    { if (mu_interval != NULL) delete[] mu_interval;
      if (p != NULL) delete[] p;
      if (back_up != NULL) delete[] back_up;
      if (back_down != NULL) delete[] back_down;
      if (subsets != NULL)
       { for (unsigned short int ms=0; ms < subs; ms++)
          if (subsets[ms] != NULL) delete[] subsets[ms];
         delete[] subsets;
       }
      if (gauss_intersect != NULL) delete[] gauss_intersect;
      if (mu_mean != NULL) delete[] mu_mean;
      if (std != NULL) delete[] std;
      throw;
    }
 }

// Read projections and rearrange indexes
template <typename T>
void Tx_PB_3D <T>::read_projections(const T * const blank_external,
                                    const T * const tx_external,
                                    const float blank_factor_in)
 { int k, j, m, p_input_offset, index;

   p_input_offset=bins*nang;
   for (m=0; m < nang; m++)
    for (j=0; j < bins; j++)
     for (k=0; k < slice; k++)
      {	index=k*p_input_offset+m*bins+j;
        *blank++=blank_external[index];
        *tx++=(float)tx_external[index]/blank_factor_in;
      }
   blank-=psize;
   tx-=psize;
 }


