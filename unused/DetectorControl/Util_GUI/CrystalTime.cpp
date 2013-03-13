//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Crystal Time Offsets Editor Graphical User Interface
// 
// Name:			CrystalTime.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	January 8, 2003
// 
// Description:		This component is the Detector Utilities Crystal Time Offsets Editor.
//					It displays the crystal time differenc of the selected block with the current
//					time offsets overlaid on top as well as a display of all time offsets
//					for the blocks and a plot for the current crystal.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// CrystalTime.cpp : implementation file
//

#include "stdafx.h"
#include "DUGUI.h"
#include "CrystalTime.h"
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
// CCrystalTime dialog


CCrystalTime::CCrystalTime(CWnd* pParent /*=NULL*/)
	: CDialog(CCrystalTime::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCrystalTime)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCrystalTime::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCrystalTime)
	DDX_Control(pDX, IDRESET, m_ResetButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDC_PREVIOUS, m_PrevButton);
	DDX_Control(pDX, IDC_NEXT, m_NextButton);
	DDX_Control(pDX, IDC_OFFSET_TEXT, m_OffsetText);
	DDX_Control(pDX, IDC_CRYSTALTEXT, m_CrystalText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCrystalTime, CDialog)
	//{{AFX_MSG_MAP(CCrystalTime)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	ON_BN_CLICKED(IDRESET, OnReset)
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrystalTime message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
//				29 Sep 03	T Gremillion	Hide Prev & Next buttons if HRRT
//				12 Jan 04	T Gremillion	Changes to handle crystal time in 
//											addition to system time
//				15 Jan 04	T Gremillion	Don't hide Prev & Next if tdc time
/////////////////////////////////////////////////////////////////////////////

