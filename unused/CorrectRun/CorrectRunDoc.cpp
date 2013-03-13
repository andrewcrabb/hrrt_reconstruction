// CorrectRunDoc.cpp : implementation of the CCorrectRunDoc class
//
/* HRRT User Community modifications
   29-aug-2008 : Add interframe decay correction
                 Add scanner dependent deadtime constant
				 Change default plane efficiency from 10.0 to 1.0
				 Limit maximum plane efficiency to 1.1
*/

#include "stdafx.h"

#include "CorrectRun.h"
#include "MainFrm.h"

#include "CorrectRunView.h"
#include "CorrectRunDoc.h"

#include <math.h>
#include <float.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CCorrectRunDoc



IMPLEMENT_DYNCREATE(CCorrectRunDoc, CDocument)

BEGIN_MESSAGE_MAP(CCorrectRunDoc, CDocument)
	//{{AFX_MSG_MAP(CCorrectRunDoc)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunDoc construction/destruction

CCorrectRunDoc::CCorrectRunDoc()
{
	// TODO: add one-time construction code here

}

CCorrectRunDoc::~CCorrectRunDoc()
{
 	FreeAll();

}


BOOL CCorrectRunDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	
	// Get a pointer to the View

	CDocTemplate* pDocTemplate = GetDocTemplate();
	POSITION pos = pDocTemplate->GetFirstDocPosition();
	CDocument* pDoc = pDocTemplate->GetNextDoc(pos);

	pos = GetFirstViewPosition();
	pView =(CCorrectRunView*) pDoc->GetNextView(pos);


	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	m_Activity = 137.0f;
	m_ActiveType = 0;
	m_Volume = 8513.7f;
	m_VolumeType = 0;
	m_RAE = 2;
	m_HighLight = 0;
	m_Edge = 0.f;
	m_InPer = 5.0f;
	m_GotData = 0;
	m_FindEdge = 0;
	m_OrgYear = 2000;
	m_OrgMonth = 11;
	m_OrgDay = 7;
	m_OrgHour = 12;
	m_OrgMinute = 30;
	m_OrgSecond = 0;
	m_ClrSelect = 1;
	m_OldClrSelect = 0;
	m_TopPercent = 0.2;
	m_CallocFlag = FALSE;
	m_nAlloc = FALSE;
	m_fAlloc = FALSE;
	m_Radius = 100.0f;
	m_MasterPlane = 100;
	m_plane = 100;


	m_GotData = 0;
	m_OldClrSelect = 0;
	m_CallocFlag = FALSE;
	m_nAlloc = FALSE;
	m_fAlloc = FALSE;
	m_plane = 100;
	m_BranchFactor = 0.f;
	m_dtc = m_FrameDecay = 1.0f; // no correction

	m_ThreadDone = TRUE;
	//m_MyStr = "Idle";
	m_dimType = 4;
	m_XSize = 256;
	m_YSize = 256;
	m_ZSize = 207;
	m_ScaleFactor = 1.21875;
	m_EnergyLevel = 8.94e-6; // default deadtime constant
	m_DeadtimeConstant = 0.0; // not set
	m_HRRTFlag = TRUE;

	// Read the configuration file parameters
	GetAppPath(&m_appPath);
	GetConfigFile();
	

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CCorrectRunDoc serialization

void CCorrectRunDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunDoc diagnostics

#ifdef _DEBUG
void CCorrectRunDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CCorrectRunDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CCorrectRunDoc commands

void CCorrectRunDoc::OnFileOpen() 
{
	// TODO: Add your command handler code here
	CString strFilter;


	
	// Set the filter and set dialogfile 
	strFilter = "Header files (*.hdr)|*.hdr|HRRT Image (*.i)|*.i|All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);



	// Update the status bar
	//m_MyStr = "Enter a header file";


	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		CString strFileName,strFullPath, str;
		CFile hdrFile;
		CFileException e; 

		strFileName = dlg.GetFileName();  // strFileName contains the file name 
		strFullPath = dlg.GetPathName();  // strFullPath contains the Full Path to your file 
		str.Format("%s\n%s",strFullPath,strFileName);   
		m_HdrFileName = dlg.GetPathName();
		m_ImageFile = dlg.GetPathName();
		// Get the header and image filename
		if (m_HdrFileName.Find(".i.hdr")!=-1)
			m_ImageFile.Replace(".i.hdr", ".i");
		else
			m_HdrFileName.Format("%s.hdr",m_ImageFile);
		// Update the status bar
		//m_MyStr = "Reading " + strFileName + " header file";

		InitThread();

	}

	
}

void CCorrectRunDoc::InitThread()
{
	m_ThreadDone = FALSE;
	hThread = (HANDLE) _beginthreadex(NULL,0,BkGndProc,this,0,&tid);
}

