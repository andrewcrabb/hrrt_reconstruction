/*! \file syngo_msg.h
    \brief Wrapper functions for the Syngo interface.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#pragma once

#include <string>

/*- exported functions ------------------------------------------------------*/

bool SyngoCancel();                      // cancel in Syngo interface pressed ?
void SyngoMsg(std::string msg);              // send message to Syngo interface
