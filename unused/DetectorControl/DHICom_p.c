/* this ALWAYS GENERATED file contains the proxy stub code */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:26:34 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\DHICom.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 440
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif // __RPCPROXY_H_VERSION__


#include "DHICom.h"

#define TYPE_FORMAT_STRING_SIZE   421                               
#define PROC_FORMAT_STRING_SIZE   2497                              

typedef struct _MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } MIDL_TYPE_FORMAT_STRING;

typedef struct _MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } MIDL_PROC_FORMAT_STRING;


extern const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString;
extern const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString;


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IAbstract_DHI, ver. 0.0,
   GUID={0x82C49018,0x554A,0x4AD8,{0x92,0xE9,0x25,0xE8,0x84,0x9C,0xAD,0x33}} */


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAbstract_DHI_ServerInfo;

#pragma code_seg(".orpc")
/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Check_Head_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pDataMode,
    /* [out] */ long __RPC_FAR *pConfiguration,
    /* [out] */ long __RPC_FAR *pLayer,
    /* [out] */ long __RPC_FAR *pLLD,
    /* [out] */ long __RPC_FAR *pULD,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1220],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1220],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&pDataMode,
                  ( unsigned char __RPC_FAR * )&pConfiguration,
                  ( unsigned char __RPC_FAR * )&pLayer,
                  ( unsigned char __RPC_FAR * )&pLLD,
                  ( unsigned char __RPC_FAR * )&pULD,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1220],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Check_CP_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pMode,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1284],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1284],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pMode,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1284],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Temperature_Limits_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ float Low,
    /* [in] */ float High,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1318],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1318],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Low,
                  ( unsigned char __RPC_FAR * )&High,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1318],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Reset_CP_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Reset,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1358],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1358],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Reset,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1358],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Reset_Head_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1392],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1392],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1392],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Load_RAM_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long RAM_Type,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1426],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1426],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Configuration,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&RAM_Type,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1426],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Time_Window_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long WindowSize,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1472],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1472],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&WindowSize,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1472],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_OS_Command_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1506],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1506],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&CommandString,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1506],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Pass_Command_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
    /* [out] */ unsigned char __RPC_FAR ResponseStr[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1546],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1546],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&CommandString,
                  ( unsigned char __RPC_FAR * )&ResponseStr,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1546],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Tag_Insert_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Size,
    /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *Tagword,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1592],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1592],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Size,
                  ( unsigned char __RPC_FAR * )&Tagword,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1592],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Tag_Control_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long SinglesTag_OffOn,
    /* [in] */ long TimeTag_OffOn,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1632],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1632],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&SinglesTag_OffOn,
                  ( unsigned char __RPC_FAR * )&TimeTag_OffOn,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1632],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Transmission_Trajectory_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Transaxial_Speed,
    /* [in] */ long Axial_Step,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1672],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1672],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Transaxial_Speed,
                  ( unsigned char __RPC_FAR * )&Axial_Step,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1672],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Diagnostics_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pResult,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1712],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1712],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pResult,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1712],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Force_Reload_Proxy( 
    IAbstract_DHI __RPC_FAR * This)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,This);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1746],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1746],
                  ( unsigned char __RPC_FAR * )&This);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1746],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Health_Check_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumFailed,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Failed,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1768],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1768],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pNumFailed,
                  ( unsigned char __RPC_FAR * )&Failed,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1768],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_DataStream_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1808],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1808],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&OnOff,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1808],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Initialize_PET_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long ScanType,
    /* [in] */ long LLD,
    /* [in] */ long ULD,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1842],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1842],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&ScanType,
                  ( unsigned char __RPC_FAR * )&LLD,
                  ( unsigned char __RPC_FAR * )&ULD,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1842],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Head_Status_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumHeads,
    /* [size_is][size_is][out] */ HeadInfo __RPC_FAR *__RPC_FAR *Info,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1888],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1888],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pNumHeads,
                  ( unsigned char __RPC_FAR * )&Info,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1888],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_CountRate_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pCorrectedSingles,
    /* [out] */ long __RPC_FAR *pUncorrectedSingles,
    /* [out] */ long __RPC_FAR *pPrompts,
    /* [out] */ long __RPC_FAR *pRandoms,
    /* [out] */ long __RPC_FAR *pScatters,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1928],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1928],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pCorrectedSingles,
                  ( unsigned char __RPC_FAR * )&pUncorrectedSingles,
                  ( unsigned char __RPC_FAR * )&pPrompts,
                  ( unsigned char __RPC_FAR * )&pRandoms,
                  ( unsigned char __RPC_FAR * )&pScatters,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1928],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Ring_Singles_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumRings,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *RingSingles,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1986],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1986],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pNumRings,
                  ( unsigned char __RPC_FAR * )&RingSingles,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[1986],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_KillSelf_Proxy( 
    IAbstract_DHI __RPC_FAR * This)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,This);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2026],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2026],
                  ( unsigned char __RPC_FAR * )&This);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2026],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_SetLog_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2048],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2048],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&OnOff,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2048],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Report_Temperature_Voltage_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pCP_Temperature,
    /* [out] */ float __RPC_FAR *pMinimum_Temperature,
    /* [out] */ float __RPC_FAR *pMaximum_Temperature,
    /* [out] */ long __RPC_FAR *pNumber_DHI_Temperatures,
    /* [out] */ long __RPC_FAR *pNumHeads,
    /* [size_is][size_is][out] */ PhysicalInfo __RPC_FAR *__RPC_FAR *Info,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2082],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2082],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&pCP_Temperature,
                  ( unsigned char __RPC_FAR * )&pMinimum_Temperature,
                  ( unsigned char __RPC_FAR * )&pMaximum_Temperature,
                  ( unsigned char __RPC_FAR * )&pNumber_DHI_Temperatures,
                  ( unsigned char __RPC_FAR * )&pNumHeads,
                  ( unsigned char __RPC_FAR * )&Info,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2082],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Crystal_Position_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Peaks[ 512 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2146],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2146],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Peaks,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2146],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Crystal_Peaks_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Peaks[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2192],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2192],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Peaks,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2192],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Time_Correction_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Correction[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2238],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2238],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Correction,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2238],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Time_Correction_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Correction[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2284],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2284],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Correction,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2284],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Crystal_Peaks_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Peaks[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2330],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2330],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Peaks,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2330],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Crystal_Position_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Peaks[ 512 ],
    /* [in] */ long StdDev,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2376],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2376],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&Peaks,
                  ( unsigned char __RPC_FAR * )&StdDev,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2376],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Build_CRM_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pStatus)
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,pStatus);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2428],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2428],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&Head,
                  ( unsigned char __RPC_FAR * )&Block,
                  ( unsigned char __RPC_FAR * )&pStatus);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2428],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}

/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Last_Async_Message_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ unsigned char __RPC_FAR LastMsg[ 2048 ])
{
CLIENT_CALL_RETURN _RetVal;


#if defined( _ALPHA_ )
    va_list vlist;
#endif
    
#if defined( _ALPHA_ )
    va_start(vlist,LastMsg);
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2468],
                  vlist.a0);
