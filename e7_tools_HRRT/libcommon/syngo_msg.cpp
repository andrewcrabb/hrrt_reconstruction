/*! \file syngo_msg.cpp
    \brief Wrapper functions for the Syngo interface.
    \author Frank Kehren (frank.kehren@cpspet.com)
    \date 2003/11/17 initial version
    \date 2004/05/18 added Doxygen style comments

    This module provides wrapper functions that call the reconbe_getcancelflag
    and reconbe_reconreply functions.
 */

#include "syngo_msg.h"
#ifdef USE_FROM_IDL
#include "reconbe_wrapper.h"
#endif

/*- exported functions ------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*! \brief Cancel button in Syngo interface pressed ?
    \return cancel button pressed

    Cancel button in Syngo interface pressed ?
 */
/*---------------------------------------------------------------------------*/
bool SyngoCancel()
 {
#ifdef USE_FROM_IDL
   return(reconbe_getcancelflag() > 0);
#endif
#ifndef USE_FROM_IDL
   return(false);
#endif
 }

/*---------------------------------------------------------------------------*/
/*! \brief Send text message to Syngo user interface.
    \param[in] msg   text message

    Send text message to Syngo user interface.
 */
/*---------------------------------------------------------------------------*/
#ifdef USE_FROM_IDL
void SyngoMsg(std::string msg)
 { msg="Detail "+msg;
   reconbe_reconreply(2, -1, (char * const)(const char *)msg.c_str());
 }
#endif
#ifndef USE_FROM_IDL
void SyngoMsg(std::string)
 {
 }
#endif
