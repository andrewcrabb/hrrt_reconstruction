/*! \class RandomsSmoothing randoms_smoothing.h "randoms_smoothing.h"
    \brief This class implements the smoothing of a randoms sinogram.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/12/22 initial version
    \date 2003/12/31 added multi-threading
    \date 2004/09/13 added Doxygen style comments

     Smooth_randoms applies a variance reduction technique to a randoms
     sinogram following the techniques outlined in:

     RD Badawi, MA Lodge and PK Marsden: <I>Variance-Reduction Techniques for
     Random Coincidence Events in 3D PET</I> J. Nucl. Med. 39(5) Supp.:178P,
     1998

     and:

     ME Casey and EJ Hoffman: <I>Quantitation in Positron Emission
     Computed Tomography: 7. A technique to reduce noise in accidental
     coincidence measurements and coincidence efficiency calibration</I>
     J. Comput. Assist.Tomogr. 10, 845-850, 1986

     This technique first forms values proportional to the crystal singles by
     summing across the detector fans. Since there are a variable number of
     bins in each fan, a count of the number bins is kept as well.
     This calculation is multi-threaded, each thread calculates a set of
     crystal rings. The number of threads is limited by the constant
     max_threads. Each singles value is then normalized by dividing by the
     count. Finally, a new randoms sinogram is constructed from the products
     of the singles values. To insure consistancy, the new randoms sinogram is
     scaled so that it's total equals the total of the original randoms
     sinogram.
 */

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <limits>
#include "randoms_smoothing.h"
#include "e7_tools_const.h"
#include "exception.h"
#include "logging.h"
#include "thread_wrapper.h"
#include "vecmath.h"

/*- local functions ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to create smoothed sinogram.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object
    \exception REC_UNKNOWN_EXCEPTION unknown exception generated

    Start thread to create smoothed randoms sinogram.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_RS_createSino(void *param)
 { try
   { RandomsSmoothing::tthread_cs_params *tp;

     tp=(RandomsSmoothing::tthread_cs_params *)param;
     tp->object->createSmoothedSinogram(tp->first_plane, tp->last_plane,
                                        tp->sinogram, tp->RhoSamples,
                                        tp->ThetaSamples, tp->mash,
                                        tp->crystals_per_ring,
                                        tp->skipinterval, tp->singles,
                                        tp->epsilon, tp->thread_number,
                                        true);
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

/*---------------------------------------------------------------------------*/
/*! \brief Start thread to calculate total randoms rate.
    \param[in] param   pointer to thread parameters
    \return NULL or pointer to Exception object

    Start thread to calculate the total randoms rate.
    The thread API is a C-API so C linkage and calling conventions have to be
    used, when creating a thread. To use a method as thread, a helper function
    in C is needed, that calls the method.
 */
