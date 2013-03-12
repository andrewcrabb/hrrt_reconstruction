// CorrectRunView.cpp : implementation of the CCorrectRunView class
//

#include "stdafx.h"
#include "CorrectRun.h"

#include "CorrectRunDoc.h"
#include "CorrectRunView.h"
#include <math.h>
#include "MainFrm.h"
#include "plotopt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunView

IMPLEMENT_DYNCREATE(CCorrectRunView, CFormView)

BEGIN_MESSAGE_MAP(CCorrectRunView, CFormView)
	//{{AFX_MSG_MAP(CCorrectRunView)
	ON_COMMAND(IDD_USER_DIALOG, OnUserDialog)
	ON_WM_MOUSEMOVE()
	ON_UPDATE_COMMAND_UI(IDD_USER_DIALOG, OnUpdateUserDialog)
	ON_COMMAND(ID_CONTROL_PLOTOPT, OnControlPlotopt)
	ON_UPDATE_COMMAND_UI(ID_CONTROL_PLOTOPT, OnUpdateControlPlotopt)
	ON_UPDATE_COMMAND_UI(ID_APP_EXIT, OnUpdateAppExit)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*const COLORREF Color[10] = {RGB(255,0,0),RGB(0,255,0),RGB(0,0,255),RGB(255,255,0),RGB(0,255,255),RGB(255,0,255),RGB(255,128,0),RGB(0,128,255),RGB(255,0,128),RGB(128,0,255)};
HBITMAP hOldBlock[10],hColorBlock[10];
HBRUSH hOldBrush,hColorBrush;
HDC hdcMemBlock[10];*/

//int m_dxyc = 2, m_XOffSet = 200, m_YOffSet = 2;
float Level[256];

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunView construction/destruction

CCorrectRunView::CCorrectRunView()
	: CFormView(CCorrectRunView::IDD)
{
	//{{AFX_DATA_INIT(CCorrectRunView)
	//}}AFX_DATA_INIT
	// TODO: add construction code here

}

CCorrectRunView::~CCorrectRunView()
{
}

void CCorrectRunView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCorrectRunView)
	DDX_Control(pDX, IDC_PROGRESS1, m_ProgBar);
	DDX_Control(pDX, IDC_EDIT_RADUIS, m_Radius);
	DDX_Control(pDX, IDC_EDIT_PERCENT, m_PerLbl);
	DDX_Control(pDX, IDC_EDIT_STRENGTH, m_Strength);
	DDX_Control(pDX, IDC_EDIT_ACTIVITY, m_Activity);
	DDX_Control(pDX, IDC_EDIT5, m_Status);
	DDX_Control(pDX, IDC_EDIT1, m_AvgPoint);
	DDX_Control(pDX, IDC_EDIT3, m_CalFactor);
	//}}AFX_DATA_MAP
}

BOOL CCorrectRunView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CCorrectRunView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	// get a pointer to the document
	pDoc = GetDocument();
	m_ProgBar.SetRange(0,100);
	m_ClrRes = 255;
	//DefineColors();
	// Initialize Variable
	m_EnblUserMenu = FALSE;
	m_GrdDiv = 8;	
	m_DisType = 0;
	m_LnLg = 0;

}

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunView diagnostics

#ifdef _DEBUG
void CCorrectRunView::AssertValid() const
{
	CFormView::AssertValid();
}

