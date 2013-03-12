// ScanItDoc.cpp : implementation of the CScanItDoc class
//
/* HRRT User Community modifications
   Authors: Merence Sibomana
   9-July-2009 : Add milliseconds study time in the header, key "!study time (hh:mm:ss.sss)"
                 Bug fix in CScanItDoc::SendCmd
   25-Sep-2009 : Move setting scan start time after Go signal ACK
                 Add "Dead time constant" to scanner configuration and header files 
*/

#include "stdafx.h"
#include "ScanIt.h"


#include "cdhi.h"
#include "serialbus.h"
#include "mainfrm.h"

#include "ScanItView.h"
#include "ScanItDoc.h"
#include "ConfigDlg.h"
#include "ECodes.h"
#include <math.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CCDHI pDHI;
extern CSerialBus pSerBus;
extern CCallB pCB;


//static unsigned int WINAPI BkGndProc(void *p);
static UINT __stdcall 	WorkerThread(LPVOID);

/////////////////////////////////////////////////////////////////////////////
// CScanItDoc

IMPLEMENT_DYNCREATE(CScanItDoc, CDocument)

BEGIN_MESSAGE_MAP(CScanItDoc, CDocument)
	//{{AFX_MSG_MAP(CScanItDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScanItDoc construction/destruction
void CCallB::acqPostComplete(int enum_flag, int fileWriteSuccess)
{
	CString str;
	str.Format("Acq Complete enum = %d filewrite = %d\n",enum_flag,fileWriteSuccess);
	OutputDebugString(str);
	m_done = TRUE;
	if (fileWriteSuccess==1)
		m_writeFile = TRUE;

}
void CCallB::acqComplete(int enum_flag)
{
	CString str;
	str.Format("SCS_acqComplete: Error Code %d\n",enum_flag);
	OutputDebugString(str);
}

void CCallB::notify (int error_code, char *extra_info)
{
	CString str;
	str.Format("SCS_Notify: Error Code %d\n SCS_Notify: %s\n",error_code, extra_info);
	OutputDebugString(str);
	if (error_code == 0)
	{
		m_ARS_Err = TRUE;
		sprintf(m_str_Error,"%s",extra_info);
	}
}
CScanItDoc::CScanItDoc()
{

	model = new CGantryModel();

	if(!model->setModel(328))	
	{
		HWND hWnd = ::AfxGetMainWnd()->GetSafeHwnd();
		MessageBox(hWnd,"Could not set model 328","Gantry Initialization Error",MB_OK); //| MB_ICONWARNING

	}	
	// Initialize Sino Info
	InitSinoInfo();	
  m_ShuttingDown = false;

}

CScanItDoc::~CScanItDoc()
{
	int status;

	if(model!=NULL)
		delete(model);

	if (m_LMInit_Flag)
		status = LMInit(NULL, NULL, NULL);
	if (hc_file != NULL)
	{	
		fclose(hc_file);
		hc_file = NULL;
	}
	
	// exit the thread
	Shutdown();

}

