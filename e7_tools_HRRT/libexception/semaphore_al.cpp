/*! \class Semaphore semaphore_al.h "semaphore_al.h"
    \brief This class implements a C++ wrapper for semaphores.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments

    This class implements a C++ wrapper for semaphores.
 */

#include <fcntl.h>
#if defined(__linux__) || defined(__SOLARIS__)
#include <unistd.h>
#endif
#ifdef __SOLARIS__
#include <semaphore.h>
#endif
#include "semaphore_al.h"

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Create semaphore.
    \param[in] value   value of semaphore

    Create semaphore with the given value.
 */
/*---------------------------------------------------------------------------*/
Semaphore::Semaphore(unsigned short int value)
 {
#if defined(__linux__) || defined(__MACOSX__) || defined(__SOLARIS__)
   sem_init(&sem, 0, value);
#endif
#ifdef WIN32
   sem=CreateSemaphore(NULL, value, 9999, NULL);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy semaphore.

    Destroy semaphore.
 */
/*---------------------------------------------------------------------------*/
Semaphore::~Semaphore()
 {
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   sem_destroy(&sem);
#endif
#ifdef WIN32
   CloseHandle(sem);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request value of semaphore.
    \return value of semaphore

    Request value of semaphore.
 */
/*---------------------------------------------------------------------------*/
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
unsigned short int Semaphore::getValue()
 { int value;

   sem_getvalue(&sem, &value);
   return((unsigned short int)value);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Signal semaphore.

    Signal semaphore.
 */
/*---------------------------------------------------------------------------*/
void Semaphore::signal()
 {
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   sem_post(&sem);
#endif
#ifdef WIN32
   ReleaseSemaphore(sem, 1, NULL);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get semaphore if available.
    \return was semaphore available ?

    Get semaphore if it is available. This method returns immediately, even if
    the semaphore is not available.
 */
/*---------------------------------------------------------------------------*/
bool Semaphore::tryWait()
 {
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   return(sem_trywait(&sem) == 0);
#endif
#ifdef WIN32
   return(WaitForSingleObject(sem, 0) != WAIT_TIMEOUT);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until semaphore is available.

    Wait until semaphore is available.
 */
/*---------------------------------------------------------------------------*/
void Semaphore::wait()
 {
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   sem_wait(&sem);
#endif
#ifdef WIN32
   WaitForSingleObject(sem, INFINITE);
#endif
 }
