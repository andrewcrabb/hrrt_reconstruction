/*! \file fwdprj3d.h
    \brief This class provides a method to forward-project images into
           sinograms.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/30 COR for image and for projections are independent
    \date 2004/03/08 uses memCtrl for memory handling
    \date 2004/03/22 add time-of-flight support
    \date 2004/04/02 use vector code for jacobians
    \date 2004/04/20 added Doxygen style comments
    \date 2004/08/09 use fact that length of norm vector in TOF is constant
    \date 2004/08/10 fix bug in calculation of positon along LOR for TOF
    \date 2004/10/28 add new lookup-tables for radial, x/y- and image
                     coordinates and interpolation factors
    \date 2005/03/29 LUT2 was using twice as much memory as required
 */

#ifndef _FWDPRJ3D_H
#define _FDWPRJ3D_H

#include <vector>
#include "e7_tools_const.h"
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class FwdPrj3D
 {                            // declare wrapper functions as friends and allow
                              // them to call private methods of the class
    friend void *executeThread_Fwdproject(void *);
    friend void *executeThread_MIPproject(void *);
    friend void *executeThread_searchFwd(void *);
   public:
                                                               /*! line mask */
    typedef struct {                         /*! x-coordinate of first voxel */
                     unsigned short int start,
                                        end; /*!< x-coordinate of last voxel */
                   } tmaskline;
                                                   /*! parameter for threads */
    typedef struct { FwdPrj3D *object;       /*!< pointer to FwdPrj3D object */
                     float *image,                     /*!< pointer to image */
                           *proj,                   /*!< pointer to sinogram */
                                    /*! LUT for radial interpolation factors */
                           *lut_fact;
                     uint32 *lut_idx;         /*!< LUT for image coordinates */
                     signed short int *lut_r,         /*!< LUT for r indices */
                                      *lut_xy;      /*!< LUT for x/y indices */
                     unsigned short int subset,        /*!< number of subset */
                                        segment,      /*!< number of segment */
                                        z_start,  /*!< start plane of thread */
                                        z_end,      /*!< end plane of thread */
                                           /*! number of time-of-flight bins */
                                        tof_bins,
                                        proc,            /*!< number of node */
                                        num_procs,      /*!< number of nodes */
                                        lut_mode;              /*!< LUT mode */
                     FwdPrj3D::tmaskline *tfp;  /*!< pointer to lookup-table */
#ifdef XEON_HYPERTHREADING_BUG
                     unsigned short int thread_number;  /*! number of thread */
              /*! padding bytes to avoid cache conflicts in parameter-access */
                     char padding[2*CACHE_LINE_SIZE-
                                  9*sizeof(unsigned short int)-
                                  2*sizeof(signed short int *)-
                                  sizeof(uint32 *)-
                                  3*sizeof(float *)-
                                  sizeof(FwdPrj3D::tmaskline *)-
                                  sizeof(FwdPrj3D *)];
#endif
                   } tthread_params;
   private:
                               /*! offset for center of image in voxel units */
    static const float image_cor;
    std::vector <tmaskline> mask;        /*!< circular mask applied on image */
                            /*! lookup-table for forward-projection (mode 2) */
    std::vector <unsigned short int> table_fwd,
                                     axis_slices;            /*!< axis table */
                            /*! first plane of segment relative to segment 0 */
    std::vector <signed short int> z_bottom,
                             /*! last plane of segment relative to segment 0 */
                                   z_top;
    unsigned short int XYSamples,                 /*!< width/height of image */
                       XYSamples_padded,   /*!< padded width/height of image */
                       RhoSamples,         /*!< numbes of bins in projection */
                                     /*! padded number of bins in projection */
                       RhoSamples_padded,
                               /*! number of projections in a sinogram plane */
                       ThetaSamples,
                       axes,                 /*!< number of axes in sinogram */
                       axes_slices,        /*!< number of planes in sinogram */
                       segments,         /*!< number of segments in sinogram */
                       subsets,     /*!< number of subsets in reconstruction */
                       span,                           /*!< span of sinogram */
                       loglevel,                      /*!< level for logging */
                       tof_bins,          /*!< number of time-of-flight bins */
                       lut_mode,                     /*!< what LUTs to use ? */
                       lut_idx,                    /*!< LUT indices (mode 1) */
                       lut_fact,                    /*!< LUT factor (mode 1) */
                       lut_r,                    /*!< LUT r-indices (mode 1) */
                       lut_xy;             /*!< LUT for x/y-indices (mode 1) */
        /*! number of first element in LUT for each azimuthal angle (mode 1) */
    std::vector <unsigned long int> lut_pos;
    unsigned long int image_plane_size,             /*!< size of image plane */
                                              /*! size of padded image plane */
                      image_plane_size_padded,
                                 /*! size of a padded subset of the sinogram */
                      subset_size_padded,
                      view_size_padded; /*!< size of padded view of sinogram */
    std::vector <float> cos_theta,    /*!< cosine-table for azimuthal angles */
                        sin_theta,      /*!< sine-table for azimuthal angles */
                        vcos_phi,         /*!< cosine-table for axial angles */
                        vsin_phi;           /*!< sine-table for axial angles */
    float BinSizeRho,                                 /*!< size of bin in mm */
          DeltaXY,                          /*!< width/height of voxel in mm */
          plane_separation,                   /*!< thickness of slices in mm */
          rho_center,                             /*!< center of projections */
          image_center;                                 /*!< center of image */
                 // calculate axial angle and jacobian scale factor for a plane
    void calculatePlaneAngle(const unsigned short int,
                             const unsigned short int, float * const,
                             float * const, unsigned short int * const) const;
                         // forward-project one subset of segment 0 without LUT
    void forward_proj3d_seg0(float * const, float *, const unsigned short int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int) const;
                     // forward-project one subset of segment 0 with mode 1 LUT
    void forward_proj3d_seg0_table1(float * const, float *,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const float * const, const uint32 * const,
                                    const signed short int * const,
                                    const unsigned short int,
                                    const unsigned short int) const;
                     // forward-project one subset of segment 0 with mode 2 LUT
    void forward_proj3d_seg0_table2(tmaskline * const, float * const, float *,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int,
                                    const unsigned short int) const;
                // forward-project one subset of an oblique segment without LUT
    void forward_proj3d_oblique(float * const, float *,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int,
                                const unsigned short int) const;
            // forward-project one subset of an oblique segment with mode 1 LUT
    void forward_proj3d_oblique_table1(float * const, float *,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const float * const,
                                       const uint32 * const,
                                       const signed short int * const,
                                       const signed short int * const,
                                       const unsigned short int,
                                       const unsigned short int) const;
            // forward-project one subset of an oblique segment with mode 2 LUT
    void forward_proj3d_oblique_table2(tmaskline * const, float * const,
                                       float *, const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int,
                                       const unsigned short int) const;
 
    void initMask(const float);                       // initialize image masks
    void initTable1();                            // initialize LUTs for mode 1
    void initTable2(const unsigned short int);     // initialize LUT for mode 2
                                                    // calculate MIP projection
    void mip_proj(float * const, float *, const unsigned short int,
                  const unsigned short int, const unsigned short int) const;
                                    // calculate MIP projection with mode 2 LUT
    void mip_proj_table2(tmaskline * const, float * const, float *,
                         const unsigned short int, const unsigned short int,
                         const unsigned short int) const;
                  // calculate mode 2 LUT for one segment of forward-projection
    void searchIndicesFwd(tmaskline * const, const unsigned short int,
                          const unsigned short int);
                    // calculate mode 2 LUT for forward-projection of segment 0
    void searchIndicesFwd_seg0(tmaskline * const) const;
             // calculate mode 2 LUT for forward-projection of oblique segments
    void searchIndicesFwd_oblique(tmaskline * const, const unsigned short int,
                                  const unsigned short int,
                                  const unsigned short int) const;
   public:
                                                           // initialize object
    FwdPrj3D(const unsigned short int, const float, const unsigned short int,
             const float, const unsigned short int,
             const std::vector <unsigned short int>, const unsigned short int,
             const unsigned short int, const float, const float,
             const unsigned short int, const unsigned short int,
             const unsigned short int,
             const unsigned short int);
    ~FwdPrj3D();                                              // destroy object
                                   // forward-project one subset of one segment
    void forward_proj3d(float * const, float *, const unsigned short int,
                        const unsigned short int, const unsigned short int,
                        const unsigned short int proc=0,
                        const unsigned short int num_procs=1);
                                       // MIP project one subset of one segment
    void mip_proj(float * const, float *, const unsigned short int,
                  const unsigned short int);
 };

#endif
