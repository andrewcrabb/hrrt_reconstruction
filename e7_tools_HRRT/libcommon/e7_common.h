/*! \file e7_common.h
    \brief This module provides some common functions which are used in the
           e7-Tools.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2004/09/08 added stripExtension function
    \date 2004/10/09 added gantryModel() function
    \date 2004/11/19 added fireCOMEvent() function
    \date 2005/02/08 added deleteDirectory() function
    \date 2005/02/11 added deleteFiles() function
    \date 2005/02/12 integrate error group in fireCOMEvent()
 */

#ifndef _E7_COMMON_H
#define _E7_COMMON_H

#include <string>
#include <vector>
#ifdef WIN32
#include "atlbase.h"
#endif
#include "ecat7.h"
#include "semaphore_al.h"
#include "types.h"

/*- global variables --------------------------------------------------------*/
                       /*! already one message fired through COM interface ? */
extern bool fired_once;
extern Semaphore sem_fire;            /*!< semaphore for COM interface usage */

/*- exported functions ------------------------------------------------------*/

                             // get bed position in mm for a given ECAT7 matrix
float bedPosition(ECAT7 * const, const unsigned short int);
                         // calculate decay and frame-length correction factors
float calcDecayFactor(const float, const float, const float, const float,
                      const bool, const bool, float * const,
                      const unsigned short int);
                   // calculate which ring pairs contribute to a sinogram plane
std::vector <std::vector<unsigned short int> > calcRingNumbers(
                                                     const unsigned short int,
                                                     const unsigned short int,
                                                     const unsigned short int);
                                        // check filename extension and cut-off
bool checkExtension(std::string * const, const std::string);
#ifdef WIN32
void deleteDirectory(std::string);        // delete directory including content
void deleteFiles(std::string);                                  // delete files
#endif
bool FileExist(const std::string);                    // check if a file exists
                                // find a matrix number to a given bed position
unsigned short int findMNR(const std::string, const bool, const float,
                           float * const);
                                        // notify COM event handler about error
void fireCOMEvent(const unsigned long int, const unsigned short int,
                  const COM_EVENT::tnotify, const std::string,
                  const unsigned short int, const unsigned long int,
                  const std::string);
                                       // request gantry model number from file
unsigned short int gantryModel(const std::string);
bool isECAT7file(const std::string);               // is file in ECAT7 format ?
bool isInterfile(const std::string);           // is file in Interfile format ?
                                        // get the number of matrices in a file
unsigned short int numberOfMatrices(const std::string);
#ifdef WIN32
int OutOfMemory(size_t);                     // error handler for "new" command
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
void OutOfMemory();                          // error handler for "new" command
#endif
bool PathExist(const std::string);                         // does path exist ?
std::string secStr(float);              // convert seconds into hh:mm:ss string
                  // convert internal segment number into normal segment number
std::string segStr(const unsigned short int);
#ifdef WIN32
BSTR string2BSTR(std::string);              // convert COM BSTR into STL string
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
std::string string2BSTR(std::string);       // convert COM BSTR into STL string
#endif
std::string stripPath(std::string);                // remove path from filename

#endif
