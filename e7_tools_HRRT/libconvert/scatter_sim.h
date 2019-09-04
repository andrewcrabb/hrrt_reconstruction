/*! \file scatter_sim.h
    \brief This class implements the simulation of a scatter sinogram, based on
           an emission image and a u-map.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
 */

#pragma once

#include "e7_tools_const.h"

/*- class definitions -------------------------------------------------------*/

class ScatterSim
 {                              // declare wrapper function as friend and allow
                                // it to call private methods of the class
   friend void *executeThread_SimulateScatter(void *);
   public:
                                   /*! point with floating point coordinates */
    typedef struct { float x,                              /*!< x-coordinate */
                           y,                              /*!< y-coordinate */
                           z;                              /*!< z-coordinate */
                   } tpoint_xyz;
   private:
                                   /*! parameters for SimulateScatter thread */
    typedef struct { ScatterSim *object;   /*!< pointer to ScatterSim object */
                     float *emiss,            /*!< pointer to emission image */
                           *scatt_sim;      /*!< pointer to scatter sinogram */
                          /*! list of planes from which scatter is generated */
                     unsigned short int *zplanes,
                                     /*! first plane of sinogram to simulate */
                                        first_plane,
                                      /*! last plane of sinogram to simulate */
                                        last_plane,
                                        thread_number; /*!< number of thread */
#ifdef XEON_HYPERTHREADING_BUG
                                  /*! padding bytes to avoid cache conflicts */
                     char padding[2*CACHE_LINE_SIZE-
                                  4*sizeof(unsigned short int)-
                                  2*sizeof(float *)-sizeof(ScatterSim *)];
#endif
                   } tthread_params;
                                     /*! bounding box for the scanned object */
    typedef struct { float xl,              /*!< x-coordinate of left border */
                           xh,             /*!< x-coordinate of right border */
                           yl,             /*!< y-coordinate of lower border */
                           yh,             /*!< y-coordinate of upper border */
                           zl,             /*!< z-coordinate of front border */
                           zh;              /*!< z-coordinate of back border */
                   } tbounds;
                                                        /*! direction vector */
    typedef struct { float u,           /*!< x-component of direction vector */
                           v,           /*!< y-component of direction vector */
                           w;           /*!< z-component of direction vector */
                   } tpoint_uvw;
                             /*! point with signed short integer coordinates */
    typedef struct { signed short int ix,                  /*!< x-coordinate */
                                      iy,                  /*!< y-coordinate */
                                      iz;                  /*!< z-coordinate */
                   } tpoint_ixyz;
                            /*! point with unsigned long integer coordinates */
    typedef struct { unsigned long int ux,                 /*!< x-coordinate */
                                       uy,                 /*!< y-coordinate */
                                       uz;                 /*!< z-coordinate */
                   } tpoint_uxyz;
                                                           /*! data of a ray */
    typedef struct { unsigned long int nsamps,        /*!< number of samples */
                                       *samps;       /*!< samples of the ray */
                     unsigned short int angle;                    /*!< angle */
                     tpoint_uvw scatter_uvw;                  /*!< direction */
                     float r2;                            /*!< length of ray */
                   } tray;
                                             /*! scatter cross section value */
    typedef struct {                          /*! energy of scattered photon */
                     unsigned short int e_index;
                     float rel_xs,         /*!< relative total cross section */
                           rel_dxs; /*!< relative differential cross section */
                   } tscatter_cross_section;
                                                           /*! detector pair */
    typedef struct { unsigned long int d1,     /*!< number of first detector */
                                       d2;    /*!< number of second detector */
                   } tdetector_pair;
    static const float EPSILON,                             /*!< small value */
                       MARGIN,                  /*!< margin for boundary box */
                       ELECTRON_MASS,     /*!< mass of electron in MeV/(c^2) */
                            /*! absolute mu value in 1/cm for scatter bounds */
                       atten_thresh,
       /*! absolute mu value in 1/cm, reject scatter points below this value */
                       atten_min;
                               /*! minimum energy of scattered photon in keV */
    static const unsigned short int MIN_ENERGY_SCATTER,
                               /*! maximum energy of scattered photon in keV */
                                    MAX_ENERGY_SCATTER,
                                 /*! energy range of scattered photon in keV */
                                    ENERGY_RANGE,
                            /*! maximum number of crystal layers in detector */
                                    MAX_CRYSTAL_LAYERS;
                                     /*! size of scatter cross section table */
    static const signed short int scat_tab_len;
                               /*! offset used in rounding voxel coordinates */
    static const tpoint_xyz myround;
           /*! number of bins in projections for scatter estimation sinogram */
    unsigned short int RhoSamplesScatterEstim,
                         /*! number of angles in scatter estimation sinogram */
                       ThetaSamplesScatterEstim,
                              /*! with/height of emission/transmission image */
                       XYSamples,
                       ZSamples,   /*!< depth of emission/transmission image */
                      /*! number of angular samplings for scatter estimation */
                       num_angular_samps,
                       loglevel;                  /*!< top level for logging */
    unsigned long int image_slice_size,             /*!< size of image slice */
                      num_scats_per_direct;    /*!< number of scatter points */
    float DeltaXY,                   /*!< width/height of image voxels in mm */
          DeltaZ,                           /*!< depth of image voxels in mm */
          detector_rad,                        /*!< radius of detector in cm */
          *norm_factor,                /*!< geometrical normalization factor */
          *detect_prob,                         /*!< detection probabilities */
          *atten,                                     /*!< attenuation image */
          step_size;                          /*!< sample spacing along rays */
    tpoint_xyz scatter_grid_data;           /*!< voxel size in sampling grid */
                     /*! maximum number of scatter samples in all directions */
    tpoint_uxyz scatter_samps_max;
    signed long int idum;                  /*!< random number generator seed */
    tray *ray;                           /*!< information about scatter rays */
    tscatter_cross_section *scatter_cs;     /*!< scatter cross section table */
    tpoint_ixyz *scatter_ixyz;                  /*!< table of scatter points */
    tdetector_pair *detector_pair;               /*!< list of detector pairs */
                                                   // compute scattering values
    void compton_xs(const float, float * const, float * const,
                    float * const) const;
                                  // calculate table of detection probabilities
    float *detector_eff(const float, const float) const;
                           // approximation of the complementary error function
    float erfc(const float) const;
                                           // 3x3x3 median filter for one voxel
    float median3(const float * const, const unsigned short int,
                  const unsigned short int, const unsigned short int) const;
    float ran2(signed long int * const) const;       // calculate random number
              // compute sampling points, angles, directions and length of rays
    void ray_init(const tpoint_xyz * const, const tbounds, tray **) const;
                              // calculate lookup-tables for scatter estimation
    void scatter_init(const float * const, const float, const float,
                      const unsigned short int, const unsigned short int,
                      float **, tray **, tscatter_cross_section **,
                      tpoint_ixyz **, float **, tdetector_pair **);
                                           // find bounds of attenuation volume
    void set_bounds(const float, const float * const, tbounds * const) const;
                                         // simulate planes of scatter sinogram
    void simulate_scatter(float * const, const unsigned short int * const,
                          const unsigned short int, const unsigned short int,
                          float * const, const unsigned short int,
                          const bool) const;
                  // calculate attenuation coefficients for both crystal layers
    void u_interp(const unsigned short int, float * const) const;
   public:
                                                           // initialize object
    ScatterSim(float * const, const unsigned short int, const float,
               const unsigned short int, const float, const float, const float,
               const unsigned short int, const unsigned short int,
               const unsigned short int, const float, const unsigned short int,
               const unsigned short int, const float, const float,
               const unsigned short int);
    ~ScatterSim();                                            // destroy object
                                                   // simulate scatter sinogram
    float *simulateScatter(float * const, unsigned short int * const,
                           unsigned short int, const unsigned short int);
 };

