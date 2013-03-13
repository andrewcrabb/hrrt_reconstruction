// Atten_Process.cpp : implementation file
/*
 *	Author : Ziad Burbar
 *  Modification History:
 *  July-Oct/2004 : Merence Sibomana
 *				e7_tools integration: the 3D-attenuation is created from mu-map rather than
 *              2D-attenuation in the previous version.
 */
#include "stdafx.h"
#include "ReconGUI.h"
#include "ReconGUIDlg.h"
#include "Atten_Process.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAtten_Process dialog


CAtten_Process::CAtten_Process(CWnd* pParent /*=NULL*/)
	: CDialog(CAtten_Process::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAtten_Process)
	//}}AFX_DATA_INIT
}


void CAtten_Process::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAtten_Process)
	DDX_Control(pDX, IDC_EDIT4, m_3D_Atten);
	DDX_Control(pDX, IDC_EDIT3, m_Mu_Map);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAtten_Process, CDialog)
	//{{AFX_MSG_MAP(CAtten_Process)
	ON_BN_CLICKED(IDC_BUTTON6, OnButton6)
	ON_BN_CLICKED(IDC_BUTTON7, OnButton7)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAtten_Process message handlers

BOOL CAtten_Process::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	m_Mu_Map.SetWindowText(pDlg->m_Mu_Map_File);
	m_3D_Atten.SetWindowText(pDlg->m_3D_Atten_File);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAtten_Process::OnOK() 
{
	// TODO: Add extra validation here
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	
	bool cmd = true;
	if (m_Mu_Map.GetWindowTextLength()<=0)
		cmd = false;
	else
	{	m_Mu_Map.GetWindowText(pDlg->m_Mu_Map_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_Mu_Map_File))
			cmd = false;
	}
	if (m_3D_Atten.GetWindowTextLength()<=0)
		cmd = false;
	else
	{	m_3D_Atten.GetWindowText(pDlg->m_3D_Atten_File);
		if (!pDlg->CheckForWhiteSpace(pDlg->m_3D_Atten_File))
			cmd = false;
	}
	
	pDlg->m_Aten_OK = cmd;

	pDlg->Invalidate(TRUE);
	CDialog::OnOK();
}

// Select Mu-Map file and update the attenuation filename when applicable.
void CAtten_Process::OnButton6() 
{
	CString strFilter,strPath, strName,str;;
	int reply, len;
	CReconGUIDlg * pDlg = dynamic_cast<CReconGUIDlg*>(GetParent());	

	// Set the filter and set dialogfile 
	strFilter = "HRRT Image files (*.i)|*.i|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_Mu_Map.SetWindowText( dlg.GetPathName());
		if (pDlg->m_Auto_Assign.GetCheck() != 0)
		{
			str = dlg.GetFileName();
			len = str.GetLength();		
			strName.Format("%s.a",str.Mid(0,len-2));
			strPath = dlg.GetPathName();
			strPath.Replace(dlg.GetFileName(),NULL);

			pDlg->m_3D_Atten_File.Format("%s%s",strPath, strName);
			m_3D_Atten.SetWindowText(pDlg->m_3D_Atten_File);
//			strName.Replace(".a", ".s");
//			pDlg->m_Scatter_File.Format("%sScatter_%s",strPath, strName);
		}
	}
	UpdateData(FALSE);
}

// Get the 3D attenuation filename
void CAtten_Process::OnButton7() 
{
	CString strFilter;
	int reply;

	// Set the filter and set dialogfile 
	strFilter = "Attenuation files (*.a)|*.a|All Files (*.*)|*.*||" ;
	CFileDialog dlg(FALSE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);

	reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_3D_Atten.SetWindowText(dlg.GetPathName());
		
	}	
}
