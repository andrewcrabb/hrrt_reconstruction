// DailyQCView.cpp : implementation of the CDailyQCView class
//
// Authors: Dongming Hu <Dongming.Hu@Siemens.com>
//          Merence Sibomana <Sibomana@gmail.com> (MS)
// Modification History
// 25-OCT-2008: 1.0.40 Bug fix line "DailyQC crashes on view statistics" (MS)

#include "stdafx.h"
#include "DailyQC.h"

#include "DailyQCDoc.h"
#include "DailyQCView.h"
#include "CrystalDlg.h"
#include "MRQCDlg.h"
#include <math.h>
#include ".\dailyqcview.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDailyQCView

IMPLEMENT_DYNCREATE(CDailyQCView, CScrollView)
//IMPLEMENT_DYNCREATE(CDailyQCDoc, CDocument)

BEGIN_MESSAGE_MAP(CDailyQCView, CScrollView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CScrollView::OnFilePrintPreview)
	ON_COMMAND(ID_VIEW_SUMMARY, OnViewSummary)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUMMARY, OnUpdateViewSummary)
	ON_COMMAND(ID_VIEW_STATISTICS, OnViewStatistics)
	ON_UPDATE_COMMAND_UI(ID_VIEW_STATISTICS, OnUpdateViewStatistics)
	ON_COMMAND(ID_VIEW_BLOCKHISTOGRAM, OnViewBlockhistogram)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BLOCKHISTOGRAM, OnUpdateViewBlockhistogram)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_LBUTTONUP()
	ON_COMMAND(ID_FILE_COMPAREWITH, OnFileComparewith)
	ON_COMMAND(ID_FILE_CLEARCOMPARISON, OnFileClearcomparison)
	ON_COMMAND(ID_FILE_EXPORTTEXTFILE, OnFileExporttextfile)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLEARCOMPARISON, OnUpdateFileClearcomparison)
	ON_UPDATE_COMMAND_UI(ID_FILE_EXPORTTEXTFILE, OnUpdateFileExporttextfile)
	ON_NOTIFY_EX( TTN_NEEDTEXT, 0, ProduceViewToolTip)
	ON_COMMAND(ID_BLOCKMAP_FRONTLAYER, OnBlockmapFrontlayer)
	ON_UPDATE_COMMAND_UI(ID_BLOCKMAP_FRONTLAYER, OnUpdateBlockmapFrontlayer)
	ON_COMMAND(ID_BLOCKMAP_BACKLAYER, OnBlockmapBacklayer)
	ON_UPDATE_COMMAND_UI(ID_BLOCKMAP_BACKLAYER, OnUpdateBlockmapBacklayer)
	ON_COMMAND(ID_BLOCKMAP_BOTHLAYERS, OnBlockmapBothlayers)
	ON_UPDATE_COMMAND_UI(ID_BLOCKMAP_BOTHLAYERS, OnUpdateBlockmapBothlayers)
	ON_COMMAND(ID_COMPARISON_GANTRY, OnComparisonGantry)
	ON_UPDATE_COMMAND_UI(ID_COMPARISON_GANTRY, OnUpdateComparisonGantry)
	ON_COMMAND(ID_COMPARISON_HEADS, OnComparisonHeads)
	ON_UPDATE_COMMAND_UI(ID_COMPARISON_HEADS, OnUpdateComparisonHeads)
	ON_COMMAND(ID_COMPARISON_BLOCKS, OnComparisonBlocks)
	ON_UPDATE_COMMAND_UI(ID_COMPARISON_BLOCKS, OnUpdateComparisonBlocks)
	ON_COMMAND(ID_VIEW_CRYSTALBITMAP, OnViewCrystalbitmap)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CRYSTALBITMAP, OnUpdateViewCrystalbitmap)
	ON_COMMAND(ID_VIEW_DETAILS, OnViewDetails)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETAILS, OnUpdateViewDetails)
	ON_COMMAND(ID_VIEW_DECAYCOMPENSATION, OnViewDecaycompensation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DECAYCOMPENSATION, OnUpdateViewDecaycompensation)
	ON_COMMAND(ID_FILE_ADDCOMPARISON, OnFileAddcomparison)
	ON_UPDATE_COMMAND_UI(ID_FILE_ADDCOMPARISON, OnUpdateFileAddcomparison)
END_MESSAGE_MAP()

// CDailyQCView construction/destruction

CDailyQCView::CDailyQCView()
: m_dViewResult(VIEW_SUMMARY)
, m_dLayer(NO_LAYER)
, m_sCompMRQC(_T(""))
{
	// TODO: add construction code here

	// select Fixedsys 10 as font for display and printing
	// get DC of entire screen area
	CWindowDC dc(NULL);
	// longfont structure define a font 
	LOGFONT	 m_lf;	
	// set all to zero
	memset (&m_lf, 0, sizeof(LOGFONT));
	lstrcpy (m_lf.lfFaceName, _T("Fixedsys"));	   // Default font
	// for MM_TEXT mapping use -MulDiv(PointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
	m_lf.lfHeight = -MulDiv(10, dc.GetDeviceCaps(LOGPIXELSY), 72);
	// normal weight  
	m_lf.lfWeight = FW_NORMAL;
	m_lf.lfOutPrecision = OUT_CHARACTER_PRECIS;
	m_lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	m_lf.lfQuality = DRAFT_QUALITY;
	m_lf.lfPitchAndFamily = FIXED_PITCH|FF_MODERN;
	
	// setup font
	m_pFont = new CFont ();
	m_pFont->CreateFontIndirect(&m_lf);	
	block_size = PIXEL_SIZE*6+1;
	// m_sizeOldViewSize = CSize(0,0);
	// m_sizeNewViewSize = CSize(0,0);
	
	m_dViewDetail = 0;

	m_pCrystalDlg = NULL;
	m_dSelectedHead = -1;;
	m_dSelectedBlock = -1;
	m_pViewToolTip = NULL;
	m_dComp = COMP_NO;
	m_bDecay = FALSE;
}

CDailyQCView::~CDailyQCView()
{
	m_pFont->DeleteObject();
	if (m_pViewToolTip == NULL)
		delete m_pViewToolTip;
	delete m_pFont;
}

BOOL CDailyQCView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CScrollView::PreCreateWindow(cs);
}

// CDailyQCView drawing

void CDailyQCView::OnDraw(CDC* pDC)
{

	// TODO: add draw code for native data here
	// Select current font into DC.
	BOOL isprinting = pDC->IsPrinting();
	if (isprinting)
	{
		m_pOldFont = pDC->SelectObject(m_pPrinterFont);
	}
	else
	{
		m_pOldFont = pDC->SelectObject(m_pFont);
	}
	switch (m_dViewResult)
	{
		case VIEW_BLOCKHIST:
		case VIEW_SUMMARY:
			OutTextString(pDC);
			break;
		case VIEW_CRYSTAL:
			CrystalMap(pDC);
			break;
		case VIEW_STATISTICS:
			DrawStatistics(pDC);
			break;
		case VIEW_BLOCKMAP:
			BlockMap(pDC);
			break;
		case VIEW_COMPARE:
			DrawCompare(pDC);
			break;
		default:
			break;
	}
	pDC->SelectObject(m_pOldFont);
}

void CDailyQCView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();
	CSize sizeTotal;
	// TODO: calculate the total size of this view
	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);

	CClientDC	vdc(this);
	vdc.SelectObject(m_pFont);
	vdc.GetTextMetrics(&tm);
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
}


// CDailyQCView printing

BOOL CDailyQCView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CDailyQCView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	// TODO: add extra initialization before printing
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// First create a printer font
	LOGFONT	lf;
	m_pFont->GetLogFont(&lf);
	m_pPrinterFont = new CFont ();
	CSize size;

	m_pPrinterFont->CreateFontIndirect(&lf);	 
	m_pOldFont = pDC->SelectObject(m_pPrinterFont);
	pDC->GetTextMetrics(&tm_printer);

	// set display mode

	if (m_dViewResult == VIEW_SUMMARY ||
		m_dViewResult == VIEW_BLOCKHIST)
	{
		pDC->SetMapMode(MM_LOENGLISH);
		// calculate left margin
		size = pDC->GetTextExtent(
			_T("=========================================================================================="));
		pDC->LPtoDP(&size);
		size.cx = (pDC->GetDeviceCaps(HORZRES)-size.cx)/2;
		pDC->DPtoLP(&size);
		m_dLeftMargin = size.cx;
		
		// lines per page
		size.cy = pDC->GetDeviceCaps(VERTRES);
		pDC->DPtoLP(&size);
		m_dLinesPerPage = size.cy/(tm_printer.tmHeight + tm_printer.tmExternalLeading)-4;
		
		// calculate how many pages 
		int index;
		if (m_dViewResult == VIEW_SUMMARY)
			index = 0;
		else
			index = 1;

		m_dPrintPages = max(1, ((int)pDoc->m_LineList[index].GetCount()+m_dLinesPerPage-1)/m_dLinesPerPage);
		pInfo->SetMaxPage(m_dPrintPages);
	}
	else
	{
		pDC->SetMapMode(MM_LOENGLISH);
		if (m_dViewResult == VIEW_COMPARE && m_dComp == COMP_BLOCKS)
		{
			size.cy = pDC->GetDeviceCaps(VERTRES);
			pDC->DPtoLP(&size);
			m_dLinesPerPage = (size.cy-80)/192;
			m_dPrintPages = max(1, 2*((BLOCK_AXIAL_NUMBER+m_dLinesPerPage-1)/m_dLinesPerPage));
			pInfo->SetMaxPage(m_dPrintPages);
		}
		else
			pInfo->SetMaxPage(1);
	}
	pDC->SelectObject(m_pOldFont);

}

void CDailyQCView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo)
{
	// TODO: add cleanup after printing
	// delete the new font
	m_pPrinterFont->DeleteObject();
}


// CDailyQCView diagnostics

#ifdef _DEBUG
void CDailyQCView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CDailyQCView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CDailyQCDoc* CDailyQCView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CDailyQCDoc)));
	return (CDailyQCDoc*)m_pDocument;
}
#endif //_DEBUG