unsigned int WINAPI CCorrectRunDoc::BkGndProc(void *pParam)
{

	CCorrectRunDoc* pDoc = (CCorrectRunDoc*)pParam;	
	
	pDoc->GetHeaderFile();
	//  Read the image file if conditions are correct
	// Diminsion the Array
	long asize;
	
	pDoc->m_ThreadDone = FALSE;
	// Update the x button
	pDoc->pView->ProcessButton();
	// No Data Available
	pDoc->m_GotData = FALSE;

	if(pDoc->m_CallocFlag)
		pDoc->FreeAll();
		
	// Calculate number of elements in the array
	asize = pDoc->m_XSize * pDoc->m_YSize * pDoc->m_ZSize ;
	switch (pDoc->m_dimType)
	{
	case 2:	// integer Type Data
		//nData.SetSize(asize);
		//if (!pDoc->m_nAlloc)
		//{
			pDoc->m_nData = (int *)calloc( asize, sizeof( int ) );
			pDoc->m_nAlloc = TRUE;
		//}
		//if(pDoc->m_fAlloc)
		//{
		//	free(pDoc->m_fData);
		//	pDoc->m_fAlloc = FALSE;
		//}
		break;
	case 4:	// float Type Data
		//lData.SetSize(asize);
		//if (!pDoc->m_fAlloc)
		//{
			pDoc->m_fData = (float *)calloc( asize, sizeof( float ) );
			pDoc->m_fAlloc = TRUE;
		//}
		//if(pDoc->m_nAlloc)
		//{
		//	free(pDoc->m_nData);
		//	pDoc->m_nAlloc = FALSE;
		//}
		break;

	default:	// There is a problem
		//cout << "The Data Type is not Specified"  << endl;
		// treat it as float data 
		pDoc->m_dimType = 4;
		//if (!pDoc->m_fAlloc)
		//{
			pDoc->m_fData = (float *)calloc( asize, sizeof( float ) );
			pDoc->m_fAlloc = TRUE;
		//}
		//if(pDoc->m_nAlloc)
		//{
		//	free(pDoc->m_nData);
		//	pDoc->m_nAlloc = FALSE;
		//}		
		break;
	}

	if (pDoc->m_HRRTFlag) 
	{
		// Reset the labels 
		pDoc->pView->m_Activity.SetWindowText (" ");
		pDoc->pView->m_Strength.SetWindowText (" ");
		pDoc->pView->m_AvgPoint.SetWindowText (" ");
		pDoc->pView->m_CalFactor.SetWindowText (" ");
		pDoc->pView->DefineColors();

		// Show the status bar
		pDoc->pView->m_ProgBar.ShowWindow(SW_SHOW);
		//m_MyStr = "Reading the Image File";
		pDoc->GetDataFile();
		//m_MyStr = "Calculating the Plane Average";
		// Average all the plane togather for now
		pDoc->CalSumAvg();
		Sleep(100);
		// Get the center point
		pDoc->FindEdge();

		for (int pln=0; pln<pDoc->m_ZSize;pln++)
		{

			// Set the plane
			pDoc->m_plane = pln;
			// Get the plane average		
			pDoc->CalPlaneAvg();
			pDoc->m_GotData = TRUE;
			// Refresh the data to the view
			switch (pDoc->pView->m_DisType)
			{
				case 0:
					pDoc->pView->UpdateBitMap();
					break;
				case 1:
					pDoc->pView->Draw3D();
					break;
			}

			// Update the progress bar
			pDoc->pView->m_ProgBar.SetPos(pln*100/pDoc->m_ZSize);
		
		}

		// Tell the program the thread is done
		pDoc->pView->m_EnblUserMenu = TRUE;
		pDoc->pView->m_ProgBar.ShowWindow(SW_HIDE);

	}
	pDoc->m_ThreadDone = TRUE;
	pDoc->pView->ProcessButton();

	return 0;
}


void CCorrectRunDoc::GetHeaderFile()
{


	CString str,NumFormat, dumstr;
	int d,m,y;
	long LLD = 400;//, ltemp;
	char t[256];
	// Open the header file
	FILE  *in_file;
	in_file = fopen(m_HdrFileName,"r");

	if( in_file!= NULL )
	{
		// Read teh file line by line and sort data
		while (fgets(t,256,in_file) != NULL )
		{			
			str.Format ("%s",t);
			//  See if it is an HRRT
			//if(str.Find("!originating system :=")!=-1) 
			//	if (str.Find ("HRRT")!=-1)
			//		m_HRRTFlag = TRUE;
			//	else
			//		m_HRRTFlag = FALSE;
			// Get the file name anyway
			//if(str.Find("name of data file :=")!=-1 )
			//	SortData (str, &m_ImageFile);
			// Get the number format
			//if(str.Find("number format :=")!=-1 )
			//{
			//	SortData (str, &NumFormat);
			//	//cout <<  (LPCTSTR)NumFormat << endl;	
			//	if (NumFormat == "signed integer")
			//		m_dimType = 2;
			//	else if (NumFormat == "float")
			//		m_dimType = 4;
			//}
			// get number of bytes per pixel
			//if(str.Find("number of bytes per pixel :=")!=-1 )
			//	SortData (str, &m_BytesPerPixel);
			// Get the second array deminsion
			//if(str.Find("matrix size [1] :=")!=-1 )
			//{
			//	SortData (str, &ltemp);
			//	m_XSize = (int)ltemp;
			//	if (m_XSize == 0) 
			//		m_XSize = 1;
			//}
			// Get the second array deminsion
			//if(str.Find("matrix size [2] :=")!=-1 )
			//{
			//	SortData (str, &ltemp);
			//	m_YSize = (int)ltemp;
			//	if (m_YSize == 0) 
			//		m_YSize = 1;
			//}
			// Get the Third array deminsion
			//if(str.Find("matrix size [3] :=")!=-1 )
			//{
			//	SortData (str, &ltemp);
			//	m_ZSize = (int)ltemp;
			//	if (m_ZSize == 0) 
			//		m_ZSize = 1;
			//}
			if(str.Find("scale factor (mm/pixel) [1] :=")!=-1)
			{
				SortData (str, &str);
				m_ScaleFactor = (float)atof(str);
				//if (m_ScaleFactor == 0) 
					//m_ScaleFactor = 1.21875;
				//	m_ScaleFactor = (float)m_XSize/312.0f;
			}
			// New Code
			if(str.Find("scaling factor (mm/pixel) [1] :=")!=-1)
			{
				SortData (str, &str);
				m_ScaleFactor = (float)atof(str);
			}
			// get the energy level
			if(str.Find("energy window lower level [1] :=")!=-1 )
			{
				SortData (str, &LLD);
				if (LLD == 250)
					m_EnergyLevel = 6.9378e-6;
				if (LLD > 250  && LLD <300)
					m_EnergyLevel = (((double)LLD - 250.0)/50.0) * (7.0677e-6 - 6.9378e-6) + 6.9378e-6;
				if (LLD == 300)
					m_EnergyLevel = 7.0677e-6;
				if (LLD > 300  && LLD <350)
					m_EnergyLevel = (((double)LLD - 300.0)/50.0) * (7.7004e-6 - 7.0677e-6 ) + 7.0677e-6;
				if (LLD == 350)
					m_EnergyLevel = 7.7004e-6;
				if (LLD > 350  && LLD <400)
					m_EnergyLevel = (((double)LLD - 350.0)/50.0) * (8.94e-6 - 7.7004e-6) + 7.7004e-6;
				if (LLD >= 400)
					m_EnergyLevel = 8.94e-6;

			}
			// Get the Date
			if(str.Find("!study date (dd:mm:yryr)")!=-1 )
			{
				SortData (str, &dumstr);
				GetDateTime(dumstr,&d,&m,&y);
				if (d>=0 && d<60)
					m_Day=d;
				else
					m_Day=0;
				if (m>0 && m<13)
					m_Month=m;
				else
					m_Month=1;

				if (y>0 )
					m_Year=y;
				else
					m_Year=2001;

			}
			// Get the time
			if(str.Find("!study time (hh:mm:ss")!=-1 )
			{
				SortData (str, &dumstr);
				GetDateTime(dumstr,&d,&m,&y);
				if (d>0 && d<25)
					m_Hour=d;
				else
					m_Hour=0;
				if (m>=0 && m<60)
					m_Minute = m;
				else
					m_Minute = 0;

				if  (y>=0 && y<60)
					m_Second = y;
				else
					m_Second = 2001;

			}
				// get the Signal/Block
			if(str.Find("average singles per block")!=-1 )
				SortData (str, &m_SinPerBlock);
			if(str.Find("axial compression")!=-1 )
				SortData (str, &m_Span);
			// get the image duration
			if(str.Find("image duration")!=-1 )
			{
				SortData (str, &m_ScanDuration);
//				scan duration is now in sec not in millisec
//				m_ScanDuration /=1000;
			}
			if(str.Find("Dead time correction factor :=")!=-1 )
			{
				SortData (str, &dumstr);
				m_dtc = atof(dumstr);
				if (m_dtc < 0)	m_dtc = 0;   // will be computing from singles rate
			}
			if(str.Find("decay correction factor2")!=-1 )
			{
				SortData (str, &dumstr);
				m_FrameDecay = atof(dumstr);
			}
			if(str.Find("normalization file name used")!=-1 )
				SortData (str, &m_NormFileName);

			if(str.Find("branching factor")!=-1 )
			{
				SortData (str, &dumstr);
				m_BranchFactor = (float)atof(dumstr);
			}
			
			
		}
		if (m_ScaleFactor == 0) 
			m_ScaleFactor = (float)m_XSize/312.0f;
		// Close the header file
		fclose(in_file);
		
	}
	else
	{
		//MessageBox("Header file could not be opened to read data","Open Header File",MB_OK);
	}
}