BOOL CScanItDoc::OnNewDocument()
{
	CString str;
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	// Get the pointer to the view
	int status = 0;

	CDocTemplate* pDocTemplate = GetDocTemplate();
	POSITION pos = pDocTemplate->GetFirstDocPosition();
	CDocument* pDoc = pDocTemplate->GetNextDoc(pos);


	pos = GetFirstViewPosition();
	pView =(CScanItView*) pDoc->GetNextView(pos);
		
	// Software initial values
	m_bDownFlag = FALSE;
	hc_file = NULL;
	m_RunFlag = FALSE;
	m_Dn_Step=1;
	m_AllocFlag = FALSE;
	pDHI.InitSystem();
	
	

	InitThread();
	
	// Default configuration
	m_Ary_Span[0] = 3;
	//m_Ary_Span[1] = 5;
	//m_Ary_Span[2] = 7;
	m_Ary_Span[1] = 9;
	m_Ary_Ring[0] = 67;
	m_Ary_Ring[1] = 67;
	//m_Ary_Ring[2] = 90;
	//m_Ary_Ring[3] = 67;
	m_en_sino[0] = 6367;
	//m_en_sino[1] = 3873;
	//m_en_sino[2] = 3197;
	m_en_sino[1] = 2209;
	m_Scan_Type = 0;
	m_Scan_Mode = 0;
	m_List_64bit = 0;
	m_Preset_Opt = 0;
	m_Count_Opt = 0;
	m_Auto_Start = 0;
	m_KCPS = 1000;
	m_config = m_Scan_Type;
	m_lld = 400;
	m_uld = 650;

	m_Duration = 120;
	m_DurType = 0;
	m_Ary_Select = 0;
	m_Span = m_Ary_Span[m_Ary_Select];
	m_Ring = m_Ary_Ring[m_Ary_Select];
	m_Speed = 50;
	m_Plane = 2;
	int i;
	// Zero the array
	for(i=0;i<=19;i++)
		m_Total_Trues[i]=0;	
	
	m_FName = "Test";
	m_Pt_Dose = 0;
	m_Strength = 2.1f;
	m_Strength_Type = 0;
	m_Cont_Scan = 0;

	
	
	//presetValue = m_preset;
	m_nprojs=256;
	m_nviews=288;
	m_Segment_Zero_Flag = FALSE;
	m_n_em_sinos=207;
	m_n_tx_sinos = 0;
	m_nframes = 1;
	pDHI.m_HRRT = TRUE;

	m_DynaFrame = "*";
	
	m_CfgFileName = "default.cnfg";

	GetConfigFile();

	// Set the frame title to default
	str.Format("Default, Build %s %s", __DATE__, __TIME__);

	SetTitle ("Default");
//////
/*
	m_Mem_Alloc = 3000;
	lData = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
	lTrues = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
	lIndex = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
	lIndex[0] = 0;//
	lData[0] = 0;
	lTrues[0] = 0;
	orgtime = (long)(GetTickCount()/1000);
	m_index = 1;
	m_AllocFlag = TRUE;
*/
///////
	// Generate a logfile name
	SYSTEMTIME st;
	GetLocalTime(&st);

	pSerBus.m_logfile.Format("%s\\log\\Serial com logfile-%d.%d.%d.%d.%d.%d.txt",pDHI.m_Work_Dir,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	m_errfile.Format("%s\\log\\Error logfile-%d.%d.%d.%d.%d.%d.txt",pDHI.m_Work_Dir,st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
	pDHI.m_errfile = m_errfile;

	// Initialize the listmode and histogram here
	// May need to move it somewhere else
#ifndef NEWACS_SUPPORT  // added  07oct2010 : Merence Sibomana 
	status = LMInit(&pLMStats, &pCB, &pSinoInfo);
	if (status!=1)
		m_LMInit_Flag = FALSE;
	else
		m_LMInit_Flag = TRUE;

	status = HISTInit(&pHISTStats, &pCB, &pSinoInfo);
	
	if (status!=1)
		m_HISTInit_Flag = FALSE;
	else
		m_HISTInit_Flag = TRUE;
#else
    // else  Will be done in InitializeLM()
#endif
	return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// CScanItDoc diagnostics

#ifdef _DEBUG
void CScanItDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CScanItDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG



//unsigned int WINAPI CScanItDoc::BkGndProc(void *p)
//unsigned int WINAPI BkGndProc(void *p)
UINT __stdcall BkGndProc(LPVOID p)

{
	CScanItDoc *pa = (CScanItDoc *)p;
	char buffer[128], cmdbuf[10];
	int ret;
	//static int delay;
	//static int head = 0;
	//static int conf = 0;
	BOOL  bResult = TRUE;
	DWORD Event = 0;
	COMSTAT comstat;
	static OVERLAPPED ov;
	static bool waitflag = false;
	//static int num = 1;
	CString str, dm;
	SYSTEMTIME st;
	
	COleDateTime startT, endT;
	COleDateTimeSpan TimeDiff;
	
	SetEvent(pa->ThreadLaunchedEvent);

	Sleep(1000);
	
	//delay = 1000;

	for (;;)
	{
		if(WaitForSingleObject(pa->ShutdownEvent, TICK) == WAIT_TIMEOUT)
    {
		//Sleep(delay);
		//	OutputDebugString("Looping\n");
		// Download setting to the CC and DHI 
			if(pa->m_bDownFlag && !pSerBus.m_Com_Err)
			{
				//if (pa->m_Dn_Step==2) // || pa->m_Dn_Step==7)
				//	delay = 500;
				//else
				//	delay = 100;
				pa->Download();
			}
			else
			{
				// Keep trying to communicate
				//ret = pDHI.SendPing();
				sprintf(cmdbuf,"Q");
				ret = pa->SendCmd(cmdbuf,buffer, 64, 1);
				if (ret >= 0)
					pa->pView->ProcessIcon(TRUE);
			}
			// Countinuous Scans
			if (pa->m_Dn_Step >= 6 && !pa->m_RunFlag && pa->m_Cont_Scan == 1)
			{
				// Wait for 5 minutes
				if (waitflag)
				{					
					GetLocalTime(&st);
					endT.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					TimeDiff = (endT - startT);
					str.Format("%f\n",TimeDiff* 24.0 * 3600.0);
					OutputDebugString(str);
					if ((TimeDiff* 24.0 * 3600.0)  > 300)
					{
						waitflag = false;
						pa->m_RunFlag = TRUE;
					}

				}
				else
				{
					waitflag = true;
					GetLocalTime(&st);
					// set timer
					startT.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);

				}
			}
			//if(pa->m_bDownFlag && !pSerBus.m_Com_Err)
			//	pa->Download();
			pSerBus.serialbus_access_control->Lock();
			// Get Async messages
			bResult = WaitCommEvent(pSerBus.hCommPort, &Event, &ov);
			if (!bResult)  
			{ 
				bResult = ClearCommError(pSerBus.hCommPort, &Event, &comstat);

				if (comstat.cbInQue) 
				//if (pa->m_Dn_Step==7) 
				{
					// Read Async method
					pa->pView->ProcessIcon(TRUE);
					ret = pSerBus.GetResponse(buffer, 2048, 1);
					str.Format("%s",buffer);
					pa->pView->ProcessIcon(TRUE);
					//if (pa->m_Dn_Step==8 && str.Find("64! 203")!=-1)
					if (str.Find("64! 203")!=-1)
					{
						// Get Singles Random
						pa->GetData(str);
						if (pa->m_Auto_Start )
						{
							pa->GetTruesAvg();
						}
					}
					// Front pannel botton was pressed process
					if (str.Find("64! 200")!=-1)
					{
						str.Format("Remote Start : m_RunFlag = %d, m_Dn_Step = %d\n",pa->m_RunFlag, pa->m_Dn_Step);
						OutputDebugString(str);
						pa->m_RunFlag = TRUE;
					}
					// Scan Is done
					if (str.Find("64! 201")!=-1)
					{
						// Only if we are in TX Scan
						if (pa->m_Scan_Type == 1)
							pa->StopScan();
					}

					// Check if Async Error
					if ((str.Find('-',1) !=-1) )//&& (str.Mid(0,2) == sd))	// Check for "-" => error
					{
						pDHI.m_err_num = pDHI.returncode(buffer);
						pa->CheckError(pDHI.m_err_num);
						pDHI.m_err_num = 0;
					}

				}
					
			}
			pSerBus.serialbus_access_control->Unlock();



			// Check Patient name and diaplay it
			str.Format("%s, %s",pa->m_Pt_Lst_Name,pa->m_Pt_Fst_Name);
			pa->pView->m_PName.GetWindowText(dm);
			if(dm != str)
				pa->pView->m_PName.SetWindowText(str);

			// Check Scan File Name and diaplay it
			pa->pView->m_FileName.GetWindowText(dm);
			if(dm != pa->m_FileName)
				pa->pView->m_FileName.SetWindowText(pa->m_FileName);			
			
			// Display Scan Type
			pa->pView->m_SCType.GetWindowText(dm);
			if (pa->m_Scan_Type == 0)
				str = "EM";
			else
				str = "TX";
			if (pa->m_Scan_Mode == 0)
			{
				if (pa->m_List_64bit == 0)
					str += " 32 Bit Listmode";
				else
					str += " 64 Bit Listmode";
			}
			else
			{
				if (pa->m_Ary_Select == 0)
					str += " Span 3";
				else
					str += " Span 9";
			}
			if(dm != str)
				pa->pView->m_SCType.SetWindowText(str);			


			// Appemd to the hdr file
			if (pa->pCB.m_writeFile || pa->pCB.m_done)
			{
				if (pa->m_Scan_Mode == 1)
					pa->AppendHeaderFile();	// Update deader file
				else if (pa->m_Scan_Mode == 0)
					pa->ListHeaderFile();
				pa->pCB.m_writeFile = FALSE;
				pa->pCB.m_done = FALSE;
				int status = HISTUnconfig();

			}
			if(pa->pCB.m_ARS_Err)
			{
				// error in scanning
				CString str;
				str.Format("%s",pa->pCB.m_str_Error);
				pa->pView->m_Status.SetWindowText(str);
				pa->m_Dn_Step = 1;
				pa->m_bDownFlag = FALSE;
				pa->m_RunFlag = FALSE;	// return to home
				pa->pCB.m_ARS_Err = FALSE;

			}


	/////////////
	/*

			if (pa->m_index >= pa->m_Mem_Alloc) 
			{
				pa->m_Mem_Alloc += 1000;
				
				pa->lData = (long *)realloc( pa->lData, pa->m_Mem_Alloc * sizeof( long ) );
				pa->lTrues = (long *)realloc( pa->lTrues, pa->m_Mem_Alloc * sizeof( long ) );
				pa->lIndex = (long *)realloc( pa->lIndex, pa->m_Mem_Alloc * sizeof( long ) );
			}

			pa->lIndex[pa->m_index] = (long)(GetTickCount()/1000) - pa->orgtime;
			pa->lData[pa->m_index] = num;
			pa->lTrues[pa->m_index] = num * 10;


			if (pa->pView->m_Move_En==1 && pa->lIndex[pa->m_index] > pa->pView->m_Move_Xmax)
			{
				pa->pView->m_Move_Xmax = pa->lIndex[pa->m_index] + 1;
				pa->pView->m_Move_Xmin = pa->pView->m_Move_Xmax - pa->pView->m_Move_Dur;
				//pa->pView->Invalidate(TRUE);
				pa->pView->DrawMoveWin();
			}

			pa->m_index++;

			num ++;
			if (num > 1000) num = 1;

			pa->pView->DrawGraph();
	*/
	//////////

		}
		else
		{
			TRACE("Thread Terminated\n");
			return THREADEXIT_SUCCESS;
		}
	}
	
	return THREADEXIT_SUCCESS;




}

/////////////////////////////////////////////////////////////////////////////
// CScanItDoc commands
void CScanItDoc::InitThread()
{
	ThreadLaunchedEvent	= CreateEvent(NULL, FALSE, TRUE, NULL);

	ShutdownEvent = WSACreateEvent();
	// Launch Helper Thread
	ResetEvent(ThreadLaunchedEvent);

	// Define and start the thread...
	hThread = (HANDLE)_beginthreadex(NULL,0,BkGndProc,this,0,&tid);
	if(!hThread)
	{
		OutputDebugString("Failed to start thread\n");
		return;
	}
	if(WaitForSingleObject(ThreadLaunchedEvent, THREADWAIT_TO) != WAIT_OBJECT_0)
	{
		OutputDebugString("Unable to get response from Worker Thread within specified Timeout");
		CloseHandle(ThreadLaunchedEvent);
		return;
	}
	
	CloseHandle(ThreadLaunchedEvent);

}


void CScanItDoc::Shutdown()
{
	int i=0, max_try = 10;
	//Killing
	//  worker thread
	//
  m_ShuttingDown = true;
	SetEvent(ShutdownEvent);
	DWORD n = WaitForSingleObject(hThread, THREADKILL_TO);

	while (n == WAIT_TIMEOUT && i++<max_try)
	{
		OutputDebugString("Waiting for background thread termination\n");
    n = WaitForSingleObject(hThread, THREADKILL_TO);
	}

	if( i==max_try || n == WAIT_FAILED)
	{
		OutputDebugString("Thread failed to terminate\n");
		// manually termnate thread in case we failed to do it
		if(!TerminateThread(hThread, THREADEXIT_SUCCESS))
			OutputDebugString("TerminateThread(.hThread.) failure, probably it is already terminated");
		// may be call GetLastError()); to find out what is wrong

	}
	
	CloseHandle(hThread); 

	// Clean up event created
	if(ShutdownEvent)
		WSACloseEvent(ShutdownEvent);
}

void CScanItDoc::Download()
{
	char buffer[2048], cmdbuf[25];
	char fname[128];
	int  ret, hd,x, status, num;
	CString str,sd,dis,cl;
	bool dn;
	
	SYSTEMTIME st;
	COleDateTimeSpan spanElapsed;	
	double tottime;

	switch(m_Dn_Step)
	{
	case 1:
		//  Enable all channel on selected head
		pView->m_Status.SetWindowText("Please wait! Initializing the system");
		
		// Turn off single polling
		sprintf(cmdbuf,"L0");
		ret = SendCmd(cmdbuf,buffer, 64, 1);

		for(hd=0;hd<8;hd++)
		{
			if(pDHI.m_Hd_En[hd])
			{
				pView->ProcessIcon(TRUE);
				// Set the command up
				sprintf(cmdbuf,"I -1");
				//ret = pDHI.SendCmd(cmdbuf,buffer, hd, 2);
				ret = SendCmd(cmdbuf,buffer, hd, 2);

				pView->ProcessIcon(TRUE);


			}
		}
		m_Dn_Step = 2;
		break;
	case 2:	
		// See if the system is in run mode
		dn = FALSE;
		// loop on all enabled heads
		for(hd=0;hd<8;hd++)
		{
			if(pDHI.m_Hd_En[hd])
			{
				// Get the system mode
				pView->ProcessIcon(TRUE);
				pDHI.GetSysMode(hd);
				pView->ProcessIcon(TRUE);

				if (pDHI.m_HRRT)
					if (pDHI.HrrtMode.mode != 3 || pDHI.HrrtMode.lld != m_lld || pDHI.HrrtMode.uld != m_uld)
						dn = TRUE;
				if (pDHI.m_PETSPECT)
					if (pDHI.PetSpectMode.mode != 3 || pDHI.PetSpectMode.lld != m_lld || pDHI.PetSpectMode.uld != m_uld)
						dn = TRUE;
			}
		}

		// one or all the heads are not in run mode
		//if (dn)	// Change this to always congigure the heads
		//{			// b/c the cc locks up or does not configure all the heads
		//	//m_Status.SetWindowText("Setting System in run mode");
		//	if(pDHI.m_All_Hd_En)
		//	{
		//		// Set all heads to run mode
		//		pView->ProcessIcon(TRUE);
		//		// Set the command up
		//		//sprintf(cmdbuf,"M %d run %d %d",m_config,m_lld, m_uld);
		//		sprintf(cmdbuf,"M 0 run %d %d",m_lld, m_uld);
		//		//ret = pDHI.SendCmd(cmdbuf,buffer, 64, 2);
		//		ret = SendCmd(cmdbuf,buffer, 64, 2);

		//		pView->ProcessIcon(TRUE);

		//		m_Dn_Step = 3;
		//	}
		//	else
		//	{
				for(hd=0;hd<8;hd++)
				{
					if(pDHI.m_Hd_En[hd])
					{
						pView->ProcessIcon(TRUE);
						// Set all heads to run mode
						// Set the command up
						//sprintf(cmdbuf,"M %d run %d %d",m_config,m_lld, m_uld);
						sprintf(cmdbuf,"M 0 run %d %d",m_lld, m_uld);
						ret = SendCmd(cmdbuf,buffer, hd, 2);
						if (ret < 0)
							CheckError(ret);
						pView->ProcessIcon(TRUE);


					}

				}
				m_Dn_Step = 3;
		//	}
		//}
		//else
		//	m_Dn_Step = 3;


		break;
	case 3:

		// Changed to reflect code change for step 2:  to status all the heads instead of CC
		//if(pDHI.m_All_Hd_En)
		//{

		//	// Get the completion status
		//	pView->ProcessIcon(TRUE);
		//	// Set the command up
		//	sprintf(cmdbuf,"P0");
		//	ret = SendCmd(cmdbuf,buffer, 64, 2);
		//	if (ret < 0)
		//		CheckError(ret);
		//	str.Format("%s",buffer);
		//	pView->ProcessIcon(TRUE);

			// Parse the return buffer for percent completion
		//	if (str.Find("64P") !=-1)
		//	{
		//		x = 3;//str.Find("64P");
		//		ret = str.GetLength();
		//		if (ret>3)
		//		{
		//			dis.Format("Process is %d%% done",atoi(str.Mid(x+1,ret-x-1)));

		//			pView->m_Status.SetWindowText(dis);
		//		}
		//		if (atoi(str.Mid(x+1,ret-x-1)) == 100)
		//		{
		//			GetLocalTime(&st);
		//			m_timeStart.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
		//			// turn off singles poling
		//			sprintf(cmdbuf,"L0");
		//			ret = SendCmd(cmdbuf,buffer, 64, 10);
		//			m_Dn_Step = 4;
		//			m_head = 0;
		//		}
		//	}
		//}
		//else
		//{
			cl = "";
			dn = FALSE;
			num = 100;
			for(hd=0;hd<8;hd++)
			{
				if(pDHI.m_Hd_En[hd])
				{	
					pView->ProcessIcon(TRUE);
					// Set the command up
					sprintf(cmdbuf,"P0");
					ret = SendCmd(cmdbuf,buffer, hd, 2);
					if (ret < 0)
						CheckError(ret);
					str.Format("%s",buffer);
					sd.Format("%dP",hd);
					pView->ProcessIcon(TRUE);

					// Parse the return buffer for percent completion
					if (str.Find(sd) !=-1)
					{
						x = 2;//str.Find("64P");
						ret = str.GetLength();
						//if (ret>3 )
						//{
						//	dis.Format("Head %d is %d%% done",hd,atoi(str.Mid(x+1,ret-x-1)));
						//	cl += dis;
			
						//	pView->m_Status.SetWindowText(dis);
						//	//UpdateData(FALSE);
						//}
						if (atoi(str.Mid(x+1,ret-x-1)) != 100)
							dn = TRUE;
						if (atoi(str.Mid(x+1,ret-x-1)) <=num)
							num = atoi(str.Mid(x+1,ret-x-1));

					}
				}
			}
			// Update status
			dis.Format("Heads configuration is %d%% done",num);
			pView->m_Status.SetWindowText(dis);

			if(!dn)
			{
				GetLocalTime(&st);
				m_timeStart.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
				// turn off singles poling
				sprintf(cmdbuf,"L0");
				ret = SendCmd(cmdbuf,buffer, 64, 10);
				m_Dn_Step = 4;
				m_head = 0;
			}
		//}
		break;
	case 4:

		// Check the Singles
		GetLocalTime(&st);
		m_timeEnd.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
		spanElapsed = m_timeEnd - m_timeStart;
		tottime = spanElapsed.GetTotalSeconds();
		if (tottime > 2.0)
		{
			if(pDHI.m_Hd_En[m_head])
			{
				// Update the status lable
				str.Format("Checking Singles for Head %d",m_head);
				pView->m_Status.SetWindowText(str);
				// Update the Communication icon
				pView->ProcessIcon(TRUE);

				// read the singles
				pDHI.GetSingles(m_head);
				// check for zeros
			}
			m_head++;
			if (m_head>=pDHI.m_TOTAL_HD)
			{
				dn = FALSE;
				pDHI.CheckSinZero(&dn);
				if (dn)
				{
					HWND hWnd = ::AfxGetMainWnd()->GetSafeHwnd();
					int a = MessageBox(hWnd,"Some of the Singles returned zeros, Would you like to continue?","Singles Warning",MB_YESNO | MB_ICONWARNING);
					if(a==6)
					{
						m_Dn_Step = 5;
						//pView->m_Status.SetWindowText("Ready to Scan");
					}
					else
					{
						m_bDownFlag = FALSE;
						m_RunFlag = FALSE;	// return to home
						m_Dn_Step = 1;
					}
				}
				else
				{
					m_Dn_Step = 5;
					//pView->m_Status.SetWindowText("Ready to Scan");
				}	
			}
		}
		break;
	case 5:	
		// Put the CC in Emission Coincidence Mode
		//m_Status.SetWindowText("Emission Coincidence Mode");
		pView->ProcessIcon(TRUE);
//		pSerBus.serialbus_access_control->Lock();
		if(m_Scan_Type == 0)
		{
			pView->m_Status.SetWindowText("Putting scanner in coincidence mode");
			// Emission Scan
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"S0");
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"G");
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"Q1");
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"L1");
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);

		}
		else
		{
			pView->m_Status.SetWindowText("Putting scanner in transmission mode");
			// Transmission Scan
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"S3");
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);
			sprintf(cmdbuf,"T %d %d",(int)m_Speed, m_Plane);
			ret = SendCmd(cmdbuf,buffer, 64, 10);
			pView->ProcessIcon(TRUE);

		}

