//** file: red_const.h
//** date: 2003/11/17

//** author: Frank Kehren
//** © CPS Innovations

//** This file contains some constants that are needed for the Remote Execution
//** Daemon.

/*-----------------------------------------------------------------------------

 history:

  2003/11/17
   initial version

-----------------------------------------------------------------------------*/


#pragma once

/*- constants ---------------------------------------------------------------*/
                  // port number for communication with remote execution daemon
const unsigned short int RED_PORT=4250;
                       // commands that are send to the remote execution daemon
const unsigned short int RD_CMD_PING=42,
                         RD_CMD_EXIT=43,
                         RD_CMD_INSTALL=44,
                         RD_CMD_EXEC=45;

