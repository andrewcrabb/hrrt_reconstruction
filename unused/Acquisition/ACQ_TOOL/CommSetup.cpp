// CommSetup.cpp : implementation file
//

#include "stdafx.h"
#include "ScanIt.h"
#include "CommSetup.h"
#include "SerialBus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommSetup dialog
extern CSerialBus pSerBus;

CCommSetup::CCommSetup(CWnd* pParent /*=NULL*/)
	: CDialog(CCommSetup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCommSetup)
	m_Buad = -1;
	m_Data = -1;
	m_Parity = -1;
	m_Stop = -1;
	//}}AFX_DATA_INIT
}


void CCommSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommSetup)
	DDX_Control(pDX, IDC_COMBO_COM, m_Com_Port);
	DDX_Radio(pDX, IDC_RADIO_BAUD, m_Buad);
	DDX_Radio(pDX, IDC_RADIO_DATA, m_Data);
	DDX_Radio(pDX, IDC_RADIO_PARITY, m_Parity);
	DDX_Radio(pDX, IDC_RADIO_STOP, m_Stop);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCommSetup, CDialog)
	//{{AFX_MSG_MAP(CCommSetup)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommSetup message handlers

BOOL CCommSetup::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	m_Com_Port.SetCurSel(pSerBus.m_ComPort);
	m_Data = pSerBus.m_DataBit;
	m_Parity = pSerBus.m_Parity;
	m_Stop = pSerBus.m_StopBit;
	m_Buad = pSerBus.m_BaudRate;
	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCommSetup::OnOK() 
{
	int comsel;
	bool ChangeFlag = FALSE;

	UpdateData(TRUE);
	if(pSerBus.m_DataBit != m_Data)
	{
		pSerBus.m_DataBit = m_Data;
		ChangeFlag = TRUE;
	}
	if(pSerBus.m_Parity != m_Parity)
	{
		pSerBus.m_Parity = m_Parity;
		ChangeFlag = TRUE;
	}
	if(pSerBus.m_StopBit != m_Stop)
	{
		pSerBus.m_StopBit = m_Stop;
		ChangeFlag = TRUE;
	}
	if(pSerBus.m_BaudRate != m_Buad)	
	{
		pSerBus.m_BaudRate = m_Buad;
		ChangeFlag = TRUE;
	}
	
	comsel = m_Com_Port.GetCurSel();
	if (comsel != pSerBus.m_ComPort)
	{
		pSerBus.m_ComPort = comsel;
		ChangeFlag = TRUE;
	}
	if(ChangeFlag)
	{
		pSerBus.ClosePort();
		pSerBus.InitSerialBus();
	}
	pSerBus.SaveSerialFile();
	CDialog::OnOK();
}
