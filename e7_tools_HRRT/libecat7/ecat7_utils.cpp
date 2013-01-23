/*! \file ecat7_utils.cpp
    \brief This module implements functions that operate on ECAT7 datasets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/17 added Doxygen style comments

    This module implements functions that operate on ECAT7 datasets. The code
    builds a wrapper for the ecat_tmpl module and takes care of the different
    data types used in ECAT7 objects.
 */

#include <cstdio>
#include "ecat7_utils.h"
#include "ecat7_global.h"
#include "ecat_tmpl.h"
#include "exception.h"
#include "fastmath.h"

/*---------------------------------------------------------------------------*/
/*! \brief Cut bins on both sides of a sinogram dataset.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     number of planes in sinogram
    \param[in]     cut        number of bins to cut off on each side
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Cut bins on both sides of a sinogram dataset.
 */
/*---------------------------------------------------------------------------*/
void utils_CutBins(void **data, const unsigned short int datatype,
                   const unsigned short int bins,
                   const unsigned short int angles,
                   const unsigned short int slices,
                   const unsigned short int cut)
 { if (data == NULL) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       cutbins((unsigned char **)data, bins, angles, slices, cut);
       break;
      case E7_DATATYPE_FLOAT:
       cutbins((float **)data, bins, angles, slices, cut);
       break;
      case E7_DATATYPE_SHORT:
       cutbins((signed short int **)data, bins, angles, slices, cut);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate decay and frame-length correction.
    \param[in,out] data               pointer to sinograms
    \param[in]     datatype           datatype of sinogram values
    \param[in]     size               size of dataset
    \param[in]     halflife           half life of isotope in seconds
    \param[in]     decay_corrected    decay correction already done ?
    \param[in]     frame_start_time   start time of frame in milliseconds
    \param[in]     frame_duration     duration of frame in milliseconds
    \return factor for decay correction

    Calculate the decay correction factor if the \em decay_corrected parameter
    is not set:
    \f[
       \lambda=-\frac{ln\left(\frac{1}{2}\right)}{\mbox{halflife}}
    \f]
    \f[
       \mbox{decay\_factor}=\frac{\exp(\lambda\cdot\mbox{start\_time})\cdot
                                  \mbox{duration}\cdot\lambda}
                                 {1-\exp(-\lambda\cdot\mbox{duration})}
    \f]
    Calculate the frame-length correction and scale the dataset by
    \f[
        \mbox{scale\_factor}=\mbox{decay\_factor}\cdot\mbox{duration}
    \f]
 */
/*---------------------------------------------------------------------------*/
float utils_DecayCorrection(void * const data,
                            const unsigned short int datatype,
                            const unsigned long int size, const float halflife,
                            const bool decay_corrected,
                            const unsigned long int frame_start_time,
                            const unsigned long int frame_duration)
 { float factor, duration;
                                                  // works only with float data
   if (datatype != E7_DATATYPE_FLOAT) return(1.0f);
                       // works only if frame duration is larger than 0 seconds
   if ((duration=(float)frame_duration/1000.0f) == 0.0f) return(1.0f);
                                                         // already corrected ?
   if (decay_corrected || (halflife <= 0.0f)) factor=1.0f;
    else { float lambda, divisor, start;
                                       // calculate factor for decay correction
           start=(float)frame_start_time/1000.0f;
           lambda=-logf(0.5f)/halflife;
           divisor=1.0f-expf(-lambda*duration);
           if (divisor == 0.0f) factor=1.0f;
            else factor=expf(lambda*start)*duration*lambda/divisor;
         }
                  // correct data for decay (without frame duration correction)
   if (data != NULL) scaledata((float *)data, size, factor/duration);
   return(factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave sinogram.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     number of planes in sinogram
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Deinterleave sinogram. The resulting sinogram will have twice the number of
    angles and half the number of bins.
 */
/*---------------------------------------------------------------------------*/
void utils_DeInterleave(void * const data, const unsigned short int datatype,
                        const unsigned short int bins,
                        const unsigned short int angles,
                        const unsigned short int slices)
 { if (data == NULL) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       deinterleave((unsigned char *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_FLOAT:
       deinterleave((float *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_SHORT:
       deinterleave((signed short int *)data, bins, angles, slices);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Mirror dataset to exchange feet and head position.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     numbers of planes in sinogram axes
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Mirror dataset to exchange feet and head position.
 */
/*---------------------------------------------------------------------------*/
void utils_Feet2Head(void * const data, const unsigned short int datatype,
                     const unsigned short int bins,
                     const unsigned short int angles,
                     const unsigned short int * const slices)
 { if (data == NULL) return;

   unsigned short int i, done_slices=0;
   unsigned long int sino_slicesize;

   sino_slicesize=(unsigned long int)bins*(unsigned long int)angles;
   for (i=0; i < 64; i++)                                       // all segments
    if (slices[i] > 0)
     { unsigned long int offs;
       unsigned short int slices_in_segment;
                                          // offset to start of segment 0 or +i
       offs=sino_slicesize*(unsigned long int)done_slices;
       slices_in_segment=slices[i];
       if (i > 0) slices_in_segment/=2;
       switch (datatype)                                     // segment 0 or +i
        { case E7_DATATYPE_BYTE:
           feet2head((unsigned char *)data+offs, bins, angles,
                     slices_in_segment);
           break;
          case E7_DATATYPE_FLOAT:
           feet2head((float *)data+offs, bins, angles, slices_in_segment);
           break;
          case E7_DATATYPE_SHORT:
           feet2head((signed short int *)data+offs, bins, angles,
                     slices_in_segment);
           break;
          default:
           throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                           "Data type not supported for ECAT7 files.");
           break;
        }
       if (i > 0)
        {                                      // offset to start of segment -i
          offs=sino_slicesize*
               ((unsigned long int)done_slices+slices_in_segment);
          switch (datatype)                                       // segment -i
           { case E7_DATATYPE_BYTE:
              feet2head((unsigned char *)data+offs, bins, angles,
                        slices_in_segment);
              break;
             case E7_DATATYPE_FLOAT:
              feet2head((float *)data+offs, bins, angles, slices_in_segment);
              break;
             case E7_DATATYPE_SHORT:
              feet2head((signed short int *)data+offs, bins, angles,
                        slices_in_segment);
              break;
             default:
              throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                              "Data type not supported for ECAT7 files.");
              break;
           }
          slices_in_segment*=2;
        }
       done_slices+=slices_in_segment;
     }
     else break;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave sinogram.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     number of planes in sinogram
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Interleave sinogram. The resulting sinogram will have half the number of
    angles and twice the number of bins.
 */
/*---------------------------------------------------------------------------*/
void utils_Interleave(void * const data, const unsigned short int datatype,
                      const unsigned short int bins,
                      const unsigned short int angles,
                      const unsigned short int slices)
 { if (data == NULL) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       interleave((unsigned char *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_FLOAT:
       interleave((float *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_SHORT:
       interleave((signed short int *)data, bins, angles, slices);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation from prone to supine.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     number of planes in sinogram
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Change orientation from prone to supine.
 */
/*---------------------------------------------------------------------------*/
void utils_Prone2Supine(void * const data, const unsigned short int datatype,
                        const unsigned short int bins,
                        const unsigned short int angles,
                        const unsigned short int slices)
 { if (data == NULL) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       prone2supine((unsigned char *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_FLOAT:
       prone2supine((float *)data, bins, angles, slices);
       break;
      case E7_DATATYPE_SHORT:
       prone2supine((signed short int *)data, bins, angles, slices);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram dataset counterclockwise along scanned axis.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     number of planes in sinogram
    \param[in]     rt         number of steps to rotate
                              (each step is 180/#angles degrees)
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Rotate sinogram dataset counterclockwise along scanned axis. Projections
    are moved from one end of the sinogram plane to the other end. During that
    they are flipped and shifted by one bin.
 */
/*---------------------------------------------------------------------------*/
void utils_Rotate(void * const data, const unsigned short int datatype,
                  const unsigned short int bins,
                  const unsigned short int angles,
                  const unsigned short int slices, const signed short int rt)
 { if (data == NULL) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       rotate((unsigned char *)data, bins, angles, slices, rt);
       break;
      case E7_DATATYPE_FLOAT:
       rotate((float *)data, bins, angles, slices, rt);
       break;
      case E7_DATATYPE_SHORT:
       rotate((signed short int *)data, bins, angles, slices, rt);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Scale dataset.
    \param[in,out] data       data part of matrix
    \param[in]     datatype   datatype of sinogram values
    \param[in]     size       number of elements in matrix
    \param[in]     factor     scale factor
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Scale dataset.
 */
/*---------------------------------------------------------------------------*/
void utils_ScaleMatrix(void * const data, const unsigned short int datatype,
                       const unsigned long int size, const float factor)
 { if ((data == NULL) || (factor == 1.0)) return;
   switch (datatype)                                  // just call the template
    { case E7_DATATYPE_BYTE:
       scaledata((unsigned char *)data, size, factor);
       break;
      case E7_DATATYPE_FLOAT:
       scaledata((float *)data, size, factor);
       break;
      case E7_DATATYPE_SHORT:
       scaledata((signed short int *)data, size, factor);
       break;
      default:
       throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                       "Data type not supported for ECAT7 files.");
       break;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from view to volume mode.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     numbers of planes in sinogram axes
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Convert dataset from view to volume mode.
 */
/*---------------------------------------------------------------------------*/
void utils_View2Volume(void * const data, const unsigned short int datatype,
                       const unsigned short int bins,
                       const unsigned short int angles,
                       const unsigned short int * const slices)
 { if (data == NULL) return;

   unsigned short int i, done_slices=0;
   unsigned long int size_sinoslice;

   size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
   for (i=0; i < 64; i++)                                       // all segments
    if (slices[i] > 0)
     { unsigned long int offs;
       unsigned short int slices_in_segment;
                                          // offset to start of segment 0 or +i
       offs=size_sinoslice*(unsigned long int)done_slices;
       slices_in_segment=slices[i];
       if (i > 0) slices_in_segment/=2;
       switch (datatype)                                     // segment 0 or +i
        { case E7_DATATYPE_BYTE:
           view2volume((unsigned char *)data+offs, bins, angles,
                       slices_in_segment);
           break;
          case E7_DATATYPE_FLOAT:
           view2volume((float *)data+offs, bins, angles, slices_in_segment);
           break;
          case E7_DATATYPE_SHORT:
           view2volume((signed short int *)data+offs, bins, angles,
                       slices_in_segment);
           break;
          default:
           throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                           "Data type not supported for ECAT7 files.");
           break;
        }
       if (i > 0)
        {                                      // offset to start of segment -i
          offs=size_sinoslice*
               ((unsigned long int)done_slices+slices_in_segment);
          switch (datatype)                                       // segment -i
           { case E7_DATATYPE_BYTE:
              view2volume((unsigned char *)data+offs, bins, angles,
                          slices_in_segment);
              break;
             case E7_DATATYPE_FLOAT:
              view2volume((float *)data+offs, bins, angles, slices_in_segment);
              break;
             case E7_DATATYPE_SHORT:
              view2volume((signed short int *)data+offs, bins, angles,
                          slices_in_segment);
              break;
             default:
              throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                              "Data type not supported for ECAT7 files.");
              break;
           }
          slices_in_segment*=2;
        }
       done_slices+=slices_in_segment;
     }
     else break;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from volume to view mode.
    \param[in,out] data       sinogram dataset
    \param[in]     datatype   datatype of sinogram values
    \param[in]     bins       number of bins in a projection
    \param[in]     angles     number of angles in sinogram plane
    \param[in]     slices     numbers of planes in sinogram axes
    \exception REC_UNKNOWN_ECAT7_DATATYPE data type not supported for ECAT7
                                          files

    Convert dataset from volume to view mode.
 */
/*---------------------------------------------------------------------------*/
void utils_Volume2View(void * const data, const unsigned short int datatype,
                       const unsigned short int bins,
                       const unsigned short int angles,
                       const unsigned short int * const slices)
 { if (data == NULL) return;

   unsigned short int i, done_slices=0;
   unsigned long int size_sinoslice;

   size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
   for (i=0; i < 64; i++)                                       // all segments
    if (slices[i] > 0)
     { unsigned long int offs;
       unsigned short int slices_in_segment;
                                          // offset to start of segment 0 or +i
       offs=size_sinoslice*(unsigned long int)done_slices;
       slices_in_segment=slices[i];
       if (i > 0) slices_in_segment/=2;
       switch (datatype)                                     // segment 0 or +i
        { case E7_DATATYPE_BYTE:
           volume2view((unsigned char *)data+offs, bins, angles,
                       slices_in_segment);
           break;
          case E7_DATATYPE_FLOAT:
           volume2view((float *)data+offs, bins, angles, slices_in_segment);
           break;
          case E7_DATATYPE_SHORT:
           volume2view((signed short int *)data+offs, bins, angles,
                       slices_in_segment);
           break;
          default:
           throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                           "Data type not supported for ECAT7 files.");
           break;
        }
       if (i > 0)
        {                                      // offset to start of segment -i
          offs=size_sinoslice*
               ((unsigned long int)done_slices+slices_in_segment);
          switch (datatype)                                       // segment -i
           { case E7_DATATYPE_BYTE:
              volume2view((unsigned char *)data+offs, bins, angles,
                          slices_in_segment);
              break;
             case E7_DATATYPE_FLOAT:
              volume2view((float *)data+offs, bins, angles,
                          slices_in_segment);
              break;
             case E7_DATATYPE_SHORT:
              volume2view((signed short int *)data+offs, bins, angles,
                          slices_in_segment);
              break;
             default:
              throw Exception(REC_UNKNOWN_ECAT7_DATATYPE,
                              "Data type not supported for ECAT7 files.");
              break;
           }
          slices_in_segment*=2;
        }
       done_slices+=slices_in_segment;
     }
     else break;
 }
