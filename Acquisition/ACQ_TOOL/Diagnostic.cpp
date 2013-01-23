// Diagnostic.cpp : implementation file
//

#include "stdafx.h"
#include "scanit.h"
#include "Diagnostic.h"
#include "ECodes.h"

#include "cdhi.h"

extern CCDHI pDHI;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CDiagnostic dialog


CDiagnostic::CDiagnostic(CWnd* pParent /*=NULL*/)
	: CDialog(CDiagnostic::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDiagnostic)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDiagnostic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDiagnostic)
	DDX_Control(pDX, IDC_EDIT_DOS, m_DOSCmd);
	DDX_Control(pDX, IDC_LIST1, m_StatList);
	DDX_Control(pDX, IDC_EDIT_UPFILE, m_EditFile);
	DDX_Control(pDX, IDC_EDIT_SOURCE_NAME, m_SName);
	DDX_Control(pDX, IDC_COMBO1, m_cmbHead);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDiagnostic, CDialog)
	//{{AFX_MSG_MAP(CDiagnostic)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_ID, OnButtonId)
	ON_BN_CLICKED(IDC_BUTTON_PID, OnButtonPid)
	ON_BN_CLICKED(IDC_BUTTON_ASD, OnButtonAsd)
	ON_BN_CLICKED(IDC_BUTTON_LPE, OnButtonLpe)
	ON_BN_CLICKED(IDC_BUTTON_DOS, OnButtonDos)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDiagnostic message handlers

void CDiagnostic::OnButton1() 
{
	// TODO: Add your control notification handler code here
	//pDHI.Upload("E:\\ScanIt.up", "ScanIt.ini", 64);	
	//WinExec("notpad scanit.up",TRUE);
	m_cmd = -1;
	m_EditFile.GetWindowText(m_UploadFile);
	int i;
	CString str;

	m_cmbHead.GetWindowText(str);
	if (str.IsEmpty())
	{
		MessageBox("You must specify a valid head.\nPlease Specify one from the combo box.","Upload Error", MB_OK);

	}
	else
	{
		m_StatList.ResetContent();

		if (str.Find("All Enabled Heads") != -1)
		{
			// Loop for all enabled heads except the CC
			for (i=0;i<pDHI.m_TOTAL_HD;i++)
			{
				// Do head 0
				if (pDHI.m_Hd_En[i])
				{
					m_upload[i] = TRUE;
				}
			}
			InitThread();
		}
		else if (str.Find("Coincidence") != -1)
		{
			// Reset the CC only
			m_upload[8] = TRUE;
			InitThread();
		}
		else
		{
			// send to the specific head
			str.Replace("Head ","");
			i = atoi(str);
			m_upload[i] = TRUE;
			InitThread();

		}
	}
}


void CDiagnostic::OnButton2() 
{
	char targetname[2048], sourcename[2048];
	CString str, filename;
	int  ret, i;
	m_cmbHead.GetWindowText(str);
	sprintf (targetname,"%s",m_targetName);
	sprintf (sourcename,"%s",m_sourceName);

	if(m_sourceName.IsEmpty() || m_targetName.IsEmpty())
	{
		MessageBox("You must specify a valid source filename","Download Error",MB_OK);

	}
	else if (str.IsEmpty())
	{
		MessageBox("You must specify a valid head.\nPlease Specify one from the combo box.","Download Error",MB_OK);

	}
	else
	{
		m_StatList.ResetContent();
		if (str.Find("All Enabled Heads") != -1)
		{
			// Loop for all enabled heads except the CC
			for (i=0;i<pDHI.m_TOTAL_HD;i++)
			{
				// Do head 0
				if (pDHI.m_Hd_En[i])
				{
					// Call Download function
					m_StatList.AddString("Please Wait.....");
					str.Format("Downloading %s",sourcename);
					m_StatList.AddString(str);
					str.Format("to Head %d", i);
					m_StatList.AddString(str);

					ret = pDHI.DownLoad(sourcename, targetname, i);
					if (ret < 0)
						DisplayError(ret);
				}
				str.Format("Download done for Head %d", i);
				m_StatList.AddString(str);
			}
		}
		else if (str.Find("Coincidence") != -1)
		{
			// Send to the CC only
			
			m_StatList.AddString("Please Wait.....");
			str.Format("Downloading %s",sourcename);
			m_StatList.AddString(str);
			m_StatList.AddString("to Coincidence Controller");
			ret = pDHI.DownLoad(sourcename, targetname, 64);
			if (ret < 0)
				DisplayError(ret);
			m_StatList.AddString("Download done to Coincidence Controller");
		}
		else
		{
			// send to the specific head
			str.Replace("Head ","");
			i = atoi(str);
			m_StatList.AddString("Please Wait.....");
			str.Format("Downloading %s",sourcename);
			m_StatList.AddString(str);
			str.Format("to Head %d", i);
			m_StatList.AddString(str);
			ret = pDHI.DownLoad(sourcename, targetname, i);
			if (ret < 0)
				DisplayError(ret);
			str.Format("Download done for Head %d", i);
			m_StatList.AddString(str);
		}
	}
	//filename.Format("%sHead %d startup.rpt",pDHI.m_Work_Dir, head);
	
}