//		pSerBus.serialbus_access_control->Unlock();
		m_Dn_Step = 6;


		break;


	case 6:
		pView->m_Status.GetWindowText(str);
		if((str !="Ready To Scan") && !m_Auto_Start)
		{
			pView->m_Status.SetWindowText("Ready to Scan");
			pView->ProcessButton();

		}


		if (m_RunFlag)
		{
			pView->m_Status.SetWindowText("Running");
			m_Rnd_Tot = 0;
			m_Trs_Tot = 0;
			pView->m_Rnd_Tot.SetWindowText("0");
			pView->m_Trs_Tot.SetWindowText("0");
			pCB.m_done = FALSE;
			m_Dn_Step = 7;
		}
		break;
	case 7:
		// Send a Go command
			
		GetLocalTime(&st);
		// initialize the time
		m_timeStart.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);

		// Set the x axis
		pView->m_Xmin = 0;

		pView->m_Move_Xmin = 0;
		pView->m_Move_Xmax = pView->m_Move_Dur;
		if ((m_Scan_Mode == 0 && m_Scan_Type !=1)|| (m_Preset_Opt ==0 && m_Scan_Type !=1) )
		//if (m_Scan_Mode == 0 && m_Preset_Opt ==0 && m_Scan_Type !=1)
		{

			if (m_DurType == 0)
				m_preset = m_Duration;
			else if (m_DurType == 1)
				m_preset = m_Duration * 60;
			else
				m_preset = m_Duration * 3600;
			pView->m_Xmax = m_preset;
			if (m_preset < 1000)
				m_Mem_Alloc = 1100;
			else
				m_Mem_Alloc = m_preset + 1000;
		}
		else
		{
			if (m_Scan_Type == 1)
			{
				// Transmission Scan.  Set an estimated time 
				m_preset = 20000;
				m_Mem_Alloc = 5000;
				pView->m_Xmax = 300;
			}
			else if (m_Preset_Opt = 1)
			{
				// By counts
				m_preset = m_Duration;
				m_Mem_Alloc = 1000;
				pView->m_Xmax = 1000;
			}

		}
		// Free the old memory
		if(m_AllocFlag)
		{
			free(lData);
			free(lTrues);
			free(lIndex);
		}
		// Allocate the new memory
		lData = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
		lTrues = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
		lIndex = (long *)calloc( m_Mem_Alloc, sizeof( long ) );
		m_AllocFlag = TRUE;


		pView->Invalidate(TRUE);

		m_index = 0;

		// Set up the complex filename
		SetFileName();

		m_ext_fname.Format("%d.%02d.%02d.%02d.%02d.%02d",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
		// Append _EM or _TX at the end of the filename
		if (m_Scan_Type == 0)
			m_ext_fname+="_EM";
		else
			m_ext_fname+="_TX";
		
		// Set the scan parameters
		SetScanParam();
		
		// Initialize the rebinner
		ConfigRebinner();

		status = 1;
		// Initialize Histogrammer
		if(m_Scan_Mode ==1)
			status = InitializeHIST();

		// initialize LM
		if(status==1)
			status = InitializeLM();
		

		if (status ==1)
		{
			if(m_Scan_Mode ==1)
				status = HISTStart();
			if (status ==1)
			{
				status = LMStart();

				if (status != 1)
				{
					pView->m_Status.SetWindowText("LMStart function failed");
					// Send a stop command to the CC
					m_Dn_Step = 1;
					m_bDownFlag = FALSE;
					m_RunFlag = FALSE;	// return to home
					pView->ProcessButton();
				}
				else
				{
					// Send a go to the CC
					pView->ProcessIcon(TRUE);
					sprintf(cmdbuf,"L1");
					ret = SendCmd(cmdbuf,buffer, 64, 10);
					pView->ProcessIcon(TRUE);
					sprintf(cmdbuf,"G");
					ret = SendCmd(cmdbuf,buffer, 64, 10);
					pView->ProcessIcon(TRUE);
					sprintf(cmdbuf,"Q1");
					ret = SendCmd(cmdbuf,buffer, 64, 10);
					pView->ProcessIcon(TRUE);

					// Set scan start time
					GetLocalTime(&st);
					m_timeStart.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
					m_timeStart_msec = st.wMilliseconds;

					// Open the .hc file
					//sprintf(fname ,"%s%s-%s.hc", pDHI.m_Work_Dir, m_FName,m_ext_fname);
					sprintf(fname ,"%s%s-%s.hc", pDHI.m_CurrentDir, m_FName,m_ext_fname);
					// Make sure the file is closed
					if (hc_file != NULL)
					{	
						fclose(hc_file);
						hc_file = NULL;
					}
					hc_file = NULL;
					hc_file = fopen(fname,"w");
					if (hc_file != NULL)
						fprintf(hc_file,"Singles,Randoms,Prompts,Time(ms)\n");
					m_Dn_Step = 8;
				}
			}
			else
			{
					pView->m_Status.SetWindowText("HISTStart function failed");
					// Send a stop command to the CC
					m_Dn_Step = 1;
					m_bDownFlag = FALSE;
					m_RunFlag = FALSE;	// return to home
					pView->ProcessButton();
			}
		}
		break;

	case 8:
	
		// see if scan is done 
		GetLocalTime(&st);
		// initialize the time
		m_timeEnd.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
		spanElapsed = m_timeEnd - m_timeStart;
		// Convert it to seconds
		//tottime = spanElapsed * 24.0 * 3660.0;
		tottime = spanElapsed.GetTotalSeconds();
		if (tottime > pView->m_Xmax)
		{
			pView->m_Xmax += 10;
			pView->Invalidate(TRUE);
		}

		//if ((tottime > m_preset+10) || pCB.m_done)
		if (pCB.m_done)
		{
			// The scan is done

/*			// Send a stop command
			pSerBus.serialbus_access_control->Lock();
			sprintf(cmdbuf,"64H");
			pSerBus.SendCmd(cmdbuf);
			ret = pSerBus.GetResponse(buffer, 2048, 1);
			pSerBus.serialbus_access_control->Unlock();
*/		
			if (hc_file != NULL)
			{	
				fclose(hc_file);
				hc_file = NULL;
			}
			m_Dn_Step = 6;
			m_RunFlag = FALSE;
			// Scan is done
			pView->ProcessButton();
			
			if (m_Scan_Mode == 0)
			//	AppendHeaderFile();	// Update deader file
			//else
				pView->m_Status.SetWindowText("Scan is Done");
			//pView->Invalidate(TRUE);
			pView->RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE );
			// Release the blue screaner
			/*if (m_LMInit_Flag)
			{
				status = LMInit(NULL, NULL, NULL);
				m_LMInit_Flag = FALSE;
			}*/
		}
		else
		{

			// Wait 30 seconds on the driver to stop the scan.
			// If not, force a stop.
			if (tottime > (double)(m_preset + 30))
			{

				// Force a stop
				StopScan();
			}
			
		}
		break;
		
	}

}