void CCorrectRunView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CCorrectRunDoc* CCorrectRunView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CCorrectRunDoc)));
	return (CCorrectRunDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunView message handlers

void CCorrectRunView::OnDraw(CDC* pDC) 
{
	// TODO: Add your specialized code here and/or call the base class
	//CCorrectRunDoc* pDoc = GetDocument();
	m_cr = pDC->GetBkColor();
	if (pDoc->m_GotData )
	{
		switch (m_DisType)
		{
			case 0:
				UpdateBitMap();
				break;
			case 1:
				Draw3D();
				break;
		}

	}
}




void CCorrectRunView::OnUserDialog() 
{
	// TODO: Add your command handler code here
	//CCorrectRunDoc* pDoc = GetDocument();
	//int a = m_Userdlg.m_comtom.
	//m_Userdlg.m_ActivityVlu = pDoc->m_Activity;
	//m_Userdlg.m_VolumeVlu = pDoc->m_Volume ;
	//m_Userdlg.m_cmbActive.SetCurSel(pDoc->m_ActiveType);


	m_Userdlg.DoModal ();
}

void CCorrectRunView::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
//	CCorrectRunDoc* pDoc = GetDocument();

	if (pDoc->m_GotData )
	{
		if((point.x >= m_XOffSet)  && (point.x < ((m_dxyc*pDoc->m_XSize)+m_XOffSet)))
			if((point.y >= m_YOffSet)  && (point.y < ((m_dxyc*pDoc->m_YSize)+m_YOffSet)))
			{
				int xpos, ypos;
				CString str;
				xpos = (point.x-m_XOffSet)/m_dxyc;
				ypos = (point.y-m_YOffSet)/m_dxyc;
				switch (m_DisType)
				{
				case 0:

					str.Format("(X=%d,Y=%d) %f",xpos,ypos,pDoc->m_AvgPoint[xpos*pDoc->m_XSize + ypos]);
					
					break;
				case 1:
					//if (ypos*2 <= 256)
					//	str.Format("(X=%d,Y=%d) %f",xpos,ypos,pDoc->m_AvgPoint[xpos*pDoc->m_XSize + ypos*2]);
					
					break;
				}
				// Update Label
				m_Status.SetWindowText(str);
			}
	}
	CFormView::OnMouseMove(nFlags, point);
}



void CCorrectRunView::DefineColors()
{
	//CPaintDC hDC (this);
	int a;

//	CCorrectRunDoc* pDoc = GetDocument();
	//pDoc->m_MyStr = "Initializing Colors";
	switch (pDoc->m_XSize)
	{
	case 64:
		m_dxyc = 6;
		break;
	case 128:
		m_dxyc = 4;
		break;
	case 256:
		m_dxyc = 2;
		break;
	case 512:
		m_dxyc = 1;
		break;
	default:
		m_dxyc = 2;
		break;
	}
	m_XOffSet = 200;
	m_YOffSet = 4;
	switch (pDoc->m_ClrSelect)
	{
	case 0:

		for (a=0;a<m_ClrRes-1;++a)
			Color[a] = RGB(255-a,255-a,255-a);
			//Color[a] = RGB(255,a,255);
		Color[m_ClrRes] = RGB(255,255,0);
		break;
	case 1:		
		Color[0] = RGB(255,255,255);
		//for (a=1;a<=m_ClrRes/2;++a)
		for (a=1;a<=127;++a)
		{
			Color[a] = RGB(m_ClrRes-a, a*2,0);
			Color[a+127] = RGB(0,m_ClrRes-a*2,a*2);
			//Color[a] = RGB(0,a*2,255-a*2);
			//Color[a+128] = RGB(a*2,255-a*2,0);
		}
		Color[m_ClrRes] = RGB(0,0,0);
		break;
	}

}



void CCorrectRunView::OnUpdateUserDialog(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable(m_EnblUserMenu);
}





void CCorrectRunView::DefineScale()
{

	int i;
	float num;
	double a,b;
	// Get the increment of colors from the low and high of the spectrum
	num = (pDoc->m_DisHigh - pDoc->m_DisLow)/253.0f;

	if (num == 0.0)
		num = 0.001f;
	a = -1.0;
	b = log(pDoc->m_DisHigh - pDoc->m_DisLow - a) / 127.0;

	Level[0] = 0.0;
	for (i=1;i<m_ClrRes;++i)
	{
		// for contour display
		Level[i] = num*(float)i;
		// for Isometric log display
		if (m_LnLg && i < 129)
			m_logz[i-1]=  exp((double)(i-1) * b) + a; 

	}


}




