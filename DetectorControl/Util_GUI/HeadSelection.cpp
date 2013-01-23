//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Head Selection Editor Graphical User Interface
// 
// Name:			HeadSelection.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	December 27, 2002
// 
// Description:		This component is the Detector Utilities Head Selection Editor.
//					It displays a list of checkboxes for all available heads for selection.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

// HeadSelection.cpp : implementation file
//

#include "stdafx.h"
#include "DUGUI.h"
#include "HeadSelection.h"
#include "DHI_Constants.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// HeadSelection dialog


HeadSelection::HeadSelection(CWnd* pParent /*=NULL*/)
	: CDialog(HeadSelection::IDD, pParent)
{
	//{{AFX_DATA_INIT(HeadSelection)
	//}}AFX_DATA_INIT
}


void HeadSelection::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(HeadSelection)
	DDX_Control(pDX, IDC_SELECT_ALL, m_SelectAll);
	DDX_Control(pDX, IDOK, m_OkButton);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_SELECT9, m_Select9);
	DDX_Control(pDX, IDC_SELECT8, m_Select8);
	DDX_Control(pDX, IDC_SELECT7, m_Select7);
	DDX_Control(pDX, IDC_SELECT6, m_Select6);
	DDX_Control(pDX, IDC_SELECT5, m_Select5);
	DDX_Control(pDX, IDC_SELECT4, m_Select4);
	DDX_Control(pDX, IDC_SELECT3, m_Select3);
	DDX_Control(pDX, IDC_SELECT2, m_Select2);
	DDX_Control(pDX, IDC_SELECT15, m_Select15);
	DDX_Control(pDX, IDC_SELECT14, m_Select14);
	DDX_Control(pDX, IDC_SELECT13, m_Select13);
	DDX_Control(pDX, IDC_SELECT12, m_Select12);
	DDX_Control(pDX, IDC_SELECT11, m_Select11);
	DDX_Control(pDX, IDC_SELECT10, m_Select10);
	DDX_Control(pDX, IDC_SELECT1, m_Select1);
	DDX_Control(pDX, IDC_SELECT0, m_Select0);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(HeadSelection, CDialog)
	//{{AFX_MSG_MAP(HeadSelection)
	ON_BN_CLICKED(IDC_SELECT_ALL, OnSelectAll)
	ON_BN_CLICKED(IDC_SELECT0, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT1, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT2, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT3, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT4, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT5, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT6, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT7, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT8, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT9, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT10, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT11, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT12, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT13, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT14, OnSelectSingle)
	ON_BN_CLICKED(IDC_SELECT15, OnSelectSingle)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// HeadSelection message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
//				09 Jan 04	T Gremillion	Added "All" button
/////////////////////////////////////////////////////////////////////////////

