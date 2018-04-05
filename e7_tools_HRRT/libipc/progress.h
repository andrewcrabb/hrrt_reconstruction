/*! \file progress.h
    \brief This class is used to pass progress information and error messages
           from the e7-tools to the reconstruction server.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/07/27 initial version
    \date 2005/01/04 added arg() method
 */

#pragma once

#include <string>
#include "comm_socket.h"
#include "stream_buffer.h"

/*= class definitions -------------------------------------------------------*/

class Progress
 { private:
    static Progress *instance;   /*!< pointer to only instance of this class */
         // size of receive buffer for communication with reconstruction server
    static const unsigned long int RECV_BUFFER_SIZE,
            // size of send buffer for communication with reconstruction server
                                   SEND_BUFFER_SIZE;
                                     // IP number of computer for queue process
    static const std::string localhost;
    CommSocket *rs; /*!< socket for communication with reconstruction server */
             /*! buffer for messages received from the reconstruction server */
    StreamBuffer *rbuffer,
                   /*! buffer for messages send to the reconstruction server */
                 *wbuffer;
    std::string msg;                  /*!< message with progress information */
    unsigned long int id;                                    /*!< message id */
    unsigned short int group;                             /*!< message group */
   protected:
    Progress();                                                // create object
    ~Progress();                                              // destroy object
   public:
    template <typename T>
     Progress *arg(T);                     // fill argument into message string
    static void close();                            // close communication link
                            // open communication link to reconstruction server
    void connect(const bool, const unsigned short int);
    static Progress *pro();                // get pointer to instance of object
                          // send a status message to the reconstruction server
    Progress *sendMsg(const unsigned long int, const unsigned short int,
                      const std::string);
 };
