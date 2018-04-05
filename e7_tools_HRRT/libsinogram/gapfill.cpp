/*! \class GapFill gapfill.h "gapfill.h"
    \brief This class provides methods to fill gaps in sinograms and smooth
           randoms.
    \author Frank Kehren (frank.kehren@siemens.com)
    \date 2003/11/17 initial version
    \date 2004/03/29 added model based gap filling
    \date 2004/04/08 reduce amount of logging messages
    \date 2004/09/10 mask border bins in norm

    This class provides two different methods to fill gaps in sinograms. The
    first method is normalization based. If the value of a bin in the
    normalization data is below a threshold, the bin is treated as a gap. The
    second method is gantry model based. Here the position of the gaps is
    calculated based on the knowledge of the gantry geometry.

    Once the gaps are found, they are filled by linear interpolation in
    azimuthal direction. To avoid noise artefacts, the interpolation is
    calculated based on a smoothed version of the sinogram (8-bin-boxcar).

    This 1d boxcar smoothing in azimuthal direction can also be used to
    smooth randoms sinograms. In this case a 32-bin-boxcar is used.

    To make the 1d filtering and linear interpolation in azimuthal direction
    across sinogram borders possible, the sinogram is resorted into a 1d
    vector. For a sinogram with RhoSamples=4 and ThetaSamples=6 this results
    in the following indexing:
    <center>
    <table>
    <tr><td> 0 </td><td> 12 </td><td> 18 </td><td>  6</tr>
    <tr><td> 1 </td><td> 13 </td><td> 19 </td><td>  7</tr>
    <tr><td> 2 </td><td> 14 </td><td> 20 </td><td>  8</tr>
    <tr><td> 3 </td><td> 15 </td><td> 21 </td><td>  9</tr>
    <tr><td> 4 </td><td> 16 </td><td> 22 </td><td> 10</tr>
    <tr><td> 5 </td><td> 17 </td><td> 23 </td><td> 11</tr>
    </table>
    </center>
 */

#include <ctime>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "gapfill.h"
#include "exception.h"
#include "logging.h"
#include "thread_wrapper.h"

#include "global_tmpl.h"
#include "raw_io.h"

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to calculate gap filling.
    \param[in] param pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to calculate gap filling.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_GapFill(void *param)
 { try
   { GapFill::tthread_params *tp;

     tp=(GapFill::tthread_params *)param;
     if (tp->norm == NULL)
      tp->object->calcGapFill(tp->sinogram, tp->slices);
      else tp->object->calcGapFill(tp->sinogram, tp->norm, tp->slices,
                                   tp->gap_fill_threshold, tp->mask_bins);
     return(NULL);
   }
   catch (const Exception r)
    { return(new Exception(r.errCode(), r.errStr()));
    }
   catch (...)
    { return(new Exception(REC_UNKNOWN_EXCEPTION,
                           "Unknown exception generated."));
    }
 }

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and calculate indices for 1d sinogram vector.
    \param[in] _RhoSamples     number of bins in projection
    \param[in] _ThetaSamples   number of angles in sinogram

    Initialize the object and calculate the indices for the 1d sinogram vector.
 */
