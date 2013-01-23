/*! \class CommSocket comm_socket.h "comm_socket.h"
    \brief This class is used for buffered TCP/IP socket communication.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2005/01/15 determine correct endianess of local machine

    This class is used for buffered TCP/IP socket communication. The socket can
    be used like a C++ stream (with << and >>). The communication is buffered
    with a read and write buffer. The user puts data into the send buffer. If
    "std::endl" is written into the buffer, the complete content is send across
    a TCP/IP socket to the receiver. The user at the other side gets data from
    the read buffer, until it is empty. Then the read buffer is refilled by new
    data from the TCP/IP socket.
 */

/*! \example example_comso.cpp
    Here are some examples of how to use the CommSocket class.
 */

#include <cstdio>
#ifdef WIN32
#include <process.h>
#endif
#if defined(__LINUX__) || defined(__MACOSX__)
#include <cstdlib>
#include <sys/wait.h>
#endif
#if defined(__SOLARIS__) || defined(__MACOSX__)
#include <unistd.h>
#endif
#ifndef _COMM_SOCKET_CPP
#define _COMM_SOCKET_CPP
#include "comm_socket.h"
#endif
#include "exception.h"
#include "str_tmpl.h"
#include "swap_tmpl.h"

/*- methods -----------------------------------------------------------------*/

#ifndef _COMM_SOCKET_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Create socket (server side).
    \param[in] server_ip     IP number of computer at server side
    \param[in] client_name   name of client executable (including path)
    \param[in] paramstr      parameter string for client
    \param[in] startup       start client process ?
    \param[in] sec           time to wait (seconds)
    \param[in] usec          time to wait (microseconds)
    \param[in] irbuffer      buffer for received data
    \param[in] iwbuffer      buffer for data to send
    \exception REC_CANT_START_PROCESS    server can't start client process
    \exception REC_CLIENT_DOESNT_CONNECT client doesn't connect to socket

    Create socket (server side). The constructor starts a client process,
    if the startup parameter is set. It wait for the given time or until a
    client process connects to the socket.
 */