// CDailyQCView message handlers
// Draw crystal map
void CDailyQCView::CrystalMap(CDC* pDC)
{
	BOOL isprinting = pDC->IsPrinting();
	CString tmpstr;
	CMainFrame * pFrame = (CMainFrame *)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));

	if (isprinting)
	{
		CSize DevSize;
		float printer_scale;
		DevSize.cx = pDC->GetDeviceCaps(HORZRES);
		DevSize.cy = pDC->GetDeviceCaps(VERTRES);
		pDC->DPtoLP(&DevSize);
		printer_scale = min((float)DevSize.cx/m_sizeTotalArea.cx,(float)DevSize.cy/m_sizeTotalArea.cy);

		pDC->TextOut((int)(printer_scale*50),(int)(-printer_scale*50), "HRRT Daily QC Crystal Map");
		int org_x = block_size*BLOCK_TRANSAXIAL_NUMBER;
		int org_y = block_size*BLOCK_AXIAL_NUMBER;
		for (int i =0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			// first, draw block grid
			DrawGrid(pDC, (int)(printer_scale*((i%4)*org_x+20)),
				(int)(-printer_scale*(100+ (i/4)*(org_y+100))), 
				(int)(printer_scale*block_size), (int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// Fill crystal here
			FillCrystal(pDC, (int)(printer_scale*((i%4)*org_x+20)),
				(int)(-printer_scale*(100+ (i/4)*(org_y+100))), 
				(int)(printer_scale*block_size), (int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// draw border of head
			DrawBorder(pDC, (int)(printer_scale*((i%4)*org_x+20)),
				(int)(-printer_scale*(100+ (i/4)*(org_y+100))), 
				(int)(printer_scale*block_size), (int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER);

			tmpstr.Format("Head %d", i);
			pDC->SelectObject(m_pFont);
			//reset color for text
			// Select text color.
			pDC->SetTextColor(pFrame->m_crForeground);
			// Select text background color.
			pDC->SetBkColor(pFrame->m_crBackground);
			pDC->TextOut((int)(printer_scale*((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+block_size*4)),
				(int)(-printer_scale*(block_size*BLOCK_AXIAL_NUMBER+120+(i/4)*(block_size*BLOCK_AXIAL_NUMBER+100))),
				tmpstr);
		}
	}
	else
	{
		CDC dcMem;
		CBitmap bitmap, *pOldBitMap;
		bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx,m_sizeTotalArea.cy);
		dcMem.CreateCompatibleDC(pDC);
		pOldBitMap = dcMem.SelectObject(&bitmap);

		dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
		dcMem.TextOut(50,50, "HRRT Daily QC Crystal Map");
		int org_x = block_size*BLOCK_TRANSAXIAL_NUMBER;
		int org_y = block_size*BLOCK_AXIAL_NUMBER;
		for (int i =0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			// first, draw block grid
			DrawGrid(&dcMem, (i%4)*org_x+20,
				100+ (i/4)*(org_y+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// Fill crystal here
			FillCrystal(&dcMem, (i%4)*org_x+20,
				100+ (i/4)*(org_y+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// draw border of head
			DrawBorder(&dcMem, (i%4)*org_x+20,
				100+ (i/4)*(org_y+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER);
			// finally draw selected block edge
			if (i == m_dSelectedHead)
			{
				int x, y;
				x = m_dSelectedBlock%BLOCK_TRANSAXIAL_NUMBER;
				y = m_dSelectedBlock/BLOCK_TRANSAXIAL_NUMBER;
				CPen pen;
				RECT rect;
				rect.left = (i%4)*org_x+20+x*block_size;
				rect.top = 100+ (i/4)*(org_y+100)+y*block_size;
				rect.bottom = rect.top+block_size;
				rect.right = rect.left+block_size;
				pen.CreatePen(PS_SOLID, 3, RGB(255,255,0)); 
				CPen* pOldPen = dcMem.SelectObject(&pen);
				dcMem.MoveTo(rect.left,rect.top);
				dcMem.LineTo(rect.right,rect.top);
				dcMem.LineTo(rect.right,rect.bottom);
				dcMem.LineTo(rect.left,rect.bottom);
				dcMem.LineTo(rect.left,rect.top);
				dcMem.SelectObject(pOldPen);
			}
			tmpstr.Format("Head %d", i);
			dcMem.SelectObject(m_pFont);
			//reset color for text
			// Select text color.
			dcMem.SetTextColor(pFrame->m_crForeground);
			// Select text background color.
			dcMem.SetBkColor(pFrame->m_crBackground);
			dcMem.TextOut((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+block_size*4,
				block_size*BLOCK_AXIAL_NUMBER+120+(i/4)*(block_size*BLOCK_AXIAL_NUMBER+100),
				tmpstr);
		}
		pDC->BitBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy, &dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBitMap);
		bitmap.DeleteObject();
	}
}

// draw statistical analysis result
void CDailyQCView::DrawStatistics(CDC* pDC)
{
	BOOL isprinting = pDC->IsPrinting();
	CPen pen;
	pen.CreatePen(PS_SOLID, 1, RGB(255,0,0));
	CString tmpstr;
	CPen* pOldPen;
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (isprinting)
	{
		CSize DevSize;
		float printer_scale;
		DevSize.cx = pDC->GetDeviceCaps(HORZRES);
		DevSize.cy = pDC->GetDeviceCaps(VERTRES);
		pDC->DPtoLP(&DevSize);
		printer_scale = min((float)DevSize.cx/m_sizeTotalArea.cx,(float)DevSize.cy/m_sizeTotalArea.cy);
		pOldPen = pDC->SelectObject( &pen );
		pDC->TextOut((int)(50*printer_scale),(int)(-printer_scale*50), "HRRT Daily QC Block Statistics");
		tmpstr.Format("%Mean = %1.0lf",pDoc->BlockMean);
		pDC->TextOut((int)(50*printer_scale),(int)(-printer_scale*70),tmpstr);
		tmpstr.Format("Standard Deviation = %1.0lf",pDoc->BlockSD);
		pDC->TextOut((int)(50*printer_scale),(int)(-printer_scale*90),tmpstr);
		pDC->MoveTo((int)(printer_scale*20),(int)(-printer_scale*450));
		pDC->LineTo((int)(printer_scale*1000),(int)(-printer_scale*450));
		DrawNCurve(pDC, pDoc->BlockMean,pDoc->BlockSD,printer_scale*90/pDoc->BlockSD,
			-printer_scale*250*pDoc->BlockSD*sqrt(2*3.14),
			pDoc->BlockMean-3*pDoc->BlockSD, 5, CPoint((int)(printer_scale*200),(int)(-printer_scale*450)));
		DrawBlockHist(pDC,printer_scale*90/pDoc->BlockSD,-printer_scale*250,
			pDoc->BlockMean-3*pDoc->BlockSD, 2, CPoint((int)(printer_scale*200),(int)(-printer_scale*450)), pDoc);
		pDC->SelectObject(pOldPen);
	}
	else
	{
		CDC dcMem;
		CBitmap bitmap, *pOldBitMap;
		bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx, m_sizeTotalArea.cy);
		dcMem.CreateCompatibleDC(pDC);
		dcMem.SetMapMode(pDC->GetMapMode());
		pOldBitMap = dcMem.SelectObject(&bitmap);
		pOldPen = dcMem.SelectObject( &pen );
		dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
		dcMem.TextOut(50,50, "HRRT Daily QC Block Statistics");
		
		tmpstr.Format("Mean = %1.0lf",pDoc->BlockMean);
		dcMem.TextOut(50,70,tmpstr);
		tmpstr.Format("Standard Deviation = %1.0lf",pDoc->BlockSD);
		dcMem.TextOut(50,90,tmpstr);

		dcMem.MoveTo(20,450);
		dcMem.LineTo(1000,450);
		//dcMem.MoveTo(50,450);
		//dcMem.LineTo(50,150);
		DrawNCurve(&dcMem, pDoc->BlockMean,pDoc->BlockSD,90/pDoc->BlockSD,250*pDoc->BlockSD*sqrt(2*3.14),
			pDoc->BlockMean-3*pDoc->BlockSD, 5, CPoint(200,450));
		DrawBlockHist(&dcMem,90/pDoc->BlockSD,250,pDoc->BlockMean-3*pDoc->BlockSD, 2, CPoint(200,450), pDoc);
		pDC->BitBlt(0,0,m_sizeTotalArea.cx, m_sizeTotalArea.cy, &dcMem, 0, 0, SRCCOPY);
		dcMem.SelectObject(pOldBitMap);
		dcMem.SelectObject(pOldPen);
		bitmap.DeleteObject();
		return;
	}
}

// display block map
void CDailyQCView::BlockMap(CDC* pDC)
{

	BOOL isprinting = pDC->IsPrinting();
	CString tmpstr;
	CMainFrame * pFrame = (CMainFrame *)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));

	if (isprinting)
	{
		CSize DevSize;
		float printer_scale;
		DevSize.cx = pDC->GetDeviceCaps(HORZRES);
		DevSize.cy = pDC->GetDeviceCaps(VERTRES);
		pDC->DPtoLP(&DevSize);
		printer_scale = min((float)DevSize.cx/m_sizeTotalArea.cx,(float)DevSize.cy/m_sizeTotalArea.cy);

		switch (m_dLayer)
		{
		case FRONT_LAYER:
			pDC->TextOut((int)(printer_scale*50),(int)(-printer_scale*50), "HRRT Daily QC Block Map - Front Layer");
			break;
		case BACK_LAYER:
			pDC->TextOut((int)(printer_scale*50),(int)(-printer_scale*50), "HRRT Daily QC Block Map - Back Layer");
			break;
		case BOTH_LAYER:
			pDC->TextOut((int)(printer_scale*50),(int)(-printer_scale*50), "HRRT Daily QC Block Map - Both Layers");
			break;
		}
		for (int i =0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			// first, draw block grid
			DrawGrid(pDC, (int)(printer_scale*((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20)),
				(int)(-printer_scale*(100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100))), 
				(int)(printer_scale*block_size),(int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER, i);
			// Fill block here
			FillBlock(pDC, (int)(printer_scale*((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20)),
				(int)(-printer_scale*(100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100))), 
				(int)(printer_scale*block_size),(int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// draw border of head
			DrawBorder(pDC, (int)(printer_scale*((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20)),
				(int)(-printer_scale*(100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100))), 
				(int)(printer_scale*block_size), (int)(-printer_scale*block_size),
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER);
			tmpstr.Format("Head %d", i);
			pDC->SelectObject(m_pFont);
			//reset color for text
			// Select text color.
			pDC->SetTextColor(pFrame->m_crForeground);
			// Select text background color.
			pDC->SetBkColor(pFrame->m_crBackground);
			pDC->TextOut((int)(printer_scale*((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+block_size*4)),
				(int)(-printer_scale*(block_size*BLOCK_AXIAL_NUMBER+120+(i/4)*(block_size*BLOCK_AXIAL_NUMBER+100))),
				tmpstr);
		}
	}
	else
	{
		CDC dcMem;
		CBitmap bitmap, *pOldBitMap;
		bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx,m_sizeTotalArea.cy);
		dcMem.CreateCompatibleDC(pDC);
		pOldBitMap = dcMem.SelectObject(&bitmap);
		dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
		switch (m_dLayer)
		{
		case FRONT_LAYER:
			dcMem.TextOut(50,50, "HRRT Daily QC Block Map - Front Layer");
			break;
		case BACK_LAYER:
			dcMem.TextOut(50,50, "HRRT Daily QC Block Map - Back Layer");
			break;
		case BOTH_LAYER:
			dcMem.TextOut(50,50, "HRRT Daily QC Block Map - Both Layers");
			break;
		}
		
		if (m_pViewToolTip != NULL)
			delete m_pViewToolTip;

		m_pViewToolTip = new CToolTipCtrl;
		m_pViewToolTip->Create(this);
		m_pViewToolTip->Activate(TRUE);
		
		for (int i =0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			// first, draw block grid
			DrawGrid(&dcMem, (i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20,
				100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			CreateBlockToolTips((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20,
				100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// Fill block here
			FillBlock(&dcMem, (i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20,
				100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER,i);
			// draw border of head
			DrawBorder(&dcMem, (i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+20,
				100+ (i/4)*(block_size*BLOCK_AXIAL_NUMBER+100), block_size, block_size,
				BLOCK_TRANSAXIAL_NUMBER, BLOCK_AXIAL_NUMBER);
			tmpstr.Format("Head %d", i);
			dcMem.SelectObject(m_pFont);
			//reset color for text
			// Select text color.
			dcMem.SetTextColor(pFrame->m_crForeground);
			// Select text background color.
			dcMem.SetBkColor(pFrame->m_crBackground);
			dcMem.TextOut((i%4)*block_size*BLOCK_TRANSAXIAL_NUMBER+block_size*4,
				block_size*BLOCK_AXIAL_NUMBER+120+(i/4)*(block_size*BLOCK_AXIAL_NUMBER+100),
				tmpstr);
		}
		pDC->BitBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy, &dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBitMap);
		bitmap.DeleteObject();	
	}
}

void CDailyQCView::OnViewBlockhistogram()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_BLOCKHIST;
	//GetDocument()->GenerateTextList(m_dViewResult);
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}
	Invalidate();
}	

void CDailyQCView::OnUpdateViewBlockhistogram(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_BLOCKHIST)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CDailyQCView::OnViewSummary()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_SUMMARY;
	//GetDocument()->GenerateTextList(m_dViewResult);
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}
	Invalidate();
}

void CDailyQCView::OnUpdateViewSummary(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_SUMMARY)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CDailyQCView::OnViewStatistics()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_STATISTICS;
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}
	Invalidate();
}

void CDailyQCView::OnUpdateViewStatistics(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_STATISTICS)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

// draw block grid for block map and crystal map
// at org_x, org_y, position
void CDailyQCView::DrawGrid(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number, int head)
{
	CPen penGrid;

	if( !penGrid.CreatePen(PS_SOLID, 1, RGB(0,255,0))) 
		return ;

	CPen* pOldPen = pDC->SelectObject( &penGrid );

	pDC->MoveTo(org_x,org_y);
	for( int i= 0; i < x_number+1; i++ )
	{
		pDC->LineTo(i*offset_x+org_x, offset_y*y_number+org_y);
		pDC->MoveTo((i+1)*offset_x+org_x,org_y);
	}

	pDC->MoveTo(org_x,org_y);
	for( int i= 0; i < y_number+1; i++ )
	{
		pDC->LineTo(offset_x*x_number+org_x, i*offset_y+org_y);
		pDC->MoveTo(org_x,(i+1)*offset_y+org_y);
	}
}


void CDailyQCView::DrawBorder(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number)
{
	CPen penBoarder;
	if( !penBoarder.CreatePen(PS_SOLID, 1, RGB(255,0,0))) 
		return ;
	// draw the boarder
	CPen* pOldPen = pDC->SelectObject(&penBoarder);
	pDC->MoveTo(org_x,org_y);
	pDC->LineTo(org_x, offset_y*y_number+org_y);
	pDC->LineTo(offset_x*x_number+org_x, 
		offset_y*y_number+org_y);
	pDC->LineTo(offset_x*x_number+org_x,org_y);
	pDC->LineTo(org_x,org_y);
	pDC->SelectObject( pOldPen );
}

void CDailyQCView::FillBlock(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number, int head)
{
	int block = 0;
	BYTE grey_level;
	RECT rect;
	int dir;
	double block_count;

	BOOL isprinting = pDC->IsPrinting();
	if (isprinting)
		dir = -1;
	else
		dir = 1;

	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	for (int j = 0; j < y_number; j++)
	{
		rect.top = org_y+j*offset_y+dir*1;
		rect.bottom = rect.top+offset_y-dir*1;
		for( int i= 0; i < x_number; i++ )
		{
			rect.left = org_x+i*offset_x+1;
			rect.right = rect.left+offset_x-1;
			switch (m_dLayer)
			{
			case FRONT_LAYER:
			case BACK_LAYER:
				block_count = pDoc->BlockCount[head][block][m_dLayer];
				break;
			case BOTH_LAYER:
				block_count = 0;
				for (int k = 0; k<DOI_NUMBER; k++)
					block_count = block_count + pDoc->BlockCount[head][block][k];
				break;
			}

			//grey_level = (BYTE)(127 + (block_count-pDoc->BlockMean)/
			//	(12*pDoc->BlockSD)*255);
			grey_level = (BYTE) (block_count * 255 / pDoc->MaxBlockCount[m_dLayer]);
			pDC->FillSolidRect(&rect,RGB(grey_level,grey_level,grey_level));

			block++;
		}
	}
}

void CDailyQCView::FillCrystal(CDC *pDC, int org_x, int org_y, int block_x, int block_y,
							int x_number, int y_number, int head)
{
	int block = 0;
	RECT rect;
	BYTE grey_level;
	double crystal;
	int dir;
	BOOL isprinting = pDC->IsPrinting();
	if (isprinting)
		dir = -1;
	else
		dir = 1;

	int crystal_x = (block_x-1)/CRYSTAL_TRANSAXIAL_NUMBER;
	int crystal_y = (block_y-dir*1)/CRYSTAL_AXIAL_NUMBER;
		
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// double crystal_sd = pDoc->BlockSD/(CRYSTAL_AXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER);
	// double crystal_mean = pDoc->BlockMean/(CRYSTAL_AXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER);
	// int tmp;
	for (int j = 0; j < y_number; j++)
	{
		rect.top = org_y+j*block_y+dir*1;
		rect.bottom = rect.top+block_y-dir*1;
		for( int i= 0; i < x_number; i++ )
		{
			rect.left = org_x+i*block_x+1;
			rect.right = rect.left+block_x-1;
			// fill crystal in one block
			for (int a = 0; a<CRYSTAL_AXIAL_NUMBER; a++)
			{
				for (int t = 0; t<CRYSTAL_TRANSAXIAL_NUMBER; t++)
				{
					crystal = pDoc->CrystalCount
						[head][i*CRYSTAL_TRANSAXIAL_NUMBER+t][j*CRYSTAL_AXIAL_NUMBER+a];
					// tmp = (int)(128+255*(crystal-crystal_mean)/(12*crystal_sd));
					// if (tmp<0) tmp = 0;
                    // grey_level = (BYTE)(tmp);
					grey_level = (BYTE) (255 * crystal / pDoc->MaxCrystalCount);
					pDC->FillSolidRect(rect.left+t*crystal_x,
						rect.top+a*crystal_y,crystal_x,crystal_y,
						RGB(grey_level,grey_level,grey_level));
				}
			}
			block++;
		}
	}
}
// when this function is called
// m_dViewResult == VIEW_SUMMARY ||
// m_dViewResult == VIEW_BLOCKHIST
int CDailyQCView::OutTextString(CDC* pDC)
{

	BOOL isprinting = pDC->IsPrinting();
	int index;
	if (m_dViewResult == VIEW_SUMMARY)
		index = 0;
	else 
		index = 1;

	CMainFrame * pFrame = (CMainFrame *)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrame->IsKindOf(RUNTIME_CLASS(CMainFrame)));
	
	// Select text color.
	pDC->SetTextColor(pFrame->m_crForeground);
	// Select text background color.
	pDC->SetBkColor(pFrame->m_crBackground);

	// get string list for display
	CStringList *pLineList = &(GetDocument()->m_LineList[index]);
	// empty list -> return
	if (pLineList == NULL)
		return 1;

	int cyLine = 0;
	if (isprinting)
	{
		cyLine = -2 * tm_printer.tmHeight + tm_printer.tmExternalLeading;
	}
	else
	{
		m_dStartLine = 0;
		m_dLastLine =  (int) pLineList->GetCount();   
		m_dLeftMargin = tm.tmAveCharWidth*6;
	}
		
	m_dLastLine = min((int)m_dLastLine, (int) pLineList->GetCount());
	// Loop through client area coordinates to draw.
	
	CString		strLine;
	POSITION	pos;
	// for display only
	while (m_dStartLine < m_dLastLine)
	{
		// Draw a line of text.
		if((pos = pLineList->FindIndex(m_dStartLine)) != NULL)
		{
			strLine = pLineList->GetAt(pos);
			pDC->TextOut(m_dLeftMargin, cyLine, strLine);
			if (isprinting)
				cyLine -= tm_printer.tmHeight + tm_printer.tmExternalLeading;
			else
                cyLine += tm.tmHeight + tm.tmExternalLeading;
			m_dStartLine++;
		}
	}		
	return 0;
}

void CDailyQCView::OnSize(UINT nType, int cx, int cy)
{
	CScrollView::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	// CalculateScrollSize();
	// SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	// Invalidate();
	// m_sizeOldViewSize = m_sizeNewViewSize;
	// m_sizeNewViewSize = CSize(cx, cy);
}

// calculate scroll size

void CDailyQCView::CalculateScrollSize(void)
{
	int index = 1;
	CStringList *pLineList;
	CRect rect;
	GetClientRect(&rect);
	
	// page size
	m_sizePage.cx = rect.right - rect.left;
	m_sizePage.cy = rect.bottom - rect.top;

	switch (m_dViewResult)
	{
	case VIEW_SUMMARY:
		index = 0;
	case VIEW_BLOCKHIST:
		m_sizeTotalArea.cx = 100*tm.tmAveCharWidth;
		pLineList = &(GetDocument()->m_LineList[index]);
		m_sizeTotalArea.cy = (LONG)pLineList->GetCount() * (tm.tmHeight + tm.tmExternalLeading);	
		m_sizeLine.cx = tm.tmAveCharWidth;
		m_sizeLine.cy = tm.tmHeight + tm.tmExternalLeading;
		break;
	case VIEW_CRYSTAL:
		//break;
	case VIEW_BLOCKMAP:
		m_sizeTotalArea.cx = DETECTOR_HEAD_NUMBER/2*block_size*BLOCK_TRANSAXIAL_NUMBER+50;
		m_sizeTotalArea.cy = 2*block_size*BLOCK_AXIAL_NUMBER+250;
		m_sizeLine.cx = block_size;
		m_sizeLine.cy = block_size;
		break;
	case VIEW_STATISTICS:
		// 1024 * 480 drawing area
		m_sizeTotalArea.cx = 1024;
		m_sizeTotalArea.cy = 480;
		m_sizeLine.cx = 20;
		m_sizeLine.cy = 20;
		break;
	case VIEW_COMPARE:
		// 800 * 480 drawing area
		if (m_dComp!=COMP_BLOCKS)
		{
			m_sizeTotalArea.cx = 640;
			m_sizeTotalArea.cy = 480;
		}
		else
		{
			m_sizeTotalArea.cx = 180*DETECTOR_HEAD_NUMBER+100;
			m_sizeTotalArea.cy = 192*BLOCK_AXIAL_NUMBER+100;
		}
		m_sizeLine.cx = 20;
		m_sizeLine.cy = 20;
		break;
	}
}

BOOL CDailyQCView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default
	// return TRUE;
	return CScrollView::OnEraseBkgnd(pDC);
}

void CDailyQCView::OnDestroy()
{
	CScrollView::OnDestroy();

	// TODO: Add your message handler code here
	if (m_pCrystalDlg!=NULL)
		m_pCrystalDlg->DestroyWindow();
}

void CDailyQCView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	CPoint click_pt;
	CPoint pt;
	if (m_dViewResult == VIEW_CRYSTAL)
	{
		click_pt = GetScrollPosition();
		click_pt +=point;
		int org_x = block_size*BLOCK_TRANSAXIAL_NUMBER;
		int org_y = block_size*BLOCK_AXIAL_NUMBER;

		for (int i =0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			pt = CPoint((i%4)*org_x+20,100+ (i/4)*(org_y+100));
			pt = click_pt-pt;
			if ((pt.x>=0)&&(pt.x<=block_size*BLOCK_TRANSAXIAL_NUMBER))
			{
				if ((pt.y>=0)&&(pt.y<=block_size*BLOCK_AXIAL_NUMBER))
				{
					m_dSelectedHead = i;
					m_dSelectedBlock = pt.x/block_size+pt.y/block_size*BLOCK_TRANSAXIAL_NUMBER;
					Invalidate(FALSE);
					if (m_pCrystalDlg != NULL)
					{
						m_pCrystalDlg->m_dBlock = m_dSelectedBlock;
						m_pCrystalDlg->m_dHead = m_dSelectedHead;
						m_pCrystalDlg->UpdateData(FALSE);
						m_pCrystalDlg->Invalidate(FALSE);
					}
					break;
				}
			}
		}
		
	}
	CScrollView::OnLButtonUp(nFlags, point);
}

void CDailyQCView::DrawNCurve(CDC *pDC, double mean, double sd, double x_scale, double y_scale, double x_offset, 
							  int pixel_step, CPoint org)
{
	double start, end, it;
	double p;
	int x, y;
	CPen pen;
	pen.CreatePen(PS_SOLID, 1, RGB(255,0,0));
	CPen* pOldPen = pDC->SelectObject( &pen );

	start = (mean - 3*sd - x_offset);
	end = (mean +3*sd - x_offset);
	it = start;
	mean = mean -x_offset;
	p = exp(-(it-mean)*(it-mean)/(2*sd*sd))/(sd*sqrt(2*sd*3.1415926));
	x = org.x+(int)(it*x_scale);
	y = org.y-(int)(p*y_scale);
	pDC->MoveTo(x, y);
	while (it <=end)
	{
		it += pixel_step/x_scale;
		p = exp(-(it-mean)*(it-mean)/(2*sd*sd))/(sd*sqrt(2*3.1415926));
		x = org.x+(int)(it*x_scale);
		y = org.y-(int)(p*y_scale);
		pDC->LineTo(x, y);
	}
	pDC->SelectObject(pOldPen);

}

double CDailyQCView::DrawBlockHist(CDC *pDC,double x_scale, double y_scale, double x_offset,
								 int pixel_step, CPoint org, CDailyQCDoc* pDoc)
{
	int hist[2048];		// no screen is bigger than 2048 pixel
						// mean value is at 1024
	double block_count;

	BOOL isprinting = pDC->IsPrinting();
	int y_off;
	if (isprinting)
		y_off = -1;
	else
		y_off = 1;
	int mean_pos;
	CPen pen;
	pen.CreatePen(PS_SOLID, pixel_step, RGB(0,255,255));
	CPen* pOldPen = pDC->SelectObject( &pen );
	int max = 0;

	mean_pos = (int) ((pDoc->BlockMean-x_offset)*x_scale)+org.x;
	for (int i =0; i<2048;i++)
		hist[i] = 0;

	
	// generate histogram array
	for (int i =0;i<DETECTOR_HEAD_NUMBER; i++)
	{
		for (int j =0; j<BLOCK_NUMBER;j++)
		{
			block_count = 0;
			for (int k = 0; k<DOI_NUMBER; k++)
				block_count = block_count + pDoc->BlockCount[i][j][k];
			hist[((int)((block_count-pDoc->BlockMean)*x_scale))/pixel_step+1024]++;
			if (hist[((int)((block_count-pDoc->BlockMean)*x_scale))/pixel_step+1024]>max)
				max = hist[((int)((block_count-pDoc->BlockMean)*x_scale))/pixel_step+1024];
		}
	}

	//  y_scale it stands for the height of the drawing
	// Then need to calculate y_scale again
	y_scale = y_scale/max;

	// draw right half histogram
	for (int i = 1024; i<2048;i++)
	{
		if (hist[i]>0)
		{
			pDC->MoveTo(mean_pos+(i-1024)*pixel_step,org.y-y_off);
			pDC->LineTo(mean_pos+(i-1024)*pixel_step,(int)(org.y-hist[i]*y_scale));
		}
	}
	// draw left half histogram
	for (int i = 1024; i>=0;i--)
	{
		if (hist[i]>0)
		{
			pDC->MoveTo(mean_pos+(i-1024)*pixel_step,org.y-y_off);
			pDC->LineTo(mean_pos+(i-1024)*pixel_step,(int)(org.y-hist[i]*y_scale));
		}
	}
	pDC->SelectObject(pOldPen);
	return y_scale;
}
void CDailyQCView::OnFileComparewith()
{
	// TODO: Add your command handler code here

	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// first confirm the master record
	CompareData QCData;
	CMRQCDlg mrqcdlg;

	CString old_MRQC = m_sCompMRQC;

	if (m_sCompMRQC =="")
		mrqcdlg.m_sFileName = pDoc->MRQC;
	else
		mrqcdlg.m_sFileName = m_sCompMRQC;

	if (mrqcdlg.DoModal() == IDOK)
	{
		m_sCompMRQC = mrqcdlg.m_sFileName;
		TRY
		{
			// load master record here
			CFile masterFile(m_sCompMRQC,CFile::modeRead);
			CArchive ar(&masterFile,CArchive::load);
			CRuntimeClass *pDocClass  = RUNTIME_CLASS(CDailyQCDoc);
			CDailyQCDoc *pMasterDoc = (CDailyQCDoc *)pDocClass->CreateObject();
			pMasterDoc->Serialize(ar);
			LoadCompareData(pMasterDoc, &QCData);
			QCData.MRQC = "master";
		}
		CATCH ( CFileException, pEx )
		{
			AfxMessageBox("Master Record QC can not be located!");
			m_sCompMRQC = old_MRQC;
			return;
		}
		END_CATCH
	}
	AddToCompareList(&QCData);
}

void CDailyQCView::OnFileClearcomparison()
{
	// TODO: Add your command handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	pDoc->CompareList.RemoveAll();
	if (m_dViewResult == VIEW_COMPARE)
	{
		m_dViewResult = VIEW_SUMMARY;
		CalculateScrollSize();
		SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
		Invalidate();
	}
}

void CDailyQCView::OnFileExporttextfile()
{
	// TODO: Add your command handler code here
    CStdioFile textFile;
	CString filename;
	CString strtmp;

	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	
	switch (m_dViewResult)
	{
		case VIEW_SUMMARY:
			{
				filename =  pDoc->GetPathName() + ".summary.txt";
				CFileDialog dlg(FALSE,"txt",filename,OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY,"Text File(*.txt)|*.txt||");
				if (dlg.DoModal()==IDOK)
				{
					filename = dlg.GetPathName(); 
					textFile.Open(filename,CFile::modeCreate|CFile::modeWrite|CFile::typeText);
					POSITION pos;
					pos = pDoc->m_LineList[0].GetHeadPosition();
					while (pos!=NULL)
					{
						strtmp = pDoc->m_LineList[0].GetNext(pos);
						strtmp += "\n";
						textFile.WriteString(strtmp);
					}
					textFile.Close();		
				}
			}
			break;
		case VIEW_BLOCKMAP:
			{
				filename =  pDoc->GetPathName() + ".blkmap.txt";
				CFileDialog dlg(FALSE,"txt",filename,OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY,"Text File(*.txt)|*.txt||");
				if (dlg.DoModal()==IDOK)
				{
					filename = dlg.GetPathName(); 
					textFile.Open(filename,CFile::modeCreate|CFile::modeWrite|CFile::typeText);
					textFile.WriteString("Head, Block, BothLayer, FrontLayer, BackLayer, Ratio\n");
					for (int i = 0; i<DETECTOR_HEAD_NUMBER;i++)
						for (int j = 0; j<BLOCK_NUMBER; j++)
						{
							strtmp.Format("%d %d %1.0lf %1.0lf %1.0lf %3.2lf%%\n", 
								i, j, pDoc->BlockCount[i][j][0]+pDoc->BlockCount[i][j][1], pDoc->BlockCount[i][j][1],
								pDoc->BlockCount[i][j][0], pDoc->BlockCount[i][j][0]/(pDoc->BlockCount[i][j][0]+pDoc->BlockCount[i][j][1])*100);
							textFile.WriteString(strtmp);
						}
					textFile.Close();		
				}
			}
			break;
		case VIEW_BLOCKHIST:
			{
				filename =  pDoc->GetPathName() + ".blkhist.txt";
				CFileDialog dlg(FALSE,"txt",filename,OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY,"Text File(*.txt)|*.txt||");
				if (dlg.DoModal()==IDOK)
				{
					filename = dlg.GetPathName(); 
					textFile.Open(filename,CFile::modeCreate|CFile::modeWrite|CFile::typeText);
					POSITION pos;
					pos = pDoc->m_LineList[1].GetHeadPosition();
					while (pos!=NULL)
					{
						strtmp = pDoc->m_LineList[1].GetNext(pos);
						strtmp += "\n";
						textFile.WriteString(strtmp);
					}
					textFile.Close();	
				}
			}
			break;
		case VIEW_CRYSTAL:
			{
				filename =  pDoc->GetPathName() + ".crystal.txt";
				CFileDialog dlg(FALSE,"txt",filename,OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY,"Text File(*.txt)|*.txt||");
				if (dlg.DoModal()==IDOK)
				{
					filename = dlg.GetPathName(); 
					textFile.Open(filename,CFile::modeCreate|CFile::modeWrite|CFile::typeText);
					//output crystal data
					textFile.WriteString("Head, Crystal-Y, Crystal-X, Count\n");
					for (int i = 0; i<DETECTOR_HEAD_NUMBER;i++)
						for (int j = 0; j<BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER; j++)
							for (int k = 0; k<BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER;k++)
							{
								strtmp.Format("%d %d %d %1.0lf\n", i, j, k, pDoc->CrystalCount[i][k][j]);
								textFile.WriteString(strtmp);
							}
					textFile.Close();		
				}
			}

			break;
		case VIEW_STATISTICS:
			break;
		case VIEW_COMPARE:
			{
				CTime CurrentTime = CTime::GetCurrentTime();
				filename =  CurrentTime.Format("%Y-%m-%d") + ".compare.txt";
				CFileDialog dlg(FALSE,"txt",filename,OFN_OVERWRITEPROMPT |OFN_HIDEREADONLY,"Text File(*.txt)|*.txt||");
				if (dlg.DoModal()==IDOK)
				{
					filename = dlg.GetPathName(); 
					textFile.Open(filename,CFile::modeCreate|CFile::modeWrite|CFile::typeText);
					POSITION pos;
					CompareData info;
					pos = pDoc->CompareList.GetHeadPosition();
					
					strtmp = "ScanDate, AssayedTime, SourceStrength, Mean, SD";
					for (int i = 0; i<DETECTOR_HEAD_NUMBER; i++)
					{
						filename.Format(", Head%d",i);
						strtmp +=filename;
					}
					strtmp +="\n";
					textFile.WriteString(strtmp);

					while (pos!=NULL)
					{
						info = pDoc->CompareList.GetNext(pos);;
						strtmp = info.ScanTime.Format("%m/%d/%y ");
						filename = info.SourceAssayTime.Format("%m/%d/%y ");
						strtmp += filename;
						filename.Format("%1.3f %1.0lf %1.0lf", info.SourceStrength, info.BlockMean, info.BlockSD);
						strtmp +=filename;
						for (int i = 0; i<DETECTOR_HEAD_NUMBER; i++)
						{
							filename.Format(" %1.0lf",info.HeadCount[i][0]+info.HeadCount[i][1]);
							strtmp +=filename;
						}
						strtmp += "\n";
						textFile.WriteString(strtmp);
					}
					for (int i = 0; i<DETECTOR_HEAD_NUMBER; i++)
					{
						strtmp.Format("---Head%d\n",i);
						textFile.WriteString(strtmp);
						strtmp = "ScanDate, AssayedTime, SourceStrength";
						for (int j= 0; j<BLOCK_NUMBER; j++)
						{
							filename.Format(", Blk%d", j);
							strtmp += filename;
						}
						strtmp +="\n";
						textFile.WriteString(strtmp);
						pos = pDoc->CompareList.GetHeadPosition();
						while (pos!=NULL)
						{
							info = pDoc->CompareList.GetNext(pos);;

							
							strtmp = info.ScanTime.Format("%m/%d/%y ");
							filename = info.SourceAssayTime.Format("%m/%d/%y ");
							strtmp += filename;
							filename.Format("%1.3f", info.SourceStrength);
							strtmp +=filename;
						
							for (int j= 0; j<BLOCK_NUMBER; j++)
							{
								filename.Format(" %1.0lf",info.BlockCount[i][j][0]+info.BlockCount[i][j][1]);
								strtmp += filename;
							}
							strtmp += "\n";
							textFile.WriteString(strtmp);
						}			
					}
					textFile.Close();		
				}
			}
			break;
		default:
			break;
	}

}

void CDailyQCView::OnUpdateFileClearcomparison(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable(!pDoc->CompareList.IsEmpty());
}

void CDailyQCView::DrawCompare(CDC* pDC)
{

	switch (m_dComp)
	{
	case COMP_GANTRY:
		DrawCompareGantry(pDC);
		break;
	case COMP_HEADS:
		DrawCompareHeads(pDC);
		break;
	case COMP_BLOCKS:
		DrawCompareBlocks(pDC);
		break;
	default:
		break;
	}
}

//void CDailyQCView::DrawCompareCurve(CDC *pDC, double max_sd, double min_sd, double max_mean, 
//									double min_mean, int time_axis_length, int y_axis_length, CPoint org)
//{
//	BOOL isprinting = pDC->IsPrinting();
//	int y_off;
//	if (isprinting)
//		y_off = -1;
//	else
//		y_off = 1;
//
//	CPen pen;
//	pen.CreatePen(PS_SOLID, 1, RGB(255,0,0));
//	CPen* pOldPen = pDC->SelectObject( &pen );
//	CPen mean_pen, sd_pen;
//	mean_pen.CreatePen(PS_SOLID, 1, RGB(0,255,0));
//	sd_pen.CreatePen(PS_SOLID, 1, RGB(0,0,255));
//	CDailyQCDoc* pDoc = GetDocument();
//	ASSERT_VALID(pDoc);
//
//	// Draw coordinate system
//	pDC->MoveTo(org);
//	pDC->LineTo(time_axis_length+org.x,org.y);
//
//	// text out time
//	CString strtmp;
//	strtmp = ((pDoc->CompareList.GetHead()).ScanTime).Format("%m/%d/%y");
//	pDC->TextOut(org.x-10,org.y+10*y_off,strtmp);
//	strtmp = ((pDoc->CompareList.GetTail()).ScanTime).Format("%m/%d/%y");
//	pDC->TextOut(org.x+time_axis_length-50,org.y+10*y_off,strtmp);
//
//	double x_scale, sd_scale, mean_scale;
//	CTimeSpan duration;
//	LONGLONG days;
//	double mean,sd;
//	CompareData info, head_info;
//
//	duration = (pDoc->CompareList.GetTail()).ScanTime-(pDoc->CompareList.GetHead()).ScanTime;
//	days = duration.GetDays()+1;
//	x_scale = (time_axis_length-10)/(double)days;
//
//	sd_scale = (y_axis_length-10)/(max_sd-min_sd)/2;
//	mean_scale = (y_axis_length-10)/(max_mean-min_mean)/2;
//	head_info = pDoc->CompareList.GetHead();
//
//	POSITION pos;
//
//	// draw mean curve
//	pDC->SelectObject(&mean_pen);
//	pDC->MoveTo(org);
//	pDC->LineTo(org.x,org.y-y_axis_length);
//	pDC->TextOut(org.x-40,org.y-y_axis_length,"Mean");
//	pos = pDoc->CompareList.GetHeadPosition();
//	
//	info = pDoc->CompareList.GetNext(pos);
//	duration = info.ScanTime-head_info.ScanTime;
//	days = duration.GetDays();
//	mean = info.BlockMean-min_mean;
//	pDC->MoveTo((int)(days*x_scale+org.x+5),(int)(org.y-mean*mean_scale-5*y_off));
//	pDC->TextOut((int)(days*x_scale+org.x),(int)(org.y-mean*mean_scale-5*y_off-5),"o");
//
//	while (pos != NULL)
//	{
//		
//		info = pDoc->CompareList.GetNext(pos);
//		duration = info.ScanTime-head_info.ScanTime;
//		mean = info.BlockMean-min_mean;
//		days = duration.GetDays();
//		pDC->LineTo((int)(days*x_scale+org.x+5), (int)(org.y-mean*mean_scale-5*y_off));
//		pDC->TextOut((int)(days*x_scale+org.x),(int)(org.y-mean*mean_scale-5*y_off-5),"o");
//
//	}
//
//	// draw sd curve
//	pos = pDoc->CompareList.GetHeadPosition();
//	info = pDoc->CompareList.GetNext(pos);
//	duration = info.ScanTime-head_info.ScanTime;
//	days = duration.GetDays();
//	sd = info.BlockSD-min_sd;
//	pDC->SelectObject(&sd_pen);
//	pDC->MoveTo(org.x+time_axis_length,org.y);
//	pDC->LineTo(org.x+time_axis_length,org.y-y_axis_length);
//	pDC->TextOut(org.x+time_axis_length+5,org.y-y_axis_length,"Standard Deviation");
//	pDC->MoveTo((int)(days*x_scale+org.x+5),(int)(org.y-sd*sd_scale-5*y_off));
//	while (pos != NULL)
//	{
//		
//		info = pDoc->CompareList.GetNext(pos);
//		duration = info.ScanTime-head_info.ScanTime;
//		sd = info.BlockSD-min_sd;
//		days = duration.GetDays();
//		pDC->LineTo((int)(days*x_scale+org.x+5), (int)(org.y-sd*sd_scale-5*y_off));
//	}
//	// draw compensated mean
//	
//	// draw compensated sd
//
//	pDC->SelectObject(pOldPen);
//}
void CDailyQCView::OnUpdateFileExporttextfile(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_SUMMARY ||
		m_dViewResult == VIEW_BLOCKHIST ||
		m_dViewResult == VIEW_COMPARE ||
		m_dViewResult == VIEW_CRYSTAL ||
		m_dViewResult == VIEW_BLOCKMAP)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CDailyQCView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
	// TODO: Add your specialized code here and/or call the base class

	CScrollView::OnPrepareDC(pDC, pInfo);

	BOOL isprinting = pDC->IsPrinting();
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (isprinting)
	{
		pDC->SetMapMode(MM_LOENGLISH);
		if (m_dViewResult == VIEW_SUMMARY||
			m_dViewResult == VIEW_BLOCKHIST)
		{
			int index;
			if (m_dViewResult == VIEW_SUMMARY)
				index = 0;
			else
				index = 1;
			
			m_dStartLine = m_dLinesPerPage * (pInfo->m_nCurPage - 1);
			m_dLastLine = min(m_dStartLine + m_dLinesPerPage,
				(int)pDoc->m_LineList[index].GetCount());
		}
		if (m_dViewResult == VIEW_COMPARE && m_dComp == COMP_BLOCKS)
		{
			m_dStartLine = m_dLinesPerPage * ((pInfo->m_nCurPage-1)%(m_dPrintPages/2))
				+BLOCK_AXIAL_NUMBER*((pInfo->m_nCurPage-1)/(m_dPrintPages/2));
			m_dLastLine = min(m_dStartLine + m_dLinesPerPage, 
				(int)((pInfo->m_nCurPage-1)/(m_dPrintPages/2)+1)*BLOCK_AXIAL_NUMBER);
		}
	 }
}

BOOL CDailyQCView::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (m_pViewToolTip != NULL)
		m_pViewToolTip->RelayEvent(pMsg);

	return CScrollView::PreTranslateMessage(pMsg);
}

// Create block tooltips
int CDailyQCView::CreateBlockToolTips(int org_x, int org_y, int offset_x, int offset_y, 
									  int x_number, int y_number, int head)
{
	CPoint click_pt = GetScrollPosition();

	RECT rect;
	for (int j = 0; j<y_number; j++)
		for (int i = 0; i<x_number; i++)
		{
			rect.top = org_y+j*offset_y - click_pt.y;
			rect.left = org_x+i*offset_x - click_pt.x;
			rect.bottom = rect.top + offset_y;
			rect.right = rect.left + offset_x;
			m_pViewToolTip->AddTool(this, LPSTR_TEXTCALLBACK, &rect,
				IDT_VIEW+head*(int)(BLOCK_NUMBER)+j*x_number+i);
		}

	return 0;
}

BOOL CDailyQCView::ProduceViewToolTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult )
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pNMHDR;
    int nID =(int)(pNMHDR->idFrom - IDT_VIEW);
	CString string;
    if (nID>=0  && nID<DETECTOR_HEAD_NUMBER*BLOCK_NUMBER)
    {
		int head = nID/(int)(BLOCK_NUMBER);
		int block = nID%(int)(BLOCK_NUMBER); 
		CDailyQCDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);
		string.Format("H%d-B%d: %1.0lf, %1.0lf, %1.0lf, %3.2f%%", head, block,
			pDoc->BlockCount[head][block][0]+pDoc->BlockCount[head][block][1],
			pDoc->BlockCount[head][block][1], pDoc->BlockCount[head][block][0],
			pDoc->BlockCount[head][block][0]*100/(pDoc->BlockCount[head][block][0]+pDoc->BlockCount[head][block][1]));	
    }
	else
	{
		string = "Not available";	
	}
	::lstrcpy(pTTT->szText,(LPCTSTR)string);
	 return(TRUE);
}


