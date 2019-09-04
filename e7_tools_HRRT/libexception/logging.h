/*! \file logging.h
    \brief This class is used to store logging information in an ASCII file or
           print it to screen.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2003/12/05 fixed deadlock in closing log-file at midnight
    \date 2004/04/03 use singleton pattern for object
    \date 2004/05/18 added Doxygen style comments
    \date 2004/08/20 added maxLoggingLevel() and loggingPath() methods
    \date 2004/09/15 check for SyngoCancel before every message
 */

#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <ctime>
#include "semaphore_al.h"

//#define MEMLOG

/*- class definitions -------------------------------------------------------*/

class Logging
 { private:
                                                 /*! parameters of a message */
    typedef struct { unsigned short int level; /*!< logging level of message */
                     std::string log_str;               /*!< text of message */
                     bool available;                /*!< message available ? */
                   } tlogmsg;
    static Logging *instance;    /*!< pointer to only instance of this class */
    Semaphore *lock;           /*!< semaphore to restrict access to log file */
    std::ofstream *file;                                       /*!< log file */
#ifdef MEMLOG
    std::ofstream *memfile;                   /*!< log file for memory usage */
#endif
    std::string path,                                  /*!< path of log file */
                fname;                   /*!< filename used in log file name */
    unsigned short int loglevel,           /*!< logging level of application */
                       creation_day;           /*!< day of log file creation */
                                     /*! destination for logging information */
    unsigned short int log_destination;
    bool logging_on;                                     /*!< log messages ? */
    tlogmsg msg;                                        /*!< current message */
#ifdef MEMLOG
    time_t timeoffset;                     /*!< start time of memory logging */
#endif
    void close(const bool);                                   // close log file
    void getPath();
#ifdef MEMLOG
    void saveMemUsage();                 // save information about memory usage
#endif
                                                    // convert time into string
    std::string timeStr(unsigned short int * const) const;
   protected:
    Logging();                                                 // create object
    ~Logging();                                               // destroy object
   public:
                                            // places to output log information
    static const unsigned short int LOG_FILE=1;             /*!< log to file */
    static const unsigned short int LOG_SCREEN=2;         /*!< log to screen */
    template <typename T>
     Logging *arg(T);                          // fill argument into log string
    static void close();                                      // close log file
    static Logging *flog();                // get pointer to instance of object
                                                               // open log file
    void init(const std::string, const unsigned short int, const std::string,
              const unsigned short int);
    void logCmdLine(int, char **) const;            // log command line options
    void loggingOn(const bool);                        // switch logging on/off
    std::string loggingPath() const;                    // request logging path
    Logging *logMsg(const std::string, const unsigned short int);// log message
    unsigned short int maxLoggingLevel() const;// request maximum logging level
 };