void CCorrectRunDoc::SortData(CString str, long *num)
{
	int y = str.Find(":= ");
	int z = str.GetLength();
	*num = atoi(str.Mid(y+3,z - y - 4));
}

void CCorrectRunDoc::SortData(CString str, CString *newstr)
{
	int y = str.Find(":= ");
	int z = str.GetLength();
	*newstr = str.Mid(y+3,z - y - 4);
}

void CCorrectRunDoc::GetDateTime(CString str, int *d, int *m, int *y)
{
	//CString s;
	int x,z,w; 
	x = str.Find(":");
	z = str.GetLength();
	//s = str.Mid(0,x);
	*d = atoi(str.Mid(0,x));
	w = str.Find (":",x+1);
	//s = str.Mid(x+1,w-x-1);
	*m = atoi(str.Mid(x+1,w-x-1));
	//s = str.Mid(w+1,z-6);
	*y  = atoi(str.Mid(w+1,z-6));
}

void CCorrectRunDoc::GetDataFile()
{
	CString str;
	int per;
	BOOL a;
	DWORD NumberOfBytesRead, dummy;
	DWORD nNumberOfBytesToRead = m_XSize*m_YSize*m_ZSize;
	DWORD TotalByteToRead;
	DWORD BytesPerSector, NOPRead=0;

	
	HANDLE hFile = CreateFile ( m_ImageFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_SEQUENTIAL_SCAN, 0 );	
	DWORD err = GetLastError();
	
	GetDiskFreeSpace( NULL, &dummy, &BytesPerSector, &dummy, &dummy);


	if (BytesPerSector >0)
		BytesPerSector *=200;
	else
		BytesPerSector = 512 * 200;

	

	if (err == NO_ERROR)
	{
		if (m_dimType ==2)
			a = ReadFile( hFile, m_nData, nNumberOfBytesToRead*2, &NumberOfBytesRead, NULL);
		else if (m_dimType ==4)
			//a = ReadFile( hFile, m_fData, nNumberOfBytesToRead*4, &NumberOfBytesRead, NULL);

		// Segemted read
		{
			TotalByteToRead = m_XSize*m_YSize*m_ZSize * 4;
			nNumberOfBytesToRead = BytesPerSector;//(DWORD)pow(2,32);
			while (TotalByteToRead > 0)
			{
				a = ReadFile( hFile, &m_fData[NOPRead], nNumberOfBytesToRead, &NumberOfBytesRead, NULL);
				TotalByteToRead -=nNumberOfBytesToRead;
				NOPRead += nNumberOfBytesToRead/4;
				if(TotalByteToRead < nNumberOfBytesToRead)
					nNumberOfBytesToRead = TotalByteToRead;
				
				per = NOPRead*100/(m_XSize*m_YSize*m_ZSize);
				str.Format("%d %% read",per);
				pView->m_Status.SetWindowText(str);
				pView->m_ProgBar.SetPos(per);
			}
			pView->m_Status.SetWindowText("");
		}	
	
		a = CloseHandle(hFile);
	}
	
	else
	{
	//	cout<<"File could not be opened to read data" << endl;
	}

}

