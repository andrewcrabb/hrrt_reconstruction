// Patient.cpp : implementation file
//
/* HRRT User Community modifications
   Authors: Merence Sibomana
   05-Aug-2009 : Remove leading and trailing whitespaces in patient firstname
                 and lastname
*/


#include "stdafx.h"
#include "scanit.h"
#include "Patient.h"
#include "PatientADO.h"
#include "DosageInfo.h"

#include "cdhi.h"
extern CCDHI pDHI;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPatient dialog

_RecordsetPtr m_pRS = NULL;
IADORecordBinding    * m_picRs = NULL;  // Interface Pointer declared
CPatientADO m_FilterRs; //C++ class object

//#import "c:\program files\common files\system\ole db\oledb32.dll" rename_namespace("oledb")
#import "oledb32.dll" rename_namespace("oledb")

inline void TESTHR(HRESULT x) {if FAILED(x) _com_issue_error(x);};


CPatient::CPatient(CWnd* pParent /*=NULL*/)
	: CDialog(CPatient::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPatient)
	m_FName = _T("");
	m_LName = _T("");
	m_Sex = -1;
	m_PatientID = _T("");
	//}}AFX_DATA_INIT
}


void CPatient::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPatient)
	//DDX_Control(pDX, IDC_COMBO_STRENGTH, m_cmbStrength);
	DDX_Control(pDX, IDC_LN_SEARCH, m_LNSearch);
	//DDX_Control(pDX, IDC_COMBO_ISO, m_Iso_Dose);
	DDX_Control(pDX, IDC_LIST1, m_ctrlList);
	DDX_Text(pDX, IDC_EDIT_DOB, m_DOB);
	DDX_Text(pDX, IDC_EDIT_FIRST_NAME, m_FName);
	DDX_Text(pDX, IDC_EDIT_LAST_NAME, m_LName);
	DDX_Radio(pDX, IDC_RADIO_SEX, m_Sex);
	DDX_Text(pDX, IDC_EDIT_PATIENT_ID, m_PatientID);
	//DDX_Text(pDX, IDC_EDIT_STRENGTH, m_Strength);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPatient, CDialog)
	//{{AFX_MSG_MAP(CPatient)
	ON_BN_CLICKED(IDC_SEARCH_ALL, OnSearchAll)
	ON_BN_CLICKED(IDC_LN_SEARCH, OnLnSearch)
	ON_LBN_SELCHANGE(IDC_LIST1, OnSelchangeList1)
	ON_BN_CLICKED(IDC_FN_SEARCH, OnFnSearch)
	ON_BN_CLICKED(IDC_DOB_SEARCH, OnDobSearch)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_ID_SEARCH, OnIdSearch)
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPatient message handlers

void CPatient::OnOK() 
{
	// TODO: Add extra validation here

	UpdateData(TRUE);
	// Remove leading and trailing whitespaces
	m_LName.TrimLeft(); m_LName.TrimRight();
	m_FName.TrimLeft(); m_FName.TrimRight();
	pDoc->m_Pt_Lst_Name = m_LName;
	pDoc->m_Pt_Fst_Name = m_FName;
	pDoc->m_Pt_DOB = m_DOB.Format();
	pDoc->m_Pt_ID = m_PatientID;
	pDoc->m_Pt_Sex = m_Sex;
	//pDoc->m_Pt_Dose = m_Iso_Dose.GetCurSel();
	//pDoc->m_Strength_Type = m_cmbStrength.GetCurSel();
	//pDoc->m_Strength = m_Strength;

	if(CheckExist())
	{
		//pDoc->m_Dn_Step = 1;
		//pDoc->m_bDownFlag = TRUE;
		CDialog::OnOK();
	}
}

