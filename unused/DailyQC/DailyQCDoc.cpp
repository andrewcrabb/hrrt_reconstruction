// DailyQCDoc.cpp : implementation of the CDailyQCDoc class
//

#include "stdafx.h"
#include "DailyQC.h"

#include "DailyQCDoc.h"
#include "DailyQCView.h"
#include ".\dailyqcdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDailyQCDoc

IMPLEMENT_DYNCREATE(CDailyQCDoc, CDocument)

BEGIN_MESSAGE_MAP(CDailyQCDoc, CDocument)
	ON_COMMAND(ID_EDIT_SOURCE, OnEditSource)
	ON_COMMAND(ID_EDIT_SCANTIME, OnEditScantime)
END_MESSAGE_MAP()


// CDailyQCDoc construction/destruction

CDailyQCDoc::CDailyQCDoc()
: L64FilePath(_T(""))
, SourceStrength(0)
, SourceAssayTime(0)
{
	// TODO: add one-time construction code here
	para_pt = new PassPara;
	// all counts are zeros
	TagWordCount = 0;
	PromptCount = 0;
	RandomCount = 0;
	// all blocks are passed
	for (int i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		for (int j = 0; j< BLOCK_NUMBER; j++)
		{
			BlockStatus[i][j] = PASS;
			for (int k = 0; k<DOI_NUMBER;k++)
				BlockCount[i][j][k] = 0;
		}
	// all crystal counts are zeros
	for (int i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		for (int j = 0; j< BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER; j++)
			for (int k = 0; k< BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER; k++)
				CrystalCount[i][j][k] = 0;

	// all heads
	for (int i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		for (int j = 0 ;j < DOI_NUMBER; j++)
		{
			HeadCount[i][j] = 0;
		}
	for (int j = 0 ;j < DOI_NUMBER; j++)
	{
		HeadA[j] = 0;
		HeadB[j] = 0;
	}

	BlockMean = 0;
	BlockSD = 0; 
	DataIntegrity = 0;
	OutOfSync = 0;
	Decay = 1.0;
	CompareList.RemoveAll();
}

CDailyQCDoc::~CDailyQCDoc()
{
	delete para_pt;
	CompareList.RemoveAll();
}

UINT L64Process(LPVOID pInfo)
{
	PassPara *para = (PassPara *)pInfo;
	CDailyQCDoc* pDoc = para->pDoc;
	CProgressDlg* pDlg = para->pProg;
	CFile m_File;
	ULONGLONG file_length;
	ULONGLONG read_position = 0;
	UINT read_bytes = 0;
	EventWord32BitUnion* pBuffer32Bit;
	BOOL data_sync = TRUE;
	unsigned char xe, ai, bi, ah, bh, blka, blkb, ax, ay, bx, by;

	// pointer to 64 bit event word
	EventWord64BitStruct * m_64BitPtr;
	// pointer to 32 bit event word
	EventWord32BitUnion * m_32BitPtr;

	int Prog_pos;
	int buffer_left = 0;
	
	if (!m_File.Open(LPCTSTR (pDoc->L64FilePath), CFile::modeRead|CFile::typeBinary))
	{
		AfxMessageBox("Can not open file " + pDoc->L64FilePath);
		pDlg->m_dProcess = PROCESS_FAIL;
		// todo: close dialog box
		pDlg->EndDialog(IDOK);
		return 0;
	}

	pBuffer32Bit = new EventWord32BitUnion [BUFFER_SIZE]; 
	file_length = m_File.GetLength();
	pDoc->L64FileSize = (double) file_length;
	Prog_pos = 0;
	do
	{
		data_sync = TRUE;
		read_bytes = m_File.Read(pBuffer32Bit,BUFFER_SIZE*sizeof(EventWord32BitUnion));
		if (read_bytes >= 8)
		{
			buffer_left = read_bytes;
			// get the head of the buffer
			m_32BitPtr = pBuffer32Bit;
			
			while (buffer_left>=8)		// more than 8 bytes left
			{
				// first ignore out of sync data
				m_64BitPtr = (EventWord64BitStruct *) m_32BitPtr;
				while(!IsSynchronized(m_64BitPtr) && buffer_left >= 8)
				{
					data_sync =FALSE;
					m_32BitPtr ++;
					m_64BitPtr = (EventWord64BitStruct *) m_32BitPtr;
					buffer_left -=4;
					read_position += 4;
					pDoc->OutOfSync++;
				}
				if (!data_sync)
				{
					m_File.Seek(-buffer_left,CFile::current);
					// we change process here in case we got a whole file
					// out of sync, then we know what happens
					pDlg->m_dProcess = PROCESS_OUTSYNC;
					break;
				}

				// here we got synchronized data
				if (pDlg->m_dProcess == PROCESS_CANCEL)
					break;
				pDlg->m_dProcess = PROCESS_GOING;
				// check tagword
				if (IsTagWord(m_64BitPtr))
					pDoc->TagWordCount ++;
				else
				{
					// calculate the fan-sum for each head and each layer
					xe = GetXE(m_64BitPtr);
					ai = GetAI(m_64BitPtr);
					bi = GetBI(m_64BitPtr);
					ah = MP_A[xe];
					bh = MP_B[xe];
					if (ah>DETECTOR_HEAD_NUMBER || bh>DETECTOR_HEAD_NUMBER)
						pDoc->DataIntegrity++;
					else
					{
						pDoc->HeadCount[ah][ai]++;
						pDoc->HeadCount[bh][bi]++;
						pDoc->HeadA[ai]++;
						pDoc->HeadB[bi]++;
						// check prompt
						if (IsPrompt(m_64BitPtr))
						{
							pDoc->PromptCount ++;
							//Crystal count
							ax = GetAX(m_64BitPtr);
							ay = GetAY(m_64BitPtr);
							bx = GetBX(m_64BitPtr);
							by = GetBY(m_64BitPtr);
							blka = ax / CRYSTAL_TRANSAXIAL_NUMBER + 
								ay/CRYSTAL_AXIAL_NUMBER*BLOCK_TRANSAXIAL_NUMBER;
							blkb = bx / CRYSTAL_TRANSAXIAL_NUMBER + 
								by/CRYSTAL_AXIAL_NUMBER*BLOCK_TRANSAXIAL_NUMBER;
							pDoc->CrystalCount[ah][ax][ay]++;
							pDoc->CrystalCount[bh][bx][by]++;
							//block count
							pDoc->BlockCount[ah][blka][ai]++;
							pDoc->BlockCount[bh][blkb][bi]++;
						}
						// check random
						else
						{
							pDoc->RandomCount ++;
						}
					}
				}
				buffer_left -= 8;
				read_position += 8;
				m_32BitPtr += 2;
			}

			if (pDlg->m_dProcess == PROCESS_CANCEL) 
				break;

			if (Prog_pos != (int(read_position*100/file_length)))
			{
				pDlg->m_ProgCtrl.SetPos(int(read_position*100/file_length));
				Prog_pos = int(read_position*100/file_length);
			}
		}
	}
	while (read_bytes == BUFFER_SIZE*sizeof(EventWord32BitUnion));
	pDlg->m_ProgCtrl.SetPos(100);
	delete []pBuffer32Bit;
	m_File.Close();
	if (pDlg->m_dProcess != PROCESS_CANCEL)
	{
		// todo: close dialog box
		pDlg->EndDialog(IDOK);
	}
	return 0;
}

BOOL CDailyQCDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)
	// first check if user drag/drop a .l64 file
	if (theApp.m_strL64FileName.Right(4).MakeLower() == ".l64")
	{
		L64FilePath = theApp.m_strL64FileName;
		theApp.m_strL64FileName = "";
	}
	//otherwise select a .l64 file
	else
	{
		// Select 64 bit list mode file
		CFileDialog dlg(TRUE,NULL,"*.l64",OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
			"64bit List Mode File(*.l64)|*.l64||");
		if (dlg.DoModal()==IDOK)
		{
			L64FilePath = dlg.GetPathName(); 
		}
		else
			return FALSE;
	}
	SetPathName(LPCTSTR(L64FilePath+".dqc"));
	
	// process phantom information

	// get default phantom information
	CStdioFile cfgFile;
	// if can not find dailyqc.cfg file
	if (!cfgFile.Open(LPCTSTR ("DailyQc.cfg"), CFile::modeRead | CFile::typeText))
	{
		SourceStrength = 0;
		SourceAssayTime = CTime(0);
		MRQC = "NULL";
	}
	else
	// read dailyqc.cfg file and get the dailyqc phantom information
	{
		CString tmpstr;
		// read source strength first
		cfgFile.ReadString(tmpstr);
		sscanf(LPCTSTR(tmpstr), "%f", &SourceStrength);
		// read source assay date
		cfgFile.ReadString(tmpstr);
		int yyyy, mm, dd;
		sscanf(LPCTSTR(tmpstr), "%d-%d-%d", &yyyy, &mm, &dd);
		SourceAssayTime = CTime(yyyy, mm, dd, 0, 0, 0); 
		//read master record QC name
		cfgFile.ReadString(tmpstr);
		MRQC = tmpstr;
		cfgFile.Close();
	}
	
	// dispaly phamtom dialog box
	CPhantomDlg phantom_dlg;
	phantom_dlg.m_Strength = SourceStrength;
	phantom_dlg.m_AssayTime = SourceAssayTime;
	phantom_dlg.m_sMRQC = MRQC;
	if (phantom_dlg.DoModal() == IDOK)
	{
		SourceStrength = phantom_dlg.m_Strength;
		SourceAssayTime = phantom_dlg.m_AssayTime;
		MRQC = phantom_dlg.m_sMRQC;
	
		CStdioFile cfgFile;
		// save dailyqc.cfg file for the changes in phantom
		if (cfgFile.Open(LPCTSTR ("DailyQc.cfg"), CFile::modeCreate | CFile::modeWrite | CFile::typeText))
		{
			CString tmpstr;
			tmpstr.Format(_T("%3.4f\n"), SourceStrength);
			cfgFile.WriteString(LPCTSTR(tmpstr));
			// output source assay date as yyyy mm dd
			cfgFile.WriteString(LPCTSTR(SourceAssayTime.Format("%Y-%m-%d\n")));
			cfgFile.WriteString(LPCTSTR(MRQC));
			cfgFile.Close();
		}
	}
	// read *.hdr file to get QC scan information
	CStdioFile hdrFile;
	int scaninfo = 0;
	if (hdrFile.Open(LPCTSTR (L64FilePath+".hdr"), CFile::modeRead | CFile::typeText))
	{
		CString tmpstr;
		int yyyy, mm, dd, hh, tt, ss;
		while(hdrFile.ReadString(tmpstr))
		{
			// scan date
			if (tmpstr.Find("study date") != -1)
			{
				if (tmpstr.FindOneOf("0123456789") != -1)
				{
					tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.FindOneOf("0123456789"));
					if (sscanf(LPCTSTR(tmpstr), "%d:%d:%d", &dd, &mm, &yyyy) == 3)
						scaninfo = 1;
				}
				break;
			}
		}
		while(hdrFile.ReadString(tmpstr))
		{
			// scan time
			if (tmpstr.Find("study time") != -1)
			{
				if (tmpstr.FindOneOf("0123456789") != -1)
				{
					tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.FindOneOf("0123456789"));
					if (sscanf(LPCTSTR(tmpstr), "%d:%d:%d", &hh, &tt, &ss) == 3)
						scaninfo += 2;
				}
				break;
			}
		}
		if (scaninfo == 3)
		{
			ScanTime = CTime(yyyy,mm,dd,hh,tt,ss);
		}
		while(hdrFile.ReadString(tmpstr))
		{
			// scan duration
			if (tmpstr.Find("image duration") != -1)
			{
				if (tmpstr.FindOneOf("0123456789") != -1)
				{
					tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.FindOneOf("0123456789"));
					if (sscanf(LPCTSTR(tmpstr), "%ld", &ScanDuration) == 1)
						scaninfo += 4;
				}
				break;
			}
		}
	}
	// does not read proper scan info
	// prompt a dialog box to input the scan date
	if (scaninfo != 7)
	{
		CString tmpstr = L64FilePath;
		int yyyy, mm, dd, hh, tt, ss;
		ScanDuration = 300;
		ScanTime = CTime::GetCurrentTime();
		if (tmpstr.FindOneOf("0123456789")!=-1)
		{
			//find file name with random number
			tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.FindOneOf("0123456789"));
			tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.Find('-'));
			tmpstr = tmpstr.Right(tmpstr.GetLength()- tmpstr.FindOneOf("0123456789"));
			if (sscanf(LPCTSTR(tmpstr), "%d.%d.%d.%d.%d.%d", &yyyy, &mm, &dd,
				&hh, &tt, &ss) == 6)
				ScanTime = CTime(yyyy,mm,dd,hh,tt,ss);
		}
		AfxMessageBox("No header file found for scan information");
		OnEditScantime();
	}

	// display progress bar
	CProgressDlg dlg;
	para_pt->pDoc = this;
	para_pt->pProg = &dlg;
	dlg.m_dProcess = PROCESS_GOING;

	// create a thread to read l64 file and do analysis here
	//L64Process(para_pt);
	dlg.pThread = AfxBeginThread(L64Process,para_pt);
	if (dlg.DoModal() == IDCANCEL)
	{
		AfxMessageBox("Analysis process is canceled!");
		return FALSE;
	}
	else
	{
		switch(dlg.m_dProcess)
		{
		case PROCESS_GOING:
			AfxMessageBox("Analysis process is finished.");
			StatisticalAnalysis();
			if (MRQC!="NULL")
				CompareToMaster();
			GenerateTextList(VIEW_SUMMARY);
			GenerateTextList(VIEW_BLOCKHIST);
			SetModifiedFlag(TRUE);
			break;
		case PROCESS_FAIL:
			break;
		default:
			break;
		}
	}
	return TRUE;
}