// load data from doc to CompareData structure
void CDailyQCView::LoadCompareData(CDailyQCDoc * pDoc, CompareData * pData)
{
	pData->ScanTime = pDoc->ScanTime;
	pData->SourceAssayTime = pDoc->SourceAssayTime;
	pData->SourceStrength = pDoc->SourceStrength;
	pData->MRQC = pDoc->MRQC;
	pData->BlockMean = pDoc->BlockMean;
	pData->BlockSD = pDoc->BlockSD;
	for (int i =0;i<DETECTOR_HEAD_NUMBER; i++)
		for (int k = 0; k<DOI_NUMBER; k++)
		{
			pData->HeadCount[i][k] = pDoc->HeadCount[i][k];
			for (int j =0; j<BLOCK_NUMBER;j++)
				pData->BlockCount[i][j][k] = pDoc->BlockCount[i][j][k];
		}


}

void CDailyQCView::OnBlockmapFrontlayer()
{
	// TODO: Add your command handler code here
	m_dLayer = FRONT_LAYER;
	m_dViewResult = VIEW_BLOCKMAP;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	m_dSelectedHead = -1;;
	m_dSelectedBlock = -1;
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	Invalidate();
}

void CDailyQCView::OnUpdateBlockmapFrontlayer(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult != VIEW_BLOCKMAP)
	{
		m_dLayer = NO_LAYER;
	}
	switch(m_dLayer)
	{
	case FRONT_LAYER:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}

void CDailyQCView::OnBlockmapBacklayer()
{
	// TODO: Add your command handler code here
	m_dLayer = BACK_LAYER;
	m_dViewResult = VIEW_BLOCKMAP;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	m_dSelectedHead = -1;;
	m_dSelectedBlock = -1;
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();	
	Invalidate();
}

void CDailyQCView::OnUpdateBlockmapBacklayer(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult != VIEW_BLOCKMAP)
	{
		m_dLayer = NO_LAYER;
	}
	switch(m_dLayer)
	{
	case BACK_LAYER:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}

void CDailyQCView::OnBlockmapBothlayers()
{
	// TODO: Add your command handler code here
	m_dLayer = BOTH_LAYER;
	m_dViewResult = VIEW_BLOCKMAP;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	m_dSelectedHead = -1;;
	m_dSelectedBlock = -1;
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();	Invalidate();
}

void CDailyQCView::OnUpdateBlockmapBothlayers(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult != VIEW_BLOCKMAP)
	{
		m_dLayer = NO_LAYER;
	}

	switch(m_dLayer)
	{
	case BOTH_LAYER:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}


void CDailyQCView::OnComparisonGantry()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_COMPARE;
	m_dComp = COMP_GANTRY;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}
	Invalidate();
}

void CDailyQCView::OnUpdateComparisonGantry(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable(!pDoc->CompareList.IsEmpty());

	if (m_dViewResult != VIEW_COMPARE)
	{
		m_dComp = COMP_NO;
	}
	
	switch(m_dComp)
	{
	case COMP_GANTRY:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}

void CDailyQCView::OnComparisonHeads()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_COMPARE;
	m_dComp = COMP_HEADS;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}

	Invalidate();
}