#elif defined( _PPC_ ) || defined( _MIPS_ )

    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2468],
                  ( unsigned char __RPC_FAR * )&This,
                  ( unsigned char __RPC_FAR * )&LastMsg);
#else
    _RetVal = NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&Object_StubDesc,
                  (PFORMAT_STRING) &__MIDL_ProcFormatString.Format[2468],
                  ( unsigned char __RPC_FAR * )&This);
#endif
    return ( HRESULT  )_RetVal.Simple;
    
}


static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    __MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x20000, /* Ndr library version */
    0,
    0x50100a4, /* MIDL Version 5.1.164 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    1,  /* Flags */
    0,  /* Reserved3 */
    0,  /* Reserved4 */
    0   /* Reserved5 */
    };

static const unsigned short IAbstract_DHI_FormatStringOffsetTable[] = 
    {
    0,
    40,
    92,
    138,
    184,
    224,
    264,
    304,
    332,
    360,
    430,
    494,
    522,
    550,
    590,
    636,
    682,
    740,
    798,
    832,
    872,
    912,
    946,
    992,
    1032,
    1072,
    1106,
    1140,
    1174,
    1220,
    1284,
    1318,
    1358,
    1392,
    1426,
    1472,
    1506,
    1546,
    1592,
    1632,
    1672,
    1712,
    1746,
    1768,
    1808,
    1842,
    1888,
    1928,
    1986,
    2026,
    2048,
    2082,
    2146,
    2192,
    2238,
    2284,
    2330,
    2376,
    2428,
    2468
    };

static const MIDL_SERVER_INFO IAbstract_DHI_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    __MIDL_ProcFormatString.Format,
    &IAbstract_DHI_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IAbstract_DHI_ProxyInfo =
    {
    &Object_StubDesc,
    __MIDL_ProcFormatString.Format,
    &IAbstract_DHI_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };

CINTERFACE_PROXY_VTABLE(63) _IAbstract_DHIProxyVtbl = 
{
    &IAbstract_DHI_ProxyInfo,
    &IID_IAbstract_DHI,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *)-1 /* IAbstract_DHI::Error_Lookup */ ,
    (void *)-1 /* IAbstract_DHI::Zap */ ,
    (void *)-1 /* IAbstract_DHI::Determine_Delay */ ,
    (void *)-1 /* IAbstract_DHI::Determine_Offsets */ ,
    (void *)-1 /* IAbstract_DHI::Coincidence_Mode */ ,
    (void *)-1 /* IAbstract_DHI::Time_Mode */ ,
    (void *)-1 /* IAbstract_DHI::PassThru_Mode */ ,
    (void *)-1 /* IAbstract_DHI::TagWord_Mode */ ,
    (void *)-1 /* IAbstract_DHI::Test_Mode */ ,
    (void *)-1 /* IAbstract_DHI::Set_Head_Mode */ ,
    (void *)-1 /* IAbstract_DHI::Initialize_Scan */ ,
    (void *)-1 /* IAbstract_DHI::Start_Scan */ ,
    (void *)-1 /* IAbstract_DHI::Stop_Scan */ ,
    (void *)-1 /* IAbstract_DHI::Refresh_Analog_Settings */ ,
    (void *)-1 /* IAbstract_DHI::Get_Analog_Settings */ ,
    (void *)-1 /* IAbstract_DHI::Set_Analog_Settings */ ,
    (void *)-1 /* IAbstract_DHI::File_Upload */ ,
    (void *)-1 /* IAbstract_DHI::File_Download */ ,
    (void *)-1 /* IAbstract_DHI::High_Voltage */ ,
    (void *)-1 /* IAbstract_DHI::Point_Source */ ,
    (void *)-1 /* IAbstract_DHI::Point_Source_Status */ ,
    (void *)-1 /* IAbstract_DHI::Get_Statistics */ ,
    (void *)-1 /* IAbstract_DHI::Singles */ ,
    (void *)-1 /* IAbstract_DHI::Progress */ ,
    (void *)-1 /* IAbstract_DHI::Ping */ ,
    (void *)-1 /* IAbstract_DHI::Report_Temperature */ ,
    (void *)-1 /* IAbstract_DHI::Report_Voltage */ ,
    (void *)-1 /* IAbstract_DHI::Report_HRRT_High_Voltage */ ,
    (void *)-1 /* IAbstract_DHI::Hardware_Configuration */ ,
    IAbstract_DHI_Check_Head_Mode_Proxy ,
    IAbstract_DHI_Check_CP_Mode_Proxy ,
    IAbstract_DHI_Set_Temperature_Limits_Proxy ,
    IAbstract_DHI_Reset_CP_Proxy ,
    IAbstract_DHI_Reset_Head_Proxy ,
    IAbstract_DHI_Load_RAM_Proxy ,
    IAbstract_DHI_Time_Window_Proxy ,
    IAbstract_DHI_OS_Command_Proxy ,
    IAbstract_DHI_Pass_Command_Proxy ,
    IAbstract_DHI_Tag_Insert_Proxy ,
    IAbstract_DHI_Tag_Control_Proxy ,
    IAbstract_DHI_Transmission_Trajectory_Proxy ,
    IAbstract_DHI_Diagnostics_Proxy ,
    IAbstract_DHI_Force_Reload_Proxy ,
    IAbstract_DHI_Health_Check_Proxy ,
    IAbstract_DHI_DataStream_Proxy ,
    IAbstract_DHI_Initialize_PET_Proxy ,
    IAbstract_DHI_Head_Status_Proxy ,
    IAbstract_DHI_CountRate_Proxy ,
    IAbstract_DHI_Ring_Singles_Proxy ,
    IAbstract_DHI_KillSelf_Proxy ,
    IAbstract_DHI_SetLog_Proxy ,
    IAbstract_DHI_Report_Temperature_Voltage_Proxy ,
    IAbstract_DHI_Get_Block_Crystal_Position_Proxy ,
    IAbstract_DHI_Get_Block_Crystal_Peaks_Proxy ,
    IAbstract_DHI_Get_Block_Time_Correction_Proxy ,
    IAbstract_DHI_Set_Block_Time_Correction_Proxy ,
    IAbstract_DHI_Set_Block_Crystal_Peaks_Proxy ,
    IAbstract_DHI_Set_Block_Crystal_Position_Proxy ,
    IAbstract_DHI_Build_CRM_Proxy ,
    IAbstract_DHI_Last_Async_Message_Proxy
};

const CInterfaceStubVtbl _IAbstract_DHIStubVtbl =
{
    &IID_IAbstract_DHI,
    &IAbstract_DHI_ServerInfo,
    63,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

#pragma data_seg(".rdata")

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT40_OR_LATER)
#error You need a Windows NT 4.0 or later to run this stub because it uses these features:
#error   -Oif or -Oicf, float, double or hyper in -Oif or -Oicf, more than 32 methods in the interface.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will die there with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const MIDL_PROC_FORMAT_STRING __MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure Error_Lookup */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
#ifndef _ALPHA_
/*  8 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x818 ),	/* 2072 */
/* 14 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter ErrorCode */

