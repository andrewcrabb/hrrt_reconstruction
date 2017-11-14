/*! \class Progress progress.h "progress.h"
    \brief This class is used to pass progress information and error messages
           from the e7-tools to the reconstruction server.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2004/07/27 initial version
    \date 2005/01/04 added arg() method

    The class establishes an IP socket connection between the e7-tool and the
    reconstruction server. Messages consisting of an id number and a text are
    send to the server.

    The class is implemented as a singleton object. Only one instance of this
    class can exist in a given application.
 */

#ifndef _PROGRESS_CPP
#define _PROGRESS_CPP
#include "progress.h"
#endif

/*- constants ---------------------------------------------------------------*/

#ifndef _PROGRESS_TMPL_CPP
                         /*! IP number of computer for reconstruction server */
const std::string Progress::localhost="127.0.0.1";
                          /*! size of receive buffer in bytes for communication
                              with reconstruction server */
const unsigned long int Progress::RECV_BUFFER_SIZE=1000;
                             /*! size of send buffer in bytes for communication
                                 with reconstruction server */
const unsigned long int Progress::SEND_BUFFER_SIZE=1000;

/*- methods -----------------------------------------------------------------*/

Progress *Progress::instance=NULL;

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Progress::Progress()
 { rs=NULL;
   rbuffer=NULL;
   wbuffer=NULL;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Progress::~Progress()
 { if (rs != NULL) delete rs;
   if (rbuffer != NULL) delete rbuffer;
   if (wbuffer != NULL) delete wbuffer;
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Fill parameter into progress string.
    \param[in] param   parameter to fill into progress string
    \return progress class with new progress string

    Fill parameter into progress string.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
Progress *Progress::arg(T param)
 { if (rs == NULL) return(this);
   std::string::size_type p;
        //  search fill position in message string ('#' followed by any number)
   if ((p=msg.find("#")) != std::string::npos)
    { std::stringstream os;

      msg+="*";
      os << msg.substr(0, p) << param
         << msg.substr((unsigned int)p+
                       (unsigned int)
                       (msg.substr(p+1).find_first_not_of("0123456789"))+1);
      msg=os.str().substr(0, os.str().length()-1);
      if (msg.find("#") == std::string::npos)
       { rs->newMessage();
         *rs << id << group << msg << std::endl;
       }
    }
   return(this);
 }

#ifndef _PROGRESS_TMPL_CPP
#ifdef WIN32
/*---------------------------------------------------------------------------*/
/*! \brief Fill GUID into progress message.
    \param[in] param   GUID to fill into progress message
    \return pointer to progress object

    Fill GUID into progress message.
 */
/*---------------------------------------------------------------------------*/
Progress *Progress::arg(GUID param)
 { OLECHAR gstr[128];

   ::StringFromGUID2(param, gstr, 128);
   return(arg(std::string((LPCTSTR)CString(gstr))));
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Delete instance of object.

    Send message with id '0' to the reconstruction server and close connection.
 */
/*---------------------------------------------------------------------------*/
void Progress::close()
 { if (instance != NULL)
    { instance->sendMsg(0, 0, "end");
#ifdef WIN32
      Sleep(1);
#endif
#if defined(__linux__) || defined(__SOLARIS__) || defined(__MACOSX__)
      sleep(1);
#endif
      delete instance;
      instance=NULL;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Connect to reconstruction server.
    \param[in] use_recoserver   is there a reconstruction server ?
    \param[in] port             port number on which the server is listening

    Establish a socket connection to the reconstruction server.
 */
/*---------------------------------------------------------------------------*/
void Progress::connect(const bool use_recoserver,
                       const unsigned short int port)
 { if (use_recoserver)
    { if (rs != NULL) delete rs;
      if (rbuffer != NULL) delete rbuffer;
      if (wbuffer != NULL) delete wbuffer;
                                                          // initialize buffers
      rbuffer=new StreamBuffer(RECV_BUFFER_SIZE);
      wbuffer=new StreamBuffer(SEND_BUFFER_SIZE);
      rs=new CommSocket(localhost, port, rbuffer, wbuffer);      // open socket
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to instace of progress object.
    \return pointer to instance of progress object

    Request pointer to the only instance of the progress object. The progress
    object is a singleton object. If it is not initialized, the constructor
    will be called.
 */
/*---------------------------------------------------------------------------*/
Progress *Progress::pro()
 { if (instance == NULL) instance=new Progress();
   return(instance);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Send message to the reconstruction server.
    \param[in] _id      message id
    \param[in] _group   ID of subsystem that sends the message
    \param[in] _msg     message text

    Send message to the reconstruction server.
 */
/*---------------------------------------------------------------------------*/
Progress *Progress::sendMsg(const unsigned long int _id,
                            const unsigned short int _group,
                            const std::string _msg)
 { if (rs == NULL) return(this);
   msg=_msg;
   id=_id;
   group=_group;
   if (msg.find("#") == std::string::npos)
    { rs->newMessage();
      *rs << id << group << msg << std::endl;
    }
   return(this);
 }
#endif

