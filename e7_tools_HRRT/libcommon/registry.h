/*! \file registry.h
    \brief This file provides names of registry keys and environment vairables.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/11/30 added key for pixel data path
    \date 2005/01/14 added key for image import path
 */

#ifndef _REGISTRY_H
#define _REGISTRY_H

#include <string>

/*- constants ---------------------------------------------------------------*/


#ifdef WIN32
                                                  /*! information about CPUs */
const std::string cpukey="Hardware\\Description\\System\\CentralProcessor";
#endif
const std::string reg_key="Software\\CPS\\Queue";      /*!< QRS specific key */
                               /*! sub key for path of reconstruction server */
const std::string sub_key_path="PathOfReconstructionServer";
                     /*! sub key for port number of queue/DCOM server socket */
const std::string sub_key_port="ListeningPort";
const std::string logging_path="LoggingPath";      /*!< key for logging path */
                                               /*! key for image import path */
const std::string image_path="ImageImportPath";
                         /*! environment variable for installation directory */
const std::string install_dir="E7TOOLS";
#endif
