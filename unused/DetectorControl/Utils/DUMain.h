// DUMain.h : Declaration of the CDUMain

#ifndef __DUMAIN_H_
#define __DUMAIN_H_

#include "resource.h"       // main symbols
#include "Setup_Constants.h"

/////////////////////////////////////////////////////////////////////////////
// CDUMain
class ATL_NO_VTABLE CDUMain : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDUMain, &CLSID_DUMain>,
	public IDUMain
{
public:
	CDUMain()
	{
	}

	// Destructor
	~CDUMain()
	{
		Release_Servers();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DUMAIN)
DECLARE_CLASSFACTORY_SINGLETON(CDUMain)					// only one instance running at a time

DECLARE_PROTECT_FINAL_CONSTRUCT()	HRESULT FinalConstruct();

BEGIN_COM_MAP(CDUMain)
	COM_INTERFACE_ENTRY(IDUMain)
END_COM_MAP()

// IDUMain
public:
	STDMETHOD(Acquisition_Statistics)(/*[out]*/ unsigned long *pNumKEvents, /*[out]*/ unsigned long *pBadAddress, /*[out]*/ unsigned long *pSyncProb);
	STDMETHOD(Load_Head)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Size, /*[in, size_is(1, Size)]*/ unsigned char** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(Save_Head)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[out]*/ long *pSize, /*[out, size_is(1, *pSize)]*/ unsigned char** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(KillSelf)();
	STDMETHOD(Acquisition_Abort)();
	STDMETHOD(Archive_Abort)();
	STDMETHOD(Archive_Progress)(/*[out]*/ long *pPercent, /*[out]*/ unsigned char StageStr [MAX_STR_LEN], /*[out]*/ long HeadStatus[MAX_HEADS]);
	STDMETHOD(Get_Master_Crystal_Location)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long CrystalPosition[278784], /*[out]*/ long *pStatus);
	STDMETHOD(Write_Master)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long Size, /*[in, size_is(1, Size)]*/ long** Data, /*[out]*/ long *pStatus);
	STDMETHOD(Read_Master)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long *pSize, /*[out, size_is(1, *pSize)]*/ long** Data, /*[out]*/ long *pStatus);
	STDMETHOD(Store_Settings)(/*[in]*/ long Send, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Settings[MAX_BLOCKS*MAX_SETTINGS], /*[out]*/ long *pStatus);
	STDMETHOD(Store_Energy_Peaks)(/*[in]*/ long Send, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long CrystalPeaks[MAX_HEAD_CRYSTALS], /*[out]*/ long *pStatus);
	STDMETHOD(Store_Crystal_Time)(/*[in]*/ long Send, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long CrystalTime[MAX_HEAD_CRYSTALS], /*[out]*/ long *pStatus);
	STDMETHOD(Store_Crystal_Position)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long CrystalPosition[MAX_HEAD_VERTICES*2], /*[out]*/ long *pStatus);
	STDMETHOD(Store_CRM)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long Block, /*[in]*/ unsigned char CRM[256*256], /*[out]*/ long *pStatus);
	STDMETHOD(Save_Setup_Archive)(/*[in]*/ long Configuration, /*[in]*/ long HeadSelect[MAX_HEADS], /*[out]*/ long HeadStatus[MAX_HEADS]);
	STDMETHOD(Remove_Setup_Archive)(/*[in]*/ unsigned char Archive[MAX_STR_LEN], /*[out]*/ long *pStatus);
	STDMETHOD(Load_Setup_Archive)(/*[in]*/ unsigned char Archive[MAX_STR_LEN], /*[in]*/ long HeadSelect[MAX_HEADS], /*[out]*/ long HeadStatus[MAX_HEADS]);
	STDMETHOD(List_Setup_Archives)(/*[in]*/ long Configuration, /*[in]*/ long HeadSelect[MAX_HEADS], /*[out]*/ long *pNumFound, /*[out, size_is(1, *pNumFound*2048)]*/ unsigned char** List, /*[out]*/ long *pStatus);
	STDMETHOD(Get_Stored_Settings)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[out]*/ long Settings[MAX_BLOCKS*MAX_SETTINGS], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Stored_Energy_Peaks)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long CrystalPeaks[MAX_HEAD_CRYSTALS], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Stored_Crystal_Time)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long CrystalTime[MAX_HEAD_CRYSTALS], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Stored_Crystal_Position)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long CrystalPosition[MAX_HEAD_VERTICES*2], /*[out]*/ long *pStatus);
	STDMETHOD(Get_Stored_CRM)(/*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long Block, /*[out]*/ unsigned char CRM[256*256], /*[out]*/ long *pStatus);
	STDMETHOD(Download_CRM)(/*[in]*/ long Configuration, /*[in]*/ long BlockSelect[MAX_TOTAL_BLOCKS], /*[out]*/ long BlockStatus[MAX_TOTAL_BLOCKS]);
	STDMETHOD(Current_Block_Data)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[in]*/ long Block, /*[out]*/ long *pSize, /*[out, size_is(1, *pSize)]*/ long** Data, /*[out]*/ long *pStatus);
	STDMETHOD(Current_Head_Data)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long Head, /*[in]*/ long Layer, /*[out]*/ long *pTime, /*[out]*/ long *pSize, /*[out, size_is(1, *pSize)]*/ long** Data, /*[out]*/ long *pStatus);
	STDMETHOD(Acquisition_Progress)(/*[out]*/ long *pPercent, /*[out]*/ long HeadStatus[MAX_HEADS]);
	STDMETHOD(Acquire_Coinc_Data)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long HeadSelect[MAX_HEADS], /*[in]*/ long LayerSelect[MAX_HEADS], /*[in]*/ long ModulePair, /*[in]*/ long Transmission, /*[in]*/ long AcquisitionType, /*[in]*/ long Amount, /*[in]*/ long lld, /*[in]*/ long uld, /*[out]*/ long *pStatus);
	STDMETHOD(Acquire_Singles_Data)(/*[in]*/ long DataMode, /*[in]*/ long Configuration, /*[in]*/ long HeadSelect[MAX_HEADS], /*[in]*/ long SourceSelect[MAX_HEADS], /*[in]*/ long LayerSelect[MAX_HEADS], /*[in]*/ long AcquisitionType, /*[in]*/ long Amount, /*[in]*/ long lld, /*[in]*/ long uld, /*[out]*/ long *pStatus);
protected:

	// internal subroutines
	long Acquire_Coinc_Driver(long DataMode, long Configuration, long HeadSelect[MAX_HEADS], long LayerSelect[MAX_HEADS], long ModulePair, long Transmission, long AcquisitionType, long Amount, long lld, long uld);
	long Acquire_Singles_Driver(long DataMode, long Configuration, long HeadSelect[MAX_HEADS], long SourceSelect[MAX_HEADS], long LayerSelect[MAX_HEADS], long AcquisitionType, long Amount, long lld, long uld);
	long Init_Globals();
	void Release_Servers();
	};

#endif //__DUMAIN_H_
