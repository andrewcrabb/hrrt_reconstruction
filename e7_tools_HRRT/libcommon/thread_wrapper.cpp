/*! \file thread_wrapper.cpp
    \brief This module is a wrapper for the operating system dependent thread
           implementation.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/04/23 added function to request thread id
    \date 2004/05/04 added Doxygen style comments
*/

#include "thread_wrapper.h"

/*---------------------------------------------------------------------------*/
/*! \brief Create a new thread.
    \param[out] thread          thread id
    \param[in]  start_routine   thread function
    \param[in]  arg             thread parameter

    Create a new thread.
 */
/*---------------------------------------------------------------------------*/
void ThreadCreate(tthread_id * const thread, void * (*start_routine)(void *),
                  void * const arg)
 { pthread_create(thread, NULL, start_routine, arg);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until thread terminates.
    \param[in]  thread          thread id
    \param[out] thread_return   return value of thread function

    Wait until thread terminates.
 */
/*---------------------------------------------------------------------------*/
void ThreadJoin(const tthread_id thread, void ** const thread_return)
 { pthread_join(thread, thread_return);
 }