void CCorrectRunView::Draw3D()
{
	bool show,shwold,Iso = FALSE;
	int i,ii,ij,ijc,j,jj,k,k8,l,l8;
	int kx,ky,kxg,kyg,lx,ly,lxg,lyg;
	int z,zo;
	int max[513];
	unsigned long UL128 = 128;


	CDC* pDC = GetDC();
	if (pDC != NULL)
	{
		CBrush brush1;

		pDC->SetBkColor(m_cr);

		// erase the old graph
		brush1.CreateSolidBrush(m_cr);   // Blue brush.


		pDC->SelectObject(&brush1);
		PatBlt(pDC->m_hDC,m_XOffSet,m_YOffSet,m_XOffSet+512,m_YOffSet+256,PATCOPY); 	

		//pDC->Draw3dRect (rect,Color[255],Color[0]);
		//pDC->Draw3dRect (m_XOffSet,m_YOffSet,256,256,Color[255],Color[0]);
		pDC->SetViewportOrg (m_XOffSet+256,m_YOffSet+ 256);
		pDC->MoveTo (0,0);pDC->LineTo (256,0);pDC->LineTo(256,-128);pDC->LineTo(0,-256);
		//pDC->LineTo (0,-128);pDC->LineTo(256,0);pDC->MoveTo (0,-128);pDC->LineTo(-256,-128);pDC->MoveTo (0,-256);
		pDC->LineTo(-256,-256); pDC->LineTo(-256,-128); pDC->LineTo(0,0); pDC->MoveTo(0,0);

		

		zo=Zlr(0,0); 
		pDC->LineTo(0,-zo); 
		ii=-m_GrdDiv;
		for (i=0; i<256; i+=m_GrdDiv) 
		{
			ii+=m_GrdDiv; z=Zlr(ii,0); 
			max[256+i]=z; 
			pDC->LineTo(i,-z);
		}
		pDC->LineTo(256,0); pDC->MoveTo(0,-zo); jj=-m_GrdDiv;
		for (j=0; j<256; j+=m_GrdDiv) 
		{
			jj+=m_GrdDiv; 
			z=Zlr(0,jj)+j/2; 
			max[256-j]=z; 
			pDC->LineTo(-j,-z);
		}
		pDC->LineTo(-256,-128);



		// Now plot rest of data with hiding algorithm 
		kxg=0; kyg=0;
		for (k=0; k<256; k+=m_GrdDiv) 
		{
			k8=k+m_GrdDiv; kx=kxg; kxg+=m_GrdDiv; ky=kyg; kyg+=m_GrdDiv; lxg=0; lyg=0;
			for (l=0; l<=k; l+=m_GrdDiv) 
			{
				l8=l+m_GrdDiv; lx=lxg; lxg+=m_GrdDiv; ly=lyg; lyg+=m_GrdDiv;
				// CONNECT PARTIAL SEGMENTS TO max[ijc] 
				z=Zlr(kx,lyg)+l8/2; show=(z>max[256+k-l8]);
				if (show) pDC->MoveTo((k-l8),-z);
				else pDC->MoveTo((k-l8),-max[256+k-l8]); ii=kx;
				for (i=k+m_GrdDiv; i<=k8; i+=m_GrdDiv) 
				{

					ii+=m_GrdDiv; z=Zlr(ii,lyg)+l8/2; ij=i-l8;
					ijc=256+ij; shwold=show; show=(z>max[ijc]);
					if (show) { max[ijc]=z; pDC->LineTo(ij,-z); }
					else 
					{
						if (shwold) pDC->LineTo(ij,-max[ijc]);
						else 
							pDC->MoveTo(ij,-max[ijc]);
					}
				}
				ii=kxg; jj=lyg;
				for (j=l8-m_GrdDiv; j>=l; j-=m_GrdDiv) 
				{
					jj-=m_GrdDiv;z=Zlr(ii,jj)+j/2; ij=k8-j;
					ijc=256+ij; shwold=show; show=(z>max[ijc]);
					if (show) { max[ijc]=z; pDC->LineTo(ij,-z); }
					else 
					{
						if (shwold) pDC->LineTo(ij,-max[ijc]);
						else pDC->MoveTo(ij,-max[ijc]);
					}
				}
				if (l<k) 
				{
					z=Zlr(lx,kyg)+k8/2; show=(z>max[256+l-k8]);
					if (show) pDC->MoveTo((l-k8),-z);
					else pDC->MoveTo((l-k8),-max[256+l-k8]); ii=lx;
					for (i=l+m_GrdDiv; i<=l8; i+=m_GrdDiv) 
					{
						ii+=m_GrdDiv; z=Zlr(ii,kyg)+k8/2; ij=i-k8;
						ijc=256+ij; shwold=show; show=(z>max[ijc]);
						if (show) { max[ijc]=z; pDC->LineTo(ij,-z); }
						else 
						{
							if (shwold) pDC->LineTo(ij,-max[ijc]);
							else pDC->MoveTo(ij,-max[ijc]);
						}
					}
					ii=lxg; jj=kyg;
					for (j=k8-m_GrdDiv; j>=k; j-=m_GrdDiv) 
					{
						jj-=m_GrdDiv; z=Zlr(ii,jj)+j/2; ij=l8-j;
						ijc=256+ij; shwold=show; show=(z>max[ijc]);
						if (show) { max[ijc]=z; pDC->LineTo(ij,-z); }
						else 
						{
							if (shwold) pDC->LineTo(ij,-max[ijc]);
							else pDC->MoveTo(ij,-max[ijc]);
						}
					}
				}
			}  // end 'for (l=...)' 
		}  // end 'for (k=...)' 
		pDC->MoveTo(0,-max[256]); pDC->LineTo(0,-256);
		brush1.DeleteObject();;
		ReleaseDC ( pDC );
	}
	//DeleteObject(hdc);
}


