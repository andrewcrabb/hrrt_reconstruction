//** file: red.h
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

//** This class implements the Remote Execution Daemon.

/*- usage examples ------------------------------------------------------------

 Red *red;

 red=new Red();
 while (!red->recvAndInterpretMsg()) ;
 delete red;

-------------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/

#pragma once

#include "comm_socket.h"
#include "stream_buffer.h"

/*- class definitions -------------------------------------------------------*/

class Red
 { private:
    StreamBuffer *rbuffer,      // buffer for messages received from the socket
                 *wbuffer;             // buffer for data to send to the socket
    CommSocket *rc;                      // communication channel to the client
   public:
    Red();                                                 // initialize object
    ~Red();                                                   // destroy object
    bool recvAndInterpretMsg();               // receive and interpret messages
 };

