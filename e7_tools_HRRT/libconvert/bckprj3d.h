/*! \file bckprj3d.h
    \brief This class provides a method to backproject sinograms into images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/30 COR for image and for projections are independent
    \date 2004/03/06 uses memCtrl for memory handling
    \date 2004/03/17 add time-of-flight support
    \date 2004/04/02 use vector code for jacobians
    \date 2004/04/06 added Doxygen style comments
    \date 2004/08/09 use fact that length of norm vector in TOF is constant
    \date 2004/08/10 fix bug in calculation of positon along LOR for TOF
    \date 2004/10/28 add new lookup-table for radial, x/y- and image
                     coordinates and interpolation factors
 */

#ifndef _BCKPRJ3D_H
#define _BCKPRJ3D_H

#include <vector>
#include "e7_tools_const.h"
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class BckPrj3D
 {                            // declare wrapper functions as friends and allow
                              // them to call private methods of the class
    friend void *executeThread_Backproject(void *);
    friend void *executeThread_searchBkp(void *);
   public:
                                                               /*! line mask */
    typedef struct {                         /*! x-coordinate of first voxel */
                     unsigned short int start,
                                        end; /*!< x-coordinate of last voxel */
                   } tmaskline;
                                                   /*! parameter for threads */
    typedef struct { BckPrj3D *object;       /*!< pointer to BckPrj3D object */
                     float *image,                     /*!< pointer to image */
                           *proj,                   /*!< pointer to sinogram */
                                    /*! LUT for radial interpolation factors */
                           *lut_fact,
                           *lut_p;              /*!< LUT for x/y-coordinates */
                     signed short int *lut_r;/*!< LUT for radial coordinates */
                     uint32 *lut_xy;          /*!< LUT for image coordinates */
                                                     /*! size of padded view */
                     unsigned long int viewsize_padded;
                     unsigned short int t,                  /*!< theta angle */
                                        subset,        /*!< number of subset */
                                        segment,      /*!< number of segment */
                                        z_start,  /*!< start plane of thread */
                                        z_end,      /*!< end plane of thread */
                                           /*! number of time-of-flight bins */
                                        tof_bins,
                                        proc,            /*!< number of node */
                                        num_procs,      /*!< number of nodes */
                                        lut_mode;              /*!< LUT mode */
                     bool sum_bins;      /*! all time-of-flight bins equal ? */
                     BckPrj3D::tmaskline *tbp;  /*!< pointer to lookup-table */
#ifdef XEON_HYPERTHREADING_BUG
                     unsigned short int thread_number;  /*! number of thread */
              /*! padding bytes to avoid cache conflicts in parameter-access */
                     char padding[2*CACHE_LINE_SIZE-
                                  10*sizeof(unsigned short int)-
                                  sizeof(unsigned long int)-
                                  sizeof(bool)-4*sizeof(float *)-
                                  sizeof(signed short int *)-sizeof(uint32 *)-
                                  sizeof(BckPrj3D::tmaskline *)-
                                  sizeof(BckPrj3D *)];
#endif
                   } tthread_params;
   private:
                               /*! offset for center of image in voxel units */
    static const float image_cor;
    std::vector <tmaskline> mask;        /*!< circular mask applied on image */
                               /*! lookup-table for back-projection (mode 2) */
    std::vector <unsigned short int> table_bkp,
                                     axis_slices;            /*!< axis table */
    unsigned short int XYSamples,                 /*!< width/height of image */
                       RhoSamples,         /*!< numbes of bins in projection */
                                     /*! padded number of bins in projection */
                       RhoSamples_padded,
                               /*! number of projections in a sinogram plane */
                       ThetaSamples,
                       axes,                 /*!< number of axes in sinogram */
                       segments,         /*!< number of segments in sinogram */
                       subsets,     /*!< number of subsets in reconstruction */
                       span,                           /*!< span of sinogram */
                       loglevel,                      /*!< level for logging */
                       tof_bins,          /*!< number of time-of-flight bins */
                       lut_mode,                     /*!< what LUTs to use ? */
                       lut_r,       /*!< LUT for radial coordinates (mode 1) */
                       lut_fact, /*!< LUT for interpolation factors (mode 1) */
                       lut_xy,       /*!< LUT for image coordinates (mode 1) */
                         /*! LUT for offset of sinogram coordinates (mode 1) */
                       lut_p;
        /*! number of first element in LUT for each azimuthal angle (mode 1) */
    std::vector <unsigned long int> lut_pos;
                            /*! first plane of segment relative to segment 0 */
    std::vector <signed short int> z_bottom,
                             /*! last plane of segment relative to segment 0 */
                                   z_top;
    unsigned long int image_plane_size,             /*!< size of image plane */
                      view_size_padded; /*!< size of padded view of sinogram */
    float BinSizeRho,                                 /*!< size of bin in mm */
          DeltaXY,                          /*!< width/height of voxel in mm */
          plane_separation,                   /*!< thickness of slices in mm */
          eff_crystal_radius,        /*!< effective radius of detector in mm */
          rho_center,                             /*!< center of projections */
          image_center;                                 /*!< center of image */
    std::vector <float> cos_theta,    /*!< cosine-table for azimuthal angles */
                        sin_theta,      /*!< sine-table for azimuthal angles */
                        cos_phi,          /*!< cosine-table for axial angles */
                        sin_phi;            /*!< sine-table for axial angles */
                             // backproject one subset of segment 0 without LUT
    void back_proj3d_seg0(float * const , float *, const unsigned long int,
                          const unsigned short int, const unsigned short int,
                          const unsigned short int, const unsigned short int,
                          const unsigned short int) const;
                         // backproject one subset of segment 0 with mode 1 LUT
    void back_proj3d_seg0_table1(float * const, float *,
                                 const unsigned long int,
                                 const unsigned short int,
                                 const unsigned short int,
                                 const unsigned short int, const float * const,
                                 const signed short int * const,
                                 const uint32 * const,
                                 const unsigned short int,
                                 const unsigned short int);
                         // backproject one subset of segment 0 with mode 2 LUT
    void back_proj3d_seg0_table2(tmaskline * const, float * const, float *,
                                 const unsigned long int,
                                 const unsigned short int,
                                 const unsigned short int,
                                 const unsigned short int,
                                 const unsigned short int,
                                 const unsigned short int) const;
                    // backproject one subset of an oblique segment without LUT
    void back_proj3d_oblique(float * const , float *, const unsigned long int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int) const;
                // backproject one subset of an oblique segment with mode 1 LUT
    void back_proj3d_oblique_table1(float * const, float *,
                                    const unsigned long int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const float * const,
                                    const signed short int * const,
                                    const uint32 * const, const float * const,
                                    const unsigned short int,
                                    const unsigned short int);
                // backproject one subset of an oblique segment with mode 2 LUT
    void back_proj3d_oblique_table2(tmaskline * const, float * const,
                                    float *, const unsigned long int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int) const;
                                         // backproject one subset of segment 0
    void initMask(const float);                       // initialize image masks
    void initTable1();                            // initialize LUTs for mode 1
    void initTable2(const unsigned short int);     // initialize LUT for mode 2
                      // calculate mode 2 LUT for one segment of backprojection
    void searchIndicesBkp(tmaskline * const, const unsigned short int,
                          const unsigned short int);
                        // calculate mode 2 LUT for backprojection of segment 0
    void searchIndicesBkp_seg0(tmaskline * const,
                                    const unsigned short int) const;
                  // calculate mode 2 LUT for backprojection of oblique segment
    void searchIndicesBkp_oblique(tmaskline * const, const unsigned short int,
                                  const unsigned short int,
                                  const unsigned short int,
                                  const unsigned short int) const;
   public:
                                                           // initialize object
    BckPrj3D(const unsigned short int, const float, const unsigned short int,
             const float, const unsigned short int,
             const std::vector <unsigned short int>,
             const unsigned short int, const unsigned short int, const float,
             const float, const unsigned short int, const unsigned short int,
             const unsigned short int,
             const unsigned short int);
    ~BckPrj3D();                                              // destroy object
                                       // backproject one subset of one segment
    void back_proj3d(float *, float * const, const unsigned short int,
                     const unsigned short int, const bool,
                     const unsigned short int, const unsigned short int proc=0,
                     const unsigned short int num_procs=1);
 };

#endif