/* 16 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 18 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 20 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Code */

/* 22 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 24 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 26 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ErrorString */

/* 28 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 30 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 32 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 34 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 36 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 38 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Zap */

/* 40 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 42 */	NdrFcLong( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x4 ),	/* 4 */
#ifndef _ALPHA_
/* 48 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 50 */	NdrFcShort( 0x20 ),	/* 32 */
/* 52 */	NdrFcShort( 0x10 ),	/* 16 */
/* 54 */	0x4,		/* Oi2 Flags:  has return, */
			0x6,		/* 6 */

	/* Parameter ZapMode */

/* 56 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 58 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 60 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Configuration */

/* 62 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 64 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 66 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 68 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 70 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 72 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 74 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 76 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 78 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 80 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 82 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 84 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 86 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 88 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 90 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Determine_Delay */

/* 92 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 94 */	NdrFcLong( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x5 ),	/* 5 */
#ifndef _ALPHA_
/* 100 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 102 */	NdrFcShort( 0x18 ),	/* 24 */
/* 104 */	NdrFcShort( 0x10 ),	/* 16 */
/* 106 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Configuration */

/* 108 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 110 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 114 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 116 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 120 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 122 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 126 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 128 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 134 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Determine_Offsets */

/* 138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x6 ),	/* 6 */
#ifndef _ALPHA_
/* 146 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 148 */	NdrFcShort( 0x18 ),	/* 24 */
/* 150 */	NdrFcShort( 0x10 ),	/* 16 */
/* 152 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Configuration */

/* 154 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 156 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 158 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 160 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 162 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 166 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 168 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 170 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 172 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 174 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 176 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 178 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 180 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Coincidence_Mode */

/* 184 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 186 */	NdrFcLong( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x7 ),	/* 7 */
#ifndef _ALPHA_
/* 192 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 194 */	NdrFcShort( 0x10 ),	/* 16 */
/* 196 */	NdrFcShort( 0x10 ),	/* 16 */
/* 198 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Transmission */

/* 200 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 202 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ModulePairs */

/* 206 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 208 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 212 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 214 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 216 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 218 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 220 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Time_Mode */

/* 224 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 226 */	NdrFcLong( 0x0 ),	/* 0 */
/* 230 */	NdrFcShort( 0x8 ),	/* 8 */
#ifndef _ALPHA_
/* 232 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 234 */	NdrFcShort( 0x10 ),	/* 16 */
/* 236 */	NdrFcShort( 0x10 ),	/* 16 */
/* 238 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Transmission */

/* 240 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 242 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 244 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ModulePairs */

/* 246 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 248 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 252 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 254 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 258 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 260 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 262 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure PassThru_Mode */

/* 264 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 266 */	NdrFcLong( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x9 ),	/* 9 */
#ifndef _ALPHA_
/* 272 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 274 */	NdrFcShort( 0x90 ),	/* 144 */
/* 276 */	NdrFcShort( 0x10 ),	/* 16 */
/* 278 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 280 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 282 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 284 */	NdrFcShort( 0xc ),	/* Type Offset=12 */

	/* Parameter PtSrc */

/* 286 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 288 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 290 */	NdrFcShort( 0xc ),	/* Type Offset=12 */

	/* Parameter pStatus */

/* 292 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 294 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 298 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 300 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure TagWord_Mode */

/* 304 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 310 */	NdrFcShort( 0xa ),	/* 10 */
#ifndef _ALPHA_
/* 312 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0x10 ),	/* 16 */
/* 318 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pStatus */

/* 320 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 322 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 324 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 326 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 328 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 330 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Test_Mode */

/* 332 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 334 */	NdrFcLong( 0x0 ),	/* 0 */
/* 338 */	NdrFcShort( 0xb ),	/* 11 */
#ifndef _ALPHA_
/* 340 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 342 */	NdrFcShort( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x10 ),	/* 16 */
/* 346 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pStatus */

/* 348 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 350 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 354 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 356 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 358 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Head_Mode */

/* 360 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 362 */	NdrFcLong( 0x0 ),	/* 0 */
/* 366 */	NdrFcShort( 0xc ),	/* 12 */
#ifndef _ALPHA_
/* 368 */	NdrFcShort( 0x28 ),	/* x86, MIPS, PPC Stack size/offset = 40 */
#else
			NdrFcShort( 0x50 ),	/* Alpha Stack size/offset = 80 */
#endif
/* 370 */	NdrFcShort( 0x38 ),	/* 56 */
/* 372 */	NdrFcShort( 0x10 ),	/* 16 */
/* 374 */	0x4,		/* Oi2 Flags:  has return, */
			0x9,		/* 9 */

	/* Parameter DataMode */

/* 376 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 378 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 380 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Configuration */

/* 382 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 384 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 386 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 388 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 390 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 392 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Layer */

/* 394 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 396 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 398 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 400 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 402 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 404 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter LLD */

/* 406 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 408 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 410 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ULD */

/* 412 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 414 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 418 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 420 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 424 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 426 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Initialize_Scan */

/* 430 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 432 */	NdrFcLong( 0x0 ),	/* 0 */
/* 436 */	NdrFcShort( 0xd ),	/* 13 */
#ifndef _ALPHA_
/* 438 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 440 */	NdrFcShort( 0x30 ),	/* 48 */
/* 442 */	NdrFcShort( 0x10 ),	/* 16 */
/* 444 */	0x4,		/* Oi2 Flags:  has return, */
			0x8,		/* 8 */

	/* Parameter Transmission */

/* 446 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 448 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 450 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ModulePairs */

/* 452 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 454 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Configuration */

/* 458 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 460 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 462 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Layer */

/* 464 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 466 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 468 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter LLD */

/* 470 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 472 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 474 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ULD */

/* 476 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 478 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 480 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 482 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 484 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 486 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 488 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 490 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Start_Scan */

/* 494 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 496 */	NdrFcLong( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0xe ),	/* 14 */
#ifndef _ALPHA_
/* 502 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x10 ),	/* 16 */
/* 508 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pStatus */

/* 510 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 512 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 514 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 516 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 518 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 520 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Stop_Scan */

/* 522 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 524 */	NdrFcLong( 0x0 ),	/* 0 */
/* 528 */	NdrFcShort( 0xf ),	/* 15 */
#ifndef _ALPHA_
/* 530 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 534 */	NdrFcShort( 0x10 ),	/* 16 */
/* 536 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter pStatus */

/* 538 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 540 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 542 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 544 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 546 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 548 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Refresh_Analog_Settings */

/* 550 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 552 */	NdrFcLong( 0x0 ),	/* 0 */
/* 556 */	NdrFcShort( 0x10 ),	/* 16 */
#ifndef _ALPHA_
/* 558 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 560 */	NdrFcShort( 0x10 ),	/* 16 */
/* 562 */	NdrFcShort( 0x10 ),	/* 16 */
/* 564 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Configuration */

