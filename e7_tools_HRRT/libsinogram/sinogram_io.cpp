/*! \class SinogramIO sinogram_io.h "sinogram_io.h"
    \brief This class handles a sinogram data structure.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana (sibomana@gmail.com)
    \date 2004/03/19 initial version
    \date 2004/03/31 added Doxygen style comments
    \date 2004/04/29 save calibration information in Interfile
    \date 2004/04/30 save TOF sinograms as flat files
    \date 2004/07/21 initialize new memory block indices
    \date 2004/09/08 correct extensions for filenames when saving
    \date 2004/09/08 added stripExtension function
    \date 2004/09/13 corrected handling of decay correction factor
    \date 2004/09/16 handle "applied corrections" not in Interfile header
    \date 2004/09/27 write correct number of beds into Interfile main header
    \date 2004/11/05 added "sumCounts" method
    \date 2009/09/26 add scatter fraction in flat sinogram header

    This class handles a PET sinogram data structure and provides methods to
    load and save these datasets from and to files. Sinograms can consist of
    several datasets: trues, prompts and randoms or a series of time-of-flight
    bins. Each sinograms is stored as a set of axis datasets in volume mode.
    The internal dataformat can be signed char, signed short int or float.
 */

#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <limits>
#ifndef _SINOGRAM_IO_CPP
#define _SINOGRAM_IO_CPP
#include "sinogram_io.h"
#endif
#include "e7_tools_const.h"
#include "e7_common.h"
#include "ecat7.h"
#include "ecat7_attenuation.h"
#include "ecat7_norm3d.h"
#include "ecat7_scan3d.h"
#include "ecat_tmpl.h"
#include "exception.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif
#include "gm.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "norm_ecat.h"
#include "raw_io.h"
#include "str_tmpl.h"
#include "vecmath.h"

/*- constants ---------------------------------------------------------------*/
#ifndef _SINOGRAM_IO_TMPL_CPP
                         /*! filename extension of flat sinogram header file */
const std::string SinogramIO::FLAT_SHDR_EXTENSION=".h33";
                        /*! filename extension of flat sinogram dataset file */
const std::string SinogramIO::FLAT_SDATA1_EXTENSION=".s";
                             /*! filename extension of flat acf dataset file */