void CScanItDoc::GetData(CString str)
{
	static long inc = 0;
	bool DoIt = FALSE;
	int i,x, ret;
	unsigned long tmx,hr,min,sec;
	CString tm;
//	double num;

	// Parse to get data
	x = 7;// str.Find("64! 203",1);
	for (i=0;i<6;i++)
	{
		if (i< 5)
		{			
			// Parse out spaces
			while(str.Mid(x+1,1) == ' ')
				x++;
			ret = str.Find(' ',x+1);
		}
		else
			ret = str.GetLength();

		switch(i)
		{
		
		case 0: 
			// Singles
			m_Singles=atoi(str.Mid(x+1,ret-x-1));
			pView->m_Sngls.SetWindowText(str.Mid(x+1,ret-x-1));
			break;	
		case 1:
			// Get Randoms
			m_Randoms=atoi(str.Mid(x+1,ret-x-1));
			pView->m_Rndms.SetWindowText(str.Mid(x+1,ret-x-1));
			break;					
		case 2: 
			// Get Prompt
			m_Prompts=atoi(str.Mid(x+1,ret-x-1));
			
			break;
		case 3:
			// Get Time
			m_Time=atoi(str.Mid(x+1,ret-x-1));
			if (m_Time < 60000)
				tm.Format("%3.2f Sec",(float)m_Time/1000.);	
			else
			{
				hr = m_Time / 3600000l;
				min = (m_Time - hr*3600000l) /60000;
				sec = m_Time  - hr*3600000l - min * 60000;
				tm.Format("%02d:%02d:%02d",hr,min,sec/1000);	
			
			}		
			if (m_Dn_Step >= 8)
				pView->m_Tm.SetWindowText(tm);
			break;
		case 4:

			break;
		
		case 5:
			if (m_Dn_Step >= 8)
				pView->m_Status.SetWindowText(str.Mid(x+2,ret-x-3));
			break;					

		}
		x = ret;
		
	}
	// Update the trues
	str.Format("%d",m_Prompts-m_Randoms);
	pView->m_Prmpts.SetWindowText(str);

	if (m_Dn_Step == 8)
	{
		if (m_index >= m_Mem_Alloc) 
		{
			m_Mem_Alloc += 100;
			
			lData = (long *)realloc( lData, m_Mem_Alloc * sizeof( long ) );
			lTrues = (long *)realloc( lTrues, m_Mem_Alloc * sizeof( long ) );
			lIndex = (long *)realloc( lIndex, m_Mem_Alloc * sizeof( long ) );
		}
		// make sure inc >= 0
		if (inc < 0) inc = 0;
		if ((m_Scan_Mode == 0 && m_Scan_Type !=1)|| (m_Preset_Opt ==0 && m_Scan_Type !=1) )
		//if (m_Scan_Mode == 0 || (m_Preset_Opt ==0 && m_Scan_Type !=1) )
		{
			if (m_preset < 40000)
				DoIt = TRUE;
			else
				if(inc == 0)
					DoIt = TRUE;
				else
					DoIt = FALSE;
				inc ++;
				if (inc > 5)
				{
					DoIt = TRUE;
					inc = 0;
				}
		}
		else
		{
			DoIt = TRUE;
		}		

		// make sure the time + 1 is greater then time
		if (m_index !=0 && lIndex[m_index-1] > m_Time/1000)
		{
			DoIt = FALSE;
		}
		// Take data only if it is allowed
		if (DoIt)
		{

			lIndex[m_index] = m_Time/1000;
			lData[m_index] = m_Randoms;
			m_Rnd_Tot += m_Randoms;
			lTrues[m_index] = m_Prompts-m_Randoms;
			if (lTrues[m_index]<0)
				lTrues[m_index] = 0;
			m_Trs_Tot += lTrues[m_index];

			if (pView->m_Move_En==1 && lIndex[m_index] > pView->m_Move_Xmax)
			{
				pView->m_Move_Xmax = lIndex[m_index] + 1;
				pView->m_Move_Xmin = pView->m_Move_Xmax - pView->m_Move_Dur;
				//pView->Invalidate(TRUE);
				pView->DrawMoveWin();

			}

			
			m_index++;

			pView->DrawGraph();

		}
		
		// write the data to *.s.hc file
		if (hc_file != NULL)
			fprintf(hc_file,"%d,%d,%d,%d\n",m_Singles,m_Randoms,m_Prompts,m_Time);
//		num = (double)m_Rnd_Tot;
//		str.Format("%G",num);
		str.Format("%I64d", m_Rnd_Tot);
		pView->m_Rnd_Tot.SetWindowText(str);
		//num = (double)m_Trs_Tot;
		//str.Format("%G",num);
		str.Format("%I64d", m_Trs_Tot);
		pView->m_Trs_Tot.SetWindowText(str);

		// update total events from LM dll
		str.Format("%d",pLMStats.total_event_rate);
		pView->m_Tot_Events.SetWindowText(str);
		if (m_Scan_Type != 1)
		{
			// update time remaining from LM dll
			tmx=pLMStats.time_remaining;
			if (tmx < 60)
				tm.Format("%02d Sec",tmx);	
			else
			{
				hr = tmx / 3600l;
				min = (tmx - hr*3600l) /60;
				sec = tmx  - hr*3600l - min * 60;
				tm.Format("%02d:%02d:%02d",hr,min,sec);	
			}	
			pView->m_Tm_Rem.SetWindowText(tm);
		}
		else
			// Just print a blank
			pView->m_Tm_Rem.SetWindowText(" ");
	}
}



