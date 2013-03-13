// DailyQCDoc.h : interface of the CDailyQCDoc class
//


#pragma once

#include "EventWord64Bit.h"
#include "ScannerDefine.h"
#include "ProgressDlg.h"
#include "PhantomDlg.h"
#include "ScanTimeDlg.h"
#include "EventWord64Bit.h"
#include <math.h>
#include "afxcoll.h"
#include <afxtempl.h>

#define BUFFER_SIZE	1024*5

// block status
#define PASS				0
#define ABOVE_3SIGMAPLUS	1
#define BELOW_3SIGMAMINUS	2
#define ABOVE_LAYER_RATIO	4
#define BELOW_LAYER_RATIO	8
#define COUNT_HIGH			16
#define COUNT_LOW			32

#define COUNT_RANGE			0.25
#define RATIO_RANGE			5

struct PassPara;
struct CompareData
{
	// event count of head
	double HeadCount[DETECTOR_HEAD_NUMBER][DOI_NUMBER];
	// block prompt count
	double BlockCount[DETECTOR_HEAD_NUMBER][BLOCK_NUMBER][DOI_NUMBER];

	// mean value of blocks
	double BlockMean;
	// standard deviation of blocks
	double BlockSD;
	// date of the scan
	CTime ScanTime;
	// source labled strength
	float SourceStrength;
	// source assayed date
	CTime SourceAssayTime;
	// file name of master record QC
	CString MRQC;
	// decay compensation
	double Decay;
};
//
class CDailyQCDoc : public CDocument
{
protected: // create from serialization only
	CDailyQCDoc();
	DECLARE_DYNCREATE(CDailyQCDoc)

// Attributes
public:

// Operations
public:

// Overrides
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CDailyQCDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:

	// tag word count
	double TagWordCount;
	// prompt count
	double PromptCount;
	// random / delayed prompt count
	double RandomCount;
	CString L64FilePath;
	// parameter tranferred to the new thread
	PassPara *para_pt;
	// event count of head
	double HeadCount[DETECTOR_HEAD_NUMBER][DOI_NUMBER];
	// event count of he4ad a/b
	double HeadA[DOI_NUMBER];
	double HeadB[DOI_NUMBER];
	// block prompt count
	double BlockCount[DETECTOR_HEAD_NUMBER][BLOCK_NUMBER][DOI_NUMBER];
	double MaxBlockCount[DOI_NUMBER+1];
	// block status
	unsigned char BlockStatus[DETECTOR_HEAD_NUMBER][BLOCK_NUMBER];
	// crystal prompt count
	double CrystalCount[DETECTOR_HEAD_NUMBER][BLOCK_TRANSAXIAL_NUMBER*CRYSTAL_TRANSAXIAL_NUMBER][BLOCK_AXIAL_NUMBER*CRYSTAL_AXIAL_NUMBER];
	double MaxCrystalCount;
	// mean value of blocks
	double BlockMean;
	// standard deviation of blocks
	double BlockSD;
	// date of the scan
	CTime ScanTime;
	// source labled strength
	float SourceStrength;
	// source assayed date
	CTime SourceAssayTime;
	// scan duration
	ULONG ScanDuration;
	// 64 list mode data integrity
	double DataIntegrity;
	// out of sync 32bit word
	double OutOfSync;
	// l64 file length
	double L64FileSize;
	// file name of master record QC
	CString MRQC;
	// decay compensation
	double Decay;

	// statistical analysis of block
	int StatisticalAnalysis(void);
	// text line list
	CStringList m_LineList[2];
	CList<CompareData, CompareData&> CompareList;

	afx_msg void OnEditSource();
	afx_msg void OnEditScantime();
	// generate text result into string list
	CStringList* GenerateTextList(int result);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	// compare block count to master record
	void CompareToMaster(void);
};

struct PassPara
{
	CDailyQCDoc * pDoc;			// document pointer
	CProgressDlg * pProg;		// progress dialog pointer
};

