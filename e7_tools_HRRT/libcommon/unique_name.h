/*! \file unique_name.h
    \brief This class provides unique names.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/08/26 initial version
    \date 2005/03/29 get IP number only once
 */

#ifndef _UNIQUE_NAME_H
#define _UNIQUE_NAME_H

#include <string>
#include "semaphore_al.h"

/*- class definitions -------------------------------------------------------*/

class UniqueName
 { private:
    static UniqueName *instance; /*!< pointer to only instance of this class */
    unsigned short int count,       /*!< counter for number of created names */
                       ip[4];                   /*!< IP number of local host */
    Semaphore *lock;                        /*!< semaphore for thread safety */
   protected:
    UniqueName();                                              // create object
    ~UniqueName();                                            // destroy object
   public:
    static void close();                           // delete instance of object
    std::string getName();                                // return unique name
    static UniqueName *un();               // get pointer to instance of object
 };

#endif
