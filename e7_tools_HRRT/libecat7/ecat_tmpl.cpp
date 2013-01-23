/*! \file ecat_tmpl.cpp
    \brief This module implements templates that operate on ECAT7 datasets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 1997/11/11 initial version
    \date 2005/01/10 added Doxygen style comments

    This module implements templates that operate on ECAT7 datasets.
 */

#include <iostream>
#include <cstdlib>
#ifndef _ECAT_TMPL_CPP
#define _ECAT_TMPL_CPP
#include "ecat_tmpl.h"
#endif
#include "data_changer.h"
#include "ecat7_global.h"
#include "vecmath.h"

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Cut bins on both sides of a sinogram dataset.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram
    \param[in]     cut      number of bins to cut off on each side

    Cut bins on both sides of a sinogram dataset.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void cutbins(T **data, const unsigned short int bins,
             const unsigned short int angles, const unsigned short int slices,
             const unsigned short int cut)
 { if ((*data == NULL) || (cut == 0)) return;
   T *buffer=NULL;

   try
   { if (bins <= 2*cut) { delete[] *data;
                          *data=NULL;
                        }

     T *bp, *dp;
     unsigned long int sizeyz, yz;
     unsigned short int new_bins;

     new_bins=bins-2*cut;                  // number of bins in cutted sinogram
     sizeyz=(unsigned long int)angles*(unsigned long int)slices;
     buffer=new T[sizeyz*(unsigned long int)new_bins];    // create new dataset
     for (bp=buffer,
          dp=*data+cut,
          yz=0; yz < sizeyz; yz++,                                  // cut bins
          dp+=bins,
          bp+=new_bins)
      memcpy(bp, dp, new_bins*sizeof(T));
     delete[] *data;
     *data=buffer;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Deinterleave sinogram.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Deinterleave sinogram. The resulting sinogram will have twice the number of
    angles and half the number of bins.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void deinterleave(T * const data, const unsigned short int bins,
                  const unsigned short int angles,
                  const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *dp;
     unsigned long int size_sinoslice;
     unsigned short int s;

     size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
     buffer=new T[size_sinoslice];                         // temporary dataset
     for (dp=data,
          s=0; s < slices; s++,
          dp+=size_sinoslice)
      { T *sp;
        unsigned short int a;

        for (sp=buffer,
             a=0; a < angles; a++,
             sp+=bins)
                               // double number of angles, halve number of bins
         { T *bp;
           unsigned short int b;

           bp=dp+a*bins;
           for (b=0; b < bins/2; b++)
            sp[b]=bp[b*2];
           for (b=0; b < bins/2; b++)
            sp[bins/2+b]=bp[b*2+1];
         }
        memcpy(dp, buffer, size_sinoslice*sizeof(T));
      }
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Mirror dataset to exchange feet and head position.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Mirror dataset to exchange feet and head position.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void feet2head(T * const data, const unsigned short int bins,
               const unsigned short int angles,
               const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *sp, *sp1;
     unsigned short int s;
     unsigned long int bytes, size_sinoslice;

     size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
     buffer=new T[size_sinoslice];                         // temporary dataset
     bytes=size_sinoslice*sizeof(T);                  // size of slice in bytes
     for (sp=data,
          sp1=sp+(slices-1)*size_sinoslice,
          s=0; s < slices/2; s++,                              // mirror slices
          sp1-=size_sinoslice,
          sp+=size_sinoslice)
      { memcpy(buffer, sp, bytes);
        memcpy(sp, sp1, bytes);
        memcpy(sp1, buffer, bytes);
      }
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Interleave sinogram.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Interleave sinogram. The resulting sinogram will have half the number of
    angles and twice the number of bins.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void interleave(T * const data, const unsigned short int bins,
                const unsigned short int angles,
                const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *dp;
     unsigned long int size_sinoslice;
     unsigned short int s;

     size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
     buffer=new T[size_sinoslice];                         // temporary dataset
     for (dp=data,
          s=0; s < slices; s++,
          dp+=size_sinoslice)
      { T *sp;
        unsigned short int a;

        for (sp=buffer,
             a=0; a < angles; a+=2,
             sp+=2*bins)
         { T *bp1, *bp2;
           unsigned short int b;
                                // half number of angles, double number of bins
           for (bp1=dp+a*bins,
                bp2=dp+(a+1)*bins,
                b=0; b < bins; b++)
            { sp[b*2]=bp1[b];
              sp[b*2+1]=bp2[b];
            }
         }
        memcpy(dp, buffer, size_sinoslice*sizeof(T));
      }
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Load data part of a matrix.
    \param[in] file             file handle
    \param[in] matrix_size      number of elements in matrix
    \param[in] matrix_records   number of records used by matrix
    \param[in] len              length of single element in file in bytes
    \return new dataset or NULL in case of error

    Load data part of a matrix. The data is converted from the big-endian
    format of the ECAT7 file into the format of this computer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
T *loaddata(std::ifstream * const file, const  unsigned long int matrix_size,
            const unsigned long int matrix_records,
            const unsigned short int len)
 { if (matrix_size == 0) return(NULL);
   T *data=NULL;
   DataChanger *dc=NULL;

   try
   { T *bp;
     unsigned long int i;

     data=new T[matrix_size];                                 // create dataset
                       // DataChanger is used to read data system independently
     dc=new DataChanger(E7_RECLEN, false, true);
     for (bp=data,
          i=0; i < matrix_records; i++)
      { dc->ResetBufferPtr();
        dc->LoadBuffer(file);                          // load data into buffer
                                                   // retrieve data from buffer
        for (unsigned long int j=0; j < E7_RECLEN/len; j++)
         if (bp < data+matrix_size) dc->Value(bp++);
          else break;
      }
     delete dc;
     dc=NULL;
     return(data);
   }
   catch (...)                                             // handle exceptions
    { if (dc != NULL) delete dc;
      if (data != NULL) delete[] data;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Change orientation from prone to supine.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Change orientation from prone to supine.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void prone2supine(T * const data, const unsigned short int bins,
                  const unsigned short int angles,
                  const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *sp, *sp1, *sp2;
     unsigned short int s, bytes;
     unsigned long int size_sinoslice, as;

     size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
     bytes=bins*sizeof(T);
     buffer=new T[bins];                                   // temporary dataset
     for (sp=data,
          s=0; s < slices; s++,                // mirror lines of all sinograms
          sp+=size_sinoslice)
      { unsigned short int a;

        for (sp1=sp,
             sp2=sp+(angles-1)*bins,
             a=0; a < angles/2; a++,
             sp2-=bins,
             sp1+=bins)
        { memcpy(buffer, sp1, bytes);
          memcpy(sp1, sp2, bytes);
          memcpy(sp2, buffer, bytes);
        }
      }
     delete[] buffer;
     buffer=NULL;
                                             // mirror columns of all sinograms
     for (sp=data,
          as=0; as < (unsigned long int)angles*(unsigned long int)slices; as++,
          sp+=bins)
      for (unsigned short int b=0; b < bins/2; b++)
       std::swap(sp[b], sp[bins-1-b]);
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rotate sinogram dataset counterclockwise along scanned axis.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram
    \param[in]     rt       number of steps to rotate
                            (each step is 180/#angles degrees)

    Rotate sinogram dataset counterclockwise along scanned axis. Projections
    are moved from one end of the sinogram plane to the other end. During that
    they are flipped and shifted by one bin.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void rotate(T * const data, const unsigned short int bins,
            const unsigned short int angles,
            const unsigned short int slices, const signed short int rt)
 { if (data == NULL) return;
   T *buffer=NULL, *buffer2=NULL;

   try
   { unsigned short int s;
     unsigned long int size_sinoslice, bytes, bytes2;

     if ((rt == 0) || (abs(rt) >= angles)) return;
     size_sinoslice=(unsigned long int)bins*(unsigned long int)angles;
     bytes=abs(rt)*bins;
     buffer=new T[bytes];                  // buffer for upper part of sinogram
     bytes2=(angles-abs(rt))*bins;
     if (rt > 0) buffer2=new T[bytes2];    // buffer for lower part of sinogram
     for (s=0; s < slices; s++)
      { unsigned short int a;
        T *sp;

        if (rt > 0)
         memcpy(buffer, data+bytes2+s*size_sinoslice, bytes*sizeof(T));
         else memcpy(buffer, data+s*size_sinoslice, bytes*sizeof(T));
                // mirror bins in upper part of each sinogram and shift one bin
        for (sp=buffer,
             a=0; a < abs(rt); a++,
             sp+=bins)
         { unsigned short int b;
           T *sp1, *sp2;

           for (sp1=sp+1,
                sp2=sp+bins-1,
                b=0; b < bins/2-1; b++,
                sp2--,
                sp1++)
            { T tmp;

              tmp=*sp1; *sp1=*sp2; *sp2=tmp;
            }
           *sp=*(sp+1);
         }
                                     // rotate lines of sinogram
                                     // (move upper part of sinogram to bottom)
        if (rt > 0)
         { memcpy(buffer2, data+s*size_sinoslice, bytes2*sizeof(T));
           memcpy(data+bytes+s*size_sinoslice, buffer2, bytes2*sizeof(T));
           memcpy(data+s*size_sinoslice, buffer, bytes*sizeof(T));
         }
         else { memcpy(data+s*size_sinoslice,
                       data+abs(rt)*bins+s*size_sinoslice, bytes2*sizeof(T));
                memcpy(data+bytes2+s*size_sinoslice, buffer, bytes*sizeof(T));
              }
      }
     if (buffer2 != NULL) { delete[] buffer2;
                            buffer2=NULL;
                          }
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      if (buffer2 != NULL) delete[] buffer2;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Save data part of a matrix.
    \param[in] file          file handle
    \param[in] data          data part of matrix
    \param[in] matrix_size   number of elements in matrix
    \param[in] len           length of single element in file in bytes

    Save data part of a matrix. The data is converted from the format of this
    computer to the big-endian format of the ECAT7 file.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void savedata(std::ofstream * const file, T * const data,
              const unsigned long int matrix_size,
              const unsigned short int len)
 { if ((data == NULL) || (matrix_size == 0) || (len == 0)) return;
   DataChanger *dc=NULL;

   try
   { T *bp;
     unsigned long int i, end;
     unsigned short int vsize;

     vsize=E7_RECLEN/len;                      // number of elements per record
     end=matrix_size/vsize;                         // number of records needed
     if ((matrix_size % vsize) != 0) end++;
                       // DataChanger is used to read data system independently
     dc=new DataChanger(E7_RECLEN, false, true);
     for (bp=data,
          i=0; i < end; i++)                                   // write records
      { dc->ResetBufferPtr();
        for (unsigned short int j=0; j < vsize; j++)    // put data into buffer
         if (bp < data+matrix_size) dc->Value(*bp++);
          else break;
        dc->SaveBuffer(file);               // store data from buffer into file
      }
     delete dc;
     dc=NULL;
   }
   catch (...)
    { if (dc != NULL) delete dc;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Scale dataset.
    \param[in,out] data          data part of matrix
    \param[in]     matrix_size   number of elements in matrix
    \param[in]     factor        scale factor

    Scale dataset.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void scaledata(T * const data, const unsigned long int matrix_size,
               const float factor)
 { if ((data == NULL) || (factor == 1.0f)) return;
   for (unsigned long int i=0; i < matrix_size; i++)
    data[i]=(T)(data[i]*factor);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from view to volume mode.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Convert dataset from view to volume mode.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void view2volume(T * const data, const unsigned short int bins,
                 const unsigned short int angles,
                 const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *bp;
     unsigned short int s;
     unsigned long int size_bs, size;

     size_bs=(unsigned long int)slices*(unsigned long int)bins;
     size=size_bs*(unsigned long int)angles;
     buffer=new T[size];                                   // temporary dataset
                                             // rotate dataset about 90 degrees
     for (bp=buffer,
          s=0; s < slices; s++)
      { unsigned short int a;
        T *sp;

        for (sp=data+(unsigned long int)s*(unsigned long int)bins,
             a=0; a < angles; a++,
             sp+=size_bs,
             bp+=bins)
         memcpy(bp, sp, bins*sizeof(T));
      }
     memcpy(data, buffer, size*sizeof(T));
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert dataset from volume to view mode.
    \param[in,out] data     sinogram dataset
    \param[in]     bins     number of bins in a projection
    \param[in]     angles   number of angles in sinogram plane
    \param[in]     slices   number of planes in sinogram

    Convert dataset from volume to view mode.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void volume2view(T * const data, const unsigned short int bins,
                 const unsigned short int angles,
                 const unsigned short int slices)
 { if (data == NULL) return;
   T *buffer=NULL;

   try
   { T *sp;
     unsigned short int s;
     unsigned long int size_bs, size;

     size_bs=(unsigned long int)slices*(unsigned long int)bins;
     size=size_bs*(unsigned long int)angles;
     buffer=new T[size];                                   // temporary dataset
                                             // rotate dataset about 90 degrees
     for (sp=data,
          s=0; s < slices; s++)
      { unsigned short int a;
        T *bp;

        for (bp=buffer+(unsigned long int)s*(unsigned long int)bins,
             a=0; a < angles; a++,
             bp+=size_bs,
             sp+=bins)
         memcpy(bp, sp, bins*sizeof(T));
      }
     memcpy(data, buffer, size*sizeof(T));
     delete[] buffer;
     buffer=NULL;
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }
