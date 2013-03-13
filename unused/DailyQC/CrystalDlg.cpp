// CrystalDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DailyQC.h"
#include "CrystalDlg.h"
#include "DailyQCView.h"
#include "ScannerDefine.h"
#include "DailyQCDoc.h"
#include ".\crystaldlg.h"

// CCrystalDlg dialog
#define CRYSTAL_WIDTH 35
#define CRYSTAL_HEIGHT 35

IMPLEMENT_DYNAMIC(CCrystalDlg, CDialog)
CCrystalDlg::CCrystalDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCrystalDlg::IDD, pParent)
{
	Create(CCrystalDlg::IDD, pParent);
	pView = (CDailyQCView*) pParent;
	m_dBlock = pView->m_dSelectedBlock;
	m_dHead = pView->m_dSelectedHead;
	m_HeadSpinCtrl.SetRange(0,7);
	m_BlockSpinCtrl.SetRange(0,116);	
	
	m_pToolTip = new CToolTipCtrl;
	m_pToolTip->Create(this);
	m_pToolTip->Activate(TRUE);
	if (pView->m_dViewResult == VIEW_CRYSTAL)
	{
		RECT rect;
		for (int i = 0; i< CRYSTAL_AXIAL_NUMBER; i++)
			for (int j =0; j<CRYSTAL_TRANSAXIAL_NUMBER; j++)
			{
				rect.top = 20 + i*CRYSTAL_WIDTH;
				rect.left = 20 + j*CRYSTAL_HEIGHT ;
				rect.bottom = rect.top + CRYSTAL_HEIGHT;
				rect.right = rect.left + CRYSTAL_WIDTH;
				m_pToolTip->AddTool(this, LPSTR_TEXTCALLBACK,&rect,IDT_RECTANGLE+i*CRYSTAL_AXIAL_NUMBER+j);
			}
	}
	SetRange(pView->range);
	//return TRUE;  // return TRUE unless you set the focus to a control
}

CCrystalDlg::~CCrystalDlg()
{
	delete m_pToolTip;
}

void CCrystalDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_HEAD_NUMBER, m_dHead);
	DDX_Text(pDX, IDC_BLOCK_NUMBER, m_dBlock);
	DDX_Control(pDX, IDC_HEAD_SPIN, m_HeadSpinCtrl);
	DDX_Control(pDX, IDC_BLOCK_SPIN, m_BlockSpinCtrl);
}


BEGIN_MESSAGE_MAP(CCrystalDlg, CDialog)
	ON_WM_CLOSE()
	ON_EN_CHANGE(IDC_HEAD_NUMBER, OnEnChangeHeadNumber)
	ON_EN_CHANGE(IDC_BLOCK_NUMBER, OnEnChangeBlockNumber)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, ProduceTip)
END_MESSAGE_MAP()


// CCrystalDlg message handlers

void CCrystalDlg::PostNcDestroy()
{
	// TODO: Add your specialized code here and/or call the base class
	CDialog::PostNcDestroy();
	pView->m_dViewDetail = 0;
	pView->m_pCrystalDlg = NULL;
	delete this;
}

void CCrystalDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class
	CDialog::OnOK();
	DestroyWindow();
}

void CCrystalDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnClose();
	DestroyWindow();
}

void CCrystalDlg::OnEnChangeHeadNumber()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	if (pView != NULL)
	{
		UpdateData(TRUE);
		pView->m_dSelectedHead = m_dHead;
		pView->Invalidate(FALSE);
		if (pView->m_dViewResult == VIEW_CRYSTAL)
			Invalidate(FALSE);
		else
			Invalidate();
	}
}

void CCrystalDlg::OnEnChangeBlockNumber()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
	if (pView != NULL)
	{
		UpdateData(TRUE);
		pView->m_dSelectedBlock = m_dBlock;
		pView->Invalidate(FALSE);
		if (pView->m_dViewResult == VIEW_CRYSTAL)
			Invalidate(FALSE);
		else
			Invalidate();
	}
}

int CCrystalDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	// initialize pView so OnEnChangeBlockNumber and
	// OnEnChangeHeadNumber can handle pView when dlg is created
	pView = NULL;
	m_pToolTip = NULL;
	return 0;
}

void CCrystalDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CDialog::OnPaint() for painting messages
	if (pView->m_dViewResult == VIEW_CRYSTAL)
	{
		CPoint offset(20,20);
		pView->DrawGrid(&dc,offset.x,offset.y,CRYSTAL_WIDTH,CRYSTAL_HEIGHT,CRYSTAL_TRANSAXIAL_NUMBER,CRYSTAL_AXIAL_NUMBER, 0);

		CDailyQCDoc* pDoc = pView->GetDocument();
		ASSERT_VALID(pDoc);

		BYTE grey_level;
		double crystal;

		// double crystal_sd = pDoc->BlockSD/(CRYSTAL_AXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER);
		// double crystal_mean = pDoc->BlockMean/(CRYSTAL_AXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER);
		int j = m_dBlock/BLOCK_TRANSAXIAL_NUMBER;
		int i = m_dBlock%BLOCK_TRANSAXIAL_NUMBER;
		// int tmp;
		if (m_dHead == 3)
			grey_level = 0;
		// fill crystal in one block
		for (int a = 0; a<CRYSTAL_AXIAL_NUMBER; a++)
		{
			for (int t = 0; t<CRYSTAL_TRANSAXIAL_NUMBER; t++)
			{
				crystal = pDoc->CrystalCount
					[m_dHead][i*CRYSTAL_TRANSAXIAL_NUMBER+t][j*CRYSTAL_AXIAL_NUMBER+a];
				// tmp = (int) (128+255*(crystal-crystal_mean)/(12*crystal_sd));
				// if (tmp<0) tmp = 0;
				// grey_level = (BYTE)(tmp);
				grey_level = (BYTE) (255 * crystal / pDoc->MaxCrystalCount);
				dc.FillSolidRect(offset.x+t*CRYSTAL_WIDTH+1,
					offset.y+a*CRYSTAL_HEIGHT+1,CRYSTAL_WIDTH-1,CRYSTAL_HEIGHT-1,
					RGB(grey_level,grey_level,grey_level));
			}
		}
	}
	if (pView->m_dViewResult == VIEW_COMPARE)
	{
		if (pView->m_dComp == COMP_BLOCKS)
		{
			RECT area = {75,50,310,260};
			CDailyQCDoc* pDoc = pView->GetDocument();
			ASSERT_VALID(pDoc);
	
			// find data range
			POSITION pos;
			CompareData info;
			PlotData * pData, *itData, *pDecayData, *itDecayData;
			pData = new PlotData[pDoc->CompareList.GetCount()];
			pDecayData = new PlotData[pDoc->CompareList.GetCount()];
			double master_activity;
			int master = -1;
	
			// get master 
			pos = pDoc->CompareList.GetHeadPosition();
			for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
			{
				info = pDoc->CompareList.GetNext(pos);
				if (info.MRQC == "master")
				{
					master = i;
					CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
					master_activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);	
					break;
				}
			}

			//get new range for no compensation
			if (pView->m_bDecay)
			{
				double block_count;
				pos = pDoc->CompareList.GetHeadPosition();
				for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
				{
					info = pDoc->CompareList.GetNext(pos);
					for (int h = 0; h<DETECTOR_HEAD_NUMBER; h++)
						for (int b= 0; b<BLOCK_NUMBER; b++)
						{

							block_count = (info.BlockCount[h][b][0]+info.BlockCount[h][b][1]);
							if (block_count>range.max_y)
								range.max_y = block_count;
							if (block_count<range.min_y)
								range.min_y = block_count;
						}
				}
			}

			// draw eight blocks
			int a = pView->m_dSelectedBlock/(int)BLOCK_TRANSAXIAL_NUMBER;
			for (int t = 0; t <BLOCK_TRANSAXIAL_NUMBER;t++)
			{
				itData = pData;
				itDecayData = pDecayData;
				pos = pDoc->CompareList.GetHeadPosition();
				for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
				{
					info = pDoc->CompareList.GetNext(pos);
					double activity;
					CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
					activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);

					itData->y = info.BlockCount[pView->m_dSelectedHead][a*BLOCK_TRANSAXIAL_NUMBER+t][0]
						+info.BlockCount[pView->m_dSelectedHead][a*BLOCK_TRANSAXIAL_NUMBER+t][1];
					itDecayData->y = itData->y * master_activity / activity;
					itData->xt = info.ScanTime;
					itDecayData->xt = info.ScanTime;
					itDecayData++;
					itData ++;
				}
				if (t==0)
					pView->DrawCurve(&dc,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
										master, 1, "Block Count",CPoint(0, 20),1,PS_SOLID);	
				else
					pView->DrawCurve(&dc,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
										-1, 0, "",CPoint(0, 20),1,PS_SOLID);
				if (pView->m_bDecay)
					pView->DrawCurve(&dc,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
										-1, 0, "",CPoint(0, 20),1,PS_DOT);
			}

			// draw selected block
			itData = pData;
			itDecayData = pDecayData;
			pos = pDoc->CompareList.GetHeadPosition();
			
			for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
			{
				info = pDoc->CompareList.GetNext(pos);
				itData->y = info.BlockCount[pView->m_dSelectedHead][pView->m_dSelectedBlock][0]
					+info.BlockCount[pView->m_dSelectedHead][pView->m_dSelectedBlock][1];
				double activity;
				CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
				activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
				itDecayData->y = itData->y * master_activity / activity;
				itDecayData++;
				itData ++;
			}
			pView->DrawCurve(&dc,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(255,0,0),
									-1, 0, "",CPoint(0, 20), 2,PS_SOLID);	
			if (pView->m_bDecay)
				pView->DrawCurve(&dc,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), RGB(255,0,0),
									-1, 0, "",CPoint(0, 20),1,PS_DOT);
			pView->DrawCoord(&dc, area, range, 0,0,CPoint(-50, 10));
			pView->DrawCoord(&dc, area, range, 1, 4, CPoint(-70, -15));

			delete []pData;
			delete []pDecayData;
		}
	}
}

BOOL CCrystalDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	// move tooltip generation to constructor
	//m_pToolTip = new CToolTipCtrl;
	//m_pToolTip->Create(this);
	//m_pToolTip->Activate(TRUE);
	//RECT rect;
	//for (int i = 0; i< CRYSTAL_AXIAL_NUMBER; i++)
	//	for (int j =0; j<CRYSTAL_TRANSAXIAL_NUMBER; j++)
	//	{
	//		rect.top = 20 + i*CRYSTAL_WIDTH;
	//		rect.left = 20 + j*CRYSTAL_HEIGHT ;
	//		rect.bottom = rect.top + CRYSTAL_HEIGHT;
	//		rect.right = rect.left + CRYSTAL_WIDTH;
	//		m_pToolTip->AddTool(this, LPSTR_TEXTCALLBACK,&rect,IDT_RECTANGLE+i*CRYSTAL_AXIAL_NUMBER+j);
	//	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CCrystalDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_pToolTip!=NULL)
		m_pToolTip->RelayEvent(pMsg);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CCrystalDlg::ProduceTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult)
{
	if (pView == NULL)
		return TRUE;
	if (pView->m_dViewResult == VIEW_CRYSTAL)
	{	

		TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
		int nID =(int)(pNMHDR->idFrom - IDT_RECTANGLE);

		CString string;
		if (nID<64 && nID>=0)
		{
			CDailyQCDoc* pDoc = pView->GetDocument();
			ASSERT_VALID(pDoc);
			double crystal;
			int j = m_dBlock/BLOCK_TRANSAXIAL_NUMBER;
			int i = m_dBlock%BLOCK_TRANSAXIAL_NUMBER;
			int a = nID/CRYSTAL_TRANSAXIAL_NUMBER;
			int t = nID%CRYSTAL_TRANSAXIAL_NUMBER;
			crystal = pDoc->CrystalCount
					[m_dHead][i*CRYSTAL_TRANSAXIAL_NUMBER+t][j*CRYSTAL_AXIAL_NUMBER+a];
			string.Format("%1.0lf",crystal);	
		}
		else
		{
			string = "Not available";	
		}
		::lstrcpy(pTTT->szText,(LPCTSTR)string);
	}
	 return(TRUE);
}

void CCrystalDlg::SetRange(PlotDataRange rangeValue)
{
	range.max_x = rangeValue.max_x;
	range.max_y = rangeValue.max_y;
	range.min_x = rangeValue.min_x;
	range.min_y = rangeValue.min_y;
}
