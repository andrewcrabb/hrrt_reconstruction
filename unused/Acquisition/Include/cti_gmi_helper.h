//==========================================================================
// File			:	cti_gmi_helper.h
//					Copyright 2002 by CTI
// Description	:	Contains the defines and enums 
//	
// Author		:	Selene F. Tolbert
//
// Date			:	18 July 2002
//
// Author		Date			Update
// S Tolbert	18 July 02		Created
//==========================================================================

// Set guards to prevent multiple header file inclusion
//

#ifndef CTI_GMI_IDLHELPERH
#define CTI_GMI_IDLHELPERH

#include <stdio.h>
#include <export.h>

#define IDLMsgOut( x ) IDL_Message(IDL_M_NAMED_GENERIC,IDL_MSG_INFO, x )
#define IDLMsgJmp( x ) IDL_Message(IDL_M_NAMED_GENERIC,IDL_MSG_LONGJMP, x)

class CTI_GMI_IDLHelper
{

public:

	
	CTI_GMI_IDLHelper();
	~CTI_GMI_IDLHelper();
	
	//Utility Functions
	IDL_VPTR gantryModelInfo	(int argc, IDL_VPTR argv[], char *argk);
	IDL_VPTR isotopeInfo		(int argc, IDL_VPTR argv[], char *argk);
	IDL_VPTR segmentTable		(int argc, IDL_VPTR argv[], char *argk);  

protected:
	
};

#endif