BOOL CDiagnostic::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	int i, j = 0;
	CString str;
	
	for (i=0;i<pDHI.m_TOTAL_HD;i++)
	{
		// Do head 0
		if (pDHI.m_Hd_En[i])
		{
			str.Format("Head %d",i);
			m_cmbHead.AddString(str);
			j++;
		}
		m_heads[i] = FALSE;
		m_upload[i] = FALSE;
		// Set Diag flag to false
		m_diagFlag[i] = FALSE;
	}
	m_heads[i] = FALSE;
	m_upload[i] = FALSE;
	m_diagFlag[i] = FALSE;
	if (j> 0)
		m_cmbHead.AddString("All Enabled Heads");

	m_cmbHead.AddString("Coincidence Controller");
	pDoc = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDiagnostic::OnButton3() 
{
	// TODO: Add your control notification handler code here
	CString strFilter,str;


	// Set the filter and set dialogfile 
	strFilter = "All Files (*.*)|*.*||" ;
	CFileDialog dlg(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT| OFN_FILEMUSTEXIST ,strFilter,NULL);


	int reply = dlg.DoModal();
	if (reply == IDOK) 
	{
		m_sourceName = dlg.GetPathName();
		m_targetName = dlg.GetFileName();
		m_SName.SetWindowText(m_sourceName);

	}	
}

void CDiagnostic::OnButtonReset() 
{
	int i, ret;
	CString str;

	m_cmbHead.GetWindowText(str);
	if (str.IsEmpty())
	{
		MessageBox("You must specify a valid head.\nPlease Specify one from the combo box.","Reset Error", MB_OK);

	}
	else
	{
		m_StatList.ResetContent();
		if (str.Find("All Enabled Heads") != -1)
		{
			ret = MessageBox("Are you sure you want to reset All Enabled Heads?","Reset Bailout", MB_ICONQUESTION | MB_YESNO);
			if (ret == 6)
			{
				// Loop for all enabled heads except the CC
				for (i=0;i<pDHI.m_TOTAL_HD;i++)
				{
					// Do head 0
					if (pDHI.m_Hd_En[i])
					{
						// Call Reset function
						str.Format("Reseting Head %d",i);
						m_StatList.AddString(str);
						ret = pDHI.ResetHead(i);
						if (ret < 0)
							DisplayError(ret);
					}
				}
			}
		}
		else if (str.Find("Coincidence") != -1)
		{
			// Reset the CC only
			ret = MessageBox("Are you sure you want to reset the Coincidence Controller?","Reset Bailout", MB_ICONQUESTION | MB_YESNO);
			if (ret == 6)
			{
				m_StatList.AddString("Reseting Coincidence Controller");
				ret = pDHI.ResetHead(64);
				if (ret < 0)
					DisplayError(ret);
			}
		}
		else
		{
			// send to the specific head
			str.Replace("Head ","");
			i = atoi(str);
			str.Format("Are you sure you want to reset Head %d\?", i);
			ret = MessageBox(str,"Reset Bailout", MB_YESNO);
			if (ret == 6)
			{
				str.Format("Reseting Head %d",i);
				m_StatList.AddString(str);
				ret = pDHI.ResetHead(i);
				if (ret < 0)
					DisplayError(ret);
			}
		}
	}
}

