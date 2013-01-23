/*! \file timestamp.h
    \brief This code is used to add a timestamp together with information about
           the used compiler to an executable.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/05 added Doxygen style comments
 */

#include <iostream>
#include <string>
#include <ctime>
#include "const.h"
#include "str_tmpl.h"

/*---------------------------------------------------------------------------*/
/*! \brief Add timestamp and compiler information to executable.
    \param prog   name of executable (including ".cpp" extension)
    \param date   copyright date

    Add timestamp and compiler information to executable.
 */
/*---------------------------------------------------------------------------*/
void timestamp(std::string prog, const std::string date)
 { std::string gmt_str;
   signed short int h;

#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   tm *tf;
   time_t t;

   t=time(NULL);
   tf=localtime(&t);
   h=tf->tm_gmtoff/3600;
   gmt_str=toString(h)+":"+toStringZero((tf->tm_gmtoff-h*3600)/60, 2);
#endif
#ifdef WIN32
   _tzset();
   h=(signed short int)-_timezone/3600;
   gmt_str=toString(h)+":"+toStringZero((-_timezone-h*3600)/60, 2);
#endif
   if (h >= 0) gmt_str="+"+gmt_str;
   prog.erase(0, prog.find_last_of("\\")+1);
   if (prog.length() >= 4)
    if (prog.substr(prog.length()-4) == ".cpp") prog.erase(prog.length()-4);
#ifdef WIN32
   prog+=".exe";
#endif
   while (prog.length() < 50+9-date.length()) prog+=" ";
   std::cout << "\n" << prog;
#ifdef WIN32
   std::cout << "(c)";
#endif
#if defined(__LINUX__) || defined(__SOLARIS__) || defined(__MACOSX__)
   std::cout << "  " << (char)169;
#endif
   std::cout << " " << date << " CPS Innovations\n\n"
             << " Revision: " << revnum/100 << "." << revnum%100 << "\n"
                                                // date and time of compilation
             << " compiled on: " << __DATE__ << "  " << __TIME__ << " (GMT"
             << gmt_str << ")\n"
                                                // compiler with version number
             << " compiler:    ";
#ifdef __INTEL_COMPILER
   std::cout << "Intel C++ " << __INTEL_COMPILER/100;
#endif
#ifndef __INTEL_COMPILER
#if _MSC_VER
   std::cout << "Microsoft Visual C++ ";
#if _MSC_VER == 1000
   std::cout << "4.0";
#elif _MSC_VER == 1010
   std::cout << "4.1";
#elif _MSC_VER == 1020
   std::cout << "4.2";
#elif _MSC_VER == 1100
   std::cout << "5.0";
#elif _MSC_VER == 1200
   std::cout << "6.0";
#elif _MSC_VER >= 1300
   std::cout << ".NET";
#else
   std::cout << "unknown version";
#endif
#endif
#ifdef __GNUG__
   std::cout << "GNU Compiler Collection C++ " << __VERSION__;
#endif
#endif
   std::cout << "\n" << std::endl;
 }