void CScanItDoc::OnFileOpen() 
{
	CString strFilter,str;
	bool res = FALSE;


	// Set the filter and set dialogfile 
	strFilter = "Header files (*.cnfg)|*.cnfg|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);


	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_CfgFileName = dlg.GetPathName();
		res = GetConfigFile();

	}
	if (res)
	{
		//CConfigDlg dlg;
		//dlg.DoModal();
		str.Format("%s, Build %s %s", dlg.GetFileTitle(), __DATE__, __TIME__);
		SetTitle (dlg.GetFileTitle());
		// Check if the user wants to setup the scanner			
		HWND hWnd = ::AfxGetMainWnd()->GetSafeHwnd();
		int a = MessageBox(hWnd,"Would you like to configure the scanner?","Gantry Response Error",MB_YESNO  | MB_ICONQUESTION); //| MB_ICONWARNING
		if (a == 6)
		{
			m_bDownFlag = TRUE;
			m_Dn_Step = 1;
		}
		else
		{
			m_bDownFlag = FALSE;
			m_RunFlag = FALSE;	// return to home
			m_Dn_Step = 1;
		}
	}
}

bool CScanItDoc::GetConfigFile()
{
	CString str,newstr;

	char t[256];
	// Open the header file
	FILE  *in_file;
	in_file = fopen(m_CfgFileName,"r");
	if( in_file!= NULL )
	{
		// Read teh file line by line and sort data
		while (fgets(t,sizeof(t),in_file) != NULL )
		{			
			str.Format ("%s",t);
			if (str.Find(":=")!=-1)				
				//  What type of system
				pDHI.SortData (str, &newstr);
			if(str.Find("Scan Type (0=emission, 1=transmission) :=")!=-1)
				m_Scan_Type = atoi(newstr);
			if(str.Find("Scan Mode (0=listmode, 1=histogram) :=")!=-1)
				m_Scan_Mode = atoi(newstr);
			if(str.Find("Energy window lower level [1] :=")!=-1)
				m_lld = atoi(newstr);
			if(str.Find("Energy window upper level [1] :=")!=-1)
				m_uld = atoi(newstr);
			if(str.Find("Scan duration :=")!=-1)
				m_Duration = atoi(newstr);
			if(str.Find("Scan duration type (0=sec, 1=min, 2=hr) :=")!=-1)
				m_DurType = atoi(newstr);
			if(str.Find("Scan speed :=")!=-1)
				m_Speed = (float)atof(newstr);
			if(str.Find("Crystal skip :=")!=-1)
				m_Plane = atoi(newstr);
			if(str.Find("Ring Difference and Scan span option :=")!=-1)
			{
				m_Ary_Select = atoi(newstr);
				if (m_Ary_Select < 0 || m_Ary_Select >4)
					m_Ary_Select =3;
				m_Span = m_Ary_Span[m_Ary_Select];
				m_Ring = m_Ary_Ring[m_Ary_Select];
			
			}

			if(str.Find("Segment zero only :=")!=-1)
			{
				if(str.Find("TRUE")!=-1)
					m_Segment_Zero_Flag = TRUE;
				else
					m_Segment_Zero_Flag = FALSE;				
			}
			if(str.Find("Preset option (0=time, 1=count) :=")!=-1)
				m_Preset_Opt = atoi(newstr);
			if(str.Find("Count option (0=net trues, 1=randoms+propmpts) :=")!=-1)
				m_Count_Opt = atoi(newstr);
			if(str.Find("64 bit listmode option (0=disable, 1=enabled) :=")!=-1)
				m_List_64bit = atoi(newstr);
			if(str.Find("Start on counts option (0=disable, 1=enabled) :=")!=-1)
				m_Auto_Start = atoi(newstr);
			if(str.Find("Count trigger point in KCPS :=")!=-1)
				m_KCPS = atol(newstr);
			if(str.Find("Frame definition :=")!=-1)
				m_DynaFrame = newstr;
		

		}
		// Close the header file
		fclose(in_file);
		return TRUE;
	}
	else
	{
		//MessageBox("Header file could not be opened to read data","Open Header File",MB_OK);
		return FALSE;
	}
}


void CScanItDoc::OnFileSave() 
{
	CString strFilter, str;
	bool res = FALSE;
	// Set the filter and set dialogfile 
	strFilter = "Scan configuration files (*.cnfg)|*.cnfg|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,"cnfg",NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);


	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_CfgFileName = dlg.GetPathName();

		SaveConfigFile();		
		str.Format("%s, Build %s %s", dlg.GetFileTitle(), __DATE__, __TIME__);
		SetTitle (dlg.GetFileTitle());
	
	}	
}

