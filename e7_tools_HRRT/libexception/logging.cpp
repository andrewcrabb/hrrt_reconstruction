/*! \class Logging logging.h "logging.h"
    \brief This class is used to store logging information in an ASCII file or
           print it to screen.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \author Merence Sibomana  - HRRT users community (sibomana@gmail.com)
    \author Peter M. Bloomfield - HRRT users community (peter.bloomfield@camhpet.ca)
    \date 2003/11/17 initial version
    \date 2003/12/05 fixed deadlock in closing log-file at midnight
    \date 2004/04/03 use singleton pattern for object
    \date 2004/05/18 added Doxygen style comments
    \date 2004/08/20 added maxLoggingLevel() and loggingPath() methods
    \date 2004/09/15 check for SyngoCancel before every message
    \date 2009/08/28 Port to Linux (peter.bloomfield@camhpet.ca)

    This class handles logging messages and writes them together with a
    timestamp to screen or/and file, if their logging level is below the
    threshold. Logging files are created for every day of the month. After one
    month the files are overwritten.

    The class is implemented as a singleton object. Only one instance of this
    class can exist in a given application. The class is thread safe.
 */

#include <iostream>
#include "logging.h"
#include "e7_tools_const.h"
#include "e7_common.h"
#include "exception.h"
#include "registry.h"
#include "str_tmpl.h"
#include "syngo_msg.h"
#include "timedate.h"
#include <stdlib.h>

/*- constants ---------------------------------------------------------------*/

                                            // places to output log information

/*- methods -----------------------------------------------------------------*/

#ifndef _LOGGING_TMPL_CPP
Logging *Logging::instance=NULL;     /*!< pointer to only instance of object */

/*---------------------------------------------------------------------------*/
/*! \brief Initialize object.

    Initialize object.
 */