void CDiagnostic::OnButtonId() 
{
	m_StatList.ResetContent();
	m_cmd = 5;
	ProcessCmd();
}

bool CDiagnostic::ProcessCmd()
{
	int i, ret;
	CString str;
	m_cmbHead.GetWindowText(str);
	if (str.IsEmpty())
	{
		MessageBox("You must specify a valid head.\nPlease Specify one from the combo box.","Diagnostic Error", MB_OK);
		return FALSE;
	}
	else
	{
		if (str.Find("All Enabled Heads") != -1)
		{

			// Loop for all enabled heads except the CC
			for (i=0;i<pDHI.m_TOTAL_HD;i++)
			{
				// Do enabled heads
				if (pDHI.m_Hd_En[i])
				{
					// Call Reset function
					str.Format("Sending X%d to Head %d",m_cmd,i);
					m_StatList.AddString(str);
					ret = pDHI.SendXCom(m_cmd, i);
					if (ret < 0)
						DisplayError(ret);
					m_heads[i] = TRUE;
					// This head must be reseted
					m_diagFlag[i] = TRUE;
				}
			}
			if(m_cmd != 3)
				InitThread();
		}
		else if (str.Find("Coincidence") != -1)
		{
			// Can't do
			ret = MessageBox("Sorry, you can not apply this command to the Coincidence Controller?","Diagnostic Error", MB_ICONEXCLAMATION | MB_OK);
			return FALSE;
		}
		else
		{
			// send to the specific head
			str.Replace("Head ","");
			i = atoi(str);
			str.Format("Sending X%d to Head %d",m_cmd,i);
			m_StatList.AddString(str);
			ret = pDHI.SendXCom(m_cmd, i);
			if (ret < 0)
				DisplayError(ret);
			m_heads[i] = TRUE;
			// This head must be reseted
			m_diagFlag[i] = TRUE;
			if(m_cmd != 3)
				InitThread();
		}
	}
	return TRUE;
}

void CDiagnostic::DisplayError(int ErrNum)
{
	CString str;
	if(ErrNum<0 && ErrNum>MAXERRORCODE)
		str.Format("%s",e_short_text[-1*ErrNum]);
	else
		str = "Unknown Error";
	MessageBox(str,"Communication Error",MB_OK);

}

void CDiagnostic::InitThread()
{
	hThread = (HANDLE) _beginthreadex(NULL,0,BkGndProc,this,0,&tid);
	CloseHandle(hThread);
}

unsigned int WINAPI CDiagnostic::BkGndProc(void *p)
{
	int i, hd, inc, ret;
	bool Continue = TRUE;
	char buffer[2048], cmdbuf[25];
	CString str;
	CDiagnostic *pa = (CDiagnostic *)p;
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	while (Continue)
	{
		inc = 0;
		for(i=0;i<=8;i++)
		{
			if (i< 8)
				hd = i;
			else
				hd = 64;
			// Check the command status
			if(pa->m_heads[i])
			{
				inc ++;

				sprintf(cmdbuf,"P0");
		
				ret = pDHI.SendCmd(cmdbuf,buffer, hd, 2);

				str.Format("%s",buffer);
				
				// Parse the return buffer for percent completion
				if (ret > 0 && str.Find("100") != -1)
				{
					// the operation is 100% complete
					pa->m_heads[i] = FALSE;
					pa->m_upload[i] = TRUE;
				}
			}
			if(pa->m_upload[i])
			{
				inc ++;
				if(pa->m_cmd == 5 || pa->m_cmd == 6 || pa->m_cmd == 7)
					pa->m_UploadFile = "diag.rpt";
				//else
				//	pa->m_UploadFile = "startup.rpt";
				pa->UploadFile(hd);
				pa->m_upload[i] = FALSE;
			}
		}
		// Check if we are done
		if (inc ==0)
			Continue = FALSE;
	}
	SetCursor(LoadCursor(NULL, IDC_ARROW));
	return 0;
}


