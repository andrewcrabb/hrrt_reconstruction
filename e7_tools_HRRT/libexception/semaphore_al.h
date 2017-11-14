/*! \file semaphore_al.h
    \brief This class implements a C++ wrapper for semaphores.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
 */

#ifndef _SEMAPHORE_AL_H
#define _SEMAPHORE_AL_H

#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <pthread.h>
#include <semaphore.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

/*- class definitions -------------------------------------------------------*/

class Semaphore
 { private:
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
    sem_t sem;                                         /*!< semaphore handle */
#endif
#ifdef WIN32
    HANDLE sem;                                        /*!< semaphore handle */
#endif
   public:
    Semaphore(unsigned short int);                          // create semaphore
    ~Semaphore();                                          // destroy semaphore
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
    unsigned short int getValue();                // request value of semaphore
#endif
    void signal();                                          // signal semaphore
    bool tryWait();                          // asynchronous wait for semaphore
    void wait();                              // synchronous wait for semaphore
 };

#endif