/*---------------------------------------------------------------------------*/
Logging::Logging()
 {
   file=NULL;
   lock=NULL;
   path=std::string();
   try
   { lock=new Semaphore(1);           // initialize lock semaphore for log file
     loglevel=99;
     creation_day=0;
     logging_on=false;
     msg.available=false;
   }
   catch (...)
    { if (lock != NULL) { delete lock;
                          lock=NULL;
                        }
      throw;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Destroy object.

    Destroy object.
 */
/*---------------------------------------------------------------------------*/
Logging::~Logging()
 { if (lock != NULL) delete lock;
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Fill parameter into log message.
    \param[in] param   parameter to fill into log message
    \return pointer to logging object

    Fill parameter into log message.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
Logging *Logging::arg(T param)
 { if ((file == NULL) && !(log_destination & LOG_SCREEN)) return(this);
   if (!logging_on) return(this);
   if (!msg.available) return(this);
   std::string::size_type p;

   if (msg.level > loglevel) { msg.available=false;
                               lock->signal();
                               return(this);
                             }
                 // search fill position in string ('#' followed by any number)
   if ((p=msg.log_str.find("#")) != std::string::npos)
    { std::stringstream os;

      msg.log_str+="*";
                                          // put parameter into logging message
      os << msg.log_str.substr(0, p) << param
         << msg.log_str.substr(
                  p+msg.log_str.substr(p+1).find_first_not_of("0123456789")+1);
      msg.log_str=os.str().substr(0, os.str().length()-1);
                                                     // all parameters filled ?
      if (msg.log_str.find("#") == std::string::npos)
       { std::string tstr;
         unsigned short int day;

         tstr=timeStr(&day);                        // get current time and day
         if (day != creation_day)          // need to create file for new day ?
          init(fname, loglevel, path, log_destination);
         if ((file != NULL) && (log_destination & LOG_FILE))
          *file << tstr << msg.log_str << std::endl;    // write string to file
                                                      // write string to screen
         if (log_destination & LOG_SCREEN)
/*        { std::string str;

            str=tstr+msg.log_str;
            IDL_Message(IDL_M_GENERIC, IDL_MSG_INFO, str.c_str());
          }
 */
          std::clog << tstr << msg.log_str << std::endl;
         msg.available=false;
         lock->signal();
       }
    }
   return(this);
 }

#ifndef _LOGGING_TMPL_CPP

/*---------------------------------------------------------------------------*/
/*! \brief Delete instance of object.

    Close log file and delete instance of object.
 */
/*---------------------------------------------------------------------------*/
void Logging::close()
 { if (instance != NULL) { instance->close(true);
                           delete instance;
                           instance=NULL;
                         }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Close log file
    \param[in] get_lock   lock file before accessing ? (otherwise already
                          locked)

    Close log file.
 */
/*---------------------------------------------------------------------------*/
void Logging::close(const bool get_lock)
 { if (lock == NULL) return;
   if (get_lock) lock->wait();
   if (file != NULL) { file->close();
                       delete file;
                       file=NULL;
                     }
   if (get_lock) lock->signal();
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request pointer to instance of logging object.
    \return pointer to instance of logging object

    Request pointer to the only instance of the logging object. The logging
    object is a singleton object. If it is not initialized, the constructor
    will be called.
 */
/*---------------------------------------------------------------------------*/
Logging *Logging::flog()
 { if (instance == NULL) { instance=new Logging();
                           instance->getPath();
                         }
   return(instance);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Retrieve logging path from environment variable/registry.

    Retrieve logging path from environment variable/registry.
 */
/*---------------------------------------------------------------------------*/
void Logging::getPath()
 {
                                                       // get logging directory

                                  // get logging path from environment variable
   if (getenv(logging_path.c_str()) != NULL)
    path=std::string(getenv(logging_path.c_str()));

 }

/*---------------------------------------------------------------------------*/
/*! \brief Open log file and delete last months file if necessary.
    \param[in] name      filename used in log file name
    \param[in] logcode   level for logging
    \param[in] logpath   path for log file
    \param[in] ld        device for logging (screen, file)

    Open log file and delete last months file if necessary. Log files are
    always closed at the end of day and a new file is created for the next day.
    This function is not thread-safe.
 */
/*---------------------------------------------------------------------------*/
void Logging::init(const std::string name, const unsigned short int level,
                   const std::string logpath, const unsigned short int ld)
 { std::ifstream *ifile=NULL;

   try
   { std::string filename, old_filename;
     TIMEDATE::tdate dt;

     loggingOn(true);
     if (logpath != std::string()) path=logpath;
     loglevel=level;
     log_destination=ld;
     fname=name;
                                                        // create/open log file
     dt=TIMEDATE::currentDate();
     if ((log_destination & LOG_FILE) && (dt.day != creation_day))
      { close(false);                                         // close log file
        creation_day=dt.day;
        filename=path+"/log_"+name+"_"+toStringZero(dt.day, 2)+".txt";
        file=new std::ofstream(filename.c_str(), std::ios::out|std::ios::app);
        file->seekp(0, std::ios::end);
        if ((unsigned long int)file->tellp() == 0) // write date if file is new
         *file << "--- Log file '" << name << "'  "
               << toStringZero(dt.month, 2) << "/"
               << toStringZero(dt.day, 2) << "/"
               << toStringZero(dt.year, 4) << " ---" << std::endl;
         else { std::string str;
                                         // open existing log file for this day
                ifile=new std::ifstream(filename.c_str(), std::ios::in);
                std::getline(*ifile, str);
                ifile->close();
                delete ifile;
                ifile=NULL;
                                          // is file from same month and year ?
                if ((atoi(str.substr(str.length()-14,
                                     2).c_str()) != dt.month) ||
                    (atoi(str.substr(str.length()-8, 4).c_str()) != dt.year))
                 { file->close();
                   delete file;
                   file=NULL;
                   unlink(filename.c_str());             // delete old log file
                   file=new std::ofstream(filename.c_str(),
                                          std::ios::out|std::ios::app);
                   *file << "--- Log file '" << name << "'  "
                         << toStringZero(dt.month, 2) << "/"
                         << toStringZero(dt.day, 2) << "/"
                         << toStringZero(dt.year, 4) << " ---" << std::endl;
                 }
              }
      }
   }
   catch (...)
    { if (file != NULL) { delete file;
                          file=NULL;
                        }
      if (ifile != NULL) delete ifile;
    }
 }

/*---------------------------------------------------------------------------*/
/*! \brief Log command line options.
    \param[in] argc   number of arguments
    \param[in] argv   command line options

    Log current date and command line options.
 */
/*---------------------------------------------------------------------------*/
void Logging::logCmdLine(int argc, char **argv) const
 { std::string s, gmt_str;
   time_t t;
   TIMEDATE::ttime ti;

   ti=TIMEDATE::currentTime(NULL);
   gmt_str=toString(abs(ti.gmt_offset_h))+":"+
           toStringZero(abs(ti.gmt_offset_m), 2);
   if ((ti.gmt_offset_h < 0) || (ti.gmt_offset_m < 0)) gmt_str="-"+gmt_str;
    else gmt_str="+"+gmt_str;
   flog()->logMsg("code compiled on: #1 #2 (GMT#3)", 0)->arg(__DATE__)->
    arg(__TIME__)->arg(gmt_str);
   flog()->logMsg("Revision: #1.#2", 0)->arg(revnum/100)->arg(revnum%100);
   time(&t);
   s=std::string(asctime(gmtime((time_t *)&t)));
   flog()->logMsg("current date and time (GMT): #1", 0)->
    arg(s.substr(0, s.length()-1));
   s=std::string();
   for (unsigned short int i=0; i < argc; i++)
    s+=std::string(argv[i])+" ";
   flog()->logMsg("command line: #1", 0)->arg(s);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Switch logging on/off.
    \param[in] _logging_on   switch logging on ?

    Switch logging on/off.
 */
/*---------------------------------------------------------------------------*/
void Logging::loggingOn(const bool _logging_on)
 { logging_on=_logging_on;
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request logging peth.
    \return logging path

    Request logging path.
 */
/*---------------------------------------------------------------------------*/
std::string Logging::loggingPath() const
 { return(path);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Write log message to file or screen.
    \param[in] msgstr   text of message
    \param[in] _level   logging level for message
    \return this object
    \exception REC_SYNGO_CANCEL processing canceled

    Write log message to file or screen.
 */
/*---------------------------------------------------------------------------*/
Logging *Logging::logMsg(const std::string msgstr,
                         const unsigned short int _level)
{
  if (SyngoCancel())
    throw Exception(REC_SYNGO_CANCEL, "processing canceled");
  if ((file == NULL) && !(log_destination & LOG_SCREEN))
    return(this);
  if (!logging_on)
    return(this);
  lock->wait();
  // create data structure for message
  msg.level=_level;
  msg.log_str=std::string();
  for (unsigned short int i=0; i < msg.level; i++)
    msg.log_str+=" ";
  msg.log_str+=msgstr;
  msg.available=true;
  // write message to file if it doesn't contain arguments
  if (msg.log_str.find("#") == std::string::npos) {
    std::string tstr;
    unsigned short int day;

    // don't write in this log level
    if (msg.level > loglevel) {
      msg.available=false;
      lock->signal();
      return(this);
    }
    // write log message
    tstr=timeStr(&day);
    if (day != creation_day)                       // create file for new day
      init(fname, loglevel, path, log_destination);
    // write to file
    if ((file != NULL) && (log_destination & LOG_FILE))
      *file << tstr << msg.log_str << std::endl;
    // write to screen
    if (log_destination & LOG_SCREEN)
      std::clog << tstr << msg.log_str << std::endl;
    lock->signal();
  }
  return(this);
}

/*---------------------------------------------------------------------------*/
/*! \brief Request maximum logging level.
    \return maximum logging level

    Request maximum logging level.
 */
/*---------------------------------------------------------------------------*/
unsigned short int Logging::maxLoggingLevel() const
 { return(loglevel);
 }


/*---------------------------------------------------------------------------*/
/*! \brief Convert current time into string.
    \param[out] day   current day
    \return string with time

    Convert current time into string.
 */
/*---------------------------------------------------------------------------*/
std::string Logging::timeStr(unsigned short int * const day) const
 {
   TIMEDATE::ttime ti;
   TIMEDATE::tdate dt;
   unsigned short int msec;

   ti=TIMEDATE::currentTime(&msec);
   dt=TIMEDATE::currentDate();
   *day=dt.day;
                                             // create string with current time
   return(toStringZero(ti.hour, 2) + ":" + toStringZero(ti.minute, 2) + ":" +
          // toStringZero(ti.second, 2) + "." + toStringZero(msec, 6) + " ");
          // ahc: Took out useless 6-digit fraction.
          toStringZero(ti.second, 2) + " ");
 }
#endif
