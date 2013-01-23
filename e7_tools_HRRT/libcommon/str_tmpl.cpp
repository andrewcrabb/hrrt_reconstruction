/*! \file str_tmpl.cpp
    \brief This module provides utilities to convert numbers into C++ strings.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/01 added Doxygen style comments

    This module provides utilities to convert numbers into C++ strings. The
    values are written into a stringstream and read back as C++ string.
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#ifndef _STR_CPP
#define _STR_CPP
#include "str_tmpl.h"
#endif
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#include <windows.h>
#endif
#include "types.h"

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Convert value into string.
    \param[in] value   value that will be converted into a string
    \return string with value

    Convert value into string.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
std::string toString(const T value)
 {
#ifdef _MSC_VER
   if ((typeid(T) == typeid(uint64)) ||
       (typeid(T) == typeid(unsigned __int64)))
    { std::string s=std::string();

      while (*(uint64 *)&value > 0)
       { s=toString((unsigned short int)(*(uint64 *)&value % 10))+s;
         *(uint64 *)&value/=10;
       }
      if (s.empty()) return("0");
      return(s);
    }
   if ((typeid(T) == typeid(int64)) ||
       (typeid(T) == typeid(signed __int64)))
    if (*(int64 *)&value < 0)
     { uint64 v;

       v=-*(int64 *)&value;
       return("-"+toString(v));
     }
     else return(toString(*(uint64 *)&value));
#endif
   std::stringstream ostr;

#ifndef _MSC_VER
   ostr << value;
#endif
#ifdef _MSC_VER
   if (typeid(T) == typeid(float)) ostr << *(float *)&value;
    else if (typeid(T) == typeid(double)) ostr << *(double *)&value;
    else if (typeid(T) == typeid(unsigned short int))
          ostr << *(unsigned short int *)&value;
    else if (typeid(T) == typeid(signed short int))
          ostr << *(signed short int *)&value;
    else if (typeid(T) == typeid(unsigned int))
          ostr << *(unsigned int *)&value;
    else if (typeid(T) == typeid(signed int))
          ostr << *(signed int *)&value;
    else if (typeid(T) == typeid(unsigned long int))
          ostr << *(unsigned long int *)&value;
    else if (typeid(T) == typeid(signed long int))
          ostr << *(signed long int *)&value;
    else if (typeid(T) == typeid(std::string))
          ostr << *(std::string *)&value;
    else if (typeid(T) == typeid(unsigned char *))
          ostr << *(unsigned char **)&value;
    else if (typeid(T) == typeid(signed char *))
          ostr << *(signed char **)&value;
    else std::cerr << "output of this type not implemented ("
                   << sizeof(T) << ")" << std::endl;
#endif
   return(ostr.str());
 }

/*---------------------------------------------------------------------------*/
/*! \brief Convert value into string of fixed size.
    \param[in] value   value that will be converted into a string
    \param[in] len     length of string
    \return string with value

    Convert value into string of fixed size.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
std::string toString(const T value, const unsigned short int len)
 { std::stringstream ostr;
   std::streamsize old_len;

   old_len=ostr.width(len);                      // set width and get old width
   ostr << value;
   ostr.width(old_len);                                      // reset old width
   return(ostr.str());
 }

#ifndef _STR_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Convert integer into hexadecimal string.
    \param[in] value   integer value
    \return hexadecimal string

    Convert interger into hexadecimal string.
 */
/*---------------------------------------------------------------------------*/
std::string toStringHex(const unsigned long int value)
 { std::stringstream ostr;

   ostr << std::uppercase << std::hex << value;
   return(ostr.str());
 }
#endif

/*---------------------------------------------------------------------------*/
/*! \brief Convert value into string of fixed size with leading zeros.
    \param[in] value   value that will be converted into a string
    \param[in] len     length of string
    \return string with value

    Convert value into string of fixed size with leading zeros.
 */
/*---------------------------------------------------------------------------*/
template <typename T>
std::string toStringZero(const T value, const unsigned short int len)
 { std::string str;

   str=toString(value);
   while (str.length() < len) str="0"+str;
   return(str);
 }

#ifndef _STR_TMPL_CPP
/*---------------------------------------------------------------------------*/
/*! \brief Convert integer into hexadecimal string.
    \param[in] value   integer value
    \param[in] len     length of string
    \return hexadecimal string

    Convert interger into hexadecimal string of fixed size with leading zeros.
 */
/*---------------------------------------------------------------------------*/
std::string toStringZeroHex(const unsigned long int value,
                            const unsigned short int len)
 { std::string str;

   str=toStringHex(value);
   while (str.length() < len) str="0"+str;
   return(str);
 }
#endif