void CDailyQCView::OnUpdateComparisonHeads(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable(!pDoc->CompareList.IsEmpty());

	if (m_dViewResult != VIEW_COMPARE)
	{
		m_dComp = COMP_NO;
	}

	switch(m_dComp)
	{
	case COMP_HEADS:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}

void CDailyQCView::OnComparisonBlocks()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_COMPARE;
	m_dComp = COMP_BLOCKS;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}

	// here to get range to pass to dlg
	CDC * pDC = GetDC();
	DrawCompareBlocks(pDC);
	ReleaseDC(pDC);

	m_dSelectedHead = 0;;
	m_dSelectedBlock = 0;
	// display detail window
	if (m_pCrystalDlg == NULL)
		m_pCrystalDlg = new CCrystalDlg(this);
	m_pCrystalDlg->SetWindowText("Block Compare");

	m_dViewDetail = 1;
	m_pCrystalDlg->UpdateData(FALSE);
	m_pCrystalDlg->ShowWindow(SW_SHOW);
	//Invalidate();
}

void CDailyQCView::OnUpdateComparisonBlocks(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable(!pDoc->CompareList.IsEmpty());

	if (m_dViewResult != VIEW_COMPARE)
	{
		m_dComp = COMP_NO;
	}

	switch(m_dComp)
	{
	case COMP_BLOCKS:
		pCmdUI->SetCheck(TRUE);
		break;
	default:
		pCmdUI->SetCheck(FALSE);
	}
}

