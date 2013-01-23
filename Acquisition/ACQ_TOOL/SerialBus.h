// SerialBus.h: interface for the CSerialBus class.
//
//////////////////////////////////////////////////////////////////////


#if !defined(AFX_SERIALBUS_H__475BEC44_BE1B_445E_B26F_03FB3EC8C885__INCLUDED_)
#define AFX_SERIALBUS_H__475BEC44_BE1B_445E_B26F_03FB3EC8C885__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <process.h>
#include <atlbase.h>


class CSerialBus  
{
public:
	CSerialBus();
	~CSerialBus();
	


public:
	bool m_Com_Err;
	int m_Icon_Stat;
	void ClosePort();
	int m_DataBit;
	int m_StopBit;
	int m_Parity;
	int m_BaudRate;
	int m_ComPort;
	void SaveSerialFile();
	void GetSerialFile();
	bool m_GotCom;
	CString m_logfile;
	CComCriticalSection *serialbus_access_control;

	HANDLE hCommPort;
	//unsigned int tid;
	//HANDLE hThread;
	//STDMETHOD(command)(/*[in,out,string]*/ unsigned char **cmd, /*[in]*/ long timeout, /*[out]*/ long *status);
	int InitSerialBus(); 
	//int InitThread();
	int SendCmd(char *buffer);
	int GetResponse(char *buf, int length, int timeout);
	//void InitThread();
	
private:
	//static unsigned int WINAPI BkGndProc(void *p);

};

#endif // !defined(AFX_SERIALBUS_H__475BEC44_BE1B_445E_B26F_03FB3EC8C885__INCLUDED_)
