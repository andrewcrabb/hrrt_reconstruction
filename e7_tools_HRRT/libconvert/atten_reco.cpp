/*! \class AttenReco atten_reco.h "atten_reco.h"
    \brief This class implements the calculation of a u-map from blank and
           transmission.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Vladimir Panin (vladimir.panin@cpspet.com)
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added deadtime correction
    \date 2004/10/01 added Doxygen style comments
    \date 2004/10/01 use memory controller object
    \date 2008/06/16 Add scatter correction for HRRT

    This blank and transmission scan are rebinned by a given factor to improve
    the statistics. The number of bins and number of angles in the sinograms
    must be a multiple of the zoom factor. There is a choice between two
    iterative reconstruction algorithms to calculate the u-map:
    osem(log(blank/transmission)) and maptr
 */

#include <cstring>
#include <algorithm>
#include <sys/stat.h>
#include "e7_tools_const.h"
#include "atten_reco.h"
#include "global_tmpl.h"
#include "gm.h"
#include "logging.h"
#include "interfile.h"
#include "matrix.h"
#include "mem_ctrl.h"
#include "PB_TV_3D.h"
#include "raw_io.h"
#include "rebin_sinogram.h"
#include "str_tmpl.h"
#include "Tx_PB_Gauss_3D.h"
#include "Tx_PB_GM_3D.h"
#include "vecmath.h"

float hrrt_tx_scatter_a = 0.0f;   // -0.15 ==> water peak=.086
float hrrt_tx_scatter_b = 1.0f;   // 1.2 ==> peak=.086
// value set wrt to water peak in 20cm water being 0.78 and instead of
// theoritical value 0.086 at 662KeV

