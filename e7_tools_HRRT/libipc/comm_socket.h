/*! \file comm_socket.h
    \brief This class is used for buffered TCP/IP socket communication.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
    \date 2005/01/15 determine correct endianess of local machine
 */

#ifndef _COMM_SOCKET_H
#define _COMM_SOCKET_H

#include <iostream>
#include <string>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef __LINUX__
#include <sys/types.h>
#include <unistd.h>
#endif
#include "sockets.h"
#include "stream_buffer.h"

/*- definitions -------------------------------------------------------------*/

class CommSocket
 { private:
    StreamBuffer *rbuffer,                               /*!< receive buffer */
                 *wbuffer;                                  /*!< send buffer */
    bool server_proc,        /*!< is this process the server of the socket ? */
         process_started,       /*!< did the server process start a client ? */
         swap_data;             /*!< swap data when retrieving from buffer ? */
    unsigned short int portnumber;            /*!< port number of the server */
    Socket *so;                                           /*!< TCP/IP socket */
    signed long int sock;                          /*!< ID of message socket */
    char endianess;                          /*!< endianess of local machine */
#ifdef WIN32
    PROCESS_INFORMATION pi;    /*!< information about started client process */
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
    pid_t child_pid;           /*!< information about started client process */
#endif
   public:
                                                // create server side of socket
    CommSocket(const std::string, const std::string, const std::string,
               const bool, const time_t, const time_t, StreamBuffer * const,
               StreamBuffer * const);
    CommSocket(const unsigned short int, StreamBuffer * const,
               StreamBuffer * const);
    CommSocket(StreamBuffer * const, StreamBuffer * const);
                                                // create client side of socket
    CommSocket(const std::string, const unsigned short int,
               StreamBuffer * const, StreamBuffer * const);
    ~CommSocket();                                            // destroy socket
    template <typename T>
     CommSocket & operator << (const T);                // write data to socket
                               // write "endl" to buffer -> send buffer content
    CommSocket & operator << (std::ostream & (*)(std::ostream &));
    CommSocket & operator << (const char * const);    // write string to buffer
    CommSocket & operator << (const std::string);
    template <typename T>
     CommSocket & operator >> (T &);                   // read data from buffer
    CommSocket & operator >> (char * const);         // read string from buffer
    CommSocket & operator >> (std::string &);
    void closeConnectionToClient() const;         // close connection to client
    void msgRecv();                          // receive message and fill buffer
    bool msgReceived() const;                                // data received ?
    bool msgReceived(const unsigned short int) const;
    void msgSend();                                      // send buffer content
    signed long int msgSock();                  // request ID of message socket
    void newMessage();                          // clear buffer for new message
    unsigned short int portNumber() const;     // request port number of server
    template <typename T>
     void readData(T * const, const unsigned long int);// read data from buffer
    signed long int socketID() const;                      // request socket ID
                                                  // wait until client connects
    bool waitForConnect(const time_t, const time_t);
    template <typename T>                               // write data to buffer
     void writeData(const T * const, const unsigned long int);
 };

#ifndef _COMM_SOCKET_CPP
#define _COMM_SOCKET_TMPL_CPP
#include "comm_socket.cpp"
#endif

#endif
