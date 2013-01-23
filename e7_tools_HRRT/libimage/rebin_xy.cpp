/*! \class RebinXY rebin_xy.h "rebin_xy.h"
    \brief This class implements the value-preserving rebinning of 2d images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/12/20 support image zoom

    This class implements the value-preserving rebinning of 2d images. It uses
    a bilinear interpolation for the rebinning. Voxels at the edges of a plane
    are copied.
 */

#include <cstdlib>
#include <cstring>
#include "rebin_xy.h"
#include "exception.h"
#include "global_tmpl.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.
    \param[in] _old_width        width of image
    \param[in] old_dx            width of voxel in mm
    \param[in] _old_height       height of image
    \param[in] old_dy            height of voxel in mm
    \param[in] _new_width        width of image after rebinning
    \param[in] new_dx            width of voxel in mm after rebinning
    \param[in] _new_height       height of image after rebinning
    \param[in] new_dy            height of voxel in mm after rebinning
    \param[in] preserve_values   preserve voxel values
                                 (otherwise preserve sum of counts)
 
    Initialize object.
 */
/*---------------------------------------------------------------------------*/
RebinXY::RebinXY(const unsigned short int _old_width, const float old_dx,
                 const unsigned short int _old_height, const float old_dy,
                 const unsigned short int _new_width, const float new_dx,
                 const unsigned short int _new_height, const float new_dy,
                 const bool preserve_values)
 { old_width=_old_width;
   old_height=_old_height;
   new_width=_new_width;
   new_height=_new_height;
   factor_x=new_dx/old_dx;
   factor_y=new_dy/old_dy;
   offsx=(signed short int)
         ((float)old_width*old_dx-(float)new_width*new_dx)/(2.0*old_dx);
   offsy=(signed short int)
         ((float)old_height*old_dy-(float)new_height*new_dy)/(2.0*old_dy);
   if ((offsx < 0) || (offsy < 0))
    throw Exception(REC_NEG_ZOOM,
                    "Zoom values smaller than 1 are not supported.");
                   // this is used for attenuation data which is scaled to 1/cm
                   // the scaling doesn't change with rebinning
   if (preserve_values) scale_factor=1.0f;
    else scale_factor=factor_x*factor_y;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Rebin image.
    \param[in] image    image
    \param[in] slices   number of slices
    \return rebinned image
 
    Rebin image with bilinear interpolation. The memory for the new image is
    allocated and a pointer to this image is returned.
 */
/*---------------------------------------------------------------------------*/
float *RebinXY::calcRebinXY(float * const image,
                            const unsigned short int slices) const
 { float *nimage=NULL;

   try
   { float *nip;
     unsigned long int old_image_size;

     old_image_size=(unsigned long int)old_width*(unsigned long int)old_height;
     nimage=new float[(unsigned long int)new_width*
                      (unsigned long int)new_height*
                      (unsigned long int)slices];
     memset(nimage, 0, (unsigned long int)new_width*
                       (unsigned long int)new_height*
                       (unsigned long int)slices*sizeof(float));
     nip=nimage;
     for (unsigned short int s=0; s < slices; s++)
      for (unsigned short int y=0; y < new_height; y++)
       { float j, j2;
         unsigned short int oy;
                                      // rebin planes by bilinear interpolation
         j=(float)y*factor_y;
         oy=(unsigned short int)j;
         j-=oy;
         j2=1.0f-j;
         for (unsigned short int x=0; x < new_width; x++)
          { unsigned short int ox;
            float i, *ip;

            i=(float)x*factor_x;
            ox=(unsigned short int)i;
            i-=ox;
            ip=image+(unsigned long int)s*old_image_size+
                     (unsigned long int)(oy+offsy)*
                     (unsigned long int)old_width+ox+offsx;
            if ((oy >= old_height) || (ox >= old_width)) nip[x]=0.0f;
             else { if (oy == old_height-1)
                     if (ox == old_width-1) nip[x]=*ip;
                      else nip[x]=(1.0f-i)*ip[0]+i*ip[1];
                     else if (ox == old_width-1)
                           nip[x]=j2*ip[0]+j*ip[old_width];
                           else { float i2;

                                  i2=1.0f-i;
                                  nip[x]=j2*i2*ip[0]+j2*i*ip[1]+
                                        j*i2*ip[old_width]+j*i*ip[old_width+1];
                                }
                    nip[x]*=scale_factor;
                  }
          }
         nip+=new_width;
       }
     return(nimage);
   }
   catch (...)
    { if (nimage != NULL) delete[] nimage;
      throw;
    }
 }


/*---------------------------------------------------------------------------*/
/*! \brief Rebin image.
    \param[in]  image    image
    \param[out] nimage   rebinned image
    \param[in]  slices   number of slices
    \return rebinned image
 
    Rebin image with bilinear interpolation. The memory for the new image is
    already allocated when the method is called.
 */
/*---------------------------------------------------------------------------*/
float *RebinXY::calcRebinXY(float * const image, float * const nimage,
                            const unsigned short int slices) const
 { float *nip;
   unsigned long int old_image_size;

   old_image_size=(unsigned long int)old_width*(unsigned long int)old_height;
   nip=nimage;
   for (unsigned short int s=0; s < slices; s++)
    for (unsigned short int y=0; y < new_height; y++)
     { float j, j2;
       unsigned short int oy;
                                      // rebin planes by bilinear interpolation
       j=(float)y*factor_y;
       oy=(unsigned short int)j;
       j-=oy;
       j2=1.0f-j;
       for (unsigned short int x=0; x < new_width; x++)
        { unsigned short int ox;
          float i, *ip;

          i=(float)x*factor_x;
          ox=(unsigned short int)i;
          i-=ox;
          ip=image+(unsigned long int)s*old_image_size+
                   (unsigned long int)oy*(unsigned long int)old_width+ox;
          if (oy == old_height-1)
           if (ox == old_width-1) *nip=*ip;
            else *nip=(1.0f-i)*ip[0]+i*ip[1];
           else if (ox == old_width-1) *nip=j2*ip[0]+j*ip[old_width];
                 else { float i2;

                        i2=1.0f-i;
                        *nip=j2*i2*ip[0]+j2*i*ip[1]+
                             j*i2*ip[old_width]+j*i*ip[old_width+1];
                      }
          *nip++=*nip*scale_factor;
        }
     }
   return(nimage);
 }