BOOL CPatient::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	// Get a pointer to the document
	_bstr_t		bstr_myConnectString="";
	CString str;

	pDoc = dynamic_cast<CScanItDoc*> (((CFrameWnd*) AfxGetApp()->m_pMainWnd)->GetActiveDocument());	
	
	m_LName = pDoc->m_Pt_Lst_Name;
	m_FName = pDoc->m_Pt_Fst_Name ;
	m_DOB.Format( pDoc->m_Pt_DOB);
	m_PatientID = pDoc->m_Pt_ID ;
	m_Sex = pDoc->m_Pt_Sex;
	

	UpdateData(FALSE);



	//if (!AfxOleInit())
	//{
	//	AfxMessageBox(_T("OLE initialization failed."));
	//	return FALSE;
	//}
     // Initialize COM
    //::CoInitialize( NULL );
	if(!FAILED(::CoInitialize(NULL)))
	{
		m_ConnString = "";
		GetDBConn();
		if (m_ConnString.Find("Data Source")== -1)
		{
			ShowDataLink( &bstr_myConnectString );
			m_ConnString.Format("%s",(char*)bstr_myConnectString);
			SaveDBConn();
		}


		try
		{	
			// Setup filter record set
			TESTHR(m_pRS.CreateInstance(__uuidof(Recordset)));

			//m_pRS->CursorLocation = adUseServer;//adUseNone;
			
			//m_ConnString.Format("Provider=Microsoft.Jet.OLEDB.4.0;Data Source=%s;user id=%s;password=%s;Persist Security Info=False","Patient.mdb",_T(""),_T(""));
			TESTHR(m_pRS->QueryInterface(__uuidof(IADORecordBinding), (LPVOID*)&m_picRs));

		}
		catch(_com_error &e)
		{
			// Notify the user of errors if any.
			// Pass a connection pointer accessed from the Recordset.
			_variant_t vtConnect = m_pRS->GetActiveConnection();
			CString ErrStr;
			ErrStr.Format("%s", e.Description());
			MessageBox(ErrStr, "DB Error",MB_OK);

		}
		
	}



	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPatient::OnSearchAll() 
{

	char sSql[128];

	UpdateData(TRUE);
	m_ctrlList.ResetContent();
	
	sprintf(sSql, "Select * from PatientInfo ORDER BY LastName ASC");
	ExeSql(sSql);


}

void CPatient::OnLnSearch() 
{
	char sSql[128];

	UpdateData(TRUE);
	m_LName.TrimLeft();
	m_LName.TrimRight();
	m_ctrlList.ResetContent();
	
	m_FName =  " ";
	m_DOB.m_dt=2;
	m_Sex =  -1;			
	m_PatientID =" ";

	UpdateData(FALSE);

	if (!m_LName.IsEmpty())
	{
		sprintf(sSql, "Select * from PatientInfo WHERE LastName LIKE '%%%s%%' ORDER BY LastName ASC", m_LName);
		ExeSql(sSql);
	}
}

void CPatient::ExeSql(char* sSql)
{
	CString ID;
	//sprintf(sSql,"Select sysdate from dual");
	if (m_pRS !=NULL)
	{
		try
		{
			//sprintf(MySql,"%s",sSql);
			m_pRS->Open(sSql, (LPCSTR)m_ConnString, adOpenDynamic,adLockReadOnly,adCmdTableDirect);

			
			//Bind the Recordset to a C++ Class
			TESTHR(m_picRs->BindToRecordset(&m_FilterRs));
			// Move the pointer to the beginning of the recordset
			//m_pRS->MoveFirst();

			// Clear the list
			m_ctrlList.ResetContent();
			// loop and update the listbox
			while (VARIANT_FALSE == m_pRS->EndOfFile)
			{
 
				CString str;

				str.Format("%s, %s %s ",m_FilterRs.m_lLastNameStatus == adFldOK ? m_FilterRs.m_szLastName : " ",
					m_FilterRs.m_lFirstNameStatus == adFldOK ? m_FilterRs.m_szFirstName : " ",
					m_FilterRs.m_lPatientIDStatus  == adFldOK ? m_FilterRs.m_szPatientID : " ");
				if(m_FilterRs.m_lSex == 0)
					str+="Male ";
				else if(m_FilterRs.m_lSex==1)
					str+="Female ";
				else 
					str += "Unknown ";
				ID.Format("%d",  m_FilterRs.m_lID);
				str += ID;
				m_ctrlList.AddString (str);
				m_pRS->MoveNext();
			}
			m_pRS->Close();
		}
		catch(_com_error &e)
		{
			// Notify the user of errors if any.
			// Pass a connection pointer accessed from the Recordset.
			HRESULT hr = e.Error();
			_variant_t vtConnect = m_pRS->GetActiveConnection();
		
			CString ErrStr;			
			ErrStr.Format("%s", (char *)e.Description());
			MessageBox(ErrStr, "DB Error",MB_OK);
			if(ErrStr.Find("Make sure that the path name is spelled correctly and that you are connected to the server on which the file resides") != -1)
			{
				_bstr_t		bstr_myConnectString="";
				ShowDataLink( &bstr_myConnectString );
				m_ConnString.Format("%s",(char*)bstr_myConnectString);
				SaveDBConn();
			}

		}
	}	
		
}

