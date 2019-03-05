/*! \class ImageIO image_io.h "image_io.h"
    \brief This class handles an image data structure.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/19 initial version
    \date 2004/03/24 fixed bug in ECAT7 multi-frame/multi-gate headers
    \date 2004/03/31 allow different prefixes for swap filenames
    \date 2004/07/30 added method to change acquisition code
    \date 2004/08/02 beds shipped with PET only system have different
                     coordinate system than PET/CT beds
    \date 2004/08/12 added isUmap(bool) method
    \date 2004/09/08 correct extensions for filenames when saving
    \date 2004/09/08 added stripExtension function
    \date 2004/09/23 handle "applied corrections" not in Interfile header
    \date 2004/09/25 write correct number of bed positions and planes into
                     ECAT7 main header for overlapped images
    \date 2004/11/05 added "sumCounts" method
    \date 2004/11/12 load Interfile images in signed short int format
    \date 2004/11/12 support series of u-maps from CT conversion
    \date 2004/11/24 added Float2Short() method with intercept
    \date 2005/03/11 added PSF-AW-OSEM

    This class handles a PET image data structure and provides methods to load
    and save these images from and to files. The internal dataformat is always
    float.
 */

#include <fstream>
#include <limits>
#include "image_io.h"
#include "e7_tools_const.h"
#include "e7_common.h"
#include "ecat7.h"
#include "ecat7_image.hpp"
#include "fastmath.h"
#include "gm.h"
#include "logging.h"
#include "mem_ctrl.h"
#include "raw_io.h"
#include "str_tmpl.h"
#include "types.h"
#include "vecmath.h"
#include "wholebody.h"

/*- constants ---------------------------------------------------------------*/

                            /*! filename extension of flat image header file */
const std::string ImageIO::FLAT_IHDR_EXTENSION=".h33";
                           /*! filename extension of flat image dataset file */
const std::string ImageIO::FLAT_IDATA_EXTENSION=".i";

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize the object.
    \param[in] _XYSamples        width/height of the image
    \param[in] _DeltaXY          width/height of a voxel in mm
    \param[in] _ZSamples         depth of the image
    \param[in] _DeltaZ           depth of a voxel in mm
    \param[in] _mnr              matrix number of image
    \param[in] _lld              lower level energy threshold in keV
    \param[in] _uld              upper level energy threshold in kev
    \param[in] _start_time       start time of scan in sec
    \param[in] _frame_duration   duration of scan frame in sec
    \param[in] _gate_duration    duration of scan gate in sec
    \param[in] _halflife         halflife of isotope in sec
    \param[in] _bedpos           horizontal position of the bed

    Initialize the object.
 */
