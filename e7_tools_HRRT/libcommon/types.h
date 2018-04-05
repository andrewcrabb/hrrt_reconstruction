/*! \file types.h
    \brief This file provides global type definitions.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/03/31 added Doxygen style comments
    \date 2004/10/14 added uint32, uint64, int64 datatypes
    \date 2005/03/11 added PSF-AW-OSEM
 */

#pragma once

/*- type declarations -------------------------------------------------------*/

/*! \brief This namespace is used for an enumeration of all recontruction
           algorithms.

    This namespace is used for an enumeration of all recontruction algorithms.
 */
namespace ALGO
{                                              /*! reconstruction algorithms */
  typedef enum { FBP_Algo,                      /*!< filtered backprojection */
                 DIFT_Algo,          /*!< discrete inverse fourier transform */
                 MLEM_Algo,                                       /*!< ML-EM */
                 OSEM_UW_Algo,          /*!< unweighted ordered subset ML-EM */
                 OSEM_AW_Algo,                /*!< attenuation weighted OSEM */
                 OSEM_NW_Algo,              /*!< normalisation weighted OSEM */
                             /*! attenuation and normalisation weighted OSEM */
                 OSEM_ANW_Algo,
                 OSEM_OP_Algo,                    /*!< ordinary poisson OSEM */
                 MAPTR_Algo,                           /*!< MAP-TR algorithm */
                 OSEM_PSF_AW_Algo    /*!< attenuation weighted OSEM with PSF */
               } talgorithm;
}

/*! \brief This namepsace is used for an enumeration of all scan types.

    This namepsace is used for an enumeration of all scan types.
 */
namespace SCANTYPE
{                                                           /*! type of scan */
  typedef enum { MULTIFRAME,                           /*!< multi-frame scan */
                 MULTIBED,                               /*!< multi-bed scan */
                 MULTIGATE,                             /*!< multi-gate scan */
                 SINGLEFRAME                          /*!< single-frame scan */
               } tscantype;
}

/*! \brief This namespace is used for an enumeration of all COM event types.

    This namespace is used for an enumeration of all COM event types.
 */
namespace COM_EVENT
{                                                  /*! status for COM events */
  typedef enum { WAITING,                                /*!< job is waiting */
                 PROCESSING,                     /*!< job is being processed */
                 ERROR_STATUS,                        /*!< error has occured */
                 FINISHED                              /*! < job is finished */
               } tnotify;
}


                                                       // supports C99 standard
typedef unsigned long long int uint64;     /*!< 64 bit unsigned integer type */
typedef signed long long int int64;          /*!< 64 bit signed integer type */


#ifdef CPU64BIT
typedef unsigned int uint32;               /*!< 32 bit unsigned integer type */
#else
typedef unsigned long int uint32;          /*!< 32 bit unsigned integer type */
#endif
