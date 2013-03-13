// ScanItView.cpp : implementation of the CScanItView class
//

#include "stdafx.h"
#include "ScanIt.h"

#include "mainfrm.h"

#include "ScanItDoc.h"
#include "ScanItView.h"
#include "cdhi.h"
#include "SerialBus.h"

#include "graphprop.h"
#include "param.h"
#include "configdlg.h"
#include "commsetup.h"
#include "patient.h"
#include "Diagnostic.h"
#include "DosageInfo.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CCDHI pDHI;
extern CSerialBus pSerBus;

/////////////////////////////////////////////////////////////////////////////
// CScanItView

IMPLEMENT_DYNCREATE(CScanItView, CFormView)

BEGIN_MESSAGE_MAP(CScanItView, CFormView)
	//{{AFX_MSG_MAP(CScanItView)
	ON_COMMAND(ID_SETUP_GRAPH_PROP, OnSetupGraphProp)
	ON_COMMAND(ID_SETUP_SYS_CONFIG, OnSetupSysConfig)
	ON_WM_CTLCOLOR()
	ON_COMMAND(ID_CONTROL_CONFIG, OnControlConfig)
	ON_COMMAND(ID_SETUP_COM, OnSetCom)
	ON_COMMAND(ID_CONTROL_PATIENT, OnControlPatient)
	ON_COMMAND(ID_HELP_TOPIC, OnHelpTopic)
	ON_COMMAND(ID_DIAGNOSTIC_SHOW, OnDiagnosticShow)
	ON_COMMAND(ID_DOSAGE_INFO, OnDosageInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScanItView construction/destruction

CScanItView::CScanItView()
	: CFormView(CScanItView::IDD)
{
	//{{AFX_DATA_INIT(CScanItView)
	//}}AFX_DATA_INIT
	// TODO: add construction code here

}

CScanItView::~CScanItView()
{
	int a;
	for(a=0;a<8;a++)
		m_ClrPen[a].DeleteObject();
		//m_ClrBrush[a].DeleteObject();
	m_font.DeleteObject();
	m_BDPen.DeleteObject();

}

void CScanItView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScanItView)
	DDX_Control(pDX, IDC_EDIT_SCAN, m_SCType);
	DDX_Control(pDX, IDC_EDIT1, m_FileName);
	DDX_Control(pDX, IDC_EDIT_PATIENT_NAME, m_PName);
	DDX_Control(pDX, IDC_EDIT_TIME_REMAIN, m_Tm_Rem);
	DDX_Control(pDX, IDC_EDIT_TOTAL_EVENTS, m_Tot_Events);
	DDX_Control(pDX, IDC_EDIT_TOT_TRS, m_Trs_Tot);
	DDX_Control(pDX, IDC_EDIT_TOT_RNDS, m_Rnd_Tot);
	DDX_Control(pDX, IDC_EDIT_TIME, m_Tm);
	DDX_Control(pDX, IDC_EDIT_SINGLES, m_Sngls);
	DDX_Control(pDX, IDC_EDIT_RANDOMS, m_Rndms);
	DDX_Control(pDX, IDC_EDIT_PROMPTS, m_Prmpts);
	DDX_Control(pDX, IDC_EDIT_STAT, m_Status);
	//}}AFX_DATA_MAP
}

