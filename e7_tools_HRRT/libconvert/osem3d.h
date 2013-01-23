/*! \file osem3d.h
    \brief This class implements a 2D/3D-OSEM reconstruction, 2D/3D-forward and
           back-projection and maximum intensity projection.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/12 reconstruction with 3D Gibbs prior
    \date 2003/12/12 reconstruction with 3D-OP-OSEM
    \date 2004/01/02 image mask is centered in image, independent from COR
    \date 2004/01/08 add Unicode support for WIN32 version
    \date 2004/01/14 create LUTs only when needed for the first time, not at
                     initialization
    \date 2004/01/30 use sinogram rebinning factor instead of COR
    \date 2004/02/27 use SinogramConversion and ImageConversion classes
    \date 2004/04/02 use vector code
    \date 2004/04/07 use memory controller in forward-projector
    \date 2004/04/08 use memory controller where possible
    \date 2004/04/08 add TOF-AW-OSEM
    \date 2004/09/14 added Doxygen style comments
    \date 2004/09/15 added TOF support to Gibbs-OSEM
    \date 2004/09/21 calculate shift for TOF-OP-OSEM
    \date 2004/09/27 fixed NW/ANW/OP-OSEM for cases where norm is 0
    \date 2004/10/04 use faster backprojector
    \date 2004/11/11 NW- and ANW-OSEM fixed for scanners with gaps
    \date 2005/01/04 added progress reporting
 */

#ifndef _OSEM3D_H
#define _OSEM3D_H

#include <list>
#include <string>
#include <vector>
#include "bckprj3d.h"
#include "comm_socket.h"
#include "fwdprj3d.h"
#include "image_conversion.h"
#include "logging.h"
#include "sinogram_conversion.h"
#include "str_tmpl.h"
#include "stream_buffer.h"
#include "types.h"

/*- constants ---------------------------------------------------------------*/

                                      // commands used between master and slave
const unsigned short int OS_CMD_EXIT=42,              /*!< terminate process */
                         OS_CMD_INIT_OSEM=43,         /*!< initialize OSEM3D */
                                               /*! receive emission sinogram */
                         OS_CMD_GET_EMISSION=44,
                         OS_CMD_RECONSTRUCT=45;    /*!< reconstruct one step */

/*- class definitions -------------------------------------------------------*/