/*---------------------------------------------------------------------------*/
CommSocket::CommSocket(const std::string server_ip,
                       const std::string client_name,
                       const std::string paramstr,
                       const bool startup, const time_t sec,
                       const time_t usec, StreamBuffer * const irbuffer,
                       StreamBuffer * const iwbuffer)
 { so=NULL;
   try
   { server_proc=true;
     process_started=false;
     if (BigEndianMachine()) endianess=1;
      else endianess=0;
     rbuffer=irbuffer;
     wbuffer=iwbuffer;
     so=new Socket(&portnumber);          // create TCP/IP socket (server side)
     if (startup)                                     // start client process ?
      { std::string params;
#ifdef WIN32
        STARTUPINFO si;

        ZeroMemory(&si, sizeof(si));
        si.cb=sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
            // start client with server IP number and port number as parameters
        params=server_ip+" "+toString(portNumber())+" "+paramstr;
        if (!CreateProcess(client_name.c_str(), (char *)params.c_str(), NULL,
                           NULL, false, 0, NULL, NULL, &si, &pi))
         throw Exception(REC_CANT_START_PROCESS,
                   "Server can't start client process '#1'.").arg(client_name);
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
        child_pid=fork();
        if (child_pid == 0)
         { params=client_name+" "+server_ip+" "+toString(portNumber())+" "+
                  paramstr;
           system(params.c_str());
           exit(127);
         }
#endif
        process_started=true;
      }
                                       // wait until client connects or timeout
     if (!waitForConnect(sec, usec))
      throw Exception(REC_CLIENT_DOESNT_CONNECT,
                      "Client process '#1' doesn't connect to TCP/IP socket of"
                      " server.").arg(client_name);
   }
   catch (...)
    { if (so != NULL) { delete so;
                        so=NULL;
                      }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create socket with given port number (server side).
    \param[in] portnumber   port number for listening
    \param[in] irbuffer     buffer for received data
    \param[in] iwbuffer     buffer for data to send

    Create socket with given port number (server side).
 */
/*---------------------------------------------------------------------------*/
CommSocket::CommSocket(const unsigned short int portnumber,
                       StreamBuffer * const irbuffer,
                       StreamBuffer * const iwbuffer)
 { so=NULL;
   try
   { process_started=false;
     server_proc=true;
     rbuffer=irbuffer;
     wbuffer=iwbuffer;
     if (BigEndianMachine()) endianess=1;
      else endianess=0;
     so=new Socket(portnumber);           // create TCP/IP socket (server side)
   }
   catch (...)
    { if (so != NULL) { delete so;
                        so=NULL;
                      }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create socket and get port number (server side).
    \param[in] irbuffer   buffer for received data
    \param[in] iwbuffer   buffer for data to send

    Create socket and get port number (server side).
 */
/*---------------------------------------------------------------------------*/
CommSocket::CommSocket(StreamBuffer * const irbuffer,
                       StreamBuffer * const iwbuffer)
 { so=NULL;
   try
   { process_started=false;
     server_proc=true;
     rbuffer=irbuffer;
     wbuffer=iwbuffer;
     if (BigEndianMachine()) endianess=1;
      else endianess=0;
     so=new Socket(&portnumber);          // create TCP/IP socket (server side)
   }
   catch (...)
    { if (so != NULL) { delete so;
                        so=NULL;
                      }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create socket (client side).
    \param[in] hostname     name of computer at server side
    \param[in] portnumber   port number of server
    \param[in] irbuffer     buffer for received data
    \param[in] iwbuffer     buffer for data to send

    Create socket (client side).
 */
/*---------------------------------------------------------------------------*/
CommSocket::CommSocket(const std::string hostname,
                       const unsigned short int portnumber,
                       StreamBuffer * const irbuffer,
                       StreamBuffer * const iwbuffer)
 { so=NULL;
   try
   { process_started=false;
     server_proc=false;
     rbuffer=irbuffer;
     wbuffer=iwbuffer;
     if (BigEndianMachine()) endianess=1;
      else endianess=0;
     so=new Socket(hostname, portnumber); // create TCP/IP socket (client side)
   }
   catch (...)
    { if (so != NULL) { delete so;
                        so=NULL;
                      }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy socket.

    Destroy socket. If a client process was started, the destructor waits for
    the client to exit.
 */
/*---------------------------------------------------------------------------*/
CommSocket::~CommSocket()
 {                                              // wait until client terminates
   if (process_started)
#ifdef WIN32
    WaitForSingleObject(pi.hProcess, INFINITE);
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
    { int status;

      waitpid(child_pid, &status, 0);
    }
#endif
   if (so != NULL) delete so;
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Write data to socket buffer.
    \param[in] data   data to be written
    \return object with data

    Write data to socket buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
CommSocket & CommSocket::operator << (const T data)
 { writeData(&data, 1);
   return(*this);
 }

#ifndef _COMM_SOCKET_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Write "endl" to socket buffer (that means: send the buffer content).
    \return object with data

    Write "endl" to socket buffer (that means: send the buffer content). The
    buffer is not deleted. If the same message has to be send to another
    process, there is no need to refill the buffer.
 */
/*---------------------------------------------------------------------------*/
CommSocket & CommSocket::operator << (std::ostream & (*)(std::ostream &))
 { msgSend();
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write C string to socket buffer.
    \param[in] str   C string to be written
    \return object with data

    Write C string to socket buffer.
 */
/*---------------------------------------------------------------------------*/
CommSocket & CommSocket::operator << (const char * const str)
 { unsigned long int length;

   length=(unsigned long int)strlen(str);
   *this << length;                                      // write string length
   writeData((unsigned char *)str, length);                     // write string
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write C++ string to socket buffer.
    \param[in] str   C++ string to be written
    \return object with data

    Write C++ string to socket buffer.
 */
/*---------------------------------------------------------------------------*/
CommSocket & CommSocket::operator << (const std::string str)
 { *this << (unsigned long int)str.length();
   writeData(str.c_str(), str.length());
   return(*this);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Read data from socket buffer.
    \param[in,out] data   data to be read
    \return object without data

    Read data from socket buffer. The data is converted between big- and
    little-endian, if required.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
CommSocket & CommSocket::operator >> (T &data)
 { readData(&data, 1);
   if (swap_data) Swap(&data);
   return(*this);
 }

#ifndef _COMM_SOCKET_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Read C string from socket buffer.
    \param[in,out] str   C string
    \return object without data

    Read C string from socket buffer. Enough memory for the string must already
    be allocated.
 */
/*---------------------------------------------------------------------------*/
CommSocket & CommSocket::operator >> (char * const str)
 { unsigned long int length;

   *this >> length;                                       // read string length
   readData(str, length);
   str[length]=0;
   return(*this);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read C++ string from socket buffer.
    \param[out] str   C++ string
    \return object without data

    Read C++ string from socket buffer.
 */
/*---------------------------------------------------------------------------*/
CommSocket & CommSocket::operator >> (std::string &str)
 { unsigned char *buffer=NULL;

   try
   { unsigned long int length;

     *this >> length;
     buffer=new unsigned char[length+1];
     readData(buffer, length);
     buffer[length]=0;
     str=std::string((const char *)buffer);
     delete[] buffer;
     buffer=NULL;
     return(*this);
   }
   catch (...)
    { if (buffer != NULL) delete[] buffer;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Close socket connection to client.

    Close socket connection to client.
 */
/*---------------------------------------------------------------------------*/
void CommSocket::closeConnectionToClient() const
 { if (server_proc) so->closeMsgSocket();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Receive message.

    Receive message from socket and fill it into the buffer. The old buffer
    content is deleted.
 */
/*---------------------------------------------------------------------------*/
void CommSocket::msgRecv()
 { char *hdr=NULL;

   try
   { unsigned long int length;
                                                      // receive message header
     hdr=new char[1+sizeof(unsigned long int)];
     so->readFromSock(hdr, 1+sizeof(unsigned long int));
     rbuffer->BigEndian(hdr[0] == 1);                   // is data big endian ?
     memcpy(&length, &hdr[1], sizeof(unsigned long int));
     delete[] hdr;
     hdr=NULL;
     swap_data=((endianess == 1) != rbuffer->BigEndian());   // need swapping ?
     if (swap_data) Swap(&length);
     rbuffer->resizeBuffer(length);
                       // receive message (remaining data in buffer is deleted)
     so->readFromSock((char *)rbuffer->start(), length);
     rbuffer->setWritePtr(length);
     rbuffer->resetReadPtr();
   }
   catch (...)
    { if (hdr != NULL) delete[] hdr;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is there a message waiting in the socket ?
    \return message is available

    Is there a message waiting in the socket ?
 */
/*---------------------------------------------------------------------------*/
bool CommSocket::msgReceived() const
 {                              // wait 1 microsecond for data if not available
   return(so->msgReceived(0, 1));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Is there a message waiting in the socket after the given number of
           seconds ?
    \param[in] seconds   number of seconds to wait
    \return message is available

    Is there a message waiting in the socket after the given number of
    seconds ?
 */
/*---------------------------------------------------------------------------*/
bool CommSocket::msgReceived(const unsigned short int seconds) const
 {                                 // wait given time for data if not available
   return(so->msgReceived(seconds, 1));
 }

/*---------------------------------------------------------------------------*/
/*! \brief Send buffer content.

    Send buffer content. The buffer is not deleted. If the same message has to
    be send to another process, there is no need to refill the buffer.
 */
/*---------------------------------------------------------------------------*/
void CommSocket::msgSend()
 { char *my_buf=NULL;

   try
   { unsigned long int size;

     if ((size=wbuffer->bufferSize()) == 0) return;           // buffer empty ?
                            // message consists of information about endianess,
                            // length information and data
     my_buf=new char[1+size+sizeof(unsigned long int)];
     my_buf[0]=endianess;
     memcpy(my_buf+1, &size, sizeof(unsigned long int));     // size of message
     memcpy(my_buf+1+sizeof(unsigned long int),
            (char *)wbuffer->start(), size);                    // copy message
     so->writeToSock(my_buf, 1+size+sizeof(unsigned long int)); // send message
                 // We could send the endianess, length and the data with three
                 // WriteToSock-operations and wouldn't need my_buf. But this
                 // would slow down the communication because bandwidth is very
                 // limited with small data packages.
     delete[] my_buf;
     my_buf=NULL;
   }
   catch (...)
    { if (my_buf != NULL) delete[] my_buf;
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request ID of message socket.
    \return ID of message socket

    Request ID of message socket.
 */
/*---------------------------------------------------------------------------*/
signed long int CommSocket::msgSock()
 { return(sock);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Clear buffer for new message.

    Clear buffer for new message.
 */
/*---------------------------------------------------------------------------*/
void CommSocket::newMessage()
 { wbuffer->resetBuffer();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request port number of server.
    \return port number of server

    Request port number of server
 */
/*---------------------------------------------------------------------------*/
unsigned short int CommSocket::portNumber() const
 { return(portnumber);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Read data from buffer.
    \param[in,out] data   data from buffer
    \param[in]     size   number of data elements
    \return got the requested amount of data ?

    Read data from buffer. If not enough data is in the buffer, the buffer is
    refilled by data from the socket. The data is converted between little- and
    big-endian if required.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void CommSocket::readData(T * const data, const unsigned long int size)
 { if (!rbuffer->testData(data, size)) msgRecv();
   rbuffer->readData(data, size);
   if (swap_data)
    for (unsigned long int i=0; i < size; i++)
     Swap(&data[i]);
 }

#ifndef _COMM_SOCKET_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Request socket ID.
    \return socket ID

    Request socket ID.
 */
/*---------------------------------------------------------------------------*/
signed long int CommSocket::socketID() const
 { return(so->socketID());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until client connects or timeout.
    \param[in] sec    time to wait (seconds)
    \param[in] usec   time to wait (microseconds)
    \return connection established ?

    Wait until client connects or timeout. This method should only be called by
    the server.
 */
/*---------------------------------------------------------------------------*/
bool CommSocket::waitForConnect(const time_t sec, const time_t usec)
 { if (!server_proc) return(false);
   return((sock=so->openMsgsock(sec, usec)) != Socket::NO_CONNECT_REQUEST);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Write data to buffer.
    \param[in] data   data to write
    \param[in] size   number of data elements

    Write data to buffer.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
void CommSocket::writeData(const T * const data, const unsigned long int size)
 { wbuffer->writeData(data, size);
 }