int CCorrectRunView::Zlr(int ii, int jj)
{

	int k,kk;
	double z;
	long xz;

//	CCorrectRunDoc* pDoc = GetDocument();

	//if (ii>255) ii = 255;
	//if (jj > 255) jj = 255;
	if (ii>pDoc->m_XSize-1) ii = pDoc->m_XSize-1;
	if (jj > pDoc->m_YSize - 1) jj = pDoc->m_YSize-1;
	if (ii<0) ii = 0;
	if (jj < 0) jj = 0;

	if (pDoc->m_DisHigh - pDoc->m_DisLow != 0)
	{
		xz = (long)(128.0*pDoc->m_AvgPoint[pDoc->m_XSize * ii + jj]/(pDoc->m_DisHigh - pDoc->m_DisLow));
		//xz = (unsigned long)(pDoc->m_AvgPoint[pDoc->m_XSize * ii + jj]);
	}
	else
		xz = 0UL;
	z = pDoc->m_AvgPoint[pDoc->m_XSize * ii + jj] + fabs(pDoc->m_DisLow);
	//if (z>0)
		
		//z-=pDoc->m_DisLow;
	//xz=(unsigned long)z;
	
	if (m_LnLg) 
	{ 
		// Log Option
		k=127;                    // successive approximation 
		if (z < 0)
			xz = 0;
		else
			for (kk=64; kk>0; kk/=2) 
			{ 
				if (z<m_logz[k-kk]) k-=kk; 
				xz=k;
			}
	
	}
	else 
	{	
		//Linear Option
		//xz>>=m_zLinScl; 
		if (xz>127) 
			xz=127;	
		if (xz<0) 
			xz=0;	
	}
	return (( int )xz);
	
}