// CDailyQCDoc serialization
void CDailyQCDoc::Serialize(CArchive& ar)
{
	int i,j,k;
	if (ar.IsStoring())
	{
		// TODO: add storing code here
		// store scan information
		ar << SourceStrength;
		ar << SourceAssayTime;
		ar << MRQC;
		ar << ScanTime;
		ar << ScanDuration;
		ar << L64FileSize;
		// store analysis result
		ar << OutOfSync;
		ar << DataIntegrity;
		ar << PromptCount;
		ar << RandomCount;
		ar << TagWordCount;
		ar << BlockMean;
		ar << BlockSD;
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_NUMBER; j++)
				for (k = 0; k<DOI_NUMBER; k++)
					ar << BlockCount[i][j][k];
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_NUMBER; j++)
				ar << BlockStatus[i][j];
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER; j++)
				for (k = 0; k< BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER; k++)
                    ar << CrystalCount[i][j][k];
		for (i = 0; i < DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< DOI_NUMBER; j++)
				ar << HeadCount[i][j];
		for (j = 0; j< DOI_NUMBER; j++)
			ar << HeadA[j];
		for (j = 0; j< DOI_NUMBER; j++)
			ar << HeadB[j];
		for (k = 0; k<DOI_NUMBER+1; k++)
			ar << MaxBlockCount[k];
		ar << MaxCrystalCount;
	}
	else
	{
		// TODO: add loading code here
		// load scan information
		ar >> SourceStrength;
		ar >> SourceAssayTime;
		ar >> MRQC;
		ar >> ScanTime;
		ar >> ScanDuration;
		ar >> L64FileSize;
		// load analysis result
		ar >> OutOfSync;
		ar >> DataIntegrity;
		ar >> PromptCount;
		ar >> RandomCount;
		ar >> TagWordCount;
		ar >> BlockMean;
		ar >> BlockSD;
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_NUMBER; j++)
				for (k = 0; k<DOI_NUMBER; k++)
					ar >> BlockCount[i][j][k];
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_NUMBER; j++)
				ar >> BlockStatus[i][j];
		for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER; j++)
				for (k = 0; k< BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER; k++)
                    ar >> CrystalCount[i][j][k];
		for (i = 0; i < DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j< DOI_NUMBER; j++)
				ar >> HeadCount[i][j];
		for (j = 0; j< DOI_NUMBER; j++)
			ar >> HeadA[j];
		for (j = 0; j< DOI_NUMBER; j++)
			ar >> HeadB[j];
		for (k = 0; k<DOI_NUMBER+1; k++)
			ar >> MaxBlockCount[k];
		ar >> MaxCrystalCount;
	}
}


