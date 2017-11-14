/*! \class Socket sockets.h "sockets.h"
    \brief This class implements an abstraction layer for TCP/IP sockets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2003/11/17 initial version
    \date 2004/03/26 added Doxygen style comments
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class implements an abstraction layer for the usage of TCP/IP sockets
    (TCP Protokoll, OSI-Layer 4). The constructors open TCP/IP sockets on a
    free port number or a specified port number and listen on this socket for a
    client. The client connects to the server on the given port number. The
    class contains also methods to send and recv data over the socket and to
    test for incoming data. More information about socket programming can be
    found in: "Unix Network Programming", W. Richard Stevens, Prentice Hall,
    Vol. 1, 2nd. ed., 1997, ISBN 0-13-490012-X
 */

/*! \example example_socket.cpp
    Here are some examples of how to use the Socket class.
 */

#include <iostream>
#include <string>
#ifdef WIN32
#include <winsock2.h>
#include <io.h>
#endif
#ifdef __MACOSX__
#include <netinet/in.h>
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif
#include "sockets.h"
#include "exception.h"
#include "swap_tmpl.h"
#include <string.h>


/*- definitions -------------------------------------------------------------*/

#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
#define closesocket close
#define SD_SEND 2
#endif

/*- methods -----------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Create a TCP/IP socket (client side).
    \param[in] server_ip     IP number of computer at server side
    \param[in] port_number   port of computer at server side
    \exception REC_SOCKET_ERROR_INIT can't initialize winsock.dll
    \exception REC_SOCKET_ERROR_CREATE_C client can't create TCP/IP socket
    \exception REC_SOCKET_ERROR_CONNECT client can't connect to the server

    Create the client side of a TCP/IP socket and connect to the server.
 */
/*---------------------------------------------------------------------------*/
Socket::Socket(const std::string server_ip,
               const unsigned short int port_number)
 { struct sockaddr_in server;

#ifdef WIN32
   { WSADATA wsaData;
                                                      // initialize winsock.dll
     if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR)
      throw Exception(REC_SOCKET_ERROR_INIT,
                      "Initialization of winsock.dll failed.");
   }
#endif
                                                               // create socket
   if ((sock=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_CREATE_C,
                      "Client can't create TCP/IP socket.");
    }
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   setBufferSizes(sock);
#endif
                                               // build TCP/IP socket to server
   memset(&server, 0, sizeof(server));
   server.sin_family=AF_INET;
   server.sin_addr.s_addr=inet_addr(server_ip.c_str());  // IP number of server
   server.sin_port=htons(port_number);                 // port number of server
                                                           // connect to server
   if (connect(sock, (struct sockaddr *)&server,
               sizeof(server)) == SOCKET_ERROR)
    { closesocket(sock);
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_CONNECT, "Client can't establish "
                      "connection to server on TCP/IP socket.");
    }
   server_proc=false;              // this process is the client of this socket
   msgsock=sock;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a TCP/IP socket (server side) on arbitrary port.
    \param[in] port_number   port number used by the server
    \exception REC_SOCKET_ERROR_INIT can't initialize winsock.dll
    \exception REC_SOCKET_ERROR_CREATE_S server can't create TCP/IP socket
    \exception REC_SOCKET_ERROR_CONF can't change the socket configuration
    \exception REC_SOCKET_ERROR_BIND can't bind socket to port
    \exception REC_SOCKET_ERROR_GET_PORT can't get port number for this socket
    \exception REC_SOCKET_ERROR_LISTEN can't listen on this socket for a client

    Create the server side of a TCP/IP socket and listen on an arbitrary port
    for a client from any IP number.
 */
/*---------------------------------------------------------------------------*/
Socket::Socket(unsigned short int * const port_number)
 { struct sockaddr_in server;

#ifdef WIN32
   { WSADATA wsaData;
                                                      // initialize winsock.dll
     if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR)
      throw Exception(REC_SOCKET_ERROR_INIT,
                      "Initialization of winsock.dll failed.");
   }
#endif
                                                               // create socket
   if ((sock=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_CREATE_S,
                      "Server can't create TCP/IP socket.");
    }
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   setBufferSizes(sock);
   { const int on=1;

     if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
                    sizeof(signed long int)) < 0)
      throw Exception(REC_SOCKET_ERROR_CONF, "Can't configure TCP/IP socket.");
   }
