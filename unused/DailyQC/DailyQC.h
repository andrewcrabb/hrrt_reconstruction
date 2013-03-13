// DailyQC.h : main header file for the DailyQC application
//
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

// CDailyQCApp:
// See DailyQC.cpp for the implementation of this class
//

class CDailyQCApp : public CWinApp
{
public:
	CDailyQCApp();


// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	BOOL ProcessShellCommand(CCommandLineInfo& rCmdInfo);
public:
	// 64 bit list mode file dragged/dropped to the application
	CString m_strL64FileName;
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);

};

extern CDailyQCApp theApp;