// CDailyQCDoc diagnostics

#ifdef _DEBUG
void CDailyQCDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CDailyQCDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


void CDailyQCDoc::OnEditSource()
{
	// TODO: Add your command handler code here

	// display phantom information dialog for confirmation
	CPhantomDlg phantom_dlg;
	phantom_dlg.m_Strength = SourceStrength;
	phantom_dlg.m_AssayTime = SourceAssayTime;
	phantom_dlg.m_sMRQC = MRQC;
	if (phantom_dlg.DoModal() == IDOK)
	{
		SourceStrength = phantom_dlg.m_Strength;
		SourceAssayTime = phantom_dlg.m_AssayTime;
		MRQC = phantom_dlg.m_sMRQC;
	
		CStdioFile cfgFile;
		// save dailyqc.cfg file for the changes in phantom
		if (cfgFile.Open(LPCTSTR ("DailyQc.cfg"), CFile::modeCreate | CFile::modeWrite | CFile::typeText))
		{
			CString tmpstr;
			tmpstr.Format(_T("%3.4f\n"), SourceStrength);
			cfgFile.WriteString(LPCTSTR(tmpstr));
			// output source assay date as yyyy mm dd
			cfgFile.WriteString(LPCTSTR(SourceAssayTime.Format("%Y-%m-%d\n")));
			cfgFile.WriteString(LPCTSTR(MRQC));
			cfgFile.Close();
		}
		if (MRQC!="NULL")
		{
			CompareToMaster();
			GenerateTextList(VIEW_SUMMARY);
			GenerateTextList(VIEW_BLOCKHIST);
			UpdateAllViews(NULL);
		}
	}

}