BOOL CCrystalTime::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long cx = 0;
	long cy = 0;
	long color = 0;
	long Ratio = 0;
	long MaxValue = 0;

	WINDOWPLACEMENT Placement;

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
	m_Time_bm = CreateCompatibleBitmap(dc, 256, 256);
	m_Time_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_Time_dc, m_Time_bm);

	// Create a memory DC and bitmap and associate them for the crystal energy
	m_Offset_bm = CreateCompatibleBitmap(dc, 256, 256);
	m_Offset_dc = ::CreateCompatibleDC(dc);
	SelectObject(m_Offset_dc, m_Offset_bm);


	// genearate a summed histogram
	for (i = 0 ; i < TimingBins ; i++) {

		// initialize
		All[i] = 0;

		// for each crystal
		for (j = 0 ; j < 256 ; j++) All[i] += Data[(TimingBins*j)+i];

		// for the HRRT also store it in the last crystal
		if (SCANNER_HRRT == Scanner) Data[(TimingBins*255)+i] = All[i];
	}

	// store incoming peaks for possible "Reset"
	for (i = 0 ; i < MAX_CRYSTALS ; i++) m_OrigTime[i] = Time[i];

	// define areas
	m_Off_X = 10;
	m_Off_Y = 10;
	m_Time_X = 275;
	m_Time_Y = m_Off_Y;
	m_Plot_X = m_Off_X;
	m_Plot_Y = 275;

	// find maximum value
	for (MaxValue = 0, i = 0 ; i < (XCrystals * YCrystals * TimingBins) ; i++) {
		if (Data[i] > MaxValue) MaxValue = Data[i];
	}

	// loop through crystals
	Ratio = 256 / TimingBins;
	for (y = 0 ; y < 256 ; y++) {

		// crystal coordintes
		cx = y % 16;
		cy = y / 16;

		// if not a valid crystal, fill with black
		if ((cx >= YCrystals) || (cy >= XCrystals)) {

			for (x = 0 ; x < 256 ; x++) BitBlt(m_Time_dc, x, y, 1, 1, GreyColor[0], 0, 0, SRCCOPY);

		// otherwise, fill with proper color
		} else {

			// index to to data
			if (View == DHI_MODE_TIME) {
				index = ((cy * 16) + cx) * TimingBins;
			} else {
				index = ((cy * XCrystals) + cx) * TimingBins;
			}

			// fill in bitmap
			for (x = 0 ; x < 256 ; x++) {
				color = (long) (256.0 * ((double) Data[index+(x/Ratio)] / (double) (MaxValue + 1)));
				BitBlt(m_Time_dc, x, y, 1, 1, GreyColor[color], 0, 0, SRCCOPY);
			}
		}
	}

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, 0, 0, 550, 480, SWP_NOMOVE);

	// Positively position controls
	m_PrevButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 5;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 65;
	Placement.rcNormalPosition.top = m_Plot_Y;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 25;
	m_PrevButton.SetWindowPlacement(&Placement);
	if ((DHI_MODE_TIME != View) && (SCANNER_HRRT == Scanner)) m_PrevButton.ShowWindow(SW_HIDE);

	m_NextButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 70;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 25;
	m_NextButton.SetWindowPlacement(&Placement);
	if ((DHI_MODE_TIME != View) && (SCANNER_HRRT == Scanner)) m_NextButton.ShowWindow(SW_HIDE);

	m_CrystalText.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 5;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 35;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 55;
	m_CrystalText.SetWindowPlacement(&Placement);

	m_OffsetText.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 5;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 60;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 80;
	m_OffsetText.SetWindowPlacement(&Placement);
	if (DHI_MODE_TIME == View) m_OffsetText.ShowWindow(SW_HIDE);

	m_ResetButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 5;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 85;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 110;
	m_ResetButton.SetWindowPlacement(&Placement);
	if (SCANNER_HRRT == Scanner) m_ResetButton.ShowWindow(SW_HIDE);
	if (DHI_MODE_TIME == View) m_ResetButton.ShowWindow(SW_HIDE);

	m_OKButton.GetWindowPlacement(&Placement);
	Placement.rcNormalPosition.left = m_Time_X + 127 + 5;
	Placement.rcNormalPosition.right = m_Time_X + 127 + 130;
	Placement.rcNormalPosition.top = m_Plot_Y + 115;
	Placement.rcNormalPosition.bottom = m_Plot_Y + 140;
	m_OKButton.SetWindowPlacement(&Placement);

	// Refresh Display
	Refresh();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		Refresh