void CCorrectRunDoc::CalSumAvg()
{
	long x,y,z;
	double sum = 0.0, Diff,MyMin, MyMax;
	MyMin = DBL_MAX;
	MyMax = -1.0*DBL_MAX;
	m_DisHigh = -1.0*FLT_MAX;
	m_DisLow = FLT_MAX;
	//m_MyStr = "Averaging All Plans";
	z = m_XSize * m_YSize;
	if (!m_CallocFlag)
	{
		m_AvgPoint = (double *)calloc( z, sizeof( double ) );
		m_DisAvgPoint = (float *)calloc( z, sizeof( long ) );
		//m_DisAvgPtBk = (long *)calloc( z, sizeof( long ) );
		m_PlotOpt = (int *)calloc( z, sizeof( int) );
		m_TopView = (DWORD *)calloc( z, sizeof( DWORD) );
		m_FrontView = (DWORD *)calloc( m_XSize *m_ZSize , sizeof( DWORD) );
		m_SideView = (DWORD *)calloc( m_YSize *m_ZSize, sizeof( DWORD) );

		m_PlnAvg = (double *)calloc( m_ZSize, sizeof( double) );
		m_CalPlnFct = (double *)calloc( m_ZSize, sizeof( double) );
		m_CallocFlag = TRUE;
	}	
	for (y=0;y<z;++y)	// Each point on one plane
	{
		sum = 0.0;
		for (x=0;x<m_ZSize;++x) // Number of Planes
		
		{
			if (m_dimType ==2)			
				sum += (double)m_nData[x*z + y];
			else if (m_dimType ==4)
				sum += (double)m_fData[x*z + y];
		}
		m_AvgPoint[y] = sum/(double)m_ZSize;

	//	if (m_AvgPoint[y] > 0.0)
		{
			if (m_AvgPoint[y]>MyMax) 
				MyMax = m_AvgPoint[y];
			if (m_AvgPoint[y]<MyMin)
				MyMin = m_AvgPoint[y];
		}
		m_PlotOpt[y] = 0;
	}
	// top 2% of the max 
	Diff = m_TopPercent*MyMax;//
	for (y=0;y<z;++y)	// loop through the whole plane
	{
		m_DisAvgPoint[y] = (float)(m_AvgPoint[y] - Diff);//MyMax	* (100.0 - Diff);//- (MyMax - MyMin)/2.0;
		//m_DisAvgPtBk[y] = -1;
		//if (m_DisAvgPoint[y] <0.0) 
		//	m_DisAvgPoint[y]=0.0;

		//if (m_DisAvgPoint[y] >0.0) 
		{
			if(m_DisAvgPoint[y]>m_DisHigh)
				m_DisHigh = m_DisAvgPoint[y];
			if(m_DisAvgPoint[y]<m_DisLow)
				m_DisLow = m_DisAvgPoint[y];
		}
	}

}

void CCorrectRunDoc::SaveAvgData()
{
	CFile MyFile;
	CFileException e; 
	CString str;
	//cout << "Start Storing Binary Data"<< endl;	
	if(MyFile.Open("AvgPlane.sax" , CFile::modeWrite | CFile::modeCreate | CFile::typeBinary, &e))
	{

		long dwBytesRemaining = (long)m_XSize * (long)m_YSize * 4L;
		//MyFile.WriteHuge(&lData[index],dwBytesRemaining);
		//MyFile.WriteHuge(m_AvgPoint,dwBytesRemaining);
		MyFile.Write(m_AvgPoint,dwBytesRemaining);
		MyFile.Close();
	}
	
}


void CCorrectRunDoc::FindEdge()
{
	int x,y,i,sLine=0,eLine=0, xpt;
	float num, numM, numP;
	bool up, down;
	
	//m_MyStr = "Finding the Edge";
	i = m_XSize ;
	m_LineLen = 0;
	xpt = 0;
	for (x=0;x<m_XSize;++x)
	{
		up = FALSE;
		down=FALSE;
		for (y=0;y<m_YSize;++y)
		{
			//num = m_AvgPoint[i*x + y];
			num = m_DisAvgPoint[i*x + y];
			if (num > (m_Edge/ 10.f))
			{
				if (x>0)
					//numM = m_AvgPoint[i*(x-1) + y];
					numM = m_DisAvgPoint[i*(x-1) + y];
				if(x<m_XSize)
					//numP = m_AvgPoint[i*(x+1) + y];
					numP = m_DisAvgPoint[i*(x+1) + y];

				if(!up)
				{
					//m_AvgPoint[i*x + y] = -1;
					if (m_FindEdge)
						m_PlotOpt[i*x + y] = 1;
					else
						m_PlotOpt[i*x + y] = 0;
					sLine = y;
					xpt = x;
					up=TRUE;
				}
				if (x > 0 && numM <= m_Edge )	//&& m_PlotOpt[i*(x-1) + y] != 1)
					if (m_FindEdge)
						m_PlotOpt[i*x + y] = 1;
					else
						m_PlotOpt[i*x + y] = 0;
				if (x < m_XSize && numP <= m_Edge)	// && m_PlotOpt[i*(x-1) + y] != 1)
					if (m_FindEdge)
						m_PlotOpt[i*x + y] = 1;
					else
						m_PlotOpt[i*x + y] = 0;
			}
			else
			{
				if (up && !down) 
				{
					if (m_FindEdge)
						m_PlotOpt[i*x + y-1] = 1;
					else
						m_PlotOpt[i*x + y-1] = 0;
					eLine = y;
					down=TRUE;
				}
			}
		}
		if (up && !down) 
		{
			if (m_FindEdge)
				m_PlotOpt[i*x + y-1] = 1;
			else
				m_PlotOpt[i*x + y-1] = 0;
			eLine = y;
			down=TRUE;
		}

		if (eLine-sLine > m_LineLen)
		{
			m_LineLen = eLine-sLine;
			m_Cy = (eLine+sLine) /2;
			m_Cx = xpt;
		}
	}

	// Find the longest xLine
	xpt = 0;


	for (x=0;x<m_XSize;++x)
	{
		up = FALSE;
		down=FALSE;
		for (y=0;y<m_YSize;++y)
		{
			num = m_DisAvgPoint[x + y*i];
			if (num > ((float)m_Edge/10.f))
			{

				if(!up)
				{

					sLine = y;
					up=TRUE;
				}
			}
			else
			{
				if (up && !down) 
				{
					eLine = y;
					down=TRUE;
				}
			}
		}
		if (up && !down) 
		{
			eLine = y;
			down=TRUE;
		}

		if (eLine-sLine > xpt)
		{
			xpt = eLine-sLine;
			m_Cx = (eLine+sLine) /2;
		}
	}

}