void CScanItDoc::SaveConfigFile()
{
	FILE  *out_file;
	out_file = fopen(m_CfgFileName,"w");
	
	fprintf( out_file, "Scan Type (0=emission, 1=transmission) := %d\n", m_Scan_Type );
	fprintf( out_file, "Scan Mode (0=listmode, 1=histogram) := %d\n",m_Scan_Mode);
	fprintf( out_file, "Energy window lower level [1] := %d\n",m_lld);
	fprintf( out_file, "Energy window upper level [1] := %d\n",m_uld);
	fprintf( out_file, "Scan duration := %d\n",m_Duration);
	fprintf( out_file, "Scan duration type (0=sec, 1=min, 2=hr) := %d\n",m_DurType);
	fprintf( out_file, "Scan speed := %2.1f\n",m_Speed);
	fprintf( out_file, "Crystal skip := %d\n",m_Plane);
	fprintf( out_file, "Ring Difference and Scan span option := %d\n",m_Ary_Select);
	if(m_Segment_Zero_Flag)
		fprintf( out_file, "Segment zero only := TRUE\n");
	else
		fprintf( out_file, "Segment zero only := FALSE\n");
	fprintf( out_file, "Preset option (0=time, 1=count) := %d\n",m_Preset_Opt);
	fprintf( out_file, "Count option (0=net trues, 1=randoms+propmpts) := %d\n",m_Count_Opt);
	fprintf( out_file, "64 bit listmode option (0=disable, 1=enabled) := %d\n",m_List_64bit);

	fprintf( out_file, "Start on counts option (0=disable, 1=enabled) := %d\n",m_Auto_Start);
	fprintf( out_file, "Count trigger point in KCPS := %d\n",m_KCPS);
	fprintf( out_file, "Frame definition := %s\n", m_DynaFrame);



	if( out_file!= NULL )
		fclose(out_file);
}

void CScanItDoc::CheckError(int err)
{
  if (pView == NULL) return; // Application exiting, no active window;
	if (pSerBus.m_Com_Err)
	{
		// Test communication port
		pView->m_Status.SetWindowText("Communication Error!");
		pView->ProcessIcon(!pSerBus.m_Com_Err);
	}
	else 
	{
		CString str;
		pView->m_Status.GetWindowText(str);
		if (str == "Communication Error!")
			pView->m_Status.SetWindowText(" ");
	}
	// See if the LM dll failed
	if (!m_LMInit_Flag)
			pView->m_Status.SetWindowText("Listmode dll did not initialize");
		else 
			// see if the HIST dll failed
			if (!m_HISTInit_Flag)
				pView->m_Status.SetWindowText("Histogram dll did not initialize");
	
	// See if there is a CC or DHI error
	if (err < 0)
	{
		CString str,fstr;
		if (err > MAXERRORCODE)
			fstr.Format("%s\n",e_short_text[-1*err]);
		else
			fstr = ("Unknow Error\n");
		str = fstr + "Would you like to continue?";
		pView->m_Status.SetWindowText(fstr);
		if (pDHI.m_msg_err)
		{
			HWND hWnd = ::AfxGetMainWnd()->GetSafeHwnd();
			int a = MessageBox(hWnd,str,"Gantry Response Error",MB_YESNO  | MB_ICONQUESTION); //| MB_ICONWARNING
			if (a == 7)
			{
				m_bDownFlag = FALSE;
				m_RunFlag = FALSE;	// return to home
				m_Dn_Step = 1;
			
			}
		}
		if(pDHI.m_LogFlag)
		{
			FILE* out_file;
			out_file = fopen(m_errfile,"a");
			if (out_file != NULL)
			{
				SYSTEMTIME st;
				GetLocalTime(&st);

				fprintf( out_file, "Time: %2d:%2d:%2d\n", st.wHour,st.wMinute,st.wSecond);	 
				fprintf( out_file, "Date: %2d:%2d:%4d\n", st.wDay,st.wMonth,st.wYear);	 
				fprintf( out_file, "%s", fstr );	 
				fclose (out_file);
			}
		}
	}

}

int CScanItDoc::InitializeLM()
{
	int status;
	static char fname[128];
	bool presetMode=FALSE;
	unsigned long bypass = 0;

#ifdef NEWACS_SUPPORT  // added 07oct2010: Merence Sibomana
	// initialize the listmode dll
	if (!m_LMInit_Flag)
		status = LMInit(&pLMStats, &pCB, &pSinoInfo);
	if (status!=1)
		m_LMInit_Flag = FALSE;
	else
		m_LMInit_Flag = TRUE;

	
	status = HISTInit(&pHISTStats, &pCB, &pSinoInfo);
	if (status!=1)
		m_HISTInit_Flag = FALSE;
	else
		m_HISTInit_Flag = TRUE;
#endif


	//sprintf(fname ,"%s%s-%s.l", pDHI.m_Work_Dir,m_FName,m_ext_fname);
	if (m_List_64bit == 0)
		sprintf(fname ,"%s%s-%s.l32", pDHI.m_CurrentDir,m_FName,m_ext_fname);
	else
		sprintf(fname ,"%s%s-%s.l64", pDHI.m_CurrentDir,m_FName,m_ext_fname);

	if(m_Scan_Mode ==0)
		m_FileName.Format("%s",fname);
		

	if (m_Scan_Mode == 0)
		bypass = m_List_64bit & 1;
	else
		bypass = 0;
	
	status = LMConfig(fname, m_preset, presetMode, m_Scan_Mode, bypass, m_rebinnerInfo);


	if (status != 1)
	{
		pView->m_Status.SetWindowText("LMConfig function failed");
		m_Dn_Step = 1;
		m_bDownFlag = FALSE;
		m_RunFlag = FALSE;	// return to home
		// Update buttons
		pView->ProcessButton();
		return 0;
	}

	return 1;
}


int CScanItDoc::InitializeHIST()
{
	static int status, em_sinm = 0, tx_sinm=0, acqm;
	static char fname[128];

	// Unconfig
	status = HISTUnconfig();
	// Set the histogram attribute
	status = HISTSetAttributes(m_nprojs, m_nviews, m_n_em_sinos, m_n_tx_sinos);
	if (status != 1)
	{
		pView->m_Status.SetWindowText("HISTSetAttributes failed");
		m_Dn_Step = 1;
		m_bDownFlag = FALSE;
		m_RunFlag = FALSE;	// return to home
		// Update buttons
		pView->ProcessButton();
		return 0;
	}
	else
	{
		// Define the histogram frame
		if (m_Scan_Mode ==0)
			m_Preset_Opt = 0;
		else
			m_Preset_Opt &= 1;

		status = HISTDefineFrames(m_nframes, m_Preset_Opt, m_preset);	//frame = 1, criteria =0 for time or 1 for counts, m_preset 
		if (status != 1)
		{
			pView->m_Status.SetWindowText("HISTDefineFrames function failed");
			m_Dn_Step = 1;
			m_bDownFlag = FALSE;
			m_RunFlag = FALSE;	// return to home
			// Update buttons
			pView->ProcessButton();
			return 0;
		}
		else
		{
			// Set the sinogram mode Prompts, or Randoms + Trues
			if(m_Scan_Type ==0)
			{
				em_sinm |= m_Count_Opt & 1;
				tx_sinm = 0;
				acqm = EM_ONLY;
			}
			else
			{
				em_sinm = 0;
				tx_sinm |= m_Count_Opt & 1;
				acqm=TX_ONLY;
			}
			status = HISTSinoMode (em_sinm, tx_sinm, acqm);  //0 for net trues, 1 for prompts + randoms
			if (status != 1)
			{
				pView->m_Status.SetWindowText("HISTSinoMode function failed");
				m_Dn_Step = 1;
				m_bDownFlag = FALSE;
				m_RunFlag = FALSE;	// return to home
				// Update buttons
				pView->ProcessButton();
				return 0;
			}
			else
			{
				// Config
				//sprintf(fname ,"%s%s-%s.s", pDHI.m_Work_Dir, m_FName,m_ext_fname);
				sprintf(fname ,"%s%s-%s.s", pDHI.m_CurrentDir, m_FName,m_ext_fname);
				m_FileName.Format("%s",fname);
					

				status = HISTConfig(fname);
				if (status != 1)
				{
					pView->m_Status.SetWindowText("HISTConfig function failed");
					m_Dn_Step = 1;
					m_bDownFlag = FALSE;
					m_RunFlag = FALSE;	// return to home
					// Update buttons
					pView->ProcessButton();
					return 0;
				}
				else
					return 1;
			}
		}
	}

}



