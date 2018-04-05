/*! \file thread_wrapper.h
    \brief This module is a wrapper for the operating system dependent thread
           implementation.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/04/23 added function to request thread id
    \date 2004/05/04 added Doxygen style comments
*/

#pragma once

#include <pthread.h>

/*- exported types ----------------------------------------------------------*/

typedef pthread_t tthread_id;                         /*!< type of thread id */

/*- exported functions ------------------------------------------------------*/

                                                           // create new thread
void ThreadCreate(tthread_id * const, void * (*start_routine)(void *),
                  void * const);
                                                // wait until thread terminates
void ThreadJoin(const tthread_id, void ** const);
