/*! \class FORE fore.h "fore.h"
    \brief This class implements the fourier rebinning to convert 3d sinograms
           into a stack of 2d sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/09 fixed bug in calculation of current ring difference for
                     oblique segments
    \date 2003/12/30 fixed bug in logging of axial angle
    \date 2005/01/27 added Doxygen style comments
    \date 2005/02/25 use vectors and memory controller

    Axis 0 and oblique axes are handled differently. The sinograms in axis 0 are
    transformed in frequency space, and only the parts smaller than K_Limit are
    cut-off. For the oblique axes the sinogram planes from +axis and -axis are
    combined into a 360 degrees sinogram. This is transformed into frequency
    space and fourier rebinned. The weights for the linear interpolation in
    frequency space are precalculated for every axis. After rebinning all axes,
    the data is normalized and transformed back into sinogram space.
    The code produces the same result as the ECAT7.2 code, except that a scaling
    for the oblique axes is added (1/cos(phi)). This is important for scanners
    with large aperture angles.
    A description of Fourier rebinning can be found in
     "Exact and Approximate Rebinning Algorithms for 3D PET Data",
     M. Defrise, P.E. Kinahan, D.W. Townsend, C. Michel, M. Sibomana,
     D.F. Newport, pp. 145-158, No. 2, Vol. 16, April 1997,
     IEEE Transactions on Medical Imaging.
 */

#include <iostream>
#include <limits>
#include <cstring>
#include "fastmath.h"
#include "fore.h"
#include "const.h"
#include "exception.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "vecmath.h"

#ifdef WIN32
#undef max
#endif

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize FORE algorithm.
    \param[in] _RhoSamples         number of bins
    \param[in] BinSizeRho          bin size in mm
    \param[in] _ThetaSamples       number of angles
    \param[in] _axis_slices        array of number of planes per axis
    \param[in] _span               span of sinogram
    \param[in] _plane_separation   plane separation in mm
    \param[in] _ring_radius        ring radius in mm
    \param[in] alim                axis limit for single-slice rebinning
    \param[in] wlim                radial limit for single-slice rebiining
    \param[in] klim                angular limit for single-slice rebinning
    \param[in] _loglevel           top level for logging
    \exception REC_FORE_BINS_TOO_SMALL number of bins in sinogram for FORE is
                                       too small
    \exception REC_FORE_ANGLES_TOO_SMALL number of angles in sinogram for FORE
                                         is too small
 
    Initialize FORE algorithm.
 */
