//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Crystal Energy Peaks Editor Graphical User Interface
// 
// Name:			CrystalPeaks.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	December 31, 2002
// 
// Description:		This component is the Detector Utilities Crystal Peaks Editor.
//					It displays the crystal energy of the selected block with the current
//					energy peaks overlaid on top as well as a display of all energy peaks
//					for the blocks and a plot for the current crystal.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DUGUI.h"
#include "CrystalPeaks.h"
#include <math.h>

#include "DHI_Constants.h"
#include "position_profile_tools.h"
#include "find_peak_1d.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCrystalPeaks dialog


CCrystalPeaks::CCrystalPeaks(CWnd* pParent /*=NULL*/)
	: CDialog(CCrystalPeaks::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCrystalPeaks)
	//}}AFX_DATA_INIT
}


void CCrystalPeaks::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCrystalPeaks)
	DDX_Control(pDX, IDC_LLD_SPIN, m_LLD_Spin);
	DDX_Control(pDX, IDC_LLD_TEXT, m_LLD_Text);
	DDX_Control(pDX, IDC_ULD_SPIN, m_ULD_Spin);
	DDX_Control(pDX, IDC_ULD_TEXT, m_ULD_Text);
	DDX_Control(pDX, IDRESET, m_ResetButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDC_SMOOTH_SPIN, m_SmoothSpin);
	DDX_Control(pDX, IDC_PREVIOUS, m_PrevButton);
	DDX_Control(pDX, IDC_NEXT, m_NextButton);
	DDX_Control(pDX, IDC_SMOOTH_TEXT, m_SmoothText);
	DDX_Control(pDX, IDC_CRYSTAL_TEXT, m_CrystalText);
	DDX_Control(pDX, IDC_PEAK_TEXT, m_PeakText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCrystalPeaks, CDialog)
	//{{AFX_MSG_MAP(CCrystalPeaks)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SMOOTH_SPIN, OnDeltaposSmoothSpin)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDRESET, OnReset)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
	ON_NOTIFY(UDN_DELTAPOS, IDC_ULD_SPIN, OnDeltaposUldSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_LLD_SPIN, OnDeltaposLldSpin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrystalPeaks message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposSmoothSpin
// Purpose:		controls the response when the "smoothing" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				16 Dec 03	T Gremillion	"Smoothing Passes" changed to "Smoothing"
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnDeltaposSmoothSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	// change value and then limit it
	CrystalSmooth -= pNMUpDown->iDelta;
	if (CrystalSmooth < 0) CrystalSmooth = 0;
	if (CrystalSmooth > 20) CrystalSmooth = 20;

	// display the new number
	sprintf(Str, "Smoothing: %d", CrystalSmooth);
	m_SmoothText.SetWindowText(Str);

	// replot
	Invalidate();
	
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPrevious
// Purpose:		Actions taken when the "Prevoius" crystal button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnPrevious() 
{
	// local variables
	long x = 0;
	long y = 0;
	long Num = 0;

	// Convert Crystal Number to Real number
	x = CrystalNum % 16;
	y = CrystalNum / 16;
	Num = (y * XCrystals) + x;

	// decrement
	if (Num > 0) Num--;

	// Convert back
	x = Num % XCrystals;
	y = Num / XCrystals;
	CrystalNum = (y * 16) + x;

	// display new plot
	Refresh();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnNext
// Purpose:		Actions taken when the "Next" crystal button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnNext() 
{
	// local variables
	long x = 0;
	long y = 0;
	long Num = 0;

	// Convert Crystal Number to Real number
	x = CrystalNum % 16;
	y = CrystalNum / 16;
	Num = (y * XCrystals) + x;

	// decrement
	if (Num < ((XCrystals * YCrystals) - 1)) Num++;

	// Convert back
	x = Num % XCrystals;
	y = Num / XCrystals;
	CrystalNum = (y * 16) + x;

	// display new plot
	Refresh();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnReset
// Purpose:		Actions taken when the "Reset" button is pressed.  values returned
//				to entry state
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnReset() 
{
	// local variables
	long i = 0;

	// transfer original peaks back
	for (i = 0 ; i < MAX_CRYSTALS ; i++) Peak[i] = m_OrigPeak[i];

	// Refresh the display
	Refresh();

	// changes have been cleared
	State = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
//				16 Dec 03	T Gremillion	added positioning of lld and uld values and spin controls
//				21 Jan 04	T Gremillion	hide reset button for ring scanners (PR 3523)
/////////////////////////////////////////////////////////////////////////////

BOOL CCrystalPeaks::OnInitDialog() 
{
	// local variables
	long i = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long DataScaling = 0;	// default scaling is historam equalized

	long BlockImg[PP_SIZE];

	WINDOWPLACEMENT Placement;

	CDialog::OnInitDialog();

	// associate the dialog control
	CPaintDC dc(this);
	
	// Initialize Greyscale Color Table
	COLORREF Color;
	HBRUSH hColorBrush;
	for (i = 0 ; i < 256 ; i++)	{
		Color = RGB(i, i, i);
		GreyColor[i] = CreateCompatibleDC(dc);
		hGreyMap[i] = CreateCompatibleBitmap(dc, 1, 1);
		SelectObject(GreyColor[i], hGreyMap[i]);
		hColorBrush = CreateSolidBrush(Color);
		SelectObject(GreyColor[i], hColorBrush);
		PatBlt(GreyColor[i], 0, 0, 1, 1, PATCOPY);
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

	// Create a memory DC and bitmap and associate them for the crystal energy
	m_en_bm = CreateCompatibleBitmap(dc, 256, 256);
	m_en_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_en_dc, m_en_bm);

	// Create a memory DC and bitmap and associate them for the crystal energy
	m_pk_bm = CreateCompatibleBitmap(dc, 256, 256);
	m_pk_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_pk_dc, m_pk_bm);

	// store incoming peaks for possible "Reset"
	for (i = 0 ; i < MAX_CRYSTALS ; i++) m_OrigPeak[i] = Peak[i];

	// define areas
	m_Pk_X = 10;
	m_Pk_Y = 10;
	m_En_X = 275;
	m_En_Y = m_Pk_Y;
	m_Plot_X = m_Pk_X;
	m_Plot_Y = 275;

	// ring scanners use linear scaling
	if (SCANNER_RING == ScannerType) DataScaling = 1;

	// histogram equalize the data
	hist_equal(DataScaling, PP_SIZE, Data, BlockImg);

	// fill in crystal energy bitmap
	for (y = 0 ; y < 256 ; y++) {
		for (x = 0 ; x < 256 ; x++) {

			// index into data
			index = (y * PP_SIZE_X) + x;

			// fill in bitmap
			BitBlt(m_en_dc, x, y, 1, 1, GreyColor[BlockImg[index]], 0, 0, SRCCOPY);
		}
	}

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, 0, 0, 550, 480, SWP_NOMOVE);

	// Positively position controls
	m_PrevButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 65;
	Placement.rcNormalPosition.top = m_Plot_Y;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 20;
	m_PrevButton.SetWindowPlacement(&Placement);

	m_NextButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 70;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 20;
	m_NextButton.SetWindowPlacement(&Placement);

	m_LLD_Text.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 3;
	Placement.rcNormalPosition.right = m_En_X + 127 + 48;
	Placement.rcNormalPosition.top = m_Plot_Y + 30;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 45;
	m_LLD_Text.SetWindowPlacement(&Placement);

	m_LLD_Spin.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 50;
	Placement.rcNormalPosition.right = m_En_X + 127 + 65;
	Placement.rcNormalPosition.top = m_Plot_Y + 25;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 45;
	m_LLD_Spin.SetWindowPlacement(&Placement);

	m_ULD_Text.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 68;
	Placement.rcNormalPosition.right = m_En_X + 127 + 113;
	Placement.rcNormalPosition.top = m_Plot_Y + 30;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 45;
	m_ULD_Text.SetWindowPlacement(&Placement);

	m_ULD_Spin.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 115;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 25;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 45;
	m_ULD_Spin.SetWindowPlacement(&Placement);

	m_CrystalText.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 50;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 70;
	m_CrystalText.SetWindowPlacement(&Placement);

	m_PeakText.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 70;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 90;
	m_PeakText.SetWindowPlacement(&Placement);

	m_SmoothText.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 110;
	Placement.rcNormalPosition.top = m_Plot_Y + 95;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 110;
	m_SmoothText.SetWindowPlacement(&Placement);

	m_SmoothSpin.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 115;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 90;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 110;
	m_SmoothSpin.SetWindowPlacement(&Placement);

	m_ResetButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 115;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 135;
	m_ResetButton.SetWindowPlacement(&Placement);
	if (SCANNER_RING == ScannerType) m_ResetButton.ShowWindow(SW_HIDE);

	m_OKButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_En_X + 127 + 5;
	Placement.rcNormalPosition.right = m_En_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 140;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 160;
	m_OKButton.SetWindowPlacement(&Placement);

	// Refresh Display
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Refresh
// Purpose:		update fields and peaks bitmap
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				16 Dec 03	T Gremillion	"Smoothing Passes" changed to "Smoothing"
//											lld and uld fields added
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::Refresh()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long color = 0;

	char Str[MAX_STR_LEN];
	
	// fill in values
	sprintf(Str, "Crystal Number: %d", CrystalNum);
	m_CrystalText.SetWindowText(Str);
	sprintf(Str, "Crystal Peak: %d", Peak[CrystalNum]);
	m_PeakText.SetWindowText(Str);
	sprintf(Str, "Smoothing: %d", CrystalSmooth);
	m_SmoothText.SetWindowText(Str);
	sprintf(Str, "LLD %d", ScannerLLD);
	m_LLD_Text.SetWindowText(Str);
	sprintf(Str, "ULD %d", ScannerULD);
	m_ULD_Text.SetWindowText(Str);

	// crystal peaks bitmap
	for (y = 0 ; y < 256 ; y++) {
		for (x = 0 ; x < 256 ; x++) {

				// determine crystal
				i = (x * XCrystals) / 256;
				j = (y * YCrystals) / 256;
				index = (j * 16) + i;

				// retrieve value
				color = Peak[index];
				BitBlt(m_pk_dc, x, y, 1, 1, GreyColor[color], 0, 0, SRCCOPY);
		}
	}

	// refresh display
	Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnPaint
// Purpose:		Refresh the display
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				21 Nov 03	T Gremillion	Max Value for scaling not allowed below 8 for scaling
//				16 Dec 03	T Gremillion	added plotting of lld and uld
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnPaint() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x1 = 0;
	long x2 = 0;
	long y1 = 0;
	long y2 = 0;
	long MaxValue = 0;
	long Power = 0;
	long YRng = 0;

	double Step = 0;
	double TickBase = 0.0;
	double Scale_X = 0.0;
	double Scale_Y = 0.0;
	double Line[256];

	long Line_X[4] = {1, 2, 1, 0};
	long Line_Y[4] = {-1, 0, 1, 0};

	char Str[MAX_STR_LEN];

	CPaintDC dc(this); // device context for painting

	// define a pens for drawing
	CPen   RedPen(PS_SOLID, 1, RGB(255,   0,   0));
	CPen GreenPen(PS_SOLID, 1, RGB(  0, 255,   0));
	CPen WhitePen(PS_SOLID, 1, RGB(255, 255, 255));
	CPen BlackPen(PS_SOLID, 1, RGB(  0,   0,   0));

	// copy peaks bitmap to dialog
	::StretchBlt(dc.m_hDC,			//destination
					m_Pk_X,
					m_Pk_Y,
					256,
					256,
					m_pk_dc,		//source
					0,
					0,
					256,
					256,
					SRCCOPY);

	// copy crystal energy bitmap to dialog
	::StretchBlt(dc.m_hDC,			//destination
					m_En_X,
					m_En_Y,
					256,
					256,
					m_en_dc,		//source
					0,
					0,
					256,
					256,
					SRCCOPY);

	// clear plot area
	::StretchBlt(dc.m_hDC,			//destination
					m_Plot_X,
					m_Plot_Y,
					390,
					170,
					GreyColor[0],	//source (black)
					0,
					0,
					1,
					1,
					SRCCOPY);

	// select the red pen
	dc.SelectObject(&RedPen);

	// loop through the crystals marking crystal energy display
	for (i = 0 ; i < MAX_CRYSTALS ; i++) {

		// if the energy peak is non-zero
		if (Peak[i] |= 0) {

			// starting point
			x1 = m_En_X + Peak[i] - 1;
			y1 = m_En_Y + i;
			dc.MoveTo(x1, y1);

			// draw rest of symbol
			for (j = 0 ; j < 4 ; j++) {
				x2 = x1 + Line_X[j];
				y2 = y1 + Line_Y[j];
				dc.LineTo(x2, y2);
			}
		}
	}

	// plot grid on offset display
	dc.SelectObject(&BlackPen);

	// horizontal
	Step = 256.0  / YCrystals;
	x1 = m_Pk_X;
	x2 = x1 + 256;
	for (i = 1 ; i < YCrystals ; i++) {
		y1 = m_Pk_Y + (long)((i * Step) + 0.5);
		dc.MoveTo(x1, y1);
		dc.LineTo(x2, y1);
	}

	// vertical
	Step = 256.0  / XCrystals;
	y1 = m_Pk_Y;
	y2 = y1 + 256;
	for (i = 1 ; i < YCrystals ; i++) {
		x1 = m_Pk_X + (long)((i * Step) + 0.5);
		dc.MoveTo(x1, y1);
		dc.LineTo(x1, y2);
	}

	// select the white pen
	dc.SelectObject(&WhitePen);

	// find maximum value in data
	index = CrystalNum * 256;
	for (i = 1, MaxValue = 0 ; i < 255 ; i++) if (Data[index+i] > MaxValue) MaxValue = Data[index+i];

	// determine the maximum end of the y range
	if (MaxValue < 8) MaxValue = 8;
	Power = (long) log10((double) MaxValue);
	TickBase = pow(10.0, (double) Power);
	YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));
	if (((double) YRng / TickBase) < 3.0) {
		Power--;
		TickBase = pow(10.0, (double) Power);
		YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));
	}

	// Put up borders
	Border_X1 = m_Plot_X + 35;
	if (YRng >= 10000) Border_X1 += 20;
	Border_X2 = m_Plot_X + 390 - 10;
	Border_Y1 = m_Plot_Y + 10;
	Border_Y2 = m_Plot_Y + 170 - 25;
	dc.MoveTo(Border_X1, Border_Y1);
	dc.LineTo(Border_X1, Border_Y2);
	dc.LineTo(Border_X2, Border_Y2);
	dc.LineTo(Border_X2, Border_Y1);
	dc.LineTo(Border_X1, Border_Y1);

	// Vertical Tick Marks
	dc.SetTextColor(RGB(255, 255, 255));
	dc.SetBkColor(RGB(0, 0, 0));
	dc.SetTextAlign(TA_RIGHT);
	x1 = Border_X1;
	y1 = Border_Y1;
	sprintf(Str, "%1.4g", (double) YRng);
	dc.TextOut(x1, y1, Str);
	y2 = Border_Y2;
	dc.TextOut(x1, y2, "0");

	// Horizontal Tick Marks
	dc.SetTextAlign(TA_CENTER);
	for (i = 50 ; i < 256 ; i += 50) {

		// draw tick mark
		x1 = Border_X1 + ((i * (Border_X2 - Border_X1)) / 256);
		y1 = Border_Y2 - 2;
		x2 = x1;
		y2 = y1 + 2;
		dc.MoveTo(x1, y1);
		dc.LineTo(x2, y2);

		// display value
		y2 = Border_Y2 + 5;
		sprintf(Str, "%d", i);
		dc.TextOut(x2, y2, Str);
	}

	// apply any smoothing to data
	Smooth(3, CrystalSmooth, &Data[CrystalNum*256], Line);

	// Scaling values
	Scale_X = (double)(Border_X2 - Border_X1) / 255.0;
	Scale_Y = (double)(Border_Y2 - Border_Y1) / (double) YRng;

	// plot line
	for (i = 0 ; i < 256 ; i++) {

		// determine position
		x1 = Border_X1 + (long)((i * Scale_X) + 0.5);
		y1 = Border_Y2 - (long)((Line[i] * Scale_Y) + 0.5);
		if (y1 < Border_Y1) y1 = Border_Y1;

		// Plot it
		if (i == 0) {
			dc.MoveTo(x1, y1);
		} else {
			dc.LineTo(x1, y1);
		}
	}

	// plot the peak position
	dc.SelectObject(&RedPen);
	x1 = Border_X1 + (long)((Peak[CrystalNum] * Scale_X) + 0.5);
	dc.MoveTo(x1, Border_Y1);
	dc.LineTo(x1, Border_Y2);
	dc.SetTextAlign(TA_LEFT);
	dc.SetTextColor(RGB(255,   0,   0));
	dc.TextOut(Border_X1+5, Border_Y1+5, "Peak");

	// plot the lld
	dc.SelectObject(&GreenPen);
	x1 = Border_X1 + (long)(((ScannerLLD*(Peak[CrystalNum]/511.0)) * Scale_X) + 0.5);
	dc.MoveTo(x1, Border_Y1);
	dc.LineTo(x1, Border_Y2);
	dc.SetTextColor(RGB(  0, 255,   0));
	dc.TextOut(Border_X1+5, Border_Y1+20, "LLD");

	// plot the uld
	x1 = Border_X1 + (long)(((ScannerULD*(Peak[CrystalNum]/511.0)) * Scale_X) + 0.5);
	dc.MoveTo(x1, Border_Y1);
	dc.LineTo(x1, Border_Y2);
	dc.TextOut(Border_X1+5, Border_Y1+35, "ULD");

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

