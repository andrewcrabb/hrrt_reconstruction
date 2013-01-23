/*! \class UniqueName unique_name.h "unique_name.h"
    \brief This class provides unique names.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/08/26 initial version
    \date 2005/03/29 get IP number only once

    This class provides unique names that can for example be used as filenames.
 */

#include <time.h>
#ifdef WIN32
#include <winsock2.h>
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <netdb.h>
#endif
#include "unique_name.h"
#include "exception.h"
#include "str_tmpl.h"

/*- methods -----------------------------------------------------------------*/

                                      /*! pointer to only instance of object */
UniqueName *UniqueName::instance=NULL;

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
UniqueName::UniqueName()
 { struct hostent *he;
   char buffer[50];
#ifdef WIN32
   WSADATA wsaData;
#endif
   
   count=0;
   lock=new Semaphore(1);
   lock->wait();
#ifdef WIN32
                                                     // initialize winsock.dll
   if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR)
    throw Exception(REC_SOCKET_ERROR_INIT,
                    "Initialization of winsock.dll failed.");
#endif
   if (gethostname(buffer, 50) == 0)
    { if ((he=gethostbyname(buffer)) == NULL)
       throw Exception( REC_GETHOSTBYNAME_FAILS, "gethostbyname() failed.");
      if (he->h_addr_list[0] != NULL)
       for (unsigned short int j=0; j < he->h_length; j++)
        ip[j]=(unsigned short int)he->h_addr_list[0][j];
    }
#ifdef WIN32
   WSACleanup();
#endif
   lock->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
UniqueName::~UniqueName()
 { if (lock != NULL) delete lock;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Delete instance of object.

    Delete instance of object.
 */
/*---------------------------------------------------------------------------*/
void UniqueName::close()
 { if (instance != NULL) { delete instance;
                           instance=NULL;
                         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Get unique name.
    \return unique name

    Get unique name. The name consists of hexadecimal presentations of the IP
    number of the computer, the process ID, the current time and a counter.
 */
/*---------------------------------------------------------------------------*/
std::string UniqueName::getName()
 { std::string s;

#ifdef WIN32
   int mypid;

   mypid=_getpid();
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   pid_t mypid;

   mypid=getpid();
#endif
   s=toStringZeroHex(ip[0], 2)+toStringZeroHex(ip[1], 2)+
     toStringZeroHex(ip[2], 2)+toStringZeroHex(ip[3], 2)+
     toStringZeroHex((unsigned long int)mypid, 4)+
     toStringZeroHex((unsigned long int)time(NULL), 8);
   lock->wait();
   s+=toStringZeroHex(count, 4);
   count++;
   lock->signal();
   return(s);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to instance of singleton object.
    \return pointer to instance of singleton object

    Request pointer to the only instance of the UniqueName object. The
    UniqueName object is a singleton object. If it is not initialized, the
    constructor will be called.
 */
/*---------------------------------------------------------------------------*/
UniqueName *UniqueName::un()
 { if (instance == NULL) instance=new UniqueName();
   return(instance);
 }