void CPatient::OnSelchangeList1() 
{
	CString str, inx;
	char sSql[128];
	int sel,idx = 0, a;
	//COleDateTime DOB;
	CDBVariant DOB;


	sel = m_ctrlList.GetCurSel();
	m_ctrlList.GetText(sel,str);

	if (!str.IsEmpty())
	{
		m_FName =  " ";
		m_LName =  " ";
		//m_DOB =  0;
		m_Sex =  -1;			
		m_PatientID =" ";

		UpdateData(FALSE);

		if (str.Find("Male") !=-1)
		{
			idx = str.Find("Male");

		}
		else if(str.Find("Female") !=-1)
		{
			idx = str.Find("Female");
		}
		else if (str.Find("Unknown") !=-1)
		{
			idx = str.Find("Unknown");
		}
		if (idx >0)
		{
			a = str.Find(' ',idx);
			idx = str.GetLength();
			inx = str.Mid(a+1, idx-a-1);
			sprintf(sSql,"Select * from PatientInfo WHERE ID = %d", atoi(inx));
			try
			{
				// Open the connection with the sql statement
				m_pRS->Open(sSql, (LPCSTR)m_ConnString, adOpenDynamic,adLockReadOnly,adCmdTableDirect);
				
				//Bind the Recordset to a C++ Class
				TESTHR(m_picRs->BindToRecordset(&m_FilterRs));



				m_FName = m_FilterRs.m_lFirstNameStatus == adFldOK ? m_FilterRs.m_szFirstName : " ";
				m_LName = m_FilterRs.m_lLastNameStatus == adFldOK ? m_FilterRs.m_szLastName : " ";
				m_DOB.m_dt = m_FilterRs.m_lDOBStatus == adFldOK ? m_FilterRs.m_dtDOB : 2;
				m_Sex = m_FilterRs.m_lSexStatus == adFldOK ? m_FilterRs.m_lSex : -1;			
				m_PatientID = m_FilterRs.m_lPatientIDStatus  == adFldOK ? m_FilterRs.m_szPatientID : " ";
				
				m_pRS->Close();

				UpdateData(FALSE);
			}
			catch(_com_error &e)
			{
				// Notify the user of errors if any.
				// Pass a connection pointer accessed from the Recordset.
				_variant_t vtConnect = m_pRS->GetActiveConnection();
			
				MessageBox(e.Description(), "DB Error",MB_OK);

			}
		}
	
	}


}

void CPatient::OnFnSearch() 
{

	char sSql[128];

	UpdateData(TRUE);
	m_FName.TrimLeft();
	m_FName.TrimRight();
	m_ctrlList.ResetContent();
	
	m_LName =  " ";
	m_DOB.m_dt=2;
	m_Sex =  -1;			
	m_PatientID =" ";

	UpdateData(FALSE);
	if (!m_FName.IsEmpty())
	{	
		sprintf(sSql, "Select * from PatientInfo WHERE FirstName LIKE '%%%s%%' ORDER BY LastName ASC", m_FName);
		ExeSql(sSql);
	}
}

void CPatient::OnDobSearch() 
{
	CString  str;
	COleDateTime dt;
	char sSql[128];
	UpdateData(TRUE);
	dt = m_DOB;
	str = dt.Format();
	m_ctrlList.ResetContent();

	m_FName =  " ";
	m_LName =  " ";
	m_Sex =  -1;			
	m_PatientID =" ";

	UpdateData(FALSE);
	if (!str.IsEmpty())
	{	
		sprintf(sSql, "Select * from PatientInfo WHERE DOB = #%s# ORDER BY LastName ASC", str);
		ExeSql(sSql);
	}
}

