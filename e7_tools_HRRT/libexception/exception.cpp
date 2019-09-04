/*! \class Exception exception.h "exception.h"
    \brief This class implements an exception object.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments

    The constructor stores some information inside of the object, that can be
    retrieved by the exception handler. Error message are logged through the
    logging object.
 */

#pragma once

#include <iostream>
#include <sstream>
#include "exception.h"
#include "logging.h"

/*- methods -----------------------------------------------------------------*/

#ifndef _EXCEPTION_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Store error information in object.
    \param[in] _err_code   error code
    \param[in] _err_str    description of error

    Store error information in object.
 */
/*---------------------------------------------------------------------------*/
Exception::Exception(const unsigned long int _err_code,
                     const std::string _err_str)
 { err_code=_err_code;
   err_str=_err_str;
                     // log error message if there are no additional parameters
   if (err_str.find("#") == std::string::npos)
    Logging::flog()->logMsg("** "+err_str, 0);
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Fill parameter into error string.
    \param[in] param   parameter to fill into error string
    \return exception class with new error string

    Fill parameter into error string.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
Exception Exception::arg(T param)
 { std::string::size_type p;
           // search fill position in error string ('#' followed by any number)
   if ((p=err_str.find("#")) != std::string::npos)
    { std::stringstream os;

      err_str+="*";
      os << err_str.substr(0, p) << param
         << err_str.substr(
                      p+err_str.substr(p+1).find_first_not_of("0123456789")+1);
      err_str=os.str().substr(0, os.str().length()-1);
      if (err_str.find("#") == std::string::npos)
       Logging::flog()->logMsg("** "+err_str, 0);
    }
   return(*this);
 }

#ifndef _EXCEPTION_TMPL_CPP

/*---------------------------------------------------------------------------*/
/*! \brief Request error code.
    \return error code

    Request error code.
 */
/*---------------------------------------------------------------------------*/
unsigned long int Exception::errCode() const
 { return(err_code);
 }

/*---------------------------------------------------------------------------*/
/*! \brief Request error description.
    \return error description

    Request error description.
 */
/*---------------------------------------------------------------------------*/
std::string Exception::errStr() const
 { return(err_str);
 }
#endif

