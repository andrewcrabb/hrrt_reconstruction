//** file: red_main.cpp
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

/*- code design ---------------------------------------------------------------

This is the main program of the Remote Execution Daemon. It contains only the
event loop.

-------------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/

#include <iostream>
#if defined(__linux__) || defined(__SOLARIS__)
#include <new>
#endif
#ifdef WIN32
#include <new.h>
#endif
#include "exception.h"
#include "logging.h"
#include "red.h"
#include "timestamp.h"

/*---------------------------------------------------------------------------*/
/* OutOfMemory: Error-handler for new command                                */
/*---------------------------------------------------------------------------*/
#ifdef WIN32
int OutOfMemory(size_t)
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
void OutOfMemory()
#endif
 { throw Exception(REC_RS_OUT_OF_MEMORY,
                   "Memory exhausted in reco_server.exe process.");
 }

/*---------------------------------------------------------------------------*/
/* main: main program                                                        */
/*  argc   number of arguments                                               */
/*  argv   list of arguments                                                 */
/*---------------------------------------------------------------------------*/
int main(int argc, char **argv)
 { Red *red=NULL;

   std::ios::sync_with_stdio(false);
#ifdef WIN32
   _set_new_handler(OutOfMemory);
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   std::set_new_handler(OutOfMemory);
#endif
   if (argc == 2)
    if (strcmp(argv[1], "-v") == 0) { timestamp("reco_server.cpp", "2003");
                                      return(EXIT_SUCCESS);
                                    }
   if (argc > 1)
    { std::cout << "\nThis program may only be called from the Queue process."
                << std::endl;
      return(EXIT_SUCCESS);
    }
   try
   { std::string logpath=std::string();
     unsigned short int logcode=52;

     switch (logcode % 10)                                // initialize logging
      { case 0:
         break;
        case 1:
         flog.init("red", logcode/10, logpath, Logging::LOG_FILE);
         break;
        case 2:
         flog.init("red", logcode/10, logpath, Logging::LOG_SCREEN);
         break;
        case 3:
         flog.init("red", logcode/10, logpath,
                   Logging::LOG_FILE|Logging::LOG_SCREEN);
         break;
      }
     flog.logMsg("Remote Execution Daemon started", 0);

     red=new Red();
     while (!red->recvAndInterpretMsg()) ;                        // event loop
     delete red;
     red=NULL;

     flog.logMsg("Remote Execution Daemon stopped", 0);
     flog.close();
   }
   catch (const Exception r)                               // handle exceptions
    { std::cerr << "Error: " << r.errStr() << std::endl;
      flog.close();
      if (red != NULL) delete red;
      return(EXIT_FAILURE);
    }
   return(EXIT_SUCCESS);
 }
