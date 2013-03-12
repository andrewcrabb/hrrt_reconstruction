// CDHI.cpp: implementation of the CCDHI class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "scanitdoc.h"
#include "CDHI.h"
#include "SerialBus.h"
#include "ECodes.h"

#define DEFAULT_DTC  8.94e-6   // Default dead time constant


extern CSerialBus pSerBus;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCDHI::CCDHI()
{
	//ReadIniFile();
	//GetSysMode();
	m_Opt_Name = TRUE;
	m_Opt_PID = FALSE;
	m_Opt_CID = FALSE;
	m_Opt_Num = FALSE;
	m_DTC = DEFAULT_DTC;

}

CCDHI::~CCDHI()
{
	
	int i;
	for (i=0;i<m_TOTAL_HD;i++)
	{
		// Do head 0
		if (m_Hd_En[i])
			switch (i)
		{
			case 0:
				free(m_Head_0);
				free(m_Hd_Sin_0);
				break;
			case 1:
				free(m_Head_1);
				free(m_Hd_Sin_1);
				break;
			case 2:
				free(m_Head_2);
				free(m_Hd_Sin_2);
				break;
			case 3:
				free(m_Head_3);
				free(m_Hd_Sin_3);
				break;
			case 4:
				free(m_Head_4);
				free(m_Hd_Sin_4);
				break;
			case 5:
				free(m_Head_5);
				free(m_Hd_Sin_5);
				break;
			case 6:
				free(m_Head_6);
				free(m_Hd_Sin_6);
				break;
			case 7:
				free(m_Head_7);
				free(m_Hd_Sin_7);
				break;
		}
	}
	
	//free(m_Hd_En);

}