void CDailyQCView::DrawCompareGantry(CDC *pDC)
{
	// first get the compare list
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CDC dcMem;
	CBitmap bitmap, *pOldBitMap;
	
	// find data range
	POSITION pos;
	CompareData info, master_info;
	double master_activity;
	int master = -1;

	PlotData * pData, *itData, *pDecayData, *itDecayData;
	pData = new PlotData[pDoc->CompareList.GetCount()];
	pDecayData = new PlotData[pDoc->CompareList.GetCount()];
	
	// draw mean curve

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
	
	// get mean data
	itData = pData;
	itDecayData = pDecayData;
	pos = pDoc->CompareList.GetHeadPosition();
	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
	{
		info = pDoc->CompareList.GetNext(pos);
		itData->y = info.BlockMean;		
		double activity;
		CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
		activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
		itDecayData->y = info.BlockMean*master_activity/activity;

		if (i==0)
		{
			range.max_x = range.min_x =info.ScanTime;
			range.max_y = range.min_y = itDecayData->y;
		}
		else
		{
			if (m_bDecay)
			{
				if (itData->y>range.max_y)
					range.max_y = itData->y;
				if (itData->y<range.min_y)
					range.min_y = itData->y;
			}
			if (itDecayData->y>range.max_y)
				range.max_y = itDecayData->y;
			if (itDecayData->y<range.min_y)
				range.min_y = itDecayData->y;
			if (info.ScanTime>range.max_x)
				range.max_x = info.ScanTime;
			if (info.ScanTime <range.min_x)
				range.min_x = info.ScanTime;

		}
		itData->xt = info.ScanTime;
		itDecayData->xt = info.ScanTime;
		itDecayData++;
		itData ++;
	}
	

	BOOL isprinting = pDC->IsPrinting();

	if (isprinting)
	{
		CSize DevSize;
		float printer_scale;
		DevSize.cx = pDC->GetDeviceCaps(HORZRES);
		DevSize.cy = pDC->GetDeviceCaps(VERTRES);
		pDC->DPtoLP(&DevSize);
		printer_scale = min((float)DevSize.cx/m_sizeTotalArea.cx,(float)DevSize.cy/m_sizeTotalArea.cy);
		pDC->TextOut(50, -50, "HRRT Daily QC Comparison: Gantry");
		RECT area = {100,-100,550,-450};
		DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
			master, 1, "Gantry Block Mean", CPoint(0, -25),2, PS_SOLID);
		if (m_bDecay)
			DrawCurve(pDC,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
				-1, 0, "", CPoint(0, -25),1, PS_DOT);	
		DrawCoord(pDC, area, range,0, 0,CPoint(-50,-10));
		DrawCoord(pDC, area, range, 1, 4, CPoint(-75,15));
	}
	else
	{
		bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx,m_sizeTotalArea.cy);
		dcMem.CreateCompatibleDC(pDC);
		pOldBitMap = dcMem.SelectObject(&bitmap);

		dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
		dcMem.TextOut(50,50, "HRRT Daily QC Comparison: Gantry");
		//DrawCompareCurve(&dcMem, max_sd, min_sd, max_mean, min_mean, 550, 350,CPoint(100,450));
		RECT area = {100,100,550,450};
		DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
			master, 1, "Gantry Block Mean", CPoint(0, 25),2, PS_SOLID);
		if (m_bDecay)
			DrawCurve(&dcMem,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
				-1, 0, "", CPoint(0, 25),1, PS_DOT);

		DrawCoord(&dcMem, area, range,0,0,CPoint(-50,10));
		DrawCoord(&dcMem, area, range, 1, 4, CPoint(-75,-15));
	}

	// draw SD curve
	itData = pData;
	pos = pDoc->CompareList.GetHeadPosition();
	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
	{
		info = pDoc->CompareList.GetNext(pos);
		if (i==0)
		{
			// range.max_x = range.min_x =(double) info.ScanTime.GetTime();
			range.max_y = range.min_y = info.BlockSD;
		}
		else
		{
			if (info.BlockSD>range.max_y)
				range.max_y = info.BlockSD;
			if (info.BlockSD<range.min_y)
				range.min_y = info.BlockSD;
		}
		itData ++;
	}

	// get sd data
	itData = pData;
	pos = pDoc->CompareList.GetHeadPosition();
	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
	{
		info = pDoc->CompareList.GetNext(pos);
		itData->y = info.BlockSD;
		itData ++;
	}
	if (isprinting)
	{
		RECT area = {100,-100,550,-450};
		DrawCurve(pDC,area,range,"x", pData, (int)pDoc->CompareList.GetCount(), RGB(0,0,255),
			-1, 2, "Gantry Block SD", CPoint(100,-25), 2,PS_SOLID);
		DrawCoord(pDC, area, range, 2, 4, CPoint(5,-15));
	}
	else
	{
		RECT area = {100,100,550,450};
		DrawCurve(&dcMem,area,range,"x", pData, (int)pDoc->CompareList.GetCount(), RGB(0,0,255),
			-1, 2, "Gantry Block SD", CPoint(100,25), 2,PS_SOLID);
		DrawCoord(&dcMem, area, range, 2, 4, CPoint(5,-15));
		pDC->BitBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy, &dcMem,0,0,SRCCOPY);
		dcMem.SelectObject(pOldBitMap);
		bitmap.DeleteObject();
	}
	delete []pData;
	delete []pDecayData;

}