void CCorrectRunDoc::FindInWord(int x, int y,long* num, double * avgsum)
{
	int i;

	double  radx, avgpnt=0;
	float cl;

	CString str, dumstr;
	
	//m_MyStr = "Calculating the Average Pixel";
	// Get the center of the line
	//cl = m_LineLen / 2 ;
	cl = m_Radius / m_ScaleFactor;
	//str.Format("%3.2f",cl);
	i = m_XSize ;
//	for (x=0;x<m_XSize;++x)
//	{

//		for (y=0;y<m_YSize;++y)
//		{
	double dx = m_Cx-x;
	double dy = m_Cy-y;

			m_PlotOpt[i*x + y] = 0;
			radx = sqrt(pow(dx,2) + pow(dy,2));
			//dumstr.Format("%3.2f",radx);
			if (radx < cl*(1.0-m_InPer/100.0))
			{
				// To do the Average here
				*avgsum += m_AvgPoint[i*x + y];
				*num+=1;
				// Highligh Edge if Enable
				if(m_HighLight)
					m_PlotOpt[i*x + y] = 2;
			}
			if ((int)radx == (int)cl)
				if(m_FindEdge)
					m_PlotOpt[i*x + y] = 1;
//		}
//	}
	// Update the view
	// Calculate the average counts per pixel

	//OutputDebugString("%d %f",m_plane,avgpnt);

}




void CCorrectRunDoc::RecalAvg()
{
	long y,z;
	double  Diff,MyMin, MyMax;
	MyMin = DBL_MAX;
	MyMax = -1.0*DBL_MAX;
	m_DisHigh = -1.0*FLT_MAX;
	m_DisLow = FLT_MAX;

	z = m_XSize * m_YSize;
	for (y=0;y<z;++y)	// Each point on one plane
	{		
		//if (m_AvgPoint[y] > 0.0)
		{
			if (m_AvgPoint[y]>MyMax) 
				MyMax = m_AvgPoint[y];
			if (m_AvgPoint[y]<MyMin)
				MyMin = m_AvgPoint[y];
		}
		m_PlotOpt[y] = 0;
	}
	// top % of the max 
	Diff = m_TopPercent*MyMax;//
	for (y=0;y<z;++y)	// loop through the whole plane
	{
		m_DisAvgPoint[y] = (float)(m_AvgPoint[y] - Diff);//MyMax	* (100.0 - Diff);//- (MyMax - MyMin)/2.0;
		//if (m_DisAvgPoint[y] <0.0) 
		//	m_DisAvgPoint[y]=0.0;
		if (m_DisAvgPoint[y] <0.0)
		{

			int sd = 0;
		}
		//if (m_DisAvgPoint[y] >0.0) 
		{
			if(m_DisAvgPoint[y]>m_DisHigh)
				m_DisHigh = m_DisAvgPoint[y];
			if(m_DisAvgPoint[y]<m_DisLow)
				m_DisLow = m_DisAvgPoint[y];
			if (m_DisAvgPoint[y]>MyMax) 
				MyMax = m_DisAvgPoint[y];
			if (m_DisAvgPoint[y]<MyMin)
				MyMin = m_DisAvgPoint[y];
			m_PlotOpt[y] = 0;
		}
	}

}


