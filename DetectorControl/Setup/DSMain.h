// DSMain.h : Declaration of the CDSMain

#ifndef __DSMAIN_H_
#define __DSMAIN_H_

#include "resource.h"       // main symbols
#include "Setup_Constants.h"

/////////////////////////////////////////////////////////////////////////////
// CDSMain
class ATL_NO_VTABLE CDSMain : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDSMain, &CLSID_DSMain>,
	public IDSMain
{
public:

	// Constructor
	CDSMain()
	{
	}

	// Destructor
	~CDSMain()
	{
		if (m_Initialized == 0) Release_Servers();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_DSMAIN)
DECLARE_CLASSFACTORY_SINGLETON(CDSMain)					// only one instance running at a time

DECLARE_PROTECT_FINAL_CONSTRUCT()	HRESULT FinalConstruct();

BEGIN_COM_MAP(CDSMain)
	COM_INTERFACE_ENTRY(IDSMain)
END_COM_MAP()

// IDSMain
public:
	STDMETHOD(KillSelf)();
	STDMETHOD(Time_Alignment)(/*[in]*/ long Type, /*[in]*/ long ModulePair, /*[out]*/ long *pStatus);
	STDMETHOD(Setup_Finished)(/*[out]*/ long BlockStatus[MAX_TOTAL_BLOCKS]);
	STDMETHOD(Setup_Progress)(/*[out]*/ unsigned char Stage[MAX_STR_LEN], /*[out]*/ long *pStagePercent, /*[out]*/ long *pTotalPercent);
	STDMETHOD(Error_Lookup)(/*[in]*/ long Code, /*[out]*/ unsigned char ErrorStr[MAX_STR_LEN]);
	STDMETHOD(Setup)(/*[in]*/ long Type, /*[in]*/ long Configuration, /*[in]*/ long BlockSelect[MAX_TOTAL_BLOCKS], /*[out]*/ long *pStatus);
	STDMETHOD(Abort)();
protected:
	void Release_Servers();

	// global variables
	long m_Initialized;

	// internal subroutines
	long Init_Globals();

};

#endif //__DSMAIN_H_