/* 566 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 568 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 570 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 572 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 574 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 576 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 578 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 580 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 582 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 584 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 586 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 588 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Get_Analog_Settings */

/* 590 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 592 */	NdrFcLong( 0x0 ),	/* 0 */
/* 596 */	NdrFcShort( 0x11 ),	/* 17 */
#ifndef _ALPHA_
/* 598 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 600 */	NdrFcShort( 0x10 ),	/* 16 */
/* 602 */	NdrFcShort( 0x4018 ),	/* 16408 */
/* 604 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Configuration */

/* 606 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 608 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 610 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 612 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 614 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Settings */

/* 618 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 620 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 622 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter pStatus */

/* 624 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 626 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 628 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 630 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 632 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 634 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Analog_Settings */

/* 636 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 638 */	NdrFcLong( 0x0 ),	/* 0 */
/* 642 */	NdrFcShort( 0x12 ),	/* 18 */
#ifndef _ALPHA_
/* 644 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 646 */	NdrFcShort( 0x4018 ),	/* 16408 */
/* 648 */	NdrFcShort( 0x10 ),	/* 16 */
/* 650 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Configuration */

/* 652 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 654 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 656 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 658 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 660 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 662 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Settings */

/* 664 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 666 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 668 */	NdrFcShort( 0x12 ),	/* Type Offset=18 */

	/* Parameter pStatus */

/* 670 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 672 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 674 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 676 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 678 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 680 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure File_Upload */

/* 682 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 684 */	NdrFcLong( 0x0 ),	/* 0 */
/* 688 */	NdrFcShort( 0x13 ),	/* 19 */
#ifndef _ALPHA_
/* 690 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 692 */	NdrFcShort( 0x810 ),	/* 2064 */
/* 694 */	NdrFcShort( 0x20 ),	/* 32 */
/* 696 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x7,		/* 7 */

	/* Parameter Head */

/* 698 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 700 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 702 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Filename */

/* 704 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 706 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 708 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter Filesize */

/* 710 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 712 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter CheckSum */

/* 716 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 718 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 722 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 724 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 726 */	NdrFcShort( 0x26 ),	/* Type Offset=38 */

	/* Parameter pStatus */

/* 728 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 730 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 732 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 734 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 736 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 738 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure File_Download */

/* 740 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 742 */	NdrFcLong( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x14 ),	/* 20 */
#ifndef _ALPHA_
/* 748 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 750 */	NdrFcShort( 0x820 ),	/* 2080 */
/* 752 */	NdrFcShort( 0x10 ),	/* 16 */
/* 754 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x7,		/* 7 */

	/* Parameter Head */

/* 756 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 758 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 760 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Filesize */

/* 762 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 764 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 766 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter CheckSum */

/* 768 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 770 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 772 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Filename */

/* 774 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 776 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 778 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter buffer */

/* 780 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 782 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 784 */	NdrFcShort( 0x52 ),	/* Type Offset=82 */

	/* Parameter pStatus */

/* 786 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 788 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 790 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 792 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 794 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure High_Voltage */

/* 798 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 800 */	NdrFcLong( 0x0 ),	/* 0 */
/* 804 */	NdrFcShort( 0x15 ),	/* 21 */
#ifndef _ALPHA_
/* 806 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 808 */	NdrFcShort( 0x8 ),	/* 8 */
/* 810 */	NdrFcShort( 0x10 ),	/* 16 */
/* 812 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter OnOff */

/* 814 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 816 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 818 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 820 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 822 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 824 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 826 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 828 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 830 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Point_Source */

/* 832 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 834 */	NdrFcLong( 0x0 ),	/* 0 */
/* 838 */	NdrFcShort( 0x16 ),	/* 22 */
#ifndef _ALPHA_
/* 840 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 842 */	NdrFcShort( 0x10 ),	/* 16 */
/* 844 */	NdrFcShort( 0x10 ),	/* 16 */
/* 846 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 848 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 850 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 852 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter OnOff */

/* 854 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 856 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 858 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 860 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 862 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 864 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 866 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 868 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Point_Source_Status */

/* 872 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 874 */	NdrFcLong( 0x0 ),	/* 0 */
/* 878 */	NdrFcShort( 0x17 ),	/* 23 */
#ifndef _ALPHA_
/* 880 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 882 */	NdrFcShort( 0x8 ),	/* 8 */
/* 884 */	NdrFcShort( 0x18 ),	/* 24 */
/* 886 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 888 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 890 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pOnOff */

/* 894 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 896 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 898 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 900 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 902 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 904 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 906 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 908 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 910 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Get_Statistics */

/* 912 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 914 */	NdrFcLong( 0x0 ),	/* 0 */
/* 918 */	NdrFcShort( 0x18 ),	/* 24 */
#ifndef _ALPHA_
/* 920 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 922 */	NdrFcShort( 0x0 ),	/* 0 */
/* 924 */	NdrFcShort( 0x64 ),	/* 100 */
/* 926 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Statistics */

/* 928 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 930 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 932 */	NdrFcShort( 0x70 ),	/* Type Offset=112 */

	/* Parameter pStatus */

/* 934 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 936 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 938 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 940 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 942 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 944 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Singles */

/* 946 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 948 */	NdrFcLong( 0x0 ),	/* 0 */
/* 952 */	NdrFcShort( 0x19 ),	/* 25 */
#ifndef _ALPHA_
/* 954 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 956 */	NdrFcShort( 0x10 ),	/* 16 */
/* 958 */	NdrFcShort( 0x218 ),	/* 536 */
/* 960 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 962 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 964 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter SinglesType */

/* 968 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 970 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 972 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Singles */

/* 974 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 976 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 978 */	NdrFcShort( 0x76 ),	/* Type Offset=118 */

	/* Parameter pStatus */

/* 980 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 982 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 984 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 986 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 988 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 990 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Progress */

/* 992 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 994 */	NdrFcLong( 0x0 ),	/* 0 */
/* 998 */	NdrFcShort( 0x1a ),	/* 26 */
#ifndef _ALPHA_
/* 1000 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1002 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1004 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1006 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 1008 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1010 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1012 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pPercent */

/* 1014 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1016 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1018 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1020 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1022 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1026 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1028 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1030 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Ping */

/* 1032 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1034 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0x1b ),	/* 27 */
#ifndef _ALPHA_
/* 1040 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1042 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1044 */	NdrFcShort( 0x818 ),	/* 2072 */
/* 1046 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 1048 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1050 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1052 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter BuildID */

/* 1054 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 1056 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1058 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter pStatus */

/* 1060 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1062 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1064 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1066 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1068 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1070 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Report_Temperature */

/* 1072 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1074 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1078 */	NdrFcShort( 0x1c ),	/* 28 */
#ifndef _ALPHA_
/* 1080 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1082 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1084 */	NdrFcShort( 0xa4 ),	/* 164 */
/* 1086 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Temperature */

