#ifndef _CPATIENTADO_H_
#define _CPATIENTADO_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// NOTE : In order to use this code against a different version of ADO, the appropriate
// ADO library needs to be used in the #import statement
#import "msado15.dll" rename_namespace("ADOCG") rename("EOF", "EndOfFile")
//#import "C:\Program Files\Common Files\System\ADO\msado15.dll" rename_namespace("ADOCG") rename("EOF", "EndOfFile")
//#import "msado25.tlb" rename_namespace("ADOCG") rename("EOF", "EndOfFile")
using namespace ADOCG;


#include "icrsint.h"

#include "oledb.h"

// PatientADO.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPatientADO class

class CPatientADO : public CADORecordBinding
{
BEGIN_ADO_BINDING(CPatientADO)
	ADO_VARIABLE_LENGTH_ENTRY2(  1, adVarChar, m_szLastName, sizeof(m_szLastName), m_lLastNameStatus, FALSE)
	ADO_VARIABLE_LENGTH_ENTRY2(  2, adVarChar, m_szFirstName, sizeof(m_szFirstName), m_lFirstNameStatus, FALSE)
	ADO_FIXED_LENGTH_ENTRY    (  3, adDate, m_dtDOB, m_lDOBStatus, FALSE)
	ADO_FIXED_LENGTH_ENTRY    (  4, adInteger, m_lSex, m_lSexStatus, FALSE)
	ADO_VARIABLE_LENGTH_ENTRY2(  5, adVarChar, m_szPatientID, sizeof(m_szPatientID), m_lPatientIDStatus, FALSE)
	ADO_FIXED_LENGTH_ENTRY    (  6, adInteger, m_lID, m_lIDStatus, FALSE)
END_ADO_BINDING()

//Attributes
public:
	CHAR			m_szLastName[51];
	ULONG			m_lLastNameStatus;
	CHAR			m_szFirstName[51];
	ULONG			m_lFirstNameStatus;
	DATE			m_dtDOB;
	ULONG			m_lDOBStatus;
	LONG			m_lSex;
	ULONG			m_lSexStatus;
	CHAR			m_szPatientID[51];
	ULONG			m_lPatientIDStatus;
	LONG			m_lID;
	ULONG			m_lIDStatus;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !_CPATIENTADO_H_