bool CPatient::CheckExist()
{
	CString fstr,lstr;
	char sSql[128];

	int a;

	UpdateData(TRUE);

	fstr = m_FName;
	lstr = m_LName;

	fstr.TrimLeft();
	fstr.TrimRight();
	lstr.TrimLeft();
	lstr.TrimRight();
	if(!fstr.IsEmpty() && !lstr.IsEmpty())
	{
		try
		{
			COleSafeArray vaFieldlist, vaValuelist;
			long lArrayIndex[6]={0, 1, 2, 3, 4, 5};

			vaFieldlist.CreateOneDim(VT_VARIANT, 5);
			vaValuelist.CreateOneDim(VT_VARIANT, 5);

			m_FName.TrimLeft();
			m_FName.TrimRight();
			m_LName.TrimLeft();
			m_LName.TrimRight();
			m_PatientID.TrimLeft();
			m_PatientID.TrimRight();

			vaFieldlist.PutElement(lArrayIndex, &(_variant_t("LastName")));
			vaFieldlist.PutElement(lArrayIndex+1, &(_variant_t("FirstName")));
			vaFieldlist.PutElement(lArrayIndex+2, &(_variant_t("DOB")));
			vaFieldlist.PutElement(lArrayIndex+3, &(_variant_t("Sex")));
			vaFieldlist.PutElement(lArrayIndex+4, &(_variant_t("PatientID")));
			
			vaValuelist.PutElement(lArrayIndex, &(_variant_t(m_LName)));
			vaValuelist.PutElement(lArrayIndex+1, &(_variant_t(m_FName)));
			vaValuelist.PutElement(lArrayIndex+2, &(_variant_t(m_DOB)));
			vaValuelist.PutElement(lArrayIndex+3, &(_variant_t((long)m_Sex,  VT_I4 )));
			vaValuelist.PutElement(lArrayIndex+4, &(_variant_t(m_PatientID)));
			

			sprintf(sSql, "Select * from PatientInfo WHERE FirstName LIKE '%s' AND LastName LIKE '%s'",fstr, lstr);
				
			m_pRS->Open(sSql, (LPCSTR)m_ConnString, adOpenDynamic,adLockOptimistic,adCmdTableDirect);

			short x = m_pRS->EndOfFile;
			if (x==-1)
			{
				// Close the record set
				m_pRS->Close();
				// Assemble a new query
				sprintf(sSql, "Select * from PatientInfo WHERE PatientID = '%s'",m_PatientID);
				m_pRS->Open(sSql, (LPCSTR)m_ConnString, adOpenDynamic,adLockOptimistic,adCmdTableDirect);
		
				x = m_pRS->EndOfFile;
				if (x==-1)
				{
					// Record does not exists, update?
					a = MessageBox("The Patient you have selected does not exists in the patient database.  Would you like to update the records?","Update Patient",MB_YESNO);
					if (a == 6)
					{
						TESTHR(m_pRS->AddNew(vaFieldlist, vaValuelist));
						TESTHR(m_pRS->Update());					
					}
				}
				else
				{
					a = MessageBox("The Patient ID you have selected exists in the patient database.  Please change the Patient ID and try again","Update Patient",MB_OK);
				
					m_pRS->Close();
					return FALSE;

				}
			}
			m_pRS->Close();
		}
		catch(_com_error &e)
		{
			// Notify the user of errors if any.
			// Pass a connection pointer accessed from the Recordset.
			_variant_t vtConnect = m_pRS->GetActiveConnection();
		
			MessageBox(e.Description(), "DB Error",MB_OK);

		}
		return TRUE;
	}
	else
	{
		MessageBox("You must have a valid patient first and last name.  Please try agin.","Update Patient",MB_OK);
		return FALSE;
	}

}

void CPatient::OnDestroy() 
{
	CDialog::OnDestroy();
	// Uninitialize the ADO DCOM
	//CoUninitialize();	

}

