// Acquire.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Acquire.h"
#include "ScanThread.h"

#include "LmHistBeUtils.h"
#include "LmHistRebinnerInfo.h"
#include "LmHistStats.h"
#include "lmioctl.h"
#include "RbIoctl.h"
#include "ListmodeDLL.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LENGTH 1024

/*
class CCallB : public CCallback
{
public:
	static enum {Preset, Stop, Abort, Error};
	
	void notify (int error_code, char *extra_info);
	void acqComplete(int enum_flag);
	void acqPostComplete(int enum_flag, int fileWriteSuccess);
	//~CCallB();
};
*/

LM_stats pLMStats;
CCallB pCB;
CSinoInfo pSinoInfo;
Rebinner_info m_rebinnerInfo;

COleDateTime m_timeStart;
COleDateTime m_timeEnd;
COleDateTime m_timeTest;
unsigned long m_preset;
CString m_ext_fname,m_FName, m_CLogName, m_patientName,m_energyWindow,m_transSpeed;
int m_scanType;
bool m_done;

char *optarg;
int	opterr = 1;
int	optind = 1;
int	optopt;

CCDHI pDHI;
CSerialBus pSerBus;

int getopt(int argc, char **argv,char *opts);
void Prog_usage ();	// Print out Command Information


/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0, span_cfg=0, val, Scan_Mode = 0;
	int scan_type=0; /* EM=0, TX=1 */
	bool presetMode=FALSE, online=FALSE, gotit = FALSE;
	unsigned long bypass = 1;
	double tottime = 0, oldtime = 0, testtime=0.;
	char c, szAppPath[MAX_PATH] = "";
	CString str;
	bool halt_only = FALSE, batch = FALSE;
 
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << _T("Fatal Error: MFC initialization failed") << endl;
		nRetCode = 1;
	}
	else
	{
		m_FName = "";
		m_preset =  30;

	    // Get the file name  and time	

		while ((c = getopt (argc, argv, "d:t:f:m:c:p:e:s:l:h:b:D:T:F:M:C:P:E:S:L:H:B")) != EOF) 
		{
			
			switch (c) 
			{
			case 'D': case 'd':
				cout << "Got Time" << endl;
				str.Format ("%s",optarg);
				m_preset = atoi(str);
				if (m_preset <=0)
				{
					Prog_usage();
					return nRetCode;
				}
			
				break;
			case 'F': case 'f':	
				cout << "Got Filename" << endl;
				m_FName.Format ("%s",optarg);
				printf("m_FName :%s\n",m_FName);
				gotit = TRUE;
				break;

			case 'M': case 'm':	
				if (sscanf(optarg,"%d",&val) != 1)
					printf("error decoding -M %s\n",optarg);
				else				
				{
					if (val == 0)
						bypass = 0;
					else
						bypass = 1;

				}
				break;

			case 'C': case 'c':	
				if (sscanf(optarg,"%d",&val) != 1)
					printf("error decoding -C %s\n",optarg);
				else				
				{
					if (val == 0)
						span_cfg = 0;
					else
						span_cfg = 1;

				}
				break;
			case 'P': case 'p':	
				cout << "Got Patient name" << endl;
				m_patientName.Format ("%s",optarg);
				gotit = TRUE;
				break;
			case 'E': case 'e':	
				cout << "Got energy window" << endl;
				m_energyWindow.Format ("%s",optarg);
				gotit = TRUE;
				break;
			case 's': case 'S':	
				cout << "Got Transmission parameters" << endl;
				m_transSpeed.Format ("%s",optarg);
				gotit = TRUE;
				break;
			case 'T': case 't':
				cout << "Got Scan Type" << endl;
				str.Format ("%s",optarg);
				m_scanType = atoi(str);
				break;
			case 'L': case 'l':	
				cout << "Got Communication Filename" << endl;
				m_CLogName.Format ("%s",optarg);
				printf("m_CLogName :%s\n",m_CLogName);
				gotit = TRUE;
				break;
			case 'H': case 'h':	
				cout << "Trying to halt the Listmode driver..before startign a scan" << endl;
				halt_only = TRUE;
				gotit = TRUE;
				break;
			case 'B': case 'b':	
				cout << "Putting Acquire in batch mode\n" << endl;
				batch = TRUE;
				gotit = TRUE;
				break;
			default:
				Prog_usage();
				return nRetCode;
				break;
			}
		}

		if (!gotit)
		{
			Prog_usage();
			return nRetCode;
		}

		cout<<"halt_only:"<<halt_only<<"\n";
		
		if(halt_only == TRUE) {
			int sta = LMStop();
			if(sta != 1) {
				cout <<"Stopping the PFT or Listmode Driver failed.. Needs a reboot of the computer to proceed\n";
				return nRetCode;
			} else {
				cout << "Stopping of the PFT Driver passed.. Now trying to init the scan again\n";
				sta = LMInit(NULL, NULL, NULL);
				cout<<"status:"<<sta<<"\n";
			}
			return nRetCode;

		}
		m_done = false;
		
		ScanThread *st = new ScanThread(m_patientName);
		st->ScanThreadInit(m_energyWindow,m_transSpeed,m_preset,m_scanType, m_FName, m_CLogName);
		while(!st->isScanDone()) 
			Sleep(10);
/* LOOK AT THIS LATER
		if (span_cfg == 0)
		{
			m_rebinnerInfo.lut = 0;
			m_rebinnerInfo.span = 3;
			m_rebinnerInfo.ringDifference = 67;
		}
		else
		{
			m_rebinnerInfo.lut = 1;
			m_rebinnerInfo.span = 9;
			m_rebinnerInfo.ringDifference = 67;
		}
		//GetModuleFileName(NULL, szAppPath, MAX_PATH);
		 // Extract directory
		//AppDirectory = szAppPath;
	
		//AppDirectory.Replace("ListCons.exe",m_FName);
		status = 1;
		status = LMInit(&pLMStats, &pCB, &pSinoInfo);

		//if(Scan_Mode ==1)
		//	status = InitializeHIST();

		if (status != 1)
		{
			cout<<"LMInit function failed"<<endl;
			// Send a stop command to the CC
		}
		else
		{	
			cout<<"LMInit function passed"<<endl;
			GetLocalTime(&st);
			// initialize the time
			m_timeStart.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);

			//if (m_FName == "Test")
			//{
			//	m_ext_fname.Format("-%d.%d.%d.%d.%d.%d.l",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
			//	AppDirectory +=m_ext_fname;	
			//}
			
			sprintf(fname ,"%s", m_FName);
			status = LMConfig(fname, m_preset, presetMode, online, bypass, m_rebinnerInfo);
			
			if (status != 1)
				cout<<"LMConfig function failed"<<endl;
			else
			{
				cout<<"LMConfig function passed"<<endl;
				status = LMStart();
				if (status != 1)
				{
					cout<<"LMStart function failed"<<endl;
				}
				else
				{
					cout<<"LMStart function passed"<<endl;
					while ((tottime <= m_preset+1) )
					{
						// see if scan is done 
						GetLocalTime(&st);
						// initialize the time
						m_timeEnd.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
						COleDateTimeSpan spanElapsed = m_timeEnd - m_timeStart;
						// Convert it to seconds
						tottime = spanElapsed * 24.0 * 3660.0;
						if (tottime - oldtime> 2.0)
						{
							str.Format("%3.2f,  %d",tottime, pLMStats.time_remaining);
							cout<<(LPCTSTR)str<<endl;
							str.Format("Total Event Rate = %d",pLMStats.total_event_rate);
							cout<<(LPCTSTR)str<<endl;
							str.Format("Total Stream Event = %I64d",pLMStats.total_stream_events);
							cout<<(LPCTSTR)str<<endl;
							oldtime = tottime;
						}
						if (m_done) 
						{
							str = "Terminating.................................\n";
							cout<<(LPCTSTR)str<<endl;
							break;
						}
						/////
						//m_timeTest.SetDateTime (st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);
						//spanElapsed = m_timeTest - m_timeStart;
						//testtime = spanElapsed * 24.0 * 3660.0;

						//if (testtime > 15.0)
						//{
						//	status = LMStop();
						//	
						//	if (status !=1)
						//		cout<<"LMStop function failed"<<endl;
						//}
						/////////////
					}
					//Sleep(1000);
					cout << "Scan is done"<<endl;
				}
			}
		}
		//status = LMStop();
		//status = LMInit(NULL, NULL, NULL);
*/
		
	}

	return nRetCode;
}