BOOL CScanItView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CScanItView::OnInitialUpdate()
{
	int a;
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();
	pDoc = GetDocument();
	
	// Initialize the graph
	m_LogPlot = 1;
	m_Xmin = 0;
	m_Xmax = 100;
	m_Ymin = 1;
	m_Ymax = 2;
	m_YminLin = 0;
	m_YmaxLin = 100;
	m_YminLog = 1;
	m_YmaxLog = 2;
	m_Rndm_Clr = 0;
	m_Trues_Clr = 3;
	m_Plt_Rnd_Flag = 1;
	m_Plt_Tru_Flag = 1;
	m_Move_Dur = 30;
	m_Move_En = 0;
	m_Auto_Range = 0;
	m_Move_Xmin = m_Xmin;
	m_Move_Xmax = m_Move_Xmin + m_Move_Dur;

	for(a=0;a<8;a++)
		m_ClrPen[a].CreatePen(PS_SOLID,1 ,crColors[a]);
		//m_ClrBrush[a].CreateSolidBrush(crColors[a]);
	m_BDPen.CreatePen(PS_DOT,1 ,RGB(0,0,0));
	ReadGraphIniFile();
	//pFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
	m_font.CreateFont(-20,0,0,0,FW_BOLD,TRUE,0,0,DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Times New Romans");


	ProcessIcon(FALSE);

}

/////////////////////////////////////////////////////////////////////////////
// CScanItView diagnostics

#ifdef _DEBUG
void CScanItView::AssertValid() const
{
	CFormView::AssertValid();
}

void CScanItView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CScanItDoc* CScanItView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CScanItDoc)));
	return (CScanItDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CScanItView message handlers

void CScanItView::OnDraw(CDC* pDC)
 
{
	// TODO: Add your specialized code here and/or call the base class
	//pDC  = pDC;
	m_cr = pDC->GetBkColor();
	DrawAxis(pDC);
	DrawGraph();
	
}

const COLORREF CScanItView::crColors[8]={
	RGB(255,0,0),RGB(0,255,0),RGB(0,0,255),
	RGB(255,255,0),RGB(0,255,255),RGB(255,0,255),
	RGB(255,128,0),RGB(0,128,255)
};


void CScanItView::DrawAxis(CDC *pDC)
{

    static bool plot = FALSE;
    long  x,y, xdif, ydif;
	//long Oldx, Oldy, Newx, Newy,Oldy1, Newy1;

    CRect rect;
    GetClientRect (&rect);


	if(m_Move_En == 0)
		xdif = m_Xmax-m_Xmin;
	else
		xdif = m_Move_Dur;

	ydif = m_Ymax-m_Ymin;



	//pDC->ScaleViewportExt (0,1000,0,1000);
	//pDC->ScaleWindowExt(0,1000,0,1000);
	nwidth = 1000;
	nheight = 1000;
	
	
	pDC->SetMapMode (MM_ANISOTROPIC);
	// Set the graph resolution
	pDC->SetWindowExt (nwidth, nheight);
	// Set the graph size
	pDC->SetViewportExt (rect.Width()*85/100, -rect.Height()*65/100);
	//pDC->SetViewportExt (rect.Width()*0.85, -270);
	//pDC->SetViewportExt (500, -270);
	// Position the graph in the window
	//pDC->SetViewportOrg (50,430);
	pDC->SetViewportOrg (100,rect.Height()-50);
	//Draw The Y-Axis
	pDC->SelectStockObject( BLACK_PEN );

	x = nwidth - 1010;
	y = nheight - 1010;
	pDC->MoveTo (x, y);
	x = nwidth - 1010;
	y = nheight ;
	pDC->LineTo (x, y);
	//Draw The X-Axis
	x = nwidth - 1010;
	y = nheight - 1010;
	pDC->MoveTo (x, y);
	x = nwidth ;
	y = nheight - 1010;	
	pDC->LineTo (x, y);
	
	DrawTics(pDC);	
	
}


void CScanItView::DrawGraph()
{

	//HDC hdc = ::GetDC(AfxGetMainWnd()->GetSafeHwnd());
	//CDC* pDC = CDC::FromHandle(hdc);
	CDC* pDC = GetDC();
	if (pDC != NULL)
	{
		static bool plot = FALSE;
		long  y, x, xdif, ydif;
		long Oldx, Oldy, Newx, Newy,Oldy1, Newy1;
		long xmin, xmax;
		
		pDC->SetBkColor(m_cr);

		CRect rect;
		GetClientRect (&rect);
		if(m_Move_En == 0)
		{
			xdif = m_Xmax-m_Xmin;
			xmin = m_Xmin;
			xmax = m_Xmax;
		}
		else
		{
			xdif = m_Move_Dur;
			xmin = m_Move_Xmin;
			xmax = m_Move_Xmax;
		}
		ydif = m_Ymax-m_Ymin;



		//pDC->ScaleViewportExt (0,1000,0,1000);
		//pDC->ScaleWindowExt(0,1000,0,1000);
		nwidth = 1000;
		nheight = 1000;
		
		
		pDC->SetMapMode (MM_ANISOTROPIC);
		// Set the graph resolution
		pDC->SetWindowExt (nwidth, nheight);
		// Set the graph size
		pDC->SetViewportExt (rect.Width()*85/100, -rect.Height()*65/100);
		//pDC->SetViewportExt (rect.Width()*0.85, -270);
		//pDC->SetViewportExt (500, -270);
		// Position the graph in the window
		//pDC->SetViewportOrg (50,430);
		pDC->SetViewportOrg (100,rect.Height()-50);
		//Draw The Y-Axis
		pDC->SelectStockObject( BLACK_PEN );
	/*
		x = nwidth - 1010;
		y = nheight - 1010;
		pDC->MoveTo (x, y);
		x = nwidth - 1010;
		y = nheight ;
		pDC->LineTo (x, y);
		//Draw The X-Axis
		x = nwidth - 1010;
		y = nheight - 1010;
		pDC->MoveTo (x, y);
		x = nwidth ;
		y = nheight - 1010;	
		pDC->LineTo (x, y);
		
		DrawTics(pDC);	
	*/	
		//pDC->EndPath();
		// Start Plotting here
		if (pDoc->m_AllocFlag)
		{
			// Define the pens' colors
			//CPen RedPen(PS_SOLID,1 ,crColors[0]);
			//CPen YellowPen(PS_SOLID,1 ,crColors[3]);
			
			if (m_Move_En == 0)
				y = m_Xmin;
			else
			{
				// Find the index value of x min
				y=0;
				while (pDoc->lIndex[y] < m_Move_Xmin)
					y ++;
			}

			Oldx = nwidth*(pDoc->lIndex[y] - xmin)/xdif;
			LinOrLog(pDoc->lData[y], &Oldy);
			LinOrLog(pDoc->lTrues[y], &Oldy1);

			plot = TRUE;
			for (x=y;x<pDoc->m_index;++x)
			{
				pDC->MoveTo (Oldx,Oldy);
				// get the x position
				if (pDoc->lIndex[x]==0)
					Newx = 0;
				else
					Newx = nwidth*(pDoc->lIndex[x] - xmin)/xdif;//-m_Xmin)/xdif;
				if (Newx>nwidth)
					Newx = nwidth;
				// Plot the Randoms
				if (m_Plt_Rnd_Flag !=0)
				{
					LinOrLog(pDoc->lData[x], &Newy);
					if (m_Rndm_Clr >=0 && m_Rndm_Clr < 8)
						pDC->SelectObject( &m_ClrPen[m_Rndm_Clr]);
					//else
						//pDC->SelectObject( &RedPen);
					
					pDC->LineTo (Newx,Newy);
				}
				// Plot the Trues
				if (m_Plt_Tru_Flag !=0)
				{
					LinOrLog(pDoc->lTrues[x], &Newy1);
					if (m_Trues_Clr >=0 && m_Trues_Clr < 8)
						pDC->SelectObject( &m_ClrPen[m_Trues_Clr]);
					//else
						//pDC->SelectObject( &YellowPen);
					pDC->MoveTo (Oldx,Oldy1);
					pDC->LineTo (Newx,Newy1);
				}
				Oldx = Newx;
				Oldy = Newy;
				Oldy1 = Newy1;
			}
			
		}
		ReleaseDC ( pDC );
	}


}

void CScanItView::LinOrLog(long y, long *y1)
{
	double yp;
	if (m_LogPlot == 1) //log plot
	{
		if (y > 0) 
			yp = log10((double)y);
		else
			yp = -15;
		yp = (yp - (double)m_Ymin)* (double)nheight / (double)(m_Ymax - m_Ymin);
  		*y1 = (long)yp ;
	}
	else
	{
		yp = (double)(y - m_Ymin)* (double)nheight / (double)(m_Ymax - m_Ymin) ;	
		*y1 = (long)yp;
	}
	
	// Check the graph boundries
	if (*y1>nheight)
		*y1 = nheight;

	if (*y1 < -5 )
		*y1 = -5;
}

void CScanItView::DrawTics(CDC* pDC)
{
	long  x,y,i, xdif, ydif,offset,yticno,ymaj,ytic;
	long xmin,xmax;
	float z,yval;
	//CPen BDPen(PS_DOT,1 ,RGB(0,0,0));

	CString tic;
	//CFont font;
	//font.CreateFont(-20,0,0,0,FW_BOLD,TRUE,0,0,DEFAULT_CHARSET,OUT_CHARACTER_PRECIS,CLIP_CHARACTER_PRECIS,
	//	DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Times New Romans");

	if(m_Move_En == 0)
	{
		xdif = m_Xmax-m_Xmin;
		xmin = m_Xmin;
		xmax = m_Xmax;
	}
	else
	{
		xdif = m_Move_Dur;
		xmin = m_Move_Xmin;
		xmax = m_Move_Xmax;
	}
	
	ydif = m_Ymax-m_Ymin;
	

	// x-axis ticks
	for (x=0;x<5;++x)
	{
		pDC->MoveTo (x*nwidth/4,-20);
		pDC->LineTo (x*nwidth/4,-10);
	}
	// Label the tics
	pDC->SelectObject( &m_font );
	//pDC->BeginPath();
	for (x=0;x<5;++x)
	{
		z = ((float)(x*nwidth/4)/((float)nwidth/(float)xdif))+ xmin; //+m_Xmin;
		tic.Format("%2.1f",z);
		//tic.Format("%d",(int)z);
		// X-Tic
		if (250*x < 10)
			offset = -5;
		else if (250*x <100)
			offset = -10;
		else if (250*x <1000)
			offset = -15;
		else if (250*x <10000)
			offset = -20;
		else if (z <100000)
			offset = -25;
		else if (z <1000000)
			offset = -30;
		pDC->TextOut(x*nwidth/4 + offset,-30,tic);
	}

	// Draw the y-axis tics
	if(m_LogPlot==0)
		yticno = 5;
	else
		yticno = m_Ymax-m_Ymin;
	ymaj = (nheight / yticno); // calculate distance between major ticks
	for(i=0;i<=yticno;i++)
	{
		pDC->SelectStockObject( BLACK_PEN );
		y = nheight*i/ yticno;	
		pDC->MoveTo (-20, y);
		pDC->LineTo (-10, y);
		//pDC->TextOut(x*nwidth/4 + offset,-30,tic);
	
		if (i > 0)
		{
			for (x=1;x<10;x++)	// add minor ticks
			{
				if(m_LogPlot==0)
					int fhg = 0;

				else
					ytic = (y + (ymaj / 10)) - (long)((ymaj / 10) * (pow(10. , ((double)x * 0.1))));
				if (ytic > 0 && ytic < nheight) 
				{
					pDC->MoveTo (-15, ytic);
					pDC->LineTo (-10, ytic);
				}
			}	
		}
		// Do dashed lines
		if (y > 0)
		{
			pDC->SelectObject( &m_BDPen);
			pDC->MoveTo (0, y);
			pDC->LineTo (nwidth, y);
		}
		
		if(m_LogPlot==0)
			yval = (float)m_Ymin + (float)ydif / (float)yticno * (float)i;
		else
			yval = (float)pow(10. ,(double)(m_Ymin + ydif / yticno * i));
		
		tic.Format("%2.1e",yval);
		
		pDC->SelectObject( &m_font );
		pDC->TextOut(-75,y+20,tic);
    
	}
	//font.DeleteObject();
	//BDPen.DeleteObject();

}





void CScanItView::ReadGraphIniFile()
{
	char t[256];
	CString str, newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "Graph.ini";
	
	// Open the header file
	FILE  *in_file;
	in_file = fopen(newstr,"r");
	if( in_file!= NULL )
	{	
		// Read teh file line by line and sort data
		while (fgets(t,256,in_file) != NULL )
		{			
			str.Format ("%s",t);
			if (str.Find(":=")!=-1)				
				//  What type of system
				pDHI.SortData (str, &newstr);
			if(str.Find("Graph scale (0=linear, 1=log) :=")!=-1)
			{
				m_LogPlot = atoi(newstr);
				if (m_LogPlot !=0 && m_LogPlot !=1)
					m_LogPlot = 1;
			}

			if(str.Find("Minimum X value for Graph :=")!=-1)
				m_Xmin = atol(newstr);
			if(str.Find("Maximum X value for Graph :=")!=-1)
				m_Xmax = atol(newstr);
			if(str.Find("Minimum Y value for Graph :=")!=-1)
				m_Ymin = atol(newstr);
			if(str.Find("Maximum Y value for Graph :=")!=-1)
				m_Ymax = atol(newstr);
			if(str.Find("Minimum Y linear value for Graph :=")!=-1)
				m_YminLin = atol(newstr);
			if(str.Find("Maximum Y linear value for Graph :=")!=-1)
				m_YmaxLin = atol(newstr);
			if(str.Find("Minimum Y log value for Graph :=")!=-1)
				m_YminLog = atol(newstr);
			if(str.Find("Maximum Y log value for Graph :=")!=-1)
				m_YmaxLog = atol(newstr);
			if(str.Find("Randoms Trace Color :=")!=-1)
				m_Rndm_Clr = atol(newstr);
			if(str.Find("Trues Trace Color :=")!=-1)
				m_Trues_Clr = atol(newstr);
			if(str.Find("Randoms Trace (1=enabled, 0=disable) :=")!=-1)
				m_Plt_Rnd_Flag = atol(newstr);
			if(str.Find("Trues Trace (1=enabled, 0=disable) :=")!=-1)
				m_Plt_Tru_Flag = atol(newstr);
			if(str.Find("Moving Graph (1=enabled, 0=disable) :=")!=-1)
				m_Move_En = atoi(newstr);
			if(str.Find("Moving Graph Duration :=")!=-1)
			{
				m_Move_Dur = atol(newstr);
				m_Move_Xmin = 0;
				m_Move_Xmax = m_Move_Dur;
			}
			if(str.Find("Auto Range Graph (1=enabled, 0=disable) :=")!=-1)
				m_Auto_Range = atoi(newstr);

			

		}
	}
	if( in_file!= NULL )
		fclose(in_file);

}

void CScanItView::SaveGraphIniFile()
{
	CString newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "Graph.ini";

	// Write the new  file
	FILE  *out_file;
	out_file = fopen(newstr,"w");
	
	fprintf( out_file, "Graph scale (0=linear, 1=log) := %d\n", m_LogPlot );
	fprintf( out_file, "Minimum X value for Graph := %d\n",m_Xmin);
	fprintf( out_file, "Maximum X value for Graph := %d\n",m_Xmax);
	fprintf( out_file, "Minimum Y value for Graph := %d\n",m_Ymin);
	fprintf( out_file, "Maximum Y value for Graph := %d\n",m_Ymax);
	fprintf( out_file, "Minimum Y linear value for Graph := %d\n",m_YminLin);
	fprintf( out_file, "Maximum Y linear value for Graph := %d\n",m_YmaxLin);
	fprintf( out_file, "Minimum Y log value for Graph := %d\n",m_YminLog);
	fprintf( out_file, "Maximum Y log value for Graph := %d\n",m_YmaxLog);
	fprintf( out_file, "Randoms Trace Color := %d\n",m_Rndm_Clr);
	fprintf( out_file, "Trues Trace Color := %d\n",m_Trues_Clr);
	fprintf( out_file, "Randoms Trace (1=enabled, 0=disable) := %d\n",m_Plt_Rnd_Flag);
	fprintf( out_file, "Trues Trace (1=enabled, 0=disable) := %d\n",m_Plt_Tru_Flag);
	fprintf( out_file, "Moving Graph (1=enabled, 0=disable) := %d\n",m_Move_En);
	fprintf( out_file, "Moving Graph Duration := %d\n",m_Move_Dur);
	fprintf( out_file, "Auto Range Graph (1=enabled, 0=disable) := %d\n",m_Auto_Range);




	if( out_file!= NULL )
		fclose(out_file);


}

void CScanItView::OnSetupGraphProp() 
{
	// TODO: Add your command handler code here
	CGraphProp pGraph;
	pGraph.DoModal();

	//pGraph.ShowWindow(SW_HIDE);
	//CGraphProp* pGraph = new CGraphProp;
	//pGraph->ShowWindow(SW_SHOW);

}

void CScanItView::OnSetupSysConfig() 
{
	// TODO: Add your command handler code here
	CParam	pPrm;
	pPrm.DoModal();

}




HBRUSH CScanItView::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

	HBRUSH hbr;
	hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);

	switch (pWnd->GetDlgCtrlID())
	{
	case IDC_EDIT_STAT:
		//pDC->SetBkColor(crColors[0]);
		//pDC->SetTextColor(crColors[3]);
		//hbr = (HBRUSH) m_RedBrsh;
		
		break;
	case IDC_EDIT_PROMPTS: case IDC_EDIT_TOT_TRS:
		//pDC->SetBkColor(crColors[1]);
		if (m_Trues_Clr>=0 && m_Trues_Clr<8)
			pDC->SetTextColor(crColors[m_Trues_Clr]);
		else
			m_Trues_Clr=3;
		//hbr = (HBRUSH) m_YlwBrsh;
		break;
	case IDC_EDIT_RANDOMS: case IDC_EDIT_TOT_RNDS:
		if (m_Rndm_Clr>=0 && m_Rndm_Clr<8)
			pDC->SetTextColor(crColors[m_Rndm_Clr]);
		else
			m_Rndm_Clr = 3;
		//hbr = (HBRUSH) m_RedBrsh;
		break;
	default:
		hbr = CFormView::OnCtlColor(pDC, pWnd, nCtlColor);
		break;
	}
	
	return hbr;
}

