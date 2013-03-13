// CDHI.h: interface for the CCDHI class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDHI_H__87590E40_0AB6_4A3C_BD4E_EA17E0DE75FF__INCLUDED_)
#define AFX_CDHI_H__87590E40_0AB6_4A3C_BD4E_EA17E0DE75FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//class CScanItDoc;

class CCDHI  
{
public:
	CString m_errfile;
	int DOS(char *doscmd, int head);
	int SendXCom(int comnum, int head);
	int ResetHead(int head);
	int returncode(char *resp);
	int SendCmd(char *cmdSnd,char *cmdRcv, int hd, int timeout);
	long bintohex(char *rawdata, char *hex, int count);
	void GetAppPath(CString* AppPath);
	int DownLoad(char *sourcefile, char *targetfile, int head);
	bool m_msg_err;
	bool m_LogFlag;
	bool m_Opt_Name;
	bool m_Opt_PID;
	bool m_Opt_CID;
	bool m_Opt_Num;
	//float m_Cal_Factor;
	int Upload(CString filename,CString file2beupld, int head);
	static int hex2bin(char *bindata, char *hexdata, int maxlength);
	int SendPing();
	void CheckSinZero(bool* fndFlag);
	void CheckEnHead();
	void WriteIniFile();
	void GetSingles(int head);
	void GetHeadConfig(int head, int conf);
	void InitSystem();
	void GetSysMode(int head);
	void ReadIniFile();
	void SortData(CString str, CString* newstr);
	
	CCDHI();
	~CCDHI();
	CString m_CurrentDir;
	CString m_Work_Dir;
	//CString m_Norm_File;
	//CString m_Blank_File;
	int m_err_num;
	int ComCode;
	int m_init;
	bool m_All_Hd_En;
	int m_MAX_CNFG;
	int m_TOTAL_HD;
	int m_MAX_COL;
	int m_MAX_ROW;
	bool m_PETSPECT;
	bool m_HRRT;
	bool m_Hd_En[8];
	int *m_Head_0;
	int *m_Head_1;
	int *m_Head_2;
	int *m_Head_3;
	int *m_Head_4;
	int *m_Head_5;
	int *m_Head_6;
	int *m_Head_7;
	int *m_Hd_Sin_0;
	int *m_Hd_Sin_1;
	int *m_Hd_Sin_2;
	int *m_Hd_Sin_3;
	int *m_Hd_Sin_4;
	int *m_Hd_Sin_5;
	int *m_Hd_Sin_6;
	int *m_Hd_Sin_7;

	int m_uld;
	int m_lld;
	double m_DTC;   // Dead Time Constant

	struct HRRTMode              // Declare struct type
	{
	   int   HV1;
	   int   HV2;
	   int   HV3;
	   int   HV4;
	   int   HV5;
	   float DC5;
	   float DC12;
	   float DC52;
	   float LocalT;
	   float RemoteT;
	   float DHI1;
	   float DHI2;
	   float DHI3;
	   float DHI4;
	   float DHI5;
	   float DHI6;
	   int   ref;
	   int   mode;
	   int   lld;
	   int   uld;
	   int   speed;
	   int   config;
	} HrrtMode;
	struct PETSPECTMode              // Declare struct type
	{
	   int   HV;
	   float DC5;
	   float DC24;
	   float DC52;
	   float LTI_Temp;
	   float DHI1;
	   float DHI2;
	   float DHI3;
	   float DHI4;
	   float DHI5;
	   float DHI6;
	   int   mode;
	   int   uld;
	   int   lld;
	   int   speed;
	   int   config;
	   int	 Pole_Zero;
	   int   LTI_Gain;
	} PetSpectMode;
};

#endif // !defined(AFX_CDHI_H__87590E40_0AB6_4A3C_BD4E_EA17E0DE75FF__INCLUDED_)