/* 1088 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 1090 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1092 */	NdrFcShort( 0x7c ),	/* Type Offset=124 */

	/* Parameter pStatus */

/* 1094 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1096 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1098 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1100 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1102 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Report_Voltage */

/* 1106 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1108 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1112 */	NdrFcShort( 0x1d ),	/* 29 */
#ifndef _ALPHA_
/* 1114 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1116 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1118 */	NdrFcShort( 0x118 ),	/* 280 */
/* 1120 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Voltage */

/* 1122 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 1124 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1126 */	NdrFcShort( 0x82 ),	/* Type Offset=130 */

	/* Parameter pStatus */

/* 1128 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1130 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1132 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1134 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1136 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Report_HRRT_High_Voltage */

/* 1140 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1142 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1146 */	NdrFcShort( 0x1e ),	/* 30 */
#ifndef _ALPHA_
/* 1148 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x158 ),	/* 344 */
/* 1154 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Voltage */

/* 1156 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 1158 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1160 */	NdrFcShort( 0x88 ),	/* Type Offset=136 */

	/* Parameter pStatus */

/* 1162 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1164 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1170 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Hardware_Configuration */

/* 1174 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1180 */	NdrFcShort( 0x1f ),	/* 31 */
#ifndef _ALPHA_
/* 1182 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1186 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1188 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x5,		/* 5 */

	/* Parameter Filesize */

/* 1190 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1192 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter CheckSum */

/* 1196 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1198 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1200 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter buffer */

/* 1202 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 1204 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1206 */	NdrFcShort( 0x9c ),	/* Type Offset=156 */

	/* Parameter pStatus */

/* 1208 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1210 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1214 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1216 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Check_Head_Mode */

/* 1220 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1222 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1226 */	NdrFcShort( 0x20 ),	/* 32 */
#ifndef _ALPHA_
/* 1228 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 1230 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1232 */	NdrFcShort( 0x38 ),	/* 56 */
/* 1234 */	0x4,		/* Oi2 Flags:  has return, */
			0x8,		/* 8 */

	/* Parameter Head */

/* 1236 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1238 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pDataMode */

/* 1242 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1244 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1246 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pConfiguration */

/* 1248 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1250 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1252 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pLayer */

/* 1254 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1256 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pLLD */

/* 1260 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1262 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pULD */

/* 1266 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1268 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1272 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1274 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 1276 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1280 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 1282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Check_CP_Mode */

/* 1284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1290 */	NdrFcShort( 0x21 ),	/* 33 */
#ifndef _ALPHA_
/* 1292 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1296 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1298 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pMode */

/* 1300 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1302 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1306 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1308 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1312 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1314 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1316 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Temperature_Limits */

/* 1318 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1320 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1324 */	NdrFcShort( 0x22 ),	/* 34 */
#ifndef _ALPHA_
/* 1326 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1328 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1330 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1332 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Low */

/* 1334 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1336 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1338 */	0xa,		/* FC_FLOAT */
			0x0,		/* 0 */

	/* Parameter High */

/* 1340 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1342 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1344 */	0xa,		/* FC_FLOAT */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1346 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1348 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1352 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1354 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1356 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Reset_CP */

/* 1358 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1360 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1364 */	NdrFcShort( 0x23 ),	/* 35 */
#ifndef _ALPHA_
/* 1366 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1370 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1372 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Reset */

/* 1374 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1376 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1380 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1382 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1386 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1388 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1390 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Reset_Head */

/* 1392 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1394 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1398 */	NdrFcShort( 0x24 ),	/* 36 */
#ifndef _ALPHA_
/* 1400 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1402 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1404 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1406 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter Head */

/* 1408 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1410 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1414 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1416 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1420 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1422 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1424 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Load_RAM */

/* 1426 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1428 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1432 */	NdrFcShort( 0x25 ),	/* 37 */
#ifndef _ALPHA_
/* 1434 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1436 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1438 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1440 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Configuration */

/* 1442 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1444 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1446 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Head */

/* 1448 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1450 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RAM_Type */

/* 1454 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1456 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1458 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1460 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1462 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1466 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1468 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Time_Window */

/* 1472 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1474 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1478 */	NdrFcShort( 0x26 ),	/* 38 */
#ifndef _ALPHA_
/* 1480 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1482 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1484 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1486 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter WindowSize */

/* 1488 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1490 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1492 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1494 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1496 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1500 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1502 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OS_Command */

/* 1506 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1508 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1512 */	NdrFcShort( 0x27 ),	/* 39 */
#ifndef _ALPHA_
/* 1514 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1516 */	NdrFcShort( 0x810 ),	/* 2064 */
/* 1518 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1520 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 1522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1524 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter CommandString */

/* 1528 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 1530 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1532 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter pStatus */

/* 1534 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1536 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1538 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1540 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1542 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Pass_Command */

/* 1546 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1548 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1552 */	NdrFcShort( 0x28 ),	/* 40 */
#ifndef _ALPHA_
/* 1554 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1556 */	NdrFcShort( 0x810 ),	/* 2064 */
/* 1558 */	NdrFcShort( 0x818 ),	/* 2072 */
/* 1560 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 1562 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1564 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1566 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter CommandString */

/* 1568 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 1570 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1572 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter ResponseStr */

/* 1574 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 1576 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1578 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter pStatus */

/* 1580 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1582 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1586 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1588 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1590 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Tag_Insert */

/* 1592 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1594 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1598 */	NdrFcShort( 0x29 ),	/* 41 */
#ifndef _ALPHA_
/* 1600 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1602 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1604 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1606 */	0x6,		/* Oi2 Flags:  clt must size, has return, */
			0x4,		/* 4 */

	/* Parameter Size */

/* 1608 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1610 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1612 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Tagword */

/* 1614 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
#ifndef _ALPHA_
/* 1616 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1618 */	NdrFcShort( 0xc8 ),	/* Type Offset=200 */

	/* Parameter pStatus */

/* 1620 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1622 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1624 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1626 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1628 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1630 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Tag_Control */

/* 1632 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1634 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1638 */	NdrFcShort( 0x2a ),	/* 42 */
#ifndef _ALPHA_
/* 1640 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1642 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1644 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1646 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter SinglesTag_OffOn */

/* 1648 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1650 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter TimeTag_OffOn */

/* 1654 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1656 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1658 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1660 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1662 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1664 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1666 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1668 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1670 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Transmission_Trajectory */

/* 1672 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1674 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1678 */	NdrFcShort( 0x2b ),	/* 43 */
#ifndef _ALPHA_
/* 1680 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1682 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1684 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1686 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Transaxial_Speed */

/* 1688 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1690 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1692 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Axial_Step */

/* 1694 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1696 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1698 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1700 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1702 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1704 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1706 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1708 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Diagnostics */