void CCrystalPeaks::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	long x = 0;
	long y = 0;
	long Num = 0;

	double Scale_X = 0;

	// if it was clicked in the Crystal Peaks Plot, then select new crystal
	if ((point.x >= m_Pk_X) && (point.x < (m_Pk_X+256)) && (point.y >= m_Pk_Y) && (point.y < (m_Pk_Y+256))) {

		// crystal coordinates
		x = ((point.x - m_Pk_X) * XCrystals) / 256;
		y = ((point.y - m_Pk_Y) * YCrystals) / 256;

		// New Crystal
		Num = (y * 16) + x;

		// if it really is new, then update display
		if (Num != CrystalNum) {
			CrystalNum = Num;
			Refresh();
		}
	}

	// if it was clicked in the Crystal Energy Plot, then select new crystal
	if ((point.x >= m_En_X) && (point.x < (m_En_X+256)) && (point.y >= m_En_Y) && (point.y < (m_En_Y+256))) {

		// New Crystal is y position
		Num = point.y - m_En_Y;

		// must be a crystal that is there
		x = Num % 16;
		if (x >= XCrystals) x = XCrystals - 1;
		y = Num / 16;
		if (y >= YCrystals) {
			x = XCrystals - 1;
			y = YCrystals - 1;
		}
		Num = (y * 16) + x;

		// if it really is new, then update display
		if (Num != CrystalNum) {
			CrystalNum = Num;
			Refresh();
		}
	}

	// if it was clicked in the plot, then select new peak
	if ((point.x >= Border_X1) && (point.x <= Border_X2) && (point.y >= Border_Y1) && (point.y <= Border_Y2) && (SCANNER_RING != ScannerType)) {

		// calculate new peak position
		Scale_X = 255.0 / (double)(Border_X2 - Border_X1);
		Num = (long)(((double)(point.x - Border_X1) * Scale_X) + 0.5);

		// if it really is new, then update display
		if (Num != Peak[CrystalNum]) {
			Peak[CrystalNum] = Num;
			Refresh();
			State = 1;
		}
	}
	
	CDialog::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL CCrystalPeaks::PreTranslateMessage(MSG* pMsg) 
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