float hrrt_blank_factor=0.0f;

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _XYSamples      width/height of u-map image
    \param[in] _RhoSamples     number of bins in projections
    \param[in] _BinSizeRho     width of bins in mm
    \param[in] _ThetaSamples   number of projections in sinogram plane
    \param[in] _rebin_r        radial rebinning factor that was applied to
                               sinogram
    \param[in] _rebin_t        azimuthal rebinning factor that was applied to
                               sinogram
    \param[in] _planes         number of planes in sinograms
    \param[in] _sino_rt_zoom   zoom factor for sinogram (before reconstruction)
    \param[in] _sino_z_zoom    zoom factor for sinogram (before reconstruction)
    \param[in] _loglevel       top level for logging

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
AttenReco::AttenReco(const unsigned short int _XYSamples,
                     const unsigned short int _RhoSamples,
                     const float _BinSizeRho,
                     const unsigned short int _ThetaSamples,
                     const unsigned short int _rebin_r,
                     const unsigned short int _rebin_t,
                     const unsigned short int _planes,
                     const unsigned short int _sino_rt_zoom,
                     const unsigned short int _sino_z_zoom,
                     const unsigned short int _loglevel)
 { XYSamples=_XYSamples;
   ThetaSamples=_ThetaSamples;
   RhoSamples=_RhoSamples;
   BinSizeRho=_BinSizeRho;
   planes=_planes;
   sino_rt_zoom=_sino_rt_zoom;
   sino_z_zoom=_sino_z_zoom;
   ThetaSamplesUMap=ThetaSamples/sino_rt_zoom;
   RhoSamplesUMap=RhoSamples/sino_rt_zoom;
   planesUMap=planes/sino_z_zoom;
   if (planesUMap*sino_z_zoom < planes) planesUMap++;
   sinoUMap_plane_size=(unsigned long int)RhoSamplesUMap*
                       (unsigned long int)ThetaSamplesUMap;
   sinoUMap_size=sinoUMap_plane_size*(unsigned long int)planesUMap;
   umap_plane_size=(unsigned long int)XYSamples*
                   (unsigned long int)XYSamples;
   umap_size=umap_plane_size*(unsigned long int)planesUMap;
   loglevel=_loglevel;
   rebin_r=_rebin_r;
   rebin_t=_rebin_t;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate log(blank/transmission).
    \param[in]  blank          blank data
    \param[in]  tx             transmission data
    \param[in]  blank_factor   ratio of the length of the transmission scan and
                               the length of the blank scan
    \param[out] idx            index of memory block for result
    \return sinogram for log(blank/transmission)

    Calculate log(blank/transmission):
    \f[
        s_i=\max\left(\log\left(\frac{\max(b_i, 1)}{\max(t_i, 1)}\right),
                      0\right)
        \qquad\forall\quad 0\le i<N
    \f]
    where \f$N\f$ is the size of the sinogram, \f$b\f$ is the blank and \f$t\f$
    is the transmission.
 */
/*---------------------------------------------------------------------------*/
float *AttenReco::calcRatio(const float * const blank, const float * const tx,
                            const float blank_factor,
                            unsigned short int * const idx) const
 { float *sino;
   float stretch_factor = 1.5f;

   Logging::flog()->logMsg("calculate blank/transmission", loglevel);
   sino=MemCtrl::mc()->createFloat(sinoUMap_size, idx, "log", loglevel);
   for (unsigned long int i=0; i < sinoUMap_size; i++)
    { float bl;

      if (blank[i] < 1.0f) bl=1.0f;
       else bl=blank_factor*blank[i];
      if (tx[i] < 1.0f) sino[i]=logf(bl);
       else if (bl <= tx[i]) sino[i]=0.0f;
             else sino[i]=stretch_factor*logf(bl/tx[i]);
    }
   return(sino);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform deadtime correction.
    \param[in] s             singles in transmission
    \param[in] db_path       path for deadtime correction database
    \param[in] db_frames     number of frames in database
    \param[in] max_threads   number of threads

    Perform deadtime correction.
 */
/*---------------------------------------------------------------------------*/
std::vector <float> AttenReco::dead_time_correction(
                                    const uint64 s, const std::string db_path,
                                    const unsigned short int db_frames,
                                    const unsigned short int max_threads) const
 { RawIO <signed short int> *rio=NULL;
   Interfile *interf=NULL;
   RebinSinogram *rebin=NULL;

   try
   { unsigned short int f, bufferf_idx, spidx;
     unsigned long int sino_planesize;
     float *bufferf;
     signed short int *sinoplane;
     std::vector <unsigned long int> singles;
     std::vector <std::vector <float> > frame;
     std::vector <float> deadtime_factor;

     Logging::flog()->logMsg("calculate deadtime correction factors",
                             loglevel);
                                     // read data base for dead time correction
     singles.clear();
     sino_planesize=(unsigned long int)RhoSamples*rebin_r*
                    (unsigned long int)ThetaSamples*rebin_t;
     if ((rebin_r > 1) || (rebin_t > 1))
      rebin=new RebinSinogram(RhoSamples*rebin_r, ThetaSamples*rebin_t,
                              RhoSamples, ThetaSamples, RebinX <float>::NO_ARC,
                              0, true);
                                          // allocate buffer for blank sinogram
     bufferf=MemCtrl::mc()->createFloat(
                                      sino_planesize*(unsigned long int)planes,
                                      &bufferf_idx, "bla", loglevel+1);
     sinoplane=MemCtrl::mc()->createSInt(sino_planesize, &spidx, "pla",
                                         loglevel);
                                                               // read database
     frame.resize(db_frames);
     for (f=0; f < db_frames; f++)
      { float *buf;
        unsigned short int buf_idx;

        Logging::flog()->logMsg("load database file '#1'", loglevel+1)->
         arg(db_path+"/TX_"+toString(f)+
            Interfile::INTERFILE_ASUBHDR_EXTENSION);
        interf=new Interfile();
                                                              // load subheader
        interf->loadSubheader(db_path+"/TX_"+toString(f)+
                              Interfile::INTERFILE_ASUBHDR_EXTENSION);
                                                                 // get singles
        singles.push_back(interf->Sub_total_uncorrected_singles());
        rio=new RawIO <signed short int>(interf->Sub_name_of_data_file(),
                           false,
                           interf->Sub_image_data_byte_order() == KV::BIG_END);
        for (unsigned short int p=0; p < planes; p++)     // read prompts/trues
         { rio->read(sinoplane, sino_planesize);
           for (unsigned long int i=0; i < sino_planesize; i++)
            bufferf[p*sino_planesize+i]=(float)sinoplane[i];
         }
        if (rebin != NULL)
         buf=rebin->calcRebinSinogram(bufferf, planes, NULL, &buf_idx,
                                      loglevel+1, max_threads);
         else buf=bufferf;
                                       // rebin prompts/trues and add up angles
        frame[f]=rebin_sinogram_add_angles(buf);
        if (rebin != NULL)
         { MemCtrl::mc()->put(buf_idx);
           MemCtrl::mc()->deleteBlock(&buf_idx);
         }
                                                          // subtract randoms ?
        if (interf->Sub_number_of_scan_data_types() == 2)
         if (interf->Sub_scan_data_type_description(1) == KV::RANDOMS)
          { std::vector <float> randoms;

            for (unsigned short int p=0; p < planes; p++)       // read randoms
             { rio->read(sinoplane, sino_planesize);
               for (unsigned long int i=0; i < sino_planesize; i++)
                bufferf[p*sino_planesize+i]=(float)sinoplane[i];
             }
            if (rebin != NULL)
             buf=rebin->calcRebinSinogram(bufferf, planes, NULL, &buf_idx,
                                          loglevel+1, max_threads);
             else buf=bufferf;
            randoms=rebin_sinogram_add_angles(buf);
            if (rebin != NULL)
             { MemCtrl::mc()->put(buf_idx);
               MemCtrl::mc()->deleteBlock(&buf_idx);
             }
            vecSub(&frame[f][0], &randoms[0], &frame[f][0], randoms.size());
          }
        delete rio;
        rio=NULL;
        delete interf;
        interf=NULL;
      }
     MemCtrl::mc()->put(spidx);
     MemCtrl::mc()->deleteBlock(&spidx);
     if (rebin != NULL) { delete rebin;
                          rebin=NULL;
                        }
     MemCtrl::mc()->put(bufferf_idx);
     MemCtrl::mc()->deleteBlock(&bufferf_idx);
                                                      // division by last frame
     for (f=0; f < db_frames; f++)
      vecDiv(&frame[f][0], &frame[db_frames-1][0], &frame[f][0],
             frame[f].size());
                                 // interpolation (Extrapolation) over singles:
                                 // fit to polynomial of second degree
     const unsigned short int DEGREE=3;
     Matrix <float> A(DEGREE, db_frames), AT(db_frames, DEGREE),
                    b(1, db_frames), COV;

     for (f=0; f < db_frames; f++)
      { A(0, f)=1.0f;
        A(1, f)=singles[f];
        A(2, f)=A(1, f)*singles[f];
      }
     AT=A;
     AT.transpose();
     COV=AT;
     COV*=A;
     COV.invert();
     COV.transpose();
     deadtime_factor.resize(RhoSamplesUMap*planesUMap);
     for (unsigned long int i=0; i < deadtime_factor.size(); i++)
      { Matrix <float> fit;

        for (f=0; f < db_frames; f++)
         b(0, f)=frame[f][i];
        fit=AT;
        fit*=b;
        fit.transpose();
        fit*=COV;
        deadtime_factor[i]=fit(0, 0)+fit(1, 0)*(double)s+
                           fit(2, 0)*(double)s*(double)s;
        if ((deadtime_factor[i] <= 0.0f) || (deadtime_factor[i] > 1.0f))
         deadtime_factor[i]=1.0f;
      }
     return(deadtime_factor);
   }
   catch (...)
    { if (rio != NULL) delete rio;
      if (interf != NULL) delete interf;
      if (rebin != NULL) delete rebin;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create GNUPlot script for u-map histogram.
    \param[in] umap_qc_path   path for scripot
    \param[in] mnr            number of matrix

    Create GNUPlot script for u-map histogram.
 */
/*---------------------------------------------------------------------------*/
void AttenReco::GNUPlotScript(const std::string umap_qc_path,
                              const unsigned short int mnr) const
 { std::ofstream *file=NULL;

   try
   { std::string fname, nr;
     struct stat statbuf;

     fname=umap_qc_path+"/umap_qc.plt";
     if (stat(fname.c_str(), &statbuf) != 0)               // does file exist ?
      { file=new std::ofstream(fname.c_str());
        *file << "set terminal postscript landscape color \"Helvetica\" 8\n"
                 "set output \"umap_qc.ps\"\n";
      }
      else {                                         // append to existing file
             file=new std::ofstream(fname.c_str(), std::ios::app);
             *file << "reset\n";
           }
     nr=toStringZero(mnr, 2);
     *file << "set title \"Quality Control for u-map Reconstruction (Matrix "
           << nr << ")\"\n"
              "set xlabel \"cm^-1\"\n"
              "set yrange [0.1:]\n"
              "set logscale y 10\n"
              "plot \"umap_histo_" << nr << ".dat\" using 1:2 title \"before "
              "segmentation\" with lines,\\\n"
              "     \"umap_histo_" << nr << ".dat\" using 1:3 title \"after "
              "segmentation and scaling\" with lines\n";
     file->close();
     delete file;
     file=NULL;
   }
   catch (...)
    { if (file != NULL) { file->close();
                          delete file;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Normalize blank and transmission scan.
    \param[in,out] blank   blank scan
    \param[in,out] tx      transmission scan

    Normalize blank and transmission scan. The mean value of the central bin
    in the projections of the blank scan is calculated for every plane and used
    as a normalization factor for blank and transmission.
 */
/*---------------------------------------------------------------------------*/
void AttenReco::normalizeBlankTx(float * const blank, float * const tx) const
 { float *tp, *bp;

   Logging::flog()->logMsg("normalize blank and transmission scan",
                           loglevel);
                                  // calculate norm factors from every slice of
                                  // blank scan by adding up the central bins
   bp=blank;
   tp=tx;
   for (unsigned short int p=0; p < planesUMap; p++,
        bp+=sinoUMap_plane_size,
        tp+=sinoUMap_plane_size)
    { float blank_norm=0.0f;
                                          // add central bin in all projections
      for (unsigned short int t=0; t < ThetaSamplesUMap; t++)
       blank_norm+=bp[t*RhoSamplesUMap+RhoSamplesUMap/2];
                           // normalize blank and tx to maximum of norm factors
      if (blank_norm != 0.0f)
       { blank_norm=(float)ThetaSamplesUMap/blank_norm;
         vecMulScalar(bp, blank_norm, bp, sinoUMap_plane_size);
         vecMulScalar(tp, blank_norm, tp, sinoUMap_plane_size);
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin sinogram in radial, azimuthal and z-direction.
    \param[in]  sino   original sinogram
    \param[out] idx    index of memory block for result
    \return rebinned sinogram

    Rebin sinogram in radial, azimuthal direction by sino_rt_zoom and in
    z-direction by sino_z_zoom.
 */
/*---------------------------------------------------------------------------*/
float *AttenReco::rebin_sinogram(const float * const sino,
                                 unsigned short int * const idx) const
 { unsigned long int sino_plane_size, ptr=0;
   float *sinoz;

   sino_plane_size=(unsigned long int)ThetaSamples*
                   (unsigned long int)RhoSamples;
   sinoz=MemCtrl::mc()->createFloat(sinoUMap_size, idx, "sino", loglevel);
   memset(sinoz, 0, sinoUMap_size*sizeof(float));
   for (unsigned short int p=0; p < planesUMap; p++)
    for (unsigned short int t=0; t < ThetaSamplesUMap; t++)
     for (unsigned short int r=0; r < RhoSamplesUMap; r++,
          ptr++)
      { unsigned short int pp_end;
        unsigned long int index;

        index=(unsigned long int)(sino_z_zoom*p)*sino_plane_size+
              (unsigned long int)(sino_rt_zoom*t)*
              (unsigned long int)RhoSamples+sino_rt_zoom*r;
        if (p == planesUMap-1) pp_end=planes-(planesUMap-1)*sino_z_zoom;
         else pp_end=sino_z_zoom;
        for (unsigned short int pp=0; pp < pp_end; pp++,
             index+=sino_plane_size)
         for (unsigned short int tt=0; tt < sino_rt_zoom; tt++)
          sinoz[ptr]+=(float)vecScalarAdd(&sino[index+tt*RhoSamples],
                                          sino_rt_zoom);
        if (sinoz[ptr] < 0.0f) sinoz[ptr]=0.0f;
      }
   return(sinoz);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin sinogram in radial and z-direction and add up in azimuthal
           direction..
    \param[in,out] sino   sinogram
    \return rebinned sinogram

    Rebin sinogram in radial direction by sino_rt_zoom and in z-direction by
    sino_z_zoom and add up all angular projections..
 */
/*---------------------------------------------------------------------------*/
std::vector <float> AttenReco::rebin_sinogram_add_angles(
                                                       const float *sino) const
 { unsigned long int planesize;
   std::vector <float> result;
   float *rptr;

   planesize=(unsigned long int)RhoSamples*
             (unsigned long int)ThetaSamples;
   result.assign(RhoSamplesUMap*ThetaSamples*planesUMap, 0.0f);
   rptr=&result[0];
   for (unsigned short int p=0; p < planesUMap; p++,
        sino+=(unsigned long int)sino_z_zoom*planesize-RhoSamples,
        rptr+=RhoSamplesUMap)
    for (unsigned short int r=0; r < RhoSamplesUMap; r++,
         sino+=sino_rt_zoom)
     { unsigned short int last_p;

       if (p == planesUMap-1) if (sino_z_zoom == 1) last_p=1;
                               else last_p=sino_z_zoom-1;
        else last_p=sino_z_zoom;
       for (unsigned short int pp=0; pp < last_p; pp++)
        for (unsigned short int t=0; t < ThetaSamples; t++)
         { unsigned short int last_r=sino_rt_zoom;

           if ((r == RhoSamplesUMap-1) && (sino_rt_zoom == 3)) last_r--;
           rptr[r]+=(float)vecScalarAdd(&sino[(unsigned long int)pp*planesize+
                                              (unsigned long int)t*
                                              (unsigned long int)RhoSamples],
                                        last_r);
         }
     }
   return(result);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate u-map from blank and transmission.
    \param[in]  blank                     blank data
    \param[in]  tx                        transmission data
    \param[in]  iterations                number of iterations for iterative
                                          reconstruction
    \param[in]  subsets                   number of subsets for OSEM
                                          reconstruction
    \param[in]  beta                      regularization parameter for
                                          smoothing prior
    \param[in]  alpha                     relaxation parameter
    \param[in]  beta_ip                   regularisation parameter for
                                          intensity prior
    \param[in]  iterations_ip             first iteration for intensity prior
    \param[in]  gaussian_smoothing        use gaussian smoothing model ?
                                          (otherwise: Geman-McClure smoothing
                                                      model)
    \param[in]  geman_mcclure_threshold   threshold for Geman-McClure smoothing
    \param[in]  number_of_segments        number of segments for segmentation
                                          of u-map
    \param[in]  mu_mean                   mean values of gaussian priors
    \param[in]  mu_std                    standard deviations for gaussian
                                          priors
    \param[in]  gauss_intersect           boundaries between segments
    \param[in]  scaling_factor            scaling factor for u-map
    \param[in]  cut_border_planes         throw away both border planes ?
    \param[in]  umap_qc                   store quality control data of u-map
                                          reconstruction ?
    \param[in]  umap_qc_path              path for quality control data of
                                          u-map reconstruction
    \param[in]  autoscale                 autoscale u-map towards water-peak ?
    \param[in]  autoscalevalue            u-value for auto scaling
    \param[in]  mnr                       matrix number
    \param[in]  tx_singles                number of singles in transmission
    \param[in]  db_path                   path for blank database
    \param[in]  db_frames                 number of frames in blank database
    \param[out] uidx                      index of memory block for u-map
    \param[in]  max_threads               maximum number of threads to use

    Calculate u-map from blank and transmission by OSEM or MAP-TR. A histogram
    of the u-map at the last iteration before segmentation starts can be
    stored.
 */
/*---------------------------------------------------------------------------*/
void AttenReco::reconstruct(float * const blank, float * const tx,
                            const unsigned short int iterations,
                            const unsigned short int subsets,
                            const float beta, const float alpha,
                            const float beta_ip,
                            const unsigned short int iterations_ip,
                            const bool gaussian_smoothing,
                            const float geman_mcclure_threshold,
                            const unsigned short int number_of_segments,
                            float * const mu_mean, float * const mu_std,
                            float * const gauss_intersect,
                            const float scaling_factor,
                            const bool cut_border_planes, const bool umap_qc,
                            const std::string umap_qc_path,
                            const bool autoscale, const float autoscalevalue,
                            const unsigned short int mnr,
                            const uint64 tx_singles, const std::string db_path,
                            const unsigned short int db_frames,
                            unsigned short int * const uidx,
                            const unsigned short int max_threads) const
 { PB_TV_3D *Mu_recon=NULL;
   Tx_PB_Gauss_3D <float> *Mu_recon_gauss_tx=NULL;
   Tx_PB_GM_3D <float> *Mu_recon_gm_tx=NULL;
   std::ofstream *file=NULL;

   try
   { const unsigned short int HISTO_SAMPLES=512;
     const float SAMPLING_DISTANCE=0.5;
     float *umap;

     unsigned short int slice_first, size_z, bidx, tidx;
     float blank_factor, *sino, *blank_zoom, *tx_zoom;
     std::string mu('mu');
     Logging *flog;
     std::vector <unsigned long int> histo[2];

     flog=Logging::flog();
         // define slices to be thrown away from the beginning and from the end
     if (cut_border_planes) { slice_first=1;
                              size_z=planesUMap-2;
                            }
      else { slice_first=0;
             size_z=planesUMap;
           }
                                                              // rebin sinogram
     if ((RhoSamples == RhoSamplesUMap) &&
         (ThetaSamples == ThetaSamplesUMap) &&
         (planes == planesUMap)) { blank_zoom=blank;
                                   tx_zoom=tx;
                                 }
      else { flog->logMsg("rebin blank from #1x#2x#3 to #4x#5x#6", loglevel)->
              arg(RhoSamples)->arg(ThetaSamples)->arg(planes)->
              arg(RhoSamplesUMap)->arg(ThetaSamplesUMap)->arg(planesUMap);
             blank_zoom=rebin_sinogram(blank, &bidx);
             flog->logMsg("rebin transmission from #1x#2x#3 to #4x#5x#6",
                          loglevel)->arg(RhoSamples)->arg(ThetaSamples)->
              arg(planes)->arg(RhoSamplesUMap)->arg(ThetaSamplesUMap)->
              arg(planesUMap);
             tx_zoom=rebin_sinogram(tx, &tidx);
           }
                                         // scale blank for deadtime correction
     if (!db_path.empty())
      { std::vector <float> dc;
        float *dp, *bp;

        bp=blank_zoom;
        dc=dead_time_correction(tx_singles, db_path, db_frames, max_threads);
        dp=&dc[0];
        for (unsigned short int p=0; p < planesUMap; p++,
             dp+=RhoSamplesUMap)
         for (unsigned short int t=0; t < ThetaSamplesUMap; t++,
              bp+=RhoSamplesUMap)
          vecMul(bp, dp, bp, RhoSamplesUMap);
      }
                 // determine blank/tx ratio from counts measured under the bed
     if ((GM::gm()->modelNumber()=="328") ||(GM::gm()->modelNumber()=="3282"))
     { 
       if (fabs(hrrt_blank_factor)>EPS) blank_factor = hrrt_blank_factor;
       else blank_factor=sumCountsUnderBed(tx_zoom)/sumCountsUnderBed(blank_zoom);
     }
     else
       blank_factor=sumCountsUnderBed(tx_zoom)/sumCountsUnderBed(blank_zoom);
     flog->logMsg("scale factor tx/blank=#1", loglevel)->arg(blank_factor);
 
     umap=MemCtrl::mc()->createFloat(umap_size, uidx, "umap", loglevel);
     memset(umap, 0, umap_size*sizeof(float));
     histo[0].assign(HISTO_SAMPLES, 0);
     histo[1].assign(HISTO_SAMPLES, 0);
     if (slice_first > 0)
      flog->logMsg("cut off first and last plane", loglevel);
     if (alpha == 0.0f)
      { unsigned short int logidx;
                                                           // take log of ratio
        sino=calcRatio(blank_zoom, tx_zoom, blank_factor, &logidx);
        if (RhoSamples != RhoSamplesUMap)
         { MemCtrl::mc()->put(bidx);
           MemCtrl::mc()->deleteBlock(&bidx);
           MemCtrl::mc()->put(tidx);
           MemCtrl::mc()->deleteBlock(&tidx);
         }
         else { blank_zoom=NULL;
                tx_zoom=NULL;
              }
                                                         // OSEM reconstruction
        flog->logMsg("OSEM reconstruction (#1 iterations, #2 subsets) from "
                     "#3x#4x#5 to #6x#7x#8", loglevel)->arg(iterations)->
         arg(subsets)->arg(RhoSamplesUMap)->arg(ThetaSamplesUMap)->
         arg(size_z)->arg(XYSamples)->arg(XYSamples)->arg(size_z);
        if (beta != 0.0f) flog->logMsg("beta=#1", loglevel+1)->arg(beta);
        Mu_recon=new PB_TV_3D(XYSamples, size_z, ThetaSamplesUMap,
                              RhoSamplesUMap, 1.0f);
        Mu_recon->change_COR(0.5f/(float)sino_rt_zoom-0.0001f);
        Mu_recon->read_proj(sino+slice_first*sinoUMap_plane_size);
        Mu_recon->OS_EM(iterations, subsets, CLOCKWISE, 2, -M_PI_2, beta, NULL,
                        false, loglevel+2);
        Mu_recon->copy_image(umap+slice_first*umap_plane_size);
        delete Mu_recon;
        Mu_recon=NULL;
        MemCtrl::mc()->put(logidx);
        MemCtrl::mc()->deleteBlock(&logidx);
        if (umap_qc)                                        // create histogram
         { float sample, pixel_size;

           sample=SAMPLING_DISTANCE/(float)histo[0].size();
           pixel_size=BinSizeRho/10.0f*(float)sino_rt_zoom;
           for (unsigned long int i=0; i < umap_size; i++)
            histo[0][std::min((unsigned short int)
                      std::max((signed short int)
                               (umap[i]*scaling_factor/(pixel_size*sample)),
                               (signed short int)0),
                      (unsigned short int)(histo[0].size()-1))]++;
           histo[1]=histo[0];
         }
      }
      else { std::string str;
             unsigned short int i;

             normalizeBlankTx(blank_zoom, tx_zoom);
             flog->logMsg("MAP-TR reconstruction (#1 iterations, #2 subsets) "
                          "from #3x#4x#5 to #6x#7x#8", loglevel)->
              arg(iterations)->arg(subsets)->arg(RhoSamplesUMap)->
              arg(ThetaSamplesUMap)->arg(size_z)->arg(XYSamples)->
              arg(XYSamples)->arg(size_z);
             if (gaussian_smoothing)
              flog->logMsg("gaussian smoothing model", loglevel+1);
              else flog->logMsg("Geman-McClure smoothing model (threshold=%f)",
                                loglevel+1)->arg(geman_mcclure_threshold);
             flog->logMsg("alpha=#1", loglevel+1)->arg(alpha);
             flog->logMsg("beta=#1", loglevel+1)->arg(beta);
             if (beta_ip != 0.0f)
              { flog->logMsg("beta_ip=#1", loglevel+1)->arg(beta_ip);
                flog->logMsg("iterations_ip=#1", loglevel+1)->
                 arg(iterations_ip);
                flog->logMsg("number of segments=#1", loglevel+1)->
                 arg(number_of_segments);
                str="priors (mean, std)=";
                for (i=0; i < number_of_segments; i++)
                 str+="("+toString(mu_mean[i])+","+toString(mu_std[i])+") ";
                flog->logMsg(str, loglevel+1);
                str="boundaries=";
                for (i=0; i < number_of_segments-2; i++)
                 str+=toString(gauss_intersect[i])+", ";
                str+=toString(gauss_intersect[number_of_segments-2]);
                flog->logMsg(str, 4);
              }

             if (gaussian_smoothing)            // use gaussian smoothing model
              { Mu_recon_gauss_tx=new Tx_PB_Gauss_3D <float>(XYSamples, size_z,
                                                         ThetaSamplesUMap,
                                                         RhoSamplesUMap, 1.0f);
                Mu_recon_gauss_tx->change_COR(0.5f/
                                              (float)sino_rt_zoom-0.0001f);
                Mu_recon_gauss_tx->read_projections(
                                    blank_zoom+slice_first*sinoUMap_plane_size,
                                    tx_zoom+slice_first*sinoUMap_plane_size,
                                    blank_factor);
                Mu_recon_gauss_tx->MAPTR(iterations, subsets, CLOCKWISE,
                                         BinSizeRho/10.0f*(float)sino_rt_zoom,
                                         alpha, number_of_segments, mu_mean,
                                         mu_std, gauss_intersect,
                                         scaling_factor, histo, 2, -M_PI_2,
                                         beta, beta_ip, iterations_ip, false,
                                         autoscale, autoscalevalue,
                                         loglevel+2);
                Mu_recon_gauss_tx->copy_image(umap+
                                              slice_first*umap_plane_size);
                delete Mu_recon_gauss_tx;
                Mu_recon_gauss_tx=NULL;
              }
              else {                        // use Geman-McClure smothing model
                     Mu_recon_gm_tx=new Tx_PB_GM_3D <float>(XYSamples, size_z,
                                      ThetaSamplesUMap, RhoSamplesUMap, 1.0f,
                                      geman_mcclure_threshold*BinSizeRho/10.0f*
                                      (float)sino_rt_zoom);
                     Mu_recon_gm_tx->change_COR(0.5f/
                                                (float)sino_rt_zoom-0.0001f);
                     Mu_recon_gm_tx->read_projections(
                                    blank_zoom+slice_first*sinoUMap_plane_size,
                                    tx_zoom+slice_first*sinoUMap_plane_size,
                                    blank_factor);
                     Mu_recon_gm_tx->MAPTR(iterations, subsets, CLOCKWISE,
                                          BinSizeRho/10.0f*(float)sino_rt_zoom,
                                          alpha, number_of_segments, mu_mean,
                                          mu_std, gauss_intersect,
                                          scaling_factor, histo, 2,
                                          -M_PI_2, beta, beta_ip,
                                          iterations_ip, false, autoscale,
                                          autoscalevalue, loglevel+2);
                     Mu_recon_gm_tx->copy_image(umap+
                                                slice_first*umap_plane_size);
                     delete Mu_recon_gm_tx;
                     Mu_recon_gm_tx=NULL;
                   }
             if (RhoSamples != RhoSamplesUMap)
              { MemCtrl::mc()->put(bidx);
                MemCtrl::mc()->deleteBlock(&bidx);
                MemCtrl::mc()->put(tidx);
                MemCtrl::mc()->deleteBlock(&tidx);
              }
              else { blank_zoom=NULL;
                     tx_zoom=NULL;
                   }
           }
     if (umap_qc)                                            // store histogram
      { std::string fname;

        fname=umap_qc_path+"/umap_histo_"+toStringZero(mnr, 2)+".dat";
        file=new std::ofstream(fname.c_str());
        for (unsigned short int i=0; i < histo[0].size(); i++)
         *file << (float)i*SAMPLING_DISTANCE/(float)histo[0].size() << " "
               << histo[0][i] << " " << histo[1][i] << std::endl;
        file->close();
        delete file;
        file=NULL;
        GNUPlotScript(umap_qc_path, mnr);
      }
                                                        // scale image to cm^-1
     flog->logMsg("scale #1-map to 1/cm", loglevel)->arg(mu);
     vecMulScalar(umap, scaling_factor/(BinSizeRho/10.0f*(float)sino_rt_zoom),
                  umap, umap_size);
     MemCtrl::mc()->put(*uidx);
   }
   catch (...)
    { if (Mu_recon != NULL) delete Mu_recon;
      if (Mu_recon_gauss_tx != NULL) delete Mu_recon_gauss_tx;
      if (Mu_recon_gm_tx != NULL) delete Mu_recon_gm_tx;
      if (file != NULL) delete file;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate the sum of all counts measured under the bed.
    \param[in] sino   sinogram
    \return sum of counts

    Calculate the sum of all counts measured under the bed.
 */
/*---------------------------------------------------------------------------*/
float AttenReco::sumCountsUnderBed(float * const sino) const
 {           // number of bins in the sinogram which are measured under the bed
   const unsigned short int under_bed=8/sino_rt_zoom;
   float sum=0.0f, *sp;

   sp=sino;
   for (unsigned long int pt=0;
        pt < (unsigned long int)planesUMap*(unsigned long int)ThetaSamplesUMap;
        pt++,
        sp+=RhoSamplesUMap)
    sum+=(float)vecScalarAdd(sp, under_bed)+
         (float)vecScalarAdd(&sp[RhoSamplesUMap-under_bed], under_bed);
   return(sum);
 }
