// SerialBus.cpp: implementation of the CSerialBus class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "SerialBus.h"
#include <time.h>
#include "cdhi.h"

extern CCDHI pDHI;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSerialBus::CSerialBus()
{
	hCommPort = NULL;
	serialbus_access_control = new CComCriticalSection();
	serialbus_access_control->Init();
	// initial values
	m_DataBit = 1; // 8 bits
	m_StopBit = 0; // One Stop bit
	m_Parity = 2;	// none
	m_BaudRate = 3;	// 115200
	m_ComPort = 0;  // COM1
	m_Icon_Stat = 0;
	m_Com_Err = TRUE;
	GetSerialFile();
	InitSerialBus();
	// InitThread();
}

CSerialBus::~CSerialBus()
{
	delete	serialbus_access_control;
	CloseHandle(hCommPort);

}
/*
void CSerialBus::InitThread()
{
	// Define and start the thread...
	hThread = (HANDLE) _beginthreadex(NULL,0,BkGndProc,this,0,&tid);
	CloseHandle(hThread);

}
*/
/*
unsigned int WINAPI CSerialBus::BkGndProc(void *p)
{
	CSerialBus *pa = (CSerialBus *)p;
	//char buffer[2048];
	//int ret;
	static int delay;
	static int head = 0;
	static int conf = 0;
	BOOL  bResult = TRUE;
	DWORD Event = 0;
	//COMSTAT comstat;
	CString str;
	//unsigned long bread,bavail,bltm;
	//int stat;
	
	delay = 1000;

	for (;;)
	{
		Sleep(delay);
		// Download setting to the CC and DHI 
		if(pa->m_bDownFlag)
		{
			if (pa->m_Dn_Step==2) // || pa->m_Dn_Step==7)
				delay = 500;
			else
				delay = 100;
			pa->Download();
		}
		pa->serialbus_access_control->Lock();
		// Get Async messages
		bResult = WaitCommEvent(pa->hCommPort, &Event, &pa->m_ov);
		if (!bResult)  
		{ 
			bResult = ClearCommError(pa->hCommPort, &Event, &comstat);

			if (comstat.cbInQue) 
			//if (pa->m_Dn_Step==7) 
			{
				// Read Async method
				ret = pa->GetResponse(buffer, 2048, 1);
				str.Format("%s",buffer);
				if (pa->m_Dn_Step==7 && str.Find("64! 203")!=-1)
				//if (str.Find("64! 203")!=-1)
				{
					// Get Singles Random
					pa->GetData(str);
				}

			}
				
		}
		pa->serialbus_access_control->Unlock();
//
	}
	return 0;




}
*/

int CSerialBus::InitSerialBus()
{
	DCB dcbCommPort;
	CString com;
	DWORD dwBuadRate;
	BYTE btDataBit, btStopBit, btParity;
	// Setup the communication parameters
	switch (m_ComPort)
	{
	case 0:
		com = "COM1";
		break;
	case 1:
		com = "COM2";
		break;
	default:
		com = "COM1";
		break;
	}
	// BaudRate
	switch (m_BaudRate)
	{
	case 0:
		dwBuadRate = CBR_9600;
		break;
	case 1:
		dwBuadRate = CBR_19200;
		break;
	case 2:
		dwBuadRate = CBR_57600;
		break;
	case 3:
		dwBuadRate = CBR_115200;
		break;
	default:
		dwBuadRate = CBR_115200;
		break;
	}
	// Parity
	switch (m_Parity)
	{
	case 0:
		btParity = EVENPARITY;
		break;
	case 1:
		btParity = ODDPARITY;
		break;
	case 2:
		btParity = NOPARITY;
		break;
	default:
		btParity = NOPARITY;
		break;
	}
	// Data Bit
	switch (m_DataBit)
	{
	case 0:
		btDataBit = 7;
		break;
	case 1:
		btDataBit = 8;
		break;
	default:
		btDataBit = 8;
		break;
	}

	// Stop Bit
	switch (m_StopBit)
	{
	case 0:
		btStopBit = ONESTOPBIT;
		break;
	case 1:
		btStopBit = TWOSTOPBITS;
		break;
	default:
		btStopBit = TWOSTOPBITS;
		break;
	}


	hCommPort = CreateFile ( com, GENERIC_READ|GENERIC_WRITE, 0, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );

	COMMTIMEOUTS ctmoCommPort;
	memset ( &ctmoCommPort, 0, sizeof ( ctmoCommPort ) );
	ctmoCommPort.ReadIntervalTimeout = 0;
	ctmoCommPort.ReadTotalTimeoutMultiplier = 0;
	ctmoCommPort.ReadTotalTimeoutConstant = 500;   // 500 ms timeout
	SetCommTimeouts ( hCommPort, &ctmoCommPort );

	dcbCommPort.DCBlength = sizeof ( DCB );
	GetCommState ( hCommPort, &dcbCommPort );
	dcbCommPort.BaudRate = dwBuadRate; //CBR_115200;
	dcbCommPort.ByteSize = btDataBit; //8;
	dcbCommPort.StopBits = btStopBit; //ONESTOPBIT;
	dcbCommPort.Parity = btParity; //NOPARITY;
	
	SetCommState ( hCommPort, &dcbCommPort );

	return 0;
}



