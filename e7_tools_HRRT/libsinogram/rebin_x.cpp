/*! \class RebinX rebin_x.h "rebin_x.h"
    \brief This template implements a rebinning in x-direction by linear
           interpolation.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2005/02/08 added Doxygen style comments
    \date 2005/02/25 use vectors

    This template implements a rebinning in x-direction by linear
    interpolation. The weights for the linear interpolation are stored in a
    look-up table to speed-up the calculation. There are different methods to
    handle the edges of a plane: don't handle the edges (EDGE_NONE), set the
    edges to zero (EDGE_ZERO) and truncate the edges and replace them by the
    neighbouring values (EDGE_TRUNCATE). The algorithm can calculate an
    arc-correction (arc sampling to uniform sampling or uniform sampling to
    arc sampling) at the same time.
 */

#include <iostream>
#include <cstring>
#include <algorithm>
#include "rebin_x.h"
#include "fastmath.h"
#include "vecmath.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize tables for rebinning algorithm.
    \param[in] _width              width of slice
    \param[in] _height             height of slice
    \param[in] _new_width          width of slice after rebinning
    \param[in] arc_correction      kind of arc correction
    \param[in] crystals_per_ring   number of crystals per ring
    \param[in] xc_in               geometric center of the slice in x-direction
    \param[in] xc_out              geometric center of the slice in x-direction
                                   after rebinning
    \param[in] zoom_factor         zoom factor
    \param[in] linear              linear interpolation (otherwise sampling)
    \param[in] values              preserve values (otherwise preserve counts)
    \param[in] _edge_handling      handling of data points near the edge

    Initialize tables for rebinning algorithm.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