void CDiagnostic::UploadFile(int head)
{
	CString filename,file2beupld, str;
	int a;
	file2beupld = m_UploadFile;
	if (m_cmd == 5)
		filename.Format("%sHead %d Interface Diagnostic.rpt",pDHI.m_Work_Dir, head);
	else if (m_cmd == 6)
		filename.Format("%sHead %d Pannel Interface Diagnostic.rpt",pDHI.m_Work_Dir, head);
	else if (m_cmd == 7)
		filename.Format("%sHead %d Analog Subsection Diagnostic.rpt",pDHI.m_Work_Dir, head);
	else
		filename.Format("%sHead %d %s",pDHI.m_Work_Dir, head, m_UploadFile);

	m_StatList.AddString("Please Wait.....");
	str.Format("Uploading %s",file2beupld);
	m_StatList.AddString(str);
	str.Format("from Head %d", head);
	m_StatList.AddString(str);
	str.Format("to %s",filename);
	m_StatList.AddString(str);
	
	a = pDHI.Upload(filename,file2beupld,head);
	if (a==0)
	{
		str.Format("notepad %s",filename);
		WinExec(str,TRUE);
	}
	else
	{
		DisplayError(a);
	/*	if(a<0 && a>MAXERRORCODE)
			str.Format("%s",e_short_text[-1*a]);
		else
			str = "Unknown Error";
		MessageBox(str,"Upload File Error",MB_OK);
	*/
	}
	str.Format("Upload done for Head %d", head);
	m_StatList.AddString(str);
}


void CDiagnostic::OnButtonPid() 
{
	m_StatList.ResetContent();
	m_cmd = 6;
	ProcessCmd();
	
}

void CDiagnostic::OnButtonAsd() 
{
	m_StatList.ResetContent();
	m_cmd = 7;
	ProcessCmd();	
	
}

void CDiagnostic::OnOK() 
{
	int i,ret, hd;
	bool rsflag = FALSE;
	CString str,t;
	str = "You must reset the following head(s):\n";
	for (i=0;i<=8;i++)
	{
		if (m_diagFlag[i])
		{
			rsflag = TRUE;
			if (i < 8)
			{
				t.Format("Head %d\n", i);
				str += t;
			}
			else
				str += "Coincidence Controller\n";
		}
	}
	str += "Would you like the software to reset them automatically?";
	if (rsflag)
	{
		ret = MessageBox(str,"Reset Option", MB_ICONQUESTION | MB_YESNO);
		if (ret == 6)
		{
			for (i=0;i<=8;i++)
			{
				if (m_diagFlag[i])
				{
					if (i < 8)
					{
						hd = i;
						str.Format("Reseting Head %d",i);
					}
					else
					{
						hd = 64;
						str.Format("Reseting Coincidence Controller");
					}
					m_StatList.AddString(str);
					ret = pDHI.ResetHead(hd);
					if (ret < 0)
						DisplayError(ret);
				}
			}
		}
	}
	CDialog::OnOK();
}



void CDiagnostic::OnButtonLpe() 
{
	int status, ret;
	char fname[256];
	char buffer[2048], cmdbuf[25];
	bool presetMode=FALSE;
	unsigned long bypass = 0;
	FILE *in, *out;

	SetCursor(LoadCursor(NULL, IDC_WAIT));
	// Send an X3 command
	m_StatList.ResetContent();
 	m_cmd = 3;
	if (ProcessCmd())
	{

		// Configure the LM DLL
		pDoc->ConfigRebinner();
		sprintf(fname ,"%sPE_TEST.l", pDHI.m_Work_Dir);
		
		bypass = pDoc->m_List_64bit & 1;

		status = LMConfig(fname, 5, presetMode, 0, bypass, pDoc->m_rebinnerInfo);

		/*CString str;
		m_SName.GetWindowText(str);
		sprintf(fname,"%s",str);
		*/status = 1;
		
		if (status == 1) 
		{
			// put CC in pass through mode
			sprintf(cmdbuf,"S1");
			ret = pDHI.SendCmd(cmdbuf,buffer, 64, 20);
			if (ret < 0) 
				DisplayError(ret);
			//Send LM Start
			status = LMStart();
			//status = 1;
			if (status == 1)
			{
				// Sleep for 10 seconds
				Sleep(10000);
			   //open the listmode data file to be read
				in = fopen(fname,"rb");
				sprintf(buffer, "%s.rpt", fname);
				out = fopen(buffer,"w");
				if (!in || !out)
				{
					MessageBox("Error: can't open input/output file ", "File Open Error", MB_OK);
				}
				else
				{
					// Reset the counters
					event_counter = 0;
					sync_error_counter = 0;
					ret = list_pe_test(in, out, bypass); // bypass = 0 32 bit, bypass = 1 64 bit
					if (ret != 0 )
						MessageBox("Operation was not completed correctly","PE_TEST Error",MB_OK);
				}
				if(in)
				   fclose(in);
				if(out)
				   fclose(out);
			}
		}
	}	
	SetCursor(LoadCursor(NULL, IDC_ARROW));
}