/*---------------------------------------------------------------------------*/
FORE::FORE(const unsigned short int _RhoSamples, const float BinSizeRho,
           const unsigned short int _ThetaSamples,
           const std::vector <unsigned short int> _axis_slices,
           const unsigned short int _span, const float _plane_separation,
           const float _ring_radius, const unsigned short int alim,
           const unsigned short int wlim, const unsigned short int klim,
           const unsigned short int _loglevel)
 { fore_fft=NULL;
   fore_vol=NULL;
   fore_norm=NULL;
   sino3d_buffer=NULL;
   sino_fft=NULL;
   try
   { std::string str;
     unsigned short int axes_slices;

     loglevel=_loglevel;
     Logging::flog()->logMsg("initialize FORE", loglevel);
     RhoSamples=_RhoSamples;
     if (RhoSamples < 4)
      throw Exception(REC_FORE_BINS_TOO_SMALL,
                      "Number of bins in sinogram for FORE is too small.");
     RhoSamples_d2p1=RhoSamples/2+1;
     ThetaSamples=_ThetaSamples;
     if (ThetaSamples < 4)
      throw Exception(REC_FORE_ANGLES_TOO_SMALL,
                      "Number of angles in sinogram for FORE is too small.");
     axis_slices=_axis_slices;
     plane_separation=_plane_separation;
     ring_radius=_ring_radius;
     span=_span;
     Logging::flog()->logMsg("RhoSamples=#1", loglevel+1)->arg(RhoSamples);
     Logging::flog()->logMsg("BinSizeRho=#1", loglevel+1)->arg(BinSizeRho);
     Logging::flog()->logMsg("ThetaSamples=#1", loglevel+1)->arg(ThetaSamples);
     Logging::flog()->logMsg("axes=#1", loglevel+1)->arg(axis_slices.size());
     str=toString(axis_slices[0]);
     axes_slices=axis_slices[0];
     for (unsigned short int i=0; i < axis_slices.size(); i++)
      { str+=","+toString(axis_slices[i+1]);
        axes_slices+=axis_slices[i+1];
      }
     Logging::flog()->logMsg("axis table=#1 {#2}", loglevel+1)->
      arg(axes_slices)->arg(str);
     Logging::flog()->logMsg("span=#1", loglevel+1)->arg(span);
     Logging::flog()->logMsg("plane separation=#1mm", loglevel+1)->
      arg(plane_separation);
     Logging::flog()->logMsg("effective detector ring radius=#1mm",
                             loglevel+1)->arg(ring_radius);
     Logging::flog()->logMsg("FORE limits (alim,wlim,klim)=#1,#2,#3",
                             loglevel+1)->arg(alim)->arg(wlim)->arg(klim);
                                 // create object to calculate FTs of sinograms
     fore_fft=new FORE_FFT(RhoSamples, ThetaSamples);
                                                         // size of a fft plane
     fft_plane_size=(unsigned long int)(ThetaSamples+1)*
                    (unsigned long int)RhoSamples_d2p1;
                                             // angle between axis 0 and axis 1
     delta_phi=atanf((float)span*plane_separation/ring_radius);
     x_res_z_res=BinSizeRho/plane_separation;
     Logging::flog()->logMsg("angle between segment 0 and segment +/-1=-/+#1 "
                             "degrees", loglevel+1)->
      arg(delta_phi*180.0f/M_PI);
                                                             // set FORE limits
     { float tmp;

       W_Limit=wlim;
       K_Limit=klim;
       tmp=(float)W_Limit*M_PI;
       if ((float)K_Limit > tmp) K_Limit=(unsigned short int)tmp;
       A_Limit=alim;
     }
     First_Proj.resize(ThetaSamples+1);
     gen_rebin_map();                       // calculate constants for all axes
     { unsigned long int size;

       size=(unsigned long int)axis_slices[0]*2*fft_plane_size;
                                                 // memory for rebinning volume
       fore_vol=MemCtrl::mc()->createFloat(size, &fore_vol_idx, "forevol",
                                           loglevel);
                                             // memory for normalization matrix
       fore_norm=MemCtrl::mc()->createFloat(size/2, &fore_norm_idx, "forenorm",
                                            loglevel);
     }
                                                    // size of a sinogram plane
     sino_plane_size=(unsigned long int)ThetaSamples*
                     (unsigned long int)RhoSamples;
                                              // initialize buffer for sinogram
     sino3d_buffer=MemCtrl::mc()->createFloat(2*sino_plane_size,
                                              &sino3d_buffer_idx, "sinoft",
                                              loglevel);
                                                   // buffer for FT of sinogram
     sino_fft=MemCtrl::mc()->createFloat((unsigned long int)ThetaSamples*2*
                                         (unsigned long int)RhoSamples_d2p1*2,
                                         &sino_fft_idx, "sinofft", loglevel);
     del_Z.resize(fft_plane_size);                      // buffer for z offsets
                                            // buffers for linear interpolation
     frac0.resize(fft_plane_size);
     frac1.resize(fft_plane_size);
   }
   catch (...)
    { if (fore_fft != NULL) { delete fore_fft;
                              fore_fft=NULL;
                            }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
FORE::~FORE()
 { MemCtrl::mc()->deleteBlockForce(&sino3d_buffer_idx);
   MemCtrl::mc()->deleteBlockForce(&sino_fft_idx);
   MemCtrl::mc()->deleteBlockForce(&fore_norm_idx);
   MemCtrl::mc()->deleteBlockForce(&fore_vol_idx);
   if (fore_fft != NULL) delete fore_fft;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate constants for rebinning of specified axis.
    \param[in]     axis         axis
 
    Calculate constants for rebinning of specified axis.
 */
/*---------------------------------------------------------------------------*/
void FORE::gen_rebin_map(const unsigned short int axis)
 { unsigned short int cur_ring_diff;
   float delR_delZ, cur_phi;

   cur_phi=(float)axis*delta_phi;              // get phi angle of current axis
                                            // get delR/delZ in "plane numbers"
   delR_delZ=tanf(cur_phi)*(float)RhoSamples*x_res_z_res/(2.0f*M_PI);
                                   // calculate maximum current ring difference
   cur_ring_diff=(unsigned short int)(M_PI*delR_delZ+1.0f);
   Logging::flog()->logMsg("calculate rebinning map for axis #1 and ring "
                           "difference #2", loglevel+1)->arg(axis)->
    arg(cur_ring_diff);
                                                                // clear arrays
   del_Z.assign(del_Z.size(), 0);
   frac0.assign(frac0.size(), 0.0f);
   frac1.assign(frac1.size(), 0.0f);
   for (unsigned short int i=0; i < ThetaSamples+1; i++)
    { bool i_K_Limit;
      float i_delR_delZ;

      i_K_Limit=(i <= K_Limit);
      i_delR_delZ=(float)i*delR_delZ;
      for (unsigned short int j=First_Proj[i]; j < RhoSamples_d2p1; j++)
       { bool j_W_Limit, i_rF_nu;
         unsigned long int idx;

         idx=(unsigned long int)i*(unsigned long int)RhoSamples_d2p1+j;
         j_W_Limit=(j <= W_Limit);
               // compare angle with calculate "current" angular slope position
         i_rF_nu=((float)i <= (float)j*M_PI);
         if (axis <= A_Limit)
          {                                                // fraction 1 is 1.0
            if ((i_K_Limit && j_W_Limit) || i_rF_nu) frac1[idx]=1.0f;
          }
          else if (i_rF_nu && ((!i_K_Limit && (j > 0)) || !j_W_Limit))
                if (j > 0) // avoid division by zero
                 { signed long int iZ;
                   float rtmp;

                   rtmp=i_delR_delZ/(float)j;              // calculate Delta Z
                   iZ=(signed long int)rtmp;             // get integer Delta Z
                   if (iZ <= cur_ring_diff)
                    { del_Z[idx]=iZ;                           // store Delta Z
                      frac0[idx]=rtmp-(float)iZ;            // store fraction 0
                      frac1[idx]=1.0f-frac0[idx];             // and fraction 1
                    }
                 }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate constants for rebinning independent from axis.
 
    Calculate constants for rebinning independent from axis.
 */
/*---------------------------------------------------------------------------*/
void FORE::gen_rebin_map()
 { unsigned short int i;

   for (i=0; i <= K_Limit; i++)
    First_Proj[i]=0;
   for (i=K_Limit+1; i < ThetaSamples+1; i++)
    { First_Proj[i]=(unsigned short int)((float)i/M_PI)+1;
      if (First_Proj[i] > RhoSamples/2)
       First_Proj[i]=std::numeric_limits <unsigned short int>::max();
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Normalize and iFFT of 2d sinogram.
    \param[in]  name          prefix for swapfile name
    \param[out] idx           index of memory block for 2d sinogram
    \param[in]  max_threads   maximum number of threads to use
 
    Normalize and iFFT of 2d sinogram.
 */
/*---------------------------------------------------------------------------*/
void FORE::Normalize(const std::string name, unsigned short int * const idx,
                     const unsigned short int max_threads) const
 { float *sino2d;
                                   // devide the FT by its normalization matrix
   rebin_norm2D(fore_vol, fore_norm);
                                                      // memory for 2d sinogram
   sino2d=MemCtrl::mc()->createFloat(sino_plane_size*
                                     (unsigned long int)axis_slices[0], idx,
                                     name, loglevel);
                                    // allocate memory for resultig 2d sinogram
   for (unsigned short int plane=0; plane < axis_slices[0]; plane++,
        sino2d+=sino_plane_size)
    {                                       // calculate iFFT of sinogram plane
      fore_fft->sino_fft_2D(sino3d_buffer, fore_vol+plane*2*fft_plane_size,
                            false, max_threads);
                                         // copy 2d sinogram into result buffer
      memcpy(sino2d, sino3d_buffer, sino_plane_size*sizeof(float));
    }
   MemCtrl::mc()->put(*idx);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Correct negative frequency half of the FT.
    \param[in,out] sino_fft   FT of oblique sinogram
                              ((RhoSamples/2+1)*ThetaSamples complex values)

    Correct negative frequency half of the FT. Due to symmetry in a "direct"
    (2D) sinogram, only the positive "quadrant" of an FFT needs to be stored.
    This is based on the unique symmetry in the angular direction - the second
    half of the angles are the reverse ordered conjugates of the first half.
    This function "corrects" the negative frequency half of the FT to allow the
    "oblique" sinogram FTs to be rebinned into the stored quadrant of the
    "direct" sinogram FTs.
 */
/*---------------------------------------------------------------------------*/
void FORE::obl_sym_fix(float *sino_fft) const
 { unsigned short int i;
                                // pointer to negative frequency half of the FT
   for (sino_fft+=(ThetaSamples*2-1)*RhoSamples_d2p1*2,
        i=0; i < ThetaSamples-1; i++,
        sino_fft-=RhoSamples_d2p1*2)
    if (i & 1)          // change sign of imaginary part in every odd row of FT
     for (unsigned short int i=1; i <= 2*RhoSamples_d2p1; i+=2)
      sino_fft[i]=-sino_fft[i];
                        // and change sign of real part in every even row of FT
     else for (unsigned short int i=0; i < 2*RhoSamples_d2p1; i+=2)
           sino_fft[i]=-sino_fft[i];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin a "direct" plane into the FORE rebinning volume.
    \param[in]     plane      number of plane
    \param[in]     sino_fft   FT of "direct" sinogram
                              ((RhoSamples/2+1)*(ThetaSamples+1) complex
                               values)
    \param[in,out] fore_vol   FT of rebinned sinogram
 
    Rebin a "direct" plane into the FORE rebinning volume. There is no
    rebinning for "direct" planes, so a part of the plane is just copied into
    the volume, but all frequencies below K_Limit are cut-off.
 */
/*---------------------------------------------------------------------------*/
void FORE::rebin_2Dplane(const unsigned short int plane, const float *sino_fft,
                         float *fore_vol) const
 { unsigned short int i, size_rho;

   size_rho=RhoSamples_d2p1*2;
   for (fore_vol+=(unsigned long int)plane*2*fft_plane_size,
        i=0; i < ThetaSamples+1; i++,
        fore_vol+=size_rho,
        sino_fft+=size_rho)
   // The fourier rebinning used in the ECAT7.2 software cuts off all
   // parts of segment 0 that are below K_Limit. Its probably better
   // to copy segment 0 as it is - there is no rebinning necessary.
   // Therefore instead of :
    if (First_Proj[i] < std::numeric_limits <unsigned short int>::max())
     { unsigned short int first;

       first=First_Proj[i]*2;
       memcpy(fore_vol+first, sino_fft+first, (size_rho-first)*sizeof(float));
     }
   // I simply could use:
   // memcpy(fore_vol, sino_fft, size_rho*sizeof(float));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Normalize FT of rebinned sinogram (FT is divided by norm).
    \param[in,out] fore_vol    FT of rebinned sinogram
    \param[in]     fore_norm   normalization matrix
 
    Normalize FT of rebinned sinogram (FT is divided by norm).
 */
/*---------------------------------------------------------------------------*/
void FORE::rebin_norm2D(float * const fore_vol,
                        const float * const fore_norm) const
 { for (unsigned long int i=0;
        i < (unsigned long int)axis_slices[0]*fft_plane_size;
        i++)
    if ((fore_norm[i] > 0.0f) && (fore_norm[i] != 1.0f))
     { float factor;

       factor=1.0f/fore_norm[i];
       fore_vol[i*2]*=factor;                        // divide real by the norm
       fore_vol[i*2+1]*=factor;                 // divide imaginary by the norm
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin "oblique" plane into the FORE rebinning volume.
    \param[in]     plane        number of "oblique" plane
    \param[in]     axis         axis to rebin
    \param[in]     sino_fft     FT of sinogram plane
                                ((RhoSamples/2+1)*ThetaSamples*2 complex
                                 values)
    \param[in,out] fore_vol     FT of rebinned sinogram
    \param[in,out] fore_norm    normalization matrix

    Rebin "oblique" plane into the FORE rebinning volume.
 */
/*---------------------------------------------------------------------------*/
void FORE::rebin_plane(const unsigned short int plane,
                       const unsigned short int axis, float *sino_fft,
                       float *fore_vol, float *fore_norm) const
 { unsigned short int mid_Z, i;
   unsigned long int negoff;
                                              // get the central plane "offset"
   mid_Z=plane+(axis_slices[0]-axis_slices[axis]/2)/2;
   for (                                      // offset to negative frequencies
        negoff=(unsigned long int)ThetaSamples*
               (unsigned long int)RhoSamples_d2p1*2,
        i=0; i < ThetaSamples+1; i++,
        negoff-=RhoSamples_d2p1*2,
        fore_norm+=RhoSamples_d2p1,
        fore_vol+=RhoSamples_d2p1*2,
        sino_fft+=RhoSamples_d2p1*2)
    for (unsigned short int j=First_Proj[i]; j < RhoSamples_d2p1; j++)
     { float f0, f1;
       unsigned long int idx;

       idx=(unsigned long int)i*(unsigned long int)RhoSamples_d2p1+j;
       if (((f0=frac0[idx]) != 0.0f) || ((f1=frac1[idx]) != 0.0f))
        { unsigned long int poffset;
          signed short int offZ;

          offZ=mid_Z+del_Z[idx];                        // get positive Z plane
          if ((offZ >= 0) && (offZ < axis_slices[0]))// check for "valid" plane
           { poffset=offZ*fft_plane_size+j;   // get offsets into 2D FT volumes
                                     // now stuff the values in the right place
             fore_vol[poffset*2]+=sino_fft[j*2]*f1;
             fore_vol[poffset*2+1]+=sino_fft[j*2+1]*f1;
             fore_norm[poffset]+=f1;
           }
          offZ++;                                            // increment plane
          if ((offZ >= 0) && (offZ < axis_slices[0]))// check for "valid" plane
           { poffset=offZ*fft_plane_size+j;   // get offsets into 2D FT volumes
                                     // now stuff the values in the right place
             fore_vol[poffset*2]+=sino_fft[j*2]*f0;
             fore_vol[poffset*2+1]+=sino_fft[j*2+1]*f0;
             fore_norm[poffset]+=f0;
           }
                          // check to see if a "negative" freq needs to be done
          if ((i > 0) && (i < ThetaSamples))
           { offZ=mid_Z-del_Z[idx]-1;                   // get negative Z plane
                                                     // check for "valid" plane
             if ((offZ >= 0) && (offZ < axis_slices[0]))
              { poffset=offZ*fft_plane_size+j;// get offsets into 2D FT volumes
                                     // now stuff the values in the right place
                                     // (reverse "fractions")
                fore_vol[poffset*2]+=sino_fft[(j+negoff)*2]*f0;
                fore_vol[poffset*2+1]+=sino_fft[(j+negoff)*2+1]*f0;
                fore_norm[poffset]+=f0;
              }
             offZ++;                                         // increment plane
                                                     // check for "valid" plane
             if ((offZ >= 0) && (offZ < axis_slices[0]))
              { poffset=offZ*fft_plane_size+j;// get offsets into 2D FT volumes
                                     // now stuff the values in the right place
                fore_vol[poffset*2]+=sino_fft[(j+negoff)*2]*f1;
                fore_vol[poffset*2+1]+=sino_fft[(j+negoff)*2+1]*f1;
                fore_norm[poffset]+=f1;
              }
           }
        }
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform fourier rebinning of one axis.
    \param[in,out] sino3d_axis   pointer to axis of 3d sinogram
    \param[in]     axis          number of axis to rebin
    \param[in]     max_threads   maximum number of threads to use
 
    Perform fourier rebinning of one axis. This method is multi-threaded.
 */
/*---------------------------------------------------------------------------*/
void FORE::Rebinning(float *sino3d_axis, const unsigned short int axis,
                     const unsigned short int max_threads)
 { if (axis >= axis_slices.size()) return;
   if (axis == 0)                               // fourier rebinning for axis 0
    { unsigned long int size;

      Logging::flog()->logMsg("copy #1 planes into fourier space",
                              loglevel+1)->arg(axis_slices[0]);
      size=(unsigned long int)axis_slices[0]*2*fft_plane_size;
      memset(fore_vol, 0, size*sizeof(float));
      for (unsigned short int plane=0; plane < axis_slices[0]; plane++)
       {                                     // copy sinogram plane into buffer
         memcpy(sino3d_buffer, sino3d_axis+plane*sino_plane_size,
                sino_plane_size*sizeof(float));
                                             // calculate FFT of sinogram plane
         fore_fft->sino_fft_2D(sino3d_buffer, sino_fft, true, max_threads);
                                                        // rebin sinogram plane
         rebin_2Dplane(plane, sino_fft, fore_vol);
       }
                                             // initialize normalization matrix
      for (unsigned long int i=0; i < size/2; i++)
       fore_norm[i]=1.0f;
      return;
    }
                    // scaling of axis depending on angle (not done in ECAT7.2)
   //   scaling(sino3d_axis, axis);
                                        // fourier rebinning for all other axes
                                           // calculate constants for this axis
   gen_rebin_map(axis);
   Logging::flog()->logMsg("fourier rebin #1 planes", loglevel+1)->
    arg(axis_slices[axis]);
                                           // rebinning of all "oblique" planes
   for (unsigned short int plane=0; plane < axis_slices[axis]/2; plane++,
        sino3d_axis+=sino_plane_size)
    { unsigned short int j;
      float *dp, *sp;
                          // copy sinogram plane from segment +axis into buffer
      memcpy(sino3d_buffer, sino3d_axis, sino_plane_size*sizeof(float));
                  // copy reverse sinogram plane from segment -axis into buffer
      for (sp=sino3d_axis+
              (unsigned long int)(axis_slices[axis]/2)*sino_plane_size,
           dp=sino3d_buffer+sino_plane_size,
           j=0; j < ThetaSamples; j++,
           dp+=RhoSamples,
           sp+=RhoSamples)
       { dp[0]=sp[RhoSamples-1];
         for (unsigned short int k=1; k < RhoSamples; k++)
          dp[k]=sp[RhoSamples-k];
       }
                                             // calculate FFT of sinogram plane
      fore_fft->sino_fft(sino3d_buffer, sino_fft, true, max_threads);
      obl_sym_fix(sino_fft);                  // fix the sinogram for rebinning
                                                        // rebin sinogram plane
      rebin_plane(plane, axis, sino_fft, fore_vol, fore_norm);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Scale the axis depending on the angle.
    \param[in,out] sino3d_axis   axis of 3d sinogram
    \param[in]     axis          number of axis
 
    Scale the axis depending on the angle.
 */
/*---------------------------------------------------------------------------*/
void FORE::scaling(float * const sino3d_axis,
                   const unsigned short int axis) const
 { vecMulScalar(sino3d_axis, cosf((float)axis*delta_phi), sino3d_axis,
                (unsigned long int)axis_slices[axis]*sino_plane_size);
 }
