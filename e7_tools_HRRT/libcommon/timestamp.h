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
#include "e7_tools_const.h"
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

   tm *tf;
   time_t t;

   t=time(NULL);
   tf=localtime(&t);
   h=tf->tm_gmtoff/3600;
   gmt_str=toString(h)+":"+toStringZero((tf->tm_gmtoff-h*3600)/60, 2);

   if (h >= 0) gmt_str="+"+gmt_str;
   prog.erase(0, prog.find_last_of("\\")+1);
   if (prog.length() >= 4)
    if (prog.substr(prog.length()-4) == ".cpp") prog.erase(prog.length()-4);
   while (prog.length() < 50+9-date.length()) prog+=" ";
   std::cout << "\n" << prog;
   std::cout << "  " << (char)169;
   std::cout << " " << date << " CPS Innovations\n\n"
             << " Revision: " << revnum/100 << "." << revnum%100 << "\n"
                                                // date and time of compilation
             << " compiled on: " << __DATE__ << "  " << __TIME__ << " (GMT"
             << gmt_str << ")\n"
                                                // compiler with version number
             << " compiler:    ";
   std::cout << "\n" << std::endl;
 }
