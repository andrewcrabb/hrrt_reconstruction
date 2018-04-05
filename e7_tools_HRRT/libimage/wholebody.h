/*! \file wholebody.h
    \brief This class assembles images to wholebody images.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/10/28 initial version
    \date 2004/08/02 beds shipped with PET only system have different
                     coordinate system than PET/CT beds
 */

#pragma once

/*- class definitions -------------------------------------------------------*/

class Wholebody
 { private:
    float *data,                             /*!< pointer to wholebody image */
          bedpos,                 /*!< bed position of wholebody image in mm */
          slice_thickness;                    /*!< thickness of slices in mm */
    unsigned short int width,                            /*!< width of image */
                       height,                          /*!< height of image */
                       depth;                            /*!< depth of image */
    unsigned long int slice_size;                   /*!< size of image slice */
   public:
                                                           // initialize object
    Wholebody(float * const, const unsigned short int,
              const unsigned short int, const unsigned short int, const float,
              const float);
    ~Wholebody();                                             // destroy object
                                                  // add bed to wholebody image
    void addBed(float * const, const unsigned short int, float, const bool,
                const bool, const unsigned short int);
                                 // convert image from feet first to head first
    template <typename T>
     static void feet2head(T * const, const unsigned short int,
                           const unsigned short int, const unsigned short int);
                                                 // get data of wholebody image
    float *getWholebody(unsigned short int * const);
 };