void CCorrectRunDoc::CalPlaneAvg()
{
	long i,j,x,y,z;
	double  MyMin=0, MyMax=0, avgpnt=0;
	static long num;
	static double avgsum;
	MyMin = DBL_MAX;
	MyMax = -1.0*DBL_MAX;
	m_DisHigh = -1.0*FLT_MAX;
	m_DisLow = FLT_MAX;
	//m_MyStr = "Averaging All Plans";
	z = m_XSize * m_YSize;
	if (!m_CallocFlag)
	{
		m_AvgPoint = (double *)calloc( z, sizeof( double ) );
		m_DisAvgPoint = (float *)calloc( z, sizeof( long ) );
		//m_DisAvgPtBk = (long *)calloc( z, sizeof( long ) );
		m_PlotOpt = (int *)calloc( z, sizeof( int) );
		m_TopView = (DWORD *)calloc( z, sizeof( DWORD) );
		m_FrontView = (DWORD *)calloc( z, sizeof( DWORD) );
		m_SideView = (DWORD *)calloc( z, sizeof( DWORD) );

		m_PlnAvg = (double *)calloc( m_ZSize, sizeof( double) );
		m_CalPlnFct = (double *)calloc( m_ZSize, sizeof( double) );
		
		
		m_CallocFlag = TRUE;
	}	
	for (y=0;y<z;++y)	// Each point on one plane
	{

		if (m_dimType ==2)			
			m_AvgPoint[y] = (double)m_nData[m_plane*z + y];
		else if (m_dimType ==4)
			m_AvgPoint[y] = (double)m_fData[m_plane*z + y];	
		if (m_AvgPoint[y]>MyMax) 
			MyMax = m_AvgPoint[y];
		if (m_AvgPoint[y]<MyMin)
			MyMin = m_AvgPoint[y];
		m_PlotOpt[y] = 0;
	}

	m_DisHigh = (float)MyMax;
	m_DisLow = (float)MyMin;
	
	CString str;
	str.Format("Cx = %d",m_Cx);
	pView->m_Activity.SetWindowText (str);
	str.Format("Cy = %d",m_Cy);
	pView->m_Strength.SetWindowText (str);

	pView->DefineScale();
	// top 2% of the max 
	//Diff = m_TopPercent*MyMax;//
	i = m_XSize;
	j = m_XSize * m_YSize;
	num = 0;
	avgsum = 0.0;
	for (x=0;x<m_XSize;++x)
	{

		for (y=0;y<m_YSize;++y)
		{
	
	
	
/*	for (y=0;y<z;++y)	// loop through the whole plane
	{


		m_DisAvgPoint[y] = (float)(m_AvgPoint[y] - Diff);//MyMax	* (100.0 - Diff);//- (MyMax - MyMin)/2.0;
		//m_DisAvgPtBk[y] = -1;
		//if (m_DisAvgPoint[y] <0.0) 
		//	m_DisAvgPoint[y]=0.0;

		//if (m_DisAvgPoint[y] >0.0) 
		{
			if(m_DisAvgPoint[y]>m_DisHigh)
				m_DisHigh = m_DisAvgPoint[y];
			if(m_DisAvgPoint[y]<m_DisLow)
				m_DisLow = m_DisAvgPoint[y];
		}
*/		

			FindInWord(x, y,&num, &avgsum);
			//m_TopView[i*x + y] = pView->RenderData(m_HighLight,m_AvgPoint[i*x + y]);
			m_TopView[i*x + y] = pView->RenderData(m_PlotOpt[i*x + y],m_AvgPoint[i*x + y]);


		}
	}

	avgpnt = avgsum/(double)num;
	
	m_PlnAvg[m_plane] = avgpnt;
	if (m_plane == m_MasterPlane)
		m_MstPlnAvg = avgpnt;

	//CString str;
	str.Format("Average = %3.2f",avgpnt);
	pView->m_AvgPoint.SetWindowText (str);
	str.Format("Plane = %d", m_plane);
	pView->m_PerLbl.SetWindowText(str);
/*
  //side and front view
	for (z=0;z<m_ZSize;z++)
	{
		for (y=0;y<m_YSize;y++)
			{

			m_FrontView[ y + z*m_XSize] = pView->RenderData(0,m_fData[j*z+y+m_plane*m_XSize]);//pView->RenderData(0,m_fData[j*z+y*m_XSize+m_plane]);
			m_SideView[ y + z*m_YSize] = pView->RenderData(0,m_fData[j*z+y*m_YSize+m_plane]);
			//else
			//	m_FrontView[i*x + y] = pView->Color[255];

		}
	}
*/
}


void CCorrectRunDoc::StoreFactor()
{
	int i;
	static char *doses[] =
	{
		"C-11",
		"F-18",
		"Ge-68"
	};
	static char *Units[] = 
	{
		"Mega-Bq",
		"Milli-Ci"	
	};
	static char *Vol[] = 
	{
		"cc",
		"liter"	
	};	

	FILE* out;
	if (m_NrmFileName.IsEmpty())
		m_NrmFileName = "EffFactor.hdr";
	out = fopen(m_NrmFileName,"w");
	if (out != NULL)
	{
		// Originating system
		fprintf( out, "!originating system := HRRT\n");
		fprintf( out, "!name of data file := %s\n",m_ImageFile);
		// Save scan date
		fprintf( out, "!study date (dd:mm:yryr) := %02d:%02d:%4d\n", m_Day, m_Month, m_Year);
		// save scan time
		fprintf( out, "!study time (hh:mm:ss GMT) := %02d:%02d:%02d\n", m_Hour, m_Minute, m_Second);		
		fprintf( out, "data format := normalization\n");
		fprintf( out, "number format := float\n");
		fprintf( out, "number of bytes per pixel := 4\n");
		fprintf( out, "number of dimensions := 3\n");
		fprintf( out, "matrix size [1] := 256\n");
		fprintf( out, "matrix size [2] := 288\n");
		if (m_Span == 9)
			fprintf( out, "matrix size [3] := 2209\n");		
		else
			fprintf( out, "matrix size [3] := 6367\n");		
		// Image duration
		fprintf(out,"image duration := %d\n",m_ScanDuration);
		fprintf(out, "decay correction factor2 := %g\n", m_FrameDecay);
		if (m_DeadtimeConstant>0)
			fprintf(out, "deadtime constant := %g\n", m_DeadtimeConstant);
		else fprintf(out, "deadtime constant := %g\n", m_EnergyLevel);
		fprintf(out, "dead time correction := %g\n", m_dtc);
		
		// dose assay date 
		fprintf( out, "sample assayed date (dd:mm:yryr) := %02d:%02d:%4d\n", m_OrgDay,m_OrgMonth,m_OrgYear);
		// dose assay time
		fprintf( out, "sample assayed time (hh:mm:ss) := %02d:%02d:%02d\n", m_OrgHour,m_OrgMinute, m_OrgSecond);		
		// dose type
		fprintf(out,"dose type := %s\n",doses[m_RAE]);
		fprintf(out,"original dose strength (%s) := %3.2f\n", Units[m_ActiveType],m_Activity);
		fprintf(out,"calculated dose strength (%s) := %3.2f\n", Units[m_ActiveType],m_NewActivity);
		

		// convert volume units to cc
		if (m_VolumeType == 0) 
		fprintf( out, "phantom volume (ml) := %g\n", m_Volume);
		else fprintf( out, "phantom volume (ml) := %g\n", m_Volume*1000);
		fprintf( out, "phantom radius (mm) := %3.2f\n", m_Radius);		
		// Master plane
		fprintf(out,"master plane := %d\n",m_MasterPlane);
		// Calibration Factor
		fprintf(out,"calibration factor := %g\n",m_CalFactor);
		fprintf(out,"calibration unit := Bq/ml\n");

		if (m_BranchFactor !=0.f)
			fprintf(out,"branching factor corrected := true\n");
		else
			fprintf(out,"branching factor corrected := false\n");

		fprintf(out,"SSR/DIFT span 3 := 1\n");
		fprintf(out,"SSR/DIFT span 9 := 1\n");
		fprintf(out,"FORE/DIFT span 3 := 1\n");
		fprintf(out,"FORE/DIFT span 9 := 1\n");	
		fprintf(out,"FORE/OSEM span 3 := 1\n");
		fprintf(out,"FORE/OSEM span 9 := 1\n");

		fprintf(out,"OSEM3D span 3 := 1\n");
		fprintf(out,"OSEM3D span 9 := 1\n");	
		fprintf(out,"HOSP span 3 := 1\n");
		fprintf(out,"HOSP span 9 := 1\n");

	}
	for (i=0;i<m_ZSize;i++)
	{
		if (m_PlnAvg[i] !=0)
		{
			m_CalPlnFct[i] = m_PlnAvg[m_MasterPlane]/m_PlnAvg[i];
			if (m_CalPlnFct[i] > 1.1)
				m_CalPlnFct[i] = 1.0;
		}
		else
			m_CalPlnFct[i] = 1.0;
		if (out != NULL)
			fprintf(out,"efficient factor for plane %d := %f\n",i,m_CalPlnFct[i]);
	}


	if(out != NULL)
		fclose(out);
}