void CCrystalPeaks::OnDestroy() 
{
	// local variables
	long i = 0;

	CDialog::OnDestroy();
	
	// release bitmaps
	::DeleteObject(m_en_bm);
	::DeleteObject(m_pk_bm);
	::DeleteObject(m_red_bm);
	for (i = 0 ; i < 256 ; i++) ::DeleteObject(hGreyMap[i]);

	// release device contexts
	DeleteDC(m_en_dc);
	DeleteDC(m_pk_dc);
	DeleteDC(m_red_dc);
	for (i = 0 ; i < 256 ; i++) DeleteDC(GreyColor[i]);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposUldSpin
// Purpose:		controls the response when the "ULD" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Dec 03	T Gremillion	Subroutine Created
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnDeltaposUldSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	// change value and then limit it
	ScannerULD -= (pNMUpDown->iDelta * 5);
	if (ScannerULD < 515) ScannerULD = 515;
	if (ScannerULD > 850) ScannerULD = 850;

	// display the new number
	sprintf(Str, "ULD %d", ScannerULD);
	m_ULD_Text.SetWindowText(Str);

	// replot
	Invalidate();
	
	*pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDeltaposLldSpin
// Purpose:		controls the response when the "LLD" spin control is used
// Arguments:	*pNMHDR		- NMHDR = Windows Control Structure (automatically generated)
//				*pResult	- LRESULT = Windows return value (automatically generated)
// Returns:		void
// History:		16 Dec 03	T Gremillion	Subroutine Created
/////////////////////////////////////////////////////////////////////////////

void CCrystalPeaks::OnDeltaposLldSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	char Str[MAX_STR_LEN];

	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

	// change value and then limit it
	ScannerLLD -= (pNMUpDown->iDelta * 5);
	if (ScannerLLD < 0) ScannerLLD = 0;
	if (ScannerLLD > 510) ScannerLLD = 510;

	// display the new number
	sprintf(Str, "LLD %d", ScannerLLD);
	m_LLD_Text.SetWindowText(Str);

	// replot
	Invalidate();
	
	*pResult = 0;
}