void CScanItDoc::SetScanParam()
{
	// fill pSinoInfo structure 
	//if (pDHI.m_HRRT)
	//	pSinoInfo.gantry = 0;	// HRRT
	// esle set up for other types of scanner
	
	// Initialize Sino Info
	InitSinoInfo();

	if (m_Scan_Type ==0) // em
	{

		m_Span = m_Ary_Span[m_Ary_Select];
		m_Ring = m_Ary_Ring[m_Ary_Select];
		if (m_Segment_Zero_Flag)
		{
			m_n_em_sinos = 207;
			m_n_tx_sinos = 0;
		}
		else
		{
			m_n_em_sinos = m_en_sino[m_Ary_Select];
			m_n_tx_sinos = 0;
		}
	}
	else
	{	// tx
		m_Span = m_Ary_Span[3];
		m_Ring = m_Ary_Ring[3];
		m_n_em_sinos = 0;
		m_n_tx_sinos = 207;

	}
}

void CScanItDoc::AppendHeaderFile()
{
	double dtc, EnergyLevel = 0.0;
	long SinPerBlock = 0;
	char t[256];
	// Update status
	pView->m_Status.SetWindowText("Updating header file");
	FILE *out_file;
	CString filename, str, newstr;
/*	static char *doses[] =
	{
		"None Selected",
		"F-18 Solution",
		"FDG",
		"N-13 Ammonia",
		"O-15 Water",
		"O-15 CO",
		"F-18 Dopa",
		"C-11 Palmitate",
		"C-11 Methionine",
		"C-11 Acetate",
		"C-11 Deprenyl",
		"C-11 Flumazenil",
		"Rb-82 Solution",
		"Ge-68 Liquid Solution",
		"Ge-68 Solid Solution",
	};
*/

	static char *Units[] = 
	{
		"Mega-Bq",
		"Milli-Ci"	
	};
	// Get energy level factor for dead time correction
	// do a linear exrapulation between the energy points
	if (m_lld == 250)
		EnergyLevel = 6.9378e-6;
	if (m_lld > 250  && m_lld <300)
		EnergyLevel = (((double)m_lld - 250.0)/50.0) * (7.0677e-6 - 6.9378e-6) + 6.9378e-6;
	if (m_lld == 300)
		EnergyLevel = 7.0677e-6;
	if (m_lld > 300  && m_lld <350)
		EnergyLevel = (((double)m_lld - 300.0)/50.0) * (7.7004e-6 - 7.0677e-6 ) + 7.0677e-6;
	if (m_lld == 350)
		EnergyLevel = 7.7004e-6;
	if (m_lld > 350  && m_lld <400)
		EnergyLevel = (((double)m_lld - 350.0)/50.0) * (8.94e-6 - 7.7004e-6) + 7.7004e-6;
	if (m_lld == 400)
		EnergyLevel = 8.94e-6;

	// open the proper header file
	if (m_Scan_Mode == 0)
		//filename.Format("%s%s-%s.l.hdr", pDHI.m_Work_Dir, m_FName,m_ext_fname);
		if (m_List_64bit == 0)
			filename.Format("%s%s-%s.l32.hdr", pDHI.m_CurrentDir, m_FName,m_ext_fname);
		else
			filename.Format("%s%s-%s.l64.hdr", pDHI.m_CurrentDir, m_FName,m_ext_fname);
	else
		//filename.Format("%s%s-%s.s.hdr", pDHI.m_Work_Dir, m_FName,m_ext_fname);
		filename.Format("%s%s-%s.s.hdr", pDHI.m_CurrentDir, m_FName,m_ext_fname);

	out_file = fopen(filename,"r");

	
	if (out_file)
	{
		while (fgets(t,256,out_file) != NULL )
		{			
			str.Format ("%s",t);
			if (str.Find(":=")!=-1)	
				pDHI.SortData (str, &newstr);
			if(str.Find("average singles per block :=")!=-1 )  
			{
				SinPerBlock = atol(newstr);
				break;
			}
		}
		fclose(out_file);
		out_file = fopen(filename,"a");
		if (out_file)
		{
			dtc = exp(EnergyLevel*(double)SinPerBlock);

			fprintf( out_file, "Patient name := %s, %s\n", m_Pt_Lst_Name, m_Pt_Fst_Name );	 
			fprintf( out_file, "Patient DOB := %s\n", m_Pt_DOB );	 
			fprintf( out_file, "Patient ID := %s\n", m_Pt_ID );	 
			if (m_Pt_Sex==0)
				fprintf( out_file, "Patient sex := Male\n");
			else if(m_Pt_Sex == 1)
				fprintf( out_file, "Patient sex := Female\n");
			else
				fprintf( out_file, "Patient sex := Unknown\n");
			if (m_Pt_Dose <0 || m_Pt_Dose > 16) m_Pt_Dose = 0;
			{
				fprintf( out_file, "Dose type := %s\n",  doses[m_Pt_Dose] );	
				fprintf( out_file, "isotope halflife := %f\n",  halflife[m_Pt_Dose] );	
				fprintf( out_file, "branching factor := %f\n",  branch_Frac[m_Pt_Dose] );	
			}
			if (m_Strength_Type <0 || m_Strength_Type > 1) m_Strength_Type = 0;
			fprintf( out_file, "Dosage Strength := %f %s\n",  m_Strength, Units[m_Strength_Type]);	 
			// dead time correction only for sinogram
			if (m_Scan_Type == 0)
				fprintf( out_file, "Dead time correction factor := %e\n", dtc);
			else
			{
 				fprintf( out_file, "axial velocity := %f\n",  m_Speed);	 
				fprintf( out_file, "transaxial skip := %d\n",  m_Plane);	 
			}
			//fprintf( out_file, "Calibration Factor := %e\n",  pDHI.m_Cal_Factor);	 
			fprintf(out_file, "Dead time constant := %g\n", pDHI.m_DTC);

			fclose(out_file);
		}
	}
	
	pView->m_Status.SetWindowText("Scan is Done");
}



void CScanItDoc::UpDateSaveFileFlag()
{
	// Save the file
	OnFileSave();
}

void CScanItDoc::SetFileName()
{
	CString str, str2;
	bool dashflag = FALSE;
	//char z[126];

	if (m_Pt_Lst_Name.IsEmpty() && m_Pt_Fst_Name.IsEmpty())
	{	 
		str2.Format("%d.%d.%d.%d.%d.%d",m_timeStart.GetYear(), m_timeStart.GetMonth(),m_timeStart.GetDay(),m_timeStart.GetHour(),m_timeStart.GetMinute(),m_timeStart.GetSecond());

		str += pDHI.m_Work_Dir;
		str += str2;
		str += "\\";
	}

	else
	{
		str = pDHI.m_Work_Dir + m_Pt_Lst_Name + "_" + m_Pt_Fst_Name + "\\";
		CreateDirectory(str,NULL);
		if (m_Scan_Type == 1) // tx scan
			str = pDHI.m_Work_Dir + m_Pt_Lst_Name + "_" + m_Pt_Fst_Name + "\\" +"Transmission\\";
	}

	pDHI.m_CurrentDir = str;


	CreateDirectory(str,NULL);

	//sprintf(z,"Directory Error = %d",GetLastError());
	//OutputDebugString(LPCTSTR(z));

	m_FName = "";
	if(pDHI.m_Opt_Name)
	{
		m_FName.Format("%s-%s",m_Pt_Lst_Name,m_Pt_Fst_Name);
		dashflag = TRUE;
	}
	if(pDHI.m_Opt_PID)
	{
		if (dashflag) 
			m_FName += "-";
		m_FName += m_Pt_ID;
		dashflag = TRUE;
	}
	if(pDHI.m_Opt_CID)
	{
		if (dashflag) 
			m_FName += "-";
		m_FName += m_Case_ID;
		dashflag = TRUE;
	}
	if(pDHI.m_Opt_Num)
	{
		if (dashflag) 
			m_FName += "-";
		int rnd;
		srand( (unsigned)time( NULL ) );
		rnd = rand();
		str.Format("%d",rnd);
		m_FName += str;
	}



}

