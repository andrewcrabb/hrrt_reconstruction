//////////////////////////////////////////////////////////////////////////////////////////////
// Component:		Energy Range Graphical User Interface
// 
// Name:			EnergyRange.cpp
// 
// Authors:			Tim Gremillion
// 
// Language:		C++
// 
// Creation Date:	January 3, 2003
// 
// Description:		This component is the Detector Utilities Energy Range Editor.
//					It allows the user to change the lld and uld for all acquirable
//					data types.
// 
// Copyright (C) CPS Innovations 2003 All Rights Reserved.
//////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DUGUI.h"
#include "EnergyRange.h"

#include "DHI_Constants.h"
#include "Validate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// global variables
static m_NumSelect;
static m_ViewIndex[NUM_DHI_MODES];

/////////////////////////////////////////////////////////////////////////////
// EnergyRange dialog


EnergyRange::EnergyRange(CWnd* pParent /*=NULL*/)
	: CDialog(EnergyRange::IDD, pParent)
{
	//{{AFX_DATA_INIT(EnergyRange)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void EnergyRange::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(EnergyRange)
	DDX_Control(pDX, IDC_VIEWDROP_RANGE, m_ViewDropRange);
	DDX_Control(pDX, IDC_ULD, m_ULD);
	DDX_Control(pDX, IDC_LLD, m_LLD);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(EnergyRange, CDialog)
	//{{AFX_MSG_MAP(EnergyRange)
	ON_EN_KILLFOCUS(IDC_ULD, OnKillfocusUld)
	ON_EN_KILLFOCUS(IDC_LLD, OnKillfocusLld)
	ON_CBN_SELCHANGE(IDC_VIEWDROP_RANGE, OnSelchangeViewdropRange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// EnergyRange message handlers

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnInitDialog
// Purpose:		GUI Initialization
// Arguments:	None
// Returns:		BOOL - whether the application has set the input focus to one of the controls in the dialog box
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

BOOL EnergyRange::OnInitDialog() 
{
	// local variables
	long i = 0;
	long j = 0;
	long lld = 0;
	long uld = 0;

	char Str[MAX_STR_LEN];

	CDialog::OnInitDialog();
	
	// Views
	m_ViewIndex[i] = DHI_MODE_POSITION_PROFILE;
	m_ViewDropRange.InsertString(i++, "Position Profile");
	m_ViewIndex[i] = DHI_MODE_CRYSTAL_ENERGY;
	m_ViewDropRange.InsertString(i++, "Crystal Energy");
	m_ViewIndex[i] = DHI_MODE_TUBE_ENERGY;
	m_ViewDropRange.InsertString(i++, "Tube Energy");
	m_ViewIndex[i] = DHI_MODE_RUN;
	if (ScannerType == SCANNER_P39_IIA) {
		m_ViewDropRange.InsertString(i++, "Run/Time Mode");
	} else {
		m_ViewDropRange.InsertString(i++, "Run Mode");
	}
	if ((ScannerType != SCANNER_P39) && (ScannerType != SCANNER_P39_IIA)){
		m_ViewIndex[i] = DHI_MODE_SHAPE_DISCRIMINATION;
		m_ViewDropRange.InsertString(i++, "Shape Discrimination");
	}
	for (j = 0, m_NumSelect = -1 ; j < i ; j++) if (m_ViewIndex[j] == RangeView) m_NumSelect = j;
	if (m_NumSelect == -1) m_NumSelect = 0;
	m_ViewDropRange.SetCurSel(m_NumSelect);

	// set the lld/uld fields based on the input mode selected
	switch (m_ViewIndex[m_NumSelect]) {

	case DHI_MODE_POSITION_PROFILE:
		lld = RangePP_lld;
		uld = RangePP_uld;
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		lld = RangeEN_lld;
		uld = RangeEN_uld;
		break;

	case DHI_MODE_TUBE_ENERGY:
		lld = RangeTE_lld;
		uld = RangeTE_uld;
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		lld = RangeSD_lld;
		uld = RangeSD_uld;
		break;

	case DHI_MODE_RUN:
		lld = RangeRun_lld;
		uld = RangeRun_uld;
		break;
	}

	// fill in lld/uld fields
	sprintf(Str, "%d", lld);
	m_LLD.SetWindowText(Str);
	sprintf(Str, "%d", uld);
	m_ULD.SetWindowText(Str);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnOK
// Purpose:		controls the response when the dialog is exited
// Arguments:	none
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void EnergyRange::OnOK() 
{
	// changes are to be acceppted
	Accept = 1;
	
	CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusUld
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the ULD
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void EnergyRange::OnKillfocusUld() 
{
	// local variables
	long Value = 0;
	long Status = 0;

	char Str[MAX_STR_LEN];

	// retrieve string from field
	m_ULD.GetWindowText(Str, 10);

	// validate it according to type
	switch (m_ViewIndex[m_NumSelect]) {

	case DHI_MODE_POSITION_PROFILE:
		Status = Validate(Str, RangePP_lld, 255, &Value);
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		Status = Validate(Str, RangeEN_lld, 255, &Value);
		break;

	case DHI_MODE_TUBE_ENERGY:
		Status = Validate(Str, RangeTE_lld, 255, &Value);
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		Status = Validate(Str, RangeSD_lld, 255, &Value);
		break;

	case DHI_MODE_RUN:
		Status = Validate(Str, RangeRun_lld, 1500, &Value);
		break;
	}

	// if valid, update
	if (Status == 0) {
		switch (m_ViewIndex[m_NumSelect]) {

		case DHI_MODE_POSITION_PROFILE:
			RangePP_uld = Value;
			break;

		case DHI_MODE_CRYSTAL_ENERGY:
			RangeEN_uld = Value;
			break;

		case DHI_MODE_TUBE_ENERGY:
			RangeTE_uld = Value;
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:
			RangeSD_uld = Value;
			break;

		case DHI_MODE_RUN:
			RangeRun_uld = Value;
			break;
		}
	}

	// fill in current value
	switch (m_ViewIndex[m_NumSelect]) {

	case DHI_MODE_POSITION_PROFILE:
		sprintf(Str, "%d", RangePP_uld);
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		sprintf(Str, "%d", RangeEN_uld);
		break;

	case DHI_MODE_TUBE_ENERGY:
		sprintf(Str, "%d", RangeTE_uld);
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		sprintf(Str, "%d", RangeSD_uld);
		break;

	case DHI_MODE_RUN:
		sprintf(Str, "%d", RangeRun_uld);
		break;
	}

	// post string
	m_ULD.SetWindowText(Str);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		OnKillfocusLld
// Purpose:		causes the value stored in memory and displayed in the text
//				field to be updated when focus is removed from the LLD
//				text field 
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void EnergyRange::OnKillfocusLld() 
{
	// local variables
	long Value = 0;
	long Status = 0;

	char Str[MAX_STR_LEN];

	// retrieve string from field
	m_LLD.GetWindowText(Str, 10);

	// validate it according to type
	switch (m_ViewIndex[m_NumSelect]) {

	case DHI_MODE_POSITION_PROFILE:
		Status = Validate(Str, 0, RangePP_uld, &Value);
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		Status = Validate(Str, 0, RangeEN_uld, &Value);
		break;

	case DHI_MODE_TUBE_ENERGY:
		Status = Validate(Str, 0, RangeTE_uld, &Value);
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		Status = Validate(Str, 0, RangeSD_uld, &Value);
		break;

	case DHI_MODE_RUN:
		Status = Validate(Str, 0, RangeRun_uld, &Value);
		break;
	}

	// if valid, update
	if (Status == 0) {
		switch (m_ViewIndex[m_NumSelect]) {

		case DHI_MODE_POSITION_PROFILE:
			RangePP_lld = Value;
			break;

		case DHI_MODE_CRYSTAL_ENERGY:
			RangeEN_lld = Value;
			break;

		case DHI_MODE_TUBE_ENERGY:
			RangeTE_lld = Value;
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:
			RangeSD_lld = Value;
			break;

		case DHI_MODE_RUN:
			RangeRun_lld = Value;
			break;
		}
	}

	// fill in current value
	switch (m_ViewIndex[m_NumSelect]) {

	case DHI_MODE_POSITION_PROFILE:
		sprintf(Str, "%d", RangePP_lld);
		break;

	case DHI_MODE_CRYSTAL_ENERGY:
		sprintf(Str, "%d", RangeEN_lld);
		break;

	case DHI_MODE_TUBE_ENERGY:
		sprintf(Str, "%d", RangeTE_lld);
		break;

	case DHI_MODE_SHAPE_DISCRIMINATION:
		sprintf(Str, "%d", RangeSD_lld);
		break;

	case DHI_MODE_RUN:
		sprintf(Str, "%d", RangeRun_lld);
		break;
	}

	// post string
	m_LLD.SetWindowText(Str);
}

/////////////////////////////////////////////////////////////////////////////
// Routine:		PreTranslateMessage
// Purpose:		causes the focus to be changed when the "Enter" key is pressed 
// Arguments:	*pMsg	- MSG = identifies the key that was pressed
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
//				11 Jun 04	T Gremillion	modified to block Escape Key as well
/////////////////////////////////////////////////////////////////////////////

BOOL EnergyRange::PreTranslateMessage(MSG* pMsg) 
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
// Routine:		OnSelchangeViewdropRange
// Purpose:		Actions taken when a new selection is made from the View drop list
// Arguments:	None
// Returns:		void
// History:		16 Sep 03	T Gremillion	History Started
/////////////////////////////////////////////////////////////////////////////

void EnergyRange::OnSelchangeViewdropRange() 
{
	// local variables
	long View = 0;
	long lld = 0;
	long uld = 0;

	char Str[MAX_STR_LEN];

	// get the new view
	View = m_ViewDropRange.GetCurSel();

	// if it has changed
	if (View != m_NumSelect) {

		// change the view
		m_NumSelect = View;

		// put in the new range
		switch (m_ViewIndex[m_NumSelect]) {

		case DHI_MODE_POSITION_PROFILE:
			lld = RangePP_lld;
			uld = RangePP_uld;
			break;

		case DHI_MODE_CRYSTAL_ENERGY:
			lld = RangeEN_lld;
			uld = RangeEN_uld;
			break;

		case DHI_MODE_TUBE_ENERGY:
			lld = RangeTE_lld;
			uld = RangeTE_uld;
			break;

		case DHI_MODE_SHAPE_DISCRIMINATION:
			lld = RangeSD_lld;
			uld = RangeSD_uld;
			break;

		case DHI_MODE_RUN:
			lld = RangeRun_lld;
			uld = RangeRun_uld;
			break;
		}

		// fill in lld/uld fields
		sprintf(Str, "%d", lld);
		m_LLD.SetWindowText(Str);
		sprintf(Str, "%d", uld);
		m_ULD.SetWindowText(Str);
	}
}
