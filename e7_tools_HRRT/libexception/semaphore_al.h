/*! \file semaphore_al.h
    \brief This class implements a C++ wrapper for semaphores.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments
 */

#pragma once

#include <pthread.h>
#include <semaphore.h>

/*- class definitions -------------------------------------------------------*/

class Semaphore
 { private:
    sem_t sem;                                         /*!< semaphore handle */
   public:
    Semaphore(unsigned short int);                          // create semaphore
    ~Semaphore();                                          // destroy semaphore
    unsigned short int getValue();                // request value of semaphore
    void signal();                                          // signal semaphore
    bool tryWait();                          // asynchronous wait for semaphore
    void wait();                              // synchronous wait for semaphore
 };