int CSerialBus::SendCmd(char *buffer)
{

	DWORD dwCount;

	//PurgeComm ( (HANDLE) hCommPort, PURGE_TXCLEAR | PURGE_RXCLEAR );
	CString str;
	
	FILE* out_file;
	//str.Format("%slogfile.txt",pDHI.m_Work_Dir);
	//out_file = fopen(str,"a");
	out_file = fopen(m_logfile,"a");

	// write the command data
	dwCount = strlen(buffer);
	WriteFile ( hCommPort, buffer, dwCount, &dwCount, NULL );
	if (out_file && pDHI.m_LogFlag)
	{
		fprintf( out_file, "Send: %s\n", buffer );	 
	}
	if (out_file != NULL)
		fclose (out_file);
	// write a terminator
	char c = '\n';
	dwCount = 1;
	WriteFile ( hCommPort, &c, dwCount, &dwCount, NULL );


	return 0;

}

int CSerialBus::GetResponse(char *buffer, int length, int timeout)
{
	DWORD dwCount = 1;
	char c;
	int count = 0;
	*buffer = '\0';
	CString str;
	
	FILE* out_file;
	//str.Format("%slogfile.txt",pDHI.m_Work_Dir);
	//out_file = fopen(str,"a");
	out_file = fopen(m_logfile,"a");

	// define Delay
	long finish_time = time(0) + timeout;

	while (count < length)
	{
		// Read the Communication port
		ReadFile ( hCommPort, &c, 1, &dwCount, NULL );
		if ( dwCount == 1 )
		{
			if ( c == '\n' )   // is it a command terminated
			{
				buffer[count] = '\0'; // We got a command
				m_Com_Err = FALSE;
				if (out_file && pDHI.m_LogFlag)
				{
					fprintf( out_file, "Receive Good: %s\n", buffer );	 
				}
				if (out_file != NULL)
					fclose (out_file);
				return 0;
			}
			buffer[count++] = c;  // Store character in the buffer
		}
		else   
			if (time(0) > finish_time) // did timed out?
			{
				m_Com_Err = TRUE;
				if (out_file && pDHI.m_LogFlag)
				{
					fprintf( out_file, "Receive TimeOut: %s\n", buffer );	 
				}
				if (out_file != NULL)
					fclose (out_file);
				return 1;
			}
	}
	
	// buffer length overrun
	//m_Com_Err = TRUE;
	*buffer = '\0';
	return 2;
}

void CSerialBus::GetSerialFile()
{
	char t[256];
	CString str, newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "rs232.ini";
	
	// Open the header file
	FILE  *in_file;
	in_file = fopen(newstr,"r");
	if( in_file!= NULL )
	{	
		// Read teh file line by line and sort data
		while (fgets(t,256,in_file) != NULL )
		{			
			str.Format ("%s",t);
			if (str.Find(":=")!=-1)				
				//  What type of system
				pDHI.SortData (str, &newstr);
			if(str.Find("Communication port(0=COM1, 1=COM2) :=")!=-1)
				m_ComPort = atoi(newstr);
			if(str.Find("BaudRate (0=9600, 1=19200, 2=57600, 3=115200) :=")!=-1)
				m_BaudRate = atoi(newstr);
			if(str.Find("Parity (0=Even, 1=Odd, 2=None) :=")!=-1)
				m_Parity = atoi(newstr);
			if(str.Find("Stop bit (0=OneStopBit, 1=TwoStopBits) :=")!=-1)
				m_StopBit = atoi(newstr);
			if(str.Find("Data bit (0=7bits, 1=8bits) :=")!=-1)
				m_DataBit = atoi(newstr);

		}
	}
	if( in_file!= NULL )
		fclose(in_file);


}

void CSerialBus::SaveSerialFile()
{
	CString newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "rs232.ini";
	// Write the new  file
	FILE  *out_file;
	out_file = fopen("rs232.ini","w");
	
	fprintf( out_file, "Communication port(0=COM1, 1=COM2) := %d\n", m_ComPort );
	fprintf( out_file, "BaudRate (0=9600, 1=19200, 2=57600, 3=115200) := %d\n",m_BaudRate);
	fprintf( out_file, "Parity (0=Even, 1=Odd, 2=None) := %d\n",m_Parity);
	fprintf( out_file, "Stop bit (0=OneStopBit, 1=TwoStopBits) := %d\n",m_StopBit);
	fprintf( out_file, "Data bit (0=7bits, 1=8bits) := %d\n",m_DataBit);

	if( out_file!= NULL )
		fclose(out_file);

}


void CSerialBus::ClosePort()
{
	CloseHandle(hCommPort);
}