void CScanItView::OnControlConfig() 
{
	// TODO: Add your command handler code here
	CConfigDlg dlg;
	dlg.DoModal();
	
}



void CScanItView::OnSetCom() 
{
	CCommSetup dlg;
	dlg.DoModal();
}

void CScanItView::ProcessIcon(bool a)
{
	//CMainFrame *pMain = (CMainFrame*) AfxGetMainWnd(); 
	//pMain->UpdateIcon(a);

}


/*
void CScanItView::OnUpdateProcessStart(CCmdUI* pCmdUI) 
{
	// Enable or disable the start menu
	if(pDoc->m_Dn_Step <6)	
		pCmdUI->Enable(FALSE);
	else
		if(pDoc->m_RunFlag)
			pCmdUI->Enable(FALSE);
		else
			pCmdUI->Enable(TRUE);

}

void CScanItView::OnProcessStart() 
{
	
	pDoc->m_RunFlag = TRUE;
	pDoc->m_Dn_Step = 6;
}

void CScanItView::OnUpdateProcessAbort(CCmdUI* pCmdUI) 
{
	if(pDoc->m_Dn_Step <6)	
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(pDoc->m_RunFlag);
}

void CScanItView::OnProcessAbort() 
{
	pDoc->AbortScan();
	
}

void CScanItView::OnUpdateProcessStop(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if(pDoc->m_Dn_Step <6)	
		pCmdUI->Enable(FALSE);
	else
		if(!pDoc->m_RunFlag)
			pCmdUI->Enable(FALSE);
		else
			pCmdUI->Enable(TRUE);
	
}

void CScanItView::OnProcessStop() 
{
	pDoc->StopScan();

}
*/
void CScanItView::OnControlPatient() 
{
	CPatient pPtn;
	pPtn.DoModal();
	//pPtn.DestroyWindow();


}