void CDailyQCView::DrawCompareHeads(CDC * pDC)
{
	// first get the compare list
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CDC dcMem;
	CBitmap bitmap, *pOldBitMap;
	
	// find data range
	POSITION pos;
	CompareData info;
	
	BOOL isprinting = pDC->IsPrinting();

	PlotData * pData, *itData, *pDecayData, *itDecayData;
	pData = new PlotData[pDoc->CompareList.GetCount()];
	pDecayData = new PlotData[pDoc->CompareList.GetCount()];

	double head_count;
	int master = -1;
	double master_activity;

	COLORREF color[DETECTOR_HEAD_NUMBER] = {RGB(0,0,0),RGB(255,0,0),RGB(0,255,255),RGB(0,0,255),
		RGB(255,0,255),RGB(255,255,0), RGB(128,0,128),RGB(0,255,0)};
	
	// first get range for all heads
	//get mater
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

	// for decay compensation, need to get range for compensated data
	itData = pData;
	itDecayData = pDecayData;
	pos = pDoc->CompareList.GetHeadPosition();
	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
	{
		info = pDoc->CompareList.GetNext(pos);
		double activity;
		CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
		activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
		if (i==0)
		{
			head_count = info.HeadCount[0][0]+info.HeadCount[0][1];
			range.max_x = range.min_x = info.ScanTime;
			range.max_y = range.min_y = head_count * master_activity/activity;
		}
		for (int h= 0; h<DETECTOR_HEAD_NUMBER; h++)
		{
			head_count = info.HeadCount[h][0]+info.HeadCount[h][1];
			if (m_bDecay)
			{
				if (head_count>range.max_y)
					range.max_y = head_count;
				if (head_count<range.min_y)
					range.min_y = head_count;
			}
			head_count = head_count*master_activity/activity;
			if (head_count>range.max_y)
				range.max_y = head_count;
			if (head_count<range.min_y)
				range.min_y = head_count;
		}

		if (info.ScanTime>range.max_x)
			range.max_x = info.ScanTime;
		if (info.ScanTime <range.min_x)
			range.min_x = info.ScanTime;

		itDecayData->xt = info.ScanTime;
		itData->xt = info.ScanTime;
		itData ++;
		itDecayData ++;
	}

	// get data and draw curves
	for (int h= 0; h<DETECTOR_HEAD_NUMBER; h++)
	{
		itData = pData;
		itDecayData = pDecayData;

		pos = pDoc->CompareList.GetHeadPosition();
		for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
		{
			info = pDoc->CompareList.GetNext(pos);
			itData->y = info.HeadCount[h][0]+info.HeadCount[h][1];
			double activity;
			CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
			activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
			itDecayData->y = itData->y*master_activity/activity;
			itDecayData++;
			itData ++;
		}

		if (isprinting)
		{
			if (h == 0)
			{
				pDC->TextOut(50,-50, "HRRT Daily QC Comparison: Heads");
			}
			//DrawCompareCurve(&dcMem, max_sd, min_sd, max_mean, min_mean, 550, 350,CPoint(100,450));
			RECT area = {100,-100,550,-450};
			if (h ==0 )
				DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), color[h],
					master, 1, "Head Count",CPoint(0, -25),2,PS_SOLID);
			else
				DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), color[h],
					-1, 0, "",CPoint(0, -25),2,PS_SOLID);
			if (m_bDecay)
				DrawCurve(pDC,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), color[h],
					-1, 0, "",CPoint(0, -25),1,PS_DOT);

			CPen pen;
			CString tmp;
			pen.CreatePen(PS_SOLID, 2, color[h]);
			pDC->SelectObject( &pen );
			pDC->MoveTo(area.right, area.top-h*20);
			pDC->LineTo(area.right+30, area.top-h*20);
			tmp.Format("H%d",h);
			pDC->TextOut(area.right+35, area.top-h*20+10, tmp); 
			if (h == DETECTOR_HEAD_NUMBER-1)
			{
				DrawCoord(pDC,area,range,0,0,CPoint(-50,-10));
				DrawCoord(pDC,area,range,1,4,CPoint(-75, 15));
			}
		}
		else
		{
			if (h == 0)
			{
				bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx,m_sizeTotalArea.cy);
				dcMem.CreateCompatibleDC(pDC);
				pOldBitMap = dcMem.SelectObject(&bitmap);

				dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
				dcMem.TextOut(50,50, "HRRT Daily QC Comparison: Heads");
			}
			//DrawCompareCurve(&dcMem, max_sd, min_sd, max_mean, min_mean, 550, 350,CPoint(100,450));
			RECT area = {100,100,550,450};
			if (h ==0 )
				DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), color[h],
					master, 1, "Head Count",CPoint(0, 25),2,PS_SOLID);
			else
				DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), color[h],
					-1, 0, "",CPoint(0, 25),2,PS_SOLID);
			if (m_bDecay)
				DrawCurve(&dcMem,area,range,"o", pData, (int)pDoc->CompareList.GetCount(), color[h],
					-1, 0, "",CPoint(0, 25),1,PS_DOT);

			CPen pen;
			CString tmp;
			pen.CreatePen(PS_SOLID, 2, color[h]);
			dcMem.SelectObject( &pen );
			dcMem.MoveTo(area.right, area.top+h*20);
			dcMem.LineTo(area.right+30, area.top+h*20);
			tmp.Format("H%d",h);
			dcMem.TextOut(area.right+35, area.top+h*20-10, tmp); 
			if (h == DETECTOR_HEAD_NUMBER-1)
			{
				DrawCoord(&dcMem,area,range,0,0,CPoint(-50,10));
				DrawCoord(&dcMem,area,range,1,4,CPoint(-75,-15));
				pDC->BitBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy, &dcMem,0,0,SRCCOPY);
				dcMem.SelectObject(pOldBitMap);
				bitmap.DeleteObject();
			}
		}

	}
	delete []pData;
	delete []pDecayData;
}

void CDailyQCView::DrawCompareBlocks(CDC *pDC)
{
	// first get the compare list
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CDC dcMem;
	CBitmap bitmap, *pOldBitMap;
	
	// find data range
	POSITION pos;
	CompareData info;
	BOOL isprinting = pDC->IsPrinting();
	PlotData *pDecayData, * itDecayData;
	pDecayData = new PlotData[pDoc->CompareList.GetCount()];
    double block_count;
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

	// first get compensated range for all blocks

	itDecayData = pDecayData;
	pos = pDoc->CompareList.GetHeadPosition();
	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
	{
		info = pDoc->CompareList.GetNext(pos);
		double activity;
		CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
		activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);

		if (i==0)
		{
			block_count = info.BlockCount[0][0][0]+info.BlockCount[0][0][1];
			range.max_x = range.min_x = info.ScanTime;
			range.max_y = range.min_y = block_count * master_activity / activity;
		}

		for (int h = 0; h<DETECTOR_HEAD_NUMBER; h++)
			for (int b= 0; b<BLOCK_NUMBER; b++)
			{
				block_count = (info.BlockCount[h][b][0]+info.BlockCount[h][b][1]) * master_activity / activity;
				if (block_count>range.max_y)
					range.max_y = block_count;
				if (block_count<range.min_y)
					range.min_y = block_count;
			}
		if (info.ScanTime>range.max_x)
			range.max_x =  info.ScanTime;
		if (info.ScanTime <range.min_x)
			range.min_x = info.ScanTime;
		itDecayData->xt = info.ScanTime;		
		itDecayData ++;
	}

	// get data and draw curves
	if (isprinting)
	{
		for (int a = m_dStartLine%BLOCK_AXIAL_NUMBER; a<= (m_dLastLine-1)%BLOCK_AXIAL_NUMBER; a++)
		{
			for (int h = DETECTOR_HEAD_NUMBER/2*(m_dStartLine/BLOCK_AXIAL_NUMBER); 
				h<DETECTOR_HEAD_NUMBER/2+DETECTOR_HEAD_NUMBER/2*(m_dStartLine/BLOCK_AXIAL_NUMBER); h++)
			{
				if (m_dStartLine == 0)
				{
					pDC->TextOut(50,-50, "HRRT Daily QC Comparison: Blocks");
				}

				RECT area = {80+180*(h%(int)(DETECTOR_HEAD_NUMBER/2)),-100-192*(a-m_dStartLine%BLOCK_AXIAL_NUMBER),
					220+180*(h%(int)(DETECTOR_HEAD_NUMBER/2)),-190-192*(a-m_dStartLine%BLOCK_AXIAL_NUMBER)};
				for (int t = 0; t <BLOCK_TRANSAXIAL_NUMBER;t++)
				{
					itDecayData = pDecayData;
					pos = pDoc->CompareList.GetHeadPosition();
					for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
					{
						info = pDoc->CompareList.GetNext(pos);
						double activity;
						CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
						activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
						itDecayData->y = (info.BlockCount[h][a*BLOCK_TRANSAXIAL_NUMBER+t][0]
							+info.BlockCount[h][a*BLOCK_TRANSAXIAL_NUMBER+t][1]) * master_activity / activity;
						itDecayData ++;
					}

					if (t ==0 )
					{
						CString tmp;
						tmp.Format("H%d B%d-%d", h, a*BLOCK_TRANSAXIAL_NUMBER, (a+1)*BLOCK_TRANSAXIAL_NUMBER-1);
						DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
						master, 1, tmp,CPoint(0, -20),1,PS_SOLID);
					}
					else
						DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
									-1, 0, "",CPoint(0, -20),1,PS_SOLID);	
				}				
			}
			//if (h == DETECTOR_HEAD_NUMBER-1)
			//{
			//	// draw selected curve
			//	itDecayData = pDecayData;
			//	pos = pDoc->CompareList.GetHeadPosition();
			//	for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
			//	{
			//		info = pDoc->CompareList.GetNext(pos);
			//		double activity;
			//		CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
			//		activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);

			//		itDecayData->y = (info.BlockCount[m_dSelectedHead][m_dSelectedBlock][0]
			//			+info.BlockCount[m_dSelectedHead][m_dSelectedBlock][1]) * master_activity / activity;
			//		itDecayData ++;
			//	}
			//	RECT area = {80+180*m_dSelectedHead,-100-192*(m_dSelectedBlock/(int)BLOCK_TRANSAXIAL_NUMBER),
			//		220+180*m_dSelectedHead,-190-192*(m_dSelectedBlock/(int)BLOCK_TRANSAXIAL_NUMBER)};
			//	DrawCurve(pDC,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(255,0,0),
			//							-1, 0, "",CPoint(0, -20),2,PS_SOLID);	

			//}
		}
	}

	else
	{
		for (int h= 0; h<DETECTOR_HEAD_NUMBER; h++)
		{

			if (h == 0)
			{
				bitmap.CreateCompatibleBitmap(pDC,m_sizeTotalArea.cx,m_sizeTotalArea.cy);
				dcMem.CreateCompatibleDC(pDC);
				pOldBitMap = dcMem.SelectObject(&bitmap);

				dcMem.PatBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy,WHITENESS);
				dcMem.TextOut(50,50, "HRRT Daily QC Comparison: Blocks");
				//pDC->TextOut(50,50, "HRRT Daily QC Comparison: Blocks");
			}
			for (int a = 0; a<BLOCK_AXIAL_NUMBER; a++)
			{
				RECT area = {80+180*h,100+192*a,220+180*h,190+192*a};
				for (int t = 0; t <BLOCK_TRANSAXIAL_NUMBER;t++)
				{
					itDecayData = pDecayData;
					pos = pDoc->CompareList.GetHeadPosition();
					for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
					{
						info = pDoc->CompareList.GetNext(pos);
						double activity;
						CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
						activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
						itDecayData->y = (info.BlockCount[h][a*BLOCK_TRANSAXIAL_NUMBER+t][0]
							+info.BlockCount[h][a*BLOCK_TRANSAXIAL_NUMBER+t][1]) * master_activity / activity;
						itDecayData ++;
					}

					if (t ==0 )
					{
						CString tmp;
						tmp.Format("H%d B%d-%d", h, a*BLOCK_TRANSAXIAL_NUMBER, (a+1)*BLOCK_TRANSAXIAL_NUMBER-1);
						DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
						master, 1, tmp,CPoint(0, 20),1,PS_SOLID);
					}
					else
						DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(0,0,0),
									-1, 0, "",CPoint(0, 20),1,PS_SOLID);	
				}
				
			}
			if (h == DETECTOR_HEAD_NUMBER-1)
			{
				// draw selected curve
				itDecayData = pDecayData;
				pos = pDoc->CompareList.GetHeadPosition();
				for (int i = 0; i<pDoc->CompareList.GetCount(); i++)
				{
					info = pDoc->CompareList.GetNext(pos);
					double activity;
					CTimeSpan DecayTime = info.ScanTime - info.SourceAssayTime;
					activity = info.SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);

					itDecayData->y = (info.BlockCount[m_dSelectedHead][m_dSelectedBlock][0]
						+info.BlockCount[m_dSelectedHead][m_dSelectedBlock][1]) * master_activity / activity;
					itDecayData ++;
				}
				RECT area = {80+180*m_dSelectedHead,100+192*(m_dSelectedBlock/(int)BLOCK_TRANSAXIAL_NUMBER),
					220+180*m_dSelectedHead,190+192*(m_dSelectedBlock/(int)BLOCK_TRANSAXIAL_NUMBER)};
				DrawCurve(&dcMem,area,range,"o", pDecayData, (int)pDoc->CompareList.GetCount(), RGB(255,0,0),
										-1, 0, "",CPoint(0, 20),2,PS_SOLID);	

				pDC->BitBlt(0,0,m_sizeTotalArea.cx,m_sizeTotalArea.cy, &dcMem,0,0,SRCCOPY);
				dcMem.SelectObject(pOldBitMap);
				bitmap.DeleteObject();
			}
		}
	}
	delete []pDecayData;
}

