/*! \class Semaphore semaphore_al.h "semaphore_al.h"
    \brief This class implements a C++ wrapper for semaphores.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments

    This class implements a C++ wrapper for semaphores.
 */

#include <fcntl.h>
#include <unistd.h>
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
   sem_init(&sem, 0, value);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy semaphore.

    Destroy semaphore.
 */
/*---------------------------------------------------------------------------*/
Semaphore::~Semaphore()
 {
   sem_destroy(&sem);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request value of semaphore.
    \return value of semaphore

    Request value of semaphore.
 */
/*---------------------------------------------------------------------------*/
unsigned short int Semaphore::getValue()
 { int value;

   sem_getvalue(&sem, &value);
   return((unsigned short int)value);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Signal semaphore.

    Signal semaphore.
 */
/*---------------------------------------------------------------------------*/
void Semaphore::signal()
 {
   sem_post(&sem);
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

   return(sem_trywait(&sem) == 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until semaphore is available.

    Wait until semaphore is available.
 */
/*---------------------------------------------------------------------------*/
void Semaphore::wait()
 {
   sem_wait(&sem);
 }