static int test0[4] = { 0x39, 0x71, 0xc3, 0x55 };
static int test1[4] = { 0x17, 0x47, 0x59, 0x33 };
static int test2[4] = { 0x63, 0xa3, 0xd4, 0x0f };

int CDiagnostic::list_pe_test(FILE *in, FILE *out, int rebinner)
{
	event_word32 ew;
	int i;
	CString str;
	//process all events
	while (!get_event(in, out, &ew, rebinner, 0)) 
	{
		for (i = 0; i < 4; i++) 
		{
			if (ew.out0 == test0[i]) break;
			if (ew.out1 == test1[i]) break;
			if (ew.out2 == test2[i]) break;
		}
		if (i < 4) 
		{
			if (ew.out0 != test0[i] || ew.out1 != test1[i] || ew.out2 != test2[i])
			{
				m_StatList.AddString("Warning:  Flaw located in test pattern:");
				fprintf(out,"Warning:  Flaw located in test pattern: ");
				str.Format("%X %X %X %X", (unsigned int)ew.head, (unsigned int)ew.out0, (unsigned int)ew.out1, (unsigned int)ew.out2);
				m_StatList.AddString(str);
				fprintf(out,str +"\n");
			}
		} 
		else 
		{
			m_StatList.AddString("Warning:  No match was found for event:");
			fprintf(out,"Warning:  No match was found for event: ");
			str.Format("%X %X %X %X", (unsigned int)ew.head, (unsigned int)ew.out0, (unsigned int)ew.out1, (unsigned int)ew.out2);
			m_StatList.AddString(str);
			fprintf(out,str + "\n");
		}
		  int a = 0;
	}

	return 0;
}

