/*! \file syngo_msg.h
    \brief Wrapper functions for the Syngo interface.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments
 */

#ifndef _SYNGO_MSG_H
#define _SYNGO_MSG_H

#include <string>

/*- exported functions ------------------------------------------------------*/

bool SyngoCancel();                      // cancel in Syngo interface pressed ?
void SyngoMsg(std::string msg);              // send message to Syngo interface

#endif