void CDailyQCView::DrawCurve(CDC * pDC, RECT area, PlotDataRange range, CString symble, 
	PlotData *pData, int dataSize, COLORREF color, int master, int label, CString label_string, CPoint label_pos,
	int pen_width, int penStyle)
// label 0: no label
// label 1: label on the left
// label 2: label on the right
{

	// first draw coordiante system, x is alway read
	int bkmode;
	CPen pen, redPen;
	static CTime master_time;
	redPen.CreatePen(PS_SOLID, 1, RGB(255,0,0));
	pDC->SelectObject( &redPen );
	pDC->MoveTo(area.left,area.bottom);
	pDC->LineTo(area.right, area.bottom);
	
	// draw y coordinate
	pen.CreatePen(PS_SOLID, 1, color);
	pDC->SelectObject( &pen );
	bkmode = pDC->GetBkMode();
	pDC->SetBkMode(TRANSPARENT);
	if (label == 1 || label == 2)
	{	
		if (label == 1)
		{
			pDC->MoveTo(area.left, area.top);
			pDC->LineTo(area.left, area.bottom);
			pDC->TextOut(area.left-label_pos.x, area.top-label_pos.y,label_string);
		}
		else
		{
			pDC->MoveTo(area.right, area.top);
			pDC->LineTo(area.right, area.bottom);
			pDC->TextOut(area.right-label_pos.x, area.top-label_pos.y,label_string);
		}	
	}
	pDC->SetBkMode(bkmode);
	// draw curve
	pen.DeleteObject();
	pen.CreatePen(penStyle, pen_width, color);
	pDC->SelectObject(&pen);
	// define the conversion from data to pixel
	double x_conv;
	double y_conv;
	double max_y, min_y;
	int limit;

	x_conv = (area.right-area.left-20)/(double)(range.max_x.GetTime()-range.min_x.GetTime());
	y_conv = log10((range.max_y - range.min_y)/10);

	limit = (int)((range.max_y+(range.max_y - range.min_y)/10)/pow(10.,(int)y_conv)+0.5);
	limit = limit +1;

	max_y = limit * pow(10.,(int)y_conv);

	limit = (int)((range.min_y-(range.max_y - range.min_y)/10)/pow(10.,(int)y_conv)+0.5);
	limit = limit - 1;
	min_y = limit *pow(10.,(int)y_conv);
	y_conv = (area.bottom - area.top)/(max_y-min_y);
	
	// get master time
	if (master>=0)
		master_time = (pData+master)->xt;

	// start to draw curve
	int x, y;
	float decay = 1.0;
	x = (int)((pData->xt.GetTime()-range.min_x.GetTime())*x_conv+area.left+10);
	y = (int)((max_y-pData->y)*y_conv)+area.top;
	pDC->MoveTo(x,y);

	for (int i = 0; i<dataSize; i++)
	{
		x = (int)((pData->xt.GetTime()-range.min_x.GetTime())*x_conv+area.left+10);
		y = (int)((max_y-pData->y)*y_conv)+area.top;
		pDC->LineTo(x,y);
		// draw master position
		if (master == i)
		{
			redPen.DeleteObject();
			redPen.CreatePen(PS_DOT, 1, RGB(255,0,0));
			pDC->SelectObject( &redPen );
			pDC->MoveTo(x,area.bottom);
			pDC->LineTo(x,area.top);
			pDC->MoveTo(x,y);
			pDC->SelectObject( &pen );
		}
		pData++;
	}
}

void CDailyQCView::OnViewCrystalbitmap()
{
	// TODO: Add your command handler code here
	m_dViewResult = VIEW_CRYSTAL;
	CalculateScrollSize();
	SetScrollSizes(MM_TEXT,m_sizeTotalArea, m_sizePage, m_sizeLine);
	Invalidate();
	m_dViewDetail = 1;
	m_dSelectedHead = 0;;
	m_dSelectedBlock = 0;

	// First destroy the dlg
	if (m_pCrystalDlg != NULL)
		m_pCrystalDlg->DestroyWindow();
	// Create the dlg
	if (m_pCrystalDlg == NULL)
		m_pCrystalDlg = new CCrystalDlg(this);
	if (m_pViewToolTip != NULL)
	{
		delete m_pViewToolTip;
		m_pViewToolTip = NULL;
	}


	m_pCrystalDlg->UpdateData(FALSE);
	m_pCrystalDlg->ShowWindow(SW_SHOW);
}

void CDailyQCView::OnUpdateViewCrystalbitmap(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_CRYSTAL)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CDailyQCView::OnViewDetails()
{
	// TODO: Add your command handler code here
	if (m_dViewDetail == 0)
	{
		m_dViewDetail = 1;
		if (m_pCrystalDlg == NULL)
		{
			//m_dSelectedHead = 0;
			//m_dSelectedBlock = 0;
			m_pCrystalDlg = new CCrystalDlg(this);
			if (m_dViewResult == VIEW_COMPARE)
			{
				if (m_dComp == COMP_BLOCKS)
				{
					m_pCrystalDlg->SetWindowText("Block Compare");
				}
			}
			m_pCrystalDlg->UpdateData(FALSE);
			m_pCrystalDlg->ShowWindow(SW_SHOW);
		}
	}
	else
	{
		m_dViewDetail = 0;
		if (m_pCrystalDlg != NULL)
		{
			m_pCrystalDlg->DestroyWindow();
			m_pCrystalDlg = NULL;
		}
	}
}

void CDailyQCView::OnUpdateViewDetails(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	if (m_dViewResult == VIEW_CRYSTAL || (m_dViewResult == VIEW_COMPARE && m_dComp == COMP_BLOCKS))
	{
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_dViewDetail);	
	}		
	else
		pCmdUI->Enable(FALSE);
}

void CDailyQCView::OnViewDecaycompensation()
{
	// TODO: Add your command handler code here
	m_bDecay = !m_bDecay;
	if (m_dComp != COMP_BLOCKS)
		Invalidate();
	if (m_pCrystalDlg!=NULL)
	{
		m_pCrystalDlg->SetRange(range);
		m_pCrystalDlg->Invalidate();
	}
}

void CDailyQCView::OnUpdateViewDecaycompensation(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	// TODO: Add your command update UI handler code here
	if (m_dViewResult != VIEW_COMPARE )
	{
		pCmdUI->Enable(FALSE);
		m_bDecay = FALSE;
	}
	else
	{
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(m_bDecay);	
	}
}

void CDailyQCView::DrawCoord(CDC * pDC, RECT area, PlotDataRange range, int dir, int div, CPoint offset)
{
	int bkmode;
	bkmode = pDC->GetBkMode();
	pDC->SetBkMode(TRANSPARENT);
	if (dir == 0)		// x direction
	{
		pDC->TextOut(area.left, area.bottom+offset.y, range.min_x.Format("%m/%d/%y"));
		pDC->TextOut(area.right+offset.x, area.bottom+offset.y, range.max_x.Format("%m/%d/%y"));
	}
	if (dir == 1 || dir ==2)		// y direction
	{
		for (int i = 0; i<=div; i++)
		{
			CString tmp;
			tmp.Format("%3.2e", range.min_y+i*(range.max_y-range.min_y)/div);
			int x;
			if (dir == 1)
				x = area.left + offset.x;
			if (dir == 2)
				x =area.right + offset.x;
			pDC->TextOut(x, area.bottom-(area.bottom-area.top)/div*i+offset.y, tmp);
		}
	}
	pDC->SetBkMode(bkmode);
}

void CDailyQCView::AddToCompareList(CompareData *masterData)
{

	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CompareData QCData;

	CFileDialog dlg(TRUE,NULL,"*.dqc",OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_EXPLORER,
			"DailyQC Files(*.dqc)|*.dqc");
	//CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY|OFN_EXPLORER);

	
	CString FilePath;
	POSITION filePos;
	POSITION listPos, oldListPos;
	
	char tmp[4096];
	int add =0;
	tmp[0] = NULL;			// first character must be NULL 
	dlg.m_ofn.lpstrFile = tmp;
	dlg.m_ofn.nMaxFile = 4096;

	if (dlg.DoModal()==IDOK)
	{
		// add master QC
		if (masterData != NULL)
		{
			pDoc->CompareList.RemoveAll();
			pDoc->CompareList.AddTail(*masterData);
		}
		filePos = dlg.GetStartPosition();
		while (filePos!=NULL)
		{
			TRY
			{
				FilePath = dlg.GetNextPathName(filePos);
				if (FilePath == m_sCompMRQC)
					continue;
				// read scan time, mean and srandard deviation of each dqc file
				CFile dqcFile(FilePath,CFile::modeRead);
				CArchive ar(&dqcFile,CArchive::load);
				CRuntimeClass *pDocClass  = RUNTIME_CLASS(CDailyQCDoc);
				CDailyQCDoc *pCompareDoc = (CDailyQCDoc *)pDocClass->CreateObject();
				pCompareDoc->Serialize(ar);
				LoadCompareData(pCompareDoc,&QCData);
			}
			CATCH ( CFileException, pEx )
			{
				AfxMessageBox("QC file can not be loaded!");
				pDoc->CompareList.RemoveAll();
				OnViewSummary();
				return;
			}
			END_CATCH
			

			// Add to compare list
			listPos = pDoc->CompareList.GetHeadPosition();
			add = 0;
			while (listPos!=NULL)
			{
				oldListPos = listPos;
				if (pDoc->CompareList.GetNext(listPos).ScanTime > QCData.ScanTime)
				{
					pDoc->CompareList.InsertBefore(oldListPos,QCData);
					add = 1;
					break;
				}
			}
			if (add == 0)
				pDoc->CompareList.AddTail(QCData);
		}
		OnComparisonGantry();
	}
}

void CDailyQCView::OnFileAddcomparison()
{
	// TODO: Add your command handler code here
	AddToCompareList(NULL);
}

void CDailyQCView::OnUpdateFileAddcomparison(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here
	CDailyQCDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->Enable(!pDoc->CompareList.IsEmpty());

}