void CCorrectRunDoc::CalFactor()
{

	long  num=0;
	double avgsum = 0,  avgpnt=0;
	double Strength, tottime, Factor;
	COleDateTime timeStart, timeEnd;
	CString str, dumstr;
	

	str.Format ("Average = %3.2f",(float)m_MstPlnAvg);
	pView->m_AvgPoint.SetWindowText (str);
	
	// Calculate the activity at time of scan
	timeStart.SetDateTime (m_OrgYear,m_OrgMonth,m_OrgDay,m_OrgHour,m_OrgMinute, m_OrgSecond);
	timeEnd.SetDateTime (m_Year,m_Month,m_Day,m_Hour,m_Minute, m_Second);
	// Time coming back in days 
	COleDateTimeSpan spanElapsed = timeEnd - timeStart;
	switch(m_RAE)
	{
	case 0:	//C11
		// Get the time difference in Minutes
		tottime = spanElapsed * 24.0 * 60.0;
		Factor = -1.0*tottime/20.3;
		break;
	case 1:	//F18
		// Get the time difference in hours
		tottime = spanElapsed * 24.0;
		Factor = -1.0*tottime/1.83;
		break;
	case 2:	// Ge68
		// Get the time difference in Days
		tottime = spanElapsed;
		Factor = -1.0*tottime/270.8;
		break;
	default:
		tottime = 1;
		break;
	}
	// Calculate new activity
	m_NewActivity = m_Activity * pow(2.0,Factor);
	if (m_ActiveType == 0)
		str =  "MBq";
	else
		str = "mCi";

	dumstr.Format ("Strength = %f "+str,m_NewActivity);
	pView->m_Activity.SetWindowText (dumstr);
	
	// Calculate sample strength
	Strength = 	m_NewActivity/m_Volume;
	if (m_ActiveType == 0)
		str =  "MBq/";
	else
		str = "mCi/";
	if (m_VolumeType == 0)
		str =  str + "cc";
	else
		str = str + "liter";
	dumstr.Format ("Activity = %f "+str,Strength);
	pView->m_Strength.SetWindowText (dumstr);

	if (m_dtc == 0)
	{	
		// Cal the dead time correction factor
		m_dtc = exp(m_EnergyLevel*(double)m_SinPerBlock);
	}

	// use scanner dependent deadtime constant when set 
	if (m_DeadtimeConstant>0)
		m_dtc = exp(m_DeadtimeConstant*(double)m_SinPerBlock);

	// Calculate the Calibration Factor
	//CalFactor = Strength/(avgpnt*m_ScaleFactor);
	if (m_ScanDuration != 0 && (m_dtc * m_MstPlnAvg) != 0)
	{
		m_CalFactor = Strength /(m_dtc * m_MstPlnAvg/m_ScanDuration);
		if(m_ActiveType == 0) m_CalFactor *= 1.0e6;
		else m_CalFactor *= 37.0e6;
		if (m_BranchFactor != 0.f)
			m_CalFactor *= m_BranchFactor;
		if (m_FrameDecay>1.0f) m_CalFactor /= m_FrameDecay;
	}
	else
		m_CalFactor = 1.0;

	str.Format("Cal Factor = %3.2f",m_CalFactor);
	pView->m_CalFactor.SetWindowText (str);

	// Define the color pallet
	if (m_OldClrSelect !=m_ClrSelect )
	{
		m_OldClrSelect = m_ClrSelect;
		//pView->DefineColors ();
	}
	// Display the diameter of the phantom in mm 
	//str.Format ("Diameter = %5.3f mm",m_LineLen*(float)m_ScaleFactor);
	str.Format ("Diameter = %5.3f mm",m_Radius * 2.0f);

	pView->m_Radius.SetWindowText(str); 

	// Display top percentage used to find phantom
	str.Format("Top %% Displayed = %3.1f%%", (1.-m_TopPercent)*100.);
	pView->m_PerLbl.SetWindowText(str);
	m_plane = m_MasterPlane;
	CalPlaneAvg();
	pView->Invalidate();

}

