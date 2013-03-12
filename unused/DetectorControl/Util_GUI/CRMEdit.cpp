//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Crystal Region Map (CRM) Editor Graphical User Interface
// 
// Name:			CRMEdit.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	December 27, 2002
// 
// Description:		This component is the Detector Utilities CRM Editor.
//					It displays the position profile of the selected block with the current
//					crystal locations overlaid on top.  On panel detector based scanners, the
//					user is able to modify the locations and crosstalk boundary.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include "stdafx.h"
#include "DUGUI.h"
#include "DUGUIDlg.h"
#include "CRMEdit.h"
#include "DesignatePeaks.h"

#include "DUCom.h"
#include "DHICom.h"

#include "position_profile_tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CErrorEventSupport* pCRMErrEvtSup;
CErrorEventSupport* pCRMErrEvtSupThread;

IDUMain *pDU = NULL;
IAbstract_DHI *pDHI = NULL;

/////////////////////////////////////////////////////////////////////////////
// CCRMEdit dialog


CCRMEdit::CCRMEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CCRMEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCRMEdit)
	//}}AFX_DATA_INIT
}


void CCRMEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCRMEdit)
	DDX_Control(pDX, ID_UNDO, m_UndoButton);
	DDX_Control(pDX, ID_DESIGNATE, m_DesignateButton);
	DDX_Control(pDX, ID_DOWNLOAD, m_DownloadButton);
	DDX_Control(pDX, ID_ORDER_BUTTON, m_OrderButton);
	DDX_Control(pDX, ID_SAVE, m_SaveButton);
	DDX_Control(pDX, IDC_SPIN_EDGE, m_SpinEdge);
	DDX_Control(pDX, IDC_SPIN_PEAKS, m_SpinPeaks);
	DDX_Control(pDX, IDC_EDGE_BUTTON, m_CrosstalkEdgeButton);
	DDX_Control(pDX, IDC_CROSSTALK_BUTTON, m_CrosstalkPeaksButton);
	DDX_Control(pDX, IDC_NUMBER_CHECK, m_NumberCheck);
	DDX_Control(pDX, IDC_EDGE_DISTANCE, m_EdgeDistance);
	DDX_Control(pDX, IDC_DISTANCE, m_Distance);
	DDX_Control(pDX, IDC_CROSSTALK_CHECK, m_CrosstalkCheck);
	DDX_Control(pDX, IDOK, m_OKButton);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCRMEdit, CDialog)
	//{{AFX_MSG_MAP(CCRMEdit)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_CROSSTALK_CHECK, OnCrosstalkCheck)
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(ID_UNDO, OnUndo)
	ON_WM_CLOSE()
	ON_BN_CLICKED(ID_SAVE, OnSave)
	ON_BN_CLICKED(IDC_CROSSTALK_BUTTON, OnCrosstalkButton)
	ON_BN_CLICKED(IDC_EDGE_BUTTON, OnEdgeButton)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_PEAKS, OnDeltaposSpinPeaks)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_EDGE, OnDeltaposSpinEdge)
	ON_BN_CLICKED(IDC_NUMBER_CHECK, OnNumberCheck)
	ON_BN_CLICKED(ID_ORDER_BUTTON, OnOrderButton)
	ON_BN_CLICKED(ID_DESIGNATE, OnDesignate)
	ON_BN_CLICKED(ID_DOWNLOAD, OnDownload)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCRMEdit message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL CCRMEdit::PreTranslateMessage(MSG* pMsg) 
{
	// capture the user clicking on the return (Enter) key to kill focus for an edit field
	// or the Escape key to prevent application termination
	if (((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN)) ||
		((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))) {

		// set focus to the main Dialog (activates the killfocus on text fields)
		SetFocus();

		// no need to do a msg translation, so quit. 
		// that way no further processing gets done  
		return TRUE; 
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
//				19 Apr 04	T Gremillion	Removed extra arguments from call to Get_Block_Crystal_Position
/////////////////////////////////////////////////////////////////////////////

BOOL CCRMEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long PositionLayer = 0;
	long Status = 0;

	long Peaks[512];
	char Str[MAX_STR_LEN];

	WINDOWPLACEMENT Placement;

	HRESULT hr = S_OK;

	char Server[MAX_STR_LEN];
	wchar_t Server_Name[MAX_STR_LEN];

	// Multiple interface pointers
	MULTI_QI mqi[1];
	IID iid;

	// server information
	COSERVERINFO si;
	ZeroMemory(&si, sizeof(si));

	// instantiate an ErrorEventSupport object
#ifndef ECAT_SCANNER
	pCRMErrEvtSup = new CErrorEventSupport(false, false);

	// if error support not establisted, note it in log
	if (pCRMErrEvtSup == NULL) {
		MessageBox("Failed to Create ErrorEventSupport");

	// if it was not fully istantiated, note it in log and release it
	} else if (pCRMErrEvtSup->InitComplete(hr) == false) {
		delete pCRMErrEvtSup;
		pCRMErrEvtSup = NULL;
	}
#endif
	// associate the dialog control
	CPaintDC dc(this);

	// global values
	m_Win_X = 10;
	m_Win_Y = 10;
	m_Verts_X = (2 * XCrystals) + 1;
	m_Verts_Y = (2 * YCrystals) + 1;
	m_Changed = 0;
	m_Current = -1;
	
	// Initialize Greyscale Color Table
	COLORREF Color;
	HBRUSH hColorBrush;
	for (i = 0 ; i < 256 ; i++)	{
		Color = RGB(i, i, i);
		CRMColor[i] = CreateCompatibleDC(dc);
		hCRMBlock[i] = CreateCompatibleBitmap(dc, 1, 1);
		SelectObject(CRMColor[i], hCRMBlock[i]);
		hColorBrush = CreateSolidBrush(Color);
		SelectObject(CRMColor[i], hColorBrush);
		PatBlt(CRMColor[i], 0, 0, 1, 1, PATCOPY);
		DeleteObject(hColorBrush);
	}

	// Initialize color red
	Color = RGB(255, 0, 0);
	m_red_dc = CreateCompatibleDC(dc);
	m_red_bm = CreateCompatibleBitmap(dc, 1, 1);
	SelectObject(m_red_dc, m_red_bm);
	hColorBrush = CreateSolidBrush(Color);
	SelectObject(m_red_dc, hColorBrush);
	PatBlt(m_red_dc, 0, 0, 1, 1, PATCOPY);
	DeleteObject(hColorBrush);

	// Initialize color blue
	Color = RGB(0, 0, 255);
	m_blue_dc = CreateCompatibleDC(dc);
	m_blue_bm = CreateCompatibleBitmap(dc, 1, 1);
	SelectObject(m_blue_dc, m_blue_bm);
	hColorBrush = CreateSolidBrush(Color);
	SelectObject(m_blue_dc, hColorBrush);
	PatBlt(m_blue_dc, 0, 0, 1, 1, PATCOPY);
	DeleteObject(hColorBrush);

	// Create a memory DC and bitmap and associate them for the position profile
	m_pp_bm = CreateCompatibleBitmap(dc, 256, 256);
	m_pp_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_pp_dc, m_pp_bm);

	// Create a memory DC and bitmap and associate them for the expanded position profile with peaks
	m_pk_bm = CreateCompatibleBitmap(dc, 512, 512);
	m_pk_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_pk_dc, m_pk_bm);

	// panel detectors used stored crystal peaks
	if (SCANNER_RING != ScannerType) {

		// Create an instance of Detector Utilities Server.
		if (getenv("DUCOM_COMPUTER") == NULL) {
			sprintf(Server, "P39ACS");
		} else {
			sprintf(Server, "%s", getenv("DUCOM_COMPUTER"));
		}
		for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
		si.pwszName = Server_Name;

		// identify interface
		iid = IID_IDUMain;
		mqi[0].pIID = &iid;
		mqi[0].pItf = NULL;
		mqi[0].hr = 0;

		// get instance
		hr = ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi);
		if (SUCCEEDED(hr)) pDU = (IDUMain*) mqi[0].pItf;
		if (FAILED(hr)) {
			Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Failed ::CoCreateInstanceEx(CLSID_DUMain, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
			MessageBox("Failed To Attach To Utilities Server", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}

		// get the crystal positions
		hr = pDU->Get_Stored_Crystal_Position(Config, Head, Layer, m_Verts, &Status);
		if (FAILED(hr)) Status = Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Method Failed (pDU->Get_Stored_Crystal_Position), HRESULT", hr);

		// if could not retrieve crystal positions, exit
		if (Status != 0) {
			MessageBox("Failed To Retrieve Crystal Locations", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}

		// retrieve CRM for block
		hr = pDU->Get_Stored_CRM(Config, Head, Layer, Block, m_CRM, &Status);
		if (FAILED(hr)) Status = Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Method Failed (pDU->Get_Stored_CRM), HRESULT", hr);

		// if could not retrieve CRM, exit
		if (Status != 0) {
			MessageBox("Failed To Retrieve CRM", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}

	// ring scanners, the positions are pulled straight from the heads
	} else {

		// Create an instance of Detector Utilities Server.
		if (getenv("DHICOM_COMPUTER") == NULL) {
			sprintf(Server, "P39ACS");
		} else {
			sprintf(Server, "%s", getenv("DHICOM_COMPUTER"));
		}
		for (i = 0 ; i < MAX_STR_LEN ; i++) Server_Name[i] = Server[i];
		si.pwszName = Server_Name;

		// identify interface
		iid = IID_IAbstract_DHI;
		mqi[0].pIID = &iid;
		mqi[0].pItf = NULL;
		mqi[0].hr = 0;

		// get instance
		hr = ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi);
		if (SUCCEEDED(hr)) pDHI = (IAbstract_DHI*) mqi[0].pItf;
		if (FAILED(hr)) {
			Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Failed ::CoCreateInstanceEx(CLSID_Abstract_DHI, NULL, CLSCTX_SERVER, &si, 1, mqi), HRESULT", hr);
			MessageBox("Failed To Attach To DAL Server", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}

		// get the crystal positions
		hr = pDHI->Get_Block_Crystal_Position(Head, Block, Peaks, &Status);
		if (FAILED(hr)) Status = Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Method Failed (pDHI->Get_Block_Crystal_Position), HRESULT", hr);

		// if could not retrieve crystal positions, exit
		if (Status != 0) {
			MessageBox("Failed To Retrieve Crystal Locations", "Error", MB_OK);
			PostMessage(WM_CLOSE);
			return TRUE;
		}

		// initialize the verts and CRM to blank
		for (i = 0  ; i < PP_SIZE ; i++) m_CRM[i] = 0;
		for (i = 0 ; i < (MAX_HEAD_VERTICES*2) ; i++) m_Verts[i] = 0;

		// transfer the peaks for the block
		for (j = 0 ; j < YCrystals ; j++) for (i = 0 ; i < XCrystals ; i++) {
			index = (Block * (2 * m_Verts_X * m_Verts_Y)) + (2 * ((2 * j) + 1) * m_Verts_X) + (2 * ((2 * i) + 1));
			m_Verts[index+0] = Peaks[(((j*XCrystals)+i)*2)+0];
			m_Verts[index+1] = Peaks[(((j*XCrystals)+i)*2)+1];
		}
	}

	// store original verticies and CRM for "Undo" operation
	for (i = 0 ; i < (MAX_HEAD_VERTICES*2) ; i++) m_OrigVerts[i] = m_Verts[i];
	for (i = 0 ; i < PP_SIZE ; i++) m_OrigCRM[i] = m_CRM[i];

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, 0, 0, 538, 610, SWP_NOMOVE);

	// Positively position controls
	m_CrosstalkCheck.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X;
	Placement.rcNormalPosition.right = m_Win_X + 60;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_CrosstalkCheck.SetWindowPlacement(&Placement);

	m_NumberCheck.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X;
	Placement.rcNormalPosition.right = m_Win_X + 60;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom  = m_Win_Y + 512 + 55;
	m_NumberCheck.SetWindowPlacement(&Placement);

	m_CrosstalkPeaksButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 70;
	Placement.rcNormalPosition.right = m_Win_X + 190;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_CrosstalkPeaksButton.SetWindowPlacement(&Placement);

	m_CrosstalkEdgeButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 70;
	Placement.rcNormalPosition.right = m_Win_X + 190;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_CrosstalkEdgeButton.SetWindowPlacement(&Placement);

	m_SpinPeaks.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 200;
	Placement.rcNormalPosition.right = m_Win_X + 230;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_SpinPeaks.SetWindowPlacement(&Placement);

	m_SpinEdge.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 200;
	Placement.rcNormalPosition.right = m_Win_X + 230;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_SpinEdge.SetWindowPlacement(&Placement);

	m_Distance.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 240;
	Placement.rcNormalPosition.right = m_Win_X + 270;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_Distance.SetWindowPlacement(&Placement);

	m_EdgeDistance.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 240;
	Placement.rcNormalPosition.right = m_Win_X + 270;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_EdgeDistance.SetWindowPlacement(&Placement);

	m_DesignateButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 270;
	Placement.rcNormalPosition.right = m_Win_X + 340;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_DesignateButton.SetWindowPlacement(&Placement);

	m_OrderButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 270;
	Placement.rcNormalPosition.right = m_Win_X + 340;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_OrderButton.SetWindowPlacement(&Placement);

	m_SaveButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 355;
	Placement.rcNormalPosition.right = m_Win_X + 425;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_SaveButton.SetWindowPlacement(&Placement);

	m_DownloadButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 355;
	Placement.rcNormalPosition.right = m_Win_X + 425;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_DownloadButton.SetWindowPlacement(&Placement);

	m_UndoButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 512 - 70;
	Placement.rcNormalPosition.right = m_Win_X + 512;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_UndoButton.SetWindowPlacement(&Placement);

	m_OKButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 512 - 70;
	Placement.rcNormalPosition.right = m_Win_X + 512;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 35;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 55;
	m_OKButton.SetWindowPlacement(&Placement);

	// if DAL in simulation mode, disable download
	if (Simulation) m_DownloadButton.EnableWindow(FALSE);

	// set CRM edit values
	m_CrosstalkCheck.SetCheck(CrossCheck);
	sprintf(Str, "%d", CrossDist);
	m_Distance.SetWindowText(Str);
	sprintf(Str, "%d", EdgeDist);
	m_EdgeDistance.SetWindowText(Str);

	// ring scanners, remove all but the "OK" button
	if (SCANNER_RING == ScannerType) {
		m_CrosstalkCheck.ShowWindow(SW_HIDE);
		m_NumberCheck.ShowWindow(SW_HIDE);
		m_CrosstalkPeaksButton.ShowWindow(SW_HIDE);
		m_CrosstalkEdgeButton.ShowWindow(SW_HIDE);
		m_SpinPeaks.ShowWindow(SW_HIDE);
		m_SpinEdge.ShowWindow(SW_HIDE);
		m_EdgeDistance.ShowWindow(SW_HIDE);
		m_Distance.ShowWindow(SW_HIDE);
		m_DesignateButton.ShowWindow(SW_HIDE);
		m_OrderButton.ShowWindow(SW_HIDE);
		m_SaveButton.ShowWindow(SW_HIDE);
		m_DownloadButton.ShowWindow(SW_HIDE);
		m_UndoButton.ShowWindow(SW_HIDE);
	}

	// Expand to 2x
	Expand();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Expand
// Purpose:		expand the position profile to 512x512 and overlay with crystal locations in bitmap
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::Expand()
{
	long index = 0;
	long oindex = 0;
	long color = 0;
	long overlap = 0;
	long i = 0;
	long io = 0;
	long j = 0;
	long jo = 0;
	long x = 0;
	long xo = 0;
	long y = 0;
	long yo = 0;

	// fill in the primary bitmap
	for (y = 0 ; y < PP_SIZE_Y ; y++) for (x = 0 ; x < PP_SIZE_X ; x++) {
		index = (y * PP_SIZE_X) + x;
		color = pp[index];
		if ((CrossCheck == 1) && (m_CRM[index] == 255)) color = 0;
		BitBlt(m_pp_dc, x, y, 1, 1, CRMColor[color], 0, 0, SRCCOPY);
	}

	// copy the image memory dc into the screen dc
	::StretchBlt(m_pk_dc,			//destination
					0,
					0,
					(2*PP_SIZE_X),
					(2*PP_SIZE_Y),
					m_pp_dc,		//source
					0,
					0,
					PP_SIZE_X,
					PP_SIZE_Y,
					SRCCOPY);

	// loop through the peak locations
	for (j = 0 ; j < YCrystals ; j++) for (i = 0 ; i < XCrystals ; i++) {

		// place in array
		index = (Block * (2 * m_Verts_X * m_Verts_Y)) + (2 * ((2 * j) + 1) * m_Verts_X) + (2 * ((2 * i) + 1));

		// break out x/y location
		x = 2 * m_Verts[index+0];
		y = 2 * m_Verts[index+1];

		for (overlap = 0, jo = 0 ; jo < YCrystals ; jo++) for (io = 0 ; io < XCrystals ; io++) {

			// place in array
			oindex = (Block * (2 * m_Verts_X * m_Verts_Y)) + (2 * ((2 * jo) + 1) * m_Verts_X) + (2 * ((2 * io) + 1));

			// ignore if original crystal
			if (oindex == index) continue;

			// break out x/y location
			xo = 2 * m_Verts[oindex+0];
			yo = 2 * m_Verts[oindex+1];

			// set flag if overlap
			if ((abs(x - xo) <= 3) && (abs(y - yo) <= 3)) overlap = 1;
		}

		// plot peak
		if (overlap == 0) {
			::StretchBlt(m_pk_dc,			//destination
							x-2,
							y-2,
							5,
							5,
							m_red_dc,		//source
							0,
							0,
							1,
							1,
							SRCCOPY);
		} else {
			::StretchBlt(m_pk_dc,			//destination
							x-2,
							y-2,
							5,
							5,
							m_blue_dc,		//source
							0,
							0,
							1,
							1,
							SRCCOPY);
		}
	}

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPaint
// Purpose:		Refresh the display
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnPaint() 
{
	long i = 0;
	long j = 0;
	long Number = 0;
	long vx = 0;
	long vy = 0;
	long index = 0;
	long x = 0;
	long y = 0;

	char Str[MAX_STR_LEN];

	// paint image
	CPaintDC dc(this);

	//copy the image memory dc into the screen dc
	::BitBlt(dc.m_hDC,			//destination
				m_Win_X,
				m_Win_Y,
				(2*PP_SIZE_X),
				(2*PP_SIZE_Y),
				m_pk_dc,		//source
				0,
				0,
				SRCCOPY);

	// if the peak numbers are turned on
	if (m_NumberCheck.GetCheck() == 1) {

		// loop through the peaks
		for (j = 0 ; j < YCrystals ; j++) for (i = 0 ; i < XCrystals ; i++) {

			// calculate number and make it into a string
			Number = (j * 16) + i;
			sprintf(Str, "%d", Number);

			// locate in array
			vx = (i * 2) + 1;
			vy = (j * 2) + 1;
			index = (2 * Block * m_Verts_X * m_Verts_Y) + (2 * vy * m_Verts_X) + (2 * vx);

			// location of peak
			x = 2 * m_Verts[index+0];
			y = 2 * m_Verts[index+1];

			// Display String
			TextOut(dc, x, y, Str, strlen(Str));
		}
	}
	
	// Do not call CDialog::OnPaint() for painting messages
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnCrosstalkCheck
// Purpose:		update the display based on the state of the crosstalk check box
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnCrosstalkCheck() 
{
	// store current state
	CrossCheck = m_CrosstalkCheck.GetCheck();

	// Re-expand data
	Expand();

	// redisplay
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// release com server (if attached)
	if (pDU != NULL) pDU->Release();
#ifndef ECAT_SCANNER
	if (pCRMErrEvtSup != NULL) delete pCRMErrEvtSup;
#endif
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnLButtonDown
// Purpose:		controls the response when the left mouse button is pressed in
//				display area
// Arguments:	nFlags	-	UINT = Indicates whether various virtual keys are down
//							(automatically generated)
//				point	-	CPoint = x/y coordinates of mouse when left button pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long dist_x = 0;
	long dist_y = 0;
	long dist = 0;
	long min_dist = 256 * 256; // not bothering to take the square root when calculating distances

	// initialize to no point selected
	m_Current = -1;

	// ring scanners, nothing allowed
	if (SCANNER_RING == ScannerType) return;

	// convert to position profile coordinates
	m_XPos = (point.x - m_Win_X) / 2;
	m_YPos = (point.y - m_Win_Y) / 2;

	// find closest peak
	for (j = 1 ; j < m_Verts_Y ; j += 2) for (i = 1 ; i < m_Verts_X ; i += 2) {

		// index to peak
		index = (2 * Block * m_Verts_Y * m_Verts_X) + (2 * j * m_Verts_X) + (2 * i);

		// distance to peak
		dist_x = m_XPos - m_Verts[index+0];
		dist_y = m_YPos - m_Verts[index+1];
		dist = (dist_x * dist_x) + (dist_y * dist_y);

		// if it is closer than previous closest, then it is new selection
		if (dist < min_dist) {
			m_Current = index;
			min_dist = dist;
		}
	}

	// offsets from closest point
	m_XOff = (point.x - m_Win_X) - (2 * m_Verts[m_Current+0]);
	m_YOff = (point.y - m_Win_Y) - (2 * m_Verts[m_Current+1]);

	// only allow close peaks to be selected
	if ((abs(m_XOff) > 10) || (abs(m_YOff) > 10)) m_Current = -1;

	// if a peak is being moved, then indicate that a change has been made
	if (m_Current != -1) m_Changed = 1;

	// update display
	Update_Display(point);
	
	CDialog::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnLButtonUp
// Purpose:		controls the response when the left mouse button is released in
//				display area
// Arguments:	nFlags	-	UINT = Indicates whether various virtual keys are down
//							(automatically generated)
//				point	-	CPoint = x/y coordinates of mouse when left button released
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// Release the current peak
	if (m_Current != -1) Let_Go();
	
	CDialog::OnLButtonUp(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnMouseMove
// Purpose:		controls the response when the mouse is moved in
//				display area
// Arguments:	nFlags	-	UINT = Indicates whether various virtual keys are down
//							(automatically generated)
//				point	-	CPoint = x/y coordinates of mouse
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnMouseMove(UINT nFlags, CPoint point) 
{
	// Update the display
	if ((nFlags & MK_LBUTTON) == MK_LBUTTON) {

		//refresh the point
		Update_Display(point);
	} else {

		// neutralize the point
		if (m_Current != -1) Let_Go();
	}
	
	CDialog::OnMouseMove(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Update_Display
// Purpose:		clears the old dot, replaces it with the original image and
//				then draws the new dot
// Arguments:	point	-	CPoint = x/y coordinates
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::Update_Display(CPoint point)
{
	// local variables
	long x;
	long y;

	// point must be selected
	if (m_Current != -1) {

		// valid position
		x = point.x - m_XOff - m_Win_X;
		y = point.y - m_YOff - m_Win_Y;
		if ((x >= 2) && (x < 510) && (y >= 2) && (y < 510)) {

			//create device context
			CClientDC dc(this);

			//copy the image memory dc into the screen dc
			::BitBlt(dc.m_hDC,			//destination
						m_Win_X+m_XPos-2,
						m_Win_Y+m_YPos-2,
						5,
						5,
						m_pk_dc,		//source
						m_XPos-2,
						m_YPos-2,
						SRCCOPY);

			//transfer coordinates
			m_XPos = x;
			m_YPos = y;

			//copy the blue box memory dc to the screen dc
			::StretchBlt(dc.m_hDC,	//destination
						m_Win_X+m_XPos-2,
						m_Win_Y+m_YPos-2,
						5,
						5,
						m_blue_dc,		//source
						0,
						0,
						1,
						1,
						SRCCOPY);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Let_Go
// Purpose:		controls the response when the left mouse button is released
//				while moving a peak position
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::Let_Go()
{
	// local variables
	long index = 0;
	long vx = 0;
	long vy = 0;

	// update peak position
	m_Verts[m_Current+0] = m_XPos / 2;
	m_Verts[m_Current+1] = m_YPos / 2;

	// adjust vertex to the left (if not on left edge)
	index = (m_Current / 2) % (m_Verts_X * m_Verts_Y);
	vx = index % m_Verts_X;
	vy = index / m_Verts_X;

	// if not on left edge, adjust vertex to left
	if (vx != 1) {
		m_Verts[m_Current-2] = (m_Verts[m_Current+0] + m_Verts[m_Current-4]) / 2;
		m_Verts[m_Current-1] = (m_Verts[m_Current+1] + m_Verts[m_Current-3]) / 2;
	}

	// if not on right edge, adjust vertex to right
	if (vx != (m_Verts_X-2)) {
		m_Verts[m_Current+2] = (m_Verts[m_Current+0] + m_Verts[m_Current+4]) / 2;
		m_Verts[m_Current+3] = (m_Verts[m_Current+1] + m_Verts[m_Current+5]) / 2;
	}

	// if not on bottom edge, adjust vertex to bottom
	if (vy != 1) {
		m_Verts[m_Current-(2*m_Verts_X)+0] = (m_Verts[m_Current+0] + m_Verts[m_Current-(4*m_Verts_X)+0]) / 2;
		m_Verts[m_Current-(2*m_Verts_X)+1] = (m_Verts[m_Current+1] + m_Verts[m_Current-(4*m_Verts_X)+1]) / 2;
	}

	// if not on top edge, adjust vertex to top
	if (vy != (m_Verts_Y-2)) {
		m_Verts[m_Current+(2*m_Verts_X)+0] = (m_Verts[m_Current+0] + m_Verts[m_Current+(4*m_Verts_X)+0]) / 2;
		m_Verts[m_Current+(2*m_Verts_X)+1] = (m_Verts[m_Current+1] + m_Verts[m_Current+(4*m_Verts_X)+1]) / 2;
	}

	// if not on left edge and not on bottom edge, adjust vertex to the bottom left
	if ((vx != 1) && (vy != 1)) {
		m_Verts[m_Current-(2*m_Verts_X)-2] = (long) (((double)(m_Verts[m_Current+0] + m_Verts[m_Current-(4*m_Verts_X)+0] + 
															   m_Verts[m_Current-4] + m_Verts[m_Current-(4*m_Verts_X)-4]) / 4.0) + 0.5);
		m_Verts[m_Current-(2*m_Verts_X)-1] = (long) (((double)(m_Verts[m_Current+1] + m_Verts[m_Current-(4*m_Verts_X)+1] + 
															   m_Verts[m_Current-3] + m_Verts[m_Current-(4*m_Verts_X)-3]) / 4.0) + 0.5);
	}

	// if not on left edge and not on top edge, adjust vertex to the top left
	if ((vx != 1) && (vy != (m_Verts_Y-2))) {
		m_Verts[m_Current+(2*m_Verts_X)-2] = (long) (((double)(m_Verts[m_Current+0] + m_Verts[m_Current+(4*m_Verts_X)+0] + 
															   m_Verts[m_Current-4] + m_Verts[m_Current+(4*m_Verts_X)-4]) / 4.0) + 0.5);
		m_Verts[m_Current+(2*m_Verts_X)-1] = (long) (((double)(m_Verts[m_Current+1] + m_Verts[m_Current+(4*m_Verts_X)+1] + 
															   m_Verts[m_Current-3] + m_Verts[m_Current+(4*m_Verts_X)-3]) / 4.0) + 0.5);
	}

	// if not on right edge and not on bottom edge, adjust vertex to the bottom right
	if ((vx != (m_Verts_X-2)) && (vy != 1)) {
		m_Verts[m_Current-(2*m_Verts_X)+2] = (long) (((double)(m_Verts[m_Current+0] + m_Verts[m_Current-(4*m_Verts_X)+0] + 
															   m_Verts[m_Current+4] + m_Verts[m_Current-(4*m_Verts_X)+4]) / 4.0) + 0.5);
		m_Verts[m_Current-(2*m_Verts_X)+3] = (long) (((double)(m_Verts[m_Current+1] + m_Verts[m_Current-(4*m_Verts_X)+1] + 
															   m_Verts[m_Current+5] + m_Verts[m_Current-(4*m_Verts_X)+5]) / 4.0) + 0.5);
	}

	// if not on right edge and not on top edge, adjust vertex to the top right
	if ((vx != (m_Verts_X-2)) && (vy != (m_Verts_Y-2))) {
		m_Verts[m_Current+(2*m_Verts_X)+2] = (long) (((double)(m_Verts[m_Current+0] + m_Verts[m_Current+(4*m_Verts_X)+0] + 
															   m_Verts[m_Current+4] + m_Verts[m_Current+(4*m_Verts_X)+4]) / 4.0) + 0.5);
		m_Verts[m_Current+(2*m_Verts_X)+3] = (long) (((double)(m_Verts[m_Current+1] + m_Verts[m_Current+(4*m_Verts_X)+1] + 
															   m_Verts[m_Current+5] + m_Verts[m_Current+(4*m_Verts_X)+5]) / 4.0) + 0.5);
	}

	// de-select
	m_Current = -1;

	// update red peaks
	Expand();

	// refresh display
	Invalidate();

}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnUndo
// Purpose:		restores to state at last save (or entry if no save)
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnUndo() 
{
	// local variables
	long i = 0;

	// revert verticies
	for (i = 0 ; i < (MAX_HEAD_VERTICES*2) ; i++) m_Verts[i] = m_OrigVerts[i];

	// revert CRM
	for (i = 0 ; i < PP_SIZE ; i++) m_CRM[i] = m_OrigCRM[i];

	// mark as unchanged
	m_Changed = 0;

	// update bitmap
	Expand();

	// update dialog
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnClose
// Purpose:		controls the response when the dialog is closed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnClose() 
{
	// if nothing changed, close out window	
	OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnOK
// Purpose:		controls the response when the dialog is exited
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnOK() 
{
	// local variables
	long i = 0;

	// if something has been changed, confirm the user wants to close the dialog
	if (m_Changed == 1) {

		// Request Confirmation
		if (MessageBox("Unsaved Changes.  Exit?", "Exit Confirmation", MB_YESNO) == IDNO) return;

	// if nothing changed, close out window	
	}

	// release bitmaps
	::DeleteObject(m_pp_bm);
	::DeleteObject(m_pk_bm);
	::DeleteObject(m_red_bm);
	::DeleteObject(m_blue_bm);
	for (i = 0 ; i < 256 ; i++) ::DeleteObject(hCRMBlock[i]);

	// release device contexts
	DeleteDC(m_pp_dc);
	DeleteDC(m_pk_dc);
	DeleteDC(m_red_dc);
	DeleteDC(m_blue_dc);
	for (i = 0 ; i < 256 ; i++) DeleteDC(CRMColor[i]);

	// execute the base class function
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSave
// Purpose:		controls the response when the current CRM is saved 
//				("Save" button pressed)
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnSave() 
{
	// local variables
	long i = 0;
	long index = 0;
	long Status = 0;

	HRESULT hr = S_OK;

	// Build the CRM
	index = 2 * Block * m_Verts_X * m_Verts_Y;
	Build_CRM(&m_Verts[index], XCrystals, YCrystals, m_CRM);

	// store the CRM
	hr = pDU->Store_CRM(Config, Head, Layer, Block, m_CRM, &Status);
	if (FAILED(hr)) Status = Add_Error(pCRMErrEvtSup, "OnSave", "Method Failed (pDU->Store_CRM), HRESULT", hr);

	// if stored successfully
	if (Status == 0) {

		// store the peak positions
		hr = pDU->Store_Crystal_Position(Config, Head, Layer, m_Verts, &Status);
		if (FAILED(hr)) Status = Add_Error(pCRMErrEvtSup, "OnSave", "Method Failed (pDU->Store_Crystal_Position), HRESULT", hr);

		// if stored successfully
		if (Status == 0) {

			// make new verticies the undo verticies
			for (i = 0 ; i < (m_Verts_X * m_Verts_Y) ; i++) {
				m_OrigVerts[(i*2)+0] = m_Verts[(i*2)+0];
				m_OrigVerts[(i*2)+1] = m_Verts[(i*2)+1];
			}

			// make new CRM the undo CRM
			for (i = 0 ; i < PP_SIZE ; i++) m_OrigCRM[i] = m_CRM[i];

			// note as unchanged
			m_Changed = 0;

			// note that it was saved
			State = 1;

		// otherwise, inform user it was not stored
		} else {
			MessageBox("Crystal Positions Not Stored", "Error", MB_OK);
		}
	// otherwise, inform user it was not stored
	} else {
		MessageBox("CRM Not Stored", "Error", MB_OK);
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnCrosstalkButton
// Purpose:		controls the response when the "Crosstalk From Peaks" button is pressed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnCrosstalkButton() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;

	// Fill in the CRM Vertices
	index = 2 * Block * m_Verts_X * m_Verts_Y;
	Fill_CRM_Verticies(CrossDist, XCrystals, YCrystals, &m_Verts[index]);

	// Build the CRM
	Build_CRM(&m_Verts[index], XCrystals, YCrystals, m_CRM);

	// mark as changed
	m_Changed = 1;

	// update bitmap
	Expand();

	// update dialog
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnEdgeButton
// Purpose:		controls the response when the "Crosstalk From Edge" button is pressed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnEdgeButton() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long Changed = 0;

	// left edge
	if ((Block % XBlocks) == 0) {
		Changed = 1;
		for (i = 0, j = 0 ; j < m_Verts_Y ; j++) {
			index = (2 * Block * m_Verts_X * m_Verts_Y) + (2 * j * m_Verts_X) + (2 * i);
			m_Verts[index+0] = EdgeDist;
		}
	}

	// right edge
	if ((Block % XBlocks) == (XBlocks-1)) {
		Changed = 1;
		for (i = (m_Verts_X-1), j = 0 ; j < m_Verts_Y ; j++) {
			index = (2 * Block * m_Verts_X * m_Verts_Y) + (2 * j * m_Verts_X) + (2 * i);
			m_Verts[index+0] = PP_SIZE_X - EdgeDist;
		}
	}

	// bottom edge
	if ((Block / XBlocks) == 0) {
		Changed = 1;
		for (j = 0, i = 0 ; i < m_Verts_X ; i++) {
			index = (2 * Block * m_Verts_X * m_Verts_Y) + (2 * j * m_Verts_X) + (2 * i);
			m_Verts[index+1] = EdgeDist;
		}
	}

	// top edge
	if ((Block / XBlocks) == (YBlocks-1)) {
		Changed = 1;
		for (j = (m_Verts_Y-1), i = 0 ; i < m_Verts_X ; i++) {
			index = (2 * Block * m_Verts_X * m_Verts_Y) + (2 * j * m_Verts_X) + (2 * i);
			m_Verts[index+1] = PP_SIZE_Y - EdgeDist;
		}
	}

	// Build the CRM if changed
	if (Changed) {

		// build
		index = 2 * Block * m_Verts_X * m_Verts_Y;
		Build_CRM(&m_Verts[index], XCrystals, YCrystals, m_CRM);

		// mark as changed
		m_Changed = 1;

		// update bitmap
		Expand();

		// update dialog
		Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposSpinPeaks
// Purpose:		controls the response when the "Crosstalk From Peaks" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnDeltaposSpinPeaks(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	// change value and then limit it
	CrossDist -= pNMUpDown->iDelta;
	if (CrossDist < 0) CrossDist = 0;
	if (CrossDist > 20) CrossDist = 20;

	// display the new number
	sprintf(Str, "%d", CrossDist);
	m_Distance.SetWindowText(Str);

	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposSpinEdge
// Purpose:		controls the response when the "Crosstalk From Edge" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnDeltaposSpinEdge(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here

	// change value and then limit it
	EdgeDist -= pNMUpDown->iDelta;
	if (EdgeDist < 2) EdgeDist = 2;
	if (EdgeDist > 30) EdgeDist = 30;

	// display the new number
	sprintf(Str, "%d", EdgeDist);
	m_EdgeDistance.SetWindowText(Str);

	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnNumberCheck
// Purpose:		updates the display when the "Numbers" checkbox is modified
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnNumberCheck() 
{
	// Refresh the dialog box
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnOrderButton
// Purpose:		updates the numbering of the peaks when the "Order" button is pressed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnOrderButton() 
{
	// local variables
	long i = 0;
	long j = 0;
	long k = 0;
	long Num = 0;
	long index = 0;
	long Curr = 0;
	long Next = 0;
	long dx = 0;
	long dy = 0;
	long sx = 0;
	long sy = 0;

	double Dist = 0.0;
	double MinDist = 0.0;
	double rad45 = atan(1.0);
	double angle = 0.0;

	long Pk_X[MAX_CRYSTALS];
	long Pk_Y[MAX_CRYSTALS];
	long Pk_Num[MAX_CRYSTALS];

	char Str[MAX_STR_LEN];

	// initialize all peak locations as invalid
	for (i = 0 ; i < MAX_CRYSTALS ; i++) Pk_Num[i] = -2;

	// transfer peak positions to temporary variables and set number to unresolved
	for (j = 0 ; j < YCrystals ; j++) for (i = 0 ; i < XCrystals ; i++) {
		Num = (j * 16) + i;
		index = (2 * Block * m_Verts_Y * m_Verts_X) + (2 * ((2 * j) + 1) * m_Verts_X) + (2 * ((2 * i) + 1));
		Pk_X[Num] = m_Verts[index+0];
		Pk_Y[Num] = m_Verts[index+1];
		Pk_Num[Num] = -1;
	}

	// loop through all rows of peaks
	for (j = 0 ; j < YCrystals ; j++) {

		// find minimum values for x and y for all remaining valid peaks
		for (sx = PP_SIZE_X, sy = PP_SIZE_Y, i = 0 ; i < MAX_CRYSTALS ; i++) {

			// peak must be available and valid
			if (Pk_Num[i] != -1) continue;

			// if closer
			if (Pk_X[i] < sx) sx = Pk_X[i];
			if (Pk_Y[i] < sy) sy = Pk_Y[i];
		}

		// loop through peak locations
		for (MinDist = PP_SIZE, Curr = -1, i = 0 ; i < MAX_CRYSTALS ; i++) {

			// if peak is not valid or not available, next peak
			if (Pk_Num[i] != -1) continue;

			// calculate distances
			dx = Pk_X[i] - sx;
			dy = Pk_Y[i] - sy;

			// get distance from origin
			Dist = sqrt((double) ((dx * dx) + (dy * dy)));

			// if it is closer than the previous closest
			if (Dist < MinDist) {
				Curr = i;
				MinDist = Dist;
			}
		}

		// first peak in row was found
		if (Curr != -1) {

			// assign peak number
			Pk_Num[Curr] = (j * 16) + 0;

			// find the rest of the row
			for (i = 1 ; i < XCrystals ; i++) {

				// find the closest acceptable peak
				for (MinDist = PP_SIZE, Next = -1, k = 0 ; k < MAX_CRYSTALS ; k++) {

					// if peak is not valid or not available, next peak
					if (Pk_Num[k] != -1) continue;

					// calculate distance
					dx = Pk_X[k] - Pk_X[Curr];
					dy = Pk_Y[k] - Pk_Y[Curr];

					// if not more than one pixel to the right of the previous point, next peak
					if (dx < 1) continue;

					// if greater than 45 degrees, next peak
					angle = fabs(atan((double) dy / (double) dx));
					if (angle > rad45) continue;

					// distance from previous peak (weighted towards horizontal)
					Dist = sqrt((double)((dx * dx) + (dy * dy))) * (1.0 + angle);

					// if it is closer than previous closest
					if (Dist < MinDist) {
						Next = k;
						MinDist = Dist;
					}
				}

				// if the next peak was not found, use the next closest to origin
				if (Next == -1) {

					// find the closest acceptable peak
					for (k = 0 ; k < MAX_CRYSTALS ; k++) {

						// if peak is not valid or not available, next peak
						if (Pk_Num[k] != -1) continue;

						// calculate angle off horizontal
						dx = Pk_X[k] - sx;
						dy = Pk_Y[k] - sy;

						// distance from previous peak
						Dist = sqrt((double) ((dx * dx) + (dy * dy)));

						// if it is closer than previous closest
						if (Dist < MinDist) {
							Next = k;
							MinDist = Dist;
						}
					}
				}

				// add peak information
				if (Next != -1) {

					// assign peak number
					Pk_Num[Next] = (j * 16) + i;

					// prepare for next peak
					Curr = Next;

				// still not found, then something is wrong, inform user
				} else {
					sprintf(Str, "Row: %d Column: %d - Could Not Find Peak", j, i);
					MessageBox(Str, "Error", MB_OK);
					return;
				}
			}

		// a peak was not found, then something is wrong, inform user
		} else {
			sprintf(Str, "Row: %d Column: 0 - Could Not Find Peak", j);
			MessageBox(Str, "Error", MB_OK);
			return;
		}
	}

	// transfer peak positions to temporary variables and set number to unresolved
	for (k = 0 ; k < MAX_CRYSTALS ; k++) {
		if (Pk_Num[k] >= 0) {
			i = Pk_Num[k] % 16;
			j = Pk_Num[k] / 16;
			index = (2 * Block * m_Verts_Y * m_Verts_X) + (2 * ((2 * j) + 1) * m_Verts_X) + (2 * ((2 * i) + 1));
			m_Verts[index+0] = Pk_X[k];
			m_Verts[index+1] = Pk_Y[k];
		}
	}

	// fill in all mid verticies
	index = 2 * Block * m_Verts_Y * m_Verts_X;
	Fill_CRM_Verticies(CrossDist, XCrystals, YCrystals, &m_Verts[index]);

	// Build the CRM
	Build_CRM(&m_Verts[index], XCrystals, YCrystals, m_CRM);

	// mark as changed
	m_Changed = 1;

	// update bitmap
	Expand();

	// update dialog
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDesignate
// Purpose:		brings up the Designate Peaks dialog (DesignatePeaks.cpp) allowing
//				the user to select new peak positions
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnDesignate() 
{
	// local variables
	long i = 0;
	long j = 0 ;
	long index = 0;

	DesignatePeaks *pPickDlg;

	// allocate dialog
	pPickDlg = new DesignatePeaks;

	// Transfer Values
	pPickDlg->XCrystals = XCrystals;
	pPickDlg->YCrystals = YCrystals;
	pPickDlg->Accept = 0;

	// Transfer Array
	for (i = 0 ; i < PP_SIZE ; i++) pPickDlg->pp[i] = pp[i];

	// put up dialog
	pPickDlg->DoModal();

	// if the new peaks are to be accepted, apply them
	if (pPickDlg->Accept == 1) {

		// transfer the peak positions
		for (j = 0 ; j < YCrystals ; j++) for (i = 0 ; i < XCrystals ; i++) {
			index = (2 * Block * m_Verts_Y * m_Verts_X) + (2 * ((2 * j) + 1) * m_Verts_X) + (2 * ((2 * i) + 1));
			m_Verts[index+0] = pPickDlg->Pk_X[(j*XCrystals)+i];
			m_Verts[index+1] = pPickDlg->Pk_Y[(j*XCrystals)+i];
		}

		// fill in all mid verticies
		index = 2 * Block * m_Verts_Y * m_Verts_X;
		Fill_CRM_Verticies(CrossDist, XCrystals, YCrystals, &m_Verts[index]);

		// Build the CRM
		Build_CRM(&m_Verts[index], XCrystals, YCrystals, m_CRM);

		// mark as changed
		m_Changed = 1;

		// update bitmap
		Expand();

		// update dialog
		Invalidate();
	}

	// release dialog
	delete pPickDlg;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDownload
// Purpose:		causes CRMs for current block to be downloaded when "Download"
//				button is pressed
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCRMEdit::OnDownload() 
{
	// local variables
	long i = 0;
	long index = 0;

	HRESULT hr = S_OK;

	long BlockSelect[MAX_TOTAL_BLOCKS];
	long BlockStatus[MAX_TOTAL_BLOCKS];

	// get confirmation
	if (MessageBox("Download Current Block?", "Confirmation", MB_OKCANCEL) == IDOK) {

		// clear selection and status
		for (i = 0 ; i < MAX_TOTAL_BLOCKS ; i++) {
			BlockSelect[i] = 0;
			BlockStatus[i] = 0;
		}

		// Select Block
		index = (Head * MAX_BLOCKS) + Block;
		BlockSelect[index] = 1;

		// download
		hr = pDU->Download_CRM(Config, BlockSelect, BlockStatus);
		if (FAILED(hr)) BlockStatus[index] = Add_Error(pCRMErrEvtSup, "CRMEdit:OnInitDialog", "Method Failed (pDU->Store_CRM), HRESULT", hr);

		// check for proper function
		if (BlockStatus[index] != 0) {
			MessageBox("CRM Not Downloaded", "Error", MB_OK);
		} else {
			State = 0;
		}
	}
}
