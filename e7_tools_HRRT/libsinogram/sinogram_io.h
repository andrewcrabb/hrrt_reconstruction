/*! \file sinogram_io.h
    \brief This class handles a sinogram data structure.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/03/19 initial version
    \date 2004/03/31 added Doxygen style comments
    \date 2004/04/29 save calibration information in Interfile
    \date 2004/04/30 save TOF sinograms as flat files
    \date 2004/07/21 initialize new memory block indices
    \date 2004/09/08 correct extensions for filenames when saving
    \date 2004/09/08 added stripExtension function
    \date 2004/09/16 handle "applied corrections" not in Interfile header
    \date 2004/09/27 write correct number of beds into Interfile main header
    \date 2004/11/05 added "sumCounts" method
 */

# pragma once

#include <string>
#include <vector>
#include "ecat7.h"
#include "ecat7_mainheader.h"
#include "interfile.h"
#include "timedate.h"
#include "types.h"

/*- class definitions -------------------------------------------------------*/

class SinogramIO
 { protected:
                                /*! maximum number of datasets in a sinogram
                                    (trues=1, P&R=2, TOF=9, TOF+P&R=10)      */
    static const unsigned short int MAX_SINO=10;
   private:
    std::string filename;                         /*!< name of sinogram file */
    unsigned short int mnr,                      /*!< number of image matrix */
                       lld,         /*!< lower level energy threshold in keV */
                       uld,         /*!< upper level energy threshold in keV */
                       matrix_frame,                    /*!< number of frame */
                       matrix_plane,                    /*!< number of plane */
                       matrix_gate,                      /*!< number of gate */
                       matrix_data,                   /*!< number of dataset */
                       matrix_bed;                        /*!< number of bed */
    float start_time,                         /*!< start time of scan in sec */
          frame_duration,                 /*!< duration of scan frame in sec */
          gate_duration,                        /*!< duration of gate in sec */
          bedpos,                         /*!< horizontal bed position in mm */
          ecf,                                  /*!< ECAT calibration factor */
          scatter_fraction,                       /*!< scatter fraction in % */
          tof_width,                             /*!< width of TOF bin in ns */
          tof_fwhm;                          /*!< FWHM of TOF gaussian in ns */
    std::vector <float> deadtime_factor,     /*!< deadtime correction factor */
                        vsingles;           /*!< number of singles from scan */
    bool feet_first,                         /*!< was scan done feet first ? */
         bed_moves_in;           /*!< did bed move into gantry during scan ? */
    SCANTYPE::tscantype scantype;                          /*!< type of scan */
    ECAT7 *e7;                                             /*!< ECAT7 object */
    Interfile *inf;                                    /*!< Interfile object */
                                     // copy a sinogram dataset into the object
    template <typename T>
     void copyData(T *, const unsigned short int, const std::string,
                   const unsigned short int);
    void deleteData();                                  // delete sinogram data
                                                   // load component based norm
    void loadComponentNorm(const std::vector <float>,
                           const unsigned short int);
                                               // load sinogram from ECAT7 file
    void loadECAT7(const std::string, const unsigned short int);
                                       // load normalization sinogram from file
    void loadECAT7Norm(const unsigned short int, const unsigned short int,
                       const std::vector <unsigned short int>,
                       const std::vector <float>, const unsigned short int);
                                           // load sinogram from Interfile file
    void loadInterfile(const std::string, const unsigned short int);
                             // load normalization sinogram from Interfile file
    void loadInterfileNorm(const std::vector <float>,
                           const unsigned short int);
                 // load P39 patient normalization sinogram from Interfile file
    void loadInterfileNormP39pat(const unsigned short int);
                                                // load sinogram from flat file
    void loadRAW(const bool, const bool, const std::string,
                 const unsigned short int);
                                             // save sinogram in Interfile file
    void saveInterfile(const std::string, const unsigned short int,
                       const unsigned short int, const unsigned short int,
                       const unsigned short int, const bool, const float,
                       const unsigned short int, const unsigned short int);
   public:
                         /*! filename extension of flat sinogram header file */
    static const std::string FLAT_SHDR_EXTENSION,
                        /*! filename extension of flat sinogram dataset file */
                             FLAT_SDATA1_EXTENSION,
                             /*! filename extension of flat acf dataset file */
                             FLAT_SDATA2_EXTENSION;
                                          /*! information about current scan */
    typedef struct {                    /*! lower level discriminator in keV */
                     unsigned short int lld,
                                        /*! upper level discriminator in keV */
                                        uld;
                     float start_time,        /*!< start time of scan in sec */
                           frame_duration, /*! frame duration of scan in sec */
                           gate_duration,   /*! gate duration of scan in sec */
                           bedpos,                    /*! bed position in mm */
                           halflife;          /*! halflife of isotope in sec */
                   } tscaninfo;
                                               /*! calibration time and date */
    typedef struct { TIMEDATE::tdate date;          /*!< date of calibration */
                     TIMEDATE::ttime time;          /*!< time of calibration */
                   } tcalibration_datetime;
   private:
    tcalibration_datetime calibration;     /*!< date and time of calibration */
   protected:
    float vBinSizeRho,                    /*!< width of a sinogram bin in mm */
          plane_separation,                      /*!< plane separation in mm */
          intrinsic_tilt,                     /*!< intrinsic tilt in degrees */
          decay_factor,                                    /*!< decay factor */
          vhalflife;                         /*!< halflife of isotope in sec */
    std::vector <unsigned short int> axis_slices;            /*!< axes table */
    std::vector <unsigned long int> axis_size;      /*!< table of axis sizes */
    unsigned short int axes_slices,        /*!< number of planes in sinogram */
                       vspan,                          /*!< span of sinogram */
                                         /*! bitmask of applied coirrections */
                       corrections_applied,
                       vRhoSamples,      /*!< number of bins in a projection */
                              /*!< number of projections in a sinogram plane */
                       vThetaSamples,
                       vmrd,                    /*!< maximum ring difference */
                       vmash,                   /*!< mash factor of sinogram */
                       tof_bins;          /*!< number of time-of-flight bins */
    unsigned long int planesize;                 /*!< size of sinogram plane */
                       /*! does sinogram have separate prompts and randoms ? */
    bool prompts_and_randoms,
         acf_data,                 /*! contains sinogram transmission data ? */
         shuffled_tof_data;      /*!< is data shuffled time-of-flight data ? */
                            /*! indices for data blocks in memory controller */
    std::vector <unsigned short int> data[MAX_SINO];
                            // initialize data structures based on model number
    void init(const SinogramIO::tscaninfo, const bool);
                                                  // initialize data structures
    void init(const unsigned short int, const float, const unsigned short int,
              const unsigned short int, const unsigned short int, const float,
              const unsigned short int, const float, const bool,
              const unsigned short int, const unsigned short int, const float,
              const float, const float, const float, const float, const bool);
                                              // log information about sinogram
    void logGeometry(const unsigned short int) const;
   public:
                                                                // constructors
    SinogramIO(const SinogramIO::tscaninfo, const bool);
    SinogramIO(const unsigned short int, const float, const unsigned short int,
               const unsigned short int, const unsigned short int,
               const float, const unsigned short int, const float, const bool,
               const unsigned short int, const unsigned short int, const float,
               const float, const float, const float, const float,
               const unsigned short int, const bool);
    virtual ~SinogramIO();                                        // destructor
    unsigned short int axes() const;                  // request number of axes
    unsigned short int axesSlices() const;          // request number of planes
                                               // request pointer to axes table
    std::vector <unsigned short int> axisSlices() const;
    bool bedMovesIn() const;                   // did bed move in during scan ?
    float bedPos() const;                    // request horizontal bed position
    float BinSizeRho() const;                 // request width of sinogram bins
                                        // request date and time of calibration
    tcalibration_datetime calibrationTime() const;
                                     // copy a sinogram dataset into the object
    template <typename T>
     void copyData(T *, const unsigned short int, const unsigned short int,
                   const unsigned short int, const unsigned short int,
                   const bool, const bool, const std::string,
                   const unsigned short int);
                                      // copy a sinogram header into the object
    void copyFileHeader(ECAT7_MAINHEADER *, Interfile *);
                                // request a bitmask of the applied corrections
    unsigned short int correctionsApplied() const;
                                    // set a bitmask of the applied corrections
    void correctionsApplied(const unsigned short int);
    std::vector <float> deadTimeFactor() const;  // request the deadtime factor
    float decayFactor() const;                      // request the decay factor
    void deleteData(const unsigned short int);     // delete a sinogram dataset
                                       // delete one axis of a sinogram dataset
    void deleteData(const unsigned short int, const unsigned short int);
                                  // request a pointer to the ECAT7 main header
    ECAT7_MAINHEADER *ECAT7mainHeader() const;
    float ECF() const;                   // request the ECAT calibration factor
    bool feetFirst() const;             // was the patient scanned feet first ?
    std::string fileName() const;          // request name of the sinogram file
    float frameDuration() const;                  // request the frame duration
    float gateDuration() const;                    // request the gate duration
    float halflife() const;                     // request the isotope halflife
                         // request the memory controller index of a data block
    unsigned short int index(const unsigned short int,
                             const unsigned short int) const;
     // create a datablock for the axis of a sinogram and initialize with zeros
    float *initFloatAxis(const unsigned short int, const unsigned short int,
                         const unsigned short int, const bool, const bool,
                         const std::string, const unsigned short int);
    Interfile *InterfileHdr() const; // request pointer to the Interfile object
    float intrinsicTilt() const;                      // request intrinsic tilt
    bool isACF() const;                // contains sinogram transmission data ?
    void isACF(const bool);         // set sinogram to transmission or emission
    unsigned short int LLD() const; // request lower level energy discriminator
                                                     // load sinogram from file
    void load(const std::string, const bool, const bool,
              const unsigned short int, const std::string,
              const unsigned short int, const bool pd_sep=false);
                                       // load normalization sinogram from file
    void loadNorm(const std::string, const unsigned short int,
                  const unsigned short int,
                  const std::vector <unsigned short int>, const float,
                  const std::vector <float>, const bool,
                  const unsigned short int);
    unsigned short int mash() const;         // request mash factor of sinogram
    unsigned short int matrixBed() const;              // request number of bed
    unsigned short int matrixData() const;         // request number of dataset
    unsigned short int matrixFrame() const;          // request number of frame
    unsigned short int matrixGate() const;            // request number of gate
    unsigned short int matrixPlane() const;          // request number of plane
    unsigned short int mrd() const;          // request maximum ring difference
                                      // request number of datasets in sinogram
    unsigned short int numberOfSinograms() const;
    float planeSeparation() const;                  // request plane separation
    unsigned long int planeSize() const;      // request size of sinogram plane
                                                 // copy axes table into object
    void putAxesTable(const std::vector <unsigned short int>);
                                                         // resize index vector
    void resizeIndexVec(const unsigned short int, const unsigned short int);
                                      // request number of bins in a projection
    unsigned short int RhoSamples() const;
                                                       // save sinogram in file
    void save(const std::string, const std::string, const unsigned short int,
              const unsigned short int, const unsigned short int,
              const unsigned short int, const unsigned short int, const bool,
              const float, const unsigned short int, const unsigned short int);
                                                  // save sinogram in flat file
    void saveFlat(std::string, const bool, const unsigned short int);
                                                              // scale sinogram
    void scale(const float, const unsigned short int,
               const unsigned short int);
    SCANTYPE::tscantype scanType() const;                  // request scan type
    float scatterFraction() const;                  // request scatter fraction
                                               // set calibration date and time
    void setCalibrationTime(const tcalibration_datetime);
    void setDeadTimeFactor(const std::vector <float>);   // set deadtime factor
    void setECF(const float);                    // set ECAT calibration factor
    void setScatterFraction(const float);               // set scatter fraction
    void setSingles(std::vector <float>);                  // set singles array
                                 // request pointer to array with singles rates
    std::vector <float> singles() const;
    unsigned long int size() const;                 // request size of sinogram
                                               // request size of sinogram axis
    unsigned long int size(const unsigned short int) const;
    unsigned short int span() const;                // request span of sinogram
    float startTime() const;                      // request start time of scan
                                              // remove extension from filename
    std::string stripExtension(std::string) const;
    double sumCounts(const unsigned short int);      // calculate sum of counts
                             // request number of projections in sinogram plane
    unsigned short int ThetaSamples() const;
    unsigned short int TOFbins() const;// request number of time-of-flight bins
    float TOFfwhm() const;                // request FWHM of TOF gaussian in ns
    float TOFwidth() const;                   // request width of TOF bin in ns
    unsigned short int ULD() const;     // request upper level energy threshold
 };