void CCorrectRunDoc::SaveConfigFile()
{
	CString newstr;

	//GetAppPath(&newstr);
	newstr 	= m_appPath +  "ConfigRun.ini";
	// Write the new  file
	FILE  *out_file;
	out_file = fopen(newstr,"w");


	if( out_file!= NULL )
	{
		fprintf( out_file, "phantom volume := %2.1f\n", m_Volume );
		fprintf( out_file, "volume units (0=cc, 1=liter) := %d\n",m_VolumeType);
		fprintf( out_file, "percent inward from phantom edge to calculate average := %2.1f\n",m_InPer);
		fprintf( out_file, "highlight %% inward option (0=disable, 1=enable) := %d\n",m_HighLight);
		fprintf( out_file, "trace the phantom edge option (0=disable, 1=enable) := %d\n",m_FindEdge);
		fprintf( out_file, "phantom edge data point threshold := %f\n",m_Edge);
		fprintf( out_file, "color scale option (0=Gray Scale, 1=Rainbow Scale) := %d\n",m_ClrSelect);

		
		fprintf( out_file, "display type option (0=contour, 1=isometric) := %d\n",pView->m_DisType);
		fprintf( out_file, "isometric scale option (0=linear, 1=log) := %d\n",pView->m_LnLg);


		fprintf( out_file, "top data values to find the phantom (0.0 < value < 1.0) := %3.2f\n",m_TopPercent);
		fprintf( out_file, "phantom radius (mm) := %3.2f\n",m_Radius);
		fprintf( out_file, "master plane := %d\n",m_MasterPlane);


		fprintf(out_file,"dose type (0=C-11, 1=F-18, 2=Ge-68) := %d\n",m_RAE);
		fprintf(out_file,"original dose strength (0=MBq, 1=mCi) := %d\n", m_ActiveType);
		
		fprintf(out_file,"activity strength := %f3.2\n", m_Activity);

		if (m_DeadtimeConstant>0)
			fprintf(out_file,"deadtime constant := %g\n", m_DeadtimeConstant);
		else fprintf(out_file, "deadtime constant := %g\n", m_EnergyLevel);

		
		// dose assay date 
		fprintf( out_file, "sample assayed date (dd:mm:yryr) := %02d:%02d:%4d\n", m_OrgDay,m_OrgMonth,m_OrgYear);
		// dose assay time
		fprintf( out_file, "sample assayed time (hh:mm:ss GMT) := %02d:%02d:%02d\n", m_OrgHour,m_OrgMinute, m_OrgSecond);		
	}



	if( out_file!= NULL )
		fclose(out_file);
}

void CCorrectRunDoc::GetAppPath(CString *AppPath)
{
	// Get the proper path
	CString str;
	char  szAppPath[MAX_PATH] = "";
	GetCurrentDirectory(MAX_PATH,szAppPath);
	str.Format("%s\\",szAppPath);
	*AppPath = str;
}

void CCorrectRunDoc::GetConfigFile()
{
	CString str,newstr;
	
	
	newstr 	= m_appPath +  "ConfigRun.ini";

	char t[256];
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
				SortData (str, &newstr);
			if(str.Find("phantom volume :=")!=-1)
				m_Volume = (float)atof(newstr);
			if(str.Find("volume units (0=cc, 1=liter) :=")!=-1)
				m_VolumeType = atoi(newstr);
			if(str.Find("percent inward from phantom edge to calculate average :=")!=-1)
				m_InPer = (float)atof(newstr);
			if(str.Find("highlight %% inward option (0=disable, 1=enable) :=")!=-1)
				m_HighLight = atoi(newstr);
			if(str.Find("trace the phantom edge option (0=disable, 1=enable) :=")!=-1)
				m_FindEdge = atoi(newstr);
			if(str.Find("phantom edge data point threshold :=")!=-1)
				m_Edge = (float)atof(newstr);
			if(str.Find("color scale option (0=Gray Scale, 1=Rainbow Scale) :=")!=-1)
				m_ClrSelect = atoi(newstr);

			if(str.Find("display type option (0=contour, 1=isometric) :=")!=-1)
				pView->m_DisType = atoi(newstr);
			if(str.Find("isometric scale option (0=linear, 1=log) :=")!=-1)
				pView->m_LnLg = atoi(newstr);
		
			
			if(str.Find("top data values to find the phantom (0.0 < value < 1.0) :=")!=-1)
				m_TopPercent = atof(newstr);
			if(str.Find("phantom radius (mm) :=")!=-1)
				m_Radius = (float)atof(newstr);
			if(str.Find("master plane :=")!=-1)
				m_MasterPlane = atoi(newstr);

			if(str.Find("dose type (0=C-11, 1=F-18, 2=Ge-68) :=")!=-1)
				m_RAE = atoi(newstr);
			if(str.Find("original dose strength (0=MBq, 1=mCi) :=")!=-1)
				m_ActiveType = atoi(newstr);
			if(str.Find("activity strength :=")!=-1)
				m_Activity = (float)atof(newstr);

			if(str.Find("sample assayed date (dd:mm:yryr) :=")!=-1)
				GetDateTime(newstr,&m_OrgDay,&m_OrgMonth,&m_OrgYear);

			if(str.Find("sample assayed time (hh:mm:ss GMT) :=")!=-1)
				GetDateTime(newstr,&m_OrgHour,&m_OrgMinute,&m_OrgSecond);

			if(str.Find("deadtime constant :=")!=-1)
			{
				m_DeadtimeConstant = (float)atof(newstr);
			}
			
		}
		// Close the header file
		fclose(in_file);
	}


}

void CCorrectRunDoc::FreeAll()
{
	if (m_CallocFlag)
	{
		free(m_AvgPoint);
		free(m_DisAvgPoint);
		//free(m_DisAvgPtBk);
		free(m_PlotOpt);
		free(m_TopView);
		free(m_FrontView);
		free (m_SideView);
		free(m_PlnAvg);
		free(m_CalPlnFct);
		
	}
	if (m_nAlloc)		
		free(m_nData);	// free reserved int data
	if(m_fAlloc)
		free(m_fData);	// free reserved long data
	m_CallocFlag = FALSE;
	m_nAlloc = FALSE;
	m_fAlloc = FALSE;
}