// Program Usage
void	Prog_usage ( )
{
	cout << " " << endl;
	cout << "NewAcquire sw_version HRRT_U 1.2 Build " << __DATE__ << " " << __TIME__ <<endl;
	cout << "Program Usage:" << endl;
	cout << "   " << endl;
	cout << "        -f for filename."  << endl;
	cout << "        -d for duration.  30 seconds is default"  << endl;
	cout << "        -m for 0 for 32 bit list mode, else for 64 bit listmode (64bit default)"  << endl;
	cout << "        -c 0 for span 3, else for span 9"  << endl;
	cout << "        -p patient name(lastName,firstName)" <<endl;
	cout << "        -e energy Window(lld,uld)" <<endl;
	cout << "        -s transmission (speed,crystalSkip)" <<endl;
	cout << "        -t ScanType(0=emmission 1=transmission)" <<endl;
}

/*
**  This is the public-domain AT&T getopt(3) code.  I added the
**  #ifndef stuff because I include <stdio.h> for the program;
**  getopt, per se, doesn't need it.  I also added the INDEX/index
**  hack (the original used strchr, of course).  And, note that
**  technically the casts in the write(2) calls shouldn't be there.
*/

#ifndef NULL
#define NULL	0
#endif
#ifndef EOF
#define EOF	(-1)
#endif
#ifndef INDEX
#define INDEX strchr
#endif


#define ERR(s, c)   if(opterr){\
	printf ( "%s %c", s, c );}


int getopt(int	argc,char **argv, char *opts)
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0 ) {	// !sv NULL -> 0
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=INDEX(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}
// End code from Stefan modified for this software
/*
void CCallB::notify(int error_code, char* extra_info)
{
//	cout << "Notify" << endl;
}

void CCallB::acqComplete(int enum_flag)
{
//	cout << "acqComplete" << endl;
}


void CCallB::acqPostComplete(int enum_flag, int fileWriteSuccess)
{
	cout << "acqPostComplete\n" << endl;
	m_done = true;

}

*/