/*! \file image_io.h
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
 */

#pragma once

#include <string>
#include "ecat7.h"
#include "ecat7_mainheader.h"
#include "interfile.h"
#include "parser.h"
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class ImageIO {
public:
  /*! types of scatter corrections */
  typedef enum {
    SCATTER_NoScatter,             /*!< no scatter correction */
    SCATTER_ScatterDeconvolution,  /*! scatter correction by deconvolution */
    SCATTER_ScatterSimulated,      /*! scatter correction based on simulation */
    SCATTER_ScatterDualEnergy      /*! scatter correction by dual energy measurement */
  } tscatter;
  /*! filename extension of flat image header file */
  static const std::string FLAT_IHDR_EXTENSION,
         /*! filename extension of flat image dataset file */
         FLAT_IDATA_EXTENSION;
private:
  unsigned short int mnr,                      /*!< number of image matrix */
           matrix_frame,                    /*!< number of frame */
           matrix_plane,                    /*!< number of plane */
           matrix_gate,                      /*!< number of gate */
           matrix_data,                   /*!< number of dataset */
           matrix_bed,                        /*!< number of bed */
           lld,         /*!< lower level energy threshold in keV */
           uld;         /*!< upper level energy threshold in keV */
  float start_time,                         /*!< start time of scan in sec */
        frame_duration,                 /*!< duration of scan frame in sec */
        gate_duration,                        /*!< duration of gate in sec */
        vhalflife;                         /*!< halflife of isotope in sec */
  std::string filename;                            /*!< name of image file */
  bool umap_data;                          /*!< is image a \f$\mu\f$-map ? */
  ECAT7 *e7;                                             /*!< ECAT7 object */
  // load image from ECAT7 file
  void loadECAT7(const std::string, const unsigned short int);
  // load image from Interfile
  void loadInterfile(const std::string, const unsigned short int);
  // load image from flat file
  void loadRAW(const std::string, const unsigned short int);
  // save image in ECAT7 format
  void saveECAT7(const std::string, const unsigned short int,
                 const unsigned short int, const unsigned short int,
                 const unsigned short int, const unsigned short int,
                 const unsigned short int, const unsigned short int,
                 const float, Parser::tparam * const, const tscatter,
                 const float, const float, const bool, const bool,
                 const bool, const SCANTYPE::tscantype,
                 const unsigned short int, const unsigned short int);
  // save image in Interfile format
  void saveInterfile(const std::string, const std::string,
                     const unsigned short int, const unsigned short int,
                     const unsigned short int, const unsigned short int,
                     const unsigned short int, const unsigned short int,
                     const float, Parser::tparam * const, const float,
                     const float, const float, const float,
                     const std::vector <float>, const bool, const bool,
                     const bool, const SCANTYPE::tscantype,
                     const unsigned short int, const unsigned short int);
protected:
  Interfile *inf;                                    /*!< Interfile object */
  bool doi_corrected;   /*!< is image corrected for depth-of-interaction ? */
  unsigned long int image_size;                         /*!< size of image */
  unsigned short int vXYSamples,                /*!< width/height of image */
           vZSamples,                        /*!< depth of image */
           /*! bitmask of applied corrections */
           corrections_applied;
  float vDeltaXY,                         /*!< width/height of voxel in mm */
        vDeltaZ,                                 /*!< depth of voxel in mm */
        gauss_fwhm_xy,/*!< FWHM of gaussian filter in mm for x/y-direction */
        gauss_fwhm_z;   /*!< FWHM of gaussian filter in mm for z-direction */
  /*! indices of memory blocks for images */
  std::vector <unsigned short int> data_idx;
  std::vector <float> bedpos;          /*!< horizontal bed positions in mm */
public:
  // initialize the object
  ImageIO(const unsigned short int, const float, const unsigned short int,
          const float, const unsigned short int, const unsigned short int,
          const unsigned short int, const float, const float, const float,
          const float, const float);
  virtual ~ImageIO();                                   // destroy the object
  float bedPos() const;                      // request bed position of image
  // copy an image dataset into the object
  void copyData(float * const, const bool, const bool, const std::string,
                const unsigned short int);
  // copy image header into object
  void copyFileHeader(ECAT7_MAINHEADER *, Interfile *);
  // correct image for depth-of-interaction
  void correctDOI(const unsigned short int);
  // request the bitmask of corrections that were applied to the data
  unsigned short int correctionsApplied() const;
  // set the bitmask of corrections that were applied to the data
  void correctionsApplied(const unsigned short int);
  float DeltaXY() const;                  // request voxel width/height in mm
  float DeltaZ() const;                          // request voxel depth in mm
  // request pointer to the ECAT7 main header
  ECAT7_MAINHEADER *ECAT7mainHeader() const;
  // convert data from float to short integer with slope
  static signed short int *Float2Short(const float * const,
                                       const unsigned long int,
                                       signed short int * const,
                                       signed short int * const,
                                       float * const);
  // convert data from float to short integer with slope and intercept
  static signed short int *Float2Short(const float * const,
                                       const unsigned long int,
                                       float * const, float * const);
  float frameDuration() const;               // request frame duration in sec
  float gateDuration() const;                 // request gate duration in sec
  // remove ownership over the data from object
  float *getRemove(const unsigned short int);
  float halflife() const;                  // request isotope halflife in sec
  // move image dataset into object
  void imageData(float **, const bool, const bool, const std::string,
                 const unsigned short int);
  // request index of data for memory controller
  unsigned short int index() const;
  // initialize acquisition code
  void initACQcode(const unsigned short int, const unsigned short int,
                   const unsigned short int, const unsigned short int,
                   const unsigned short int);
  // initialize image
  void initImage(const bool, const bool, const std::string,
                 const unsigned short int);
  Interfile *InterfileHdr() const;     // request pointer to Interfile header
  bool isUMap() const;                              // is the image a u-map ?
  void isUMap(const bool);                              // set image to u-map
  unsigned short int LLD() const;      // lower level energy threshold in keV
  // load image
  void load(const std::string, const bool, const unsigned short int,
            const std::string, const unsigned short int);
  // write information about image to logging mechanism
  void logGeometry(const unsigned short int) const;
  unsigned short int matrixBed() const;              // request number of bed
  unsigned short int matrixData() const;         // request number of dataset
  unsigned short int matrixFrame() const;          // request number of frame
  unsigned short int matrixGate() const;            // request number of gate
  unsigned short int matrixPlane() const;          // request number of plane
  // print minimum and maximum value to logging mechanism
  void printStat(const unsigned short int) const;
  // save image that was previously loaded
  void save(const std::string, const std::string, Parser::tparam * const,
            const bool, const unsigned short int);
  // save image
  void save(const std::string, const std::string, const unsigned short int,
            const unsigned short int, const unsigned short int,
            const unsigned short int, const unsigned short int,
            const unsigned short int, const unsigned short int,
            const float, Parser::tparam * const, const float, const tscatter,
            const float, const float, const float, const std::vector <float>,
            const bool, const bool, const bool, const SCANTYPE::tscantype,
            const unsigned short int, const unsigned short int);
  // save image in flat format
  void saveFlat(std::string, const unsigned short int) const;
  void scale(const float, const unsigned short int) const;     // scale image
  unsigned long int size() const;                    // request size of image
  float startTime() const;               // request start time of scan in sec
  // remove extension from filename
  static std::string stripExtension(std::string);
  double sumCounts(const unsigned short int);      // calculate sum of counts
  unsigned short int ULD() const;         // upper level energy discriminator
  // uncorrect image for depth-of-interaction
  void uncorrectDOI(const unsigned short int);
  unsigned short int XYSamples() const;             // request width of image
  unsigned short int ZSamples() const;              // request depth of image
};

