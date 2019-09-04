/*! \file sockets.h
    \brief This class implements an abstraction layer for TCP/IP sockets.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
 */

#pragma once

#include <ctime>
#include <string>

/*- class definitions -------------------------------------------------------*/

class Socket
 { private:
                              /*! maximum number of connectors to one socket */
    static const int MAX_CONNECT=1;
    static const int SOCK_BUF_LEN=60000;         /*!< size of socket buffers */
    static const int INVALID_SOCKET=-1;                  /*!< invalid socket */
    static const int SOCKET_ERROR=-1;                      /*!< socket error */
    bool server_proc;           /*!< is this the server side of the socket ? */
    signed long int sock,                                 /*!< socket handle */
                    msgsock;                  /*!< handle for message socket */
    void setBufferSizes(const unsigned long int); // specify socket buffer size
   public:
                                    /*! no client is requesting a connection */
    static const int NO_CONNECT_REQUEST=-111;
                                          // create TCP/IP socket (client side)
    Socket(const std::string, const unsigned short int);
    Socket(unsigned short int * const);   // create TCP/IP socket (server side)
    Socket(const unsigned short int);     // create TCP/IP socket (server side)
    ~Socket();                                                // destroy socket
    void closeMsgSocket();                    // close message socket to client
    bool msgReceived(const time_t, const time_t) const;   // message received ?
    int openMsgsock(const time_t, const time_t);       // create message socket
    int openMsgsock();                         // open message socket to client
    void readFromSock(char *, unsigned long int);      // read data from socket
    signed long int socketID() const;                      // request socket ID
                                                        // write data to socket
    void writeToSock(const char * const, const unsigned long int);
 };
