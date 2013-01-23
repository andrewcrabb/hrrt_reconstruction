// Abstract_DHI.h : Declaration of the CAbstract_DHI

#ifndef __ABSTRACT_DHI_H_
#define __ABSTRACT_DHI_H_

#include "resource.h"       // main symbols
#include <atlbase.h>
#include <stdio.h>

// always included
#include "DHI_Constants.h"

// includes based on build
#ifdef ECAT_SCANNER

#include "PetCTGCIdll.h"
#include "ServMgrCommon.h"
#include "GCIResponseCodes.h"

#else

#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"
#include "ecodes.h"
#include "OSCom_i.c"
#include "OSCom.h"
#include "cpcom_i.c"
#include "cpcom.h"

#endif

/////////////////////////////////////////////////////////////////////////////
// CAbstract_DHI
class ATL_NO_VTABLE CAbstract_DHI : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAbstract_DHI, &CLSID_Abstract_DHI>,
	public IAbstract_DHI
{
public:
	CAbstract_DHI()
	{
	}

	~CAbstract_DHI()
	{
		// handle shutdown
		Close_Globals();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ABSTRACT_DHI)
DECLARE_CLASSFACTORY_SINGLETON(CAbstract_DHI)					// only one instance running at a time

DECLARE_PROTECT_FINAL_CONSTRUCT()	HRESULT FinalConstruct();

BEGIN_COM_MAP(CAbstract_DHI)
	COM_INTERFACE_ENTRY(IAbstract_DHI)
END_COM_MAP()

// IAbstract_DHI Methods
public:
	STDMETHOD(Last_Async_Message)(/*[out]*/ unsigned char LastMsg[2048]);
	STDMETHOD(Build_CRM)(/*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long *pStatus);
	STDMETHOD(Set_Block_Crystal_Position)(/*[in]*/ long Head, /*[in]*/ long Block, /*[in]*/ long Peaks[512], /*[in]*/ long StdDev, /*[out]*/ long *pStatus);
	STDMETHOD(Set_Block_Crystal_Peaks)(/*[in]*/ long Head, /*[in]*/ long Block, /*[in]*/ long Peaks[256], /*[out]*/ long *pStatus);
	STDMETHOD(Set_Block_Time_Correction)(/*[in]*/ long Head, /*[in]*/ long Block, /*[in]*/ long Correction[256], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Block_Time_Correction)(/*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long Correction[256], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Block_Crystal_Peaks)(/*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long Peaks[256], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Block_Crystal_Position)(/*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long Peaks[512], /*[out]*/ long *pStatus);
	STDMETHOD(Report_Temperature_Voltage)(/*[out]*/ float *pCP_Temperature, /*[out]*/ float *pMinimum_Temperature, /*[out]*/ float *pMaximum_Temperature, /*[out]*/ long *pNumber_DHI_Temperatures, /*[out]*/ long *pNumHeads, /*[out, size_is(1, *pNumHeads)]*/ PhysicalInfo **Info, /*[out]*/ long *pStatus);
	STDMETHOD(SetLog)(/*[in]*/ long OnOff, /*[out]*/ long *pStatus);
	STDMETHOD(KillSelf)();
	STDMETHOD(Ring_Singles)(/*[out]*/ long *pNumRings, /*[out, size_is(1, *pNumRings)]*/ long** RingSingles, /*[out]*/ long *pStatus);
	STDMETHOD(CountRate)(/*[out]*/ long *pCorrectedSingles, /*[out]*/ long *pUncorrectedSingles, /*[out]*/ long *pPrompts, /*[out]*/ long *pRandoms, /*[out]*/ long *pScatters, /*[out]*/ long *pStatus);
	STDMETHOD(Head_Status)(/*[out]*/ long *pNumHeads, /*[out, size_is(1, *pNumHeads)]*/ HeadInfo **Info, /*[out]*/ long *pStatus);
	STDMETHOD(Initialize_PET)(/*[in]*/ long ScanType, /*[in]*/ long LLD, /*[in]*/ long ULD, /*[out]*/ long *pStatus);
	STDMETHOD(DataStream)(/*[in]*/ long OnOff, /*[out]*/ long *pStatus);
	STDMETHOD(Health_Check)(/*[out]*/ long *pNumFailed, /*[out, size_is(1,*pNumFailed)]*/ long **Failed, /*[out]*/ long *pStatus);
	STDMETHOD(Force_Reload)();
	STDMETHOD(Diagnostics)(/*[out]*/ long *pResult, /*[out]*/ long *pStatus);
	STDMETHOD(Error_Lookup)(/*[in]*/ long ErrorCode, /*[out]*/ long *Code, /*[out]*/ unsigned char ErrorString[MAX_STR_LEN]);
	STDMETHOD(Zap)(/*[in]*/ long ZapMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long *pStatus);
	STDMETHOD(Determine_Delay)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long *pStatus);
	STDMETHOD(Determine_Offsets)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Block, /*[out]*/ long *pStatus);
	STDMETHOD(Coincidence_Mode)(/*[in]*/ long Transmission, /*[in]*/ long ModulePairs, /*[out]*/ long *pStatus);
	STDMETHOD(Time_Mode)(/*[in]*/ long Transmission, /*[in]*/ long ModulePairs, /*[out]*/ long *pStatus);
	STDMETHOD(PassThru_Mode)(/*[in]*/ long Head[MAX_HEADS], long PtSrc[MAX_HEADS], /*[out]*/ long *pStatus);
	STDMETHOD(TagWord_Mode)(/*[out]*/ long *pStatus);
	STDMETHOD(Test_Mode)(/*[out]*/ long *pStatus);
	STDMETHOD(Set_Head_Mode)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long Block, /*[in]*/ long LLD, /*[in]*/ long ULD, /*[out]*/ long *pStatus);
	STDMETHOD(Initialize_Scan)(/*[in]*/ long Transmission, /*[in]*/ long ModulePairs, /*[in]*/ long Configuration, /*[in]*/ long Layer, /*[in]*/ long LLD, /*[in]*/ long ULD, /*[out]*/ long *pStatus);
	STDMETHOD(Start_Scan)(/*[out]*/ long *pStatus);
	STDMETHOD(Stop_Scan)(/*[out]*/ long *pStatus);
	STDMETHOD(Refresh_Analog_Settings)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[out]*/ long *pStatus);
	STDMETHOD(Get_Analog_Settings)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[out]*/ long Settings[MAX_BLOCKS * MAX_SETTINGS], /*[out]*/ long *pStatus);
	STDMETHOD(Set_Analog_Settings)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Settings[MAX_BLOCKS * MAX_SETTINGS], /*[out]*/ long *pStatus);
	STDMETHOD(File_Upload)(/*[in]*/ long Head, /*[in]*/ unsigned char Filename[MAX_STR_LEN], /*[out]*/ long *Filesize, /*[out]*/ unsigned long *CheckSum, /*[out, size_is(1, *Filesize)]*/ unsigned char** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(File_Download)(/*[in]*/ long Head, /*[in]*/ long Filesize, /*[in]*/ unsigned long CheckSum, /*[in]*/ unsigned char Filename[MAX_STR_LEN], /*[in, size_is(1, Filesize)]*/ unsigned char** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(High_Voltage)(/*[in]*/ long OnOff, /*[out]*/ long *pStatus);
	STDMETHOD(Point_Source)(/*[in]*/ long Head, /*[in]*/ long OnOff, /*[out]*/ long *pStatus);
	STDMETHOD(Point_Source_Status)(/*[in]*/ long Head, /*[out]*/ long *pOnOff, /*[out]*/ long *pStatus);
	STDMETHOD(Get_Statistics)(/*[out]*/ long Statistics[19], /*[out]*/ long *pStatus);
	STDMETHOD(Singles)(/*[in]*/ long Head, /*[in]*/ long SinglesType, /*[out]*/ long Singles[MAX_BLOCKS], /*[out]*/ long *pStatus);
	STDMETHOD(Progress)(/*[in]*/ long Head, /*[out]*/ long *pPercent, /*[out]*/ long *pStatus);
	STDMETHOD(Ping)(/*[in]*/ long Head, /*[out]*/ unsigned char BuildID[MAX_STR_LEN], /*[out]*/ long *pStatus);
	STDMETHOD(Report_Temperature)(/*[out]*/ float Temperature[35], /*[out]*/ long *pStatus);
	STDMETHOD(Report_Voltage)(/*[out]*/ float Voltage[64], /*[out]*/ long *pStatus);
	STDMETHOD(Report_HRRT_High_Voltage)(/*[out]*/ long Voltage[80], /*[out]*/ long *pStatus);
	STDMETHOD(Hardware_Configuration)(/*[out]*/ long *Filesize, /*[out]*/ unsigned long *CheckSum, /*[out, size_is(1, *Filesize)]*/ unsigned char** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(Check_Head_Mode)(/*[in]*/ long Head, /*[out]*/ long *pDataMode, /*[out]*/ long *pConfiguration, /*[out]*/ long *pLayer, /*[out]*/ long *pLLD, /*[out]*/ long *pULD, /*[out]*/ long *pStatus);
	STDMETHOD(Check_CP_Mode)(/*[out]*/ long *pMode, /*[out]*/ long *pStatus);
	STDMETHOD(Set_Temperature_Limits)(/*[in]*/ float Low, /*[in]*/ float High, /*[out]*/ long *pStatus);
	STDMETHOD(Reset_CP)(/*[in]*/ long Reset, /*[out]*/ long *pStatus);
	STDMETHOD(Reset_Head)(/*[in]*/ long Head, /*[out]*/ long *pStatus);
	STDMETHOD(Load_RAM)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long RAM_Type, /*[out]*/ long *pStatus);
	STDMETHOD(Time_Window)(/*[in]*/ long WindowSize, /*[out]*/ long *pStatus);
	STDMETHOD(OS_Command)(/*[in]*/ long Head, /*[in]*/ unsigned char CommandString[MAX_STR_LEN], /*[out]*/ long *pStatus);
	STDMETHOD(Pass_Command)(/*[in]*/ long Head, /*[in]*/ unsigned char CommandString[MAX_STR_LEN], /*[out]*/ unsigned char ResponseString[256], /*[out]*/ long *pStatus);
	STDMETHOD(Tag_Insert)(/*[in]*/ long Size, /*[in, size_is(1, Filesize)]*/ ULONGLONG** Tagword, /*[out]*/ long *pStatus);
	STDMETHOD(Tag_Control)(/*[in]*/ long SinglesTag_OffOn, /*[in]*/ long TimeTag_OffOn, /*[out]*/ long *pStatus);
	STDMETHOD(Transmission_Trajectory)(/*[in]*/ long Transaxial_Speed, /*[in]*/ long Axial_Step, /*[out]*/ long *pStatus);


// protected data and functions
protected:

	// local data
	char m_ErrorArray[MAX_ERRORS][MAX_STR_LEN];
	char m_CP_Name[MAX_STR_LEN];
	long m_ErrorCode[MAX_ERRORS];
	long m_Settings[MAX_CONFIGS * MAX_HEADS * MAX_BLOCKS * MAX_SETTINGS];
	long m_Energy[MAX_CONFIGS];
	long m_NumLayers[MAX_HEADS];
	long m_HeadPresent[MAX_HEADS];
	long m_PointSource[MAX_HEADS];
	long m_TransmissionStart[MAX_HEADS];
	long m_LastBlock[MAX_HEADS];
	long m_LastMode[MAX_HEADS];
	long m_Trust[MAX_CONFIGS*MAX_HEADS];
	long m_NextError;
	long m_ScannerType;
	long m_ModelNumber;
	long m_Rotated;
	long m_Port;
	long m_NumConfigs;
	long m_NumSettings;
	long m_NumBlocks;
	long m_NumHeads;
	long m_XBlocks;
	long m_YBlocks;
	long m_XCrystals;
	long m_YCrystals;
	long m_SettingsInitialized;
	long m_Transmission;
	long m_ModulePairs;
	long m_SecuritySet;
	long m_NumDHITemps;
	long m_Module_Em;
	long m_Module_EmTx;
	long m_Module_Tx;
	long m_MaxTimingWindow;
	long m_DefaultTimingWindow;

	long m_Simulated_CP_Mode;
	long m_Simulated_Data_Flow;
	long m_Simulated_High_Voltage;
	long m_Simulated_Coincidence_Window;

	long m_SimulatedStart[MAX_HEADS+1];
	long m_SimulatedFinish[MAX_HEADS+1];
	long m_SimulatedSource[MAX_HEADS];
	long m_SimulatedMode[MAX_HEADS];
	long m_SimulatedLayer[MAX_HEADS];
	long m_SimulatedLLD[MAX_HEADS];
	long m_SimulatedULD[MAX_HEADS];
	long m_SimulatedConfig[MAX_HEADS];
	long m_SimulatedPassThru[MAX_HEADS];

	long m_Phase;

	// local functions
	unsigned long bintohex(unsigned char *data, char *hex, long count);
	long Init_Globals();
	void Close_Globals();
	long Init_Servers();
	long InitSerial();
	long SerialResponse(char *buffer, int timeout);
	long SerialCommand(char *buffer);
	long Internal_Pass(long Head, long Retry, char *CommandString, char *ResponseString);
	long CP_Error_Query(long ErrorStatus);
	long OS_Error_Query(long ErrorStatus);
	long hextobin(char *hex, unsigned char *data, long maxlength);
	long Add_Error(long System, char *Where, char *What, long Why);
	long Compare_Checksum(long Head);
	long Save_Settings(long Config, long Head);
	void Add_Information(long System, char *Where, char *What);
	void Add_Warning(long System, char *Where, char *What, long Why);
	void Release_Servers();

// variables included based on build
#ifndef ECAT_SCANNER

	// Error handler
	CErrorEventSupport* pErrEvtSup;

	// Coincidence Processor DCOM Servers
	Iosmain* m_pOSMain;
	Icpmain* m_pCPMain;
	Idhimain* m_pDHIMain;
#endif
};

#endif //__ABSTRACT_DHI_H_
