// ACQMain.h : Declaration of the CACQMain

#ifndef __ACQMAIN_H_
#define __ACQMAIN_H_

#include "resource.h"       // main symbols
#include <stdio.h>

// build based includes
#ifndef ECAT_SCANNER

#include "ErrorEventSupport.h"
#include "ArsEventMsgDefs.h"

#endif

/////////////////////////////////////////////////////////////////////////////
// CACQMain
class ATL_NO_VTABLE CACQMain : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CACQMain, &CLSID_ACQMain>,
	public IACQMain
{
public:
	CACQMain()
	{
	}

	~CACQMain()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ACQMAIN)
DECLARE_CLASSFACTORY_SINGLETON(CACQMain)					// only one instance running at a time

DECLARE_PROTECT_FINAL_CONSTRUCT()	HRESULT FinalConstruct();

BEGIN_COM_MAP(CACQMain)
	COM_INTERFACE_ENTRY(IACQMain)
END_COM_MAP()

// IACQMain
public:
	STDMETHOD(Statistics)(/*[out]*/ unsigned long *pNumKEvents, /*[out]*/ unsigned long *pBadAddress, /*[out]*/ unsigned long *pSyncProb);
	STDMETHOD(KillSelf)();
	STDMETHOD(Abort)();
	STDMETHOD(Retrieve_Singles)(/*[in]*/ long Head, /*[out]*/ long *pDataSize, /*[out, size_is(1, *pDataSize)]*/ long** buffer, /*[out]*/ long *pStatus);
	STDMETHOD(Progress)(/*[out]*/ long *pPercent, /*[out]*/ long *pStatus);
	STDMETHOD(Acquire_Singles)(/*[in]*/ long Mode, /*[in]*/ long AcquireSelect[16], /*[in]*/ long Type, /*[in]*/ long Amount, /*[out]*/ long *pStatus);
protected:
	long Init_Globals();

};

#endif //__ACQMAIN_H_
