/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Wed Nov 13 09:38:43 2002
 */
/* Compiler settings for C:\ComExamples\ErrorEvents\ErrorEvents.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ErrorEvents_h__
#define __ErrorEvents_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IErrorEvents_FWD_DEFINED__
#define __IErrorEvents_FWD_DEFINED__
typedef interface IErrorEvents IErrorEvents;
#endif 	/* __IErrorEvents_FWD_DEFINED__ */


#ifndef __ErrorEventsClass_FWD_DEFINED__
#define __ErrorEventsClass_FWD_DEFINED__

#ifdef __cplusplus
typedef class ErrorEventsClass ErrorEventsClass;
#else
typedef struct ErrorEventsClass ErrorEventsClass;
#endif /* __cplusplus */

#endif 	/* __ErrorEventsClass_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IErrorEvents_INTERFACE_DEFINED__
#define __IErrorEvents_INTERFACE_DEFINED__

/* interface IErrorEvents */
/* [unique][helpstring][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IErrorEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80B7D210-AC3F-4A4B-96CA-D46D9DF812C3")
    IErrorEvents : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE NewErrorEvent( 
            /* [string][in] */ unsigned char __RPC_FAR *ApplicationName,
            /* [in] */ int ErrorCode,
            /* [string][in] */ unsigned char __RPC_FAR *AddlText,
            /* [in] */ long RawDataSize,
            /* [size_is][in] */ unsigned char __RPC_FAR *RawData,
            /* [in] */ unsigned char LogIt) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IErrorEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IErrorEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IErrorEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IErrorEvents __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewErrorEvent )( 
            IErrorEvents __RPC_FAR * This,
            /* [string][in] */ unsigned char __RPC_FAR *ApplicationName,
            /* [in] */ int ErrorCode,
            /* [string][in] */ unsigned char __RPC_FAR *AddlText,
            /* [in] */ long RawDataSize,
            /* [size_is][in] */ unsigned char __RPC_FAR *RawData,
            /* [in] */ unsigned char LogIt);
        
        END_INTERFACE
    } IErrorEventsVtbl;

    interface IErrorEvents
    {
        CONST_VTBL struct IErrorEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IErrorEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IErrorEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IErrorEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IErrorEvents_NewErrorEvent(This,ApplicationName,ErrorCode,AddlText,RawDataSize,RawData,LogIt)	\
    (This)->lpVtbl -> NewErrorEvent(This,ApplicationName,ErrorCode,AddlText,RawDataSize,RawData,LogIt)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IErrorEvents_NewErrorEvent_Proxy( 
    IErrorEvents __RPC_FAR * This,
    /* [string][in] */ unsigned char __RPC_FAR *ApplicationName,
    /* [in] */ int ErrorCode,
    /* [string][in] */ unsigned char __RPC_FAR *AddlText,
    /* [in] */ long RawDataSize,
    /* [size_is][in] */ unsigned char __RPC_FAR *RawData,
    /* [in] */ unsigned char LogIt);


void __RPC_STUB IErrorEvents_NewErrorEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IErrorEvents_INTERFACE_DEFINED__ */



#ifndef __ERROREVENTSLib_LIBRARY_DEFINED__
#define __ERROREVENTSLib_LIBRARY_DEFINED__

/* library ERROREVENTSLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ERROREVENTSLib;

EXTERN_C const CLSID CLSID_ErrorEventsClass;

#ifdef __cplusplus

class DECLSPEC_UUID("E667D463-753F-4AC2-83BF-B57C5FD3C9C2")
ErrorEventsClass;
#endif
#endif /* __ERROREVENTSLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