// Purpose:		update fields and offsets bitmap
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				29 Sep 03	T Gremillion	display centroid and fwhm for HRRT
//				12 Jan 04	T Gremillion	don't make bitmap of crystal offsets 
//											if crystal time
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::Refresh()
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x = 0;
	long y = 0;
	long color = 0;

	double fwhm = 0.0;
	double centroid = 0.0;

	char Str[MAX_STR_LEN];
	
	// fill in values with individual crystal values if not HRRT
	if (SCANNER_HRRT != Scanner) {
		sprintf(Str, "Crystal Number: %d", CrystalNum);
		m_CrystalText.SetWindowText(Str);
		sprintf(Str, "Time Offset: %d", Time[CrystalNum]);
		m_OffsetText.SetWindowText(Str);

	// for the HRRT fill in these fields with the centroid and fwhm
	} else {

		// determine the centroid and fwhm of the composite data (stored in crystal 255)
		index = 255 * TimingBins;
		fwhm = peak_analyze(&Data[index], TimingBins, &centroid);

		// fill in values
		sprintf(Str, "Centroid %5.2f", centroid);
		m_CrystalText.SetWindowText(Str);
		sprintf(Str, "FWHM: %5.2f", fwhm);
		m_OffsetText.SetWindowText(Str);
	}

	// crystal peaks bitmap
	if (View != DHI_MODE_TIME) {
		for (y = 0 ; y < 256 ; y++) {
			for (x = 0 ; x < 256 ; x++) {

					// determine crystal
					i = (x * XCrystals) / 256;
					j = (y * YCrystals) / 256;
					index = (j * 16) + i;

					// retrieve value
					color = 128 + (Time[index] * 4);
					BitBlt(m_Offset_dc, x, y, 1, 1, GreyColor[color], 0, 0, SRCCOPY);
			}
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
//				12 Jan 04	T Gremillion	Crystal Time - upper left bitmap 
//											is composite histogram
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnPaint() 
{
	// local variables
	long i = 0;
	long j = 0;
	long index = 0;
	long x1 = 0;
	long x2 = 0;
	long y1 = 0;
	long y2 = 0;
	long MidPoint = 0;
	long HighPoint = 0;
	long MaxValue = 0;
	long Power = 0;
	long XRng = 0;
	long YRng = 0;
	long Value = 0;
	long TickSkip = 0;
	long Start = 0;
	long CurrCrystal = 0;

	double Step = 0;
	double TickBase = 0.0;
	double Scale_X = 0.0;
	double Scale_Y = 0.0;

	char Str[MAX_STR_LEN];

	CPaintDC dc(this); // device context for painting

	// define a pens for drawing
	CPen RedPen(PS_SOLID, 1, RGB(255, 0 , 0));
	CPen WhitePen(PS_SOLID, 1, RGB(255, 255 , 255));
	CPen BlackPen(PS_SOLID, 1, RGB(0, 0 , 0));

	// copy offsets bitmap to dialog
	::StretchBlt(dc.m_hDC,			//destination
					m_Off_X,
					m_Off_Y,
					256,
					256,
					m_Offset_dc,		//source
					0,
					0,
					256,
					256,
					SRCCOPY);

	// copy time bitmap to dialog
	::StretchBlt(dc.m_hDC,			//destination
					m_Time_X,
					m_Time_Y,
					256,
					256,
					m_Time_dc,		//source
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

	// Current crystal
	CurrCrystal = CrystalNum;
	if (SCANNER_HRRT == Scanner) CurrCrystal = 255;

	// plot grid on offset display
	dc.SelectObject(&BlackPen);

	// horizontal
	Step = 256.0 / YCrystals;
	x1 = m_Off_X;
	x2 = x1 + 256;
	for (i = 1 ; i < YCrystals ; i++) {
		y1 = m_Off_Y + (long)((i * Step) + 0.5);
		dc.MoveTo(x1, y1);
		dc.LineTo(x2, y1);
	}

	// vertical
	Step = 256.0 / XCrystals;
	y1 = m_Off_Y;
	y2 = y1 + 256;
	for (i = 1 ; i < YCrystals ; i++) {
		x1 = m_Off_X + (long)((i * Step) + 0.5);
		dc.MoveTo(x1, y1);
		dc.LineTo(x1, y2);
	}

	// select the white pen
	dc.SelectObject(&WhitePen);

	// for tdc time data
	if (DHI_MODE_TIME == View) {

		// find maximum value in data
		for (i = 0, MaxValue = 0 ; i < 256 ; i++) {
			if (All[i] > MaxValue) MaxValue = All[i];
		}

		// determine the maximum end of the y range
		if (MaxValue < 8) MaxValue = 8;
		Power = (long) log10((double) MaxValue);
		TickBase = pow(10.0, (double) Power);
		YRng = MaxValue + ((long) TickBase - (MaxValue % (long) TickBase));

		// put up range values
		dc.SetTextAlign(TA_BOTTOM | TA_LEFT);
		dc.TextOut(m_Off_X, m_Off_Y+256, "0");
		dc.SetTextAlign(TA_BOTTOM | TA_RIGHT);
		dc.TextOut(m_Off_X+256, m_Off_Y+256, "256");
		sprintf(Str, "%d", YRng);
		dc.SetTextAlign(TA_TOP | TA_LEFT);
		dc.TextOut(m_Off_X, m_Off_Y, Str);

		// plot
		Scale_Y = 256.0 / (double) YRng;
		for (i = 0 ; i < 256 ; i++) {

			// coordintes
			x1 = m_Off_X + (long)((i * 256) / 256);
			y1 = (m_Off_Y + 256) - (long)((All[i] * Scale_Y) + 0.5);

			// Plot it
			if (i == 0) {
				dc.MoveTo(x1, y1);
			} else {
				dc.LineTo(x1, y1);
			}
		}
	}

	// find maximum value in data
	index = (((CurrCrystal / 16) * XCrystals) + (CurrCrystal % 16)) * TimingBins;
	if (SCANNER_HRRT == Scanner) index = 255 * TimingBins;
	if (DHI_MODE_TIME == View) index = CurrCrystal * TimingBins;
	for (i = 0, HighPoint = 0, MaxValue = 0 ; i < TimingBins ; i++) {
		if (Data[index+i] > MaxValue) {
			HighPoint = i;
			MaxValue = Data[index+i];
		}
	}

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
	dc.SetTextAlign(TA_RIGHT);
	x1 = Border_X1;
	y1 = Border_Y1;
	sprintf(Str, "%1.4g", (double) YRng);
	dc.TextOut(x1, y1, Str);
	y2 = Border_Y2;
	dc.TextOut(x1, y2, "0");

	// determine the range of values to be plotted
	if (DHI_MODE_TIME != View) {
		MidPoint = TimingBins / 2;
		XRng = 2 * abs(HighPoint - MidPoint);
		Power = (long) log((double) XRng) + 1;
		if (Power < 4) Power = 4;
		m_PlotRange = (long)pow(2, Power);
		if (m_PlotRange > TimingBins) m_PlotRange = TimingBins;
	} else {
		MidPoint = TimingBins / 2;
		m_PlotRange = TimingBins;
		Power = 8;
	}

	// horizontal tick marks
	y1 = Border_Y2;
	y2 = y1 - 5;
	TickSkip = (long) pow(2, (Power - 4));
	for (i = 0 ; i < m_PlotRange ; i += TickSkip) {
		x1 = Border_X1 + (long)((i * (Border_X2 - Border_X1)) / m_PlotRange);
		dc.MoveTo(x1, y1);
		dc.LineTo(x1, y2);
	}

	// Horizontal tick labels
	if (DHI_MODE_TIME == View) {
		dc.SetTextAlign(TA_CENTER);
		x1 = Border_X1;
		x2 = x1 + ((Border_X2 - Border_X1) / 4);
		Value = m_PlotRange / 4;
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
		x2 = x1 + ((Border_X2 - Border_X1) / 2);
		Value = m_PlotRange / 2;
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
		x2 = x1 + ((3*(Border_X2 - Border_X1)) / 4);
		Value = (3 * m_PlotRange) / 4;
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
		x2 = Border_X2;
		Value = m_PlotRange;
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
	} else {
		dc.SetTextAlign(TA_CENTER);
		x1 = (Border_X2 + Border_X1) / 2;
		Value = 0;
		sprintf(Str, "%d", Value);
		dc.TextOut(x1, Border_Y2, Str);
		x2 = (x1 + Border_X1) / 2;
		Value = 0 - (long)(m_PlotRange / 4);
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
		x2 = (x1 + Border_X2) / 2;
		Value = 0 + (long)(m_PlotRange / 4);
		sprintf(Str, "%d", Value);
		dc.TextOut(x2, Border_Y2, Str);
	}

	// Plot
	Start = MidPoint - (long) (m_PlotRange / 2);
	Scale_Y = (double) (Border_Y2 - Border_Y1) / (double) MaxValue;
	for (i = 0 ; i < m_PlotRange ; i++) {

		// coordintes
		x1 = Border_X1 + (long)((i * (Border_X2 - Border_X1)) / m_PlotRange);
		y1 = Border_Y2 - (long)((Data[index+Start+i] * Scale_Y) + 0.5);

		// Plot it
		if (i == 0) {
			dc.MoveTo(x1, y1);
		} else {
			dc.LineTo(x1, y1);
		}
	}


	// offset value not drawn for tdc time
	
	// for tdc time data
	if (DHI_MODE_TIME != View) {

		// select the red pen
		dc.SelectObject(&RedPen);

		// line indicating offset value
		x1 = Border_X1 + (long)(((MidPoint + Time[CrystalNum] - Start) * (Border_X2 - Border_X1)) / m_PlotRange);
		dc.MoveTo(x1, Border_Y2);
		dc.LineTo(x1, Border_Y1);
	}

	// Do not call CDialog::OnPaint() for painting messages
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnNext
// Purpose:		Actions taken when the "Next" crystal button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnNext() 
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
// Routine:		OnPrevious
// Purpose:		Actions taken when the "Prevoius" crystal button is pressed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnPrevious() 
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
// Routine:		OnReset
// Purpose:		Actions taken when the "Reset" button is pressed.  values returned
//				to entry state
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnReset() 
{
		// local variables
	long i = 0;

	// transfer original peaks back
	for (i = 0 ; i < MAX_CRYSTALS ; i++) Time[i] = m_OrigTime[i];

	// Refresh the display
	Refresh();

	// changes have been cleared
	State = 0;

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
//				15 Jan 04	T Gremillion	Deactivated click in offset plot or 
//											crystal plot for tdc data
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// local variables
	long x = 0;
	long y = 0;
	long Num = 0;

	double Scale_X = 0;

	// if it was clicked in the Time Offsets Plot, then select new crystal
	if ((DHI_MODE_TIME != View) && (point.x >= m_Off_X) && (point.x < (m_Off_X+256)) && (point.y >= m_Off_Y) && (point.y < (m_Off_Y+256))) {

		// crystal coordinates
		x = ((point.x - m_Off_X) * XCrystals) / 256;
		y = ((point.y - m_Off_Y) * YCrystals) / 256;

		// New Crystal
		Num = (y * 16) + x;

		// if it really is new, then update display
		if (Num != CrystalNum) {
			CrystalNum = Num;
			Refresh();
		}
	}

	// if it was clicked in the Crystal Time Plot, then select new crystal
	if ((point.x >= m_Time_X) && (point.x < (m_Time_X+256)) && (point.y >= m_Time_Y) && (point.y < (m_Time_Y+256))) {

		// New Crystal is y position
		Num = point.y - m_Time_Y;

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
	if ((DHI_MODE_TIME != View) && (point.x >= Border_X1) && (point.x <= Border_X2) && (point.y >= Border_Y1) && (point.y <= Border_Y2) && (SCANNER_HRRT != Scanner) && (SCANNER_RING != Scanner)) {

		// calculate new peak position
		Scale_X = m_PlotRange / (double)(Border_X2 - Border_X1);
		Num = (long)(((double)(point.x - Border_X1) * Scale_X) + 0.5) - (m_PlotRange / 2);

		// if it really is new, then update display
		if (Num != Time[CrystalNum]) {
			Time[CrystalNum] = Num;
			Refresh();
			State = 1;
		}
	}
	
	CDialog::OnLButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnDestroy
// Purpose:		Steps to be performed when the GUI is closed
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void CCrystalTime::OnDestroy() 
{
	// local variables
	long i = 0;

	CDialog::OnDestroy();
	
	// release bitmaps
	::DeleteObject(m_Time_bm);
	::DeleteObject(m_Offset_bm);
	::DeleteObject(m_red_bm);
	for (i = 0 ; i < 256 ; i++) ::DeleteObject(hGreyMap[i]);

	// release device contexts
	DeleteDC(m_Time_dc);
	DeleteDC(m_Offset_dc);
	DeleteDC(m_red_dc);
	for (i = 0 ; i < 256 ; i++) DeleteDC(GreyColor[i]);
}
