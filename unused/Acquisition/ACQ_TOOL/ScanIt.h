// ScanIt.h : main header file for the SCANIT application
//

#if !defined(AFX_SCANIT_H__548AC991_C078_43E6_B149_C5CF6BCE3AAB__INCLUDED_)
#define AFX_SCANIT_H__548AC991_C078_43E6_B149_C5CF6BCE3AAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

static char *doses[] =
{
	"None Selected",
	"Br-75",
    "C-11",       
    "Cu-62",
    "Cu-64",
    "F-18", 
    "Fe-52",
    "Ga-68",
    "Ge-68",
    "N-13", 
    "O-14", 
    "O-15", 
    "Rb-82",
    "Na-22",
    "Zn-62",
    "Br-76",
    "K-38", 
};
static float halflife[] = 
{
	0.f,
   5880.f,        
   1224.f,       
   583.8f,        
   46080.f,      
   6588.f,      
   298800.f,     
   4098.f,        
   23760000.f,  
   598.2f,        
   70.91f,          
   123.f,        
   78.f,          
   82080000.f,   
   33480.f,      
   58320.f,      
   458.16f,
   };

static float branch_Frac[] = 
{       0.f,
	   0.755f,
       0.9976f,
       0.980f,
       0.184f,
       0.967f,
       0.57f,
       0.891f,
       0.891f,
       0.9981f,
       1.f,
       0.9990f,
       0.950f,
       0.9055f,
       0.152f,
       0.57f,      
       1.f 
	   };
/////////////////////////////////////////////////////////////////////////////
// CScanItApp:
// See ScanIt.cpp for the implementation of this class
//

class CScanItApp : public CWinApp
{
public:
	CScanItApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScanItApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CScanItApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCANIT_H__548AC991_C078_43E6_B149_C5CF6BCE3AAB__INCLUDED_)