#endif
                                                         // build TCP/IP socket
   memset(&server, 0, sizeof(server));
   server.sin_family=AF_INET;
                           // allow connections to any IP number of this server
   server.sin_addr.s_addr=htonl(INADDR_ANY);
   server.sin_port=htons(INADDR_ANY);                  // allow any port number
                                         // bind socket to a port of the server
   if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    { closesocket(sock);
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_BIND,
                      "Server can't bind TCP/IP socket to port.");
    }
   {
#if defined(WIN32) || defined(__MACOSX__) || defined(__SOLARIS__)
     int length;
#endif
#if defined(__linux__)
     socklen_t length;
#endif
                                                // request the used port number
     length=sizeof(server);
     // ahc
     // if (getsockname(sock, (struct sockaddr *)&server, &length) == SOCKET_ERROR) { 
     if (getsockname(sock, (struct sockaddr *)&server, (unsigned int *)&length) == SOCKET_ERROR) { 
       closesocket(sock);
#ifdef WIN32
        WSACleanup();
#endif
        throw Exception(REC_SOCKET_ERROR_GETPORT,
			"Server can't determine port assigned to TCP/IP socket.");
     }
     *port_number=ntohs(server.sin_port);            // return used port number
   }
                                        // listen on port for connecting client
   if (listen(sock, MAX_CONNECT) == SOCKET_ERROR)
    { closesocket(sock);
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_LISTEN,
                      "Server can't listen on TCP/IP socket.");
    }
   server_proc=true;               // this process is the server of this socket
   msgsock=NO_CONNECT_REQUEST;                      // client not yet connected
 }

/*---------------------------------------------------------------------------*/
/*! \brief Create a TCP/IP socket (server side) on given port.
    \param[in] port_number   port number used by the server
    \exception REC_SOCKET_ERROR_INIT can't initialize winsock.dll
    \exception REC_SOCKET_ERROR_CREATE_S server can't create TCP/IP socket
    \exception REC_SOCKET_ERROR_CONF can't change the socket configuration
    \exception REC_SOCKET_ERROR_BIND can't bind socket to port
    \exception REC_SOCKET_ERROR_LISTEN can't listen on this socket for a client

    Create the server side of a TCP/IP socket and listen on the given port for
    a client from any IP number.
 */
/*---------------------------------------------------------------------------*/
Socket::Socket(const unsigned short int port_number)
 { struct sockaddr_in server;

#ifdef WIN32
   { WSADATA wsaData;
                                                      // initialize winsock.dll
     if (WSAStartup(MAKEWORD(1, 1), &wsaData) == SOCKET_ERROR)
      throw Exception(REC_SOCKET_ERROR_INIT,
                      "Initialization of winsock.dll failed.");
   }
#endif
                                                               // create socket
   if ((sock=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_CREATE_S,
                      "Server can't create TCP/IP socket.");
    }
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
   setBufferSizes(sock);
   { const int on=1;

     if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
                    sizeof(signed long int)) < 0)
      throw Exception(REC_SOCKET_ERROR_CONF, "Can't configure TCP/IP socket.");
   }
#endif
                                                         // build TCP/IP socket
   memset(&server, 0, sizeof(server));
   server.sin_family=AF_INET;
                           // allow connections to any IP number of this server
   server.sin_addr.s_addr=htonl(INADDR_ANY);
   server.sin_port=htons(port_number);
                  // assign protocol address (IP number, port number) to socket
   if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
    { closesocket(sock);
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_BIND,
                      "Server can't bind TCP/IP socket to port.");
    }
                                        // listen on port for connecting client
   if (listen(sock, MAX_CONNECT) == SOCKET_ERROR)
    { closesocket(sock);
#ifdef WIN32
      WSACleanup();
#endif
      throw Exception(REC_SOCKET_ERROR_LISTEN,
                      "Server can't listen on TCP/IP socket.");
    }
   server_proc=true;               // this process is the server of this socket
   msgsock=NO_CONNECT_REQUEST;                      // client not yet connected
 }

/*---------------------------------------------------------------------------*/
/*! \brief Close the socket.

    Close the socket.
 */
