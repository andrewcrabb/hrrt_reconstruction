/*! \file hwinfo.h
    \brief This module is used to gather information about the hardware (CPUs).
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/06/02 added Doxygen style comments
 */

#pragma once

/*- exported functions ------------------------------------------------------*/

unsigned short int logicalCPUs();             // request number of logical CPUs
void printHWInfo(const unsigned short int);       // print hardware information

