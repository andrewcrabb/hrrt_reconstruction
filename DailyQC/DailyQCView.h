// DailyQCView.h : interface of the CDailyQCView class
//
#pragma once

#include "MainFrm.h"
#include "atltypes.h"

//view resulr define
#define VIEW_SUMMARY	0
#define VIEW_BLOCKHIST  1
#define VIEW_CRYSTAL	2
#define VIEW_BLOCKMAP   3
#define VIEW_STATISTICS	4
#define VIEW_COMPARE    5

#define PIXEL_SIZE				4


#define BACK_LAYER		0
#define FRONT_LAYER		1
#define BOTH_LAYER		2
#define NO_LAYER		3

#define COMP_GANTRY		0
#define COMP_HEADS		1
#define COMP_BLOCKS		2
#define	COMP_NO			3

class CCrystalDlg;
class CDailyQCDoc;
struct CompareData;
struct PlotDataRange
{
	//double min_x;
	CTime min_x;
	double min_y;
	//double max_x;
	CTime max_x;
	double max_y;
};
struct PlotData
{
	CTime  xt;
	// double x;
	double y;
};
class CDailyQCView : public CScrollView
{
protected: // create from serialization only
	CDailyQCView();
	DECLARE_DYNCREATE(CDailyQCView)

// Attributes
public:
	CDailyQCDoc* GetDocument() const;
	int m_dViewResult;
	int m_dViewDetail;
	// size of total graphical area
	CSize m_sizeTotalArea;
	CSize m_sizePage;
	CSize m_sizeLine;
	//CSize m_sizeOldViewSize;
	//CSize m_sizeNewViewSize;
	TEXTMETRIC tm, tm_printer;
	int block_size;
	CCrystalDlg* m_pCrystalDlg;
	int m_dSelectedHead;
	int m_dSelectedBlock;
	int m_dLinesPerPage;
	int m_dPrinterLineHeight;
	int m_dPrintPages;
	int m_dLeftMargin;
	int m_dStartLine;
	int m_dLastLine;
	int m_dLayer;
	CToolTipCtrl * m_pViewToolTip;
	// master record for QC result comparing
	CString m_sCompMRQC;
	int m_dComp;
	BOOL m_bDecay;
	PlotDataRange range;

// Operations
public:

// Overrides
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CDailyQCView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CFont* m_pFont;
	CFont* m_pPrinterFont;
	CFont* m_pOldFont;
// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnViewSummary();
	afx_msg void OnUpdateViewSummary(CCmdUI *pCmdUI);
	afx_msg void OnViewStatistics();
	afx_msg void OnUpdateViewStatistics(CCmdUI *pCmdUI);
	afx_msg void OnViewBlockhistogram();
	afx_msg void OnUpdateViewBlockhistogram(CCmdUI *pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnDestroy();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFileComparewith();
	afx_msg void OnFileClearcomparison();
	afx_msg void OnFileExporttextfile();
	afx_msg void OnUpdateFileClearcomparison(CCmdUI *pCmdUI);
	afx_msg void OnUpdateFileExporttextfile(CCmdUI *pCmdUI);
	afx_msg void OnBlockmapFrontlayer();
	afx_msg void OnUpdateBlockmapFrontlayer(CCmdUI *pCmdUI);
	afx_msg void OnBlockmapBacklayer();
	afx_msg void OnUpdateBlockmapBacklayer(CCmdUI *pCmdUI);
	afx_msg void OnBlockmapBothlayers();
	afx_msg void OnUpdateBlockmapBothlayers(CCmdUI *pCmdUI);
	afx_msg void OnComparisonGantry();
	afx_msg void OnUpdateComparisonGantry(CCmdUI *pCmdUI);
	afx_msg void OnComparisonHeads();
	afx_msg void OnUpdateComparisonHeads(CCmdUI *pCmdUI);
	afx_msg void OnComparisonBlocks();
	afx_msg void OnUpdateComparisonBlocks(CCmdUI *pCmdUI);
	afx_msg void OnViewCrystalbitmap();
	afx_msg void OnUpdateViewCrystalbitmap(CCmdUI *pCmdUI);
	afx_msg void OnViewDetails();
	afx_msg void OnUpdateViewDetails(CCmdUI *pCmdUI);
	afx_msg void OnViewDecaycompensation();
	afx_msg void OnUpdateViewDecaycompensation(CCmdUI *pCmdUI);
	afx_msg void OnFileAddcomparison();
	afx_msg void OnUpdateFileAddcomparison(CCmdUI *pCmdUI);
	// Draw crystal map
	void CrystalMap(CDC *pDC);
	// draw statistical analysis result
	void DrawStatistics(CDC* pDC);
	// display block map
	void BlockMap(CDC* pDC);
	// draw block grid for block map and crystal map
	void DrawGrid(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number, int head);
	void DrawBorder(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number);	
	void FillBlock(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number, int head);
	void FillCrystal(CDC *pDC, int org_x, int org_y, int offset_x, int offset_y,
							int x_number, int y_number, int head);
	// display text output
	int OutTextString(CDC* pDC);	
	// calculate scroll size
	void CalculateScrollSize(void);
	// draw normal distribution curve
	void DrawNCurve(CDC *pDC, double mean, double sd, double x_scale, double y_scale, 
		double x_offset,int pixel_step,CPoint org);
	double DrawBlockHist(CDC *pDC,double x_scale, double y_scale, double x_offset,
		int pixel_step,CPoint org, CDailyQCDoc* pDoc);
	void DrawCompare(CDC* pDC);
	/*void DrawCompareCurve(CDC *pDC,double max_sd, double min_sd, double max_mean, 
		double min_mean, int time_axis_length, int y_axis_length, CPoint org);*/
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	// Create block tooltips
	int CreateBlockToolTips(int org_x, int org_y, int offset_x, int offset_y, 
		int x_number, int y_number, int head);
	BOOL ProduceViewToolTip( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
	// load data from doc to CompareData structure
	void LoadCompareData(CDailyQCDoc * pDoc, CompareData * pData);
	void DrawCompareGantry(CDC *pDC);
	void DrawCompareHeads(CDC * pDC);
	void DrawCompareBlocks(CDC * pDC);
	void DrawCurve(CDC * pDC, RECT area, PlotDataRange range, CString symble, PlotData *pData,
		int dataSize, COLORREF color, int master, int label, CString label_string, CPoint label_pos,
		int pen_width, int penStyle);
	void DrawCoord(CDC * pDC, RECT area, PlotDataRange range, int dir, int div, CPoint offset);
	void AddToCompareList(CompareData *masterData);
};

#ifndef _DEBUG  // debug version in DailyQCView.cpp
inline CDailyQCDoc* CDailyQCView::GetDocument() const
   { return reinterpret_cast<CDailyQCDoc*>(m_pDocument); }
#endif



