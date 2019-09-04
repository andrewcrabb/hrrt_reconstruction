//** file: red_client.h
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

//** This module provides functions for the communication with a Remote
//** Execution Daemon.

/*- usage examples ------------------------------------------------------------

 if (RED_ping("10.1.1.34"))                          // test if RED is working
  {                                     // install executable on REDs computer
    RED_install("10.1.1.34", "/home/test/myprog", "/home/dest/exec");
    RED_execute("10.1.1.34", "/home/dest/exec/myprog");     // execute program
    RED_terminate("10.1.1.34");                               // terminate RED
  }

-------------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/


#pragma once

#include <string>

/*- exported functions ------------------------------------------------------*/
                                            // execute program on REDs computer
extern void RED_execute(const std::string, const std::string,
                        const std::string);
                                            // install program on REDs computer
extern void RED_install(const std::string, const std::string,
                        const std::string);
extern bool RED_ping(const std::string);                    // send ping to RED
extern void RED_terminate(const std::string);                  // terminate RED