/*---------------------------------------------------------------------------*/
GapFill::GapFill(const unsigned short int _RhoSamples,
                 const unsigned short int _ThetaSamples)
 { RhoSamples=_RhoSamples;
   ThetaSamples=_ThetaSamples;
   planesize=(unsigned long int)RhoSamples*(unsigned long int)ThetaSamples;
   calc_indices();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object and calculate mask for gaps.
    \param[in] _RhoSamples                     number of bins in projection
    \param[in] _ThetaSamples                   number of angles in sinogram
    \param[in] transaxial_crystals_per_block   number of crystals per block in
                                               transxial direction
    \param[in] transaxial_block_gaps           number of gap-crystals per block
                                               in transaxial direction
    \param[in] crystals_per_ring               number of crystals per ring
    \param[in] intrinsic_tilt                  intrinsic tilt of scanner

    Initialize the object and calculate a mask for the gaps, based on the
    knowledge of the scanner geometry. Calculate also the indices for the 1d
    sinogram vector.
 */
/*---------------------------------------------------------------------------*/
GapFill::GapFill(const unsigned short int _RhoSamples,
                 const unsigned short int _ThetaSamples,
                 const unsigned short int transaxial_crystals_per_block,
                 const unsigned short int transaxial_block_gaps,
                 const unsigned short int crystals_per_ring,
                 const float intrinsic_tilt)
 { signed short int angle;

   RhoSamples=_RhoSamples;
   ThetaSamples=_ThetaSamples;
   planesize=(unsigned long int)RhoSamples*(unsigned long int)ThetaSamples;
   angle=(signed short int)(-(float)ThetaSamples/180.0f*intrinsic_tilt);
   gap_mask.assign(planesize, false);
                                                   // run over all gap crystals
   for (unsigned short int i=0; i < crystals_per_ring; i++)
    if ((i % (transaxial_crystals_per_block+transaxial_block_gaps)) >=
        transaxial_crystals_per_block)
     { unsigned short int det;

       det=(i-angle+crystals_per_ring) % crystals_per_ring;
       for (unsigned short int r=0; r < RhoSamples; r++)
        { unsigned short int d1, d2, view;
          signed short int elem;
                                          // two crystals of this LOR are d1,d2
          d2=(r+det+ThetaSamples-RhoSamples/2) % crystals_per_ring;
          d1=std::min(det, d2);
          d2=std::max(det, d2);
                                       // calcutale position of bin in sinogram
          view=((d1+d2+ThetaSamples+1) % crystals_per_ring)/2;
          elem=abs(d2-d1-ThetaSamples);
          if ((d1 < view) || (d2 > view+ThetaSamples)) elem=-elem;
          elem=(elem+RhoSamples/2) % RhoSamples;
          gap_mask[view*(unsigned long int)RhoSamples+elem]=true;
        }
     }
   calc_indices();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate indices for 1d sinogram vector.

    Calculate the indices for the mapping of the sinogram into a 1d vector.
    The vector follows the row of the sinogram.
 */
/*---------------------------------------------------------------------------*/
void GapFill::calc_indices()
 { unsigned long int b=0, next_column=1;
   bool flag=false;

   m_index.resize(planesize);
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
/*! \brief Fill gaps of sinogram plane.
    \param[in,out] sino     sinogram dataset
    \param[in]     planes   number of planes in sinogram

    If the value of a bin in the gap_mask is true, the corresponding sinogram
    bin is treated as a gap and filled by linear interpolation in azimuthal
    direction.
    The code first converts the sinogram plane into a vector without gaps.
    The vector is smoothed with an 8-bin-boxcar filter and used to determine
    the weights for the linear interpolation used to fill the sinogram gaps.
 */
/*---------------------------------------------------------------------------*/
void GapFill::calcGapFill(float *sino, const unsigned short int planes) const
 { std::string str;
   std::vector <signed long int> m_i1, m_i2;
   std::vector <float> m_fltd;

   for (unsigned short int p=0; p < planes; p++,
        sino+=planesize)
    { unsigned long int num=0;

      m_i1.clear();
      m_i2.clear();
      m_fltd.clear();
                                     // fill sino voxels into m_fltd/m_i1 array
                                     // and store indices of gap voxels in m_i2
      for (unsigned long int tr=0; tr < planesize; tr++)
       if (!gap_mask[m_index[tr]]) { m_i1.push_back(tr);
                                     m_fltd.push_back(sino[m_index[tr]]);
                                   }
        else m_i2.push_back(tr);
                                                          // filter sino voxels
      smooth(&m_fltd[0], (unsigned long int)m_fltd.size(), 8);
                                // calculate gap voxels by linear interpolation
      for (unsigned long int a=0; a < m_i2.size(); a++)
       {                                   // search last sino voxel before gap
         bool found=false;

         for (;num+1 < m_i1.size(); num++)
          if(m_i1[num] > m_i2[a]) { found=true;
                                    break;
                                  }
//       if (!found) continue;
		 if (!found)
		 {
			 if (a ==  m_i2.size()-1)  // last pixel is a gap, use last value before gap.
			 {
				 num = m_i1.size()-1;
				 sino[m_index[m_i2[a]]]= m_fltd[num]; // use last non gap bin
			 }
			 else
			 {
				 throw Exception(REC_UNKNOWN_EXCEPTION,"GapFill Internal Error");
			 }
		  }
		 else
		 {

			 if (num > 0) num--;
																	 // interpolate
			 sino[m_index[m_i2[a]]]=(float)(m_i2[a]-m_i1[num])/
									(float)(m_i1[num+1]-m_i1[num])*
									(m_fltd[num+1]-m_fltd[num])+m_fltd[num];
		 }
       }
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill gaps of sinogram plane.
    \param[in,out] sino         sinogram dataset
    \param[in]     norm         normalization dataset
    \param[in]     planes       number of planes in sinogram
    \param[in]     threshold    threshold for normalization data to identify
                                gaps
    \param[in]     mask_bins    number of bins to ignore on each side of norm

    If the value of a bin in the normalization data is below a threshold, the
    corresponding sinogram bin is treated as a gap and filled by linear
    interpolation in azimuthal direction.
    The code first converts the sinogram plane into a vector without gaps.
    The vector is smoothed with an 8-bin-boxcar filter and used to determine
    the weights for the linear interpolation used to fill the sinogram gaps.
 */
/*---------------------------------------------------------------------------*/
void GapFill::calcGapFill(float * sino, const float * norm,
                          const unsigned short int planes,
                          const float threshold,
                          const unsigned short int mask_bins) const
 { std::string str;
   std::vector <signed long int> m_i1, m_i2;
   std::vector <float> m_fltd;

   try {for (unsigned short int p=0; p < planes; p++,
        sino+=planesize,
        norm+=planesize)
    { unsigned long int num=0, ptr=0;

      m_i1.clear();
      m_i2.clear();
      m_fltd.clear();
                                     // fill sino voxels into m_fltd/m_i1 array
                                     // and store indices of gap voxels in m_i2
      for (unsigned short int t=0; t < ThetaSamples; t++)
       for (unsigned short int r=0; r < RhoSamples; r++,
            ptr++)
        if ((norm[m_index[ptr]] > threshold) ||
            (r < mask_bins) || (r >= RhoSamples-mask_bins))
         { m_i1.push_back(ptr);
           m_fltd.push_back(sino[m_index[ptr]]);
         }
         else m_i2.push_back(ptr);
                                                          // filter sino voxels
      smooth(&m_fltd[0], (unsigned long int)m_fltd.size(), 8);
                                // calculate gap voxels by linear interpolation
      for (unsigned long int a=0; a < m_i2.size(); a++)
       {                                   // search last sino voxel before gap
         for (;num+1 < m_i1.size(); num++)
          if(m_i1[num] > m_i2[a]) break;
		 if (num > 0) { num--;
		 float df = (float)(m_i1[num+1]-m_i1[num]);
                                                                 // interpolate
         if (df>0) sino[m_index[m_i2[a]]]=(float)(m_i2[a]-m_i1[num])/
                                (float)(m_i1[num+1]-m_i1[num])*
								(m_fltd[num+1]-m_fltd[num])+m_fltd[num];
		 else sino[m_index[m_i2[a]]] = 0.0f; }
       }
    }
   } catch(...) { throw; }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Smooth randoms sinogram.
    \param[in,out] randoms   randoms sinogram

    Smooth randoms sinogram. The code converts the sinogram plane into a vector
    which is then smoothed by a 32-bin boxcar filter. The smoothed data is
    converted back into a sinogram.
 */
/*---------------------------------------------------------------------------*/
void GapFill::calcRandomsSmooth(float * const randoms) const
 { unsigned long int tr;
   std::vector <float> m_fltd;

   m_fltd.resize(planesize);
                                          // fill sino voxels into m_fltd array
   for (tr=0; tr < planesize; tr++)
    m_fltd[tr]=randoms[m_index[tr]];
   smooth(&m_fltd[0], planesize, 32);                     // filter sino voxels
                                      // put filtered voxels back into sinogram
   for (tr=0; tr < planesize; tr++)
    randoms[m_index[tr]]=m_fltd[tr];
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill gaps in sinogram based on the gantry geometry.
    \param[in,out] sinogram      sinogram data
    \param[in]     planes        number of planes in sinogram
    \param[in]     loglevel      level for logging
    \param[in]     max_threads   maximum number of threads to use

    Fill gaps in sinogram based on the gantry geometry. This method is
    multi-threaded. Every thread processed a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void GapFill::fillGaps(float * const sinogram, unsigned short int planes,
                       const unsigned short int loglevel,
                       const unsigned short int max_threads)
 { try
   { void *thread_result;
     unsigned short int threads=1, t;
     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;

     try
     { std::vector <tthread_params> tp;

       threads=std::min(max_threads, planes);
	   Logging::flog()->logMsg("fill #1 planes #2 threads", loglevel+1)->arg(planes)->arg(threads);
       if (threads == 1)
        { calcGapFill(sinogram, planes);
         return;
        }
       float *sp;
       unsigned short int i;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (sp=sinogram,
            t=threads,
            i=0; i < threads; i++,// distribute sinogram onto different threads
            t--)
        { tp[i].object=this;
          tp[i].sinogram=sp;
          tp[i].norm=NULL;
          tp[i].gap_fill_threshold=0.0f;
          tp[i].slices=planes/t;
          tp[i].thread_number=i;
          thread_running[i]=true;
                                                                // start thread
          ThreadCreate(&tid[i], &executeThread_GapFill, &tp[i]);
          planes-=tp[i].slices;
          sp+=(unsigned long int)tp[i].slices*planesize;
        }
       for (i=0; i < threads; i++)       // wait until all threads are finished
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
     }
     catch (...)
      { thread_result=NULL;
        for (t=0; t < thread_running.size(); t++)
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
                       // if we catch exceptions from the terminating threads,
                       // we store only the first one and ignore the others
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
                    // if the threads have thrown exceptions we throw the first
                    // one of them, not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Fill gaps in sinogram based on the normalization data.
    \param[in,out] sinogram             sinogram data
    \param[in]     norm                 normalization data
    \param[in]     planes               number of planes in sinogram
    \param[in]     gap_fill_threshold   threshold for normalization data to
                                        identify gaps
    \param[in]     mask_bins            number of bins to ignore on each side
                                        of norm
    \param[in]     loglevel             level for logging
    \param[in]     max_threads          maximum number of threads to use

    Fill gaps in sinogram based on the normalization data. This method is
    multi-threaded. Every thread processed a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void GapFill::fillGaps(float * const sinogram, float * const norm,
                       unsigned short int planes,
                       const float gap_fill_threshold,
                       const unsigned short int mask_bins,
                       const unsigned short int loglevel,
                       const unsigned short int max_threads)
 { try
   { void *thread_result;
     unsigned short int threads=1, t;
     std::vector <bool> thread_running(0);
     std::vector <tthread_id> tid;

     try
     { std::vector <tthread_params> tp;

       threads=std::min(max_threads, planes);
	   Logging::flog()->logMsg("fill #1 planes #2 threads", loglevel+1)->arg(planes)->arg(threads);
       if (threads == 1)
        { calcGapFill(sinogram, norm, planes, gap_fill_threshold, mask_bins);
          return;
        }
       float *sp, *np;
       unsigned short int i;

       tid.resize(threads);
       tp.resize(threads);
       thread_running.assign(threads, false);
       for (sp=sinogram,
            np=norm,
            t=threads,
            i=0; i < threads; i++,// distribute sinogram onto different threads
            t--)
        { tp[i].object=this;
          tp[i].sinogram=sp;
          tp[i].norm=np;
          tp[i].gap_fill_threshold=gap_fill_threshold;
          tp[i].mask_bins=mask_bins;
          tp[i].slices=planes/t;
          tp[i].thread_number=i;
          thread_running[i]=true;
                                                                // start thread
          ThreadCreate(&tid[i], &executeThread_GapFill, &tp[i]);
          planes-=tp[i].slices;
          sp+=(unsigned long int)tp[i].slices*planesize;
          np+=(unsigned long int)tp[i].slices*planesize;
        }
       for (i=0; i < threads; i++)       // wait until all threads are finished
        { ThreadJoin(tid[i], &thread_result);
          thread_running[i]=false;
          if (thread_result != NULL) throw (Exception *)thread_result;
        }
     }
     catch (...)
      { thread_result=NULL;
        for (t=0; t < thread_running.size(); t++)
                                      // thread_running is only true, if the
                                      // exception was not thrown by the thread
         if (thread_running[t])
          { void *tr;

            ThreadJoin(tid[t], &tr);
                       // if we catch exceptions from the terminating threads,
                       // we store only the first one and ignore the others
            if (thread_result == NULL)
             if (tr != NULL) thread_result=tr;
          }
                    // if the threads have thrown exceptions we throw the first
                    // one of them, not the one we are currently dealing with.
        if (thread_result != NULL) throw (Exception *)thread_result;
        throw;
      }
   }
   catch (const Exception *r)                 // handle exceptions from threads
    { std::string err_str;
      unsigned long int err_code;
                                    // move exception object from heap to stack
      err_code=r->errCode();
      err_str=r->errStr();
      delete r;
      throw Exception(err_code, err_str);                    // and throw again
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Smooth vector by boxcar filter.
    \param[in,out] data           vector to be filtered
    \param[in]     size           length of vector
    \param[in]     filter_width   width of filter in elements

    The vector is smoothed by a boxcar filter of the given width. At the
    beginning and end of the vector, the filter assumes that the missing
    vector elements are equal to the first or last existing element.
    The filter width is always an odd number or replaced by the next odd
    number.
 */
/*---------------------------------------------------------------------------*/
void GapFill::smooth(float * const data, const unsigned long int size,
                     unsigned long int filter_width) const
 { unsigned long int i;
   signed long int idx;
   std::vector <float> data_smooth;

   if ((filter_width & 0x1) == 0) filter_width++;
#ifdef __GNUG__
             // The GNU optimizer (at least version 3.2 and 3.2.1) has problems
             // with the min/max construct. Therefore we use this code:
   if (filter_width < 3) filter_width=3;
    else if (filter_width > size) filter_width=size;
#else
   filter_width=std::min(std::max((unsigned long int)3, filter_width), size);
#endif
   data_smooth.assign(size, 0.0f);
                                                                 // filter data
   for (idx=-(signed long int)filter_width/2,
        i=0; i < size; i++,
        idx++)
    { for (signed long int f=0; f < (signed long int)filter_width; f++)
#if __GNUG__
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
      data_smooth[i]/=(float)filter_width;
    }
   memcpy(data, &data_smooth[0], size*sizeof(float));
 }