void CDailyQCDoc::OnEditScantime()
{
	// TODO: Add your command handler code here
	CScanTimeDlg dlg;
	dlg.m_ScanDate = ScanTime;
	dlg.m_ScanTime = ScanTime;
	dlg.m_ScanDuration = ScanDuration;

	if (dlg.DoModal() == IDOK)
	{
		ScanTime = CTime(dlg.m_ScanDate.GetYear(),dlg.m_ScanDate.GetMonth(),
			dlg.m_ScanDate.GetDay(),dlg.m_ScanTime.GetHour(),
			dlg.m_ScanTime.GetMinute(),dlg.m_ScanTime.GetSecond());
		ScanDuration = dlg.m_ScanDuration;
	}
}

// calculate fan-sum  statistics
int CDailyQCDoc::StatisticalAnalysis(void)
{
	double sum = 0;
	double dif_sq = 0;
	int i, j, k;
	double block_count;

	BlockMean = 0;
	for (k = 0; k<DOI_NUMBER+1; k++)
		MaxBlockCount[k] = 0;
	for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
		for (j = 0; j< BLOCK_NUMBER; j++)
		{
			block_count = 0;
			for (int k = 0; k<DOI_NUMBER; k++)
			{
				sum += BlockCount[i][j][k];
				block_count = block_count + BlockCount[i][j][k];
				if (BlockCount[i][j][k]>MaxBlockCount[k])
					MaxBlockCount[k] = BlockCount[i][j][k];
			}
			if (block_count > MaxBlockCount[DOI_NUMBER])
				MaxBlockCount[DOI_NUMBER] = block_count;
		}
	BlockMean = sum / (BLOCK_NUMBER * DETECTOR_HEAD_NUMBER);

	for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
		for (j = 0; j< BLOCK_NUMBER; j++)
		{
			block_count = 0;
			for (int k = 0; k<DOI_NUMBER; k++)
				block_count = block_count + BlockCount[i][j][k];
			dif_sq += (block_count - BlockMean) * (block_count - BlockMean);
		}

	BlockSD = sqrt(dif_sq / (BLOCK_NUMBER * DETECTOR_HEAD_NUMBER - 1));

	for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
		for (j = 0; j< BLOCK_NUMBER; j++)
		{
			block_count = 0;
			for (int k = 0; k<DOI_NUMBER; k++)
				block_count = block_count + BlockCount[i][j][k];

			if (block_count > (BlockMean + 3 * BlockSD))
				BlockStatus[i][j] = BlockStatus[i][j] | ABOVE_3SIGMAPLUS;
			else
			{
				if (block_count < (BlockMean - 3 * BlockSD))
					BlockStatus[i][j] = BlockStatus[i][j] | BELOW_3SIGMAMINUS;
			}
		}

	// find the crystalcount here
	MaxCrystalCount = 0;
	for (int i = 0; i<DETECTOR_HEAD_NUMBER;i++)
		for (j = 0; j<BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER;j++)
			for (k = 0; k<BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER; k++)
				if (CrystalCount[i][j][k]>MaxCrystalCount)
					MaxCrystalCount = CrystalCount[i][j][k];
	return 0;
}