/* 1712 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1714 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1718 */	NdrFcShort( 0x2c ),	/* 44 */
#ifndef _ALPHA_
/* 1720 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1722 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1724 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1726 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter pResult */

/* 1728 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1730 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1732 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1734 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1736 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1738 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1740 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1742 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1744 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Force_Reload */

/* 1746 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1748 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1752 */	NdrFcShort( 0x2d ),	/* 45 */
#ifndef _ALPHA_
/* 1754 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1758 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1760 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 1762 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1764 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1766 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Health_Check */

/* 1768 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1770 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1774 */	NdrFcShort( 0x2e ),	/* 46 */
#ifndef _ALPHA_
/* 1776 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1778 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1780 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1782 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter pNumFailed */

/* 1784 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1786 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1788 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Failed */

/* 1790 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 1792 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1794 */	NdrFcShort( 0xf4 ),	/* Type Offset=244 */

	/* Parameter pStatus */

/* 1796 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1798 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1800 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1802 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1804 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1806 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DataStream */

/* 1808 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1810 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1814 */	NdrFcShort( 0x2f ),	/* 47 */
#ifndef _ALPHA_
/* 1816 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1818 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1820 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1822 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter OnOff */

/* 1824 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1826 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1828 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1830 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1832 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1834 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1836 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1838 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1840 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Initialize_PET */

/* 1842 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1844 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1848 */	NdrFcShort( 0x30 ),	/* 48 */
#ifndef _ALPHA_
/* 1850 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1852 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1854 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1856 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter ScanType */

/* 1858 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1860 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1862 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter LLD */

/* 1864 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1866 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1868 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ULD */

/* 1870 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 1872 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1874 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1876 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1878 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1880 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1882 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1884 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Head_Status */

/* 1888 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1890 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1894 */	NdrFcShort( 0x31 ),	/* 49 */
#ifndef _ALPHA_
/* 1896 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1898 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1900 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1902 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter pNumHeads */

/* 1904 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1906 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1908 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Info */

/* 1910 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 1912 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1914 */	NdrFcShort( 0x130 ),	/* Type Offset=304 */

	/* Parameter pStatus */

/* 1916 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1918 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1920 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1922 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1924 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1926 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CountRate */

/* 1928 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1930 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1934 */	NdrFcShort( 0x32 ),	/* 50 */
#ifndef _ALPHA_
/* 1936 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 1938 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1940 */	NdrFcShort( 0x38 ),	/* 56 */
/* 1942 */	0x4,		/* Oi2 Flags:  has return, */
			0x7,		/* 7 */

	/* Parameter pCorrectedSingles */

/* 1944 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1946 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 1948 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pUncorrectedSingles */

/* 1950 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1952 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 1954 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pPrompts */

/* 1956 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1958 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 1960 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pRandoms */

/* 1962 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1964 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 1966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pScatters */

/* 1968 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1970 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1972 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 1974 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 1976 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 1978 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1980 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 1982 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 1984 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Ring_Singles */

/* 1986 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1988 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1992 */	NdrFcShort( 0x33 ),	/* 51 */
#ifndef _ALPHA_
/* 1994 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 1996 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1998 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2000 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x4,		/* 4 */

	/* Parameter pNumRings */

/* 2002 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2004 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2006 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter RingSingles */

/* 2008 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 2010 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2012 */	NdrFcShort( 0xf4 ),	/* Type Offset=244 */

	/* Parameter pStatus */

/* 2014 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2016 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2018 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2022 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure KillSelf */

/* 2026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2032 */	NdrFcShort( 0x34 ),	/* 52 */
#ifndef _ALPHA_
/* 2034 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2040 */	0x4,		/* Oi2 Flags:  has return, */
			0x1,		/* 1 */

	/* Return value */

/* 2042 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2044 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2046 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetLog */

/* 2048 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2050 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2054 */	NdrFcShort( 0x35 ),	/* 53 */
#ifndef _ALPHA_
/* 2056 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2058 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2060 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2062 */	0x4,		/* Oi2 Flags:  has return, */
			0x3,		/* 3 */

	/* Parameter OnOff */

/* 2064 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2066 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 2070 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2072 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2078 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Report_Temperature_Voltage */

/* 2082 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2088 */	NdrFcShort( 0x36 ),	/* 54 */
#ifndef _ALPHA_
/* 2090 */	NdrFcShort( 0x24 ),	/* x86, MIPS, PPC Stack size/offset = 36 */
#else
			NdrFcShort( 0x48 ),	/* Alpha Stack size/offset = 72 */
#endif
/* 2092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2094 */	NdrFcShort( 0x38 ),	/* 56 */
/* 2096 */	0x5,		/* Oi2 Flags:  srv must size, has return, */
			0x8,		/* 8 */

	/* Parameter pCP_Temperature */

/* 2098 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2100 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2102 */	0xa,		/* FC_FLOAT */
			0x0,		/* 0 */

	/* Parameter pMinimum_Temperature */

/* 2104 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2106 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2108 */	0xa,		/* FC_FLOAT */
			0x0,		/* 0 */

	/* Parameter pMaximum_Temperature */

/* 2110 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2112 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2114 */	0xa,		/* FC_FLOAT */
			0x0,		/* 0 */

	/* Parameter pNumber_DHI_Temperatures */

/* 2116 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2118 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2120 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pNumHeads */

/* 2122 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2124 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Info */

/* 2128 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
#ifndef _ALPHA_
/* 2130 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2132 */	NdrFcShort( 0x17a ),	/* Type Offset=378 */

	/* Parameter pStatus */

/* 2134 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2136 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 2138 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2142 */	NdrFcShort( 0x20 ),	/* x86, MIPS, PPC Stack size/offset = 32 */
#else
			NdrFcShort( 0x40 ),	/* Alpha Stack size/offset = 64 */
#endif
/* 2144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Get_Block_Crystal_Position */

/* 2146 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2152 */	NdrFcShort( 0x37 ),	/* 55 */
#ifndef _ALPHA_
/* 2154 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2156 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2158 */	NdrFcShort( 0x818 ),	/* 2072 */
/* 2160 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 2162 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2164 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2168 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2170 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Peaks */

/* 2174 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 2176 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2178 */	NdrFcShort( 0x198 ),	/* Type Offset=408 */

	/* Parameter pStatus */

/* 2180 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2182 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2186 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2188 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2190 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Get_Block_Crystal_Peaks */

/* 2192 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2194 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2198 */	NdrFcShort( 0x38 ),	/* 56 */
#ifndef _ALPHA_
/* 2200 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2202 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2204 */	NdrFcShort( 0x418 ),	/* 1048 */
/* 2206 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 2208 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2210 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2214 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2216 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Peaks */

/* 2220 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 2222 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2224 */	NdrFcShort( 0x19e ),	/* Type Offset=414 */

	/* Parameter pStatus */

/* 2226 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2228 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2230 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2232 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2234 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2236 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Get_Block_Time_Correction */