void CCDHI::ReadIniFile()
{
	int i;
	char t[256];
	CString str, newstr;

	GetAppPath(&newstr);

	newstr+= "ScanIt.ini";
	for (i=0;i<8;i++)
		m_Hd_En[i] = FALSE;
	
	// Open the header file
	FILE  *in_file;
	in_file = fopen(newstr,"r");
	if( in_file!= NULL )
	{	
		// Read teh file line by line and sort data
		while (fgets(t,256,in_file) != NULL )
		{			
			str.Format ("%s",t);
			//if (str.Find("!")==-1 )
			{
				//  What type of system
				if(str.Find("system :=")!=-1)
				{
					if(str.Find("HRRT")!=-1)
					{
						m_HRRT = TRUE;
						m_PETSPECT = FALSE;
					}
					if(str.Find("PETSPECT")!=-1)
					{
						//#define PETSPECT TRUE
						m_HRRT = TRUE;
						m_PETSPECT = FALSE;
					}
				}
				SortData (str, &newstr);
				if(str.Find("total head :=")!=-1)
					m_TOTAL_HD = atoi(newstr);
	
				if(str.Find("max row number :=")!=-1)
					m_MAX_ROW = atoi(newstr);
				if(str.Find("max column number :=")!=-1)
					m_MAX_COL = atoi(newstr);
				if(str.Find("max configuration used :=")!=-1)
					m_MAX_CNFG = atoi(newstr);

				if(str.Find("head 0 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[0] = TRUE;
					else
						m_Hd_En[0] = FALSE;
				}

				if(str.Find("head 1 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[1] = TRUE;
					else
						m_Hd_En[1] = FALSE;
				}

				if(str.Find("head 2 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[2] = TRUE;
					else
						m_Hd_En[2] = FALSE;
				}
				if(str.Find("head 3 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[3] = TRUE;
					else
						m_Hd_En[3] = FALSE;
				}
				if(str.Find("head 4 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[4] = TRUE;
					else
						m_Hd_En[4] = FALSE;
				}
				if(str.Find("head 5 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[5] = TRUE;
					else
						m_Hd_En[5] = FALSE;
				}
				if(str.Find("head 6 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[6] = TRUE;
					else
						m_Hd_En[6] = FALSE;
				}
				if(str.Find("head 7 :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Hd_En[7] = TRUE;
					else
						m_Hd_En[7] = FALSE;
				}
				//if(str.Find("energy window lower level [1] :=")!=-1)
				//	m_lld = atoi(newstr);
				//if(str.Find("energy window upper level [1] :=")!=-1)
				//	m_uld = atoi(newstr);
				if(str.Find("working directory :=")!=-1)
					m_Work_Dir = newstr;
				//if(str.Find("normalization file name and path :=")!=-1)
				//	m_Norm_File = newstr;
				//if(str.Find("blank file name and path :=")!=-1)
				//	m_Blank_File = newstr;
				//if(str.Find("calibration factor :=")!=-1)
				//	m_Cal_Factor = (float)atof(newstr);
				

				if(str.Find("Filename by patient name option :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Opt_Name = TRUE;
					else
						m_Opt_Name = FALSE;
				}					
				if(str.Find("Filename by patient ID :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Opt_PID = TRUE;
					else
						m_Opt_PID = FALSE;
				}	
				if(str.Find("Filename by case ID :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Opt_CID = TRUE;
					else
						m_Opt_CID = FALSE;
				}
				if(str.Find("Filename by random number :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_Opt_Num = TRUE;
					else
						m_Opt_Num = FALSE;
				}

				if(str.Find("Log file flag :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_LogFlag = TRUE;
					else
						m_LogFlag = FALSE;
				}
				if(str.Find("Error dialog message flag :=")!=-1)
				{
					if(str.Find("TRUE")!=-1)
						m_msg_err = TRUE;
					else
						m_msg_err = FALSE;
				}

				if(str.Find("Dead time constant :=")!=-1)
				m_DTC = atof(newstr);
				
			}
		}
	}
	if( in_file!= NULL )
		fclose(in_file);

}

void CCDHI::SortData(CString str, CString* newstr)
{
	int y = str.Find(":= ");
	int z = str.GetLength();
	*newstr = str.Mid(y+3,z - y - 4);
	//*num = atoi(str.Mid(y+3,z - y - 4));
	//cout << *num << endl;
	
}

void CCDHI::GetSysMode(int head)
{

	int i, ret,x,y;
	char cmdbuf[10];
	char buffer [2048];
	CString str, sd;
	// Set the command up
	sprintf(cmdbuf,"A");
	ret = SendCmd(cmdbuf,buffer, head, 2);

	str.Format("%s",buffer);
	if (ret >= 0 )
	{		
		// Parse through each column of the row data 
		//str.Format("%s",buffer);
		x = str.Find(' ',1);
		if (m_HRRT)
			y = 22;
		if (m_PETSPECT)
			y = 18;
		if (x >0)
		{	
			for (i=0;i<y;i++)
			{
				if (i< y-1)
				{			
					// Parse out spaces
					while(str.Mid(x+1,1) == ' ')
						x++;
					ret = str.Find(' ',x+1);
				}
				else
					ret = str.GetLength();
				switch(i)
				{
				// Get High Voltage
				case 0: 
					if (m_HRRT)
						HrrtMode.HV1 = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.HV = atoi(str.Mid(x+1,ret-x-1));
					break;	
				case 1:
					if (m_HRRT)
						HrrtMode.HV2 = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DC5 = (float)atof(str.Mid(x+1,ret-x-1));
					break;					
				case 2: 
					if (m_HRRT)
						HrrtMode.HV3 = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DC24 = (float)atof(str.Mid(x+1,ret-x-1));
					break;
				case 3:
					if (m_HRRT)
						HrrtMode.HV4 = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DC52 = (float)atof(str.Mid(x+1,ret-x-1));
					break;
				case 4:
					if (m_HRRT)
						HrrtMode.HV5 = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.LTI_Temp = (float)atof(str.Mid(x+1,ret-x-1));
					break;
				// Get DC Voltage
				case 5:
					if (m_HRRT)
						HrrtMode.DC5  = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DHI1 = (float)atof(str.Mid(x+1,ret-x-1));
					break;					
				case 6:
					if (m_HRRT)
						HrrtMode.DC12 = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DHI2 = (float)atof(str.Mid(x+1,ret-x-1));
					break;
				case 7:
					if (m_HRRT)
						HrrtMode.DC52 = (float)atof(str.Mid(x+1,ret-x-1));	
					else if (m_PETSPECT)
						PetSpectMode.DHI3 = (float)atof(str.Mid(x+1,ret-x-1));
					break;							
				// Get Temperatires
				case 8:
					if (m_HRRT)
						HrrtMode.LocalT = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DHI4 = (float)atof(str.Mid(x+1,ret-x-1));
					break;					
				case 9:
					if (m_HRRT)
						HrrtMode.RemoteT = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DHI5 = (float)atof(str.Mid(x+1,ret-x-1));
					break;
				case 10:
					if (m_HRRT)
						HrrtMode.DHI1 = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.DHI6 = (float)atof(str.Mid(x+1,ret-x-1));
					break;		
				case 11:
					if (m_HRRT)
						HrrtMode.DHI2 = (float)atof(str.Mid(x+1,ret-x-1));
					if (m_PETSPECT)
					{
						if (str.Mid(x+1,ret-x-1)=="off")
							PetSpectMode.mode  = 0;
						if (str.Mid(x+1,ret-x-1)=="pp" || str.Mid(x+1,ret-x-1) == "PP")
							PetSpectMode.mode  = 1;
						if (str.Mid(x+1,ret-x-1)=="te" || str.Mid(x+1,ret-x-1) == "TE")
							PetSpectMode.mode  = 2;
						if (str.Mid(x+1,ret-x-1)=="run" )
							PetSpectMode.mode  = 3;
						if (str.Mid(x+1,ret-x-1)=="en" )
							PetSpectMode.mode  = 4;
						if (str.Mid(x+1,ret-x-1)=="sd" )
							PetSpectMode.mode  = 5;
						if (str.Mid(x+1,ret-x-1)=="time" )
							PetSpectMode.mode  = 6;
						if (str.Mid(x+1,ret-x-1)=="list" )
							PetSpectMode.mode  = 7;
						if (str.Mid(x+1,ret-x-1)=="trans" )
							PetSpectMode.mode  = 8;
						if (str.Mid(x+1,ret-x-1)=="test1" )
							PetSpectMode.mode  = 9;
						if (str.Mid(x+1,ret-x-1)=="test2" )
							PetSpectMode.mode  = 10;
						if (str.Mid(x+1,ret-x-1)=="test3" )
							PetSpectMode.mode  = 11;
					}
					break;		
				case 12:
					if (m_HRRT)
						HrrtMode.DHI3 = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.uld = atoi(str.Mid(x+1,ret-x-1));
					break;		
				case 13:
					if (m_HRRT)
						HrrtMode.DHI4 = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.lld = atoi(str.Mid(x+1,ret-x-1));
					break;		
				case 14:
					if (m_HRRT)
						HrrtMode.DHI5 = (float)atof(str.Mid(x+1,ret-x-1));
					if (m_PETSPECT)
					{
						if (str.Mid(x+1,ret-x-1)=="none" )
							PetSpectMode.speed = 0;
						if (str.Mid(x+1,ret-x-1)=="fast" )
							PetSpectMode.speed = 1;
						if (str.Mid(x+1,ret-x-1)=="slow" )
							PetSpectMode.speed = 2;
						if (str.Mid(x+1,ret-x-1)=="all" )
							PetSpectMode.speed = 3;
					}
					break;		
				case 15:
					if (m_HRRT)
						HrrtMode.DHI6 = (float)atof(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
						PetSpectMode.config = atoi(str.Mid(x+1,ret-x-1));
					break;	
				// Get Refrence 
				case 16:
					if (m_HRRT)
						HrrtMode.ref = atoi(str.Mid(x+1,ret-x-1));
					else if (m_PETSPECT)
					{
						if (str.Mid(x+1,ret-x-1)=="on" )
							PetSpectMode.Pole_Zero = 1;
						if (str.Mid(x+1,ret-x-1)=="off" )
							PetSpectMode.Pole_Zero = 0;
					}
					break;						
				// Get Mode
				case 17:
					if (m_HRRT)
					{
						if (str.Mid(x+1,ret-x-1)=="off")
							HrrtMode.mode  = 0;
						if (str.Mid(x+1,ret-x-1)=="pp" || str.Mid(x+1,ret-x-1) == "PP")
							HrrtMode.mode  = 1;
						if (str.Mid(x+1,ret-x-1)=="te" || str.Mid(x+1,ret-x-1) == "TE")
							HrrtMode.mode  = 2;
						if (str.Mid(x+1,ret-x-1)=="run" )
							HrrtMode.mode  = 3;
						if (str.Mid(x+1,ret-x-1)=="en" )
							HrrtMode.mode  = 4;
						if (str.Mid(x+1,ret-x-1)=="sd" )
							HrrtMode.mode  = 5;
						if (str.Mid(x+1,ret-x-1)=="time" )
							HrrtMode.mode  = 6;
						if (str.Mid(x+1,ret-x-1)=="list" )
							HrrtMode.mode  = 7;
						if (str.Mid(x+1,ret-x-1)=="trans" )
							HrrtMode.mode  = 8;
						if (str.Mid(x+1,ret-x-1)=="test1" )
							HrrtMode.mode  = 9;
						if (str.Mid(x+1,ret-x-1)=="test2" )
							HrrtMode.mode  = 10;
						if (str.Mid(x+1,ret-x-1)=="test3" )
							HrrtMode.mode  = 11;

					}
					else if (m_PETSPECT)
					{
						if (str.Mid(x+1,ret-x-1)=="1X" )
							PetSpectMode.Pole_Zero = 1;
						if (str.Mid(x+1,ret-x-1)=="5X" )
							PetSpectMode.Pole_Zero = 5;
					}
					break;						
				// Get lld uld 
				case 18:
					if (m_HRRT)
						HrrtMode.lld = atoi(str.Mid(x+1,ret-x-1));
					break;						
				case 19:
					if (m_HRRT)
						HrrtMode.uld = atoi(str.Mid(x+1,ret-x-1));
					break;
				// Get Fast Slow All
				case 20:
					if (m_HRRT)
					{
						if (str.Mid(x+1,ret-x-1)=="none" )
							HrrtMode.speed = 0;
						if (str.Mid(x+1,ret-x-1)=="fast" )
							HrrtMode.speed = 1;
						if (str.Mid(x+1,ret-x-1)=="slow" )
							HrrtMode.speed = 2;
						if (str.Mid(x+1,ret-x-1)=="all" )
							HrrtMode.speed = 3;
					}
					break;
				//  Get Config
				case 21:
					if (m_HRRT)
						HrrtMode.config = atoi(str.Mid(x+1,ret-x-1));
					break;
				}
				x = ret;
			}
		}
	}
}

void CCDHI::InitSystem()
{
	int a,b,i;
	

	// Read the ini file
	ReadIniFile();
	// Check Enabled Head
	CheckEnHead();

	// Allocate the memory for all heads
	a =  11 * m_MAX_COL * m_MAX_ROW * m_MAX_CNFG;
	b =120;
	for (i=0;i<m_TOTAL_HD;i++)
	{

		// Do head 0
		if (m_Hd_En[i])
		switch (i)
		{
			case 0:
				m_Head_0 =  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_0 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 1
			case 1:
				m_Head_1=  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_1 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 2
			case 2:
				m_Head_2 =  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_2 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 3
			case 3:
				m_Head_3=  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_3 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 4
			case 4:
				m_Head_4=  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_4 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 5
			case 5:
				m_Head_5 =  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_5 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 6
			case 6:
				m_Head_6=  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_6 =  (int *)calloc( b, sizeof( int ) );
				break;
			// Do head 7
			case 7:
				m_Head_7 =  (int *)calloc( a, sizeof( int ) );
				m_Hd_Sin_7 =  (int *)calloc( b, sizeof( int ) );
				break;
		}
	}


	m_init = TRUE;
}

void CCDHI::GetHeadConfig(int head, int conf)
{
	char buffer[2048];
	char cmdbuf[10];
	int  chan, ret, x, i,totchan;
	CString str;
	totchan = m_MAX_ROW * m_MAX_COL;

	//for (conf=0;conf<m_MAX_CNFG;conf++)
	//{
	for(chan=0;chan<(m_MAX_COL * m_MAX_ROW);chan++) // (m_MAX_COL * m_MAX_ROW) = 9 * 13 = 117
	{	
		// Setup the command
		sprintf(cmdbuf,"G%d %d", conf,chan);
		ret = SendCmd(cmdbuf,buffer, head, 2);
/*		pSerBus.serialbus_access_control->Lock();

		sprintf(cmdbuf,"%dG%d %d",head,conf,chan);
		pSerBus.SendCmd(cmdbuf);
		ret = pSerBus.GetResponse(buffer, 2048, 1);
		if (str.Find('!',1) !=-1)
			// An Async command came back reread the buffer
			ret = pSerBus.GetResponse(buffer, 2048, 1);
		//Sleep(10);
*/
		if (ret >= 0)
		{
			// Parse through each column of the row data 
			str.Format("%s",buffer);
			x = str.Find(' ',1);
			if (x >0)
			{	
				for (i=0;i<11;i++)
				{
					if (i<10)
						ret = str.Find(' ',x+1);
					else
						ret = str.GetLength();
					switch(head)
					{
					case 0:
						m_Head_0[conf*totchan + chan*11 +i] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 4:
						m_Head_4[conf*120 + chan*11 +i] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					}
					x = ret;
				}
			}
		}
//		pSerBus.serialbus_access_control->Unlock();
	}
	//}

}

void CCDHI::GetSingles(int head)
{
	char buffer[2048], cmdbuf[25];
	int  ret, row,x,i,inc = 0;
	CString str,sd,dis,cl;

	
	for (row=0;row<m_MAX_ROW;row++)
	{	
		// Set the command up
		sprintf(cmdbuf,"Q %d", row);
		ret = SendCmd(cmdbuf,buffer, head, 2);

		str.Format("%s",buffer);
		if (ret >= 0)
		{
			// Parse through each column of the row data 
			ret = str.Find('V',1);
			x = str.Find(' ',ret + 1);
			if (ret >0)
			{	
				for (i=0;i<m_MAX_COL;i++)
				{
					if (i<m_MAX_COL-1)
						ret = str.Find(' ',x+1);
					else
						ret = str.GetLength();
					if(atoi(str.Mid(x+1,ret-x-1)) == 0)
					{
						sd .Format("Head %d Chan %d Reported zero",head, inc);
						//m_List.AddString(sd);
					}
					switch (head)
					{
					case 0:
						m_Hd_Sin_0[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 1:
						m_Hd_Sin_1[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 2:
						m_Hd_Sin_2[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 3:
						m_Hd_Sin_3[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 4:
						m_Hd_Sin_4[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 5:
						m_Hd_Sin_5[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 6:
						m_Hd_Sin_6[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					case 7:
						m_Hd_Sin_7[inc] = atoi(str.Mid(x+1,ret-x-1));	
						break;
					}
					inc ++;
					x = ret;
				}
			}
		}
//		pSerBus.serialbus_access_control->Unlock();

	}

}

void CCDHI::WriteIniFile()
{
	int i;
	
	CString str, newstr;

	GetAppPath(&newstr);
	newstr+= "ScanIt.ini";
	// Open the header file
	FILE  *out_file;
	out_file = fopen(newstr,"w");
	if( out_file!= NULL )
	{
		if (m_HRRT)
			fprintf( out_file, "system := HRRT\n" );
		else if (m_PETSPECT)
			fprintf( out_file, "system := PETSPECT\n" );
		// Total Heads
		fprintf( out_file, "total head := %d\n", m_TOTAL_HD );
		fprintf( out_file, "max row number := %d\n", m_MAX_ROW );
		fprintf( out_file, "max column number := %d\n", m_MAX_COL );
		fprintf( out_file, "max configuration used := %d\n", m_MAX_CNFG );
		for (i=0;i<8;i++)
			if (m_Hd_En[i])
				fprintf( out_file, "head %d := TRUE\n", i );
			else
				fprintf( out_file, "head %d := FALSE\n", i );
		fprintf( out_file, "working directory := %s\n", m_Work_Dir);
		//fprintf( out_file, "normalization file name and path := %s\n", m_Norm_File);
		//fprintf( out_file, "blank file name and path := %s\n",m_Blank_File);
		//fprintf( out_file, "calibration factor := %e\n",m_Cal_Factor);
		if (m_Opt_Name)
			fprintf( out_file, "Filename by patient name := TRUE\n");
		else
			fprintf( out_file, "Filename by patient name:= FALSE\n");
		if (m_Opt_PID)
			fprintf( out_file, "Filename by patient ID := TRUE\n");
		else
			fprintf( out_file, "Filename by patient ID := FALSE\n");
		if (m_Opt_CID)
			fprintf( out_file, "Filename by case ID := TRUE\n");
		else
			fprintf( out_file, "Filename by case ID:= FALSE\n");
		if (m_Opt_Num)
			fprintf( out_file, "Filename by random number := TRUE\n");
		else
			fprintf( out_file, "Filename by random number := FALSE\n");
		if (m_LogFlag)
			fprintf( out_file, "Log file flag := TRUE\n");
		else
			fprintf( out_file, "Log file flag := FALSE\n");
		if (m_msg_err)
			fprintf( out_file, "Error dialog message flag := TRUE\n");
		else
			fprintf( out_file, "Error dialog message flag := FALSE\n");
	
		fprintf(out_file, "Dead time constant := %g\n", m_DTC);
	}
	if( out_file!= NULL )
		fclose(out_file);
}

void CCDHI::CheckEnHead()
{
	int i;
	bool b;

	b = FALSE;
	for(i=0;i<m_TOTAL_HD;i++)
		if (!m_Hd_En[i])
			b = TRUE;
	if (b)
		m_All_Hd_En = FALSE;
	else
		m_All_Hd_En = TRUE;
}

void CCDHI::CheckSinZero(bool* fndFlag)
{
	int hd=0, x;
	*fndFlag = FALSE;

	while (hd<m_TOTAL_HD && !*fndFlag)
	{
		if(m_Hd_En[hd])
			for (x=0;x<(m_MAX_ROW * m_MAX_COL); x++)
			{
				switch (hd)
				{
				case 0:
					if (m_Hd_Sin_0[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 1:
					if (m_Hd_Sin_1[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 2:
					if (m_Hd_Sin_2[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 3:
					if (m_Hd_Sin_3[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}						
					break;
				case 4:
					if (m_Hd_Sin_4[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 5:
					if (m_Hd_Sin_5[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 6:
					if (m_Hd_Sin_6[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				case 7:
					if (m_Hd_Sin_7[x] == 0)
					{
						*fndFlag = TRUE;
						break;	// exit for loop
					}	
					break;
				}

			}
		hd++;
	}
}

int CCDHI::SendPing()
{
	// Send a ping command
	char buffer[2048], cmdbuf[25];
	int ret;

	sprintf(cmdbuf,"P 1");
	ret = SendCmd(cmdbuf,buffer, 64, 1);
	return ret;

}
int CCDHI::hex2bin(char *bindata, char *hexdata, int maxlength)
{
	// Convert Hex to binary
	int i, num;
	char *hexvals = "0123456789abcdef";

	int length = strlen(hexdata) / 2;
	if (length > maxlength)
		return 0;

	for (i = 0; i < length; i++)
	{
		num = (int)(strchr(hexvals,tolower(hexdata[i * 2])) - hexvals) << 4;
		num |= (int)(strchr(hexvals,tolower(hexdata[i * 2 +1])) - hexvals);
		bindata[i] = (char)num;
	}

	return i;
}

int CCDHI::Upload(CString filename,CString file2beupld, int head)
{
	char *cptr;
	char buffer[2048], cmdbuf[25];
	CString str, sd;
	int retcode = 0;
	unsigned long checksum;
	unsigned long local_csum;
	int state = 0;
	int block;
	int errors = 0;
	FILE *fp;


	fp = fopen(filename,"wb");

	if (!fp)
	  return E_OPEN_FAILED;
	// disable asyncronous messaging on the coincidence controller
	sprintf(cmdbuf,"Q 0");
	errors = SendCmd(cmdbuf, buffer, 64, 1);

	if (errors !=0)
		return errors;
	for (block = 1;; block++)
	{
		//sprintf(cmdbuf,"%dU %s %d",head,file2beupld,state);
		sprintf(cmdbuf,"U %s %d",file2beupld,state);
		if (state == 0) state = 1;  // change states after first open
		errors = SendCmd(cmdbuf,buffer, head, 5);
		if (errors !=0 && errors != -33)
			return errors;



		cptr = strtok(buffer," ");  // ignore initial preamble
		if (!cptr)   // syntax error, better get out now
			break;
		cptr = strtok(NULL," ");     // get the return code
		if (cptr)
			retcode = atoi(cptr);

		if (retcode != 0)
			break;

		// parse the checksum
		cptr = strtok(NULL," ");
		checksum = atol(cptr);

		// parse the data out
		cptr = strtok(NULL," ");

		int length = hex2bin(buffer,cptr,1024);
		local_csum = 0;
		for (int i = 0; i < length; i++)
			local_csum = local_csum + ((unsigned int)buffer[i] & 0xff);

		if (local_csum != checksum)
		{
			//if (verbose) cout << "Error:  checksums do not match actual=" << checksum << " local=" << local_csum << " length=" << length << endl;
				errors++;
		}

		fwrite(buffer,1,length,fp);
	};

	// enable asyncronous messaging on the coincidence controller
	sprintf(cmdbuf,"Q 1");
	errors = SendCmd(cmdbuf,buffer, 64, 1);

	fclose(fp);

	// return the appropriate error condition if present
	if (retcode && retcode != -33)	// -33 is E_EOF
	  return retcode;
	else if (errors)
	  return errors;
	else
	  return 0;
}


int CCDHI::DownLoad(char *sourcefile, char *targetfile, int head)
{
	char buffer[2048], cmdbuf[512];
	//CString str, sd;
	int bytesread;
	long checksum;
	char int_target[128];
	char *cptr;
	int i = 0;
	int errors = 0;

	#define DHI_BUFSIZE 256

	FILE *fp = fopen(sourcefile,"rb");
	if (!fp)
	  return E_OPEN_FAILED;

	if ((cptr = strrchr(targetfile,'\\')) != 0)
	  strcpy(int_target,cptr + 1);
	else
	  strcpy(int_target,targetfile);

	// go to the end of the file
	fseek(fp, 0, SEEK_END);

	// get the size (in blocks) of the file
	float size = (float) ftell(fp) / DHI_BUFSIZE;

	// go to the start of the file
	fseek(fp, 0, SEEK_SET);

	// disable asyncronous messaging on the coincidence controller
//	sprintf(cmdbuf,"Q 0");
//	errors = SendCmd(cmdbuf,buffer, 64, 1);
//	if (errors !=0)
//		return errors;

	static char hexbuffer[1024];
	int control = 0;

	do
	{
		//if (verbose) cout << "Downloading Buffer [" << targetname << "] " << (int)((i++ / size) * 100.0) << "%\r" << flush;

		bytesread = fread(buffer,1,DHI_BUFSIZE,fp);
		if (bytesread == 0)  // nothing left to download, so close the file
		{


			sprintf(cmdbuf,"D %s 2 0",int_target);
			errors = SendCmd(cmdbuf,buffer, head, 1);
			if (errors !=0)
				return errors;

			break;
		}
		else if (bytesread < DHI_BUFSIZE)
			control = 2;

		// compute the checksum and hex data
		checksum = bintohex(buffer,hexbuffer,bytesread);

		// format the command
		char csumbuf[32];
		char controlbuf[32];
		sprintf(csumbuf,"%ld ",checksum);
		strcpy(cmdbuf,"D ");
		strcat(cmdbuf,int_target);
		sprintf(controlbuf," %d ",control);
		strcat(cmdbuf,controlbuf);
		strcat(cmdbuf,csumbuf);
		strcat(cmdbuf,hexbuffer);

		// send it down, retry up to 4 times if needed

		errors = SendCmd(cmdbuf, buffer, head, 1);
		if (errors !=0)
			return errors;
		control = 1;
	}
	while (bytesread == DHI_BUFSIZE);  // last load will be less than 256

	// close file
	fclose(fp);

	// enable asyncronous messaging on the coincidence controller
	//sprintf(cmdbuf,"Q 1");
	//errors = SendCmd(cmdbuf,buffer,  64, 1);
	if (errors !=0)
		return errors;
	

	return (0);

}

void CCDHI::GetAppPath(CString* AppPath)
{
	// Get the proper path
	CString str;
	char  szAppPath[MAX_PATH] = "";
	GetCurrentDirectory(MAX_PATH,szAppPath);
	str.Format("%s\\",szAppPath);
	*AppPath = str;


}

long CCDHI::bintohex(char *rawdata, char *hex, int count)
{
	// convert Binary to Hex
	char *rawptr = rawdata;
	unsigned int sumval;
	static char tmp[128];
	*hex = '\0';
	long csum = 0;

	for (int i = 0; i < count; i++)
	{
		sumval = ((unsigned int)*rawptr) & 0xff;
		csum += sumval;

		sprintf(tmp,"%X",sumval);
		if (strlen(tmp) < 2)
			strcat(hex,"0");
		strcat(hex,tmp);

		rawptr++;
	}

	return csum;
}

int CCDHI::SendCmd(char *cmdSnd,char *cmdRcv, int hd, int timeout)
{
	char buffer[2048], cmdbuf[512];
	CString str, sd;
	int  inc = 0,num, ret;

	str.Format("%s",cmdSnd);
	pSerBus.serialbus_access_control->Lock();

	sprintf(cmdbuf,"%d%s",hd,cmdSnd);
	
	if (hd < 10)
		num = 2;
	else
		num = 3;

	sd.Format("%d%s", hd, str.Mid(0,1));
	// Send the command
	pSerBus.SendCmd(cmdbuf);
	// Get the respond
	ret = pSerBus.GetResponse(buffer, 2048, timeout);
	// Check if the command timed out
	if(ret != 0)
	{
		pSerBus.serialbus_access_control->Unlock();
		return E_TIMED_OUT;
	}

	str.Format("%s",buffer);

	long finish_time = time(0) + timeout;

	while (((str.Find('!',1) !=-1) || (str.Mid(0,num) != sd)) && 	finish_time > time(0))// Get any ASync Commands  
	//while (((str.Find('!',1) !=-1) || (str.Mid(0,num) != sd)) && 	inc < 3)// Get any ASync Commands  

		// An Async command came back reread the buffer
	{
		if ((str.Find('-',1) !=-1) )//&& (str.Mid(0,2) == sd))	// Check for "-" => error
		{
			pSerBus.SendCmd(cmdbuf);		// Resend command
			m_err_num = returncode(buffer);
			pSerBus.serialbus_access_control->Unlock();
			return m_err_num;
		}
		ret = pSerBus.GetResponse(buffer, 2048, 2);
		str.Format("%s",buffer);	
	}


	// Release control of the serial port
	pSerBus.serialbus_access_control->Unlock();

	if (ret >0)
		return E_TIMED_OUT;
	else
	{
		sprintf(cmdRcv, "%s", buffer);
		m_err_num = returncode(buffer);
		// Log the error to error file 
		if (m_err_num < 0)
		{
			CString fstr;
			if (m_err_num > MAXERRORCODE)
				fstr.Format("%s\n",e_short_text[-1*m_err_num]);
			else
				fstr = ("Unknow Error\n");
			if(m_LogFlag)
			{
				FILE* out_file;
				out_file = fopen(m_errfile,"a");
				if (out_file != NULL)
				{
					SYSTEMTIME st;
					GetLocalTime(&st);

					fprintf( out_file, "Time: %2d:%2d:%2d\n", st.wHour,st.wMinute,st.wSecond);	 
					fprintf( out_file, "Date: %2d:%2d:%4d\n", st.wDay,st.wMonth,st.wYear);	 
					fprintf( out_file, "%s", fstr );	 
					fclose (out_file);
				}
			}
		}
		return m_err_num;
	}


}

int CCDHI::returncode(char *resp)
{
	int retval = 0;
	char *cptr = strchr(resp,' ');
	if (cptr)
		retval = atoi((cptr + 1));
	return retval;
}

int CCDHI::ResetHead(int head)
{
	char buffer[2048], cmdbuf[25];
	int ret;

	sprintf(cmdbuf,"R 999");
	ret = SendCmd(cmdbuf,buffer, head, 1);
	return ret;
}

int CCDHI::SendXCom(int comnum, int head)
{
	char buffer[2048], cmdbuf[25];
	int ret;

	sprintf(cmdbuf,"X %d", comnum);
	ret = SendCmd(cmdbuf,buffer, head, 1);
	return ret;
}

int CCDHI::DOS(char *doscmd, int head)
{
	char buffer[2048], cmdbuf[25];
	int ret;

	sprintf(cmdbuf,"O %s", doscmd);
	ret = SendCmd(cmdbuf,buffer, head, 1);
	return ret;
}