// generate text result into string list
CStringList* CDailyQCDoc::GenerateTextList(int result)
{
	char  pad[256];
	int i, j, k;
	CString tmpstr;
	double block_count;

	// space pad
	for (i = 0; i<256; i++)
	{
		pad[i] = 32;
	}

	//output summary result
	if (result == VIEW_SUMMARY)
	{

		m_LineList[0].RemoveAll();
		// produce head of summary
		m_LineList[0].AddTail(
			"==========================================================================================");
		m_LineList[0].AddTail(
			"                               HRRT Daily QC Summary");
		m_LineList[0].AddTail(
			"                               Siemens Medical Solutions, USA");
		m_LineList[0].AddTail(
			"                               Molecular Imaging");
		m_LineList[0].AddTail(
			"                               810 Innovation Drive");
		m_LineList[0].AddTail(
			"                               Knoxville, TN, 37932");
		m_LineList[0].AddTail(
			"                               Tel: 865-966-4444");
		m_LineList[0].AddTail(
			"                               Fax: 865-218-3000");
		m_LineList[0].AddTail(
			"                               Email:hrrt@cpspet.com");
		m_LineList[0].AddTail(
			"==========================================================================================");
		// produce scan info
		m_LineList[0].AddTail("");
		m_LineList[0].AddTail(
			"1. QC Scan Information");
		tmpstr = "    Scan Time: ";
		tmpstr += CString(pad, 40-tmpstr.GetLength());
		m_LineList[0].AddTail(
			tmpstr + ScanTime.Format("%#c"));
		
		tmpstr = "    Scan Duration: ";
		tmpstr +=CString(pad, 40-tmpstr.GetLength());
		tmpstr.AppendFormat("%lu Seconds", ScanDuration);
		m_LineList[0].AddTail(tmpstr) ;

		tmpstr = "    Source Strength: ";
		tmpstr +=CString(pad, 40-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.4f mCi", SourceStrength);
		m_LineList[0].AddTail(tmpstr) ;

		tmpstr = "    Source Assayed Date: ";
		tmpstr +=CString(pad, 40-tmpstr.GetLength());
		m_LineList[0].AddTail(tmpstr+ SourceAssayTime.Format("%#x")) ;

		tmpstr = "    Master Record QC: ";
		tmpstr +=CString(pad, 40-tmpstr.GetLength());
		m_LineList[0].AddTail(tmpstr+ MRQC) ;


		tmpstr = "    64 Bit List Mode File Size: ";
		tmpstr +=CString(pad, 40-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf Bytes",L64FileSize);
		m_LineList[0].AddTail(tmpstr) ;
		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
		// Statistics
		m_LineList[0].AddTail(
			"2. Block Prompt Statistics");

		tmpstr = "    Mean Value:";
		tmpstr += CString(pad, 30-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", BlockMean);
		m_LineList[0].AddTail(tmpstr);

		tmpstr = "    Standard Deviatino:";
		tmpstr += CString(pad, 30-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.4lf", BlockSD);
		m_LineList[0].AddTail(tmpstr);

		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
		//diagnostic
		BOOL pass = TRUE;
		m_LineList[0].AddTail(
			"3. Diagnostic Result");
		m_LineList[0].AddTail(
			"    Flagged Block:");
		if (MRQC == "NULL")
		{
			m_LineList[0].AddTail("          This document is a Master Record QC.");
			m_LineList[0].AddTail("          All Blocks in Master Record QC are supposed to be PASSED!");
		}
		else
		{

			for (i = 0; i< DETECTOR_HEAD_NUMBER; i++)
				for (j = 0; j< BLOCK_NUMBER; j++)
				{
					if (BlockStatus[i][j] >= ABOVE_LAYER_RATIO)
					{
						pass = FALSE;
						tmpstr = "          WARNING: ";
						tmpstr.AppendFormat("Head %d ",i);
						tmpstr += "Block ";
						tmpstr.AppendFormat("%3d",j);
						
						if (BlockStatus[i][j] & ABOVE_LAYER_RATIO)
							tmpstr += " Back Layer Count Ratio High.";
						else
						{
							if (BlockStatus[i][j] & BELOW_LAYER_RATIO)
								tmpstr += " Back Layer Count Ratio Low.";
							else
							{
								if (BlockStatus[i][j] &  COUNT_HIGH)
									tmpstr += " High Sensitivity";
								else
								{
									if (BlockStatus[i][j] & COUNT_LOW)
										tmpstr += " Low Sensitivity";
								}
							}

						}
						m_LineList[0].AddTail(tmpstr);
					}
				}
			if (pass)
				m_LineList[0].AddTail("          All Blocks Check OK!");
		}
		m_LineList[0].AddTail(
			"    Data Integrity:");
		if (DataIntegrity < 0.5)
			tmpstr = "          Data Channel Integrity Check PASSED!";
		else
		{
			tmpstr = "          WARNING: ";
			tmpstr.AppendFormat("%1.0lf 64 Bit Event Word(s) Data Integrity Check FAILED!", DataIntegrity);
		}
		m_LineList[0].AddTail(tmpstr);

		m_LineList[0].AddTail(
			"    Event Word Sync:");
		if (OutOfSync < 0.5)
			tmpstr = "          Data Channel Sync Check PASSED!";
		else
		{
			tmpstr = "          WARNING: ";
			tmpstr.AppendFormat("%1.0lf 32 Bit Event Word(s) Data Sync Check FAILED!", DataIntegrity);
		}
		m_LineList[0].AddTail(tmpstr);

		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
		// produce analysis summary
		// prompt /random /tagword
		m_LineList[0].AddTail(
			"4. Prompt / Random / TagWord");
		m_LineList[0].AddTail(
			CString(pad, 22)+"Count"+CString(pad,14)+"Percent"+CString(pad,8)+"Count-Rate(cps)");

		tmpstr = "    Prompt:";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", PromptCount);
		tmpstr += CString(pad, 42-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",PromptCount/(PromptCount+RandomCount)*100);
		tmpstr += CString(pad, 60-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf",PromptCount/ScanDuration);
		m_LineList[0].AddTail(tmpstr);

		tmpstr = "    Random:";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", RandomCount);
		tmpstr += CString(pad, 42-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",RandomCount/(PromptCount+RandomCount)*100);
		tmpstr += CString(pad, 60-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf",RandomCount/ScanDuration);
		m_LineList[0].AddTail(tmpstr);

		tmpstr = "    Total:";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", (RandomCount+PromptCount));
		tmpstr += CString(pad, 41-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",100.0);
		tmpstr += CString(pad, 60-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf",(RandomCount+PromptCount)/ScanDuration);
		m_LineList[0].AddTail(tmpstr);
	
		tmpstr = "    TagWord:";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", TagWordCount);
		m_LineList[0].AddTail(tmpstr);

		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
		// DOI
		m_LineList[0].AddTail(
			"5. Depth of Interaction Analysis");
		m_LineList[0].AddTail(
			"    Head"+CString(pad, 12)+"Layer 0"+CString(pad,14)+"Layer 1"+CString(pad,12)+"Percent");
		// todo: output counts of each head
		//calculate head count of each head and head A / B;

		for (i = 0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			tmpstr.Format("      %d",i);
			tmpstr += CString(pad, 20-tmpstr.GetLength());
			tmpstr.AppendFormat("%1.0lf", HeadCount[i][0]);
			tmpstr += CString(pad, 41-tmpstr.GetLength());
			tmpstr.AppendFormat("%1.0lf%%",HeadCount[i][1]);
			tmpstr += CString(pad, 61-tmpstr.GetLength());
			tmpstr.AppendFormat("%3.2lf%%",HeadCount[i][0]/(HeadCount[i][0]+HeadCount[i][1])*100);
			m_LineList[0].AddTail(tmpstr);
		}

		tmpstr = "      A";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", HeadA[0]);
		tmpstr += CString(pad, 41-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf%%",HeadA[1]);
		tmpstr += CString(pad, 61-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",HeadA[0]/(HeadA[0]+HeadA[1])*100);
		m_LineList[0].AddTail(tmpstr);

		tmpstr = "      B";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", HeadB[0]);
		tmpstr += CString(pad, 41-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf%%",HeadB[1]);
		tmpstr += CString(pad, 61-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",HeadB[0]/(HeadB[0]+HeadB[1])*100);
		m_LineList[0].AddTail(tmpstr);

		tmpstr = "    Total";
		tmpstr += CString(pad, 20-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf", (HeadA[0]+HeadB[0]));
		tmpstr += CString(pad, 41-tmpstr.GetLength());
		tmpstr.AppendFormat("%1.0lf%%",(HeadA[1]+HeadB[1]));
		tmpstr += CString(pad, 61-tmpstr.GetLength());
		tmpstr.AppendFormat("%3.2lf%%",(HeadA[0]+HeadB[0])
			/(HeadB[0]+HeadB[1]+HeadA[0]+HeadA[1])*100);
		m_LineList[0].AddTail(tmpstr);

		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
		// head 
		m_LineList[0].AddTail(
			"6. Detector Head Analysis");
		m_LineList[0].AddTail(
			"    Head"+CString(pad, 14)+"Count"+CString(pad,14)+"Percent"+CString(pad,8)+"Count-Rate(cps)");
		// todo: output counts of each head
		//calculate head count and layer count;
		double headc[DETECTOR_HEAD_NUMBER];
		double layerc[DOI_NUMBER];

		for (i = 0; i<DETECTOR_HEAD_NUMBER; i++)
			headc[i] = 0;
		for (j = 0; j<DOI_NUMBER; j++)
			layerc[j] = 0;

		for (i = 0; i<DETECTOR_HEAD_NUMBER; i++)
			for (j = 0; j<DOI_NUMBER; j++)
			{
				headc[i] += HeadCount[i][j];
				layerc[j] += HeadCount[i][j];
			}

		for (i = 0; i<DETECTOR_HEAD_NUMBER; i++)
		{
			tmpstr.Format("      %d",i);
			tmpstr += CString(pad, 20-tmpstr.GetLength());
			tmpstr.AppendFormat("%1.0lf", headc[i]);
			tmpstr += CString(pad, 42-tmpstr.GetLength());
			// one LOR is caused by two detected partticles in two heads. 
			// so  (PromptCount+RandomCount) must times 2
			tmpstr.AppendFormat("%3.2lf%%",headc[i]/(PromptCount+RandomCount)*50);
			tmpstr += CString(pad, 60-tmpstr.GetLength());
			tmpstr.AppendFormat("%1.0lf",headc[i]/ScanDuration);
			m_LineList[0].AddTail(tmpstr);
		}
		m_LineList[0].AddTail(
			"------------------------------------------------------------------------------------------");
	}

	//generate block histogram
	if (result == VIEW_BLOCKHIST)
	{
		m_LineList[1].RemoveAll();
		// produce head of block histogram
		m_LineList[1].AddTail(
			"==========================================================================================");
		m_LineList[1].AddTail(
			"                               HRRT Daily QC Block Histogram");
		m_LineList[1].AddTail(
			"                               Siemens Medical Solutions, USA");
		m_LineList[1].AddTail(
			"                               Molecular Imaging");
		m_LineList[1].AddTail(
			"                               810 Innovation Drive");
		m_LineList[1].AddTail(
			"                               Knoxville, TN, 37932");
		m_LineList[1].AddTail(
			"                               Tel: 865-966-4444");
		m_LineList[1].AddTail(
			"                               Fax: 865-218-3000");
		m_LineList[1].AddTail(
			"                               Email:hrrt@cpspet.com");
		m_LineList[1].AddTail(
			"==========================================================================================");
		m_LineList[1].AddTail(
            "-------------------------------------Both Layers------------------------------------------");

		for (i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		{
			tmpstr.Format("Head %d",i);
			m_LineList[1].AddTail(tmpstr);
			tmpstr = CString(pad, 7);
			for (j =0; j< BLOCK_TRANSAXIAL_NUMBER;j++)
			{
				tmpstr.AppendFormat("%d", j);
				tmpstr += CString(pad, 9);   
			}
			m_LineList[1].AddTail(tmpstr);
			for ( j = 0; j< BLOCK_AXIAL_NUMBER; j++)
			{
				tmpstr.Format("%d",j);
				tmpstr += CString(pad, 3-tmpstr.GetLength());
				for (k = 0 ; k< BLOCK_TRANSAXIAL_NUMBER; k++)
				{
					block_count = 0;
					for (int l = 0; l<DOI_NUMBER; l++)
						block_count = block_count + BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][l];
					tmpstr.AppendFormat("%1.0lf",block_count);
					tmpstr += CString(pad, 4+(k+1)*10-tmpstr.GetLength());
				}
				m_LineList[1].AddTail(tmpstr);
			}
			m_LineList[1].AddTail(
                "------------------------------------------------------------------------------------------");
		}

		m_LineList[1].AddTail(
            "-------------------------------------Front Layer------------------------------------------");
		for (i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		{
			tmpstr.Format("Head %d",i);
			m_LineList[1].AddTail(tmpstr);
			tmpstr = CString(pad, 7);
			for (j =0; j< BLOCK_TRANSAXIAL_NUMBER;j++)
			{
				tmpstr.AppendFormat("%d", j);
				tmpstr += CString(pad, 9);   
			}
			m_LineList[1].AddTail(tmpstr);
			for ( j = 0; j< BLOCK_AXIAL_NUMBER; j++)
			{
				tmpstr.Format("%d",j);
				tmpstr += CString(pad, 3-tmpstr.GetLength());
				for (k = 0 ; k< BLOCK_TRANSAXIAL_NUMBER; k++)
				{
					tmpstr.AppendFormat("%1.0lf",BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][1]);
					tmpstr += CString(pad, 4+(k+1)*10-tmpstr.GetLength());
				}
				m_LineList[1].AddTail(tmpstr);
			}
			m_LineList[1].AddTail(
                "------------------------------------------------------------------------------------------");
		}
		m_LineList[1].AddTail(
            "-------------------------------------Back Layer-------------------------------------------");

		for (i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		{
			tmpstr.Format("Head %d",i);
			m_LineList[1].AddTail(tmpstr);
			tmpstr = CString(pad, 7);
			for (j =0; j< BLOCK_TRANSAXIAL_NUMBER;j++)
			{
				tmpstr.AppendFormat("%d", j);
				tmpstr += CString(pad, 9);   
			}
			m_LineList[1].AddTail(tmpstr);
			for ( j = 0; j< BLOCK_AXIAL_NUMBER; j++)
			{
				tmpstr.Format("%d",j);
				tmpstr += CString(pad, 3-tmpstr.GetLength());
				for (k = 0 ; k< BLOCK_TRANSAXIAL_NUMBER; k++)
				{
					tmpstr.AppendFormat("%1.0lf",BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][0]);
					tmpstr += CString(pad, 4+(k+1)*10-tmpstr.GetLength());
				}
				m_LineList[1].AddTail(tmpstr);
			}
			m_LineList[1].AddTail(
                "------------------------------------------------------------------------------------------");
		}
		m_LineList[1].AddTail(
            "-------------------------------------Back Layer Ratio %-----------------------------------");
		for (i = 0; i< DETECTOR_HEAD_NUMBER ;i++)
		{
			tmpstr.Format("Head %d",i);
			m_LineList[1].AddTail(tmpstr);
			tmpstr = CString(pad, 7);
			for (j =0; j< BLOCK_TRANSAXIAL_NUMBER;j++)
			{
				tmpstr.AppendFormat("%d", j);
				tmpstr += CString(pad, 9);   
			}
			m_LineList[1].AddTail(tmpstr);
			for ( j = 0; j< BLOCK_AXIAL_NUMBER; j++)
			{
				tmpstr.Format("%d",j);
				tmpstr += CString(pad, 3-tmpstr.GetLength());
				for (k = 0 ; k< BLOCK_TRANSAXIAL_NUMBER; k++)
				{
					tmpstr.AppendFormat("%1.2lf",BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][0]/
						(BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][0]+BlockCount[i][j*BLOCK_TRANSAXIAL_NUMBER+k][1])*100);
					tmpstr += CString(pad, 4+(k+1)*10-tmpstr.GetLength());
				}
				m_LineList[1].AddTail(tmpstr);
			}
			m_LineList[1].AddTail(
                "------------------------------------------------------------------------------------------");
		}

	}

	UpdateAllViews(NULL);
	if (result == VIEW_SUMMARY)
		return &m_LineList[0];
	else
		return &m_LineList[1];
}

BOOL CDailyQCDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	// TODO:  Add your specialized creation code here
	//if (MRQC!="NULL")
	//	CompareToMaster();
	GenerateTextList(VIEW_SUMMARY);
	GenerateTextList(VIEW_BLOCKHIST);
	return TRUE;
}
// compare block count to master record
void CDailyQCDoc::CompareToMaster(void)
{
	double block_count;
	double layer_ratio;
	double master_count;
	double master_ratio;

	// need to read master record QC for block comparison
	TRY
	{
		CFile dqcFile(MRQC,CFile::modeRead);
		CArchive ar(&dqcFile,CArchive::load);
		CRuntimeClass *pDocClass  = RUNTIME_CLASS(CDailyQCDoc);
		CDailyQCDoc *pMRQCDoc = (CDailyQCDoc *)pDocClass->CreateObject();
		pMRQCDoc->Serialize(ar);
		
		// decay equation
		// A(1) = A(0)e^(-0.693t/Thalf)
		// Thalf for Ge-68 is 270.8 days
		// also consider change phantom, the activities of phantoms are not same
		CTimeSpan DecayTime = ScanTime - SourceAssayTime;
		// decay of QC scan
		Decay = SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8);
		DecayTime = pMRQCDoc->ScanTime - pMRQCDoc->SourceAssayTime;
		Decay = pMRQCDoc->SourceStrength*exp(-0.693*DecayTime.GetDays()/270.8)/ Decay;

		for (int i = 0; i< DETECTOR_HEAD_NUMBER; i++)
			for (int j = 0; j< BLOCK_NUMBER; j++)
			{
				block_count = BlockCount[i][j][0] + BlockCount[i][j][1];
				layer_ratio = BlockCount[i][j][0] / (block_count+1)*100;
				block_count = block_count * Decay;
				master_count = pMRQCDoc->BlockCount[i][j][0]+pMRQCDoc->BlockCount[i][j][1];
				master_ratio = pMRQCDoc->BlockCount[i][j][0]/master_count*100;
				if ((block_count-master_count) > (COUNT_RANGE*master_count))
				{
					BlockStatus[i][j] = BlockStatus[i][j] |COUNT_HIGH;
				}
				else
				{
					if ((master_count-block_count) > (COUNT_RANGE*master_count))
						BlockStatus[i][j] = BlockStatus[i][j] |COUNT_LOW;
				}

				if ((layer_ratio-master_ratio)>RATIO_RANGE)
				{
					BlockStatus[i][j] = BlockStatus[i][j] | ABOVE_LAYER_RATIO;
				}
				else
				{
					if ((master_ratio-layer_ratio)>RATIO_RANGE)
						BlockStatus[i][j] = BlockStatus[i][j] | BELOW_LAYER_RATIO;
				}
			}

	}
	CATCH ( CFileException, pEx )
	{
		AfxMessageBox("Master Record QC can not be located!");
		MRQC = "NULL";
	}
	END_CATCH
}