/*---------------------------------------------------------------------------*/
ImageIO::ImageIO(const unsigned short int _XYSamples, const float _DeltaXY,
                 const unsigned short int _ZSamples, const float _DeltaZ,
                 const unsigned short int _mnr,
                 const unsigned short int _lld,
                 const unsigned short int _uld, const float _start_time,
                 const float _frame_duration, const float _gate_duration,
                 const float _halflife, const float _bedpos)
 { gauss_fwhm_xy=0.0f;
   gauss_fwhm_z=0.0f;
   e7=NULL;
   inf=NULL;
   matrix_frame=0;
   matrix_plane=0;
   matrix_gate=0;
   matrix_data=0;
   matrix_bed=0;
   vXYSamples=_XYSamples;
   vDeltaXY=_DeltaXY;
   vZSamples=_ZSamples;
   vDeltaZ=_DeltaZ;
   mnr=_mnr;
   corrections_applied=0;
   start_time=_start_time;
   frame_duration=_frame_duration;
   gate_duration=_gate_duration;
   vhalflife=_halflife;
   bedpos.assign(1, _bedpos);
   lld=_lld;
   uld=_uld;
                                          // calculate size of the image volume
   image_size=(unsigned long int)XYSamples()*(unsigned long int)XYSamples()*
              (unsigned long int)ZSamples();
   data_idx.clear();                   // index of dataset in memory controller
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy the instance of this class.

    Release the used memory and destroy the instance of this class.
 */
/*---------------------------------------------------------------------------*/
ImageIO::~ImageIO()
 { for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlockForce(&data_idx[i]);
   if (e7 != NULL) delete e7;
   if (inf != NULL) delete inf;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the bed position of the image.
    \return bed position of the image in mm

    Request the bed position of the image.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::bedPos() const
 { return(bedpos[0]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy an image dataset into the object.
    \param[in] data             pointer to dataset
    \param[in] _umap_data       is dataset a \f$\mu\f$-map (in units of 1/cm) ?
    \param[in] _doi_corrected   is dataset corrected for depth-of-interaction ?
    \param[in] name             prefix for swap filenames
    \param[in] loglevel         level for logging

    Copy an image dataset into the object. A previously stored image is lost.
    The data pointed to by <I>data</I> is still owned by the user afterwards.
    The size of the new image must be the same as the current size.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::copyData(float * const data, const bool _umap_data,
                       const bool _doi_corrected,
                       const std::string name,
                       const unsigned short int loglevel)
 { float *dptr;

   for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlock(&data_idx[i]);  // delete currently stored image
                                            // request new data block for image
   data_idx.resize(1);
   dptr=MemCtrl::mc()->createFloat(size(), &data_idx[0], name, loglevel);
   memcpy(dptr, data, size()*sizeof(float));                      // copy image
   MemCtrl::mc()->put(data_idx[0]);
   matrix_frame=0;
   matrix_plane=0;
   matrix_gate=0;
   matrix_data=0;
   matrix_bed=0;
   umap_data=_umap_data;
   doi_corrected=_doi_corrected;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Copy an image header into the object.
    \param[in] mh       pointer to an ECAT7 main header
    \param[in] infhdr   pointer to an Interfile object

    Copy an image header into the object. In case of an ECAT7 dataset, only
    the main header is copied and an empty directory structure is created. In
    case of an Interfile object the main header and subheader stored in this
    object are copied and transformed into image headers, if necessary.
    Previously stored headers are lost. Unused parameters can be NULL.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::copyFileHeader(ECAT7_MAINHEADER *mh, Interfile *infhdr)
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
       // remove all main header keys that don't belong in an image main header
      inf->convertToImageMainHeader();
           // remove all subheader keys that don't belong in an image subheader
      inf->convertToImageSubheader();
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Correct the image for depth-of-interaction.
    \param[in] loglevel   level for logging

    Correct the image voxelsize for the depth-of-interaction of the
    detector-crystals. The voxel width and height are multiplied by
    \f[
         \frac{ring\_radius+doi}{ring\_radius}
    \f]
 */
/*---------------------------------------------------------------------------*/
void ImageIO::correctDOI(const unsigned short int loglevel)
 { if (doi_corrected) return;
   Logging::flog()->logMsg("correct image for DOI", loglevel);
   vDeltaXY*=(GM::gm()->ringRadius()+GM::gm()->DOI())/GM::gm()->ringRadius();
   doi_corrected=true;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the corrections that were applied on the data.
    \return bitmask of corrections that were applied on the data.

    Request the corrections that were applied on the data. The return value is
    a bitmask encoding the different possible corrections. See the
    <I>CORR_</I> flags in the file e7_tools_const.h for a list of corrections.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::correctionsApplied() const
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
void ImageIO::correctionsApplied(const unsigned short int ca)
 { corrections_applied=ca;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the voxel with/height.
    \return width/height of a voxel in mm

    Request the width/height of a voxel in mm.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::DeltaXY() const
 { return(vDeltaXY);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the voxel depth.
    \return depth of a voxel in mm

    Request the depth of a voxel in mm.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::DeltaZ() const
 { return(vDeltaZ);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a pointer to the ECAT7 main header.
    \return pointer to the ECAT7 main header.

    Request a pointer to the ECAT7 main header. If no such header is stored in
    this object, NULL is returned.
 */
/*---------------------------------------------------------------------------*/
ECAT7_MAINHEADER *ImageIO::ECAT7mainHeader() const
 { if (e7 == NULL) return(NULL);
   return(e7->MainHeader());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert the input data from float to short integer format.
    \param[in]  data        data in float format
    \param[in]  data_size   size of data
    \param[out] min_value   min. value in short integer data
    \param[out] max_value   max. value in short integer data
    \param[out] factor      scaling factor to convert short integer data back
                            to float
    \return data in short integer format

    Convert the input data from float into short integer format. The data is
    scaled to use the full positive range of the integer representation:
    \f[
        f=\frac{2^{15}-1}{\max(\|\max(data)\|,\|\min(data)\|)}
    \f]
    To convert the data from short integer back to float, the following
    equation can be used:
    \f[
        \mbox{float}=\mbox{factor}*\mbox{short\_int}
    \f]
 */
/*---------------------------------------------------------------------------*/
signed short int *ImageIO::Float2Short(const float * const data,
                                       const unsigned long int data_size,
                                       signed short int * const min_value,
                                       signed short int * const max_value,
                                       float * const factor)
 { float maxv, minv;
   signed short int *buffer;
   unsigned long int i;
                                           // find minimum and maximum of image
   maxv=data[0];
   minv=data[0];
   for (i=1; i < data_size; i++)
    if (data[i] > maxv) maxv=data[i];
     else if (data[i] < minv) minv=data[i];
   *factor=std::max(fabsf(maxv), fabsf(minv));
   if (*factor != 0.0f)
    *factor=(float)std::numeric_limits <signed short int>::max()/(*factor);
    else *factor=1.0f;
                                                   // create short integer data
   buffer=new signed short int[data_size];
   for (i=0; i < data_size; i++)
    buffer[i]=(signed short int)(data[i]**factor);
   if (min_value != NULL) *min_value=(signed short int)(minv**factor);
   if (max_value != NULL) *max_value=(signed short int)(maxv**factor);
   *factor=1.0f/(*factor);
   return(buffer);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert the input data from float to short integer format.
    \param[in]  data        data in float format
    \param[in]  data_size   size of data
    \param[out] factor      scaling factor to convert short integer data back
                            to float
    \param[out] intercept   intercept for conversion from short integer to
                            float
    \return data in short integer format

    Convert the input data from float into short integer format with slope and
    intercept. The data is scaled to use the full positive and negative range
    of the integer representation:
    \f[
        f=\frac{2^{16}-1}{\max(data)\-\min(data))}
    \f]
    To convert the data from short integer back to float, the following
    equation can be used:
    \f[
        \mbox{float}=\mbox{intercept}+\mbox{factor}*\mbox{short\_int}
    \f]
 */
/*---------------------------------------------------------------------------*/
signed short int *ImageIO::Float2Short(const float * const data,
                                       const unsigned long int data_size,
                                       float * const factor,
                                       float * const intercept)
 { float maxv, minv;
   signed short int *buffer;
   unsigned long int i;
                                           // find minimum and maximum of image
   maxv=data[0];
   minv=data[0];
   for (i=1; i < data_size; i++)
    if (data[i] > maxv) maxv=data[i];
     else if (data[i] < minv) minv=data[i];
   *factor=maxv-minv;
   if (*factor != 0.0f)
    *factor=(float)std::numeric_limits <signed short int>::max()/(*factor);
    else *factor=1.0f;
                                                   // create short integer data
   buffer=new signed short int[data_size];
   for (i=0; i < data_size; i++)
    buffer[i]=(signed short int)(((data[i]-minv)**factor)+
                                std::numeric_limits <signed short int>::min());
   *factor=1.0f/(*factor);
   *intercept=minv-(float)std::numeric_limits <signed short int>::min();
   return(buffer);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the frame duration of the scan.
    \return frame duration of the scan in sec

    Request the frame duration of the scan in sec.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::frameDuration() const
 { return(frame_duration);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the gate duration of the scan.
    \return gate duration of the scan in sec

    Request the gate duration of the scan in sec.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::gateDuration() const
 { return(gate_duration);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove the ownership over the data from this object.
    \param[in] loglevel   level for logging
    \return pointer to the image data

    Remove the ownership over the data from this object. The ownership goes
    over to the caller. The data is not released.
 */
/*---------------------------------------------------------------------------*/
float *ImageIO::getRemove(const unsigned short int loglevel)
 { if (data_idx.size() == 0) return(NULL);
   return(MemCtrl::mc()->getRemoveFloat(&data_idx[0], loglevel));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the halflife of the isotope.
    \return halflife of the isotope in sec

    Request the halflife of the isotope in sec
 */
/*---------------------------------------------------------------------------*/
float ImageIO::halflife() const
 { return(vhalflife);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Move an image dataset into the object.
    \param[in,out] data             pointer to dataset
    \param[in]     _umap_data       is dataset a \f$\mu\f$-map (in units of
                                    1/cm) ?
    \param[in]     _doi_corrected   is dataset corrected for depth-of-
                                    interaction ?
    \param[in]     name             prefix for swap filenames
    \param[in]     loglevel         level for logging

    Move an image dataset into the object. A previously stored image is lost.
    The data pointed to by <I>data</I> is owned by the object afterwards and
    the <I>data</I> pointer will be NULL. The size of the new image must be the
    same as the current size.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::imageData(float **data, const bool _umap_data,
                        const bool _doi_corrected, const std::string name,
                        const unsigned short int loglevel)
 { for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlock(&data_idx[i]);  // delete currently stored image
                                      // move image data into memory controller
   data_idx.resize(1);
   MemCtrl::mc()->putFloat(data, image_size, &data_idx[0], name, loglevel);
   matrix_frame=0;
   matrix_plane=0;
   matrix_gate=0;
   matrix_data=0;
   matrix_bed=0;
   umap_data=_umap_data;
   doi_corrected=_doi_corrected;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request memory controller index of dataset.
    \return index for memory controller

    Request memory controller index of dataset.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::index() const
 { return(data_idx[0]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize acquisition code.
    \param[in] frame   frame number
    \param[in] plane   plane number
    \param[in] gate    gate number
    \param[in] data    dataset number
    \param[in] bed     bed number

    Initialize acquisition code.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::initACQcode(const unsigned short int frame,
                          const unsigned short int plane,
                          const unsigned short int gate,
                          const unsigned short int data,
                          const unsigned short int bed)
 { matrix_frame=frame;
   matrix_plane=plane;
   matrix_gate=gate;
   matrix_data=data;
   matrix_bed=bed;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize image and fill it with zeros.
    \param[in] _umap_data       is dataset a \f$\mu\f$-map (in units of 1/cm) ?
    \param[in] _doi_corrected   is dataset corrected for depth-of-interaction ?
    \param[in] name             prefix for swap filenames
    \param[in] loglevel         level for logging

    Initialize image and fill it with zeros.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::initImage(const bool _umap_data, const bool _doi_corrected,
                        const std::string name,
                        const unsigned short int loglevel)
 { float *data;

   for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlock(&data_idx[i]);
   data_idx.resize(1);
   data=MemCtrl::mc()->createFloat(size(), &data_idx[0], name, loglevel);
   memset(data, 0, size()*sizeof(float));
   MemCtrl::mc()->put(data_idx[0]);
   matrix_frame=0;
   matrix_plane=0;
   matrix_gate=0;
   matrix_data=0;
   matrix_bed=0;
   umap_data=_umap_data;
   doi_corrected=_doi_corrected;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request a pointer to the Interfile object.
    \return pointer to the Interfile object.

    Request a pointer to the Interfile object.
 */
/*---------------------------------------------------------------------------*/
Interfile *ImageIO::InterfileHdr() const
 { return(inf);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is the stored image a \f$\mu\f$-map ?
    \return stored image is a \f$\mu\f$-map

    Is the stored image a \f$\mu\f$-map ?
 */
/*---------------------------------------------------------------------------*/
bool ImageIO::isUMap() const
 { return(umap_data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set stored image to \f$\mu\f$-map.
    \param[in] _umap_data   is image a \f$\mu\f$-map ?

    Set stored image to \f$\mu\f$-map.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::isUMap(const bool _umap_data)
 { umap_data=_umap_data;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request lower level energy threshold of the scan.
    \return lower level energy threshold of the scan in keV

    Request lower level energy threshold of the scan.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::LLD() const
 { return(lld);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load an image from file.
    \param[in] _filename    name of file
    \param[in] _umap_data   is dataset a \f$\mu\f$-map (in units of 1/cm) ?
    \param[in] _mnr         number of matrix in file
    \param[in] name         prefix for swap filenames
    \param[in] loglevel     level for logging

    Load an image from file. Supported formats are ECAT7, Interfile and flat
    (IEEE float or signed short integer). A previously stored image is lost.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::load(const std::string _filename, const bool _umap_data,
                   const unsigned short int _mnr, const std::string name,
                   const unsigned short int loglevel)
 { filename=_filename;
   mnr=_mnr;
   for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlock(&data_idx[i]);
   data_idx.clear();
   Logging::flog()->logMsg("load image '#1'", loglevel)->arg(filename);
   Logging::flog()->logMsg("matrix number=#1", loglevel+1)->arg(mnr);
   doi_corrected=true;
   if (isECAT7file(filename)) loadECAT7(name, loglevel);
    else if (isInterfile(filename)) loadInterfile(name, loglevel);
          else loadRAW(name, loglevel);
   umap_data=_umap_data;
   logGeometry(loglevel+1);                            // log geometry of image
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load an image from an ECAT7 file.
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging
    \exception REC_UNKNOWN_ECAT7_MATRIXTYPE unknown matrix type in file

    Load an image from an ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::loadECAT7(const std::string name,
                        const unsigned short int loglevel)
 { if (e7 != NULL) delete e7;
   e7=new ECAT7();
   e7->LoadFile(filename);
   GM::gm()->init(toString(e7->Main_system_type()));
                                           // get sinogram geometry from header
   if (e7->Main_file_type() != E7_FILE_TYPE_Volume16)
    throw Exception(REC_UNKNOWN_ECAT7_MATRIXTYPE,
                    "Unknown ECAT7 matrix type '#1'.").arg(filename);
   matrix_frame=e7->Dir_frame(mnr);
   matrix_plane=e7->Dir_plane(mnr);
   matrix_gate=e7->Dir_gate(mnr);
   matrix_data=e7->Dir_data(mnr);
   matrix_bed=e7->Dir_bed(mnr);
   bedpos.assign(1, bedPosition(e7, mnr));
   e7->LoadData(mnr);
   vXYSamples=e7->Image_x_dim(mnr);
   vZSamples=e7->Image_z_dim(mnr);
   vDeltaXY=e7->Image_x_pixel_size(mnr)*10.0f;
   vDeltaZ=e7->Image_z_pixel_size(mnr)*10.0f;
   if (e7->Image_filter_code(mnr) == E7_FILTER_CODE_Gaussian)
    { gauss_fwhm_xy=e7->Image_x_resolution(mnr)*10.0f;
      gauss_fwhm_z=e7->Image_z_resolution(mnr)*10.0f;
    }
   start_time=(float)e7->Image_frame_start_time(mnr)/1000.0f;
   frame_duration=(float)e7->Image_frame_duration(mnr)/1000.0f;
   gate_duration=(float)e7->Image_gate_duration(mnr)/1000.0f;
   vhalflife=e7->Main_isotope_halflife();
   lld=e7->Main_lwr_true_thres();
   uld=e7->Main_upr_true_thres();
   { signed long int c;                               // read corrections flags

     corrections_applied=0;
     c=e7->Image_processing_code(mnr);
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
     if (c & E7_APPLIED_PROC_Z_smoothing) corrections_applied|=CORR_ZSmoothing;
   }
   if (e7->DataType(mnr) == E7_DATATYPE_SHORT)       // convert data to float ?
    { e7->Short2Float(mnr);
      switch (e7->Main_file_type())
       { case E7_FILE_TYPE_Volume16:
          e7->Image_data_type(E7_DATA_TYPE_IeeeFloat, mnr);
          ((ECAT7_IMAGE *)e7->Matrix(mnr))->ScaleMatrix(
                                                  e7->Image_scale_factor(mnr));
          break;
       }
    }
   for (unsigned short int i=0; i < data_idx.size(); i++)
    MemCtrl::mc()->deleteBlock(&data_idx[i]);          // delete old image data
   data_idx.clear();
                                                 // calculate size of new image
   image_size=(unsigned long int)XYSamples()*(unsigned long int)XYSamples()*
              (unsigned long int)ZSamples();
   float *ptr;

   ptr=(float *)e7->getecat_matrix::MatrixData(mnr);
                                      // move image data into memory controller
   data_idx.resize(1);
   MemCtrl::mc()->putFloat(&ptr, image_size, &data_idx[0], name, loglevel);
   e7->DataDeleted(mnr);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load an image from an Interfile file.
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Load an image from an Interfile file.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::loadInterfile(const std::string name,
                            const unsigned short int loglevel)
 { RawIO <float> *rio_f=NULL;
   RawIO <signed short int> *rio_si=NULL;

   try
   { std::string subhdr_filename, unit;
     bool multibed;
     unsigned short int i;

     if (inf != NULL) delete inf;
     inf=new Interfile();
     inf->loadMainHeader(filename);
     GM::gm()->init(inf->Main_originating_system());
     multibed=(inf->Main_number_of_horizontal_bed_offsets() > 1);
     matrix_plane=0;
                                               // search name of subheader file
     for (i=0; i < inf->Main_total_number_of_data_sets(); i++)
      { Interfile::tdataset ds;
        Interfile::tscan_type st;

        ds=inf->Main_data_set(i);
        inf->acquisitionCode(ds.acquisition_number, &st, &matrix_frame,
                             &matrix_gate, &matrix_bed, &matrix_data, NULL,
                             NULL);
        if ((multibed && (matrix_bed == mnr)) ||
            (!multibed && (matrix_frame == mnr)))
         { subhdr_filename=*(ds.headerfile);
           break;
         }
      }
     Logging::flog()->logMsg("load subheader #1", loglevel)->
      arg(subhdr_filename);
     inf->loadSubheader(subhdr_filename);
     vXYSamples=inf->Sub_matrix_size(0);
     vZSamples=inf->Sub_matrix_size(2);
     vDeltaXY=inf->Sub_scale_factor(0, &unit);
     vDeltaZ=inf->Sub_scale_factor(2, &unit);
     try
     { Logging::flog()->loggingOn(false);
       gauss_fwhm_xy=inf->Sub_xy_filter(&unit);
     }
     catch (...)
      { gauss_fwhm_xy=0.0f;
      }
     try
     { gauss_fwhm_z=inf->Sub_z_filter(&unit);
     }
     catch (...)
      { gauss_fwhm_z=0.0f;
      }
     Logging::flog()->loggingOn(false);
     bedpos.resize(1);
     bedpos[0]=inf->Sub_start_horizontal_bed_position(&unit);
     start_time=(float)inf->Sub_image_relative_start_time(&unit);
     frame_duration=(float)inf->Sub_image_duration(&unit);
     gate_duration=0.0f;
     try
     { Logging::flog()->loggingOn(false);
       vhalflife=inf->Main_isotope_gamma_halflife(&unit);
     }
     catch (...)
      { vhalflife=0;
      }
     Logging::flog()->loggingOn(true);
     if (inf->Main_number_of_energy_windows() > 0)
      { lld=inf->Main_energy_window_lower_level(0, &unit);
        uld=inf->Main_energy_window_upper_level(0, &unit);
      }
      else { lld=0;
             uld=0;
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
         else if (str == "radial arc correction")
               corrections_applied|=CORR_Radial_Arc_correction;
         else if (str == "axial arc correction")
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
     image_size=(unsigned long int)XYSamples()*(unsigned long int)XYSamples()*
                (unsigned long int)ZSamples();
     Logging::flog()->logMsg("load image data #1", loglevel)->
      arg(inf->Sub_name_of_data_file());
     for (i=0; i < data_idx.size(); i++)
      MemCtrl::mc()->deleteBlock(&data_idx[i]);             // delete old image
     data_idx.clear();
                                                             // read image data
     switch (inf->Sub_number_format())
      { case KV::SIGNED_INT_NF:
         { signed short int *data;
                                                // request memory for new image
           data_idx.resize(1);
           data=MemCtrl::mc()->createSInt(image_size, &data_idx[0], name,
                                          loglevel);
           rio_si=new RawIO <signed short int>(inf->Sub_name_of_data_file(),
                              false,
                              inf->Sub_image_data_byte_order() == KV::BIG_END);
           rio_si->read(data, image_size);
           delete rio_si;
           rio_si=NULL;
           MemCtrl::mc()->put(data_idx[0]);
           break;
         }
        case KV::FLOAT_NF:
         { float *data;
                                                // request memory for new image
           data_idx.resize(1);
           data=MemCtrl::mc()->createFloat(image_size, &data_idx[0], name,
                                           loglevel);
           rio_f=new RawIO <float>(inf->Sub_name_of_data_file(), false,
                              inf->Sub_image_data_byte_order() == KV::BIG_END);
           rio_f->read(data, image_size);
           delete rio_f;
           rio_f=NULL;
           MemCtrl::mc()->put(data_idx[0]);
           break;
         }
      }
   }
   catch (...)
    { if (rio_f != NULL) delete rio_f;
      if (rio_si != NULL) delete rio_si;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load an image from a flat file.
    \param[in] name       prefix for swap filenames
    \param[in] loglevel   level for logging

    Load an image from a flat file.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::loadRAW(const std::string name,
                      const unsigned short int loglevel)
 { RawIO <float> *rio=NULL;

   try
   { float *data;

     for (unsigned short int i=0; i < data_idx.size(); i++)
      MemCtrl::mc()->deleteBlock(&data_idx[i]);             // delete old image
     corrections_applied=0;                          // calculate size of image
     image_size=(unsigned long int)XYSamples()*(unsigned long int)XYSamples()*
                (unsigned long int)ZSamples();
                                               // request memory for image data
     data_idx.resize(1);
     data=MemCtrl::mc()->createFloat(image_size, &data_idx[0], name, loglevel);
                                                             // read image data
     rio=new RawIO <float>(filename, false, false);
     rio->read(data, image_size);
     delete rio;
     rio=NULL;
     MemCtrl::mc()->put(data_idx[0]);
     matrix_frame=0;
     matrix_plane=0;
     matrix_gate=0;
     matrix_data=0;
     matrix_bed=0;
     doi_corrected=false;
   }
   catch (...)
    { if (rio != NULL) delete rio;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write information about image to logging mechanism.
    \param[in] loglevel   level for logging

    Write information about image to logging mechanism.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::logGeometry(const unsigned short int loglevel) const
 { std::string str;
   Logging *flog;

   flog=Logging::flog();
   flog->logMsg("XYSamples=#1", loglevel)->arg(XYSamples());
   flog->logMsg("DeltaXY=#1mm", loglevel)->arg(DeltaXY());
   flog->logMsg("ZSamples=#1", loglevel)->arg(ZSamples());
   flog->logMsg("DeltaZ=#1mm", loglevel)->arg(DeltaZ());
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
    str+="radial arc correction, ";
   if (corrections_applied & CORR_Axial_Arc_correction)
    str+="axial arc correction, ";
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
   if (gauss_fwhm_xy != 0.0f)
    flog->logMsg("smoothed with #1mm gaussian in x/y-direction", loglevel)->
     arg(gauss_fwhm_xy);
   if (gauss_fwhm_z != 0.0f)
    flog->logMsg("smoothed with #1mm gaussian in z-direction", loglevel)->
     arg(gauss_fwhm_z);
   if (isUMap()) flog->logMsg("u-map data=yes", loglevel);
    else flog->logMsg("u-map data=no", loglevel);
   flog->logMsg("bed position=#1mm", loglevel)->arg(bedPos());
   flog->logMsg("scan start time=#1 sec", loglevel)->arg(startTime());
   flog->logMsg("scan frame duration=#1 sec (#2 min)", loglevel)->
    arg(frameDuration())->arg(frameDuration()/60.0f);
   flog->logMsg("scan gate duration=#1 sec (#2 min)", loglevel)->
    arg(gateDuration())->arg(gateDuration()/60.0f);
   flog->logMsg("isotope halflife=#1 sec", loglevel)->arg(halflife());
   flog->logMsg("lld=#1 KeV", loglevel)->arg(lld);
   flog->logMsg("uld=#1 KeV", loglevel)->arg(uld);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of bed.
    \return number of bed

    Request number of bed.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::matrixBed() const
 { return(matrix_bed);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of dataset.
    \return number of dataset

    Request number of dataset.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::matrixData() const
 { return(matrix_data);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of frame.
    \return number of frame

    Request number of frame.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::matrixFrame() const
 { return(matrix_frame);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of gate.
    \return number of gate

    Request number of gate.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::matrixGate() const
 { return(matrix_gate);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request number of plane.
    \return number of plane

    Request number of plane.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::matrixPlane() const
 { return(matrix_plane);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Print maximum and minimum value of image to logging mechanism.
    \param[in] loglevel   level for logging

    Print maximum and minimum value of image to logging mechanism.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::printStat(const unsigned short int loglevel) const
 { float minv, maxv, *dp;

   dp=MemCtrl::mc()->getFloat(index(), loglevel);
   minv=dp[0];
   maxv=dp[0];
   for (unsigned long int i=1; i < size(); i++)
    if (dp[i] < minv) minv=dp[i];
     else if (dp[i] > maxv) maxv=dp[i];
   MemCtrl::mc()->put(index());
   Logging::flog()->logMsg("image min: #1  max: #2", loglevel)->arg(minv)->
    arg(maxv);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save image that was previously loaded to file.
    \param[in] _filename          name of file
    \param[in] orig_filename      name of original file (e.g. sinogram file)
    \param[in] v                  command line parameters of application
    \param[in] overlap            overlap beds to wholebody image ?
    \param[in] loglevel           level for logging

    Save image that was previously loaded to file. Supported formats are ECAT7,
    Interfile and flat (IEEE float). In case of a multi-bed scan, the method
    can append the image to the image that is already stored, thus creating a
    wholebody image. It can also store each bed as a separate frame. Images
    are always stored in head-first orientation.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::save(const std::string _filename,
                   const std::string orig_filename,
                   Parser::tparam * const v, const bool overlap,
                   const unsigned short int loglevel)
{ if (e7 != NULL)
   { for (unsigned short int bed=0; bed < data_idx.size(); bed++)
      { bool feet_first;
        SCANTYPE::tscantype scantype;
        tscatter scatter_method;

        switch (e7->Main_patient_orientation())
         { case E7_PATIENT_ORIENTATION_Feet_First_Prone:
           case E7_PATIENT_ORIENTATION_Feet_First_Supine:
           case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Right:
           case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Left:
            feet_first=true;
            Logging::flog()->logMsg("patient orientation: feet first",
                                    loglevel);
            break;
           default:
            feet_first=false;
            Logging::flog()->logMsg("patient orientation: head first",
                                    loglevel);
            break;
         }
        switch (e7->Image_scatter_type(mnr))
         { case E7_SCATTER_TYPE_ScatterDeconvolution:
            scatter_method=SCATTER_ScatterDeconvolution;
            break;
           case E7_SCATTER_TYPE_ScatterSimulated:
            scatter_method=SCATTER_ScatterSimulated;
            break;
           case E7_SCATTER_TYPE_ScatterDualEnergy:
            scatter_method=SCATTER_ScatterDualEnergy;
            break;
           case E7_SCATTER_TYPE_NoScatter:
           default:
            scatter_method=SCATTER_NoScatter;
            break;
         }
        if ((e7->Main_num_bed_pos() > 0) || (bed > 0))
         scantype=SCANTYPE::MULTIBED;
         else if (e7->Main_num_gates() > 1) scantype=SCANTYPE::MULTIGATE;
         else if (e7->Main_num_frames() > 1) scantype=SCANTYPE::MULTIFRAME;
         else scantype=SCANTYPE::SINGLEFRAME;
        save(_filename, orig_filename,
             (unsigned short int)e7->Image_num_r_elements(mnr),
             (unsigned short int)e7->Image_num_angles(mnr), matrix_frame,
             matrix_plane, matrix_gate, matrix_data, matrix_bed,
             e7->Main_intrinsic_tilt(), v, 0.0f, scatter_method,
             e7->Image_decay_corr_fctr(mnr), 0.0f,
             e7->Main_ecat_calibration_factor(), std::vector <float>(),
             overlap, feet_first, e7->Main_bed_position(0) > 0.0f, scantype,
             bed, loglevel);
      }
     return;
   }
  if (inf != NULL)
   { for (unsigned short int bed=0; bed < data_idx.size(); bed++)
      { bool feet_first;
        SCANTYPE::tscantype scantype;
        std::vector <float> dtf;
        unsigned short int recbins, recviews;
        float dcfac, scfac, sqf;
        std::string unit;

        try
        { Logging::flog()->loggingOn(false);
          for (unsigned short int c=0;; c++)
           if ((inf->Sub_dead_time_factor(c) == float()) || (c > 2))
            { dtf.resize(c);
              break;
            }
        }
        catch (...)
         { dtf.clear();
         }
        Logging::flog()->loggingOn(true);
        for (unsigned short int i=0; i < dtf.size(); i++)
         dtf[i]=inf->Sub_dead_time_factor(i);
        switch (inf->Main_patient_orientation())
         { case KV::FFP:
           case KV::FFS:
           case KV::FFDR:
           case KV::FFDL:
            feet_first=true;
            Logging::flog()->logMsg("patient orientation: feet first",
                                    loglevel);
            break;
           default:
            feet_first=false;
            Logging::flog()->logMsg("patient orientation: head first",
                                    loglevel);
            break;
         }
        if ((inf->Main_number_of_horizontal_bed_offsets() > 1) || (bed > 0))
         scantype=SCANTYPE::MULTIBED;
         else if (inf->Main_number_of_time_frames() > 1)
               scantype=SCANTYPE::MULTIFRAME;
         else scantype=SCANTYPE::SINGLEFRAME;
        try
        { Logging::flog()->loggingOn(false);
          recbins=inf->Sub_reconstruction_bins();
        }
        catch (...)
         { recbins=0;
         }
        try
        { recviews=inf->Sub_reconstruction_views();
        }
        catch (...)
         { recviews=0;
         }
        try
        { dcfac=inf->Sub_decay_correction();
        }
        catch (...)
         { dcfac=1.0f;
         }
        try
        { scfac=inf->Sub_scatter_fraction_factor();
        }
        catch (...)
         { scfac=0.0f;
         }
        try
        { sqf=inf->Main_scanner_quantification_factor(&unit);
        }
        catch (...)
         { sqf=0.0f;
         }
        Logging::flog()->loggingOn(true);
        save(_filename, orig_filename, recbins, recviews, matrix_frame,
             matrix_plane, matrix_gate, matrix_data, matrix_bed,
             inf->Main_gantry_tilt_angle(&unit), v,
             inf->Sub_reconstruction_diameter(&unit), SCATTER_NoScatter, dcfac,
             scfac, sqf, dtf, overlap, feet_first,
             inf->Main_scan_direction() == KV::SCAN_IN, scantype, bed,
             loglevel);
      }
     return;
   }
  save(_filename, orig_filename, 0, 0, matrix_frame, matrix_plane, matrix_gate,
       matrix_data, matrix_bed, 0.0f, v, 0.0f, SCATTER_NoScatter, 1.0f,
       0.0f, 1.0f, std::vector <float>(), overlap, false, false,
       SCANTYPE::SINGLEFRAME, 0, loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save image to file.
    \param[in] _filename          name of file
    \param[in] orig_filename      name of original file (e.g. sinogram file)
    \param[in] RhoSamples         number of bins in sinogram projections
    \param[in] ThetaSamples       number of angles in sinogram
    \param[in] matrix_frame       number of frame
    \param[in] matrix_plane       number of plane
    \param[in] matrix_gate        number of gate
    \param[in] matrix_data        number of dataset
    \param[in] matrix_bed         number of bed
    \param[in] tilt               tilt of image
    \param[in] v                  command line parameters of application
    \param[in] fov_diameter       diameter of the FOV in mm
    \param[in] sc                 method of scatter correction
    \param[in] decay_factor       decay correction factor
    \param[in] scat_frac          scatter fraction
    \param[in] ecf                scanner calibration factor
    \param[in] deadtime_fac       deadtime factor
    \param[in] overlap            overlap beds to wholebody image ?
    \param[in] feet_first         patient was scanned feet-first ?
    \param[in] bed_moves_in       bed was moving in gantry direction during
                                  scan ?
    \param[in] scantype           type of scan (multi-bed/-frame/-gate)
    \param[in] bed                number of bed
    \param[in] loglevel           level for logging

    Save image to file. Supported formats are ECAT7, Interfile and flat (IEEE
    float). In case of a multi-bed scan, the method can append the image to
    the image that is already stored, thus creating a wholebody image. It can
    also store each bed as a separate frame. Images are always stored in
    head-first orientation.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::save(const std::string _filename,
                   const std::string orig_filename,
                   const unsigned short int RhoSamples,
                   const unsigned short int ThetaSamples,
                   const unsigned short int matrix_frame,
                   const unsigned short int matrix_plane,
                   const unsigned short int matrix_gate,
                   const unsigned short int matrix_data,
                   const unsigned short int matrix_bed, const float tilt,
                   Parser::tparam * const v, const float fov_diameter,
                   const tscatter sc, const float decay_factor,
                   const float scat_frac, const float ecf,
                   const std::vector <float> deadtime_fac, const bool overlap,
                   const bool feet_first, const bool bed_moves_in,
                   const SCANTYPE::tscantype scantype,
                   const unsigned short int bed,
                   const unsigned short int loglevel)
 { filename=_filename;
   if ((mnr == 0) && (e7 == NULL) && (inf == NULL))
    { saveFlat(filename, loglevel);
      return;
    }
   if (isInterfile(orig_filename))
    { saveInterfile(filename, orig_filename, RhoSamples, ThetaSamples,
                    matrix_frame, matrix_gate, matrix_data, matrix_bed, tilt,
                    v, fov_diameter, decay_factor, scat_frac, ecf,
                    deadtime_fac, overlap, feet_first, bed_moves_in,
                    scantype, bed, loglevel);
      return;
    }
   saveECAT7(filename, RhoSamples, ThetaSamples, matrix_frame, matrix_plane,
             matrix_gate, matrix_data, matrix_bed, tilt, v, sc, decay_factor,
             ecf, overlap, feet_first, bed_moves_in, scantype, bed, loglevel);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save image to ECAT7 file.
    \param[in] filename           name of file
    \param[in] RhoSamples         number of bins in sinogram projections
    \param[in] ThetaSamples       number of angles in sinogram
    \param[in] matrix_frame       number of frame
    \param[in] matrix_plane       number of plane
    \param[in] matrix_gate        number of gate
    \param[in] matrix_data        number of dataset
    \param[in] matrix_bed         number of bed
    \param[in] tilt               tilt of image
    \param[in] v                  command line parameters of application
    \param[in] sc                 method of scatter correction
    \param[in] decay_factor       decay correction factor
    \param[in] ecf                ECAT calibration factor
    \param[in] overlap            overlap beds to wholebody image ?
    \param[in] feet_first         patient was scanned feet-first ?
    \param[in] bed_moves_in       bed was moving in gantry direction during
                                  scan ?
    \param[in] scantype           type of scan (multi-bed/-frame/-gate)
    \param[in] bed                number of bed
    \param[in] loglevel           level for logging

    Save image to ECAT7 file. In case of a multi-bed scan, the method can
    append the image to the image that is already stored, thus creating a
    wholebody image. It can also store each bed as a separate frame. Images are
    always stored in head-first orientation.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::saveECAT7(const std::string filename,
                        const unsigned short int RhoSamples,
                        const unsigned short int ThetaSamples,
                        const unsigned short int matrix_frame,
                        const unsigned short int matrix_plane,
                        const unsigned short int matrix_gate,
                        const unsigned short int matrix_data,
                        const unsigned short int matrix_bed, const float tilt,
                        Parser::tparam * const v, const tscatter sc,
                        const float decay_factor, const float ecf,
                        const bool overlap, const bool feet_first,
                        const bool bed_moves_in,
                        const SCANTYPE::tscantype scantype,
                        const unsigned short int bed,
                        const unsigned short int loglevel)
 { signed short int *simage=NULL;
   float *fptr=NULL;
   Wholebody *wb=NULL;

   try
   { float flip_offset, fbedpos;
     unsigned short int bn;

     if (bed > 1) bn=bed;
      else bn=mnr;
     Logging::flog()->logMsg("write image file #1 (matrix #2)", loglevel)->
      arg(filename)->arg(bn);
     if (overlap && feet_first) flip_offset=(float)ZSamples()*DeltaZ();
      else flip_offset=0.0f;
     if (bn == 0)                                         // create main header
      { e7->Main_magic_number((unsigned char *)"MATRIX72v");
        e7->Main_original_file_name((unsigned char *)
                                    stripPath(filename).c_str());
        e7->Main_ecat_calibration_factor(ecf);
        fbedpos=e7->Main_init_bed_position()*10.0f;
        e7->Main_file_type(E7_FILE_TYPE_Volume16);
        e7->Main_intrinsic_tilt(tilt);
        e7->Main_isotope_halflife(halflife());
        e7->Main_lwr_true_thres(LLD());
        e7->Main_upr_true_thres(ULD());
        if ((feet_first && (fbedpos < bedpos[bed])) ||
            (!feet_first && (fbedpos > bedpos[bed])))
         e7->Main_init_bed_position(bedpos[bed]/10.0f);
        if (overlap && feet_first)
         { e7->Main_init_bed_position(e7->Main_init_bed_position()+
                                      flip_offset/10.0f);
           switch (e7->Main_patient_orientation())
            { case E7_PATIENT_ORIENTATION_Feet_First_Prone:
               e7->Main_patient_orientation(
                                      E7_PATIENT_ORIENTATION_Head_First_Prone);
               break;
              case E7_PATIENT_ORIENTATION_Feet_First_Supine:
               e7->Main_patient_orientation(
                                     E7_PATIENT_ORIENTATION_Head_First_Supine);
               break;
              case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Right:
               e7->Main_patient_orientation(
                            E7_PATIENT_ORIENTATION_Head_First_Decubitus_Right);
               break;
              case E7_PATIENT_ORIENTATION_Feet_First_Decubitus_Left:
               e7->Main_patient_orientation(
                             E7_PATIENT_ORIENTATION_Head_First_Decubitus_Left);
               break;
            }
         }
        if (e7->Main_num_bed_pos() > 0)
         for (unsigned short int i=0; i < 15; i++)
          e7->Main_bed_position(0.0f, i);
        e7->Main_num_bed_pos(0);
        e7->Main_num_gates(1);
        e7->Main_num_frames(1);
        e7->Main_num_planes(ZSamples());
        e7->SaveFile(filename);                             // save main header
      }
      else { if (e7 != NULL) delete e7;
             e7=new ECAT7();
             e7->LoadFile(filename);                      // load existing file
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
                case SCANTYPE::SINGLEFRAME:                     // can't happen
                 break;
              }
             fbedpos=e7->Main_init_bed_position()*10.0f-flip_offset;
             if (overlap)                                        // overlap bed
              { signed short int min_value, max_value;
                unsigned short int depth;
                float factor, topos;

                e7->LoadData(0);
                e7->Short2Float(0);
                e7->Image_data_type(E7_DATA_TYPE_IeeeFloat, 0);
                ((ECAT7_IMAGE *)e7->Matrix(0))->ScaleMatrix(
                                                    e7->Image_scale_factor(0));
                                                   // calculate wholebody image
                float offset = (float)e7->Image_z_dim(0) * e7->Image_z_pixel_size(0) * 10.0f;
                offset = (feet_first) ? -1.0f * offset : offset;
                topos = e7->Main_init_bed_position() * 10.0f + offset;
                Logging::flog()->logMsg("existing image: pos=#1mm to #2mm, #3 "
                                        "planes", loglevel+1)->
                 arg(e7->Main_init_bed_position()*10.0f)->arg(topos)->
                 arg(e7->Image_z_dim(0));
                wb=new Wholebody((float *)e7->getecat_matrix::MatrixData(0),
                                 e7->Image_x_dim(0),
                                 e7->Image_y_dim(0),
                                 e7->Image_z_dim(0),
                                 e7->Image_z_pixel_size(0)*10.0f, fbedpos);
                e7->DataDeleted(0);
                if (feet_first) topos=bedpos[bed]+(float)ZSamples()*DeltaZ();
                 else topos=bedpos[bed]-(float)ZSamples()*DeltaZ();
                Logging::flog()->logMsg("add bed at pos #1mm to #2mm, #3 "
                                        "planes", loglevel+1)->
                 arg(bedpos[bed])->arg(topos)->arg(ZSamples());
                wb->addBed(MemCtrl::mc()->getFloatRO(data_idx[bed], loglevel),
                           ZSamples(), bedpos[bed], feet_first, bed_moves_in,
                           loglevel+1);
                MemCtrl::mc()->put(data_idx[bed]);
                fptr=wb->getWholebody(&depth);
                       // convert image data from float to signed short integer
                simage=Float2Short(fptr, (unsigned long int)XYSamples()*
                                         (unsigned long int)XYSamples()*
                                         (unsigned long int)depth, &min_value,
                                   &max_value, &factor);
                delete[] fptr;
                fptr=NULL;
                delete wb;
                wb=NULL;
                if ((feet_first == bed_moves_in) != GM::gm()->isPETCT())
                 e7->Main_init_bed_position((bedpos[bed]+flip_offset)/10.0f);
                e7->Main_num_planes(depth);
                e7->Main_num_bed_pos(0);
                e7->Image_z_dim(depth, 0);
                e7->Image_scale_factor(factor, 0);
                e7->Image_image_min(min_value, 0);
                e7->Image_image_max(max_value, 0);
                e7->Image_data_type(E7_DATA_TYPE_SunShort, 0);
                e7->getecat_matrix::MatrixData(simage, E7_DATATYPE_SHORT, 0);
                e7->SaveFile(filename);                     // save file header
                e7->AppendMatrix(0);                            // append image
                e7->DeleteData(0);
                return;
              }
                                                     // create multi-frame file
             e7->Main_bed_position((bedpos[bed]-fbedpos)/10.f, bn-1);
             e7->UpdateMainHeader(filename);
           }
     e7->CreateImageMatrices(1);
                                           // create directory entry for matrix
     e7->Dir_frame(matrix_frame, bn);
     e7->Dir_plane(matrix_plane, bn);
     e7->Dir_gate(matrix_gate, bn);
     e7->Dir_data(matrix_data, bn);
     e7->Dir_bed(matrix_bed, bn);
                                                      // create image subheader
     e7->Image_data_type(E7_DATA_TYPE_SunShort, bn);
     e7->Image_num_dimensions(3, bn);
     e7->Image_x_dim(XYSamples(), bn);
     e7->Image_y_dim(XYSamples(), bn);
     e7->Image_z_dim(ZSamples(), bn);
     e7->Image_x_offset(0.0f, bn);
     e7->Image_y_offset(0.0f, bn);
     e7->Image_z_offset(0.0f, bn);
     e7->Image_recon_zoom(1.0f, bn);
     { float sf;

       if (doi_corrected) sf=1.0f;
        else sf=(GM::gm()->ringRadius()+GM::gm()->DOI())/
                GM::gm()->ringRadius();
       e7->Image_x_pixel_size(DeltaXY()*sf/10.0f, bn);
       e7->Image_y_pixel_size(DeltaXY()*sf/10.0f, bn);
     }
     e7->Image_z_pixel_size(e7->Main_plane_separation(), bn);
     e7->Image_frame_start_time((signed int)(startTime()*1000.0f), bn);
     e7->Image_frame_duration((signed int)(frameDuration()*1000.0f), bn);
     e7->Image_gate_duration((signed int)(gateDuration()*1000.0f), bn);
     if ((gauss_fwhm_xy != 0.0f) || (gauss_fwhm_z != 0.0f))
      e7->Image_filter_code(E7_FILTER_CODE_Gaussian, bn);
     e7->Image_x_resolution(gauss_fwhm_xy/10.0f, bn);
     e7->Image_y_resolution(gauss_fwhm_xy/10.0f, bn);
     e7->Image_z_resolution(gauss_fwhm_z/10.0f, bn);
     e7->Image_num_r_elements(RhoSamples, bn);
     e7->Image_num_angles(ThetaSamples, bn);
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
       e7->Image_processing_code(c, bn);
     }
     switch (sc)
      { case SCATTER_NoScatter:
         e7->Image_scatter_type(E7_SCATTER_TYPE_NoScatter, bn);
         break;
        case SCATTER_ScatterDeconvolution:
         e7->Image_scatter_type(E7_SCATTER_TYPE_ScatterDeconvolution, bn);
         break;
        case SCATTER_ScatterSimulated:
         e7->Image_scatter_type(E7_SCATTER_TYPE_ScatterSimulated, bn);
         break;
        case SCATTER_ScatterDualEnergy:
         e7->Image_scatter_type(E7_SCATTER_TYPE_ScatterDualEnergy, bn);
         break;
      }
     e7->Image_scatter_type(sc, bn);
     e7->Image_z_rotation_angle(0.0f, bn);
     e7->Image_decay_corr_fctr(decay_factor, bn);
     switch (v->algorithm)
      { case ALGO::FBP_Algo:
         e7->Image_recon_type(E7_RECON_TYPE_FBPJ, bn);
         break;
        default:
         e7->Image_recon_type(99, bn);
         break;
      }
     { signed short int min_value, max_value;
       float factor;
                           // convert image data from float to signed short int
       simage=Float2Short(MemCtrl::mc()->getFloatRO(data_idx[bed], loglevel),
                          size(), &min_value, &max_value, &factor);
       MemCtrl::mc()->put(data_idx[bed]);
       if (overlap && feet_first)   // turn image from feet-first to head-first
        Wholebody::feet2head(simage, XYSamples(), XYSamples(), ZSamples());
       e7->Image_scale_factor(factor, bn);
       e7->Image_image_min(min_value, bn);
       e7->Image_image_max(max_value, bn);
       e7->getecat_matrix::MatrixData(simage, E7_DATATYPE_SHORT, bn);
     }
     e7->AppendMatrix(bn);                             // append matrix to file
     e7->DeleteData(bn);
   }
   catch (...)
    { if (simage != NULL) delete[] simage;
      if (fptr != NULL) delete[] fptr;
      if (wb != NULL) delete wb;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save image to flat file.
    \param[in] filename   name of file
    \param[in] loglevel   level for logging

    Save image to flat file. An additional ASCII file is generated with
    information about the size of the image dataset and the size of the voxels.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::saveFlat(std::string filename,
                       const unsigned short int loglevel) const
 { RawIO <float> *rio=NULL;
   std::ofstream *file=NULL;

   try
   { float sf;
     std::string datafilename, hdrfilename;

     for (unsigned short int bed=0; bed < data_idx.size(); bed++)
      { unsigned short int bn;

        if (bed > 0) bn=bed;
         else bn=mnr;
        hdrfilename=stripExtension(filename);
        if (data_idx.size() > 1) hdrfilename+="_"+toStringZero(bed, 2);
        datafilename=hdrfilename+FLAT_IDATA_EXTENSION;
        hdrfilename+=FLAT_IHDR_EXTENSION;
                                                            // create data file
        Logging::flog()->logMsg("write image file #1 (matrix #2)", loglevel)->
         arg(datafilename)->arg(bn);
        rio=new RawIO <float>(datafilename, true, false);
        rio->write(MemCtrl::mc()->getFloat(data_idx[bed], loglevel), size());
        MemCtrl::mc()->put(data_idx[bed]);
        delete rio;
        rio=NULL;
                                                    // create ASCII header file
        Logging::flog()->logMsg("write image header file #1 (matrix #2)",
                                loglevel)->arg(hdrfilename)->arg(bn);
        if (doi_corrected) sf=1.0f;
         else sf=(GM::gm()->ringRadius()+GM::gm()->DOI())/
                 GM::gm()->ringRadius();
        file=new std::ofstream(hdrfilename.c_str(), std::ios::out);
        *file << "!INTERFILE\n"
                 "name of data file := "
              << stripPath(datafilename) << "\n"
                 "image data byte order := LITTLEENDIAN\n"
                 "number of dimensions := 3\n"
                 "matrix size [1] := " << XYSamples()
              << "\nmatrix size [2] := " << XYSamples()
              << "\nmatrix size [3] := " << ZSamples()
              << "\nnumber format := float\n"
                 "number of bytes per pixel := " << sizeof(float)
              << "\ndata offset in bytes := 0"
              << "\nscaling factor (mm/pixel) [1] := " << DeltaXY()*sf
              << "\nscaling factor (mm/pixel) [2] := " << DeltaXY()*sf
              << "\nscaling factor (mm/pixel) [3] := " << DeltaZ()
              << std::endl;
        delete file;
        file=NULL;
      }
   }
   catch (...)
    { if (rio != NULL) delete rio;
      if (file != NULL) delete file;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save image to Interfile file.
    \param[in] filename           name of file
    \param[in] orig_filename      name of original file (e.g. sinogram)
    \param[in] RhoSamples         number of bins in sinogram projections
    \param[in] ThetaSamples       number of angles in sinogram
    \param[in] matrix_frame       number of frame
    \param[in] matrix_gate        number of gate
    \param[in] matrix_data        number of dataset
    \param[in] matrix_bed         number of bed
    \param[in] tilt               tilt of image
    \param[in] v                  command line parameters of application
    \param[in] fov_diameter       diameter of the FOV in mm
    \param[in] decay_factor       decay correction factor
    \param[in] scatter_frac       scatter fraction
    \param[in] ecf                scanner calibration factor
    \param[in] deadtime_factor    deadtime factor
    \param[in] overlap            overlap beds to wholebody image ?
    \param[in] feet_first         patient was scanned feet-first ?
    \param[in] bed_moves_in       bed was moving in gantry direction during
                                  scan ?
    \param[in] scantype           type of scan (multi-bed/-frame/-gate)
    \param[in] bed                number of bed
    \param[in] loglevel           level for logging

    Save image to ECAT7 file. In case of a multi-bed scan, the method can
    append the image to the image that is already stored, thus creating a
    wholebody image. It can also store each bed as a separate frame. Images are
    always stored in head-first orientation.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::saveInterfile(const std::string filename,
                            const std::string orig_filename,
                            const unsigned short int RhoSamples,
                            const unsigned short int ThetaSamples,
                            const unsigned short int matrix_frame,
                            const unsigned short int matrix_gate,
                            const unsigned short int matrix_data,
                            const unsigned short int matrix_bed,
                            const float tilt, Parser::tparam * const v,
                            const float fov_diameter, const float decay_factor,
                            const float scatter_fraction, const float ecf,
                            const std::vector <float> deadtime_factor,
                            const bool overlap, const bool feet_first,
                            const bool bed_moves_in,
                            const SCANTYPE::tscantype scantype,
                            const unsigned short int bed,
                            const unsigned short int loglevel)
 { signed short int *simage=NULL;
   float *fptr=NULL;
   Wholebody *wb=NULL;
   RawIO <float> *rio=NULL;
   Interfile::tdataset ds;

   ds.headerfile=NULL;
   ds.datafile=NULL;
   try
   { float flip_offset;
     std::string filename_noext, str, subheader_filename,
                 prev_subheader_filename, unit;
     unsigned short int bn;

     if (bed > 0) bn=bed;
      else bn=mnr;
     Logging::flog()->logMsg("write image file #1 (matrix #2)", loglevel)->
      arg(filename)->arg(bn);
     filename_noext=stripExtension(filename);
     if (!overlap)
      { prev_subheader_filename=filename_noext+"_"+toStringZero(bn-1, 2);
        filename_noext+="_"+toStringZero(bn, 2);
      }
      else prev_subheader_filename=filename_noext;
     prev_subheader_filename+=Interfile::INTERFILE_IHDR_EXTENSION;
     subheader_filename=filename_noext+Interfile::INTERFILE_IHDR_EXTENSION;
     if (overlap && feet_first && (scantype == SCANTYPE::MULTIBED))
      flip_offset=(float)ZSamples()*DeltaZ();
      else flip_offset=0.0f;
     if (bn == 0)                                         // create main header
      { inf->Main_comment("created with code from "+std::string(__DATE__)+" "+
                          std::string(__TIME__));
        inf->Main_gantry_tilt_angle((unsigned short int)tilt, "degrees");
        inf->Main_isotope_gamma_halflife(halflife(), "sec");
        if (inf->Main_number_of_energy_windows() > 0)
         { inf->Main_energy_window_lower_level(LLD(), 0, "keV");
           inf->Main_energy_window_upper_level(ULD(), 0, "keV");
         }
        inf->Main_CPS_data_type("image main header");
        inf->Main_data_description("3d raw image");
        inf->Main_number_of_time_frames(1);
        inf->Main_number_of_horizontal_bed_offsets(1);
        inf->Main_data_set_remove();
        inf->Main_total_number_of_data_sets(1);
        ds.headerfile=new std::string(stripPath(filename_noext)+
                                      Interfile::INTERFILE_IHDR_EXTENSION);
        ds.datafile=new std::string(stripPath(filename_noext)+
                                    Interfile::INTERFILE_IDATA_EXTENSION);
        if (bed > 0) bn=bed;
         else bn=matrix_bed;
        if (isUMap())
         ds.acquisition_number=inf->acquisitionCode(
                                                  Interfile::TRANSMISSION_TYPE,
                                                  matrix_frame, matrix_gate,
                                                  bn, matrix_data,
                                                  Interfile::TRUES_MODE, 0);
         else ds.acquisition_number=inf->acquisitionCode(
                                                     Interfile::EMISSION_TYPE,
                                                     matrix_frame, matrix_gate,
                                                     bn, matrix_data,
                                                     Interfile::TRUES_MODE, 0);
        inf->Main_data_set(ds, 0);
        ds.headerfile=NULL;
        ds.datafile=NULL;
        inf->Main_blank_line();
        inf->Main_SUPPLEMENTARY_ATTRIBUTES();
        if (GM::gm()->ringArchitecture())
         inf->Main_PET_scanner_type(KV::CYLINDRICAL_RING);
         else inf->Main_PET_scanner_type(KV::MULTIPLE_PLANAR);
        inf->Main_transaxial_FOV_diameter((float)GM::gm()->RhoSamples()*
                                          GM::gm()->BinSizeRho(),
                                          std::string("mm"));
        inf->Main_number_of_rings(GM::gm()->rings());
        inf->Main_distance_between_rings(GM::gm()->planeSeparation(),
                                         std::string("mm"));
        inf->Main_bin_size(GM::gm()->BinSizeRho(), std::string("mm"));
        inf->Main_scanner_quantification_factor(ecf, std::string("mCi/ml"));

        inf->MainKeyAfterKey(inf->Main_patient_scan_orientation(
                              inf->Main_patient_orientation(), "during scan"),
                             inf->Main_patient_orientation());
        if (overlap || !isUMap())
         switch (inf->Main_patient_orientation())
          { case KV::FFP:
             inf->Main_patient_orientation(KV::HFP, "in image");
             break;
            case KV::FFS:
             inf->Main_patient_orientation(KV::HFS, "in image");
             break;
            case KV::FFDR:
             inf->Main_patient_orientation(KV::HFDR, "in image");
             break;
            case KV::FFDL:
             inf->Main_patient_orientation(KV::HFDL, "in image");
             break;
            case KV::HFP:           // orientation didn't change for head first
            case KV::HFS:
            case KV::HFDR:
            case KV::HFDL:
             break;
          }
        Logging::flog()->logMsg("write main header file #1", loglevel+1)->
         arg(filename);
        inf->saveMainHeader(filename);
        inf->SubKeyBeforeKey(inf->Sub_name_of_sinogram_file(" "),
                             inf->Sub_name_of_data_file());
        inf->SubKeyBeforeKey(inf->Sub_image_scale_factor(0.0f),
                             inf->Sub_number_of_dimensions());

        inf->SubKeyBeforeKey(inf->Sub_reconstruction_diameter(0.0f, "mm"),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_reconstruction_bins(0),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_reconstruction_views(0),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_process_status(KV::RECONSTRUCTED),
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
        inf->SubKeyBeforeKey(inf->Sub_slice_orientation(KV::TRANSVERSE),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_method_of_reconstruction(" "),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_number_of_iterations(0),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_number_of_subsets(0),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_stopping_criteria(" "),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_filter_name(" "),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_xy_filter(0.0f, "mm"),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_z_filter(0.0f, "mm"),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_image_zoom(0.0f),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_x_offset(0.0f, "mm"),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_y_offset(0.0f, "mm"),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_method_of_scatter_correction(" "),
                             inf->Sub_axial_compression());
        inf->SubKeyBeforeKey(inf->Sub_method_of_random_correction(KV::DLYD),
                             inf->Sub_axial_compression());
        inf->Sub_blank_line();
        inf->Sub_SUPPLEMENTARY_ATTRIBUTES();
        inf->Sub_number_of_projections(0);
        inf->Sub_number_of_views(0);
        inf->SubKeyAfterKey(inf->Sub_axial_compression(0),
                            inf->Sub_number_of_views());
        inf->SubKeyAfterKey(inf->Sub_maximum_ring_difference(0),
                            inf->Sub_axial_compression());

        { float sf;

          if (doi_corrected) sf=1.0f;
           else sf=(GM::gm()->ringRadius()+GM::gm()->DOI())/
                   GM::gm()->ringRadius();
          inf->Sub_scale_factor(DeltaXY()*sf, 0, "mm/pixel");
          inf->Sub_scale_factor(DeltaXY()*sf, 1, "mm/pixel");
          inf->Sub_scale_factor(DeltaZ(), 2, "mm/pixel");
        }
      }
      else { if (inf != NULL) delete inf;
             inf=new Interfile();
             inf->loadMainHeader(filename);               // load existing file
             inf->loadSubheader(prev_subheader_filename);
             if (overlap && (scantype == SCANTYPE::MULTIBED))   // overlap beds
              { unsigned short int depth;
                float topos, fbedpos;
                unsigned long int datasize;
                std::string unit;

                datasize=(unsigned long int)inf->Sub_matrix_size(0)*
                         (unsigned long int)inf->Sub_matrix_size(1)*
                         (unsigned long int)inf->Sub_matrix_size(2);
                fbedpos=inf->Sub_start_horizontal_bed_position(&unit)-
                        flip_offset;
                                                    // load existing image data
                fptr=new float[datasize];
                rio=new RawIO <float>(filename_noext+
                                      Interfile::INTERFILE_IDATA_EXTENSION,
                                      false, false);
                rio->read(fptr, datasize);
                delete rio;
                rio=NULL;
                                                   // calculate wholebody image
                if (!feet_first)
                 topos=inf->Sub_start_horizontal_bed_position(&unit)-
                       (float)inf->Sub_matrix_size(2)*
                       inf->Sub_scale_factor(2, &unit);
                 else topos=inf->Sub_start_horizontal_bed_position(&unit)-
                            (float)inf->Sub_matrix_size(2)*
                            inf->Sub_scale_factor(2, &unit);
                Logging::flog()->logMsg("existing image: pos=#1mm to #2mm, #3 "
                                        "planes", loglevel+1)->
                 arg(inf->Sub_start_horizontal_bed_position(&unit))->
                 arg(topos)->arg(inf->Sub_matrix_size(2));
                wb=new Wholebody(fptr, inf->Sub_matrix_size(0),
                                 inf->Sub_matrix_size(1),
                                 inf->Sub_matrix_size(2),
                                 inf->Sub_scale_factor(2, &unit), fbedpos);
                fptr=NULL;
                if (!feet_first) topos=bedpos[bed]-(float)ZSamples()*DeltaZ();
                 else topos=bedpos[bed]-(float)ZSamples()*DeltaZ();
                Logging::flog()->logMsg("add bed at pos #1mm to #2mm, #3 "
                                        "planes", loglevel+1)->
                 arg(bedpos[bed])->arg(topos)->arg(ZSamples());
                wb->addBed(MemCtrl::mc()->getFloat(data_idx[bed], loglevel+1),
                           ZSamples(), bedpos[bed], feet_first, bed_moves_in,
                           loglevel+1);
                MemCtrl::mc()->put(data_idx[bed]);
                fptr=wb->getWholebody(&depth);
                delete wb;
                wb=NULL;
                if (!bed_moves_in)
                 inf->Sub_start_horizontal_bed_position(bedpos[bed], "mm");
                /*
                if ((feet_first == bed_moves_in) != GM::gm()->isPETCT())
                 inf->Sub_start_horizontal_bed_position(bedpos[bed], "mm");
                 else if ((feet_first == bed_moves_in) == GM::gm()->isPETCT())
                       inf->Sub_start_horizontal_bed_position(bedpos[bed],
                                                              "mm");
                */
                inf->Sub_matrix_size(depth, 2);
                                                      // save updated subheader
                Logging::flog()->logMsg("write subheader file #1",
                                        loglevel+1)->arg(subheader_filename);
                inf->saveSubheader(subheader_filename);
                datasize=(unsigned long int)inf->Sub_matrix_size(0)*
                         (unsigned long int)inf->Sub_matrix_size(1)*
                         (unsigned long int)inf->Sub_matrix_size(2);
                                                        // save wholebody image
                Logging::flog()->logMsg("write data file #1", loglevel+1)->
                 arg(filename_noext+Interfile::INTERFILE_IDATA_EXTENSION);
                rio=new RawIO <float>(filename_noext+
                                      Interfile::INTERFILE_IDATA_EXTENSION,
                                      true, false);
                rio->write(fptr, datasize);
                delete rio;
                rio=NULL;
                delete[] fptr;
                fptr=NULL;
                return;
              }
                                                     // create multi-frame file
             inf->Main_total_number_of_data_sets(
                                      inf->Main_total_number_of_data_sets()+1);
             ds.headerfile=new std::string(stripPath(filename_noext)+
                                          Interfile::INTERFILE_IHDR_EXTENSION);
             ds.datafile=new std::string(stripPath(filename_noext)+
                                         Interfile::INTERFILE_IDATA_EXTENSION);
             if (bed > 0) bn=bed;
              else bn=matrix_bed;
             if (isUMap())
              ds.acquisition_number=inf->acquisitionCode(
                                                  Interfile::TRANSMISSION_TYPE,
                                                  matrix_frame, matrix_gate,
                                                  bn, matrix_data,
                                                  Interfile::TRUES_MODE, 0);
              else ds.acquisition_number=inf->acquisitionCode(
                                                     Interfile::EMISSION_TYPE,
                                                     matrix_frame, matrix_gate,
                                                     bn, matrix_data,
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
             inf->saveMainHeader(filename);         // save updated main header
           }
     inf->Sub_comment("created with code from "+std::string(__DATE__)+" "+
                      std::string(__TIME__));
     inf->Sub_start_horizontal_bed_position(bedpos[bed], "mm");
     /*
     if (overlap && feet_first)
      inf->Sub_start_horizontal_bed_position(
                     inf->Sub_start_horizontal_bed_position(&unit)+flip_offset,
                     "mm");
     */
     inf->Sub_CPS_data_type("image subheader");
     inf->Sub_name_of_sinogram_file(orig_filename);
     inf->Sub_name_of_data_file(stripPath(filename_noext)+
                                Interfile::INTERFILE_IDATA_EXTENSION);
     inf->Sub_PET_data_type(KV::IMAGE);
     inf->Sub_image_data_byte_order(KV::LITTLE_END);
     inf->Sub_number_format(KV::FLOAT_NF);
     inf->Sub_number_of_bytes_per_pixel(sizeof(float));
     inf->Sub_matrix_axis_label("x", 0);
     inf->Sub_matrix_axis_label("y", 1);
     inf->Sub_matrix_axis_label("z", 2);
     inf->Sub_matrix_size(XYSamples(), 0);
     inf->Sub_matrix_size(XYSamples(), 1);
     inf->Sub_matrix_size(ZSamples(), 2);
     inf->Sub_image_scale_factor(1.0f);
     inf->Sub_reconstruction_diameter(fov_diameter, "mm");
     inf->Sub_reconstruction_bins(RhoSamples);
     inf->Sub_reconstruction_views(ThetaSamples);
     inf->Sub_process_status(KV::RECONSTRUCTED);
     if (isUMap()) str="1/cm";
      else if (corrections_applied & CORR_Normalized) str="mCi/ml";
            else str="ECAT counts";
     inf->Sub_quantification_units(str);
     if (decay_factor != 1.0f)
      { inf->Sub_decay_correction(KV::START);
        inf->Sub_decay_correction_factor(decay_factor);
      }
      else { inf->Sub_decay_correction(KV::NODEC);
             inf->Sub_decay_correction_factor(1.0f);
           }
     inf->Sub_scatter_fraction_factor(scatter_fraction);
     for (unsigned short int i=0; i < deadtime_factor.size(); i++)
      inf->Sub_dead_time_factor(deadtime_factor[i], i);
     inf->Sub_slice_orientation(KV::TRANSVERSE);
     switch (v->algorithm)
      { case ALGO::FBP_Algo:
         str="FBP";
         break;
        case ALGO::DIFT_Algo:
         str="DIFT";
         break;
        case ALGO::OSEM_UW_Algo:
         str="UW-OSEM";
         break;
        case ALGO::OSEM_AW_Algo:
         str="AW-OSEM";
         break;
        case ALGO::OSEM_NW_Algo:
         str="NW-OSEM";
         break;
        case ALGO::OSEM_ANW_Algo:
         str="ANW-OSEM";
         break;
        case ALGO::OSEM_OP_Algo:
         str="OP-OSEM";
         break;
        case ALGO::MAPTR_Algo:
         str="MAP-TR";
         break;
        case ALGO::MLEM_Algo:
         str="MLEM";
         break;
        case ALGO::OSEM_PSF_AW_Algo:
         str="PSF-AW-OSEM";
         break;
      }
     inf->Sub_method_of_reconstruction(str);
     if ((v->algorithm == ALGO::FBP_Algo) ||
         (v->algorithm == ALGO::DIFT_Algo))
      { inf->Sub_number_of_iterations(0);
        inf->Sub_number_of_subsets(0);
      }
      else { inf->Sub_number_of_iterations(v->iterations);
             inf->Sub_number_of_subsets(v->subsets);
           }
     inf->Sub_stopping_criteria("fixed iterations");
     if ((v->gauss_fwhm_xy != 0.0f) || (v->gauss_fwhm_z != 0.0f))
      inf->Sub_filter_name("gaussian");
      else inf->Sub_filter_name("unknown");
     inf->Sub_xy_filter(v->gauss_fwhm_xy, "mm");
     inf->Sub_z_filter(v->gauss_fwhm_z, "mm");
     inf->Sub_image_zoom(v->zoom_factor);
     inf->Sub_x_offset(0.0f, "mm");
     inf->Sub_y_offset(0.0f, "mm");
     inf->Sub_method_of_scatter_correction("unknown");
     inf->Sub_method_of_random_correction(KV::DLYD);
     { unsigned short int c=0;

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
        inf->Sub_applied_corrections("radial arc correction", c++);
       if (corrections_applied & CORR_Axial_Arc_correction)
        inf->Sub_applied_corrections("axial arc correction", c++);
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
     inf->Sub_number_of_projections(GM::gm()->RhoSamples());
     inf->Sub_number_of_views(GM::gm()->ThetaSamples());
     inf->Sub_axial_compression(v->span);
     inf->Sub_maximum_ring_difference(v->mrd);
                                                              // save subheader
     Logging::flog()->logMsg("write subheader file #1", loglevel+1)->
      arg(subheader_filename);
     inf->saveSubheader(subheader_filename);
                                                                  // save image
     Logging::flog()->logMsg("write data file #1", loglevel+1)->
      arg(filename_noext+Interfile::INTERFILE_IDATA_EXTENSION);
     rio=new RawIO <float>(filename_noext+Interfile::INTERFILE_IDATA_EXTENSION,
                           true, false);
                                                 // convert image to head-first
     if (overlap && feet_first && (scantype == SCANTYPE::MULTIBED))
      { float *data;

        data=MemCtrl::mc()->getFloat(data_idx[bed], loglevel+1);
        Wholebody::feet2head(data, XYSamples(), XYSamples(), ZSamples());
        rio->write(data, size());
        Wholebody::feet2head(data, XYSamples(), XYSamples(), ZSamples());
      }
      else rio->write(MemCtrl::mc()->getFloat(data_idx[bed], loglevel+1),
                      size());
     MemCtrl::mc()->put(data_idx[bed]);
     delete rio;
     rio=NULL;
   }
   catch (...)
    { if (simage != NULL) delete[] simage;
      if (fptr != NULL) delete[] fptr;
      if (wb != NULL) delete wb;
      if (rio != NULL) delete rio;
      if (ds.headerfile != NULL) delete ds.headerfile;
      if (ds.datafile != NULL) delete ds.datafile;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Scale image.
    \param[in] factor     scaling factor
    \param[in] loglevel   level for logging

    Scale image by scalar factor.
 */
/*---------------------------------------------------------------------------*/
void ImageIO::scale(const float factor,
                    const unsigned short int loglevel) const
 { if (factor == 1.0f) return;
   float *data;

   data=MemCtrl::mc()->getFloat(data_idx[0], loglevel);
   vecMulScalar(data, factor, data, size());
   MemCtrl::mc()->put(data_idx[0]);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request size of image volume.
    \return: size of image volume

    Request size of image volume
 */
/*---------------------------------------------------------------------------*/
unsigned long int ImageIO::size() const
 { return(image_size);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request start time of scan.
    \return start time of scan in sec

    Request start time of scan in sec.
 */
/*---------------------------------------------------------------------------*/
float ImageIO::startTime() const
 { return(start_time);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Remove extension from filename.
    \param[in] str   filename
    \return filename without extension

    Remove extension from filename.
 */
/*---------------------------------------------------------------------------*/
std::string ImageIO::stripExtension(std::string str)
 { if (checkExtension(&str, Interfile::INTERFILE_MAINHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_IHDR_EXTENSION))
    return(str);
   if (checkExtension(&str, Interfile::INTERFILE_IDATA_EXTENSION))
    return(str);
   if (checkExtension(&str, ECAT7_IMAG_EXTENSION)) return(str);
   if (checkExtension(&str, ImageIO::FLAT_IHDR_EXTENSION)) return(str);
   if (checkExtension(&str, ImageIO::FLAT_IDATA_EXTENSION)) return(str);
   return(str);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate sum of counts in image.
    \param[in] loglevel   level for logging
    \return sum of counts

    Calculate sum of counts in image.
*/
/*---------------------------------------------------------------------------*/
double ImageIO::sumCounts(const unsigned short int loglevel)
 { double sum_counts;
   float *data;

   data=MemCtrl::mc()->getFloat(data_idx[0], loglevel);
   sum_counts=vecScalarAdd(data, size());
   MemCtrl::mc()->put(data_idx[0]);
   return(sum_counts);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request upper level energy threshold of the scan.
    \return upper level energy threshold of the scan in keV

    Request upper level energy threshold of the scan.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::ULD() const
 { return(uld);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Uncrrect the image for depth-of-interaction.
    \param[in] loglevel   level for logging

    Uncorrect the image voxelsize for the depth-of-interaction of the
    detector-crystals. The voxel width and height are multiplied by
    \f[
         \frac{ring\_radius}{ring\_radius+doi}
    \f]
 */
/*---------------------------------------------------------------------------*/
void ImageIO::uncorrectDOI(const unsigned short int loglevel)
 { if (!doi_corrected) return;
   float sf;

   Logging::flog()->logMsg("uncorrect image for DOI", loglevel);
   sf=(GM::gm()->ringRadius()+GM::gm()->DOI())/GM::gm()->ringRadius();
   vDeltaXY*=1.0f/sf;
   doi_corrected=false;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the width/height of the image.
    \return width/height of the image in voxel

    Request the width/height of the image in voxel.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::XYSamples() const
 { return(vXYSamples);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the depth of the image.
    \return depth of the image in planes

    Request the depth of the image in planes.
 */
/*---------------------------------------------------------------------------*/
unsigned short int ImageIO::ZSamples() const
 { return(vZSamples);
 }