void CScanItView::OnHelpTopic() 
{
	DWORD dwData = 0;
	
	::WinHelp(this->GetSafeHwnd(),	"Scan Control Software.HLP",HELP_CONTENTS, dwData);
	
}


void CScanItView::DrawMoveWin()
{
	CDC* pDC = GetDC();
	if (pDC != NULL)
	{
		CBrush brush1;
		CRect rect;

		pDC->SetBkColor(m_cr);

		nwidth = 1000;
		nheight = 1000;
		
		GetClientRect (&rect);

		pDC->SetMapMode (MM_ANISOTROPIC);
		pDC->SetWindowExt (nwidth, nheight);
		pDC->SetViewportExt (rect.Width()*85/100, -rect.Height()*65/100);
		pDC->SetViewportOrg (100,rect.Height()-50);
		brush1.CreateSolidBrush(m_cr);   // Blue brush.


		pDC->SelectObject(&brush1);
		PatBlt(pDC->m_hDC,-10,-10,1020,1010,PATCOPY); 	

		DrawAxis(pDC);
		//DrawGraph();
		brush1.DeleteObject();
		ReleaseDC ( pDC );
	}	
	
}

void CScanItView::HideWindow()
{
	//CPatient pPtn;
	//int nResult;

	//pPtn.EndDialog(IDOK);
	

}



void CScanItView::ProcessButton()
{

	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view
	pFrame->SendMessage(WM_ACTIVATE, 1,0);

	

}

void CScanItView::DisableCloseBtn()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	//CMenu* menu = pFrame->GetMenu();
	CMenu* pMenu = pFrame->GetSystemMenu(false);
	//CMenu* menu = CChildFrame::GetSystemMenu(false);
	
	pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

	pMenu = pFrame->GetMenu();
	CMenu* pWindowList = pMenu->GetSubMenu(0); // Position where the exit menu
	pWindowList->EnableMenuItem(ID_APP_EXIT, MF_DISABLED | MF_GRAYED);
	//BOOL x = pMenu->DestroyMenu();
	//x = pWindowList->DestroyMenu();


}

void CScanItView::OnDiagnosticShow() 
{
	pDoc->m_Dn_Step = 1;
	pDoc->m_bDownFlag = FALSE;
	pDoc->m_RunFlag = FALSE;	// return to home
	ProcessButton();
	CDiagnostic pDg;
	pDg.DoModal();
}

void CScanItView::OnDosageInfo() 
{
	CDosageInfo pDlg;
	pDlg.DoModal();

	
}
