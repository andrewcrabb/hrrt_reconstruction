/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri May 24 15:01:59 2002
 */
/* Compiler settings for C:\Data\Backup1\Code\PVCS\P39\CC_assy\ccmon-nt\cpcom\cpcom.idl:
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

#ifndef __cpcom_h__
#define __cpcom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __Icpmain_FWD_DEFINED__
#define __Icpmain_FWD_DEFINED__
typedef interface Icpmain Icpmain;
#endif 	/* __Icpmain_FWD_DEFINED__ */


#ifndef __Idhimain_FWD_DEFINED__
#define __Idhimain_FWD_DEFINED__
typedef interface Idhimain Idhimain;
#endif 	/* __Idhimain_FWD_DEFINED__ */


#ifndef __cpmain_FWD_DEFINED__
#define __cpmain_FWD_DEFINED__

#ifdef __cplusplus
typedef class cpmain cpmain;
#else
typedef struct cpmain cpmain;
#endif /* __cplusplus */

#endif 	/* __cpmain_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __Icpmain_INTERFACE_DEFINED__
#define __Icpmain_INTERFACE_DEFINED__

/* interface Icpmain */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_Icpmain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FFCBB3A7-FF42-4CDE-A4F8-35362D3357D3")
    Icpmain : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CP_Error_Lookup( 
            /* [in] */ long error_value,
            /* [out] */ unsigned char __RPC_FAR error_string[ 2048 ],
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Rpt_Temps( 
            /* [out] */ float __RPC_FAR temps[ 35 ],
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Temps( 
            /* [in] */ float low,
            /* [in] */ float high,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Mode( 
            /* [in] */ long mode,
            /* [in] */ long mp_head,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Tag_Control( 
            /* [in] */ long singles,
            /* [in] */ long timetag,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CP_Ping( 
            /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
            /* [out] */ long __RPC_FAR *pct_complete,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CP_Reset( 
            /* [in] */ long reset,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Tag_Insert( 
            /* [in] */ long size,
            /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *tagword,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CP_Conf( 
            /* [out] */ long __RPC_FAR *config,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stats( 
            /* [out] */ long __RPC_FAR stat[ 19 ],
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE HW_Conf( 
            /* [out] */ unsigned long __RPC_FAR *fsize,
            /* [out] */ unsigned long __RPC_FAR *checksum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *HW_file,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CP_Diag( 
            /* [out] */ long __RPC_FAR *diag_status,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start( 
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop( 
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Time_Window( 
            /* [in] */ long window,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Peek( 
            /* [in] */ unsigned short offset,
            /* [out] */ unsigned short __RPC_FAR *value,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Poke( 
            /* [in] */ unsigned short offset,
            /* [in] */ unsigned short value,
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IcpmainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Icpmain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Icpmain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CP_Error_Lookup )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long error_value,
            /* [out] */ unsigned char __RPC_FAR error_string[ 2048 ],
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Rpt_Temps )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ float __RPC_FAR temps[ 35 ],
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Temps )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ float low,
            /* [in] */ float high,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Mode )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long mode,
            /* [in] */ long mp_head,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Tag_Control )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long singles,
            /* [in] */ long timetag,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CP_Ping )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
            /* [out] */ long __RPC_FAR *pct_complete,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CP_Reset )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long reset,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Tag_Insert )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long size,
            /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *tagword,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CP_Conf )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *config,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stats )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR stat[ 19 ],
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *HW_Conf )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ unsigned long __RPC_FAR *fsize,
            /* [out] */ unsigned long __RPC_FAR *checksum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *HW_file,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CP_Diag )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *diag_status,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            Icpmain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Time_Window )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ long window,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Peek )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ unsigned short offset,
            /* [out] */ unsigned short __RPC_FAR *value,
            /* [out] */ long __RPC_FAR *error_status);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Poke )( 
            Icpmain __RPC_FAR * This,
            /* [in] */ unsigned short offset,
            /* [in] */ unsigned short value,
            /* [out] */ long __RPC_FAR *error_status);
        
        END_INTERFACE
    } IcpmainVtbl;

    interface Icpmain
    {
        CONST_VTBL struct IcpmainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Icpmain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Icpmain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Icpmain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Icpmain_CP_Error_Lookup(This,error_value,error_string,error_status)	\
    (This)->lpVtbl -> CP_Error_Lookup(This,error_value,error_string,error_status)

#define Icpmain_Rpt_Temps(This,temps,error_status)	\
    (This)->lpVtbl -> Rpt_Temps(This,temps,error_status)

#define Icpmain_Set_Temps(This,low,high,error_status)	\
    (This)->lpVtbl -> Set_Temps(This,low,high,error_status)

#define Icpmain_Set_Mode(This,mode,mp_head,error_status)	\
    (This)->lpVtbl -> Set_Mode(This,mode,mp_head,error_status)

#define Icpmain_Tag_Control(This,singles,timetag,error_status)	\
    (This)->lpVtbl -> Tag_Control(This,singles,timetag,error_status)

#define Icpmain_CP_Ping(This,res_string,pct_complete,error_status)	\
    (This)->lpVtbl -> CP_Ping(This,res_string,pct_complete,error_status)

#define Icpmain_CP_Reset(This,reset,error_status)	\
    (This)->lpVtbl -> CP_Reset(This,reset,error_status)

#define Icpmain_Tag_Insert(This,size,tagword,error_status)	\
    (This)->lpVtbl -> Tag_Insert(This,size,tagword,error_status)

#define Icpmain_CP_Conf(This,config,error_status)	\
    (This)->lpVtbl -> CP_Conf(This,config,error_status)

#define Icpmain_Stats(This,stat,error_status)	\
    (This)->lpVtbl -> Stats(This,stat,error_status)

#define Icpmain_HW_Conf(This,fsize,checksum,HW_file,error_status)	\
    (This)->lpVtbl -> HW_Conf(This,fsize,checksum,HW_file,error_status)

#define Icpmain_CP_Diag(This,diag_status,error_status)	\
    (This)->lpVtbl -> CP_Diag(This,diag_status,error_status)

#define Icpmain_Start(This,error_status)	\
    (This)->lpVtbl -> Start(This,error_status)

#define Icpmain_Stop(This,error_status)	\
    (This)->lpVtbl -> Stop(This,error_status)

#define Icpmain_Time_Window(This,window,error_status)	\
    (This)->lpVtbl -> Time_Window(This,window,error_status)

#define Icpmain_Peek(This,offset,value,error_status)	\
    (This)->lpVtbl -> Peek(This,offset,value,error_status)

#define Icpmain_Poke(This,offset,value,error_status)	\
    (This)->lpVtbl -> Poke(This,offset,value,error_status)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_CP_Error_Lookup_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long error_value,
    /* [out] */ unsigned char __RPC_FAR error_string[ 2048 ],
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_CP_Error_Lookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Rpt_Temps_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ float __RPC_FAR temps[ 35 ],
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Rpt_Temps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Set_Temps_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ float low,
    /* [in] */ float high,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Set_Temps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Set_Mode_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long mode,
    /* [in] */ long mp_head,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Set_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Tag_Control_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long singles,
    /* [in] */ long timetag,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Tag_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_CP_Ping_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
    /* [out] */ long __RPC_FAR *pct_complete,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_CP_Ping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_CP_Reset_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long reset,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_CP_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Tag_Insert_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long size,
    /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *tagword,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Tag_Insert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_CP_Conf_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *config,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_CP_Conf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Stats_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR stat[ 19 ],
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Stats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_HW_Conf_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ unsigned long __RPC_FAR *fsize,
    /* [out] */ unsigned long __RPC_FAR *checksum,
    /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *HW_file,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_HW_Conf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_CP_Diag_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *diag_status,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_CP_Diag_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Start_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Stop_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Time_Window_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ long window,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Time_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Peek_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ unsigned short offset,
    /* [out] */ unsigned short __RPC_FAR *value,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Peek_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE Icpmain_Poke_Proxy( 
    Icpmain __RPC_FAR * This,
    /* [in] */ unsigned short offset,
    /* [in] */ unsigned short value,
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Icpmain_Poke_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Icpmain_INTERFACE_DEFINED__ */


#ifndef __Idhimain_INTERFACE_DEFINED__
#define __Idhimain_INTERFACE_DEFINED__

/* interface Idhimain */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_Idhimain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E47CA878-87F5-4724-A6C2-9D7D10BAB197")
    Idhimain : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Send_DHI( 
            /* [in] */ long addr,
            /* [in] */ unsigned char __RPC_FAR cmd_string[ 2048 ],
            /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
            /* [out] */ long __RPC_FAR *error_status) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IdhimainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            Idhimain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            Idhimain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            Idhimain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Send_DHI )( 
            Idhimain __RPC_FAR * This,
            /* [in] */ long addr,
            /* [in] */ unsigned char __RPC_FAR cmd_string[ 2048 ],
            /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
            /* [out] */ long __RPC_FAR *error_status);
        
        END_INTERFACE
    } IdhimainVtbl;

    interface Idhimain
    {
        CONST_VTBL struct IdhimainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Idhimain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define Idhimain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define Idhimain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define Idhimain_Send_DHI(This,addr,cmd_string,res_string,error_status)	\
    (This)->lpVtbl -> Send_DHI(This,addr,cmd_string,res_string,error_status)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE Idhimain_Send_DHI_Proxy( 
    Idhimain __RPC_FAR * This,
    /* [in] */ long addr,
    /* [in] */ unsigned char __RPC_FAR cmd_string[ 2048 ],
    /* [out] */ unsigned char __RPC_FAR res_string[ 2048 ],
    /* [out] */ long __RPC_FAR *error_status);


void __RPC_STUB Idhimain_Send_DHI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __Idhimain_INTERFACE_DEFINED__ */



#ifndef __CPCOMLib_LIBRARY_DEFINED__
#define __CPCOMLib_LIBRARY_DEFINED__

/* library CPCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_CPCOMLib;

EXTERN_C const CLSID CLSID_cpmain;

#ifdef __cplusplus

class DECLSPEC_UUID("EC238923-6546-463D-8BBE-794DDC0937BB")
cpmain;
#endif
#endif /* __CPCOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
