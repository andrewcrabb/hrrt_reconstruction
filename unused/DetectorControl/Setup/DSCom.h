/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:36 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\Setup\DSCom.idl:
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

#ifndef __DSCom_h__
#define __DSCom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDSMain_FWD_DEFINED__
#define __IDSMain_FWD_DEFINED__
typedef interface IDSMain IDSMain;
#endif 	/* __IDSMain_FWD_DEFINED__ */


#ifndef __DSMain_FWD_DEFINED__
#define __DSMain_FWD_DEFINED__

#ifdef __cplusplus
typedef class DSMain DSMain;
#else
typedef struct DSMain DSMain;
#endif /* __cplusplus */

#endif 	/* __DSMain_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDSMain_INTERFACE_DEFINED__
#define __IDSMain_INTERFACE_DEFINED__

/* interface IDSMain */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDSMain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3179ED9A-BDE8-4BBA-9FF6-D222FD35EDEF")
    IDSMain : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Setup( 
            /* [in] */ long Type,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Error_Lookup( 
            /* [in] */ long Code,
            /* [out] */ unsigned char __RPC_FAR ErrorStr[ 2048 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Setup_Progress( 
            /* [out] */ unsigned char __RPC_FAR Stage[ 2048 ],
            /* [out] */ long __RPC_FAR *pStagePercent,
            /* [out] */ long __RPC_FAR *pTotalPercent) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Setup_Finished( 
            /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Time_Alignment( 
            /* [in] */ long Type,
            /* [in] */ long ModulePair,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KillSelf( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDSMainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDSMain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDSMain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDSMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IDSMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Setup )( 
            IDSMain __RPC_FAR * This,
            /* [in] */ long Type,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Error_Lookup )( 
            IDSMain __RPC_FAR * This,
            /* [in] */ long Code,
            /* [out] */ unsigned char __RPC_FAR ErrorStr[ 2048 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Setup_Progress )( 
            IDSMain __RPC_FAR * This,
            /* [out] */ unsigned char __RPC_FAR Stage[ 2048 ],
            /* [out] */ long __RPC_FAR *pStagePercent,
            /* [out] */ long __RPC_FAR *pTotalPercent);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Setup_Finished )( 
            IDSMain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Time_Alignment )( 
            IDSMain __RPC_FAR * This,
            /* [in] */ long Type,
            /* [in] */ long ModulePair,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KillSelf )( 
            IDSMain __RPC_FAR * This);
        
        END_INTERFACE
    } IDSMainVtbl;

    interface IDSMain
    {
        CONST_VTBL struct IDSMainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDSMain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDSMain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDSMain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDSMain_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IDSMain_Setup(This,Type,Configuration,BlockSelect,pStatus)	\
    (This)->lpVtbl -> Setup(This,Type,Configuration,BlockSelect,pStatus)

#define IDSMain_Error_Lookup(This,Code,ErrorStr)	\
    (This)->lpVtbl -> Error_Lookup(This,Code,ErrorStr)

#define IDSMain_Setup_Progress(This,Stage,pStagePercent,pTotalPercent)	\
    (This)->lpVtbl -> Setup_Progress(This,Stage,pStagePercent,pTotalPercent)

#define IDSMain_Setup_Finished(This,BlockStatus)	\
    (This)->lpVtbl -> Setup_Finished(This,BlockStatus)

#define IDSMain_Time_Alignment(This,Type,ModulePair,pStatus)	\
    (This)->lpVtbl -> Time_Alignment(This,Type,ModulePair,pStatus)

#define IDSMain_KillSelf(This)	\
    (This)->lpVtbl -> KillSelf(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Abort_Proxy( 
    IDSMain __RPC_FAR * This);


void __RPC_STUB IDSMain_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Setup_Proxy( 
    IDSMain __RPC_FAR * This,
    /* [in] */ long Type,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDSMain_Setup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Error_Lookup_Proxy( 
    IDSMain __RPC_FAR * This,
    /* [in] */ long Code,
    /* [out] */ unsigned char __RPC_FAR ErrorStr[ 2048 ]);


void __RPC_STUB IDSMain_Error_Lookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Setup_Progress_Proxy( 
    IDSMain __RPC_FAR * This,
    /* [out] */ unsigned char __RPC_FAR Stage[ 2048 ],
    /* [out] */ long __RPC_FAR *pStagePercent,
    /* [out] */ long __RPC_FAR *pTotalPercent);


void __RPC_STUB IDSMain_Setup_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Setup_Finished_Proxy( 
    IDSMain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]);


void __RPC_STUB IDSMain_Setup_Finished_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_Time_Alignment_Proxy( 
    IDSMain __RPC_FAR * This,
    /* [in] */ long Type,
    /* [in] */ long ModulePair,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDSMain_Time_Alignment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDSMain_KillSelf_Proxy( 
    IDSMain __RPC_FAR * This);


void __RPC_STUB IDSMain_KillSelf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDSMain_INTERFACE_DEFINED__ */



#ifndef __DSCOMLib_LIBRARY_DEFINED__
#define __DSCOMLib_LIBRARY_DEFINED__

/* library DSCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DSCOMLib;

EXTERN_C const CLSID CLSID_DSMain;

#ifdef __cplusplus

class DECLSPEC_UUID("E0F5B940-9FDA-4273-BA79-B55501521E97")
DSMain;
#endif
#endif /* __DSCOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
