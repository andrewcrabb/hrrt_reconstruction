//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Designate Peaks Graphical User Interface
// 
// Name:			DesignatePeaks.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	December 27, 2002
// 
// Description:		This component is the Detector Utilities Peaks Designator.
//					It displays the position profile of the selected block and allows the
//					user to select new crystal locations.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// DesignatePeaks.cpp : implementation file
//

#include "stdafx.h"
#include "DUGUI.h"
#include "CRMEdit.h"
#include "DesignatePeaks.h"

#include "DHI_Constants.h"
#include "position_profile_tools.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DesignatePeaks dialog


DesignatePeaks::DesignatePeaks(CWnd* pParent /*=NULL*/)
	: CDialog(DesignatePeaks::IDD, pParent)
{
	//{{AFX_DATA_INIT(DesignatePeaks)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void DesignatePeaks::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DesignatePeaks)
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, ID_UNDO, m_UndoButton);
	DDX_Control(pDX, IDC_COUNT, m_Count);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DesignatePeaks, CDialog)
	//{{AFX_MSG_MAP(DesignatePeaks)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(ID_UNDO, OnUndo)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DesignatePeaks message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

BOOL DesignatePeaks::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// local variables
	long i = 0;

	char Str[MAX_STR_LEN];

	WINDOWPLACEMENT Placement;
	
	// associate the dialog control
	CPaintDC dc(this);

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

	// initialize global variables
	m_Win_X = 10;
	m_Win_Y = 10;
	Curr = 0;

	// fill the image into the bitmap
	Expand();

	// show current count
	sprintf(Str, "%d of %d", Curr, (XCrystals * YCrystals));
	m_Count.SetWindowText(Str);

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, 0, 0, 538, 582, SWP_NOMOVE);

	// Positively position controls
	m_Count.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X;
	Placement.rcNormalPosition.right = m_Win_X + 80;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 15;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 35;
	m_Count.SetWindowPlacement(&Placement);

	m_UndoButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 144;
	Placement.rcNormalPosition.right = m_Win_X + 224;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_UndoButton.SetWindowPlacement(&Placement);

	m_CancelButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 288;
	Placement.rcNormalPosition.right = m_Win_X + 368;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_CancelButton.SetWindowPlacement(&Placement);

	m_OKButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Win_X + 432;
	Placement.rcNormalPosition.right = m_Win_X + 512;
	Placement.rcNormalPosition.top = m_Win_Y + 512 + 10;
	Placement.rcNormalPosition.bottom = m_Win_Y + 512 + 30;
	m_OKButton.SetWindowPlacement(&Placement);
 
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Expand
// Purpose:		expand the position profile to 512x512 in bitmap
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::Expand()
{
	long index = 0;
	long color = 0;
	long i = 0;
	long j = 0;
	long x = 0;
	long y = 0;

	// fill in the primary bitmap
	for (y = 0 ; y < PP_SIZE_Y ; y++) for (x = 0 ; x < PP_SIZE_X ; x++) {
		index = (y * PP_SIZE_X) + x;
		color = pp[index];
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
	for (i = 0 ; i < Curr ; i++) {

		// break out x/y location
		x = 2 * Pk_X[i];
		y = 2 * Pk_Y[i];

		// plot peak
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
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPaint
// Purpose:		Refresh the display
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

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

	// Do not call CDialog::OnPaint() for painting messages
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

void DesignatePeaks::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	char Str[MAX_STR_LEN];

	// if not all peaks have been designated
	if (Curr < (XCrystals * YCrystals)) {

		// if not all peaks have been designated, then transfer peak coordinates
		Pk_X[Curr] = (point.x - m_Win_X) / 2;
		Pk_Y[Curr] = (point.y - m_Win_Y) / 2;

		// increment peak number
		Curr++;

		// plot peak
		::StretchBlt(m_pk_dc,			//destination
						point.x-m_Win_X-2,
						point.y-m_Win_Y-2,
						5,
						5,
						m_red_dc,		//source
						0,
						0,
						1,
						1,
						SRCCOPY);

		// refresh dialog
		Invalidate();

		// show current count
		sprintf(Str, "%d of %d", Curr, (XCrystals * YCrystals));
		m_Count.SetWindowText(Str);
	}

	CDialog::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnCancel
// Purpose:		action when the "Cancel" button is pressed.  (Changes not accepted)
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::OnCancel() 
{
	// set public variable to indicate peak positions are not to be used
	Accept = 0;
	
	CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnOK
// Purpose:		controls the response when the dialog is exited
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::OnOK() 
{
	// if not all peaks have been designated
	if (Curr < (XCrystals * YCrystals)) {

		// query user
		if (MessageBox("Unsaved Changes.  Exit?", "All Peaks Not Designated.", MB_YESNO) == IDYES) {

			// indicate that the positions are not to be used
			Accept = 0;

			// Exit
			CDialog::OnOK();
		}

	// all peaks have been designated
	} else {

		// indicate that the positions are to be used
		Accept = 1;

		// Exit
		CDialog::OnOK();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnUndo
// Purpose:		erases last peak position deposited
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::OnUndo() 
{
	// local variables
	char Str[MAX_STR_LEN];

	// if at least one peak exists
	if (Curr > 0) {
		
		// remove
		Curr--;

		// show current count
		sprintf(Str, "%d of %d", Curr, (XCrystals * YCrystals));
		m_Count.SetWindowText(Str);

		// refresh
		Expand();

		// redisplay
		Invalidate();
	}
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL DesignatePeaks::PreTranslateMessage(MSG* pMsg) 
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
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void DesignatePeaks::OnDestroy() 
{
	// local variables
	long i = 0;

	CDialog::OnDestroy();
		
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
	
}