/*---------------------------------------------------------------------------*/
void *executeThread_RS_total(void *param)
 { try
   { RandomsSmoothing::tthread_rs_params *tp;

     tp=(RandomsSmoothing::tthread_rs_params *)param;
     tp->object->calculateTotalRandomsRate(tp->first_ring, tp->last_ring,
                                           tp->sinogram, tp->RhoSamples,
                                           tp->ThetaSamples, tp->mash,
                                           tp->crystals_per_ring,
                                           tp->skipinterval, tp->singles,
                                           tp->epsilon, tp->cnt,
                                           tp->thread_number, true);
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
/*! \brief Initialize object and calculate LUT for plane numbers of all ring
           pairs.
    \param[in] axis_slices      number of planes for axes of 3d sinogram
    \param[in] _crystal_rings   number of crystal rings in gantry
    \param[in] span             span of sinogram

    Initialize object and calculate LUT for plane numbers of all ring pairs.
 */
/*---------------------------------------------------------------------------*/
RandomsSmoothing::RandomsSmoothing(
                            const std::vector <unsigned short int> axis_slices,
                            const unsigned short int _crystal_rings,
                            const unsigned short int span)
 { total_sem=NULL;
   try
   { unsigned short int size, i, j, offset;
     std::vector <unsigned short int> ringa_ringb, lor_index;
                                  // calculate plane numbers for all ring pairs
     crystal_rings=_crystal_rings;
     size=crystal_rings*crystal_rings;


     plane_numbers.assign(size, -1);
     ringa_ringb.resize(size);
     for (j=0; j < crystal_rings; j++)
      for (i=0; i < crystal_rings; i++)
       ringa_ringb[j*crystal_rings+i]=i+j;
     lor_index.resize(size);
     offset=0;
     for (unsigned short int segment=0;
          segment < axis_slices.size()*2-1;
          segment++)
      { signed short int pn_segment;
        unsigned short int count, minv;

        if (segment & 0x1) pn_segment=(segment+1)/2;
         else pn_segment=-(segment+1)/2;
                                                      // calculate index of LOR
        count=0;
        for (j=0; j < crystal_rings; j++)
         for (i=0; i < crystal_rings; i++)
          if (abs(i-j-pn_segment*span) < (span+1)/2)
           lor_index[count++]=j*crystal_rings+i;
                                                              // search minimum
        minv=std::numeric_limits <unsigned short int>::max();
        for (i=0; i < count; i++)
         minv=std::min(ringa_ringb[lor_index[i]], minv);
                                                     // calculate plane numbers
        for (i=0; i < count; i++)
         plane_numbers[lor_index[i]]=ringa_ringb[lor_index[i]]-minv+offset;
        if (segment == 0) offset+=axis_slices[0];
         else offset+=axis_slices[(segment+1)/2]/2;
      }
     total_sem=new Semaphore(1);
   }
   catch (...)
    { if (total_sem != NULL) delete total_sem;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
RandomsSmoothing::~RandomsSmoothing()
 { if (total_sem != NULL) delete total_sem;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Calculate total randoms rate for each crystal.
    \param[in]     first_ring          number of first detector ring
    \param[in]     last_ring           number of last detector ring
    \param[in]     sinogram            randoms sinogram
    \param[in]     RhoSamples          number of bins in projections
    \param[in]     ThetaSamples        number of angles in sinograms
    \param[in]     mash                mash factor of sinogram
    \param[in]     crystals_per_ring   number of crystals per ring
    \param[in]     skipinterval        number of crystal to skip because of gap
    \param[in,out] singles             number of counts in a crystal
    \param[in,out] epsilon             randoms sensitivity in a crystal
    \param[in,out] cnt                 number of LORs contributing to each
                                       crystal
    \param[in]     thread_num          number of thread
    \param[in]     threaded            method called as a thread ?

    Calculate total randoms rate for each crystal. Find for each sinogram bin
    the physical LORs (crystal pairs) that are contributing to this bin. Then
    add the sinogram value to the singles value of each crystal. Count also
    to how many sinogram LORs a crystal is contributing.
    This method processes a range of crystal rings, so that different threads
    can calculate different ring-ranges.
 */
/*---------------------------------------------------------------------------*/
void RandomsSmoothing::calculateTotalRandomsRate(
                                  const unsigned short int first_ring,
                                  const unsigned short int last_ring,
                                  float * const sinogram,
                                  const unsigned short int RhoSamples,
                                  const unsigned short int ThetaSamples,
                                  const unsigned short int mash,
                                  const unsigned short int crystals_per_ring,
                                  const unsigned short int skipinterval,
                                  std::vector <float> * const singles,
                                  std::vector <float> * const epsilon,
                                  std::vector <unsigned short int> * const cnt,
                                  const unsigned short int thread_num,
                                  const bool threaded)
 { unsigned long int sino_planesize, crystals;
   std::string str;
   std::vector <float> singles_t;
   std::vector <unsigned short int> cnt_t;

   short int iA,iB,ii,iq,ik;
   std::vector <float> epsilon_A_i, eta_AB_ij;
   std::vector <float> meanB_eta_AB_i, meanAB_eta_AB, meanA_epsilon_A;

   if (threaded) str="thread "+toString(thread_num+1)+": ";
    else str=std::string();
   Logging::flog()->logMsg(str+"sum coincidences between rings #1-#2 and all "
                           "rings", 5)->arg(first_ring)->arg(last_ring);
                                                      // size of sinogram plane
   sino_planesize=(unsigned long int)RhoSamples*
                  (unsigned long int)ThetaSamples;
                                            // number of crystals in the gantry
   crystals=(unsigned long int)crystal_rings*
            (unsigned long int)crystals_per_ring;
                                           // allocate local buffers for totals
   singles_t.assign(crystals, 0.0f);
   cnt_t.assign(crystals, 0);

   if(number_groups>1){
     epsilon_A_i.assign(crystals, 0.0f);
     eta_AB_ij.resize(crystals_per_ring*crystals_per_ring);
     meanB_eta_AB_i.resize(number_groups*crystals_per_ring);
     meanAB_eta_AB.resize(number_groups*number_groups);
     meanA_epsilon_A.resize(number_groups);
   }

                                              // count randoms for each crystal
   for (unsigned short int ring_a=first_ring; ring_a <= last_ring; ring_a++)
    if ((ring_a+1) % skipinterval > 0)                 // ignore gaps in ring a
     { float *saptr, *eaptr;
       unsigned short int *captr;
       if(number_groups>1){
        eta_AB_ij.assign(crystals_per_ring*crystals_per_ring, 0.0f);
        meanB_eta_AB_i.assign(number_groups*crystals_per_ring, 0.0f);
        meanAB_eta_AB.assign(number_groups*number_groups, 0.0f);
        meanA_epsilon_A.assign(number_groups, 0.0f);
       }

       unsigned long int indx_a;
       indx_a = (unsigned long int)ring_a*
                (unsigned long int)crystals_per_ring;
       eaptr=&epsilon_A_i[indx_a];
       saptr=&singles_t[indx_a];
       captr=&cnt_t[indx_a];

       for (unsigned short int ring_b=0; ring_b < crystal_rings; ring_b++)
        if ((ring_b+1) % skipinterval > 0)             // ignore gaps in ring b
         if (plane_numbers[ring_b*crystal_rings+ring_a] > -1)
          { float *sp, *sbptr, *ebptr;
            unsigned short int *cbptr;
                               // pointer to sinogram plane into which physical
                               // LORs that connect ring a with ring b belong
            sp=&sinogram[(unsigned long int)
                         plane_numbers[ring_b*crystal_rings+ring_a]*
                         sino_planesize];
            unsigned long int indx_b;
            indx_b=(unsigned long int)ring_b*
                   (unsigned long int)crystals_per_ring;
            sbptr=&singles_t[indx_b];
            ebptr=&epsilon_A_i[indx_b];
            cbptr=&cnt_t[indx_b];

            for (unsigned short int t=0; t < ThetaSamples; t++)
             for (signed short int r=-RhoSamples/2; r < RhoSamples/2; r++,
                  sp++)
              for (unsigned short int tm=t*mash; tm < (t+1)*mash; tm++)
               { unsigned long int crys_a, crys_b;

                                           // which two crystals (physical LOR)
                                           // are making up the sinogram LOR ?
                 crys_a=(tm+(r >> 1)+crystals_per_ring) % crystals_per_ring;
                 if ((crys_a+1) % skipinterval == 0) continue;    // ignore gap
                 crys_b=(tm-((r+1) >> 1)+crystals_per_ring+
                         ThetaSamples*mash) % crystals_per_ring;
                 if ((crys_b+1) % skipinterval == 0) continue;    // ignore gap
                                         // add to singles rates and count LORs
                 saptr[crys_a]+=*sp;
                 captr[crys_a]++;
                 sbptr[crys_b]+=*sp;
                 cbptr[crys_b]++;
                 iA=crys_a/crystal_in_each_group;
                 iB=crys_b/crystal_in_each_group;
                 ii=crys_a%crystal_in_each_group;
                 meanB_eta_AB_i[(iA*number_groups+iB)*
                                crystal_in_each_group+ii]
                              += *sp;
                 meanB_eta_AB_i[(iB*number_groups+iA)*
                                crystal_in_each_group+ii]
                              += *sp;
               }
              if(number_groups==1) continue;

              float	*sum_ptr, *idx_ptr, temp0, temp1, temp2;	

      				for(iA=0;iA<number_groups;iA++) for(iB=0;iB<number_groups;iB++){
      					sum_ptr=&meanAB_eta_AB[iA*number_groups+iB];
      					idx_ptr=&meanB_eta_AB_i[(iA*number_groups+iB)
                                        *crystal_in_each_group];
      					for(ii=0;ii<crystal_in_each_group;ii++,idx_ptr++)
                 *sum_ptr += *idx_ptr;
      					//*sum_ptr /= crystal_in_each_group;
      				}
      
      				for(iA=0;iA<number_groups;iA++){
               temp0=1.0f;
               for(ik=0;ik<number_groups/2-1;ik++){
                temp1=meanAB_eta_AB[(iA+ik+1)%number_groups*number_groups
                                    +(iA+ik+number_groups/2)%number_groups];
                if(temp1 > 1e-9)
                 temp0 *= meanAB_eta_AB[(iA+ik)%number_groups*number_groups
                              +(iA+ik+number_groups/2)%number_groups]/temp1;
                else {temp0 = 0.0f; flag_diamond_method=false;}
               }
               temp0 *= meanAB_eta_AB[iA*number_groups+(iA+number_groups/2)
                                     %number_groups];
               meanA_epsilon_A[iA] += (float)sqrt(temp0);
              }
      
      				for(iA=0;iA<number_groups;iA++)
               for(ii=0;ii<crystal_in_each_group;ii++){
      					for (iq=-1;iq<2;iq++){// \sum_{q=-1}^{1}; 
      						temp1=meanA_epsilon_A[(iA+number_groups/2+iq)%number_groups];
      						if(temp1 > 1e-9){
                    temp2=meanB_eta_AB_i[(iA*number_groups+
                                          (iA+number_groups/2+iq)%
                                          number_groups)*
                                         crystal_in_each_group+ii]/temp1;
      							eaptr[iA*crystal_in_each_group+ii]+=temp2;
      							ebptr[iA*crystal_in_each_group+ii]+=temp2;
      						} else 
      							flag_diamond_method=false;
      					}
      				}
              // end of diamonds computation
          }
     }
   if (threaded) str="thread "+toString(thread_num+1)+": ";
    else str=std::string();
   Logging::flog()->logMsg(str+"add sum to total randoms rate", 5);
                               // add randoms rates to buffers of parent thread
   total_sem->wait();
   vecAdd(&(*singles)[0], &singles_t[0], &(*singles)[0], crystals);
   if(number_groups>1)
    vecAdd(&(*epsilon)[0], &epsilon_A_i[0], &(*epsilon)[0], crystals);
   for (unsigned short int i=0; i < crystals; i++)
    (*cnt)[i]+=cnt_t[i];
   total_sem->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create smoothed sinogram from total randoms rate.
    \param[in]     first_plane         number of first sinogram plane
    \param[in]     last_plane          number of last sinogram plane
    \param[in,out] sinogram            randoms sinogram
    \param[in]     RhoSamples          number of bins in projections
    \param[in]     ThetaSamples        number of angles in sinograms
    \param[in]     mash                mash factor of sinogram
    \param[in]     crystals_per_ring   number of crystals per ring
    \param[in]     skipinterval        number of crystal to skip because of gap
    \param[in]     singles             number of counts in a crystal
    \param[in]     epsilon             randoms sensitivity in a crystal
    \param[in]     thread_num          number of thread
    \param[in]     threaded            method called as a thread ?

    Create smoothed sinogram from total randoms rate.
    This method processes a range of sinogram planes, so that different threads
    can calculate different plane-ranges.
 */
/*---------------------------------------------------------------------------*/
void RandomsSmoothing::createSmoothedSinogram(
                                    const unsigned short int first_plane,
                                    const unsigned short int last_plane,
                                    float * const sinogram,
                                    const unsigned short int RhoSamples,
                                    const unsigned short int ThetaSamples,
                                    const unsigned short int mash,
                                    const unsigned short int crystals_per_ring,
                                    const unsigned short int skipinterval,
                                    std::vector <float> * const singles,
                                    std::vector <float> * const epsilon,
                                    const unsigned short int thread_num,
                                    const bool threaded) const
 { unsigned long int sino_planesize;
   std::string str;
   std::vector <float> * epsilon_array;

   if (threaded) str="thread "+toString(thread_num+1)+": ";
    else str=std::string();
   Logging::flog()->logMsg(str+"create smoothed randoms sinogram planes #1-#2",
                           5)->arg(first_plane)->arg(last_plane);
                                                      // size of sinogram plane

   Logging::flog()->logMsg("flag_diamond_method #1",
                            5)->arg(flag_diamond_method);

   sino_planesize=(unsigned long int)RhoSamples*
                  (unsigned long int)ThetaSamples;

   if (!flag_diamond_method || number_groups == 1) 
    epsilon_array=(singles);
   else 
    epsilon_array=(epsilon);

   for (unsigned short int ring_a=0; ring_a < crystal_rings; ring_a++)
     if ((ring_a+1) % skipinterval > 0)                // ignore gaps in ring a
     { float *eaptr;

       unsigned long int indx_a;
       indx_a = (unsigned long int)ring_a*
                (unsigned long int)crystals_per_ring;

       eaptr=&(*epsilon_array)[indx_a];

       for (unsigned short int ring_b=0; ring_b < crystal_rings; ring_b++)
         if ((ring_b+1) % skipinterval > 0)            // ignore gaps in ring b
         { signed short int p;

           p=plane_numbers[ring_b*crystal_rings+ring_a];
           if ((p >= first_plane) && (p <= last_plane))
            { float *sp, *ebptr;
              unsigned long int indx_b;
              indx_b=(unsigned long int)ring_b*
                     (unsigned long int)crystals_per_ring;
              ebptr=&(*epsilon_array)[indx_b];
                               // pointer to sinogram plane into which physical
                               // LORs that connect ring a with ring b belong
              sp=&sinogram[(unsigned long int)p*sino_planesize];
              for (unsigned short int t=0; t < ThetaSamples; t++)
               for (signed short int r=-RhoSamples/2; r < RhoSamples/2; r++,
                    sp++)
                for (unsigned short int tm=t*mash; tm < (t+1)*mash; tm++)
                 { unsigned long int crys_a, crys_b;
                                           // which two crystals (physical LOR)
                                           // are making up the sinogram LOR ?
                   crys_a=(tm+(r >> 1)+crystals_per_ring) %
                          crystals_per_ring;
                   if ((crys_a+1) % skipinterval == 0) continue; // ignore gaps
                   crys_b=(tm-((r+1) >> 1)+crystals_per_ring+
                          ThetaSamples*mash) % crystals_per_ring;
                   if ((crys_b+1) % skipinterval == 0) continue; // ignore gaps

                   *sp+=eaptr[crys_a]*ebptr[crys_b];
                 }
            }
         }
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Smooth randoms sinogram.
    \param[in,out] sinogram            randoms sinogram
    \param[in]     RhoSamples          number of bins in projections
    \param[in]     ThetaSamples        number of angles in sinograms
    \param[in]     axes_slices         number of planes in 3d sinogram
    \param[in]     mash                mash factor of sinogram
    \param[in]     crystals_per_ring   number of crystals per ring
    \param[in]     skipinterval        number of crystal to skip because of gap
    \param[in]     max_threads         maximum number of threads to use

    Smooth randoms sinogram. The method calculates first the average randoms
    rates for each crystal and then creates a sinogram from these averages.
    The calculation of the average randoms rates and the creation of the
    sinogram are multi-threaded. Each thread calculates a range of crystal
    rings, respectively a range of sinogram planes.
 */
/*---------------------------------------------------------------------------*/
void RandomsSmoothing::smooth(float * const sinogram,
                              const unsigned short int RhoSamples,
                              const unsigned short int ThetaSamples,
                              const unsigned short int axes_slices,
                              const unsigned short int mash,
                              const unsigned short int crystals_per_ring,
                              const unsigned short int skipinterval,
                              const unsigned short int max_threads)
 { try
   { unsigned short int threads=0, t;
     void *thread_result;
     std::vector <tthread_id> tid;
     std::vector <bool> thread_running(0);

     try
     { unsigned long int crystals, sino_planesize, i, plane;
       float *sp;
       std::vector <float> singles, epsilon, oldtot;
       std::vector <unsigned short int> cnt;
       std::vector <tthread_rs_params> tp;
       std::vector <tthread_cs_params> tps;

       sino_planesize=(unsigned long int)RhoSamples*
                      (unsigned long int)ThetaSamples;
       crystals=(unsigned long int)crystal_rings*
                (unsigned long int)crystals_per_ring;
                                // find the total randoms rate for each crystal

       flag_diamond_method=true;
       number_groups=1;
       if     (crystals_per_ring%16==0)  number_groups=16;
       else if(crystals_per_ring%12==0)  number_groups=12;
       else                              flag_diamond_method=false;

       crystal_in_each_group=crystals_per_ring/number_groups;

       Logging::flog()->logMsg("number of groups #1", 4)->arg(number_groups);
       Logging::flog()->logMsg("number of crystals in each group #1",
                                4)->arg(crystal_in_each_group);

       singles.assign(crystals, 0.0f);
       cnt.assign(crystals, 0);
       //epsilon.assign(crystals*crystal_rings, 0.0f);
       epsilon.assign(crystals, 0.0f);
       threads=std::min(max_threads, crystal_rings);
       if (threads == 1)
        calculateTotalRandomsRate(0, crystal_rings-1, sinogram, RhoSamples,
                                  ThetaSamples, mash, crystals_per_ring,
                                  skipinterval, &singles, &epsilon, &cnt,
                                  1, false);
        else { unsigned short int rings;

               tid.resize(threads);
               tp.resize(threads);
               thread_running.assign(threads, false);
               rings=crystal_rings;
               for (t=threads,
                    i=0; i < threads; i++,
                    t--)
                { tp[i].object=this;
                  if (i == 0) tp[i].first_ring=0;
                   else tp[i].first_ring=tp[i-1].last_ring+1;
                  tp[i].last_ring=tp[i].first_ring+rings/t-1;
                  tp[i].RhoSamples=RhoSamples;
                  tp[i].ThetaSamples=ThetaSamples;
                  tp[i].mash=mash;
                  tp[i].crystals_per_ring=crystals_per_ring;
                  tp[i].skipinterval=skipinterval;
                  tp[i].cnt=&cnt;
                  tp[i].sinogram=sinogram;
                  tp[i].singles=&singles;
                  tp[i].epsilon=&epsilon;
                  tp[i].thread_number=i;
                  thread_running[i]=true;
                                                               // create thread
                  Logging::flog()->logMsg("start thread #1 to calculate total "
                                          "randoms rate for #2 rings", 4)->
                   arg(i+1)->arg(tp[i].last_ring-tp[i].first_ring+1);
                  ThreadCreate(&tid[i], &executeThread_RS_total, &tp[i]);
                  rings-=tp[i].last_ring-tp[i].first_ring+1;
                }
                                              // wait for all threads to finish
               for (i=0; i < threads; i++)
                { ThreadJoin(tid[i], &thread_result);
                  thread_running[i]=false;
                  Logging::flog()->logMsg("thread #1 finished", 4)->arg(i+1);
                  if (thread_result != NULL) throw (Exception *)thread_result;
                }
             }
                             // calculate average randoms rate for each crystal
       for (i=0; i < crystals; i++)
        if (cnt[i] > 0) singles[i]/=(float)cnt[i];
                                          // find total for each sinogram plane
       oldtot.resize(axes_slices);
       for (plane=0; plane < axes_slices; plane++)
        oldtot[plane]=vecScalarAdd(&sinogram[(unsigned long int)plane*
                                             sino_planesize],
                                   sino_planesize);
                                          // reconstruct sinogram from averages
       memset(sinogram, 0,
              (unsigned long int)axes_slices*sino_planesize*sizeof(float));
       if (threads == 1)
        createSmoothedSinogram(0, axes_slices-1, sinogram, RhoSamples,
                               ThetaSamples, mash, crystals_per_ring,
                               skipinterval, &singles, &epsilon, 1, false);
        else { unsigned short int planes;

               tid.resize(threads);
               tps.resize(threads);
               thread_running.assign(threads, false);
               planes=axes_slices;
               for (t=threads,
                    i=0; i < threads; i++,
                    t--)
                { tps[i].object=this;
                  if (i == 0) tps[i].first_plane=0;
                   else tps[i].first_plane=tps[i-1].last_plane+1;
                  tps[i].last_plane=tps[i].first_plane+planes/t-1;
                  tps[i].RhoSamples=RhoSamples;
                  tps[i].ThetaSamples=ThetaSamples;
                  tps[i].mash=mash;
                  tps[i].crystals_per_ring=crystals_per_ring;
                  tps[i].skipinterval=skipinterval;
                  tps[i].sinogram=sinogram;
                  tps[i].singles=&singles;
                  tps[i].epsilon=&epsilon;
                  tps[i].thread_number=i;
                  thread_running[i]=true;
                                                               // create thread
                  Logging::flog()->logMsg("start thread #1 to calculate planes"
                                          " #1-#2 of smoothed randoms "
                                          "sinogram", 4)->arg(i+1)->
                   arg(tps[i].first_plane)->arg(tps[i].last_plane);
                  ThreadCreate(&tid[i], &executeThread_RS_createSino, &tps[i]);
                  planes-=tps[i].last_plane-tps[i].first_plane+1;
                }
                                              // wait for all threads to finish
               for (i=0; i < threads; i++)
                { ThreadJoin(tid[i], &thread_result);
                  thread_running[i]=false;
                  Logging::flog()->logMsg("thread #1 finished", 4)->arg(i+1);
                  if (thread_result != NULL) throw (Exception *)thread_result;
                }
             }
                       // normalize so that old and new sinogram total the same
       sp=sinogram;
       for (plane=0; plane < axes_slices; plane++,
            sp+=sino_planesize)
        { float newtot;
                                       // calculate total of new sinogram plane
          newtot=vecScalarAdd(sp, sino_planesize);
                                                    // scale new sinogram plane
          if (newtot > 0.0f) newtot=oldtot[plane]/newtot;
          vecMulScalar(sp, newtot, sp, sino_planesize);
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