int CDiagnostic::get_event(FILE *in, FILE *out, event_word32 *ew, int rebinner, int include_tag)
{
	static event_word32 *listbuf;
	static event_word32 *listptr;
	static int nevents;
	event_word32 junk;
	CString str;
	include_tag = 0;
	int verbose = 1;	// always report
	if (!listbuf) 
	{
		listbuf = (event_word32 *)malloc(MAX_EVENTS * sizeof(event_word32));
		if (!listbuf) return 1;
		nevents = 0;
	}

	if (!nevents || (nevents < 2 && !rebinner)) 
	{
		nevents = fread(listbuf, sizeof(event_word32), MAX_EVENTS, in);
		if (!nevents) return 2;
		//if (hash) cout << "." << flush;
		listptr = listbuf;
	}
	// is 32-bit format
	if (rebinner == 0) 
	{
		*ew = *listptr;
		listptr++;
		nevents--;
		event_counter++;

	// otherwise use 64-bit format
	}
	else 
	{
		for (;;) 
		{
         // make sure everything stays sync'ed
			while (((listptr + 1)->head & 0x80) != 0x80) 
			{

				if (verbose) 
				{
					/*  cout << endl << "Sync Error: (first packet): " << hex << (int)listptr->head << " "
					<< (int)listptr->out0 << " " << (int)listptr->out1 << " " << (int)listptr->out2 << endl;
					cout << "Sync Error: (second packet):" << hex << (int)(listptr + 1)->head << " " 
					<< (int)(listptr +1)->out0 << " " << (int)(listptr +1)->out1 << " " 
					<< (int)(listptr +1)->out2 << endl;
					cout << "Nevents= " << nevents << endl;
					*/
					m_StatList.AddString("Sync Error: (first packet):");
					fprintf(out,"Sync Error: (first packet): ");
					str.Format("%X %X %X %X",(int)listptr->head, (int)listptr->out0, (int)listptr->out1, (int)listptr->out0);
					m_StatList.AddString(str);
					fprintf(out,str + "\n");
					m_StatList.AddString("Sync Error: (second packet):");
					fprintf(out,"Sync Error: (second packet): ");
					str.Format("%X %X %X %X",(int)(listptr +1)->head, (int)(listptr +1)->out0, (int)(listptr +1)->out1, (int)(listptr +1)->out0);
					m_StatList.AddString(str);
					fprintf(out,str + "\n");
					str.Format("Nevents= %d",nevents);
					m_StatList.AddString(str);
					fprintf(out,str + "\n");

				}

				sync_error_counter++;

				// move the buffer 1 event word to resync buffers
				if (!fread(&junk,sizeof(event_word32),1,in)) return 2;

				listptr++;
				nevents--;
				if (nevents < 2) 
				{
					//if (hash) cout << "." << flush;
					nevents = fread(listbuf,sizeof(event_word32),MAX_EVENTS,in);
					if (!nevents) return 2;
					listptr = listbuf;
				}
			}

			// copy data from the 64-bit stream to a 32-bit stream
			ew->head = (listptr + 1)->out1;
			ew->out0 = (listptr + 1)->out2;
			ew->out1 = listptr->out1;
			ew->out2 = listptr->out2;
			ew_low = *(listptr + 1);
			ew_high = *(listptr);

			event_counter++;
			listptr += 2;
			nevents -= 2;

			// regular event, pass through
			if (!(ew->head & 0x80) || include_tag) 
			{
				break;

			// tag event, process here
			} 
			else if (verbose) 
			{
				//cout << "Tag: " << hex << (unsigned int)ew->head << " " << (unsigned int)ew->out0 
				//<< " " << (unsigned int)ew->out1 << " " << (unsigned int)ew->out2 << endl;
				str.Format("Tag: %X %X %X %X",(unsigned int)ew->head, (unsigned int)ew->out0, (unsigned int)ew->out1, (unsigned int)ew->out2);
				m_StatList.AddString(str);
				fprintf(out,str +"\n");
				// time tag
				if ((ew->head & 0xe0) == 0x80) 
				{
					long time = ((ew->out0 << 16) + (ew->out1 << 8) + ew->out2);
					//cout << "Timetag: " << time << endl;
					str.Format("Timetag: %d",time);
					m_StatList.AddString(str);
					fprintf(out,str+"\n");
				// singles tag
				} 
				else if ((ew->head & 0xe0) == 0xa0) 
				{
					int block = ((ew->head & 0x1f) << 5) + (ew->out0 >> 3);
					int countrate = ew->out2 + (ew->out1 << 8) + ((ew->out0 & 0x07) << 16);
					//cout << "Block Singles: " << block << " " << countrate << dec << " " << block 
					//<< " " << countrate << endl;
					str.Format("Block Singles: %d %d",block, countrate);
					m_StatList.AddString(str);
					fprintf(out,str+"\n");
				}
			}
		}
	}
	return 0;
}


void CDiagnostic::OnButtonDos() 
{
	CString str;
	char cmd[128];
	int ret,i;


	m_DOSCmd.GetWindowText(str);
	sprintf(cmd,"%s",str);
	m_cmbHead.GetWindowText(str);
	if (str.IsEmpty())
	{
		MessageBox("You must specify a valid head.\nPlease Specify one from the combo box.","Upload Error", MB_OK);

	}
	else
	{
		m_StatList.ResetContent();

		if (str.Find("All Enabled Heads") != -1)
		{
			// Loop for all enabled heads except the CC
			for (i=0;i<pDHI.m_TOTAL_HD;i++)
			{
				// Do head 0
				if (pDHI.m_Hd_En[i])
				{

					ret = pDHI.DOS(cmd, i);
					if (ret < 0)
						DisplayError(ret);
				}
			}
		}
		else if (str.Find("Coincidence") != -1)
		{
			// The CC only
			ret = pDHI.DOS(cmd, 64);
			if (ret < 0)
				DisplayError(ret);
		}
		else
		{
			// send to the specific head
			str.Replace("Head ","");
			i = atoi(str);
			ret = pDHI.DOS(cmd, i);
			if (ret < 0)
				DisplayError(ret);			

		}
	}




}