/*---------------------------------------------------------------------------*/
Socket::~Socket()
 { if (server_proc) { closeMsgSocket();                 // close message socket
                      closesocket(sock);              // close listening socket
                    }
    else { shutdown(msgsock, SD_SEND);                          // stop sending
           closesocket(msgsock);                                   // and close
         }
#ifdef WIN32
   WSACleanup();                                         // cleanup winsock.dll
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Close the message socket to the client.

    Close the message socket to the client. Data that is not yet read from the
    socket is discarded.
 */
/*---------------------------------------------------------------------------*/
void Socket::closeMsgSocket()
 { if (server_proc)                                              // server side
    if (msgsock != NO_CONNECT_REQUEST)
     { char buffer[1000];

       shutdown(msgsock, SD_SEND);                              // stop sending
                                                  // read until socket is empty
       while (recv(msgsock, buffer, 1000, 0) > 0) ;
       closesocket(msgsock);
       msgsock=NO_CONNECT_REQUEST;
     }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Perform a non-blocking test for a message.
    \param[in] sec    time to wait (seconds)
    \param[in] usec   time to wait (microseconds)
    \return mesage available ?

    Test the socket for a message. If the given time is over, the function
    returns.
 */
/*---------------------------------------------------------------------------*/
bool Socket::msgReceived(const time_t sec, const time_t usec) const
 { fd_set rfds;
   struct timeval timeout;
   unsigned long int tmp_sock;

   if (msgsock == NO_CONNECT_REQUEST) tmp_sock=sock;
    else tmp_sock=msgsock;
   FD_ZERO(&rfds);                                    // clear all bits in rfds
   FD_SET(tmp_sock, &rfds);                      // set bits for message socket
   timeout.tv_sec=sec;
   timeout.tv_usec=usec;                   // how long should I wait for data ?
#ifdef FDSETISINT
   if (select(tmp_sock+1, (int *)&rfds, (int *)0, (int *)0, &timeout) < 1)
#endif
#ifndef FDSETISINT
   if (select(tmp_sock+1, &rfds, (fd_set *)0, (fd_set *)0, &timeout) < 1)
#endif
    return(false);
   return(FD_ISSET(tmp_sock, &rfds) != 0);                   // data received ?
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait for a given time if a client connects.
    \param[in] sec    time to wait (seconds)
    \param[in] usec   time to wait (microseconds)
    \return socket handle or NO_CONNECT_REQUEST

    Wait for a given time if a client connects. If no client connects the
    value NO_CONNECT_REQUEST is returned, otherwise the handle of the message
    socket is returned.
 */
/*---------------------------------------------------------------------------*/
int Socket::openMsgsock(const time_t sec, const time_t usec)
 { if (server_proc)
    if (msgReceived(sec, usec)) return(openMsgsock());
   return(NO_CONNECT_REQUEST);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Wait until a client connects.
    \exception REC_SOCKET_ERROR_ACCEPT error in accepting the connection
    \return socket handle

    Wait until a client connects and return the handle of the message socket.
 */
/*---------------------------------------------------------------------------*/
int Socket::openMsgsock()
 {                                             // accept connection from client
   if (server_proc)
    if ((msgsock=accept(sock, NULL, NULL)) == INVALID_SOCKET)
     { msgsock=NO_CONNECT_REQUEST;
       throw Exception(REC_SOCKET_ERROR_ACCEPT, "Server can't accept requested"
                       " TCP/IP socket connection.");
     }
   return(msgsock);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Read data from socket.
    \param[in,out] buffer   buffer for received data
    \param[in]     size     number of bytes to receive
    \exception REC_SOCKET_ERROR_BROKEN the socket connection is broken

    Read the given amount of data from the socket.
 */
/*---------------------------------------------------------------------------*/
void Socket::readFromSock(char *buffer, unsigned long int size)
 {                       // read data until desired number of bytes is received
   do { signed long int rcv_len;
#ifdef WIN32
        rcv_len=recv(msgsock, buffer, size, 0);
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
        rcv_len=(signed long int)read(msgsock, buffer, size);
#endif
        if (rcv_len > 0) { buffer+=rcv_len;
                           size-=rcv_len;
                         }
         else if (rcv_len < 0)
               throw Exception(REC_SOCKET_ERROR_BROKEN,
                               "TCP/IP socket connection broken.");
               else if (rcv_len == 0) return;
      } while (size > 0);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Set the sizes of the receive and send buffers.
    \param[in] sock   socket handle
    \exception REC_SOCKET_ERROR_CONF can't change the socket configuration

    Set the sizes of the receive and send buffers.
 */
/*---------------------------------------------------------------------------*/
void Socket::setBufferSizes(const unsigned long int sock)
 { int bufsize=SOCK_BUF_LEN;
                                                     // set size of send buffer
   if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize,
                  sizeof(signed long int)) < 0)
    throw Exception(REC_SOCKET_ERROR_CONF, "Can't configure TCP/IP socket.");
                                                     // set size of recv buffer
   if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *)&bufsize,
                  sizeof(signed long int)) < 0)
    throw Exception(REC_SOCKET_ERROR_CONF, "Can't configure TCP/IP socket.");
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request the handle of the message socket.
    \return handle of the message socket

    Request the handle of the message socket.
 */
/*---------------------------------------------------------------------------*/
signed long int Socket::socketID() const
 { return(msgsock);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write data into socket.
    \param[in] buffer   data to send
    \param[in] size     number of bytes to send
    \exception REC_SOCKET_ERROR_BROKEN the socket connection is broken

    Write the given amount of data into the socket.
 */
/*---------------------------------------------------------------------------*/
void Socket::writeToSock(const char * const buffer,
                         const unsigned long int size)
 { if ((send(msgsock, buffer, size, 0)) == SOCKET_ERROR)
    throw Exception(REC_SOCKET_ERROR_BROKEN,
                    "TCP/IP socket connection broken.");
 }