/* 2238 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2240 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2244 */	NdrFcShort( 0x39 ),	/* 57 */
#ifndef _ALPHA_
/* 2246 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2248 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2250 */	NdrFcShort( 0x418 ),	/* 1048 */
/* 2252 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 2254 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2256 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2260 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2262 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2264 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Correction */

/* 2266 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 2268 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2270 */	NdrFcShort( 0x19e ),	/* Type Offset=414 */

	/* Parameter pStatus */

/* 2272 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2274 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2276 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2280 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Block_Time_Correction */

/* 2284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2290 */	NdrFcShort( 0x3a ),	/* 58 */
#ifndef _ALPHA_
/* 2292 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2294 */	NdrFcShort( 0x418 ),	/* 1048 */
/* 2296 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2298 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 2300 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2302 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2306 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2308 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Correction */

/* 2312 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 2314 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2316 */	NdrFcShort( 0x19e ),	/* Type Offset=414 */

	/* Parameter pStatus */

/* 2318 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2320 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2324 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2326 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Block_Crystal_Peaks */

/* 2330 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2336 */	NdrFcShort( 0x3b ),	/* 59 */
#ifndef _ALPHA_
/* 2338 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2340 */	NdrFcShort( 0x418 ),	/* 1048 */
/* 2342 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2344 */	0x4,		/* Oi2 Flags:  has return, */
			0x5,		/* 5 */

	/* Parameter Head */

/* 2346 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2348 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2350 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2352 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2354 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2356 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Peaks */

/* 2358 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 2360 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2362 */	NdrFcShort( 0x19e ),	/* Type Offset=414 */

	/* Parameter pStatus */

/* 2364 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2366 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2368 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2370 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2372 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2374 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Set_Block_Crystal_Position */

/* 2376 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2378 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2382 */	NdrFcShort( 0x3c ),	/* 60 */
#ifndef _ALPHA_
/* 2384 */	NdrFcShort( 0x1c ),	/* x86, MIPS, PPC Stack size/offset = 28 */
#else
			NdrFcShort( 0x38 ),	/* Alpha Stack size/offset = 56 */
#endif
/* 2386 */	NdrFcShort( 0x820 ),	/* 2080 */
/* 2388 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2390 */	0x4,		/* Oi2 Flags:  has return, */
			0x6,		/* 6 */

	/* Parameter Head */

/* 2392 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2394 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2396 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2398 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2400 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2402 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Peaks */

/* 2404 */	NdrFcShort( 0xa ),	/* Flags:  must free, in, */
#ifndef _ALPHA_
/* 2406 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2408 */	NdrFcShort( 0x198 ),	/* Type Offset=408 */

	/* Parameter StdDev */

/* 2410 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2412 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2414 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 2416 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2418 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2420 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2422 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2424 */	NdrFcShort( 0x18 ),	/* x86, MIPS, PPC Stack size/offset = 24 */
#else
			NdrFcShort( 0x30 ),	/* Alpha Stack size/offset = 48 */
#endif
/* 2426 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Build_CRM */

/* 2428 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2430 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2434 */	NdrFcShort( 0x3d ),	/* 61 */
#ifndef _ALPHA_
/* 2436 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 2438 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2440 */	NdrFcShort( 0x10 ),	/* 16 */
/* 2442 */	0x4,		/* Oi2 Flags:  has return, */
			0x4,		/* 4 */

	/* Parameter Head */

/* 2444 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2446 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2448 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter Block */

/* 2450 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
#ifndef _ALPHA_
/* 2452 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2454 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStatus */

/* 2456 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
#ifndef _ALPHA_
/* 2458 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2460 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2462 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2464 */	NdrFcShort( 0x10 ),	/* x86, MIPS, PPC Stack size/offset = 16 */
#else
			NdrFcShort( 0x20 ),	/* Alpha Stack size/offset = 32 */
#endif
/* 2466 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Last_Async_Message */

/* 2468 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2470 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2474 */	NdrFcShort( 0x3e ),	/* 62 */
#ifndef _ALPHA_
/* 2476 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 2478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2480 */	NdrFcShort( 0x810 ),	/* 2064 */
/* 2482 */	0x4,		/* Oi2 Flags:  has return, */
			0x2,		/* 2 */

	/* Parameter LastMsg */

/* 2484 */	NdrFcShort( 0x12 ),	/* Flags:  must free, out, */
#ifndef _ALPHA_
/* 2486 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 2488 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 2490 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
#ifndef _ALPHA_
/* 2492 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 2494 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const MIDL_TYPE_FORMAT_STRING __MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/*  8 */	NdrFcShort( 0x800 ),	/* 2048 */
/* 10 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 12 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 14 */	NdrFcShort( 0x40 ),	/* 64 */
/* 16 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 18 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 20 */	NdrFcShort( 0x4000 ),	/* 16384 */
/* 22 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 24 */	
			0x11, 0x0,	/* FC_RP */
/* 26 */	NdrFcShort( 0xc ),	/* Offset= 12 (38) */
/* 28 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 30 */	NdrFcShort( 0x1 ),	/* 1 */
/* 32 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 34 */	NdrFcShort( 0xc ),	/* x86, MIPS, PPC Stack size/offset = 12 */
#else
			NdrFcShort( 0x18 ),	/* Alpha Stack size/offset = 24 */
#endif
/* 36 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 38 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 40 */	NdrFcShort( 0x4 ),	/* 4 */
/* 42 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 44 */	NdrFcShort( 0x1 ),	/* 1 */
/* 46 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 48 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 50 */	NdrFcShort( 0x4 ),	/* 4 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x1 ),	/* 1 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	0x13, 0x0,	/* FC_OP */
/* 62 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (28) */
/* 64 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 66 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 68 */	
			0x11, 0x0,	/* FC_RP */
/* 70 */	NdrFcShort( 0xc ),	/* Offset= 12 (82) */
/* 72 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 74 */	NdrFcShort( 0x1 ),	/* 1 */
/* 76 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 78 */	NdrFcShort( 0x8 ),	/* x86, MIPS, PPC Stack size/offset = 8 */
#else
			NdrFcShort( 0x10 ),	/* Alpha Stack size/offset = 16 */
#endif
/* 80 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 82 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 84 */	NdrFcShort( 0x4 ),	/* 4 */
/* 86 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 88 */	NdrFcShort( 0x1 ),	/* 1 */
/* 90 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 92 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 94 */	NdrFcShort( 0x4 ),	/* 4 */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x1 ),	/* 1 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 104 */	0x12, 0x0,	/* FC_UP */
/* 106 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (72) */
/* 108 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 110 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 112 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 114 */	NdrFcShort( 0x4c ),	/* 76 */
/* 116 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 118 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 120 */	NdrFcShort( 0x200 ),	/* 512 */
/* 122 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 124 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 126 */	NdrFcShort( 0x8c ),	/* 140 */
/* 128 */	0xa,		/* FC_FLOAT */
			0x5b,		/* FC_END */
/* 130 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 132 */	NdrFcShort( 0x100 ),	/* 256 */
/* 134 */	0xa,		/* FC_FLOAT */
			0x5b,		/* FC_END */