class OSEM3D
 { public:
                    /*! information about slave computers for reconstruction */
    typedef struct { std::string ip,                          /*!< IP-number */
                                 path;               /*!< path of executable */
                   } tnode;
   private:
    static const float epsilon, // threshold to avoid large errors in divisions
                       image_max;             // maximum allowed value in image
    static const std::string slave_executable; // name of OSEM slave executable
                              // size of receive buffer for communication slave
    static const unsigned long int OS_RECV_BUFFER_SIZE,
                            // size of send buffer for communication with slave
                                   OS_SEND_BUFFER_SIZE;
                                             /*! information about line mask */
    typedef struct { unsigned short int start,            /*!< start of mask */
                                        end;                /*!< end of mask */
                   } tmaskline;
    Logging *flog;                            /*!< pointer to logging object */
                    /*! circular mask applied on image during reconstruction */
    std::vector <tmaskline> mask,
                              /*! circular mask applied after reconstruction */
                            mask2;
    std::vector <unsigned short int> axis_slices,            /*!< axes table */
                                 /*! numbers of first plane for each segment */
                                     seg_plane_offset,
                                     order;         /*!< ordering of subsets */
    unsigned short int XYSamples,                 /*!< width/height of image */
                       XYSamples_padded,   /*!< padded width/height of image */
                       RhoSamples,         /*!< numbes of bins in projection */
                                     /*! padded number of bins in projection */
                       RhoSamples_padded,
                       RhoSamples_TOF, /*!< number of bins in TOF projection */
                                 /*! padded number of bins in TOF projection */
                       RhoSamples_padded_TOF,
                       ThetaSamples,       /*!< number of angles in sinogram */
                       axes,              /*!< number of axes in 3d sinogram */
                       axes_slices,     /*!< number of planes in 3d sinogram */
                       segments,         /*!< number of segments in sinogram */
                       subsets,     /*!< number of subsets in reconstruction */
                       span,                           /*!< span of sinogram */
                       mrd,                     /*!< maximum ring difference */
                       loglevel,                  /*!< top level for logging */
                       lut_level;                 /*!< number of LUTs to use */
                            /*! first plane of segment relative to segment 0 */
    std::vector <signed short int> z_bottom,
                             /*! last plane of segment relative to segment 0 */
                                   z_top;
    unsigned long int image_plane_size,             /*!< size of image plane */
                                              /*! size of padded image plane */
                      image_plane_size_padded,
                      image_volume_size,           /*!< size of image volume */
                                             /*! size of padded image volume */
                      image_volume_size_padded,
                                 /*! size of a padded subset of the sinogram */
                      subset_size_padded,
                             /*! size of a padded subset of the TOF sinogram */
                      subset_size_padded_TOF,
                      view_size,               /*!< size of view of sinogram */
                      view_size_padded, /*!< size of padded view of sinogram */
                                     /*! size of padded view of TOF sinogram */
                      view_size_padded_TOF,
                      sino_plane_size,           /*!< size of sinogram plane */
                      sino_plane_size_TOF;   /*!< size of TOF sinogram plane */
    float BinSizeRho,                                    /*!< bin size in mm */
          DeltaXY,                                      /*!< voxelsize in mm */
          reb_factor,                      /*!< rebinning factor of sinogram */
          fov_diameter,            /*!< diameter of FOV after reconstruction */
          plane_separation;                      /*!< plane separation in mm */
    ALGO::talgorithm algorithm;                /*!< reconstruction algorithm */
    BckPrj3D *bp;                                         /*!< backprojector */
    FwdPrj3D *fwd;                                    /*!< forward-projector */
                                       // buffers for communication with slaves
    StreamBuffer *wbuffer,        /*!< buffer for data to send to the socket */
                 *rbuffer; /*!< buffer for messages received from the socket */
                                        /*! communication channels to slaves */
    std::vector <CommSocket *> os_slave;
    std::string syngo_msg;               /*!< messages that is send to syngo */
                                  /*! normalization matrices for each subset */
    std::vector <unsigned short int> norm_factors,
                                             /*! offset matrices for OP-OSEM */
                                     offset_matrix;
                                  // factorize a number into a product of primes
    std::vector <unsigned short int> factorize(unsigned short int) const;
                            // calculate Herman-Meyer permutation of a sequence
    std::vector <unsigned short int> herman_meyer_permutation(
                                                           unsigned short int);
                                                           // initialize object
    void init(const unsigned short int, const float, const unsigned short int,
              const float, const unsigned short int,
              const std::vector <unsigned short int>,
              const unsigned short int, const unsigned short int, const float,
              const float, const unsigned short int, const ALGO::talgorithm,
              const unsigned short int, std::list <tnode> * const,
              const unsigned short int);
                                                   // initialize back-projector
    void initBckProjector(const unsigned short int);
                                                // initialize forward-projector
    void initFwdProjector(const unsigned short int);
                                                            // initialize image
    float *initImage(unsigned short int * const,
                     const unsigned short int) const;
    void initMask(const float, const float);          // initialize image masks
                                             // calculate list of prime numbers
    std::vector <unsigned short int> primes(const unsigned short int) const;
#ifdef UNUSED
                         // calculate OSEM reconstruction with multi-processing
    float *reconstructMP_master(float * const, const unsigned short int,
                                float * const, const unsigned short int);
#endif
    void terminateSlave(CommSocket *);                       // terminate slave
   public:
                                                                // constructors
    OSEM3D(const unsigned short int, const float, const unsigned short int,
           const float, const unsigned short int,
           const std::vector <unsigned short int>, const unsigned short int,
           const unsigned short int, const float, const float,
           const unsigned short int, const ALGO::talgorithm,
           const unsigned short int, std::list <tnode> * const,
           const unsigned short int);
    OSEM3D(SinogramConversion * const, const unsigned short int,
           const float, const float, const unsigned short int,
           const ALGO::talgorithm, const unsigned short int,
           std::list <tnode> * const, const unsigned short int);
    ~OSEM3D();                                                // destroy object
                                                   // calculate back-projection
    float *bck_proj3d(SinogramConversion *, unsigned short int * const,
                      const unsigned short int);
    float *bck_proj3d(float * const, const unsigned short int);
    void calcNormFactorsUW(const unsigned short int);     // calculate bkprj(1)
                                                        // calculate bkprj(1/A)
    void calcNormFactorsAW(SinogramConversion *, const unsigned short int);
    void calcNormFactorsAW(float * const, const unsigned short int,
                           const unsigned short int, const unsigned short int);
                                                        // calculate bkprj(1/N)
    void calcNormFactorsNW(SinogramConversion *, const unsigned short int);
    void calcNormFactorsNW(float * const, const unsigned short int,
                           const unsigned short int);
                                                     // calculate bkprj(1/(AN))
    void calcNormFactorsANW(SinogramConversion *, SinogramConversion *,
                            const unsigned short int);
    void calcNormFactorsANW(float * const, const unsigned short int,
                            const unsigned short int, float * const,
                            const unsigned short int,
                            const unsigned short int);
                                                 // calculate (smooth(r)*n+s)*a
    void calcOffsetOP(SinogramConversion *, SinogramConversion *,
                      SinogramConversion *, const unsigned short int);
                                                // calculate forward projection
    SinogramConversion *fwd_proj3d(ImageConversion * const,
                                   const unsigned short int);
    float *fwd_proj3d(float *, const unsigned short int);
                                    // initialize time-of-flight reconstruction
    void initTOF(const float, const float, const unsigned short int);
                                      // calculate maximum intensity projection
    float *mip_proj(float * const, const unsigned short int);
                                               // calculate OSEM reconstruction
    ImageConversion *reconstruct(SinogramConversion * const, const bool,
                                 const unsigned short int,
                                 const unsigned short int, float * const,
                                 const std::string, const bool,
                                 const unsigned short int);
    float *reconstruct(float * const, const unsigned short int, float * const,
                       const std::string, const unsigned short int);
                              // calculate OSEM reconstruction with Gibbs prior
    ImageConversion *reconstructGibbs(SinogramConversion * const, const bool,
                                      const unsigned short int, float const,
                                      const unsigned short int, float * const,
                                      const std::string,
                                      const unsigned short int);
    float *reconstructGibbs(float * const, const unsigned short int,
                            float const, float * const, const std::string,
                            const unsigned short int);
#ifdef UNUSED
                                              // calculate OSEM step for subset
    void reconstructMP_slave(float * const, float * const, float * const,
                             float * const, const unsigned short int,
                             const unsigned short int,
                             const unsigned short int);
#endif
                                              // divide emission by attenuation
    void uncorrectEmissionAW(SinogramConversion *, SinogramConversion *,
                             const bool, const unsigned short int) const;
    void uncorrectEmissionAW(float * const, float * const,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int) const;
                                            // divide emission by normalization
    void uncorrectEmissionNW(SinogramConversion *, SinogramConversion *,
                             const bool, const unsigned short int) const;
    void uncorrectEmissionNW(float * const, float * const,
                             const unsigned short int,
                             const unsigned short int,
                             const unsigned short int) const;
                          // divide emission by product of norm and attenuation
    void uncorrectEmissionANW(SinogramConversion *, SinogramConversion *,
                              const bool, SinogramConversion *, const bool,
                              const unsigned short int) const;
    void uncorrectEmissionANW(float * const, float * const,
                              const unsigned short int,
                              const unsigned short int, float * const,
                              const unsigned short int,
                              const unsigned short int,
                              const unsigned short int) const;
 };

#endif
