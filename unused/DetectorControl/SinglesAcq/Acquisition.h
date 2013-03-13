/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:07 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\SinglesAcq\Acquisition.idl:
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

#ifndef __Acquisition_h__
#define __Acquisition_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IACQMain_FWD_DEFINED__
#define __IACQMain_FWD_DEFINED__
typedef interface IACQMain IACQMain;
#endif 	/* __IACQMain_FWD_DEFINED__ */


#ifndef __ACQMain_FWD_DEFINED__
#define __ACQMain_FWD_DEFINED__

#ifdef __cplusplus
typedef class ACQMain ACQMain;
#else
typedef struct ACQMain ACQMain;
#endif /* __cplusplus */

#endif 	/* __ACQMain_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IACQMain_INTERFACE_DEFINED__
#define __IACQMain_INTERFACE_DEFINED__

/* interface IACQMain */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IACQMain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("28874B6F-9FFA-4898-95EA-823F1EE11A3F")
    IACQMain : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquire_Singles( 
            /* [in] */ long Mode,
            /* [in] */ long __RPC_FAR AcquireSelect[ 16 ],
            /* [in] */ long Type,
            /* [in] */ long Amount,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Progress( 
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Retrieve_Singles( 
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pDataSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KillSelf( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Statistics( 
            /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
            /* [out] */ unsigned long __RPC_FAR *pBadAddress,
            /* [out] */ unsigned long __RPC_FAR *pSyncProb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IACQMainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IACQMain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IACQMain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IACQMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquire_Singles )( 
            IACQMain __RPC_FAR * This,
            /* [in] */ long Mode,
            /* [in] */ long __RPC_FAR AcquireSelect[ 16 ],
            /* [in] */ long Type,
            /* [in] */ long Amount,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Progress )( 
            IACQMain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Retrieve_Singles )( 
            IACQMain __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pDataSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Abort )( 
            IACQMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KillSelf )( 
            IACQMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Statistics )( 
            IACQMain __RPC_FAR * This,
            /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
            /* [out] */ unsigned long __RPC_FAR *pBadAddress,
            /* [out] */ unsigned long __RPC_FAR *pSyncProb);
        
        END_INTERFACE
    } IACQMainVtbl;

    interface IACQMain
    {
        CONST_VTBL struct IACQMainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IACQMain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IACQMain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IACQMain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IACQMain_Acquire_Singles(This,Mode,AcquireSelect,Type,Amount,pStatus)	\
    (This)->lpVtbl -> Acquire_Singles(This,Mode,AcquireSelect,Type,Amount,pStatus)

#define IACQMain_Progress(This,pPercent,pStatus)	\
    (This)->lpVtbl -> Progress(This,pPercent,pStatus)

#define IACQMain_Retrieve_Singles(This,Head,pDataSize,buffer,pStatus)	\
    (This)->lpVtbl -> Retrieve_Singles(This,Head,pDataSize,buffer,pStatus)

#define IACQMain_Abort(This)	\
    (This)->lpVtbl -> Abort(This)

#define IACQMain_KillSelf(This)	\
    (This)->lpVtbl -> KillSelf(This)

#define IACQMain_Statistics(This,pNumKEvents,pBadAddress,pSyncProb)	\
    (This)->lpVtbl -> Statistics(This,pNumKEvents,pBadAddress,pSyncProb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_Acquire_Singles_Proxy( 
    IACQMain __RPC_FAR * This,
    /* [in] */ long Mode,
    /* [in] */ long __RPC_FAR AcquireSelect[ 16 ],
    /* [in] */ long Type,
    /* [in] */ long Amount,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IACQMain_Acquire_Singles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_Progress_Proxy( 
    IACQMain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pPercent,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IACQMain_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_Retrieve_Singles_Proxy( 
    IACQMain __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pDataSize,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IACQMain_Retrieve_Singles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_Abort_Proxy( 
    IACQMain __RPC_FAR * This);


void __RPC_STUB IACQMain_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_KillSelf_Proxy( 
    IACQMain __RPC_FAR * This);


void __RPC_STUB IACQMain_KillSelf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IACQMain_Statistics_Proxy( 
    IACQMain __RPC_FAR * This,
    /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
    /* [out] */ unsigned long __RPC_FAR *pBadAddress,
    /* [out] */ unsigned long __RPC_FAR *pSyncProb);


void __RPC_STUB IACQMain_Statistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IACQMain_INTERFACE_DEFINED__ */



#ifndef __ACQUISITIONLib_LIBRARY_DEFINED__
#define __ACQUISITIONLib_LIBRARY_DEFINED__

/* library ACQUISITIONLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_ACQUISITIONLib;

EXTERN_C const CLSID CLSID_ACQMain;

#ifdef __cplusplus

class DECLSPEC_UUID("E8A509A4-3C3F-4348-8EF3-084992DDDC78")
ACQMain;
#endif
#endif /* __ACQUISITIONLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
