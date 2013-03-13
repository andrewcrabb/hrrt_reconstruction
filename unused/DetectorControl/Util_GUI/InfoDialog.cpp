//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Information Graphical User Interface
// 
// Name:			InfoDialog.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	August 14, 2003
// 
// Description:		This component is the Detector Utilities Information GUI.  it is used to
//					asynchronously display messages.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DUGUI.h"
#include "InfoDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// InfoDialog dialog


InfoDialog::InfoDialog(CWnd* pParent /*=NULL*/)
	: CDialog(InfoDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(InfoDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void InfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(InfoDialog)
	DDX_Control(pDX, IDC_STATIC_MSG, m_MsgStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(InfoDialog, CDialog)
	//{{AFX_MSG_MAP(InfoDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// InfoDialog message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		ShowMessage
// Purpose:		Displays intended text message
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void InfoDialog::ShowMessage()
{
	// put up message
	m_MsgStr.SetWindowText(Message);
}