void CPatient::OnIdSearch() 
{

	char sSql[128];

	UpdateData(TRUE);
	m_PatientID.TrimLeft();
	m_PatientID.TrimRight();
	m_ctrlList.ResetContent();
	
	m_LName =  " ";
	m_FName =" ";
	m_DOB.m_dt=2;
	m_Sex =  -1;			
	
	UpdateData(FALSE);
	if (!m_FName.IsEmpty())
	{	
		sprintf(sSql, "Select * from PatientInfo WHERE PatientID LIKE '%%%s%%' ORDER BY LastName ASC", m_PatientID);
		ExeSql(sSql);
	}
	
}


void CPatient::GetDBConn()
{
	char t[256];
	CString str, newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "db.cfg";

	str = "";

	// Open the header file
	FILE  *in_file;
	in_file = fopen(newstr,"r");
	if( in_file!= NULL )
	{	
		// Read the config line 
		fgets(t,256,in_file); 	
		m_ConnString.Format ("%s",t);
	}
	else
		m_ConnString = "";
	if( in_file!= NULL )
		fclose(in_file);

}

void CPatient::SaveDBConn()
{
	// Write the new  file

	CString newstr;

	pDHI.GetAppPath(&newstr);
	newstr+= "db.cfg";


	FILE  *out_file;
	out_file = fopen(newstr,"w");
	fprintf( out_file, "%s", m_ConnString );
	if( out_file!= NULL )
		fclose(out_file);
	
}


BOOL CPatient::ShowDataLink(_bstr_t *bstr_ConnectString)
{
	HRESULT		hr;
	oledb::IDataSourceLocatorPtr	p_IDSL= NULL;			// This is the Data Link dialog object
	_ConnectionPtr					p_conn = NULL;			// We need a connection pointer too
	BOOL							b_ConnValid = FALSE;
	
	try
	{
		 // Create an instance of the IDataSourceLocator interface and check for errors.
		hr = p_IDSL.CreateInstance( __uuidof( oledb::DataLinks ));
		TESTHR( hr );

		
		// If the passed in Connection String is blank, we are creating a new connection
		 
		if( *bstr_ConnectString == _bstr_t("") )
		{
			
			// Simply display the dialog. It will return a NULL or a valid connection object 
			// with the connection string filled in for you. If it returns NULL, the user
			// must of clicked the cancel button.

			p_conn = p_IDSL->PromptNew();
			if( p_conn != NULL ) 
				b_ConnValid = TRUE;
			
		}
		else 
		{
			
			// We are going to use a pre-existing connection string, so first we need to
			// create a connection object ourselves. After creating it, we'll copy the
			// connection string into the object's connection string member
			 
			p_conn.CreateInstance( "ADODB.Connection" );
			p_conn->ConnectionString = *bstr_ConnectString;

				 
			// When editing a data link, the IDataSourceLocator interface requires we pass 
			// it a connection object under the guise of an IDispatch interface. So,
			// here we'll query the interface to get a pointer to the IDispatch interface,
			// and we'll pass that to the data link dialog.
			
			IDispatch * p_Dispatch = NULL;
			p_conn.QueryInterface( IID_IDispatch, (LPVOID*) & p_Dispatch );
			
			if( p_IDSL->PromptEdit( &p_Dispatch )) b_ConnValid = TRUE;
			
			p_Dispatch->Release();
		}
		   
		// Once we arrive here, we're done editing. Did the user press OK or cancel ?
		// If OK was pressed, we want to return the new connection string in the parameter given to us.
		
		if( b_ConnValid )
		{
			*bstr_ConnectString = p_conn->ConnectionString;
		}

	}
	catch( _com_error & e )
	{
		
		
		//   Catcher for any _com_error's that are generated.
		CString str, t;
		str = "Com Exception raised\n";
		t.Format( "Description : %s\n", e.Description());
		str += t;
		t.Format( "Message     : %s\n", e.ErrorMessage());
		str += t;
		MessageBox(str,"Show Data Link Error",MB_ICONEXCLAMATION | MB_OK);
		
		return FALSE;
	}

	return b_ConnValid;

}

void CPatient::OnButton1() 
{
	CDosageInfo pPID;
	pPID.DoModal();

}