BOOL HeadSelection::OnInitDialog() 
{
	// local variables
	long i = 0;
	char Str[MAX_STR_LEN];
	char ElectronicsType[20] = "Head";

	CDialog::OnInitDialog();

	// Initialize selection to none
	for (i = 0 ; i < MAX_HEADS ; i++) Selection[i] = 0;

	// if ring scanner, switch the name to "Bucket"
	if (SCANNER_RING == ScannerType) {
		sprintf(ElectronicsType, "Bucket");
		::SetWindowText(m_hWnd, "Bucket Selection");
	}

	// turn off visibility on all selection check boxes
	m_Select0.ShowWindow(SW_HIDE);
	m_Select1.ShowWindow(SW_HIDE);
	m_Select2.ShowWindow(SW_HIDE);
	m_Select3.ShowWindow(SW_HIDE);
	m_Select4.ShowWindow(SW_HIDE);
	m_Select5.ShowWindow(SW_HIDE);
	m_Select6.ShowWindow(SW_HIDE);
	m_Select7.ShowWindow(SW_HIDE);
	m_Select8.ShowWindow(SW_HIDE);
	m_Select9.ShowWindow(SW_HIDE);
	m_Select10.ShowWindow(SW_HIDE);
	m_Select11.ShowWindow(SW_HIDE);
	m_Select12.ShowWindow(SW_HIDE);
	m_Select13.ShowWindow(SW_HIDE);
	m_Select14.ShowWindow(SW_HIDE);
	m_Select15.ShowWindow(SW_HIDE);

	// activate the correct number of heads
	switch (NumHeads) {

	case 16:
		m_Select15.MoveWindow(5, 325, 100, 15, TRUE);
		m_Select15.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[15]) m_Select15.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[15]);
		m_Select15.SetWindowText(Str);

	case 15:
		m_Select14.MoveWindow(5, 305, 100, 15, TRUE);
		m_Select14.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[14]) m_Select14.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[14]);
		m_Select14.SetWindowText(Str);

	case 14:
		m_Select13.MoveWindow(5, 285, 100, 15, TRUE);
		m_Select13.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[13]) m_Select13.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[13]);
		m_Select13.SetWindowText(Str);

	case 13:
		m_Select12.MoveWindow(5, 265, 100, 15, TRUE);
		m_Select12.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[12]) m_Select12.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[12]);
		m_Select12.SetWindowText(Str);

	case 12:
		m_Select11.MoveWindow(5, 245, 100, 15, TRUE);
		m_Select11.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[11]) m_Select11.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[11]);
		m_Select11.SetWindowText(Str);

	case 11:
		m_Select10.MoveWindow(5, 225, 100, 15, TRUE);
		m_Select10.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[10]) m_Select10.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[10]);
		m_Select10.SetWindowText(Str);

	case 10:
		m_Select9.MoveWindow(5, 205, 100, 15, TRUE);
		m_Select9.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[9]) m_Select9.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[9]);
		m_Select9.SetWindowText(Str);

	case 9:	
		m_Select8.MoveWindow(5, 185, 100, 15, TRUE);
		m_Select8.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[8]) m_Select8.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[8]);
		m_Select8.SetWindowText(Str);

	case 8:	
		m_Select7.MoveWindow(5, 165, 100, 15, TRUE);
		m_Select7.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[7]) m_Select7.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[7]);
		m_Select7.SetWindowText(Str);

	case 7:	
		m_Select6.MoveWindow(5, 145, 100, 15, TRUE);
		m_Select6.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[6]) m_Select6.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[6]);
		m_Select6.SetWindowText(Str);

	case 6:	
		m_Select5.MoveWindow(5, 125, 100, 15, TRUE);
		m_Select5.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[5]) m_Select5.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[5]);
		m_Select5.SetWindowText(Str);

	case 5:	
		m_Select4.MoveWindow(5, 105, 100, 15, TRUE);
		m_Select4.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[4]) m_Select4.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[4]);
		m_Select4.SetWindowText(Str);

	case 4:	
		m_Select3.MoveWindow(5, 85, 100, 15, TRUE);
		m_Select3.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[3]) m_Select3.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[3]);
		m_Select3.SetWindowText(Str);

	case 3:	
		m_Select2.MoveWindow(5, 65, 100, 15, TRUE);
		m_Select2.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[2]) m_Select2.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[2]);
		m_Select2.SetWindowText(Str);

	case 2:	
		m_Select1.MoveWindow(5, 45, 100, 15, TRUE);
		m_Select1.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[1]) m_Select1.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[1]);
		m_Select1.SetWindowText(Str);

	case 1:	
		m_Select0.MoveWindow(5, 25, 100, 15, TRUE);
		m_Select0.ShowWindow(SW_SHOW);
		if (0 == HeadAllow[0]) m_Select0.EnableWindow(FALSE);
		sprintf(Str, "%s %d", ElectronicsType, HeadIndex[0]);
		m_Select0.SetWindowText(Str);

	default:
		m_SelectAll.MoveWindow(5, 5, 100, 15, TRUE);
		for (NumAllow = 0, i = 0 ; i < NumHeads ; i++) if (HeadAllow[i]) NumAllow++;
		if (0 == NumAllow) m_SelectAll.EnableWindow(FALSE);
	}

	// set position of buttons
	m_CancelButton.MoveWindow(5, 25+(20 * NumHeads), 50, 25, TRUE);
	m_OkButton.MoveWindow(65, 25+(20 * NumHeads), 50, 25, TRUE);

	// resize window
	::SetWindowPos((HWND) m_hWnd, HWND_TOP, 0, 0, 130, 80+(20*NumHeads), SWP_NOMOVE);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL HeadSelection::PreTranslateMessage(MSG* pMsg) 
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
// Routine:		OnOK
// Purpose:		controls the response when the dialog is exited
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void HeadSelection::OnOK() 
{
	// check each head switch displayed to see if it has been selected
	switch (NumHeads) {
	case 16: Selection[15] = m_Select15.GetCheck();
	case 15: Selection[14] = m_Select14.GetCheck();
	case 14: Selection[13] = m_Select13.GetCheck();
	case 13: Selection[12] = m_Select12.GetCheck();
	case 12: Selection[11] = m_Select11.GetCheck();
	case 11: Selection[10] = m_Select10.GetCheck();
	case 10: Selection[9] = m_Select9.GetCheck();
	case 9: Selection[8] = m_Select8.GetCheck();
	case 8: Selection[7] = m_Select7.GetCheck();
	case 7: Selection[6] = m_Select6.GetCheck();
	case 6: Selection[5] = m_Select5.GetCheck();
	case 5: Selection[4] = m_Select4.GetCheck();
	case 4: Selection[3] = m_Select3.GetCheck();
	case 3: Selection[2] = m_Select2.GetCheck();
	case 2: Selection[1] = m_Select1.GetCheck();
	case 1: Selection[0] = m_Select0.GetCheck();
	}

	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelectAll
// Purpose:		changes the state of all check boxes the same as the one labeled "All"
// Arguments:	none
// Returns:		void
// History:		09 Jan 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

void HeadSelection::OnSelectAll() 
{
	// local variable
	long State = 0;

	// retrieve the state of the check box
	State = m_SelectAll.GetCheck();

	// set all present to the same state
	switch (NumHeads) {
	case 16: if (0 != HeadAllow[15]) m_Select15.SetCheck(State);
	case 15: if (0 != HeadAllow[14]) m_Select14.SetCheck(State);
	case 14: if (0 != HeadAllow[13]) m_Select13.SetCheck(State);
	case 13: if (0 != HeadAllow[12]) m_Select12.SetCheck(State);
	case 12: if (0 != HeadAllow[11]) m_Select11.SetCheck(State);
	case 11: if (0 != HeadAllow[10]) m_Select10.SetCheck(State);
	case 10: if (0 != HeadAllow[9]) m_Select9.SetCheck(State);
	case 9: if (0 != HeadAllow[8]) m_Select8.SetCheck(State);
	case 8: if (0 != HeadAllow[7]) m_Select7.SetCheck(State);
	case 7: if (0 != HeadAllow[6]) m_Select6.SetCheck(State);
	case 6: if (0 != HeadAllow[5]) m_Select5.SetCheck(State);
	case 5: if (0 != HeadAllow[4]) m_Select4.SetCheck(State);
	case 4: if (0 != HeadAllow[3]) m_Select3.SetCheck(State);
	case 3: if (0 != HeadAllow[2]) m_Select2.SetCheck(State);
	case 2: if (0 != HeadAllow[1]) m_Select1.SetCheck(State);
	case 1: if (0 != HeadAllow[0]) m_Select0.SetCheck(State);
	}	
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnSelectSingle
// Purpose:		Controls the state of the "All" checkbox by checking the
//				state of all the individual ones.
// Arguments:	none
// Returns:		void
// History:		09 Jan 04	T Gremillion	New Routine
/////////////////////////////////////////////////////////////////////////////

void HeadSelection::OnSelectSingle() 
{
	// local variable
	long State = 0;

	// Add the state of each head present to the overall state
	switch (NumHeads) {
	case 16: State += m_Select15.GetCheck();
	case 15: State += m_Select14.GetCheck();
	case 14: State += m_Select13.GetCheck();
	case 13: State += m_Select12.GetCheck();
	case 12: State += m_Select11.GetCheck();
	case 11: State += m_Select10.GetCheck();
	case 10: State += m_Select9.GetCheck();
	case  9: State += m_Select8.GetCheck();
	case  8: State += m_Select7.GetCheck();
	case  7: State += m_Select6.GetCheck();
	case  6: State += m_Select5.GetCheck();
	case  5: State += m_Select4.GetCheck();
	case  4: State += m_Select3.GetCheck();
	case  3: State += m_Select2.GetCheck();
	case  2: State += m_Select1.GetCheck();
	case  1: State += m_Select0.GetCheck();
	}

	// integer math will result in a single overall state
	if (NumAllow > 0) State /= NumAllow;

	// set the overall state
	m_SelectAll.SetCheck(State);
}
