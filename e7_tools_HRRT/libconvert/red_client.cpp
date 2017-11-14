//** file: red_client.cpp
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

/*- code design ---------------------------------------------------------------

This module provides functions for the communication with a Remote Execution
Daemon.

-------------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/

#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <sys/utsname.h>
#endif
#include "red_client.h"
#include "comm_socket.h"
#include "logging.h"
#include "raw_io.h"
#include "red_const.h"
#include "stream_buffer.h"

/*- constants ---------------------------------------------------------------*/

const unsigned short int PING_TIMEOUT=3;         // timeout for ping in seconds

/*---------------------------------------------------------------------------*/
/* RED_execute: execute program on REDs computer                             */
/*  hostname   name or IP number of REDs computer                            */
/*  filename   name of executable, including path used on REDs computer      */
/*  params     parameters for executable                                     */
/*---------------------------------------------------------------------------*/
void RED_execute(const std::string hostname, const std::string filename,
                 const std::string params)
 { StreamBuffer *rbuffer=NULL, *wbuffer=NULL;
   CommSocket *rc=NULL;

   try
   { std::string name;
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     { struct utsname un;
                                                  // get name of local computer
       uname(&un);
       name=std::string(un.nodename);
     }
#endif
#ifdef WIN32
     { DWORD size=MAX_COMPUTERNAME_LENGTH+1;
       char name_char[MAX_COMPUTERNAME_LENGTH+1];
                                                  // get name of local computer
       GetComputerName(name_char, &size);
       name=std::string(name_char);
     }
#endif
                                                 // establish connection to RED
     rbuffer=new StreamBuffer(500);
     wbuffer=new StreamBuffer(500);
     rc=new CommSocket(hostname, RED_PORT, rbuffer, wbuffer);
                                                                // send message
     rc->newMessage();
     *rc << RD_CMD_EXEC << name << filename << params << std::endl;
                                                   // destroy connection to RED
     delete rc;
     rc=NULL;
     delete wbuffer;
     wbuffer=NULL;
     delete rbuffer;
     rbuffer=NULL;
     return;
   }
   catch (...)
    { if (rc != NULL) delete rc;
      if (wbuffer != NULL) delete wbuffer;
      if (rbuffer != NULL) delete rbuffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* RED_install: install program on REDs computer                             */
/*  hostname   name or IP number of REDs computer                            */
/*  filename   name of executable, including path used local computer        */
/*  destpath   path used on REDs computer                                    */
/*---------------------------------------------------------------------------*/
void RED_install(const std::string hostname, const std::string filename,
                 const std::string destpath)
 { StreamBuffer *rbuffer=NULL, *wbuffer=NULL;
   CommSocket *rc=NULL;
   RawIO <unsigned char> *rio=NULL;
   unsigned char *buffer=NULL;

   try
   { std::string name, destfilename;
     unsigned long int filesize;
     std::string::size_type p;
                                                  // get name of local computer
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     { struct utsname un;

       uname(&un);
       name=std::string(un.nodename);
     }
#endif
#ifdef WIN32
     { DWORD size=MAX_COMPUTERNAME_LENGTH+1;
       char name_char[MAX_COMPUTERNAME_LENGTH+1];

       GetComputerName(name_char, &size);
       name=std::string(name_char);
     }
#endif
                                                 // establish connection to RED
     rbuffer=new StreamBuffer(10000);
     wbuffer=new StreamBuffer(10000);
     rc=new CommSocket(hostname, RED_PORT, rbuffer, wbuffer);
                                                             // load executable
     rio=new RawIO <unsigned char>(filename, false, false);
     filesize=rio->size();
     buffer=new unsigned char[filesize];
     rio->read(buffer, filesize);
     delete rio;
     rio=NULL;

     destfilename=destpath+'/';
     if ((p=filename.rfind('/')) != std::string::npos)
      destfilename+=filename.substr(p+1);
      else destfilename+=filename;
                                                 // send message and executable
     rc->newMessage();
     *rc << RD_CMD_INSTALL << name << destfilename << filesize;
     rc->writeData(buffer, filesize);
     *rc << std::endl;
                                                   // destroy connection to RED
     delete rc;
     rc=NULL;
     delete[] buffer;
     buffer=NULL;
     delete wbuffer;
     wbuffer=NULL;
     delete rbuffer;
     rbuffer=NULL;
     return;
   }
   catch (...)
    { if (rc != NULL) delete rc;
      if (wbuffer != NULL) delete wbuffer;
      if (rbuffer != NULL) delete rbuffer;
      if (rio != NULL) delete rio;
      if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* RED_ping: send ping to RED                                                */
/*  hostname   name or IP number of REDs computer                            */
/* return: successful ?                                                      */
/*---------------------------------------------------------------------------*/
bool RED_ping(const std::string hostname)
 { StreamBuffer *rbuffer=NULL, *wbuffer=NULL;
   CommSocket *rc=NULL;

   try
   { unsigned short int result;
     std::string name;
                                                  // get name of local computer
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     { struct utsname un;

       uname(&un);
       name=std::string(un.nodename);
     }
#endif
#ifdef WIN32
     { DWORD size=MAX_COMPUTERNAME_LENGTH+1;
       char name_char[MAX_COMPUTERNAME_LENGTH+1];

       GetComputerName(name_char, &size);
       name=std::string(name_char);
     }
#endif
                                                 // establish connection to RED
     rbuffer=new StreamBuffer(200);
     wbuffer=new StreamBuffer(200);
     try
     { Logging::flog()->loggingOn(false);
       rc=new CommSocket(hostname, RED_PORT, rbuffer, wbuffer);
                                                                // send message
       rc->newMessage();
       *rc << RD_CMD_PING << name << std::endl;
       if (rc->msgReceived(PING_TIMEOUT)) *rc >> result;
        else result=0;
                                                   // destroy connection to RED
       delete rc;
       rc=NULL;
       Logging::flog()->loggingOn(true);
     }
     catch (...)
      { Logging::flog()->loggingOn(true);
        if (rc != NULL) { delete rc;
                          rc=NULL;
                        }
        result=0;
      }
     delete wbuffer;
     wbuffer=NULL;
     delete rbuffer;
     rbuffer=NULL;
     return(result == 4242);
   }
   catch (...)
    { if (rc != NULL) delete rc;
      if (wbuffer != NULL) delete wbuffer;
      if (rbuffer != NULL) delete rbuffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/* RED_ping: send terminate command to RED                                   */
/*  hostname   name or IP number of REDs computer                            */
/*---------------------------------------------------------------------------*/
void RED_terminate(const std::string hostname)
 { StreamBuffer *rbuffer=NULL, *wbuffer=NULL;
   CommSocket *rc=NULL;

   try
   { unsigned short int result;
     std::string name;
                                                  // get name of local computer
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
     { struct utsname un;

       uname(&un);
       name=std::string(un.nodename);
     }
#endif
#ifdef WIN32
     { DWORD size=MAX_COMPUTERNAME_LENGTH+1;
       char name_char[MAX_COMPUTERNAME_LENGTH+1];

       GetComputerName(name_char, &size);
       name=std::string(name_char);
     }
#endif
                                                 // establish connection to RED
     rbuffer=new StreamBuffer(200);
     wbuffer=new StreamBuffer(200);
     rc=new CommSocket(hostname, RED_PORT, rbuffer, wbuffer);
                                                                // send message
     rc->newMessage();
     *rc << RD_CMD_EXIT << name << std::endl;
     *rc >> result;
                                                   // destroy connection to RED
     delete rc;
     rc=NULL;
     delete wbuffer;
     wbuffer=NULL;
     delete rbuffer;
     rbuffer=NULL;
     return;
   }
   catch (...)
    { if (rc != NULL) delete rc;
      if (wbuffer != NULL) delete wbuffer;
      if (rbuffer != NULL) delete rbuffer;
      throw;
    }
 }
