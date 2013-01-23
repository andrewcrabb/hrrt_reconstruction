//** file: gapfill.cpp
//** date: 2002/11/04

//** author: Ziad Burbar
//** © CPS Innovations

/*- code design ---------------------------------------------------------------

The sinogram is resorted into a 1d vector to allow 1d filtering and
interpolation in theta direction across the sinogram borders.
For a sinogram with RhoSamples=4 and ThetaSamples=6 this results in the
following indexing:

 0 12 18  6
 1 13 19  7
 2 14 20  8
 3 15 21  9
 4 16 22 10
 5 17 23 11

The gaps are determined by a threshold in the normalization dataset. The
non-gap data is boxcar-filtered and the gaps are filled by linear 
interpolation.

-----------------------------------------------------------------------------*/

#include <time.h>
#include <iostream>
#include <algorithm>
#include <string.h>
#include "gapfill.h"
#ifdef WIN32
#include "global_tmpl.h"
#endif

/*---------------------------------------------------------------------------*/
/* GapFill: initialize object                                                */
/*  _RhoSamples     number of bins in projection                             */
/*  _ThetaSamples   number of angle in sinogram                              */
/*  threshold       gap-threshold for normalization data                     */
/*---------------------------------------------------------------------------*/
GapFill::GapFill(const unsigned short int _RhoSamples,
                 const unsigned short int _ThetaSamples,
                 float _threshold)
 { m_index=NULL;
   try
   { RhoSamples=_RhoSamples;
     ThetaSamples=_ThetaSamples;
     threshold=_threshold;
     planesize=(unsigned long int)RhoSamples*(unsigned long int)ThetaSamples;
     m_index=new unsigned long int[planesize];
     calc_indices();
   }
   catch (...)
    { if (m_index != NULL) { delete[] m_index;
                             m_index=NULL;
                           }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* ~GapFill: destroy object                                                  */
/*---------------------------------------------------------------------------*/
GapFill::~GapFill()
 { if (m_index != NULL) delete[] m_index;
 }

/*---------------------------------------------------------------------------*/
/* calc_indices: calculate the indices for the mapping of the sinogram into  */
/*               a 1d vector                                                 */
/*---------------------------------------------------------------------------*/
void GapFill::calc_indices() const
 { unsigned long int b=0, next_column=1;
   bool flag=false;

   for (unsigned long int tr=0; tr < planesize; tr++)
    { if (b >= planesize)
       { if (flag) b=next_column++;
          else b=RhoSamples-next_column;
         flag=!flag;
       }
      m_index[tr]=b;
      b+=RhoSamples;
    }
 }

/*---------------------------------------------------------------------------*/
/* calcGapFill: calculate GAP filling                                        */
/*  norm   normalization dataset                                             */
/*  sino   sinogram dataset                                                  */
/*---------------------------------------------------------------------------*/
void GapFill::calcGapFill(const float * const norm, float * const sino) const
 { signed long int *m_i1=NULL, *m_i2=NULL;
   float *m_fltd=NULL;

   try
   { unsigned long int i1size=0, i2size=0;

     m_i1=new signed long int[planesize];
     m_i2=new signed long int[planesize];
     m_fltd=new float[planesize];
                                     // fill sino voxels into m_fltd/m_i1 array
                                     // and store indices of gap voxels in m_i2
     for (unsigned long int tr=0; tr < planesize; tr++)
      if (norm[m_index[tr]] > threshold)
       { m_i1[i1size]=tr;
         m_fltd[i1size++]=sino[m_index[tr]];
       }
       else m_i2[i2size++]=tr;
     m_fltd[i1size]=0;
     m_i1[i1size]=0;
     m_i2[i2size]=0;

     smooth(m_fltd, i1size, 8);                           // filter sino voxels
     unsigned long int num=0;
                                // calculate gap voxels by linear interpolation
     for (unsigned long int a=0; a < i2size; a++)
      {                                    // search last sino voxel before gap
        for(;num < i1size; num++)
         if(m_i1[num] > m_i2[a]) break;
        if (num > 0) num--;
                                                                 // interpolate
        sino[m_index[m_i2[a]]]=(float)(m_i2[a]-m_i1[num])/
                               (float)(m_i1[num+1]-m_i1[num])*
                               (m_fltd[num+1]-m_fltd[num])+m_fltd[num];
      }
     delete[] m_fltd;
     m_fltd=NULL;
     delete[] m_i2;
     m_i2=NULL;
     delete[] m_i1;
     m_i1=NULL;
   }
   catch (...)
    { if (m_fltd != NULL) delete[] m_fltd;
      if (m_i2 != NULL) delete[] m_i2;
      if (m_i1 != NULL) delete[] m_i1;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* calcGapFill: calculate GAP filling                                        */
/*  norm   normalization dataset                                             */
/*  sino   sinogram dataset                                                  */
/*---------------------------------------------------------------------------*/
void GapFill::calcRandomsSmooth(float * const randoms) const
 { float *m_fltd=NULL;

   try
   { unsigned long int tr;

     m_fltd=new float[planesize];
                                          // fill sino voxels into m_fltd array
     for (tr=0; tr < planesize; tr++)
      m_fltd[tr]=randoms[m_index[tr]];
     smooth(m_fltd, planesize, 32);                       // filter sino voxels
                                      // put filtered voxels back into sinogram
     for (tr=0; tr < planesize; tr++)
      randoms[m_index[tr]]=m_fltd[tr];
     delete[] m_fltd;
     m_fltd=NULL;
   }
   catch (...)
    { if (m_fltd != NULL) delete[] m_fltd;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* smooth: 1d boxcar smoothing filter                                        */
/*         (edge values are copied to filter edges)                          */
/*  data           data to be filtered                                       */
/*  size           length of data                                            */
/*  filter_width   width of filter                                           */
/*---------------------------------------------------------------------------*/
void GapFill::smooth(float * const data, const unsigned long int size,
                     unsigned long int filter_width) const
 { float *data_smooth=NULL;

   try
   { unsigned long int i;

     if ((filter_width & 0x1) == 0) filter_width++;
#ifdef WIN32
     filter_width=mini(maxi((unsigned long int)3, filter_width), size);
#else
  #ifdef __GNU__
               // The GNU optimizer (at least version 3.2 and 3.2.1) has problems
               // with the min/max construct. Therefore we use this code:
       if (filter_width < 3) filter_width=3;
        else if (filter_width > size) filter_width=size;
  #else
       filter_width=std::min(std::max((unsigned long int)3, filter_width),
                             size);
  #endif
#endif
     data_smooth=new float[size];
     memset(data_smooth, 0, size*sizeof(float));
     { signed long int idx;
                                                                 // filter data
       for (idx=-(signed long int)filter_width/2,
            i=0; i < size; i++,
            idx++)
        { for (signed long int f=0; f < (signed long int)filter_width; f++)
#ifdef WIN32
           data_smooth[i]+=data[mini(maxi((signed long int)0, idx+f),
                                     (signed long int)(size-1))];
#else
  #ifdef __GNU__
               // The GNU optimizer (at least version 3.2 and 3.2.1) has problems
               // with the min/max construct. Therefore we use this code:
             if (idx+f < 0) data_smooth[i]+=data[0];
              else if (idx+f > (signed long int)(size-1))
                    data_smooth[i]+=data[size-1];
                    else data_smooth[i]+=data[idx+f];
  #else
             data_smooth[i]+=data[std::min(std::max((signed long int)0, idx+f),
                                                     (signed long int)(size-1))];
  #endif
#endif
          data_smooth[i]/=(float)filter_width;
        }
     }
     memcpy(data, data_smooth, size*sizeof(float));
     delete[] data_smooth;
     data_smooth=NULL;
   }
   catch (...)
    { if (data_smooth != NULL) delete[] data_smooth;
      throw;
    }
 }