RebinX <T>::RebinX(const unsigned short int _width,
                   const unsigned short int _height,
                   const unsigned short int _new_width,
                   const tarc_correction arc_correction,
                   const unsigned short int crystals_per_ring,
                   const T xc_in, const T xc_out, const T zoom_factor,
                   const bool linear, const bool values,
                   const tedge_handling _edge_handling)
 { RebinX <T> *rb=NULL;

   try
   { double *left_edge_source, *right_edge_source;
     unsigned short int i;
     std::vector <double> source_edge_positions;
     std::vector <T> mock_in, mock_out;

     width=_width;
     height=_height;
     new_width=_new_width;
     edge_handling=_edge_handling;
     old_slice_size=(unsigned long int)width*(unsigned long int)height;
     new_slice_size=(unsigned long int)new_width*(unsigned long int)height;
             // compute the interpolation coordinates. These work for the left
             // and right edges of each pixel, hence (nx_out+1) points in all.
     { double zoom;

       zoom=(double)width/((double)new_width*zoom_factor);

       source_edge_positions.resize(new_width+1);
       left_edge_source=&source_edge_positions[0];
       right_edge_source=&source_edge_positions[1];
       switch (arc_correction)
        { case UNIFORM2ARC:
           { double coeff1, delta_theta;

             delta_theta=M_PI/(double)crystals_per_ring;
             coeff1=zoom/delta_theta;
             for (i=0; i <= new_width; i++)
              source_edge_positions[i]=xc_in+coeff1*
                                       sin(delta_theta*((double)i-0.5-xc_out));
           }
           break;
          case ARC2UNIFORM:
           { double coeff1, coeff2, delta_theta;

             delta_theta=M_PI/(double)crystals_per_ring;
             coeff1=1.0/delta_theta;
             coeff2=zoom*delta_theta;
             for (i=0; i <= new_width; i++)
              source_edge_positions[i]=xc_in+coeff1*
                                       asin(coeff2*((double)i-0.5-xc_out));
           }
           break;
          case NO_ARC:
           for (i=0; i <= new_width; i++)
            source_edge_positions[i]=xc_in+zoom*((double)i-0.5-xc_out);
           break;
        }
     }
                               // compute indices and weights for each position
     masks.resize(new_width);
     for (i=0; i < new_width; i++)
      { signed short int index;
        unsigned short int n_source_pxls;

        if (linear)
         { index=(signed short int)floor(left_edge_source[i]);
           n_source_pxls=(signed short int)ceilf(right_edge_source[i])-index+1;
         }
         else { index=(signed short int)round(left_edge_source[i]);
                n_source_pxls=(signed short int)round(right_edge_source[i])-
                              index+1;
              }
        masks[i].weight.clear();
        for (unsigned short int j=0; j < n_source_pxls; j++,
             index++)
         { if ((index < 0) || (index >= width)) continue;
           double x1, x2, weight;

           if (masks[i].weight.size() == 0)
            masks[i].first_index=(unsigned short int)index;
           x1=left_edge_source[i]-index;
           x2=right_edge_source[i]-index;
           if (linear)
            { if (fabs(x2) <= 1.0) weight=0.5*(1.0+x2*(2.0-fabs(x2)));
               else weight=0.0;
              if (x2 > 1.0) weight+=1.0;
              if (fabs(x1) <= 1.0) weight-=0.5*(1.0+x1*(2.0-fabs(x1)));
              if (x1 > 1.0) weight-=1.0;
            }
            else { if (fabs(x2) <= 0.5) weight=x2+0.5;
                    else weight=0.0;
                   if (x2 > 0.5) weight+=1.0;
                   if (fabs(x1) <= 0.5) weight-=x1+0.5;
                   if (x1 > 0.5) weight-=1.0;
                 }
           if (values) weight/=std::max(x2-x1, 0.001);
           masks[i].weight.push_back(weight);
         }
      }
     if (edge_handling != EDGE_NONE)              // special treatment of edges
      { unsigned short int count;
                                    // rebin a uniform array to see if it fails
                                    // to rebin to the expected value
        mock_in.assign(width, 1.0);
        rb=new RebinX <T>(width, 1, new_width, arc_correction,
                          crystals_per_ring, xc_in, xc_out, zoom_factor,
                          linear, true, EDGE_NONE);
        mock_out.resize(new_width);
        rb->calcRebinX(&mock_in[0], &mock_out[0], 1);
        delete rb;
        rb=NULL;
                                // search regions which don't rebin as expected
        count=0;
        edge_i1=0;
        edge_i2=0;
        for (i=0; i < new_width; i++)
         if (fabs(mock_out[i]-1.0) < 1E-4) { if (count++ == 0) edge_i1=i;
                                             edge_i2=i;
                                           }
                               // everything ok ? -> no edge handling necessary
        if ((count == 0) || (count == new_width)) edge_handling=EDGE_NONE;
      }
   }
   catch (...)
    { if (rb != NULL) delete rb;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin dataset in x-direction.
    \param[in]     array_in    dataset
    \param[in,out] array_out   rebinned dataset
    \param[in]     slices      number of slices

    Rebin dataset in x-direction by linear interpolation. The edges of the
    lines are treated separately.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void RebinX <T>::calcRebinX(T * const array_in, T * const array_out,
                            const unsigned short int slices) const
 {                                                        // create empty image
   memset(array_out, 0, new_slice_size*(unsigned long int)slices*sizeof(T));
   for (unsigned short int z=0; z < slices; z++)                // rebin slices
    { unsigned short int i;

      for (i=0; i < new_width; i++)                               // rebin rows
       if (masks[i].weight.size() > 0)
        { unsigned short int y;
          T *in_ptr, *out_ptr;

          for (out_ptr=array_out+(unsigned long int)z*new_slice_size+i,
               in_ptr=array_in+(unsigned long int)z*old_slice_size,
               y=0; y < height; y++,                            // rebin column
               in_ptr+=width,
               out_ptr+=new_width)
           { T *ip;
                                                  // weighted sum of neighbours
             ip=in_ptr+masks[i].first_index;
             for (unsigned short int j=0; j < masks[i].weight.size(); j++)
              out_ptr[0]+=ip[j]*masks[i].weight[j];
           }
        }
    }
   if (edge_handling != EDGE_NONE)                // special treatment of edges
    { T *ap;

      ap=array_out;
      for (unsigned short int z=0; z < slices; z++)
       for (unsigned short int y=0; y < height; y++,
            ap+=new_width)
        if (edge_handling == EDGE_TRUNCATE)
         {                          // truncate edges and replace by neighbours
           if (edge_i1 > 0)                                        // left edge
            for (unsigned short int i=0; i < edge_i1; i++)
             ap[i]=ap[edge_i1];
           if (edge_i2 < new_width-1)                             // right edge
            for (unsigned short int i=edge_i2+1; i < new_width; i++)
             ap[i]=ap[edge_i2];
         }
         else {                                            // set edges to zero
                if (edge_i1 > 0) memset(ap, 0, edge_i1*sizeof(T)); // left edge
                if (edge_i2 < new_width-1)                        // right edge
                 memset(ap+edge_i2+1, 0, (new_width-edge_i2-1)*sizeof(T));
              }
    }
 }
