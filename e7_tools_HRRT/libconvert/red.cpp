//** file: red.cpp
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

/*- code design ---------------------------------------------------------------

The object opens a listening socket available to clients. It is able to respond
to a ping, terminate, install a program and execute a program.

-------------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/

#include <sys/stat.h>
#include <sys/types.h>
#ifdef WIN32
#include <direct.h>
#include <io.h>
#endif
#include "red.h"
#include "exception.h"
#include "logging.h"
#include "raw_io.h"
#include "red_const.h"

/*- constants ---------------------------------------------------------------*/

const unsigned long int RECV_BUFFER_SIZE=1000,        // size of receive buffer
                        SEND_BUFFER_SIZE=1000;           // size of send buffer

/*---------------------------------------------------------------------------*/
/* Red: open listening socket for clients                                    */
/*---------------------------------------------------------------------------*/
Red::Red()
 { rbuffer=NULL;
   wbuffer=NULL;
   rc=NULL;
   try
   { rbuffer=new StreamBuffer(RECV_BUFFER_SIZE);
     wbuffer=new StreamBuffer(SEND_BUFFER_SIZE);
                                            // open socket on fixed port number
     rc=new CommSocket(RED_PORT, rbuffer, wbuffer);
   }
   catch (...)
    { if (rc != NULL) { delete rc;
                        rc=NULL;
                      }
      if (rbuffer != NULL) { delete rbuffer;
                             rbuffer=NULL;
                           }
      if (wbuffer != NULL) { delete wbuffer;
                             wbuffer=NULL;
                           }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* ~Red: destory object                                                      */
/*---------------------------------------------------------------------------*/
Red::~Red()
 { if (rc != NULL) delete rc;
   if (rbuffer != NULL) delete rbuffer;
   if (wbuffer != NULL) delete wbuffer;
 }

/*---------------------------------------------------------------------------*/
/* recvAndInterpretMsg: received and interpret messages from clients         */
/*---------------------------------------------------------------------------*/
bool Red::recvAndInterpretMsg()
 { if (rc == NULL) return(false);
   unsigned short int cmd;
   std::string name;

   while (!rc->waitForConnect(0, 10)) ;            // wait till client connects
   *rc >> cmd;
   switch (cmd)
    { case RD_CMD_PING:                                                 // ping
       *rc >> name;
       flog.logMsg("RED received PING from #1", 1)->arg(name);
       rc->newMessage();
       *rc << (unsigned short int)4242 << std::endl;
       break;
      case RD_CMD_EXIT:                                            // terminate
       *rc >> name;
       flog.logMsg("RED received TERM from #1", 1)->arg(name);
       rc->newMessage();
       *rc << RD_CMD_EXIT << std::endl;
#ifdef WIN32
       Sleep(1);
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
       sleep(1);
#endif
       rc->closeConnectionToClient();             // close connection to client
       return(true);
      case RD_CMD_INSTALL:                                // install executable
       { unsigned char *buffer=NULL;
         RawIO <unsigned char> *rio=NULL;

         try
         { std::string filename, fname, directory=std::string();
           unsigned long int size;

           *rc >> name >> filename >> size;
           flog.logMsg("RED received INSTALL(#1) from #2", 1)->arg(filename)->
            arg(name);
                                                            // create directory
           fname=filename;
           for (;;)
            { std::string::size_type p;

              p=fname.find('/');
              if (p == std::string::npos) break;
              directory+=fname.substr(0, p+1);
              fname=fname.substr(p+1);
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
              mkdir(directory.c_str(), S_IXUSR | S_IRUSR | S_IWUSR |
                                       S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
#ifdef WIN32
              _mkdir(directory.c_str());
#endif
            }
                                                // receive and store executable
           buffer=new unsigned char[size];
           rc->readData(buffer, size);
           rio=new RawIO <unsigned char>(filename, true, false);
           rio->write(buffer, size);
           delete rio;
           rio=NULL;
           delete[] buffer;
           buffer=NULL;
                                         // change access rights for executable
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
           chmod(filename.c_str(), S_IXUSR | S_IRUSR | S_IWUSR |
                                   S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
#ifdef WIN32
           _chmod(filename.c_str(), _S_IWRITE | _S_IREAD);
#endif
         }
         catch (...)
          { if (buffer != NULL) delete[] buffer;
            if (rio != NULL) delete rio;
            throw;
          }
       }
       break;
      case RD_CMD_EXEC:                                      // execute program
       { std::string filename, params;

         *rc >> name >> filename >> params;
         flog.logMsg("RED received EXEC(#1) from #2", 1)->arg(filename)->
          arg(name);
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
         if (fork() == 0) { filename+=" "+params;
                            system(filename.c_str());
                            exit(127);
                          }
#endif
#ifdef WIN32
         STARTUPINFO si;
         PROCESS_INFORMATION pi;

         ZeroMemory(&si, sizeof(si));
         si.cb=sizeof(si);
         ZeroMemory(&pi, sizeof(pi));
         CreateProcess(filename.c_str(), (char *)params.c_str(), NULL,
                       NULL, false, 0, NULL, NULL, &si, &pi);
#endif
       }
       break;
      default:                                                  // can't happen
       throw Exception(REC_UNKNOWN_RED_COMMAND, "Remote Execution Daemon "
                       "received unknown command.");
       break;
    }
   rc->closeConnectionToClient();                 // close connection to client
   return(false);
 }
