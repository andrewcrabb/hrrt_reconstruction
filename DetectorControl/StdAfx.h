// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__6521E17B_8DA8_4648_9C13_980CAE281C62__INCLUDED_)
#define AFX_STDAFX_H__6521E17B_8DA8_4648_9C13_980CAE281C62__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#define _WIN32_DCOM				// CES
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
class CExeModule : public CComModule
{
public:
	LONG Unlock();
	DWORD dwThreadID;
	HANDLE hEventShutdown;
	void MonitorShutdown();
	bool StartMonitor();
	bool bActivity;
protected:
};
extern CExeModule _Module;

#include <time.h>
#include <stdio.h>
#include <atlcom.h>
#include <process.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__6521E17B_8DA8_4648_9C13_980CAE281C62__INCLUDED)