void CScanItDoc::StopScan()
{
	char buffer[2048], cmdbuf[25];
	int  ret;
	int status;
	// Send a stop command
	
	pSerBus.serialbus_access_control->Lock();
	sprintf(cmdbuf,"64H");
	pSerBus.SendCmd(cmdbuf);
	ret = pSerBus.GetResponse(buffer, 2048, 1);
	pSerBus.serialbus_access_control->Unlock();
	
	m_RunFlag = FALSE;
	
	

	if(m_Scan_Mode ==1)
	{
		status = HISTStop();
		if (status !=1)
			pView->m_Status.SetWindowText("HISTStop function failed");
		//AppendHeaderFile();	// Update deader file
		status = LMHalt();
	}
	else
	{
		status = LMStop();
		
		if (status !=1)
			pView->m_Status.SetWindowText("LMStop function failed");
	}	
/*	// Release the blue screaner
	if (m_LMInit_Flag)
	{
		status = LMInit(NULL, NULL, NULL);
		m_LMInit_Flag = FALSE;
	}
*/	if (hc_file != NULL)
	{	
		fclose(hc_file);
		hc_file = NULL;
	}
	m_Dn_Step = 6;
}

void CScanItDoc::AbortScan()
{
	char buffer[2048], cmdbuf[25];
	int  ret;
	int status;
	// Send a stop command
	pSerBus.serialbus_access_control->Lock();
	sprintf(cmdbuf,"64H");
	pSerBus.SendCmd(cmdbuf);
	ret = pSerBus.GetResponse(buffer, 2048, 1);
	pSerBus.serialbus_access_control->Unlock();
	if(m_Scan_Mode ==1)
	{
		status = HISTAbort();
		if (status !=1)
			pView->m_Status.SetWindowText("HISTStop function failed");
		//AppendHeaderFile();	// Update deader file
		status = LMHalt();
	}
	else
	{
		status = LMAbort();
		
		if (status !=1)
			pView->m_Status.SetWindowText("LMStop function failed");
	}
	

/*	// Release the blue screaner
	if (m_LMInit_Flag)
	{
		status = LMInit(NULL, NULL, NULL);
		m_LMInit_Flag = FALSE;
	}
*/	if (hc_file != NULL)
	{	
		fclose(hc_file);
		hc_file = NULL;
	}	
	m_RunFlag = FALSE;
	m_Dn_Step = 6;
}

void CScanItDoc::GetTruesAvg()
{
	static int num = 0;
	int i;
	CString str;
	
	if (num >= 19) 
	{
		// Move the array by one
		for(i=0;i<18;i++)
			m_Total_Trues[i]=m_Total_Trues[i+1];
		// Store the trues in the last member of the array
		num = 19;
		m_Total_Trues[num] = m_Prompts-m_Randoms;
		
	}
	else
	{
		// Fill the array
		m_Total_Trues[num] = m_Prompts-m_Randoms;
		num ++;
	}

	m_Trues_Move_Avg = 0;
	for(i=0;i<=num;i++)
		m_Trues_Move_Avg+=m_Total_Trues[i];
	// Get the average
	m_Trues_Move_Avg = m_Trues_Move_Avg/ (num + 1);

	
	// See if the Net Trues are above the baseline + KCPS
	if (!m_RunFlag)
	{
		if ((m_Prompts-m_Randoms) > (m_Trues_Move_Avg + (m_KCPS * 1000)))
		{
			/*// Zero the array
			for(i=0;i<=19;i++)
				m_Total_Trues[i]=0;
			num = 0;
			*/
			m_RunFlag = TRUE;
		}
		else
		{
			if (m_Dn_Step == 6)
			{
				str.Format("Ready To Scan, Net Average = %d",m_Trues_Move_Avg);
				pView->m_Status.SetWindowText(str);
			}
		}

	}


}

void CScanItDoc::ListHeaderFile()
{
	// Update status
	CString filename;
	pView->m_Status.SetWindowText("Updating header file");
	FILE *out_file;
	//filename.Format("%s%s-%s.l.hdr", pDHI.m_Work_Dir, m_FName,m_ext_fname);
	if (m_List_64bit == 0)
		filename.Format("%s%s-%s.l32.hdr", pDHI.m_CurrentDir, m_FName,m_ext_fname);
	else
		filename.Format("%s%s-%s.l64.hdr", pDHI.m_CurrentDir, m_FName,m_ext_fname);

	out_file = fopen(filename,"w");
	if(out_file != NULL)
	{
		fprintf( out_file, "!INTERFILE\n");
		fprintf( out_file, "!originating system := HRRT\n");
		//fprintf( out_file, "!name of data file := %s%s-%s.l\n", pDHI.m_Work_Dir,m_FName,m_ext_fname);
		if (m_List_64bit == 0)
			fprintf( out_file, "!name of data file := %s%s-%s.l32\n", pDHI.m_CurrentDir,m_FName,m_ext_fname);
		else
			fprintf( out_file, "!name of data file := %s%s-%s.l64\n", pDHI.m_CurrentDir,m_FName,m_ext_fname);

		fprintf( out_file, "!study date (dd:mm:yryr) := %02d:%02d:%4d\n", m_timeStart.GetDay(), m_timeStart.GetMonth(), m_timeStart.GetYear());
		fprintf( out_file, "!study time (hh:mm:ss) := %02d:%02d:%02d\n", m_timeStart.GetHour(),
			m_timeStart.GetMinute(), m_timeStart.GetSecond());
		fprintf( out_file, "!study time (hh:mm:ss.sss) := %02d:%02d:%02d.%03d\n", m_timeStart.GetHour(),
			m_timeStart.GetMinute(), m_timeStart.GetSecond(), m_timeStart_msec);
		if (m_Scan_Type == 0)
			fprintf( out_file, "!PET data type := emission\n");
		else
			fprintf( out_file, "!PET data type := transmission\n");
		fprintf( out_file, "data format := listmode\n");

		if (m_List_64bit == 1)
		{
			fprintf( out_file, "axial compression := 0\n");
			fprintf( out_file, "maximum ring difference := 103\n");
		}
		else
		{
			if (m_Scan_Mode == 0)
			{
				fprintf( out_file, "axial compression := %d\n", m_Ary_Span[m_Ary_Select]);
				fprintf( out_file, "maximum ring difference := %d\n", m_Ary_Ring[m_Ary_Select]);
			}
			else
			{
				fprintf( out_file, "axial compression := 0\n");
				fprintf( out_file, "maximum ring difference := %d\n", m_Ary_Ring[m_Ary_Select]);
			}
		}
		fprintf( out_file, "energy window lower level[1] := %d\n", m_lld);
		fprintf( out_file, "energy window upper level[1] := %d\n", m_uld);
		fprintf( out_file, "image duration := %d\n", m_preset);
		fprintf( out_file, "Frame definition := %s\n", m_DynaFrame);
		
		//	close the file
		fclose(out_file);
		AppendHeaderFile();
		
	}

}

void CScanItDoc::ConfigRebinner()
{
	//  Set the rebinner card configuration
	if (m_Ary_Select == 0)
	{
		m_rebinnerInfo.lut = 0;
		m_rebinnerInfo.span = m_Ary_Span[m_Ary_Select];
		m_rebinnerInfo.ringDifference = m_Ary_Ring[m_Ary_Select];
	}
	else
	{
		m_rebinnerInfo.lut = 1;
		m_rebinnerInfo.span = m_Ary_Span[m_Ary_Select];
		m_rebinnerInfo.ringDifference = m_Ary_Ring[m_Ary_Select];
	}
}

int CScanItDoc::SendCmd(char *cmdSnd, char *cmdRcv, int hd, int timeout)
{

	int ret = pDHI.SendCmd(cmdSnd, cmdRcv, hd, timeout);
	if (ret <0)
  {
    cmdRcv[0] = '\0'; // empty string
    m_err_num = 0;
		if (!m_ShuttingDown) CheckError(m_err_num);
		return m_err_num;
	}
	else
		return ret;
}

void CScanItDoc::InitSinoInfo()
{
// This will change acording to setting file
	
	pSinoInfo.fileType = m_Scan_Mode + 1;	// LM
	pSinoInfo.gantry =  GNT_HRRT; //
	pSinoInfo.scanType = m_Scan_Type;	//em
	pSinoInfo.span = m_Span;
	pSinoInfo.ringDifference = m_Ring;
	pSinoInfo.lld = m_lld;
	pSinoInfo.uld = m_uld;
	pSinoInfo.presetTime = m_Duration;
	sprintf(pSinoInfo.gantryModel,"%d", model->modelNumber());

}



/*	int ret;
	char buffer[2048], cmdbuf[25];
	sprintf(cmdbuf,"L0");
	ret = SendCmd(cmdbuf,buffer, 64, 1);
*/
