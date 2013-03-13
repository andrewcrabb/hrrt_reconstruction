/*[ ProcessingThread.cpp
------------------------------------------------

   Name            : ProcessingThread.cpp - Thread where are executed reconstruction tasks.
                   : Communicates with ReconGUI using Windows Messages to get the command lines to be executed
				   : end to communicate back command acknowledgment and completion.
   Authors         : Merence Sibomana
   Language        : C++

   Creation Date   : 09/10/2004
  
               Copyright (C) CPS Innovations 2004 All Rights Reserved.

---------------------------------------------------------------------*/
/*
 *  Modification History:
 *  07-Oct-2008: Added TX TV3DReg to MAP-TR segmentation methods list (Todo: design better interface)
*/

#include "stdafx.h"
#include "ProcessingThread.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include <io.h>

// ProcessingTask

int ProcessingTask::SetName(const char *output_path)
{

	switch(task_ID)
	{
	case TASK_BL_HISTOGRAM:
		name = "BL Histogram  ";
		break;
	case TASK_TX_HISTOGRAM:
		name = "TX Histogram  ";
		break;
	case TASK_EM_HISTOGRAM:
		name = "EM Histogram  ";
		break;
	case TASK_TX_PROCESS:
		name = "TX Process    ";
		break;
	case TASK_TX_TV3D:
		name = "TX_TV3D       ";
		break;
	case TASK_AT_PROCESS:
		name = "AT Process    ";
		break;
	case TASK_SCAT_PROCESS:
		name = "SCAT Process  ";
		break;
	case TASK_DELAYED_SMOOTHING:
		name = "RA Smooth ";
		break;
	case TASK_RECON:
		name = "RECON         ";
		break;			
	case TASK_COPY_RECON_FILE:
		name = "Copy Recon files ";
		break;	
	case TASK_QUANTIFY_IMAGE:
		name = "Quantify Image ";
		break;
		break;	
	case TASK_HOSP_RECON:
		name = "HOSP Recon    ";
		break;
	case TASK_CLEAR_TMP_FILES:
		name = "Clear TMP Files ";
		break;
	default:
		name = "";
		return 0;
	}
	if (output_path != NULL)
	{
		output_fname = output_path;
		const char *fname = strrchr(output_path, '\\');
		if (fname != NULL) name += fname+1;
		else name += output_path;
	}
	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// ProcessingThread

IMPLEMENT_DYNCREATE(CProcessingThread, CWinThread)

BEGIN_MESSAGE_MAP(CProcessingThread, CWinThread)
	ON_THREAD_MESSAGE(WM_START_TASK, OnStartTask)
	ON_THREAD_MESSAGE(WM_TASK_READY, OnTaskReady)
END_MESSAGE_MAP()

CProcessingThread::CProcessingThread()
{
	pReconGUI = NULL;
	status = busy;
}

CProcessingThread::~CProcessingThread()
{
}

static void start_process(LPTSTR title, LPSTR cmd_line)
{
	STARTUPINFO stInfo;
	PROCESS_INFORMATION prInfo;
	BOOL bResult;
	ZeroMemory( &stInfo, sizeof(stInfo) );
	stInfo.cb = sizeof(stInfo);
	stInfo.dwFlags=STARTF_USESHOWWINDOW;
	stInfo.wShowWindow=SW_SHOWDEFAULT;//SW_MINIMIZE;
	stInfo.lpTitle = title;
	//try
	//{
		bResult = CreateProcess( NULL, 
							 cmd_line, 
							 NULL, 
							 NULL, 
							 TRUE,
							 CREATE_NEW_CONSOLE 
							 | NORMAL_PRIORITY_CLASS ,
							 NULL,
							 NULL ,
							 &stInfo, 
							 &prInfo);
	//}
	//catch (CException r)
	//{
		// Get the error if there is one
	//}
	// Close the Handle
	WaitForSingleObject(prInfo.hProcess,INFINITE);
	CloseHandle(prInfo.hThread); 

	// We are all done
	CloseHandle(prInfo.hProcess);
}

void CProcessingThread::ClearTmpFiles(const CString &tmp_files)
{
	int start=0;
	int end = tmp_files.Find(" ", start);
	CString filename;
	while (end>start)
	{
		filename = tmp_files.Mid(start,end-start);
		if (_unlink(filename) != 0)
		{
			if (pReconGUI != NULL)
				((CReconGUIDlg*)pReconGUI)->log_message(filename + " delete failed", 2);
		}
		start = end+1;
		end = tmp_files.Find(" ", start);
	}
	// delete last file
	if (start < tmp_files.GetLength())
	{
		filename = tmp_files.Mid(start);
		if (_unlink(filename) != 0) 
		{
			if (pReconGUI != NULL)
				((CReconGUIDlg*)pReconGUI)->log_message(filename + " delete failed", 2);
		}
	}
}

void CProcessingThread::OnStartTask(UINT msg, LONG par)
{
	CString  normfac;
	char drive[_MAX_DRIVE];
	char in_dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];

	status = busy;
	ProcessingTask *ptask = (ProcessingTask*)par;
	LPTSTR title=NULL;
	char *cmd_line = new char[ptask->cmd_line.GetLength()+1];
	strcpy(cmd_line, (const char*)ptask->cmd_line);
	switch(ptask->task_ID)
	{
	case TASK_BL_HISTOGRAM:
		title = "Blank Histogram";
		break;
	case TASK_TX_HISTOGRAM:
		title = "Transmission Histogram";
		break;
	case TASK_EM_HISTOGRAM:
		title = "Emission Histogram";
		break;
	case TASK_TX_PROCESS:
		title = "Transmission Process";
		break;
	case TASK_TX_TV3D:
		title = "TX_TV3D Process";
		break;
	case TASK_AT_PROCESS:
		title = "Attenuation Process";
		break;
	case TASK_SCAT_PROCESS:
		title = "Scatter Process";
		break;
	case TASK_RECON:
		title = "Reconstruction";
		_splitpath((const char*)ptask->output_fname, drive, in_dir, fname, ext);
		normfac.Format("%s%snormfac.i",drive,in_dir);
		if (_access(normfac,0) == 0) 
		{
			int ret= _unlink(normfac);
			if (pReconGUI != NULL)
			{
				if (ret==0) ((CReconGUIDlg*)pReconGUI)->log_message(normfac + " deleted");
				else ((CReconGUIDlg*)pReconGUI)->log_message(normfac + " delete failed", 2);
			}
			
		}
		break;			
	case TASK_DELAYED_SMOOTHING:
		title = "Random Smoothing";
		break;
	case TASK_COPY_RECON_FILE:
		title = "Copy Reconstruction files";
		break;		
		case TASK_QUANTIFY_IMAGE:
		if (pReconGUI != NULL) ((CReconGUIDlg*)pReconGUI)->QuantifyImage(cmd_line);
		break;			
	}

	if (pReconGUI != NULL) pReconGUI->PostMessage(WM_PROCESSING_THREAD, WM_START_TASK_OK, (LPARAM)ptask);
	if (ptask->task_ID != TASK_CLEAR_TMP_FILES) start_process(title, cmd_line);
	else ClearTmpFiles(ptask->cmd_line);
	if (pReconGUI != NULL) pReconGUI->PostMessage(WM_PROCESSING_THREAD, WM_TASK_DONE, (LPARAM)ptask);
	delete[] cmd_line;
	status = ready;
}
void CProcessingThread::OnTaskReady(UINT, LONG)
{
	if (status != ready) return;
	if (pReconGUI != NULL) pReconGUI->PostMessage(WM_PROCESSING_THREAD, WM_TASK_READY);
}

BOOL CProcessingThread::InitInstance( )
{
	status = ready;
	return true;
}