void CCorrectRunView::UpdateBitMap()
{
	//BITMAPINFOHEADER bi;
	BITMAPINFO bi;
	int xSize, ySize;
	int xoff, yoff;
	int nRet;
	DWORD *pdw = pDoc->m_TopView;

//	CCorrectRunDoc* pDoc = GetDocument();
	
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = pDoc->m_XSize;
	bi.bmiHeader.biHeight = pDoc->m_YSize;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biSizeImage = 0;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biXPelsPerMeter = 0;
	bi.bmiHeader.biYPelsPerMeter = 0;
	bi.bmiHeader.biClrImportant = 0;
	bi.bmiHeader.biClrUsed = 0;

	xSize = pDoc->m_XSize *m_dxyc;
	ySize = pDoc->m_YSize *m_dxyc;
	xoff = m_XOffSet;
	yoff = m_YOffSet;

	CDC* pDC = GetDC();
		
	if (pDC != NULL)
	{
	//	int nRet = StretchDIBits(hdc ,m_XOffSet,m_YOffSet,pDoc->m_XSize*2,pDoc->m_YSize *2,
	//		0,0,bi.bmiHeader.biWidth,bi.bmiHeader.biHeight,pDoc->m_TopView, &bi,0,SRCCOPY);
		nRet = StretchDIBits(pDC->m_hDC ,xoff,yoff,xSize,ySize,
			0,0,bi.bmiHeader.biWidth,bi.bmiHeader.biHeight,pdw, &bi,0,SRCCOPY);
	/*
		// To put the front view
		bi.bmiHeader.biHeight = pDoc->m_ZSize;
		xoff = m_XOffSet+xSize;
		pdw = pDoc->m_FrontView;
		//yoff = m_YOffSet+ySize;

		nRet = StretchDIBits(pDC->m_hDC ,xoff,yoff,xSize,ySize,
			0,0,bi.bmiHeader.biWidth,bi.bmiHeader.biHeight,pdw, &bi,0,SRCCOPY);

		// To put the side view
		xoff = m_XOffSet+ xSize;
		yoff = m_YOffSet+ ySize;
		pdw = pDoc->m_SideView;
		//yoff = m_YOffSet+ySize;

		nRet = StretchDIBits(pDC->m_hDC ,xoff,yoff,xSize,ySize,
			0,0,bi.bmiHeader.biWidth,bi.bmiHeader.biHeight,pdw, &bi,0,SRCCOPY);
	*/
		ReleaseDC ( pDC );
		
	}
}



DWORD CCorrectRunView::RenderData(int pltopt, double num)
{

	int z,a;
	int i,j;	
	//float num;

	i = pDoc->m_XSize ;
	j = pDoc->m_YSize;
//	for (x=0;x<pDoc->m_XSize;++x)
//	{
//		for (y=0;y<pDoc->m_YSize;++y)
//		{
			if (pltopt == 0)
			{
				// Loop through all 256 levels
				//num = pDoc->m_DisAvgPoint[i*x + y];
				a = 255;
				for (z=0;z<m_ClrRes;++z)
				{
					if (num <=Level[z])
					{
						a = z;	// We found the level
						//z=270;	// Exit for
						break;
					}
				}
				
			}
			else if (pltopt == 1)
			{
				// Paint it distict color
				a = 255;
			}
			else if (pltopt == 2)
			{
				// Paint it distict color
				a = 255;
			}
			//pDoc->m_TopView[i*x + y] = Color[a];
			return Color[a];
//		}
//	}

}

void CCorrectRunView::ProcessBar(int i)
{
	CMainFrame *pMain = (CMainFrame*) AfxGetMainWnd();


	//pMain->BarChangetext(i);
}

void CCorrectRunView::OnControlPlotopt() 
{
	CPlotOpt pOpt;
	pOpt.DoModal();
	
}

void CCorrectRunView::OnUpdateControlPlotopt(CCmdUI* pCmdUI) 
{
	//pCmdUI->Enable(m_EnblUserMenu);	
}

void CCorrectRunView::OnUpdateAppExit(CCmdUI* pCmdUI) 
{
	
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	
	CMenu* pMenu = pFrame->GetSystemMenu(false);
	if(pDoc->m_ThreadDone)
	{
		pCmdUI->Enable(TRUE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED );
	}
	else
	{
		pCmdUI->Enable(FALSE);
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
	}

}

void CCorrectRunView::OnUpdateFileOpen(CCmdUI* pCmdUI) 
{

	if(pDoc->m_ThreadDone)
	{
		pCmdUI->Enable(TRUE);
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}	
}

void CCorrectRunView::ProcessButton()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	// Get a pointer to the view
		//CMenu* menu = pFrame->GetMenu();
	CMenu* pMenu = pFrame->GetSystemMenu(false);
	//CMenu* menu = CChildFrame::GetSystemMenu(false);
	


	if (pDoc->m_ThreadDone)
	{
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
	}
	else
	{
		pMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
	}
	pFrame->SendMessage(WM_ACTIVATE, 1,0);

}