const std::string SinogramIO::FLAT_SDATA2_EXTENSION=".a";

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] si       scan information
    \param[in] sino2d   2d sinogram ? (otherwise: 3d)

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
SinogramIO::SinogramIO(const SinogramIO::tscaninfo si, const bool sino2d)
 { e7=NULL;
   inf=NULL;
   ecf=1.0f;
   decay_factor=1.0f;
   scatter_fraction=0.0f;
   deadtime_factor.clear();
   calibration.date.year=1970;
   calibration.date.month=1;
   calibration.date.day=1;
   calibration.time.hour=0;
   calibration.time.minute=0;
   calibration.time.second=0;
   calibration.time.gmt_offset_h=0;
   calibration.time.gmt_offset_m=0;
   mnr=0;
   for (unsigned short int s=0; s < MAX_SINO; s++)
    data[s].clear();
   init(si, sino2d);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] _RhoSamples         number of bins in a projection
    \param[in] _BinSizeRho         width of bins in mm
    \param[in] _ThetaSamples       number of projections in a sinogram plane
    \param[in] _span               span
    \param[in] _mrd                maximum ring difference
    \param[in] _plane_separation   plane separation in mm
    \param[in] rings               number of crystal rings
    \param[in] _intrinsic_tilt     intrinsic tilt in degrees
    \param[in] tilted              is sinogram tilted ?
    \param[in] _lld                lower level energy threshold in keV
    \param[in] _uld                upper level energy threshold in keV
    \param[in] _start_time         start time of scan in sec
    \param[in] _frame_duration     duration of frame in sec
    \param[in] _gate_duration      duration of gate in sec
    \param[in] _halflife           halflife of isotope in sec
    \param[in] _bedpos             horizontal bed position in mm
    \param[in] _mnr                matrix number
    \param[in] sino2d              2d sinogram ? (otherwise: 3d)

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
SinogramIO::SinogramIO(const unsigned short int _RhoSamples,
                       const float _BinSizeRho,
                       const unsigned short int _ThetaSamples,
                       const unsigned short int _span,
                       const unsigned short int _mrd,
                       const float _plane_separation,
                       const unsigned short int rings,
                       const float _intrinsic_tilt, const bool tilted,
                       const unsigned short int _lld,
                       const unsigned short int _uld, const float _start_time,
                       const float _frame_duration, const float _gate_duration,
                       const float _halflife, const float _bedpos,
                       const unsigned short int _mnr, const bool sino2d)
 { e7=NULL;
   inf=NULL;
   ecf=1.0f;
   decay_factor=1.0f;
   scatter_fraction=0.0f;
   deadtime_factor.clear();
   calibration.date.year=1970;
   calibration.date.month=1;
   calibration.date.day=1;
   calibration.time.hour=0;
   calibration.time.minute=0;
   calibration.time.second=0;
   calibration.time.gmt_offset_h=0;
   calibration.time.gmt_offset_m=0;
   for (unsigned short int s=0; s < MAX_SINO; s++)
    data[s].clear();
   mnr=_mnr;
   init(_RhoSamples, _BinSizeRho, _ThetaSamples, _span, _mrd,
        _plane_separation, rings, _intrinsic_tilt, tilted, _lld, _uld,
        _start_time, _frame_duration, _gate_duration, _halflife, _bedpos,
        sino2d);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy the instance of this class.

    Release the used memory and destroy the instance of this class.
 */
/*---------------------------------------------------------------------------*/
SinogramIO::~SinogramIO()
 { for (unsigned short int s=0; s < MAX_SINO; s++)
    { for (unsigned short int axis=0; axis < data[s].size(); axis++)
       MemCtrl::mc()->deleteBlockForce(&data[s][axis]);
      data[s].clear();
    }
   if (e7 != NULL) delete e7;
   if (inf != NULL) delete inf;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of axes in the sinogram.
    \return number of axes in the sinogram

    Request the number of axes in the sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::axes() const
 { return(axis_slices.size());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of planes in the sinogram.
    \return number of planes in the sinogram

    Request the number of planes in the sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::axesSlices() const
 { return(axes_slices);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a pointer to an array with the number of planes per axis.
    \return pointer to array with the number of planes per axis

    Request a pointer to an array with the number of planes per axis. The size
    of the array is axes().
 */
/*---------------------------------------------------------------------------*/
std::vector <unsigned short int> SinogramIO::axisSlices() const
 { return(axis_slices);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Did the bed move into the gantry during scanning ?
    \return did the bed move into the gantry during scanning ?

    Did the bed move into the gantry during scanning ?
 */
/*---------------------------------------------------------------------------*/
bool SinogramIO::bedMovesIn() const
 { return(bed_moves_in);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the horizontal bed position in mm.
    \return horizontal bed position in mm

    Request the horizontal bed position in mm.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::bedPos() const
 { return(bedpos);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the width of a sinogram bin in mm.
    \return width of a sinogram bin in mm

    Request the width of a sinogram bin in mm.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::BinSizeRho() const
 { return(vBinSizeRho);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request date and time of calibration.
    \return date and time of calibration

    Request date and time of calibration.
 */
/*---------------------------------------------------------------------------*/
SinogramIO::tcalibration_datetime SinogramIO::calibrationTime() const
 { return(calibration);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Copy a sinogram dataset into the object.
    \param[in] dptr       pointer to sinogram dataset
    \param[in] s          number of dataset in object
                          (trues=0, p/r=0/1, tof=0-8)
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Copy a sinogram dataset into the object.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void SinogramIO::copyData(T *dptr, const unsigned short int s,
                          const std::string name,
                          const unsigned short int loglevel)
 { deleteData(s);
   resizeIndexVec(s, axes());
   for (unsigned short int axis=0; axis < axes(); axis++)
    { T *dp;
                                  // store each axis in a separate memory block
      if (typeid(T) == typeid(char))
       dp=(T *)MemCtrl::mc()->createChar(axis_size[axis], &data[s][axis], name,
                                         loglevel);
       else if (typeid(T) == typeid(float))
             dp=(T *)MemCtrl::mc()->createFloat(axis_size[axis],
                                                &data[s][axis], name,
                                                loglevel);
       else if (typeid(T) == typeid(signed short int))
             dp=(T *)MemCtrl::mc()->createSInt(axis_size[axis], &data[s][axis],
                                               name, loglevel);
             else break;
      memcpy(dp, dptr, axis_size[axis]*sizeof(T));
      MemCtrl::mc()->put(data[s][axis]);
      dptr+=axis_size[axis];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy a sinogram dataset into the object.
    \param[in] dptr           pointer to sinogram dataset
    \param[in] s              number of dataset in object
                          (trues=0, p/r=0/1, tof=0-8)
    \param[in] _axes          number of axes in sinogram
    \param[in] planes         number of planes in segment 0 of sinogram
    \param[in] _tof_bins      number of time-of-flight bins in sinogram
    \param[in] tof_shuffled   tof data shuffled ?
    \param[in] _acf_data      contains sinogram transmission data ?
    \param[in] name           prefix for swap filenames
    \param[in] loglevel       level for logging

    Copy a sinogram dataset into the object.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void SinogramIO::copyData(T * dptr, const unsigned short int s,
                          const unsigned short int _axes,
                          const unsigned short int planes,
                          const unsigned short int _tof_bins,
                          const bool tof_shuffled,
                          const bool _acf_data, const std::string name,
                          const unsigned short int loglevel)
 { unsigned short int axis;

   matrix_frame=0;
   matrix_plane=0;
   matrix_gate=0;
   matrix_data=0;
   matrix_bed=0;
   acf_data=_acf_data;
   tof_bins=_tof_bins;
   shuffled_tof_data=tof_shuffled;
                                                           // create axes table
   axis_slices.resize(_axes);
   axis_slices[0]=planes;//2*rings-1;
   axes_slices=axis_slices[0];
   axis_size.resize(axes());
   axis_size[0]=(unsigned long int)axis_slices[0]*planesize;
   if (shuffled_tof_data)
    axis_size[0]*=(unsigned long int)tof_bins;
   for (axis=1; axis < axes(); axis++)
    { axis_slices[axis]=2*(axis_slices[0]-2*(axis*span()-(span()-1)/2));
      axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
      if (shuffled_tof_data)
       axis_size[axis]*=(unsigned long int)tof_bins;
      axes_slices+=axis_slices[axis];
    }
   copyData(dptr, s, name, loglevel);                  // copy data into object
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy a sinogram header into the object.
    \param[in] mh       pointer to an ECAT7 main header
    \param[in] infhdr   pointer to an Interfile object

    Copy a sinogram header into the object. In case of an ECAT7 dataset, only
    the main header is copied and an empty directory structure is created. In
    case of an Interfile object the main header and subheader stored in this
    object are copied and transformed into sinogram headers, if necessary.
    Previously stored headers are lost. Unused parameters can be NULL.
 */
/*---------------------------------------------------------------------------*/
#ifndef _SINOGRAM_IO_TMPL_CPP
void SinogramIO::copyFileHeader(ECAT7_MAINHEADER *mh, Interfile *infhdr)
 { if (mh != NULL)
    { if (e7 != NULL) delete e7;
      e7=new ECAT7();
      e7->CreateMainHeader();
      *(e7->MainHeader())=*mh;                        // copy ECAT7 main header
      e7->CreateDirectoryStruct();
    }
   if (infhdr != NULL)
    { if (inf != NULL) delete inf;
      inf=new Interfile();
      *inf=*infhdr;          // copy all headers stored in the Interfile object
     // remove all main header keys that don't belong in a sinogram main header
      inf->convertToSinoMainHeader();
      try { Logging::flog()->loggingOn(false);
            inf->Main_number_of_TOF_time_bins();
            Logging::flog()->loggingOn(true);
          }
      catch (const Exception r)
       { Logging::flog()->loggingOn(true);
         if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING)
          inf->MainKeyAfterKey(inf->Main_number_of_TOF_time_bins(1),
                               inf->Main_DATA_MATRIX_DESCRIPTION());
          else throw;
       }
       // remove all main header keys that don't belong in a sinogram subheader
      inf->convertToSinoSubheader();
      try { Logging::flog()->loggingOn(false);
            inf->Sub_number_of_bucket_rings();
            Logging::flog()->loggingOn(true);
          }
      catch (const Exception r)
       { Logging::flog()->loggingOn(true);
         if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING)
          inf->SubKeyAfterKey(inf->Sub_number_of_bucket_rings(0),
                              inf->Sub_IMAGE_DATA_DESCRIPTION());
          else throw;
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the corrections that were applied on the data.
    \return bitmask of corrections that were applied on the data.

    Request the corrections that were applied on the data. The return value is
    a bitmask encoding the different possible corrections. See the
    <I>CORR_</I> flags in the file e7_tools_const.h for a list of corrections.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::correctionsApplied() const
 { return(corrections_applied);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the corrections that were applied on the data.
    \param[in] ca   bitmask of applied corrections

    Set the corrections that were applied on the data. The parameter is a
    bitmask encoding the different corrections. See the <I>CORR_</I> flags in
    the file e7_tools_const.h for a list of corrections.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::correctionsApplied(const unsigned short int ca)
 { corrections_applied=ca;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the deadtime correction factor.
    \return deadtime correction factor

    Request the deadtime correction factor.
 */
/*---------------------------------------------------------------------------*/
std::vector <float> SinogramIO::deadTimeFactor() const
 { return(deadtime_factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the decay correction factor.
    \return decay correction factor

    Request the decay correction factor.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::decayFactor() const
 { return(decay_factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete a sinogram dataset.
    \param[in] s   number of dataset

    Delete a sinogram dataset.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::deleteData(const unsigned short int s)
 { for (unsigned short int axis=0; axis < data[s].size(); axis++)
    MemCtrl::mc()->deleteBlock(&data[s][axis]);
   data[s].clear();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete one axis of a sinogram dataset.
    \param[in] axis   axis of sinogram
    \param[in] s      number of dataset

    Delete one axis of a sinogram dataset.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::deleteData(const unsigned short int axis,
                            const unsigned short int s)
 { if (data[s].size() > axis)
    MemCtrl::mc()->deleteBlock(&data[s][axis]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete a sinogram.

    Delete all axes of all sinogram datasets.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::deleteData()
 { for (unsigned short int s=0; s < MAX_SINO; s++)
    deleteData(s);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a pointer to the ECAT7 main header.
    \return pointer to the ECAT7 main header.

    Request a pointer to the ECAT7 main header. If no such header is stored in
    this object, NULL is returned.
 */
/*---------------------------------------------------------------------------*/
ECAT7_MAINHEADER *SinogramIO::ECAT7mainHeader() const
 { if (e7 == NULL) return(NULL);
   return(e7->MainHeader());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the ECAT calibration factor.
    \return ECAT calibration factor

    Request the ECAT calibration factor.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::ECF() const
 { return(ecf);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Was the patient scanned feet first ?
    \return patient was scanned feet first

    Was the patient scanned feet first ?
 */
/*---------------------------------------------------------------------------*/
bool SinogramIO::feetFirst() const
 { return(feet_first);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the name of the file.
    \return name of the file

    Request the name of the file.
 */
/*---------------------------------------------------------------------------*/
std::string SinogramIO::fileName() const
 { return(filename);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the frame duration of the scan.
    \return frame duration of the scan in sec

    Request the frame duration of the scan in sec.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::frameDuration() const
 { return(frame_duration);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the gate duration of the scan.
    \return gate duration of the scan in sec

    Request the gate duration of the scan in sec.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::gateDuration() const
 { return(gate_duration);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the halflife of the isotope.
    \return halflife of the isotope in sec

    Request the halflife of the isotope in sec
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::halflife() const
 { return(vhalflife);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request memory controller index of a dataset.
    \param[in] axis   axis of sinogram
    \param[in] s      number of sinogram dataset
    \return index for memory controller

    Request memory controller index of a dataset.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::index(const unsigned short int axis,
                                     const unsigned short int s) const
 { if (data[s].size() > axis) return(data[s][axis]);
   return(std::numeric_limits <unsigned short int>::max());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize data structures of object.
    \param[in] si       scan information
    \param[in] sino2d   2d sinogram ? (otherwise: 3d)

    Initialize data structures of object.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::init(const SinogramIO::tscaninfo si, const bool sino2d)
 { init(GM::gm()->RhoSamples(), GM::gm()->BinSizeRho(),
        GM::gm()->ThetaSamples(), GM::gm()->span(), GM::gm()->mrd(),
        GM::gm()->planeSeparation(), GM::gm()->rings(),
        GM::gm()->intrinsicTilt(), true, si.lld, si.uld, si.start_time,
        si.frame_duration, si.gate_duration, si.halflife, si.bedpos, sino2d);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize data structures of object.
    \param[in] _RhoSamples         number of bins in a projection
    \param[in] _BinSizeRho         width of bins in mm
    \param[in] _ThetaSamples       number of projections in a sinogram plane
    \param[in] _span               span
    \param[in] _mrd                maximum ring difference
    \param[in] _plane_separation   plane separation in mm
    \param[in] rings               number of crystal rings
    \param[in] _intrinsic_tilt     intrinsic tilt in degrees
    \param[in] tilted              is sinogram tilted ?
    \param[in] _lld                lower level energy threshold in keV
    \param[in] _uld                upper level energy threshold in keV
    \param[in] _start_time         start time of scan in sec
    \param[in] _frame_duration     duration of frame in sec
    \param[in] _gate_duration      duration of gate in sec
    \param[in] _halflife           halflife of isotope in sec
    \param[in] _bedpos             horizontal bed position in mm
    \param[in] sino2d              2d sinogram ? (otherwise: 3d)

    Initialize data structures of object.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::init(const unsigned short int _RhoSamples,
                      const float _BinSizeRho,
                      const unsigned short int _ThetaSamples,
                      const unsigned short int _span,
                      const unsigned short int _mrd,
                      const float _plane_separation,
                      const unsigned short int rings,
                      const float _intrinsic_tilt, const bool tilted,
                      const unsigned short int _lld,
                      const unsigned short int _uld, const float _start_time,
                      const float _frame_duration, const float _gate_duration,
                      const float _halflife, const float _bedpos,
                      const bool sino2d)
 { vRhoSamples=_RhoSamples;
   vThetaSamples=_ThetaSamples;
   vspan=_span;
   vmrd=_mrd;
   shuffled_tof_data=false;
                                // flat panel scanners don't have crystal rings
   vBinSizeRho=_BinSizeRho;
   lld=_lld;
   uld=_uld;
   plane_separation=_plane_separation;
   intrinsic_tilt=_intrinsic_tilt;
   start_time=_start_time,
   frame_duration=_frame_duration;
   gate_duration=_gate_duration;
   vhalflife=_halflife;
   bedpos=_bedpos;
   tof_bins=1;
                                                      // size of sinogram plane
   planesize=(unsigned long int)RhoSamples()*
             (unsigned long int)ThetaSamples();
                                                           // create axes table
   if (sino2d || (span() == 0)) axis_slices.resize(1);
    else axis_slices.resize((mrd()-(span()-1)/2)/span()+1);
   axis_slices[0]=2*rings-1;
   axes_slices=axis_slices[0];
   axis_size.resize(axes());
   axis_size[0]=(unsigned long int)axis_slices[0]*planesize;
   for (unsigned short int axis=1; axis < axes(); axis++)
    { axis_slices[axis]=2*(axis_slices[0]-2*(axis*span()-(span()-1)/2));
      axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
      axes_slices+=axis_slices[axis];
    }
                                                         // mashing of sinogram
   if ((ThetaSamples() != 0) && GM::gm()->initialized())
    vmash=GM::gm()->crystalsPerRing()/2/ThetaSamples();
    else vmash=1;
   corrections_applied=0;
   if (!tilted) corrections_applied|=CORR_untilted;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize axis of sinogram and fill it with zeros.
    \param[in] axis           axis of sinogram
    \param[in] s              number of sinogram dataset
    \param[in] _tof_bins      number of time-of-flight bins in sinogram
    \param[in] tof_shuffled   tof data shuffled ?
    \param[in] _acf_data      contains sinogram transmission data ?
    \param[in] name           prefix for swap filenames
    \param[in] loglevel       level for logging

    Initialize axis of sinogram and fill it with zeros.
 */
/*---------------------------------------------------------------------------*/
float *SinogramIO::initFloatAxis(const unsigned short int axis,
                                 const unsigned short int s,
                                 const unsigned short int _tof_bins,
                                 const bool tof_shuffled, const bool _acf_data,
                                 const std::string name,
                                 const unsigned short int loglevel)
 { float *dp;

   acf_data=_acf_data;
   tof_bins=_tof_bins;
   shuffled_tof_data=tof_shuffled;
   if (data[s].size() <= axis) resizeIndexVec(s, axis+1);
    else MemCtrl::mc()->deleteBlock(&data[s][axis]);
   if (shuffled_tof_data)
    axis_size[axis]*=(unsigned long int)tof_bins;
   dp=MemCtrl::mc()->createFloat(axis_size[axis], &data[s][axis], name,
                                 loglevel);
   memset(dp, 0, axis_size[axis]*sizeof(float));
   return(dp);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a pointer to the Interfile object.
    \return pointer to the Interfile object.

    Request a pointer to the Interfile object.
 */
/*---------------------------------------------------------------------------*/
Interfile *SinogramIO::InterfileHdr() const
 { return(inf);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Reqeust the intrinsic tilt of the sinogram.
    \return intrinsic tilt of the sinogram in degrees

    Request the intrinsic tilt of the sinogram in degrees.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::intrinsicTilt() const
 { if (corrections_applied & CORR_untilted) return(0.0f);
   return(intrinsic_tilt);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Contains the sinogram transmission data ?
    \return the sinogram contrains transmission data

    Contains the sinogram transmission data ?
 */
/*---------------------------------------------------------------------------*/
bool SinogramIO::isACF() const
 { return(acf_data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the sinogram data to transmission or emission.
    \return contains the sinogram transmission data ?

    Set the sinogram data to transmission or emission.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::isACF(const bool _acf_data)
 { acf_data=_acf_data;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request lower level energy threshold of the scan.
    \return lower level energy threshold of the scan in keV

    Request lower level energy threshold of the scan.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::LLD() const
 { return(lld);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a sinogram from file.
    \param[in] _filename       name of file
    \param[in] is_acf          is dataset a transmission sinogram ?
    \param[in] arc_corrected   is sinogram arc corrected ?
    \param[in] _mnr            number of matrix in file
    \param[in] name            prefix for swap filenames
    \param[in] loglevel        level for logging
    \param[in] pd_sep          does file contain separate prompts and randoms ?

    Load a sinogram from file. Supported formats are ECAT7, Interfile and flat
    (IEEE float or signed short integer). A previously stored sinogram is lost.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::load(const std::string _filename, const bool is_acf,
                      const bool arc_corrected, const unsigned short int _mnr,
                      const std::string name,
                      const unsigned short int loglevel, const bool pd_sep)
 { filename=_filename;
   mnr=_mnr;
   deleteData();
   Logging::flog()->logMsg("load sinogram '#1'", loglevel)->arg(filename);
   Logging::flog()->logMsg("matrix number=#1", loglevel+1)->arg(mnr);
   acf_data=is_acf;
                                         // is this a time-of-flight sinogram ?
   if (isECAT7file(filename)) loadECAT7(name, loglevel+2);
    else if (isInterfile(filename)) loadInterfile(name, loglevel+2);
          else { if (numberOfSinograms() > 2) tof_bins=numberOfSinograms();
                  else tof_bins=1;
                 loadRAW(pd_sep, arc_corrected, name, loglevel);
               }
                                         // is this a time-of-flight sinogram ?
   if (numberOfSinograms() > 2)
    { tof_bins=numberOfSinograms();
      if (prompts_and_randoms) tof_bins--;
    }
    else tof_bins=1;
   logGeometry(loglevel+1);                         // log geometry of sinogram
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load components for normalization.
    \param[in] singles    singles from emission scan
    \param[in] loglevel   level for logging

    Load a normalization dataset from a flat file. The components of the norm
    are expanded into a 3d sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadComponentNorm(const std::vector <float> singles,
                                   const unsigned short int loglevel)
 { RawIO <float> *rio=NULL;
   NormECAT *norm_ecat=NULL;

   try
   { std::vector <float> paralyzing_ring_DT_param(GM::gm()->rings()),
                         non_paralyzing_ring_DT_param(GM::gm()->rings()),
                         plane_corr(RhoSamples()*axis_slices[0]),
                         intfcorr((GM::gm()->transaxialCrystalsPerBlock()+
                                   GM::gm()->transaxialBlockGaps())*
                                   RhoSamples()),
                         crystal_eff(GM::gm()->crystalsPerRing()*
                                    GM::gm()->rings()),
                         axial_pf;
     float *n3d, *norm3d;
     unsigned short int norm_idx;
     unsigned long int fsize;
     bool has_axial_component=false;

     mnr=0;
     deleteData();
     acf_data=true;
     tof_bins=1;
     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     start_time=0.0f;
     frame_duration=0.0f;
     gate_duration=0.0f;
     vhalflife=0.0f;
     scantype=SCANTYPE::SINGLEFRAME;
     corrections_applied=0;

     prompts_and_randoms=false;
     scatter_fraction=0.0f;
     deadtime_factor.clear();
     calibration.date.year=1970;
     calibration.date.month=1;
     calibration.date.day=1;
     calibration.time.hour=0;
     calibration.time.minute=0;
     calibration.time.second=0;
     calibration.time.gmt_offset_h=0;
     calibration.time.gmt_offset_m=0;
     Logging::flog()->logMsg("load component norm data", loglevel+1);

     fsize=RhoSamples()*axis_slices[0]+
           (GM::gm()->transaxialCrystalsPerBlock()+
            GM::gm()->transaxialBlockGaps())*RhoSamples()+
           GM::gm()->crystalsPerRing()*GM::gm()->rings()+2*GM::gm()->rings()+
           GM::gm()->transaxialCrystalsPerBlock()+
           GM::gm()->transaxialBlockGaps();

     rio=new RawIO <float>(filename, false, false);
                                                           // geometric effects
     rio->read((float *)&plane_corr[0], RhoSamples()*axis_slices[0]);
                                                        // crystal interference
     rio->read((float *)&intfcorr[0], (GM::gm()->transaxialCrystalsPerBlock()+
                                       GM::gm()->transaxialBlockGaps())*
                                      RhoSamples());
                                                        // crystal efficiencies
     rio->read((float *)&crystal_eff[0],
               GM::gm()->crystalsPerRing()*GM::gm()->rings());
     if (rio->size() == (fsize+axes_slices)*sizeof(float))     // axial effects
      { axial_pf.resize(axes_slices);
        rio->read((float *)&axial_pf[0], axes_slices);
        has_axial_component=true;
      }
      else if (rio->size() != fsize*sizeof(float))
            throw Exception(REC_NORM_FILE_INVALID, "The size of the norm file "
                            "'#1' is different than expected.").arg(filename);
                                               // paralyzing ring DT parameters
     rio->read((float *)&paralyzing_ring_DT_param[0], GM::gm()->rings());
                                           // non-paralyzing ring DT parameters
     rio->read((float *)&non_paralyzing_ring_DT_param[0], GM::gm()->rings());
     /*                                                              never used
                                                     // TX crystal DT parameter
     std::vector <float> TX_crystal_DT_param(
                                        GM::gm()->transaxialCrystalsPerBlock()+
                                        GM::gm()->transaxialBlockGaps());
     rio->read((float *)&TX_crystal_DT_param[0],
               GM::gm()->transaxialCrystalsPerBlock()+
               GM::gm()->transaxialBlockGaps());
     */
     delete rio;
     rio=NULL;
     Logging::flog()->logMsg("expand norm data", loglevel+1);
     norm_ecat=new NormECAT(RhoSamples(), ThetaSamples(), axisSlices(),
                            GM::gm()->transaxialCrystalsPerBlock()+
                            GM::gm()->transaxialBlockGaps(),
                            GM::gm()->transaxialBlocksPerBucket(),
                            GM::gm()->axialCrystalsPerBlock()+
                            GM::gm()->axialBlockGaps(),
                            GM::gm()->crystalsPerRing(),
                            (float *)&plane_corr[0], axis_slices[0],
                            (float *)&intfcorr[0],
                            (float *)&crystal_eff[0],
                            (float *)&paralyzing_ring_DT_param[0],
                            (float *)&non_paralyzing_ring_DT_param[0],
                            singles, 0, NULL);
     norm_idx=norm_ecat->calcNorm(loglevel+1);
     delete norm_ecat;
     norm_ecat=NULL;
     norm3d=MemCtrl::mc()->getFloat(norm_idx, loglevel+1);
     n3d=norm3d;
                                               // multiply with axial component
     if (has_axial_component)
      for (unsigned short int p=0; p < axes_slices; p++,
           n3d+=planesize)
       vecMulScalar(n3d, axial_pf[p], n3d, planesize);
                                       // transfer dataset to memory controller
     copyData(norm3d, 0, "norm", loglevel);
     MemCtrl::mc()->put(norm_idx);
     MemCtrl::mc()->deleteBlock(&norm_idx);
   }
   catch (...)
    { if (rio != NULL) delete rio;
      if (norm_ecat != NULL) delete norm_ecat;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a sinogram from an ECAT7 file.
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging
    \exception REC_UNKNOWN_ECAT7_MATRIXTYPE unknown matrix type in file
    \exception REC_COINSAMPMODE unknown scan mode

    Load a sinogram from an ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadECAT7(const std::string name,
                           const unsigned short int loglevel)
 { unsigned short int axis, ds;

   if (e7 != NULL) delete e7;
   e7=new ECAT7();
   e7->LoadFile(filename);
   GM::gm()->init(toString(e7->Main_system_type()));
   matrix_frame=e7->Dir_frame(mnr);
   matrix_plane=e7->Dir_plane(mnr);
   matrix_gate=e7->Dir_gate(mnr);
   matrix_data=e7->Dir_data(mnr);
   matrix_bed=e7->Dir_bed(mnr);
                                           // get sinogram geometry from header
   switch (e7->Main_file_type())
    { case E7_FILE_TYPE_AttenuationCorrection:
       vThetaSamples=e7->Attn_num_angles(mnr);
       vRhoSamples=e7->Attn_num_r_elements(mnr);
       planesize=(unsigned long int)RhoSamples()*
                 (unsigned long int)ThetaSamples();
       axis_slices.clear();
       axes_slices=0;
       axis_size.clear();
       for (axis=0; axis < 64; axis++)
        if (e7->Attn_z_elements(axis, mnr) == 0) break;
         else { axis_slices.push_back(e7->Attn_z_elements(axis, mnr));
                axis_size.push_back((unsigned long int)axis_slices[axis]*
                                    planesize);
                axes_slices+=axis_slices[axis];
              }
       start_time=0.0f;
       frame_duration=0.0f;
       gate_duration=0.0f;
       vhalflife=e7->Main_isotope_halflife();
       corrections_applied=0;
       vspan=0;
       vmrd=e7->Attn_ring_difference(mnr);
       break;
      case E7_FILE_TYPE_3D_Sinogram16:
      case E7_FILE_TYPE_3D_SinogramFloat:
       vThetaSamples=e7->Scan3D_num_angles(mnr);
       vRhoSamples=e7->Scan3D_num_r_elements(mnr);
       planesize=(unsigned long int)RhoSamples()*
                 (unsigned long int)ThetaSamples();
       axis_slices.clear();
       axes_slices=0;
       axis_size.clear();
       for (axis=0; axis < 64; axis++)
        if (e7->Scan3D_num_z_elements(axis, mnr) == 0) break;
         else { axis_slices.push_back(e7->Scan3D_num_z_elements(axis, mnr));
                axis_size.push_back((unsigned long int)axis_slices[axis]*
                                    planesize);
                axes_slices+=axis_slices[axis];
              }
       start_time=(float)e7->Scan3D_frame_start_time(mnr)/1000.0f;
       frame_duration=(float)e7->Scan3D_frame_duration(mnr)/1000.0f;
       gate_duration=(float)e7->Scan3D_gate_duration(mnr)/1000.0f,
       vhalflife=e7->Main_isotope_halflife();
       { signed short int c;                          // read corrections flags

         corrections_applied=0;
         c=e7->Scan3D_corrections_applied(mnr);
         if (c & E7_APPLIED_PROC_Normalized)
          corrections_applied|=CORR_Normalized;
         if (c & E7_APPLIED_PROC_Measured_Attenuation_Correction)
          corrections_applied|=CORR_Measured_Attenuation_Correction;
         if (c & E7_APPLIED_PROC_Calculated_Attenuation_Correction)
          corrections_applied|=CORR_Calculated_Attenuation_Correction;
         if (c & E7_APPLIED_PROC_2D_scatter_correction)
          corrections_applied|=CORR_2D_scatter_correction;
         if (c & E7_APPLIED_PROC_3D_scatter_correction)
          corrections_applied|=CORR_3D_scatter_correction;
         if (c & E7_APPLIED_PROC_Arc_correction)
          corrections_applied|=CORR_Radial_Arc_correction|
                               CORR_Axial_Arc_correction;
         if (c & E7_APPLIED_PROC_Decay_correction)
          corrections_applied|=CORR_Decay_correction;
         if (c &  E7_APPLIED_PROC_FORE) corrections_applied|=CORR_FORE;
         if (c &  E7_APPLIED_PROC_SSRB) corrections_applied|=CORR_SSRB;
         if (c &  E7_APPLIED_PROC_SEG0) corrections_applied|=CORR_SEG0;
         if (c & E7_APPLIED_PROC_Randoms_Smoothing)
          corrections_applied|=CORR_Randoms_Smoothing;
         if ((c & E7_APPLIED_PROC_X_smoothing) &&
             (c & E7_APPLIED_PROC_Y_smoothing))
          corrections_applied|=CORR_XYSmoothing;
         if (c & E7_APPLIED_PROC_Z_smoothing)
          corrections_applied|=CORR_ZSmoothing;
       }
       vspan=e7->Scan3D_axial_compression(mnr);
       vmrd=e7->Scan3D_ring_difference(mnr);
       vsingles.resize(128);
       { unsigned short int bucket;

         for (bucket=0; bucket < 128; bucket++)
          vsingles[bucket]=e7->Scan3D_uncor_singles(bucket, mnr);
       }
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_MATRIXTYPE,
                       "Unknown ECAT7 matrix type '#1'.").arg(filename);
       break;
    }
   vmash=1 << e7->Main_angular_compression();
   vBinSizeRho=e7->Main_bin_size()*10.0f;
   plane_separation=e7->Main_plane_separation()*10.0f;
   lld=e7->Main_lwr_true_thres();
   uld=e7->Main_upr_true_thres();
   scatter_fraction=0.0f;
   deadtime_factor.clear();
   calibration.date.year=1970;
   calibration.date.month=1;
   calibration.date.day=1;
   calibration.time.hour=0;
   calibration.time.minute=0;
   calibration.time.second=0;
   calibration.time.gmt_offset_h=0;
   calibration.time.gmt_offset_m=0;
   ecf=e7->Main_ecat_calibration_factor();
   intrinsic_tilt=e7->Main_intrinsic_tilt();
   bedpos=bedPosition(e7, mnr);
   switch (e7->Main_patient_orientation())
    { case E7_PATIENT_ORIENTATION_Feet_First_Prone:
      case E7_PATIENT_ORIENTATION_Feet_First_Supine:
      case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Right:
      case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Left:
       feet_first=true;
       Logging::flog()->logMsg("patient orientation: feet first", loglevel);
       break;
      default:
       feet_first=false;
       Logging::flog()->logMsg("patient orientation: head first", loglevel);
       break;
    }
   bed_moves_in=((e7->Main_bed_position(0) > 0.0f) != GM::gm()->isPETCT());
   if (bed_moves_in)
    Logging::flog()->logMsg("bed moving direction: in", loglevel);
    else Logging::flog()->logMsg("bed moving direction: out", loglevel);
   if (e7->Main_num_bed_pos() > 0) scantype=SCANTYPE::MULTIBED;
    else if (e7->Main_num_gates() > 1) scantype=SCANTYPE::MULTIGATE;
    else if (e7->Main_num_frames() > 1) scantype=SCANTYPE::MULTIFRAME;
    else scantype=SCANTYPE::SINGLEFRAME;
   shuffled_tof_data=false;
   switch (e7->Main_coin_samp_mode())
    { case E7_COIN_SAMP_MODE_PromptsDelayed:
       ds=2;
       prompts_and_randoms=true;
       break;
      case E7_COIN_SAMP_MODE_NetTrues:
       ds=1;
       prompts_and_randoms=false;
       break;
      case E7_COIN_SAMP_MODE_TruesTOF:
       ds=9;
       prompts_and_randoms=false;
       break;
      default:
       throw Exception(REC_COINSAMPMODE,
                       "Data in '#1' acquired in unknown scan mode"
                       ".").arg(filename);
    }
   for (unsigned short int s=mnr; s < mnr+ds; s++)
    { if (ds == 9)
       Logging::flog()->logMsg("load TOF frame #1", loglevel)->arg(s-mnr+1);
      e7->LoadData(s);
                                     // convert from view mode to volume mode ?
      switch (e7->Main_file_type())
       { case E7_FILE_TYPE_AttenuationCorrection:
          if (e7->Attn_storage_order(s) == E7_STORAGE_ORDER_RZTD)
           { ((ECAT7_ATTENUATION *)e7->Matrix(s))->View2Volume();
             e7->Attn_storage_order(E7_STORAGE_ORDER_RTZD, s);
           }
          break;
         case E7_FILE_TYPE_3D_Sinogram16:
         case E7_FILE_TYPE_3D_SinogramFloat:
          if (e7->Scan3D_storage_order(s) == E7_STORAGE_ORDER_RZTD)
           { ((ECAT7_SCAN3D *)e7->Matrix(s))->View2Volume();
             e7->Scan3D_storage_order(E7_STORAGE_ORDER_RTZD, s);
           }
          break;
       }
      switch (e7->DataType(s))
       { case E7_DATATYPE_BYTE:
          copyData((signed char *)e7->MatrixData(s), s-mnr, name, loglevel);
          break;
         case E7_DATATYPE_SHORT:
          copyData((signed short int *)e7->MatrixData(s), s-mnr, name,
                   loglevel);
          break;
         case E7_DATATYPE_FLOAT:
          copyData((float *)e7->MatrixData(s), s-mnr, name, loglevel);
          break;
       }
      e7->DeleteData(s);
      switch (e7->Main_file_type())
       { case E7_FILE_TYPE_AttenuationCorrection:
          scale(e7->Attn_scale_factor(s), s-mnr, loglevel);
          break;
         case E7_FILE_TYPE_3D_Sinogram16:
         case E7_FILE_TYPE_3D_SinogramFloat:
          scale(e7->Scan3D_scale_factor(s), s-mnr, loglevel);
          break;
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a normalization dataset from an ECAT7 file.
    \param[in] _RhoSamples     number of bins in a projection
    \param[in] _ThetaSamples   number of projections in a sinogram plane
    \param[in] _axis_slices    array with number of planes per axis
    \param[in] singles         singles from emission scan
    \param[in] loglevel        level for logging
    \exception REC_INVALID_NORM   unknown normalization file type

    Load a normalization dataset from an ECAT7 file. The normalization is
    expanded into the given size of a sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadECAT7Norm(const unsigned short int _RhoSamples,
                           const unsigned short int _ThetaSamples,
                           const std::vector <unsigned short int> _axis_slices,
                           const std::vector <float> singles,
                           const unsigned short int loglevel)
 { ECAT7 *e7_norm=NULL;
   NormECAT *norm_ecat=NULL;

   try
   { ECAT7_NORM3D *nsh;
     std::vector <float> ring_dtcor1, ring_dtcor2;
     unsigned short int norm_idx;

     e7_norm=new ECAT7();
     e7_norm->LoadFile(filename);
     if (e7_norm->Main_file_type() != E7_FILE_TYPE_3D_Normalization)
      throw Exception(REC_INVALID_NORM, "unknown file type in file '"+
                                        filename+"'");
     GM::gm()->init(toString(e7_norm->Main_system_type()));
                                      // get geometry of normalization sinogram
     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     vRhoSamples=_RhoSamples;
     vThetaSamples=_ThetaSamples;
     planesize=(unsigned long int)RhoSamples()*
               (unsigned long int)ThetaSamples();
     axis_slices=_axis_slices;
     axes_slices=0;
     axis_size.resize(axes());
     for (unsigned short int axis=0; axis < axes(); axis++)
      { axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
        axes_slices+=axis_slices[axis];
      }
     start_time=0.0f;
     frame_duration=0.0f;
     gate_duration=0.0f;
     vhalflife=0.0f;
     scantype=SCANTYPE::SINGLEFRAME;
     corrections_applied=0;
     if (axes() > 1) vspan=axis_slices[0]-axis_slices[1]/2-1;
      else vspan=GM::gm()->span();
     vmrd=((axes()*2-1)*span()-1)/2;
     vmash=GM::gm()->crystalsPerRing()/2/ThetaSamples();
     vBinSizeRho=e7_norm->Main_bin_size()*10.0f;
     plane_separation=e7_norm->Main_plane_separation()*10.0f;
     lld=e7_norm->Main_lwr_true_thres();
     uld=e7_norm->Main_upr_true_thres();
     intrinsic_tilt=e7_norm->Main_intrinsic_tilt();
     bedpos=bedPosition(e7_norm, 0);
     shuffled_tof_data=false;
                                             // get deadtime correction factors
     ring_dtcor1.resize(32);
     ring_dtcor2.resize(32);
     for (unsigned short int ring=0; ring < 32; ring++)
      { ring_dtcor1[ring]=e7_norm->Norm3D_ring_dtcor1(ring, 0);
        ring_dtcor2[ring]=e7_norm->Norm3D_ring_dtcor2(ring, 0);
      }
     prompts_and_randoms=false;
     e7_norm->LoadData(0);
     nsh=(ECAT7_NORM3D *)e7_norm->Matrix(0);
                                                                 // expand norm
     norm_ecat=new NormECAT(RhoSamples(), ThetaSamples(), axisSlices(),
                            GM::gm()->transaxialCrystalsPerBlock(),
                            GM::gm()->transaxialBlocksPerBucket(),
                            GM::gm()->axialCrystalsPerBlock(),
                            GM::gm()->crystalsPerRing(),
                            nsh->PlaneCorrPtr(),
                            e7_norm->Norm3D_num_geo_corr_planes(0),
                            nsh->IntfCorrPtr(), nsh->CrystalEffPtr(),
                            &ring_dtcor1[0], &ring_dtcor2[0], singles, 0,
                            NULL);
     norm_idx=norm_ecat->calcNorm(loglevel+1);
     delete norm_ecat;
     norm_ecat=NULL;
     ecf=e7_norm->Main_ecat_calibration_factor();
     scatter_fraction=0.0f;
     deadtime_factor.clear();
     calibration.date.year=1970;
     calibration.date.month=1;
     calibration.date.day=1;
     calibration.time.hour=0;
     calibration.time.minute=0;
     calibration.time.second=0;
     calibration.time.gmt_offset_h=0;
     calibration.time.gmt_offset_m=0;
     delete e7_norm;
     e7_norm=NULL;
                                       // transfer dataset to memory controller
     copyData(MemCtrl::mc()->getFloat(norm_idx, loglevel), 0, "norm",
              loglevel);
     MemCtrl::mc()->put(norm_idx);
     MemCtrl::mc()->deleteBlock(&norm_idx);
   }
   catch (...)
    { if (e7_norm != NULL) delete e7_norm;
      if (norm_ecat != NULL) delete norm_ecat;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a sinogram from an Interfile file.
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Load a sinogram from an Interfile file.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadInterfile(const std::string name,
                               const unsigned short int loglevel)
 { RawIO <float> *rio_f=NULL;
   RawIO <signed short int> *rio_si=NULL;

   try
   { std::string subhdr_filename, unit;
     unsigned short int axis, ds;

     if (inf != NULL) delete inf;
     inf=new Interfile();
     inf->loadMainHeader(filename);
     GM::gm()->init(inf->Main_originating_system());
     if (inf->Main_number_of_horizontal_bed_offsets() > 1)
      scantype=SCANTYPE::MULTIBED;
      else if (inf->Main_number_of_time_frames() > 1)
            scantype=SCANTYPE::MULTIFRAME;
      else scantype=SCANTYPE::SINGLEFRAME;
     matrix_plane=0;
                                               // search name of subheader file
     for (unsigned short int i=0;
          i < inf->Main_total_number_of_data_sets();
          i++)
      { Interfile::tdataset ds;
        Interfile::tscan_type st;

        ds=inf->Main_data_set(i);
        inf->acquisitionCode(ds.acquisition_number, &st, &matrix_frame,
                             &matrix_gate, &matrix_bed, &matrix_data, NULL,
                             NULL);
        if ((acf_data && (st == Interfile::TRANSMISSION_TYPE)) ||
            (!acf_data && (st == Interfile::EMISSION_TYPE)))
         if (((scantype == SCANTYPE::MULTIBED) && (matrix_bed == mnr)) ||
             ((scantype != SCANTYPE::MULTIBED) && (matrix_frame == mnr)))
          { subhdr_filename=*(ds.headerfile);
            break;
          }
      }
     Logging::flog()->logMsg("load subheader #1", loglevel)->
      arg(subhdr_filename);
     inf->loadSubheader(subhdr_filename);
     vRhoSamples=inf->Sub_matrix_size(0);
     vThetaSamples=inf->Sub_matrix_size(1);
                                                    // size of a sinogram plane
     planesize=(unsigned long int)RhoSamples()*
               (unsigned long int)ThetaSamples();
                                                           // create axes table
     axis_slices.resize((inf->Sub_number_of_segments()+1)/2);
     axes_slices=0;
     axis_size.resize(axes());
     for (axis=0; axis < axes(); axis++)
      { if (axis == 0) axis_slices[axis]=inf->Sub_segment_table(0);
         else axis_slices[axis]=inf->Sub_segment_table(axis*2-1)+
                                inf->Sub_segment_table(axis*2);
        axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
        axes_slices+=axis_slices[axis];
      }
     start_time=(float)inf->Sub_image_relative_start_time(&unit);
     frame_duration=(float)inf->Sub_image_duration(&unit);
     gate_duration=0.0f;
     vhalflife=inf->Main_isotope_gamma_halflife(&unit);
     vspan=inf->Sub_axial_compression();
     vmrd=inf->Sub_maximum_ring_difference();
     try
     { Logging::flog()->loggingOn(false);
       vsingles.resize(inf->Sub_number_of_bucket_rings());
       { unsigned short int bucket;

         for (bucket=0; bucket < vsingles.size(); bucket++)
          vsingles[bucket]=inf->Sub_bucket_ring_singles(bucket);
       }
     }
     catch (...)
      { vsingles.clear();
      }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       tof_width=inf->Main_TOF_bin_width(&unit);
       tof_fwhm=inf->Main_TOF_gaussian_fwhm(&unit);
     }
     catch (...)
      { tof_width=0.0f;
        tof_fwhm=0.0f;
      }
     Logging::flog()->loggingOn(true);
     vmash=1;
     vBinSizeRho=inf->Sub_scale_factor(0, &unit);
     plane_separation=inf->Sub_scale_factor(2, &unit);
     if (inf->Main_number_of_energy_windows() > 0)
      { lld=inf->Main_energy_window_lower_level(0, &unit);
        uld=inf->Main_energy_window_upper_level(0, &unit);
      }
      else { lld=0;
             uld=0;
           }
     try
     { Logging::flog()->loggingOn(false);
       decay_factor=inf->Sub_decay_correction_factor();
     }
     catch (...)
      { decay_factor=1.0f;
      }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       scatter_fraction=inf->Sub_scatter_fraction_factor();
     }
     catch (...)
      { scatter_fraction=0.0f;
      }
     Logging::flog()->loggingOn(true);
     try
     { unsigned short int c;

       Logging::flog()->loggingOn(false);
       for (c=0;; c++)
        if (inf->Sub_dead_time_factor(c) == std::numeric_limits <float>::max())
         break;
       deadtime_factor.resize(c);
       for (unsigned short int i=0; i < c; i++)
        deadtime_factor[i]=inf->Sub_dead_time_factor(i);
     }
     catch (...)
      { deadtime_factor.clear();
      }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       calibration.date=inf->Main_calibration_date(&unit);
       calibration.time=inf->Main_calibration_time(&unit);
     }
     catch (...)
      { calibration.date.year=1970;
        calibration.date.month=1;
        calibration.date.day=1;
        calibration.time.hour=0;
        calibration.time.minute=0;
        calibration.time.second=0;
        calibration.time.gmt_offset_h=0;
        calibration.time.gmt_offset_m=0;
      }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       ecf=inf->Main_scanner_quantification_factor(&unit);
     }
     catch (...)
      { ecf=1.0;
      }
     Logging::flog()->loggingOn(true);
     intrinsic_tilt=GM::gm()->intrinsicTilt();
     bedpos=inf->Sub_start_horizontal_bed_position(&unit);
     switch (inf->Main_patient_orientation())
      { case KV::FFP:
        case KV::FFS:
        case KV::FFDR:
        case KV::FFDL:
         feet_first=true;
         Logging::flog()->logMsg("patient orientation: feet first", loglevel);
         break;
        default:
         feet_first=false;
         Logging::flog()->logMsg("patient orientation: head first", loglevel);
         break;
      }
     bed_moves_in=(inf->Main_scan_direction() == KV::SCAN_IN);
     if (bed_moves_in)
      Logging::flog()->logMsg("bed moving direction: in", loglevel);
      else Logging::flog()->logMsg("bed moving direction: out", loglevel);
     shuffled_tof_data=false;
     ds=inf->Sub_number_of_scan_data_types();
     switch (inf->Sub_scan_data_type_description(0))
      { case KV::CORRECTED_PROMPTS:
         prompts_and_randoms=false;
         break;
        case KV::PROMPTS:
         prompts_and_randoms=true;
         break;
        case KV::RANDOMS:
         prompts_and_randoms=true;
         break;
      }
     corrections_applied=0;
     for (unsigned short int c=0;; c++)               // read corrections flags
      { std::string str;

        try { Logging::flog()->loggingOn(false);
              str=inf->Sub_applied_corrections(c);
              Logging::flog()->loggingOn(true);
            }
        catch (const Exception r)
         { Logging::flog()->loggingOn(true);
           if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING)
            str=std::string();
         }
        if (str.empty()) break;
        if (str == "normalized") corrections_applied|=CORR_Normalized;
         else if (str == "measured attenuation correction")
               corrections_applied|=CORR_Measured_Attenuation_Correction;
         else if (str == "calculated attenuation correction")
               corrections_applied|=CORR_Calculated_Attenuation_Correction;
         else if (str == "2d scatter correction")
               corrections_applied|=CORR_2D_scatter_correction;
         else if (str == "3d scatter correction")
               corrections_applied|=CORR_3D_scatter_correction;
         else if (str == "radial arc-correction")
               corrections_applied|=CORR_Radial_Arc_correction;
         else if (str == "axial arc-correction")
               corrections_applied|=CORR_Axial_Arc_correction;
         else if (str == "decay correction")
               corrections_applied|=CORR_Decay_correction;
         else if (str == "frame-length correction")
               corrections_applied|=CORR_FrameLen_correction;
         else if (str == "fore") corrections_applied|=CORR_FORE;
         else if (str == "ssrb") corrections_applied|=CORR_SSRB;
         else if (str == "seg0") corrections_applied|=CORR_SEG0;
         else if (str == "randoms smoothing")
               corrections_applied|=CORR_Randoms_Smoothing;
         else if (str == "randoms subtraction")
               corrections_applied|=CORR_Randoms_Subtraction;
         else if (str == "untilted") corrections_applied|=CORR_untilted;
         else if (str == "xy-smoothing") corrections_applied|=CORR_XYSmoothing;
         else if (str == "z-smoothing") corrections_applied|=CORR_ZSmoothing;
      }
     Logging::flog()->logMsg("load data file #1", loglevel)->
      arg(inf->Sub_name_of_data_file());
                              // load dataset and transfer to memory controller
     switch (inf->Sub_number_format())
      { case KV::SIGNED_INT_NF:
         rio_si=new RawIO <signed short int>(inf->Sub_name_of_data_file(),
                              false,
                              inf->Sub_image_data_byte_order() == KV::BIG_END);
         break;
        case KV::FLOAT_NF:
         rio_f=new RawIO <float>(inf->Sub_name_of_data_file(), false,
                              inf->Sub_image_data_byte_order() == KV::BIG_END);
         break;
      }
     for (unsigned short int s=mnr; s < mnr+ds; s++)
      { if (ds > 2)
         Logging::flog()->logMsg("load sinogram #1", loglevel)->arg(s-mnr+1);
        resizeIndexVec(s-mnr, axes());
        switch (inf->Sub_number_format())
         { case KV::SIGNED_INT_NF:
            for (axis=0; axis < axes(); axis++)
             { rio_si->read(MemCtrl::mc()->createSInt(axis_size[axis],
                                                      &data[s-mnr][axis], name,
                                                      loglevel),
                            axis_size[axis]);
               MemCtrl::mc()->put(data[s-mnr][axis]);
             }
            break;
           case KV::FLOAT_NF:
            for (axis=0; axis < axes(); axis++)
             { rio_f->read(MemCtrl::mc()->createFloat(axis_size[axis],
                                                      &data[s-mnr][axis], name,
                                                      loglevel),
                           axis_size[axis]);
               MemCtrl::mc()->put(data[s-mnr][axis]);
             }
            break;
         }
      }
     if (rio_f != NULL) { delete rio_f;
                          rio_f=NULL;
                        }
     if (rio_si != NULL) { delete rio_si;
                           rio_si=NULL;
                         }
   }
   catch (...)
    { if (rio_f != NULL) delete rio_f;
      if (rio_si != NULL) delete rio_si;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a normalization dataset from an Interfile file.
    \param[in] singles    singles from emission scan
    \param[in] loglevel   level for logging

    Load a normalization dataset from an Interfile file. The components of the
    norm are expanded into a 3d sinogram.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadInterfileNorm(const std::vector <float> singles,
                                   const unsigned short int loglevel)
 { Interfile *infnorm=NULL;
   RawIO <float> *rio=NULL;
   NormECAT *norm_ecat=NULL;

   try
   { std::string norm_file, unit;
     unsigned short int axis;
     std::vector <float> paralyzing_ring_DT_param, crystal_eff, axial_pf,
                         non_paralyzing_ring_DT_param, plane_corr, intfcorr;
     Interfile::tdataset ds;
     bool has_axial_component=false;
     float *norm3d, *n3d;
     unsigned short int norm_idx;

     mnr=0;
     deleteData();
     acf_data=true;
     tof_bins=1;
     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     start_time=0.0f;
     frame_duration=0.0f;
     gate_duration=0.0f;
     vhalflife=0.0f;
     scantype=SCANTYPE::SINGLEFRAME;
     corrections_applied=0;
     prompts_and_randoms=false;
     scatter_fraction=0.0f;

     infnorm=new Interfile();
     infnorm->loadNormHeader(filename);
     GM::gm()->init(infnorm->Norm_originating_system());
                                                    // get geometry of sinogram
     vRhoSamples=GM::gm()->RhoSamples();
     vThetaSamples=GM::gm()->ThetaSamples();
     planesize=(unsigned long int)RhoSamples()*
               (unsigned long int)ThetaSamples();
     vspan=infnorm->Norm_axial_compression();
     vmrd=infnorm->Norm_maximum_ring_difference();
     axis_slices.resize((mrd()-(span()-1)/2)/span()+1);
     axes_slices=0;
     axis_size.resize(axes());
     for (axis=0; axis < axes(); axis++)
      { if (axis == 0) axis_slices[0]=GM::gm()->rings()*2-1;
         else axis_slices[axis]=2*(axis_slices[0]-
                                   2*(axis*span()-(span()-1)/2));
        axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
        axes_slices+=axis_slices[axis];
      }
     vmash=1;
     vBinSizeRho=infnorm->Norm_scale_factor(0, 0);
     plane_separation=infnorm->Norm_scale_factor(0, 1);
     lld=infnorm->Norm_energy_window_lower_level(0, &unit);
     uld=infnorm->Norm_energy_window_upper_level(0, &unit);
     intrinsic_tilt=GM::gm()->intrinsicTilt();
     bedpos=0.0f;
     shuffled_tof_data=false;
     ecf=infnorm->Norm_scanner_quantification_factor(&unit);
     try
     { unsigned short int c;

       Logging::flog()->loggingOn(false);
       for (c=0;; c++)
        if (infnorm->Norm_dead_time_quantification_factor(c) ==
            std::numeric_limits <float>::max())
         break;
       deadtime_factor.resize(c);
       for (unsigned short int i=0; i < c; i++)
        deadtime_factor[i]=infnorm->Norm_dead_time_quantification_factor(i);
     }
     catch (...)
      { deadtime_factor.clear();
      }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       calibration.date=infnorm->Norm_calibration_date(&unit);
       calibration.time=infnorm->Norm_calibration_time(&unit);
     }
     catch (...)
      { calibration.date.year=1970;
        calibration.date.month=1;
        calibration.date.day=1;
        calibration.time.hour=0;
        calibration.time.minute=0;
        calibration.time.second=0;
        calibration.time.gmt_offset_h=0;
        calibration.time.gmt_offset_m=0;
      }
     Logging::flog()->loggingOn(true);
     ds=infnorm->Norm_data_set(mnr);
     norm_file=*(ds.datafile);
     Logging::flog()->logMsg("load component norm data #1", loglevel+1)->
      arg(norm_file);
     rio=new RawIO <float>(norm_file, false, false);
                                                           // geometric effects
     plane_corr.resize(RhoSamples()*axis_slices[0]);
     rio->read((float *)&plane_corr[0], RhoSamples()*axis_slices[0]);
                                                        // crystal interference
     intfcorr.resize((GM::gm()->transaxialCrystalsPerBlock()+
                      GM::gm()->transaxialBlockGaps())*RhoSamples());
     rio->read((float *)&intfcorr[0], (GM::gm()->transaxialCrystalsPerBlock()+
                                       GM::gm()->transaxialBlockGaps())*
                                      RhoSamples());
                                                        // crystal efficiencies
     crystal_eff.resize(GM::gm()->crystalsPerRing()*GM::gm()->rings());
     rio->read((float *)&crystal_eff[0],
               GM::gm()->crystalsPerRing()*GM::gm()->rings());
                                                               // axial effects
     if (infnorm->Norm_number_of_normalization_components() == 7)
      { axial_pf.resize(axes_slices);
        rio->read((float *)&axial_pf[0], axes_slices);
        has_axial_component=true;
      }
                                               // paralyzing ring DT parameters
     paralyzing_ring_DT_param.resize(GM::gm()->rings());
     rio->read((float *)&paralyzing_ring_DT_param[0], GM::gm()->rings());
                                           // non-paralyzing ring DT parameters
     non_paralyzing_ring_DT_param.resize(GM::gm()->rings());
     rio->read((float *)&non_paralyzing_ring_DT_param[0], GM::gm()->rings());
     /*                                                              never used
                                                     // TX crystal DT parameter
     std::vector <float> TX_crystal_DT_param(
                                        GM::gm()->transaxialCrystalsPerBlock()+
                                        GM::gm()->transaxialBlockGaps());
     rio->read((float *)&TX_crystal_DT_param[0],
               GM::gm()->transaxialCrystalsPerBlock()+
               GM::gm()->transaxialBlockGaps());
     */
     delete rio;
     rio=NULL;
     delete infnorm;
     infnorm=NULL;
     Logging::flog()->logMsg("expand norm data", loglevel+1);

     norm_ecat=new NormECAT(RhoSamples(), ThetaSamples(), axisSlices(),
                            GM::gm()->transaxialCrystalsPerBlock()+
                            GM::gm()->transaxialBlockGaps(),
                            GM::gm()->transaxialBlocksPerBucket(),
                            GM::gm()->axialCrystalsPerBlock()+
                            GM::gm()->axialBlockGaps(),
                            GM::gm()->crystalsPerRing(),
                            (float *)&plane_corr[0], axis_slices[0],
                            (float *)&intfcorr[0],
                            (float *)&crystal_eff[0],
                            (float *)&paralyzing_ring_DT_param[0],
                            (float *)&non_paralyzing_ring_DT_param[0],
                            singles, 0, NULL);
     norm_idx=norm_ecat->calcNorm(loglevel+1);
     delete norm_ecat;
     norm_ecat=NULL;
     norm3d=MemCtrl::mc()->getFloat(norm_idx, loglevel+1);
     n3d=norm3d;
                                               // multiply with axial component
     if (has_axial_component)
      for (unsigned short int p=0; p < axes_slices; p++,
           n3d+=planesize)
       vecMulScalar(n3d, axial_pf[p], n3d, planesize);
                                       // transfer dataset to memory controller
     copyData(norm3d, 0, "norm", loglevel);
     MemCtrl::mc()->put(norm_idx);
     MemCtrl::mc()->deleteBlock(&norm_idx);
   }
   catch (...)
    { if (infnorm != NULL) delete infnorm;
      if (rio != NULL) delete rio;
      if (norm_ecat != NULL) delete norm_ecat;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a P39 patient normalization dataset from an Interfile file.
    \param[in] loglevel   level for logging

    Load a P39 patient normalization dataset from an Interfile file. The
    normalization has to be an expanded "patient-norm", i.e. adjusted to the
    singles rate of the emission scan.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadInterfileNormP39pat(const unsigned short int loglevel)
 { Interfile *infnorm=NULL;
   RawIO <float> *rio=NULL;

   try
   { Interfile::tdataset ds;
     std::string norm_file, unit;
     unsigned short int axis;

     infnorm=new Interfile();
     infnorm->loadNormHeader(filename);
     GM::gm()->init(infnorm->Norm_originating_system());
                                                    // get geometry of sinogram
     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     vRhoSamples=infnorm->Norm_matrix_size(0, 0);
     vThetaSamples=1;
     planesize=(unsigned long int)RhoSamples()*
               (unsigned long int)ThetaSamples();
     vspan=infnorm->Norm_axial_compression();
     vmrd=infnorm->Norm_maximum_ring_difference();
     axis_slices.resize((mrd()-(span()-1)/2)/span()+1);
     axes_slices=0;
     axis_size.resize(axes());
     for (axis=0; axis < axes(); axis++)
      { if (axis == 0) axis_slices[0]=GM::gm()->rings()*2-1;
         else axis_slices[axis]=2*(axis_slices[0]-
                                   2*(axis*span()-(span()-1)/2));
        axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
        axes_slices+=axis_slices[axis];
      }
     start_time=0.0f;
     frame_duration=0.0f;
     gate_duration=0.0f;
     vhalflife=0.0f;
     scantype=SCANTYPE::SINGLEFRAME;
     corrections_applied=0;
     vmash=1;
     vBinSizeRho=infnorm->Norm_scale_factor(0, 0);
     plane_separation=infnorm->Norm_scale_factor(0, 1);
     lld=infnorm->Norm_energy_window_lower_level(0, &unit);
     uld=infnorm->Norm_energy_window_upper_level(0, &unit);
     intrinsic_tilt=GM::gm()->intrinsicTilt();
     bedpos=0.0f;
     shuffled_tof_data=false;
     prompts_and_randoms=false;
     ecf=infnorm->Norm_scanner_quantification_factor(&unit);
     scatter_fraction=0.0f;
     { unsigned short int c;

       for (c=0;; c++)
        if (infnorm->Norm_dead_time_quantification_factor(c) ==
            std::numeric_limits <float>::max())
         break;
       deadtime_factor.resize(c);
       for (unsigned short int i=0; i < c; i++)
        deadtime_factor[i]=
         infnorm->Norm_dead_time_quantification_factor(i);
     }
     Logging::flog()->loggingOn(true);
     try
     { Logging::flog()->loggingOn(false);
       calibration.date=infnorm->Norm_calibration_date(&unit);
       calibration.time=infnorm->Norm_calibration_time(&unit);
     }
     catch (...)
      { calibration.date.year=1970;
        calibration.date.month=1;
        calibration.date.day=1;
        calibration.time.hour=0;
        calibration.time.minute=0;
        calibration.time.second=0;
        calibration.time.gmt_offset_h=0;
        calibration.time.gmt_offset_m=0;
      }
     Logging::flog()->loggingOn(true);
     ds=infnorm->Norm_data_set(mnr);
     norm_file=*(ds.datafile);
     Logging::flog()->logMsg("load norm data #1", loglevel)->arg(norm_file);
     rio=new RawIO <float>(norm_file, false, false);
     resizeIndexVec(0, axes());
     for (axis=0; axis < axes(); axis++)
      { rio->read(MemCtrl::mc()->createFloat(axis_size[axis], &data[0][axis],
                                             "norm", loglevel),
                  axis_size[axis]);
        MemCtrl::mc()->put(data[0][axis]);
      }
     delete rio;
     rio=NULL;
     delete infnorm;
     infnorm=NULL;
   }
   catch (...)
    { if (infnorm != NULL) delete infnorm;
      if (rio != NULL) delete rio;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a normalization sinogram from file.
    \param[in] _filename          name of file
    \param[in] _RhoSamples        number of bins in a projection
    \param[in] _ThetaSamples      number of projections in a sinogram plane
    \param[in] _axis_slices       array with number of planes per axis
    \param[in] _ecf               ECAT calibration factor
    \param[in] singles            singles from emission scan
    \param[in] p39_patient_norm   is dataset P39 patient norm ?
    \param[in] loglevel           level for logging

    Load a normalization sinogram from file. Supported formats are ECAT7,
    Interfile and flat (IEEE float or signed short integer). A previously
    stored normalization is lost.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadNorm(const std::string _filename,
                          const unsigned short int _RhoSamples,
                          const unsigned short int _ThetaSamples,
                          const std::vector <unsigned short int> _axis_slices,
                          const float _ecf, const std::vector <float> singles,
                          const bool p39_patient_norm,
                          const unsigned short int loglevel)
 { filename=_filename;
   mnr=0;
   deleteData();
   ecf=_ecf;
   Logging::flog()->logMsg("load norm '#1'", loglevel)->arg(filename);
   if (isECAT7file(filename))
    loadECAT7Norm(_RhoSamples, _ThetaSamples, _axis_slices, singles, loglevel);
    else if (isInterfile(filename))
          if (p39_patient_norm) loadInterfileNormP39pat(loglevel);
           else loadInterfileNorm(singles, loglevel);
          else { try
                 { Logging::flog()->loggingOn(false);
                   loadRAW(false, false, "norm", loglevel);
                   Logging::flog()->loggingOn(true);
                 }
                 catch (...)
                  { Logging::flog()->loggingOn(true);
                    loadComponentNorm(singles, loglevel);
                  }
               }
   Logging::flog()->logMsg("matrix number=#1", loglevel+1)->arg(mnr);
   acf_data=true;
   tof_bins=1;
   logGeometry(loglevel+1);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load a sinogram from a flat file.
    \param[in] _prompts_and_randoms   does sinogram contain prompts and
                                      randoms ?
    \param[in] arc_corrected          is sinogram arc corrected ?
    \param[in] name                   prefix for swap filenames
    \param[in] loglevel               level for logging

    Load a sinorgam from a flat file.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::loadRAW(const bool _prompts_and_randoms,
                         const bool arc_corrected,
                         const std::string name,
                         const unsigned short int loglevel)
 { RawIO <float> *riof=NULL;
   RawIO <signed short int> *rioi=NULL;

   try
   { unsigned short int ds;
     unsigned long int fsize, real_size;

     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     if (arc_corrected)
      { corrections_applied=CORR_Radial_Arc_correction;
        if (GM::gm()->needsAxialArcCorrection())
         corrections_applied|=CORR_Axial_Arc_correction;
      }
      else corrections_applied=0;
     scantype=SCANTYPE::SINGLEFRAME;
     prompts_and_randoms=_prompts_and_randoms;
                                                // calculate required file size
     if (prompts_and_randoms) ds=2;
      else ds=tof_bins;
     fsize=(unsigned long int)axes_slices*planesize*
           (unsigned long int)ds*sizeof(float);
                                                            // get size of file
     riof=new RawIO <float>(filename, false, false);
     real_size=riof->size();
     delete riof;
     riof=NULL;
                                 // load data and transfer to memory controller
     if ((real_size == fsize) || (real_size == 5180020))
      { riof=new RawIO <float>(filename, false, false);
        for (unsigned short int s=0; s < ds; s++)
         { resizeIndexVec(s, axes());
           for (unsigned short int axis=0; axis < axes(); axis++)
            { riof->read(MemCtrl::mc()->createFloat(axis_size[axis],
                                                    &data[s][axis], name,
                                                    loglevel),
                         axis_size[axis]);
              MemCtrl::mc()->put(data[s][axis]);
            }
         }
        delete riof;
        riof=NULL;
      }
      else if (real_size == fsize/2)
            { rioi=new RawIO <signed short int>(filename, false, false);
              for (unsigned short int s=0; s < ds; s++)
               { resizeIndexVec(s, axes());
                 for (unsigned short int axis=0; axis < axes(); axis++)
                  { rioi->read(MemCtrl::mc()->createSInt(axis_size[axis],
                                                         &data[s][axis], name,
                                                         loglevel),
                               axis_size[axis]);
                    MemCtrl::mc()->put(data[s][axis]);
                  }
               }
              delete rioi;
              rioi=NULL;
              return;
            }
            else throw Exception(REC_FILESIZE_WRONG,
                                 "The size of the file '#1' is wrong.").
                                 arg(filename);
   }
   catch (...)
    { if (riof != NULL) delete riof;
      if (rioi != NULL) delete rioi;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write information about sinogram to logging mechanism.
    \param[in] loglevel   level for logging

    Write information about sinogram to logging mechanism.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::logGeometry(const unsigned short int loglevel) const
 { std::string str;
   Logging *flog;

   flog=Logging::flog();
   flog->logMsg("RhoSamples=#1", loglevel)->arg(RhoSamples());
   flog->logMsg("BinSizeRho=#1mm", loglevel)->arg(BinSizeRho());
   flog->logMsg("ThetaSamples=#1", loglevel)->arg(ThetaSamples());
   flog->logMsg("mash factor=#1", loglevel)->arg(mash());
   flog->logMsg("plane separation=#1mm", loglevel)->arg(plane_separation);
   flog->logMsg("axes=#1", loglevel)->arg(axes());
   str=toString(axis_slices[0]);
   for (unsigned short int i=0; i < axes()-1; i++)
    str+=","+toString(axis_slices[i+1]);
   flog->logMsg("axis table=#1 {#2}", loglevel)->arg(axes_slices)->arg(str);
   if (span() != 65535)
    flog->logMsg("span=#1", loglevel)->arg(span());
   if (mrd() != 65535)
    flog->logMsg("maximum ring difference=#1", loglevel)->arg(mrd());
   flog->logMsg("intrinsic tilt=#1 degrees", loglevel)->arg(intrinsic_tilt);
   flog->logMsg("bed position=#1mm", loglevel)->arg(bedPos());
   flog->logMsg("scan start time=#1 sec", loglevel)->arg(startTime());
   flog->logMsg("scan frame duration=#1 sec (#2 min)", loglevel)->
    arg(frameDuration())->arg(frameDuration()/60.0f);
   flog->logMsg("scan gate duration=#1 sec (#2 min)", loglevel)->
    arg(gateDuration())->arg(gateDuration()/60.0f);
   flog->logMsg("isotope halflife=#1 sec", loglevel)->arg(halflife());
   flog->logMsg("lld=#1 KeV", loglevel)->arg(lld);
   flog->logMsg("uld=#1 KeV", loglevel)->arg(uld);
   str=std::string();
   if (corrections_applied & CORR_Normalized) str+="normalized, ";
   if (corrections_applied & CORR_Measured_Attenuation_Correction)
    str+="measured attenuation correction, ";
   if (corrections_applied & CORR_Calculated_Attenuation_Correction)
    str+="calculated attenuation correction, ";
   if (corrections_applied & CORR_2D_scatter_correction)
    str+="2d scatter correction, ";
   if (corrections_applied & CORR_3D_scatter_correction)
    str+="3d scatter correction, ";
   if (corrections_applied & CORR_Radial_Arc_correction)
    str+="radial arc-correction, ";
   if (corrections_applied & CORR_Axial_Arc_correction)
    str+="axial arc-correction, ";
   if (corrections_applied & CORR_Decay_correction) str+="decay correction, ";
   if (corrections_applied & CORR_FORE) str+="FORE, ";
   if (corrections_applied & CORR_SSRB) str+="SSRB, ";
   if (corrections_applied & CORR_SEG0) str+="SEG0, ";
   if (corrections_applied & CORR_Randoms_Smoothing)
    str+="randoms smoothing, ";
   if (corrections_applied & CORR_Randoms_Subtraction)
    str+="randoms subtraction, ";
   if (corrections_applied & CORR_untilted) str+="untilted, ";
   if (corrections_applied & CORR_XYSmoothing) str+="xy-smoothing, ";
   if (corrections_applied & CORR_ZSmoothing) str+="z-smoothing, ";
   if (str.empty()) flog->logMsg("corrections applied=", loglevel);
    else flog->logMsg("corrections applied=#1", loglevel)->
          arg(str.substr(0, str.length()-2));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request mash factor of sinogram.
    \return mash factor of sinogram

    Request mash factor of sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::mash() const
 { return(vmash);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bed.
    \return number of bed

    Request number of bed.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::matrixBed() const
 { return(matrix_bed);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of dataset.
    \return number of dataset

    Request number of dataset.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::matrixData() const
 { return(matrix_data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of frame.
    \return number of frame

    Request number of frame.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::matrixFrame() const
 { return(matrix_frame);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of gate.
    \return number of gate

    Request number of gate.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::matrixGate() const
 { return(matrix_gate);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of plane.
    \return number of plane

    Request number of plane.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::matrixPlane() const
 { return(matrix_plane);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request maximum ring difference.
    \return maximum ring difference

    Request maximum ring difference.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::mrd() const
 { return(vmrd);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of sinogram datasets.
    \return number of sinogram datasets

    Request the number of sinogram datasets.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::numberOfSinograms() const
 { for (unsigned short int s=0; s < MAX_SINO; s++)
    if (data[s].size() == 0) return(s);
   return(MAX_SINO);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the plane separation.
    \return plane separation in mm

    Request the plane separation in mm.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::planeSeparation() const
 { return(plane_separation);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the size of a sinogram plane.
    \return size of a sinogram plane

    Request the size of a sinogram plane.
 */
/*---------------------------------------------------------------------------*/
unsigned long int SinogramIO::planeSize() const
 { return(planesize);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy an axes table into the object.
    \param[in] _axis_slices   axes table

    Copy an axes table into the object.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::putAxesTable(
                           const std::vector <unsigned short int> _axis_slices)
 {                                                           // copy axes table
   axis_slices=_axis_slices;
                    // calculate sizes of axes and number of planes in sinogram
   axis_size.resize(axes());
   axes_slices=0;
   for (unsigned short int axis=0; axis < axes(); axis++)
    { axis_size[axis]=(unsigned long int)axis_slices[axis]*planesize;
      axes_slices+=axis_slices[axis];
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Resize the index vector.
    \param[in] s          sinogram number
    \param[in] new_size   new size of vector

    Resize the index vector. New elements are initialized.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::resizeIndexVec(const unsigned short int s,
                                const unsigned short int new_size)
 { if (data[s].size() >= new_size) data[s].resize(new_size);
    else { unsigned short int oldsize;

           oldsize=data[s].size();
           data[s].resize(new_size);
           for (unsigned short int i=oldsize; i < new_size; i++)
            data[s][i]=MemCtrl::MAX_BLOCK;
         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bins in a projection.
    \return number of bins in a projection

    Request number of bins in a projection.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::RhoSamples() const
 { return(vRhoSamples);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save sinogram in file.
    \param[in] filename        name of file
    \param[in] orig_filename   name of original file
    \param[in] frame           number of frame
    \param[in] plane           number of plane
    \param[in] gate            number of gate
    \param[in] data_num        number of dataset in file
    \param[in] bed             number of bed
    \param[in] view_mode       store sinogram in view mode ?
    \param[in] bedpos          horizontal bed position of sinogram
    \param[in] mnr             matrix number
    \param[in] loglevel        level for logging

    Save sinogram in file. Supported formats are ECAT7, Interfile and flat
    (IEEE float or signed short integer). If the file already exists, the new
    data is appended.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::save(const std::string filename,
                      const std::string orig_filename,
                      const unsigned short int frame,
                      const unsigned short int plane,
                      const unsigned short int gate,
                      const unsigned short int data_num,
                      const unsigned short int bed, const bool view_mode,
                      const float bedpos, const unsigned short int mnr,
                      const unsigned short int loglevel)
 { if ((mnr == 0) && (e7 == NULL) && (inf == NULL))      // save as flat file ?
    { saveFlat(filename, false, loglevel);
      return;
    }
   if (isInterfile(orig_filename))                       // save as interfile ?
    { saveInterfile(filename, frame, gate, data_num, bed, view_mode, bedpos,
                    mnr, loglevel);
      return;
    }
   float *fptr=NULL;

   try
   { unsigned short int axis;

     Logging::flog()->logMsg("write sinogram file #1 (matrix #2)", loglevel)->
      arg(filename)->arg(mnr);
     if (mnr == 0)                                        // create main header
      { if (acf_data) e7->Main_file_type(E7_FILE_TYPE_AttenuationCorrection);
         else e7->Main_file_type(E7_FILE_TYPE_3D_SinogramFloat);
        if (corrections_applied & CORR_untilted) e7->Main_intrinsic_tilt(0);
         else e7->Main_intrinsic_tilt(intrinsic_tilt);
        e7->Main_isotope_halflife(halflife());
        e7->Main_lwr_true_thres(LLD());
        e7->Main_upr_true_thres(ULD());
        { unsigned short int m=0;

          while ((1 << m) != mash()) m++;
          e7->Main_angular_compression(m);
        }
        e7->Main_bin_size(BinSizeRho()/10.0f);
        e7->Main_init_bed_position(bedpos/10.0f);
        if (e7->Main_num_bed_pos() > 0)
         { for (unsigned short int i=0; i < 15; i++)
            e7->Main_bed_position(0.0f, i);
         }
        e7->Main_num_bed_pos(0);
        e7->Main_num_frames(1);
        e7->Main_num_gates(1);
        e7->Main_num_planes(axes_slices);
        if (corrections_applied & CORR_Normalized)
         { e7->Main_ecat_calibration_factor(ecf);
           e7->Main_calibration_units(E7_CALIBRATION_UNITS_Calibrated);
           e7->Main_calibration_units_label(E7_CALIBRATION_UNITS_LABEL_LMRGLU);
           e7->Main_data_units((unsigned char *)"Bq/ml");
         }
         else e7->Main_data_units((unsigned char *)"ECAT counts");
        e7->SaveFile(filename);
      }
      else { if (e7 != NULL) delete e7;
                                                   // load existing main header
             e7=new ECAT7();
             e7->LoadFile(filename);
                                                          // update main header
             switch (scantype)
              { case SCANTYPE::MULTIBED:
                 e7->Main_num_bed_pos(e7->Main_num_bed_pos()+1);
                 break;
                case SCANTYPE::MULTIFRAME:
                 e7->Main_num_frames(e7->Main_num_frames()+1);
                 break;
                case SCANTYPE::MULTIGATE:
                 e7->Main_num_gates(e7->Main_num_gates()+1);
                 break;
                case SCANTYPE::SINGLEFRAME:
                 break;
              }
             e7->Main_bed_position(
                              (bedpos-e7->Main_init_bed_position()*10.0f)/10.f,
                              mnr-1);
             e7->UpdateMainHeader(filename);
           }
                                                           // create new matrix
     if (e7->NumberOfMatrices() <= mnr)
      if (acf_data) e7->CreateAttnMatrices(1);
       else e7->CreateScan3DMatrices(1);
                                                      // create directory entry
     e7->Dir_frame(frame, mnr);
     e7->Dir_plane(plane, mnr);
     e7->Dir_gate(gate, mnr);
     e7->Dir_data(data_num, mnr);
     e7->Dir_bed(bed, mnr);
                                                            // create subheader
     switch (e7->Main_file_type())
      { case E7_FILE_TYPE_AttenuationCorrection:
         e7->Attn_num_r_elements(RhoSamples(), mnr);
         e7->Attn_num_angles(ThetaSamples(), mnr);
         e7->Attn_x_resolution(BinSizeRho()/10.0f, mnr);
         switch (MemCtrl::mc()->dataType(data[0][0]))
          { case MemCtrl::DT_FLT:
             e7->Attn_data_type(E7_DATA_TYPE_IeeeFloat, mnr);
             break;
            case MemCtrl::DT_UINT:
             e7->Attn_data_type(E7_DATA_TYPE_SunLong, mnr);
             break;
            case MemCtrl::DT_SINT:
             e7->Attn_data_type(E7_DATA_TYPE_SunShort, mnr);
             break;
            case MemCtrl::DT_CHAR:
             e7->Attn_data_type(E7_DATA_TYPE_ByteData, mnr);
            break;
          }
         e7->Attn_num_dimensions(3, mnr);
         e7->Attn_attenuation_type(E7_ATTENUATION_TYPE_AttenCalc, mnr);
         e7->Attn_num_z_elements(axis_slices[0], mnr);
         for (axis=0; axis < axes(); axis++)
          e7->Attn_z_elements(axis_slices[axis], axis, mnr);
         for (axis=axes(); axis < 64; axis++)
          e7->Attn_z_elements(0, axis, mnr);
         e7->Attn_ring_difference(mrd(), mnr);
         e7->Attn_span(span(), mnr);
         e7->Attn_scale_factor(1.0f, mnr);
         break;
        case E7_FILE_TYPE_3D_Sinogram16:
        case E7_FILE_TYPE_3D_SinogramFloat:
         switch (MemCtrl::mc()->dataType(data[0][0]))
          { case MemCtrl::DT_FLT:
             e7->Scan3D_data_type(E7_DATA_TYPE_IeeeFloat, mnr);
             break;
            case MemCtrl::DT_UINT:
             e7->Scan3D_data_type(E7_DATA_TYPE_SunLong, mnr);
             break;
            case MemCtrl::DT_SINT:
             e7->Scan3D_data_type(E7_DATA_TYPE_SunShort, mnr);
             break;
            case MemCtrl::DT_CHAR:
             e7->Scan3D_data_type(E7_DATA_TYPE_ByteData, mnr);
             break;
          }
         e7->Scan3D_num_dimensions(3, mnr);
         e7->Scan3D_num_r_elements(RhoSamples(), mnr);
         e7->Scan3D_num_angles(ThetaSamples(), mnr);
         { signed short int c=0;

           if (corrections_applied & CORR_Normalized)
            c|=E7_APPLIED_PROC_Normalized;
           if (corrections_applied & CORR_Measured_Attenuation_Correction)
            c|=E7_APPLIED_PROC_Measured_Attenuation_Correction;
           if (corrections_applied & CORR_Calculated_Attenuation_Correction)
            c|=E7_APPLIED_PROC_Calculated_Attenuation_Correction;
           if (corrections_applied & CORR_2D_scatter_correction)
            c|=E7_APPLIED_PROC_2D_scatter_correction;
           if (corrections_applied & CORR_3D_scatter_correction)
            c|=E7_APPLIED_PROC_3D_scatter_correction;
           if ((corrections_applied & CORR_Radial_Arc_correction) ||
               (corrections_applied & CORR_Axial_Arc_correction))
            c|=E7_APPLIED_PROC_Arc_correction;
           if (corrections_applied & CORR_Decay_correction)
            c|=E7_APPLIED_PROC_Decay_correction;
           if (corrections_applied & CORR_FORE) c|=E7_APPLIED_PROC_FORE;
           if (corrections_applied & CORR_SSRB) c|=E7_APPLIED_PROC_SSRB;
           if (corrections_applied & CORR_SEG0) c|=E7_APPLIED_PROC_SEG0;
           if ((corrections_applied & CORR_Randoms_Smoothing) &&
               (corrections_applied & CORR_Randoms_Subtraction))
            c|=E7_APPLIED_PROC_Randoms_Smoothing;
           if (corrections_applied & CORR_XYSmoothing)
            c|=E7_APPLIED_PROC_X_smoothing | E7_APPLIED_PROC_Y_smoothing;
           if (corrections_applied & CORR_ZSmoothing)
            c|=E7_APPLIED_PROC_Z_smoothing;
           e7->Scan3D_corrections_applied(c, mnr);
         }
         for (axis=0; axis < axes(); axis++)
          e7->Scan3D_num_z_elements(axis_slices[axis], axis, mnr);
         for (axis=axes(); axis < 64; axis++)
          e7->Scan3D_num_z_elements(0, axis, mnr);
         e7->Scan3D_ring_difference(mrd(), mnr);
         e7->Scan3D_axial_compression(span(), mnr);
         e7->Scan3D_x_resolution(BinSizeRho()/10.0f, mnr);
         e7->Scan3D_gate_duration((signed long int)(gateDuration()*1000.0f),
                                  mnr);
         e7->Scan3D_scale_factor(1.0f, mnr);
         e7->Scan3D_scan_min(-1, mnr);
         e7->Scan3D_scan_max(-1, mnr);
         e7->Scan3D_frame_start_time((signed long int)(startTime()*1000.0f),
                                     mnr);
         e7->Scan3D_frame_duration((signed long int)(frameDuration()*1000.0f),
                                   mnr);
         break;
      }
     if (data[0].size() > 0)                   // transfer data in ECAT7 object
      { float *fp;

        fptr=new float[size()];
        fp=fptr;
        for (axis=0; axis < axes(); axis++)
         { memcpy(fp, MemCtrl::mc()->getFloatRO(data[0][axis], loglevel),
                  axis_size[axis]*sizeof(float));
           MemCtrl::mc()->put(data[0][axis]);
           fp+=axis_size[axis];
         }
        e7->MatrixData(fptr, E7_DATATYPE_FLOAT, mnr);
        fptr=NULL;
      }
                                                        // store in view mode ?
     switch (e7->Main_file_type())
      { case E7_FILE_TYPE_AttenuationCorrection:
         if (view_mode)
          { ((ECAT7_ATTENUATION *)e7->Matrix(mnr))->Volume2View();
            e7->Attn_storage_order(E7_STORAGE_ORDER_RZTD, mnr);
          }
          else e7->Attn_storage_order(E7_STORAGE_ORDER_RTZD, mnr);
         break;
        case E7_FILE_TYPE_3D_Sinogram16:
        case E7_FILE_TYPE_3D_SinogramFloat:
         if (view_mode)
          { ((ECAT7_SCAN3D *)e7->Matrix(mnr))->Volume2View();
            e7->Scan3D_storage_order(E7_STORAGE_ORDER_RZTD, mnr);
          }
          else e7->Scan3D_storage_order(E7_STORAGE_ORDER_RTZD, mnr);
         break;
      }
     e7->AppendMatrix(mnr);                            // append matrix to file
     e7->DeleteData(mnr);
   }
   catch (...)
    { if (fptr != NULL) delete[] fptr;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save sinogram in flat file.
    \param[in] filename   name of file
    \param[in] add_tof    add TOF bins ?
    \param[in] loglevel   level for logging

    Save sinogram in flat file. An ascii header file with the extension ".h33"
    is generated, that contains information about the geometry of the sinogram.
    The data is stored in signed char, signed short int or float format.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::saveFlat(std::string filename, const bool add_tof,
                          const unsigned short int loglevel)
 { RawIO <float> *rio_f=NULL;
   RawIO <signed short int> *rio_si=NULL;
   RawIO <char> *rio_c=NULL;
   RawIO <uint32> *rio_ui=NULL;
   std::ofstream *file=NULL;

   try
   { MemCtrl::tdatatype dt;
     std::string datafilename, hdrfilename;
     unsigned short int s, axis;

     hdrfilename=stripExtension(filename);
     if (isACF()) datafilename=hdrfilename+FLAT_SDATA2_EXTENSION;
      else datafilename=hdrfilename+FLAT_SDATA1_EXTENSION;
     hdrfilename+=FLAT_SHDR_EXTENSION;
                                                             // write data file
     Logging::flog()->logMsg("write sinogram data file #1 (matrix #2)",
                             loglevel)->arg(datafilename)->arg(mnr);
     if (data[1].size() > 0) dt=MemCtrl::mc()->dataType(data[1][0]);
      else dt=MemCtrl::mc()->dataType(data[0][0]);
     if (add_tof) rio_f=new RawIO <float>(datafilename, true, false);
      else switch (dt)
            { case MemCtrl::DT_FLT:
               rio_f=new RawIO <float>(datafilename, true, false);
               break;
              case MemCtrl::DT_UINT:
               rio_ui=new RawIO <uint32>(datafilename, true, false);
               break;
              case MemCtrl::DT_SINT:
               rio_si=new RawIO <signed short int>(datafilename, true, false);
               break;
              case MemCtrl::DT_CHAR:
               rio_c=new RawIO <char>(datafilename, true, false);
               break;
            }
     if (add_tof)
      for (axis=0; axis < data[0].size(); axis++)
       { float *sumptr;
         unsigned short int idx;

         sumptr=MemCtrl::mc()->createFloat(axis_size[axis], &idx, "emis",
                                           loglevel);
         memset(sumptr, 0, axis_size[axis]*sizeof(float));
         for (s=0; s < MAX_SINO; s++)
          if (data[s].size() > axis)
           { float *ptr;

             ptr=MemCtrl::mc()->getFloatRO(data[s][axis], loglevel);
             vecAdd(sumptr, ptr, sumptr, axis_size[axis]);
             MemCtrl::mc()->put(data[s][axis]);
           }
         rio_f->write(sumptr, axis_size[axis]);
         MemCtrl::mc()->put(idx);
         MemCtrl::mc()->deleteBlock(&idx);
       }
      else for (s=0; s < MAX_SINO; s++)
            for (axis=0; axis < data[s].size(); axis++)
             { switch (dt)
                { case MemCtrl::DT_FLT:
                   rio_f->write((float *)MemCtrl::mc()->getFloatRO(
                                                      data[s][axis], loglevel),
                                axis_size[axis]);
                   break;
                  case MemCtrl::DT_SINT:
                   rio_si->write((signed short int *)
                             MemCtrl::mc()->getSIntRO(data[s][axis], loglevel),
                             axis_size[axis]);
                   break;
                  case MemCtrl::DT_UINT:
                   rio_ui->write((uint32 *)
                             MemCtrl::mc()->getUIntRO(data[s][axis], loglevel),
                             axis_size[axis]);
                   break;
                  case MemCtrl::DT_CHAR:
                   rio_c->write((char *)MemCtrl::mc()->getCharRO(data[s][axis],
                                                                 loglevel),
                                axis_size[axis]);
                   break;
                }
               MemCtrl::mc()->put(data[s][axis]);
             }
     if (rio_f != NULL) { delete rio_f;
                          rio_f=NULL;
                        }
     if (rio_ui != NULL) { delete rio_ui;
                           rio_ui=NULL;
                         }
     if (rio_si != NULL) { delete rio_si;
                           rio_si=NULL;
                         }
     if (rio_c != NULL) { delete rio_c;
                          rio_c=NULL;
                        }
                                                           // write header file
     Logging::flog()->logMsg("write sinogram header file #1 (matrix #2)",
                             loglevel)->arg(hdrfilename)->arg(mnr);
     file=new std::ofstream(hdrfilename.c_str(), std::ios::out);
     *file << "!INTERFILE\n"
              "name of data file := "
           << stripPath(datafilename) << "\n"
              "image data byte order := LITTLEENDIAN\n"
              "number of dimensions := 3\n"
              "matrix size [1] := " << RhoSamples() << "\n"
              "matrix size [2] := " << ThetaSamples() << "\n"
              "matrix size [3] := " << axis_slices[0] << "\n"
              "number of z elements := ";
     if (add_tof) for (axis=0; axis < data[0].size(); axis++)
                   *file << axis_slices[axis] << " ";
      else for (s=0; s < MAX_SINO; s++)
            for (axis=0; axis < data[s].size(); axis++)
             *file << axis_slices[axis] << " ";
     *file << "\nnumber format := ";
     switch (dt)
      { case MemCtrl::DT_FLT:
         *file << "float\nnumber of bytes per pixel := " << sizeof(float);
         break;
        case MemCtrl::DT_SINT:
         *file << "signed int\nnumber of bytes per pixel := "
               << sizeof(signed short int);
         break;
        case MemCtrl::DT_UINT:
         *file << "unsigned long int\nnumber of bytes per pixel := "
               << sizeof(uint32);
         break;
        case MemCtrl::DT_CHAR:
         *file << "char\nnumber of bytes per pixel := " << sizeof(char);
         break;
      }
     *file << "\ndata offset in bytes := 0"
           << "\nscaling factor (mm/pixel) [1] := " << BinSizeRho()
           << "\nscaling factor (mm/pixel) [2] := 1.0\n"
              "scaling factor (mm/pixel) [3] := " << planeSeparation()
           << std::endl;
     if (scatter_fraction>0) *file << "scatter fraction := " << scatter_fraction/100;
     delete file;
     file=NULL;
   }
   catch (...)
    { if (rio_f != NULL) delete rio_f;
      if (rio_ui != NULL) delete rio_ui;
      if (rio_si != NULL) delete rio_si;
      if (rio_c != NULL) delete rio_c;
      if (file != NULL) delete file;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save sinogram in Interfile file.
    \param[in] filename    name of file
    \param[in] frame       number of frame
    \param[in] gate        number of gate
    \param[in] data_num    number of dataset in file
    \param[in] bed         number of bed
    \param[in] view_mode   store sinogram in view mode ?
    \param[in] bedpos      horizontal bed position of sinogram
    \param[in] mnr         matrix number
    \param[in] loglevel    level for logging

    Save sinogram in Interfile file. The data is always store in float format.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::saveInterfile(const std::string filename,
                               const unsigned short int frame,
                               const unsigned short int gate,
                               const unsigned short int data_num,
                               const unsigned short int bed,
                               const bool view_mode, const float bedpos,
                               const unsigned short int mnr,
                               const unsigned short int loglevel)
 { RawIO <float> *rio=NULL;
   Interfile::tdataset ds;

   ds.headerfile=NULL;
   ds.datafile=NULL;
   try
   { unsigned short int axis, i;
     std::string filename_noext, subheader_filename, prev_subheader_filename,
                 data_extension, hdr_extension;

     Logging::flog()->logMsg("write sinogram file #1 (matrix #2)", loglevel)->
      arg(filename)->arg(mnr);
     if (isACF()) { data_extension=Interfile::INTERFILE_ADATA_EXTENSION;
                    hdr_extension=Interfile::INTERFILE_ASUBHDR_EXTENSION;
                  }
      else { data_extension=Interfile::INTERFILE_SDATA_EXTENSION;
             hdr_extension=Interfile::INTERFILE_SSUBHDR_EXTENSION;
           }
     filename_noext=stripExtension(filename);
     prev_subheader_filename=filename_noext+"_"+toStringZero(mnr-1, 2)+
                             hdr_extension;
     filename_noext+="_"+toStringZero(mnr, 2);
     subheader_filename=filename_noext+hdr_extension;
     if (mnr == 0)                                        // create main header
      { inf->Main_comment("created with code from "+std::string(__DATE__)+" "+
                          std::string(__TIME__));
        if (corrections_applied & CORR_untilted)
         inf->Main_gantry_tilt_angle(0, "degrees");
         else inf->Main_gantry_tilt_angle((unsigned long int)intrinsicTilt(),
                                          "degrees");
        inf->Main_isotope_gamma_halflife(halflife(), "sec");
        if (inf->Main_number_of_energy_windows() > 0)
         { inf->Main_energy_window_lower_level(LLD(), 0, "keV");
           inf->Main_energy_window_upper_level(ULD(), 0, "keV");
         }
        inf->Main_CPS_data_type("sinogram main header");
        inf->Main_number_of_time_frames(1);
        inf->Main_number_of_horizontal_bed_offsets(1);
        if (isACF())
         { inf->Main_number_of_emission_data_types(0);
           inf->Main_emission_data_type_description_remove();
           inf->Main_number_of_transmission_data_types(1);
           inf->Main_transmission_data_type_description_remove();
           inf->MainKeyAfterKey(inf->Main_transmission_data_type_description(
                                                     KV::CORRECTED_PROMPTS, 0),
                                inf->Main_number_of_transmission_data_types());
         }
         else { inf->Main_number_of_transmission_data_types(0);
                inf->Main_transmission_data_type_description_remove();
                inf->Main_number_of_emission_data_types(1);
                inf->Main_emission_data_type_description_remove();
                inf->MainKeyAfterKey(inf->Main_emission_data_type_description(
                                                     KV::CORRECTED_PROMPTS, 0),
                                    inf->Main_number_of_emission_data_types());
              }
        if (corrections_applied & CORR_Normalized)
         { time_t utc;

           inf->time2UTC(calibration.date, calibration.time, &utc);
           for (i=0; i < deadtime_factor.size(); i++)
            inf->MainKeyAfterKey(inf->Main_dead_time_quantification_factor(
                                                        deadtime_factor[i], i),
                                 inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_calibration_UTC(
                                                (unsigned long int)utc, "sec"),
                                inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_calibration_time(calibration.time,
                                                           "hh:mm:ss"),
                                inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_calibration_date(calibration.date,
                                                           "yyyy:mm:dd"),
                                inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_scanner_quantification_factor(ecf,
                                                                     "mCi/ml"),
                                inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_GLOBAL_SCANNER_CALIBRATION_FACTOR(),
                                inf->Main_number_of_TOF_time_bins());
           inf->MainKeyAfterKey(inf->Main_blank_line(std::string()),
                                inf->Main_number_of_TOF_time_bins());
         }
        inf->Main_data_set_remove();
        inf->Main_total_number_of_data_sets(1);
        ds.headerfile=new std::string(stripPath(filename_noext)+hdr_extension);
        ds.datafile=new std::string(stripPath(filename_noext)+data_extension);
        if (isACF())
         ds.acquisition_number=inf->acquisitionCode(
                                                  Interfile::TRANSMISSION_TYPE,
                                                  frame, gate, bed, data_num,
                                                  Interfile::TRUES_MODE, 0);
         else ds.acquisition_number=inf->acquisitionCode(
                                                     Interfile::EMISSION_TYPE,
                                                     frame, gate, bed,
                                                     data_num,
                                                     Interfile::TRUES_MODE, 0);
        inf->Main_data_set(ds, 0);
        ds.headerfile=NULL;
        ds.datafile=NULL;
        Logging::flog()->logMsg("write main header file #1", loglevel+1)->
         arg(filename);
        inf->saveMainHeader(filename);
        inf->SubKeyBeforeKey(inf->Sub_angular_compression(0),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_quantification_units(" "),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_decay_correction(KV::NODEC),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_decay_correction_factor(0.0f),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_scatter_fraction_factor(0.0f),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_dead_time_factor(0.0f, 0),
                             inf->Sub_axial_compression());
      }
      else { std::string unit;

             if (inf != NULL) delete inf;
                                                   // load existing main header
             inf=new Interfile();
             inf->loadMainHeader(filename);
             inf->loadSubheader(prev_subheader_filename);
             if (inf->Sub_start_horizontal_bed_position(&unit) != bedpos)
              scantype=SCANTYPE::MULTIBED;
                                                          // update main header
             inf->Main_total_number_of_data_sets(
                                      inf->Main_total_number_of_data_sets()+1);
             ds.headerfile=new std::string(stripPath(filename_noext)+
                                           hdr_extension);
             ds.datafile=new std::string(stripPath(filename_noext)+
                                         data_extension);
             if (isACF())
              ds.acquisition_number=inf->acquisitionCode(
                                                  Interfile::TRANSMISSION_TYPE,
                                                  frame, gate, bed, data_num,
                                                  Interfile::TRUES_MODE, 0);
              else ds.acquisition_number=inf->acquisitionCode(
                                                     Interfile::EMISSION_TYPE,
                                                     frame, gate, bed,
                                                     data_num,
                                                     Interfile::TRUES_MODE, 0);
             inf->Main_data_set(ds, inf->Main_total_number_of_data_sets()-1);
             ds.headerfile=NULL;
             ds.datafile=NULL;
             if (scantype == SCANTYPE::MULTIBED)
              inf->Main_number_of_horizontal_bed_offsets(
                               inf->Main_number_of_horizontal_bed_offsets()+1);
              else inf->Main_number_of_time_frames(
                                          inf->Main_number_of_time_frames()+1);
             Logging::flog()->logMsg("write main header file #1", loglevel+1)->
             arg(filename);
             inf->saveMainHeader(filename);
           }
                                                            // create subheader
     inf->Sub_comment("created with code from "+std::string(__DATE__)+" "+
                      std::string(__TIME__));
     inf->Sub_start_horizontal_bed_position(bedpos, "mm");
     inf->Sub_CPS_data_type("sinogram subheader");
     inf->Sub_name_of_data_file(stripPath(filename_noext)+data_extension);
     inf->Sub_image_data_byte_order(KV::LITTLE_END);
     inf->Sub_number_format(KV::FLOAT_NF);
     inf->Sub_number_of_bytes_per_pixel(sizeof(float));
     if (view_mode)
      { inf->Sub_matrix_axis_label("number of sinograms", 1);
        inf->Sub_matrix_axis_label("sinogram views", 2);
      }
      else { inf->Sub_matrix_axis_label("sinogram views", 1);
             inf->Sub_matrix_axis_label("number of sinograms", 2);
           }
     inf->Sub_matrix_size(RhoSamples(), 0);
     inf->Sub_matrix_size(ThetaSamples(), 1);
     inf->Sub_matrix_size(axes_slices, 2);
     inf->Sub_scale_factor(BinSizeRho(), 0, "mm/pixel");
     inf->Sub_scale_factor(180.0f/(float)ThetaSamples(), 1, "degree/pixel");
     inf->Sub_scale_factor(GM::gm()->planeSeparation(), 2, "mm/pixel");
     inf->Sub_start_horizontal_bed_position(bedpos, "mm");
     inf->Sub_number_of_scan_data_types(tof_bins);
     inf->Sub_scan_data_type_description_remove();
     for (i=0; i < tof_bins; i++)
      { inf->SubKeyAfterKey(inf->Sub_scan_data_type_description(
                                                     KV::CORRECTED_PROMPTS, i),
                            inf->Sub_number_of_scan_data_types());
        inf->Sub_data_offset_in_bytes_remove();
        inf->SubKeyAfterKey(inf->Sub_data_offset_in_bytes(
                                 size()*sizeof(float)*(unsigned long int)i, i),
                            inf->Sub_scan_data_type_description(0));
      }
     inf->Sub_axial_compression(span());
     inf->Sub_maximum_ring_difference(mrd());
     inf->Sub_number_of_segments(axes()*2-1);
     inf->Sub_segment_table_remove();
     inf->SubKeyAfterKey(inf->Sub_segment_table(axis_slices[0], 0),
                         inf->Sub_number_of_segments());
     for (axis=1; axis < axes(); axis++)
      { inf->Sub_segment_table(axis_slices[axis]/2, axis*2-1);
        inf->Sub_segment_table(axis_slices[axis]/2, axis*2);
      }
     { unsigned short int c=0;                        // write correction flags

       inf->Sub_applied_corrections_remove();
       if (corrections_applied & CORR_Normalized)
        inf->Sub_applied_corrections("normalized", c++);
       if (corrections_applied & CORR_Measured_Attenuation_Correction)
        inf->Sub_applied_corrections("measured attenuation correction", c++);
       if (corrections_applied & CORR_Calculated_Attenuation_Correction)
        inf->Sub_applied_corrections("calculated attenuation correction", c++);
       if (corrections_applied & CORR_2D_scatter_correction)
        inf->Sub_applied_corrections("2d scatter correction", c++);
       if (corrections_applied & CORR_3D_scatter_correction)
        inf->Sub_applied_corrections("3d scatter correction", c++);
       if (corrections_applied & CORR_Radial_Arc_correction)
        inf->Sub_applied_corrections("radial arc-correction", c++);
       if (corrections_applied & CORR_Axial_Arc_correction)
        inf->Sub_applied_corrections("axial arc-correction", c++);
       if (corrections_applied & CORR_Decay_correction)
        inf->Sub_applied_corrections("decay correction", c++);
       if (corrections_applied & CORR_FrameLen_correction)
        inf->Sub_applied_corrections("frame-length correction", c++);
       if (corrections_applied & CORR_FORE)
        inf->Sub_applied_corrections("fore", c++);
       if (corrections_applied & CORR_SSRB)
        inf->Sub_applied_corrections("ssrb", c++);
       if (corrections_applied & CORR_SEG0)
        inf->Sub_applied_corrections("seg0", c++);
       if (corrections_applied & CORR_Randoms_Smoothing)
        inf->Sub_applied_corrections("randoms smoothing", c++);
       if (corrections_applied & CORR_Randoms_Subtraction)
        inf->Sub_applied_corrections("randoms subtraction", c++);
       if (corrections_applied & CORR_untilted)
        inf->Sub_applied_corrections("untilted", c++);
       if (corrections_applied & CORR_XYSmoothing)
        inf->Sub_applied_corrections("xy-smoothing", c++);
       if (corrections_applied & CORR_ZSmoothing)
        inf->Sub_applied_corrections("z-smoothing", c++);
      }
     if (corrections_applied & CORR_Measured_Attenuation_Correction)
      inf->Sub_method_of_attenuation_correction("measured");
      else if (corrections_applied & CORR_Calculated_Attenuation_Correction)
            inf->Sub_method_of_attenuation_correction("calculated");
     inf->Sub_image_relative_start_time((unsigned long int)startTime(), "sec");
     inf->Sub_image_duration((unsigned long int)frameDuration(), "sec");
     inf->Sub_total_number_of_data_sets(1);
     //inf->Sub_total_prompts(0);
     //inf->Sub_total_randoms(0);
     //inf->Sub_total_net_trues(0);
     //inf->Sub_total_uncorrected_singles(0);
     try { Logging::flog()->loggingOn(false);
           inf->Sub_number_of_bucket_rings();
           Logging::flog()->loggingOn(true);
         }
     catch (const Exception r)
      { Logging::flog()->loggingOn(true);
        if (r.errCode() == REC_ITF_HEADER_ERROR_KEY_MISSING)
         inf->SubKeyAfterKey(inf->Sub_number_of_bucket_rings(0),
                             inf->Sub_IMAGE_DATA_DESCRIPTION());
         else throw;
      }
     for (i=0; i < inf->Sub_number_of_bucket_rings(); i++)
      inf->Sub_bucket_ring_singles(0, i);
     { unsigned short int m=0;

       while ((1 << m) != mash()) m++;
       inf->Sub_angular_compression(m);
     }
     if (corrections_applied & CORR_Normalized)
      inf->Sub_quantification_units("mCi/ml");
      else inf->Sub_quantification_units("ECAT counts");
     if (decay_factor != 1.0f)
      { inf->Sub_decay_correction(KV::START);
        inf->Sub_decay_correction_factor(decay_factor);
      }
      else { inf->Sub_decay_correction(KV::NODEC);
             inf->Sub_decay_correction_factor(1.0f);
           }
     inf->Sub_scatter_fraction_factor(scatter_fraction);
     for (i=0; i < deadtime_factor.size(); i++)
      inf->Sub_dead_time_factor(deadtime_factor[i], i);
     ds=inf->Main_data_set(inf->Main_total_number_of_data_sets()-1);
     Logging::flog()->logMsg("write subheader file #1", loglevel+1)->
      arg(subheader_filename);
     inf->saveSubheader(subheader_filename);
     ds.headerfile=NULL;
     ds.datafile=NULL;
     if (data[0].size() > 0)                                   // write dataset
      { Logging::flog()->logMsg("write data file #1", loglevel+1)->
         arg(filename_noext+data_extension);
        rio=new RawIO <float>(filename_noext+data_extension, true, false);
        for (unsigned short int s=0; s < MAX_SINO; s++)
         for (axis=0; axis < data[s].size(); axis++)
          { float *fp;

            fp=MemCtrl::mc()->getFloatRO(data[s][axis], loglevel);
            if (view_mode)
             volume2view(fp, RhoSamples(), ThetaSamples(), axis_slices[axis]);
            rio->write(fp, axis_size[axis]);
            if (view_mode)
             view2volume(fp, RhoSamples(), ThetaSamples(), axis_slices[axis]);
            MemCtrl::mc()->put(data[s][axis]);
          }
        delete rio;
        rio=NULL;
      }
   }
   catch (...)
    { if (rio != NULL) delete rio;
      if (ds.headerfile != NULL) delete ds.headerfile;
      if (ds.datafile != NULL) delete ds.datafile;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Scale sinogram.
    \param[in] factor     scaling factor
    \param[in] s          number of sinogram dataset
    \param[in] loglevel   level for logging

    Scale image by scalar factor.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::scale(const float factor, const unsigned short int s,
                       const unsigned short int loglevel)
 { if (factor == 1.0f) return;
   for (unsigned short int axis=0; axis < data[s].size(); axis++)
    { float *dp;

      dp=MemCtrl::mc()->getFloat(data[s][axis], loglevel);
      vecMulScalar(dp, factor, dp, axis_size[axis]);
      MemCtrl::mc()->put(data[s][axis]);
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the scan type.
    \return scan type

    Request the scan type.
 */
/*---------------------------------------------------------------------------*/
SCANTYPE::tscantype SinogramIO::scanType() const
 { return(scantype);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the scatter fraction.
    \return scatter fraction in %

    Request the scatter fraction.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::scatterFraction() const
 { return(scatter_fraction);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set date and time of calibration.
    \param[in] calib   date and time of calibration

    Set date and time of calibration.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::setCalibrationTime(const tcalibration_datetime calib)
 { calibration=calib;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the deadtime factor.
    \param[in] dtf   deadtime factor

    Set the deadtime factor.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::setDeadTimeFactor(const std::vector <float> dtf)
 { deadtime_factor=dtf;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the ECAT calibration factor.
    \param[in] _ecf   ECAT calibration factor

    Set the ECAT calibration factor.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::setECF(const float _ecf)
 { ecf=_ecf;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the scatter fraction.
    \param[in] sf   scatter fraction in %

    Set the scatter fraction.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::setScatterFraction(const float sf)
 { scatter_fraction=sf;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill an array with the singles rates.
    \param[in] _singles   singles rates

    Fill an array with the singles rates. The array needs to contain one
    value per bucket.
 */
/*---------------------------------------------------------------------------*/
void SinogramIO::setSingles(const std::vector <float> _singles)
 { vsingles=_singles;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a vector with the singles rates.
    \return vector with singles rates

    Request a vector with the singles rates.
 */
/*---------------------------------------------------------------------------*/
std::vector <float> SinogramIO::singles() const
 { return(vsingles);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the size of the sinogram.
    \return size of the sinogram

    Request the size of the sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned long int SinogramIO::size() const
 { return((unsigned long int)axes_slices*planesize);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the size of a sinogram axis.
    \param[in] axis   axis of sinogram
    \return size of axis
 */
/*---------------------------------------------------------------------------*/
unsigned long int SinogramIO::size(const unsigned short int axis) const
 { if (axis >= axes()) return(0);
   return(axis_size[axis]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the span of the sinogram.
    \return span of the sinogram

    Request the span of the sinogram.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::span() const
 { return(vspan);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request start time of scan.
    \return start time of scan in sec

    Request start time of scan in sec.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::startTime() const
 { return(start_time);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove extension from filename.
    \param[in] str   filename
    \return filename without extension

    Remove extension from filename.
 */
/*---------------------------------------------------------------------------*/
std::string SinogramIO::stripExtension(std::string str) const
 { if (checkExtension(&str, Interfile::INTERFILE_MAINHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_SSUBHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_SDATA_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_ASUBHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_ADATA_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_NHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_NDATA_EXTENSION))
    return(str);
   if (checkExtension(&str, ECAT7_SINO1_EXTENSION)) return(str);
   if (checkExtension(&str, ECAT7_SINO2_EXTENSION)) return(str);
   if (checkExtension(&str, ECAT7_ATTN_EXTENSION)) return(str);
   if (checkExtension(&str, ECAT7_NORM1_EXTENSION)) return(str);
   if (checkExtension(&str, ECAT7_NORM2_EXTENSION)) return(str);
   if (checkExtension(&str, SinogramIO::FLAT_SHDR_EXTENSION)) return(str);
   if (checkExtension(&str, SinogramIO::FLAT_SDATA1_EXTENSION)) return(str);
   if (checkExtension(&str, SinogramIO::FLAT_SDATA2_EXTENSION)) return(str);
   return(str);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate sum of counts in sinogram.
    \param[in] loglevel   level for logging
    \return sum of counts

    Calculate sum of counts in sinogram.
*/
/*---------------------------------------------------------------------------*/
double SinogramIO::sumCounts(const unsigned short int loglevel)
 { double sum_sino=0.0;

   for (unsigned short int s=0; s < TOFbins(); s++)
    for (unsigned short int axis=0; axis < data[s].size(); axis++)
     { float *sp;
                                                 // get segment of TOF sinogram
       sp=MemCtrl::mc()->getFloatRO(data[s][axis], loglevel);
                                                     // calculate sum of counts
       for (unsigned long int ptr=0; ptr < axis_size[axis]; ptr++)
        sum_sino+=(double)sp[ptr];
       MemCtrl::mc()->put(data[s][axis]);
     }
   return(sum_sino);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of projections in a sinogram plane.
    \return number of projections in a sinogram plane

    Request the number of projections in a sinogram plane.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::ThetaSamples() const
 { return(vThetaSamples);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the number of time-of-flight bins.
    \return number of time-of-flight bins

    Request the number of time-of-flight bins.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::TOFbins() const
 { return(tof_bins);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the FWHM of TOF gaussian in ns.
    \return FWHM of TOF gaussian in ns

    Request the FWHM of the TOF gaussian in ns.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::TOFfwhm() const
 { return(tof_fwhm);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the width of a TOF bin in ns.
    \return width a of TOF bin in ns

    Request the width of a TOF bin in ns.
 */
/*---------------------------------------------------------------------------*/
float SinogramIO::TOFwidth() const
 { return(tof_width);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request upper level energy threshold of the scan.
    \return upper level energy threshold of the scan in keV

    Request upper level energy threshold of the scan.
 */
/*---------------------------------------------------------------------------*/
unsigned short int SinogramIO::ULD() const
 { return(uld);
 }
#endif