/* 136 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 138 */	NdrFcShort( 0x140 ),	/* 320 */
/* 140 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 142 */	
			0x11, 0x0,	/* FC_RP */
/* 144 */	NdrFcShort( 0xc ),	/* Offset= 12 (156) */
/* 146 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 148 */	NdrFcShort( 0x1 ),	/* 1 */
/* 150 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 152 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 154 */	0x2,		/* FC_CHAR */
			0x5b,		/* FC_END */
/* 156 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 158 */	NdrFcShort( 0x4 ),	/* 4 */
/* 160 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 162 */	NdrFcShort( 0x1 ),	/* 1 */
/* 164 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 166 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 168 */	NdrFcShort( 0x4 ),	/* 4 */
/* 170 */	NdrFcShort( 0x0 ),	/* 0 */
/* 172 */	NdrFcShort( 0x1 ),	/* 1 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	0x13, 0x0,	/* FC_OP */
/* 180 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (146) */
/* 182 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 184 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 186 */	
			0x11, 0x0,	/* FC_RP */
/* 188 */	NdrFcShort( 0xc ),	/* Offset= 12 (200) */
/* 190 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 192 */	NdrFcShort( 0x8 ),	/* 8 */
/* 194 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x0,		/*  */
#ifndef _ALPHA_
/* 196 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 198 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 200 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 202 */	NdrFcShort( 0x4 ),	/* 4 */
/* 204 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 206 */	NdrFcShort( 0x1 ),	/* 1 */
/* 208 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 210 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 212 */	NdrFcShort( 0x4 ),	/* 4 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 216 */	NdrFcShort( 0x1 ),	/* 1 */
/* 218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 222 */	0x12, 0x0,	/* FC_UP */
/* 224 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (190) */
/* 226 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 228 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 230 */	
			0x11, 0x0,	/* FC_RP */
/* 232 */	NdrFcShort( 0xc ),	/* Offset= 12 (244) */
/* 234 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 236 */	NdrFcShort( 0x4 ),	/* 4 */
/* 238 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 240 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 242 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 244 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 246 */	NdrFcShort( 0x4 ),	/* 4 */
/* 248 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 250 */	NdrFcShort( 0x1 ),	/* 1 */
/* 252 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 254 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 256 */	NdrFcShort( 0x4 ),	/* 4 */
/* 258 */	NdrFcShort( 0x0 ),	/* 0 */
/* 260 */	NdrFcShort( 0x1 ),	/* 1 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 266 */	0x13, 0x0,	/* FC_OP */
/* 268 */	NdrFcShort( 0xffffffde ),	/* Offset= -34 (234) */
/* 270 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 272 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 274 */	
			0x11, 0x0,	/* FC_RP */
/* 276 */	NdrFcShort( 0x1c ),	/* Offset= 28 (304) */
/* 278 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 280 */	NdrFcShort( 0x1c ),	/* 28 */
/* 282 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 284 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 286 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 288 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 290 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 292 */	NdrFcShort( 0x1c ),	/* 28 */
/* 294 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 296 */	NdrFcShort( 0x4 ),	/* x86, MIPS, PPC Stack size/offset = 4 */
#else
			NdrFcShort( 0x8 ),	/* Alpha Stack size/offset = 8 */
#endif
/* 298 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 300 */	NdrFcShort( 0xffffffea ),	/* Offset= -22 (278) */
/* 302 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 304 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 306 */	NdrFcShort( 0x4 ),	/* 4 */
/* 308 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 310 */	NdrFcShort( 0x1 ),	/* 1 */
/* 312 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 314 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 316 */	NdrFcShort( 0x4 ),	/* 4 */
/* 318 */	NdrFcShort( 0x0 ),	/* 0 */
/* 320 */	NdrFcShort( 0x1 ),	/* 1 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 326 */	0x13, 0x0,	/* FC_OP */
/* 328 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (290) */
/* 330 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 332 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 334 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 336 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 338 */	
			0x11, 0x0,	/* FC_RP */
/* 340 */	NdrFcShort( 0x26 ),	/* Offset= 38 (378) */
/* 342 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 344 */	NdrFcShort( 0x40 ),	/* 64 */
/* 346 */	0xa,		/* FC_FLOAT */
			0x5b,		/* FC_END */
/* 348 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 350 */	NdrFcShort( 0x58 ),	/* 88 */
/* 352 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 354 */	0xa,		/* FC_FLOAT */
			0xa,		/* FC_FLOAT */
/* 356 */	0xa,		/* FC_FLOAT */
			0xa,		/* FC_FLOAT */
/* 358 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 360 */	NdrFcShort( 0xffffffee ),	/* Offset= -18 (342) */
/* 362 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 364 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 366 */	NdrFcShort( 0x58 ),	/* 88 */
/* 368 */	0x28,		/* Corr desc:  parameter, FC_LONG */
			0x54,		/* FC_DEREFERENCE */
#ifndef _ALPHA_
/* 370 */	NdrFcShort( 0x14 ),	/* x86, MIPS, PPC Stack size/offset = 20 */
#else
			NdrFcShort( 0x28 ),	/* Alpha Stack size/offset = 40 */
#endif
/* 372 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 374 */	NdrFcShort( 0xffffffe6 ),	/* Offset= -26 (348) */
/* 376 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 378 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 380 */	NdrFcShort( 0x4 ),	/* 4 */
/* 382 */	0x40,		/* Corr desc:  constant, val=256 */
			0x0,		/* 0 */
/* 384 */	NdrFcShort( 0x1 ),	/* 1 */
/* 386 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 388 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 390 */	NdrFcShort( 0x4 ),	/* 4 */
/* 392 */	NdrFcShort( 0x0 ),	/* 0 */
/* 394 */	NdrFcShort( 0x1 ),	/* 1 */
/* 396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 400 */	0x13, 0x0,	/* FC_OP */
/* 402 */	NdrFcShort( 0xffffffda ),	/* Offset= -38 (364) */
/* 404 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 406 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 408 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 410 */	NdrFcShort( 0x800 ),	/* 2048 */
/* 412 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 414 */	
			0x1d,		/* FC_SMFARRAY */
			0x3,		/* 3 */
/* 416 */	NdrFcShort( 0x400 ),	/* 1024 */
/* 418 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */

			0x0
        }
    };

const CInterfaceProxyVtbl * _DHICom_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IAbstract_DHIProxyVtbl,
    0
};

const CInterfaceStubVtbl * _DHICom_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IAbstract_DHIStubVtbl,
    0
};

PCInterfaceName const _DHICom_InterfaceNamesList[] = 
{
    "IAbstract_DHI",
    0
};


#define _DHICom_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _DHICom, pIID, n)

int __stdcall _DHICom_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_DHICom_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo DHICom_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _DHICom_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _DHICom_StubVtblList,
    (const PCInterfaceName * ) & _DHICom_InterfaceNamesList,
    0, // no delegation
    & _DHICom_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
