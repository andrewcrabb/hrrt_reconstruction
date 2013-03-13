/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:27:19 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\Utils\DUCom.idl:
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

#ifndef __DUCom_h__
#define __DUCom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IDUMain_FWD_DEFINED__
#define __IDUMain_FWD_DEFINED__
typedef interface IDUMain IDUMain;
#endif 	/* __IDUMain_FWD_DEFINED__ */


#ifndef __DUMain_FWD_DEFINED__
#define __DUMain_FWD_DEFINED__

#ifdef __cplusplus
typedef class DUMain DUMain;
#else
typedef struct DUMain DUMain;
#endif /* __cplusplus */

#endif 	/* __DUMain_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IDUMain_INTERFACE_DEFINED__
#define __IDUMain_INTERFACE_DEFINED__

/* interface IDUMain */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDUMain;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C0AA4C6E-2E3E-4512-8435-114CE27E07FD")
    IDUMain : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquire_Singles_Data( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [in] */ long __RPC_FAR SourceSelect[ 16 ],
            /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
            /* [in] */ long AcquisitionType,
            /* [in] */ long Amount,
            /* [in] */ long lld,
            /* [in] */ long uld,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquire_Coinc_Data( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
            /* [in] */ long ModulePair,
            /* [in] */ long Transmission,
            /* [in] */ long AcquisitionType,
            /* [in] */ long Amount,
            /* [in] */ long lld,
            /* [in] */ long uld,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquisition_Progress( 
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Current_Head_Data( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR *pTime,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Current_Block_Data( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Download_CRM( 
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
            /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Stored_CRM( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [out] */ unsigned char __RPC_FAR CRM[ 65536 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Stored_Crystal_Position( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Stored_Crystal_Time( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalTime[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Stored_Energy_Peaks( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPeaks[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Stored_Settings( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE List_Setup_Archives( 
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR *pNumFound,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *List,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Load_Setup_Archive( 
            /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Remove_Setup_Archive( 
            /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Save_Setup_Archive( 
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Store_CRM( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [in] */ unsigned char __RPC_FAR CRM[ 65536 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Store_Crystal_Position( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Store_Crystal_Time( 
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalTime[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Store_Energy_Peaks( 
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalPeaks[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Store_Settings( 
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Read_Master( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Write_Master( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Master_Crystal_Location( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Archive_Progress( 
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ unsigned char __RPC_FAR StageStr[ 2048 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Archive_Abort( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquisition_Abort( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KillSelf( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Save_Head( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Load_Head( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Acquisition_Statistics( 
            /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
            /* [out] */ unsigned long __RPC_FAR *pBadAddress,
            /* [out] */ unsigned long __RPC_FAR *pSyncProb) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDUMainVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDUMain __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDUMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquire_Singles_Data )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [in] */ long __RPC_FAR SourceSelect[ 16 ],
            /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
            /* [in] */ long AcquisitionType,
            /* [in] */ long Amount,
            /* [in] */ long lld,
            /* [in] */ long uld,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquire_Coinc_Data )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
            /* [in] */ long ModulePair,
            /* [in] */ long Transmission,
            /* [in] */ long AcquisitionType,
            /* [in] */ long Amount,
            /* [in] */ long lld,
            /* [in] */ long uld,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquisition_Progress )( 
            IDUMain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Current_Head_Data )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR *pTime,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Current_Block_Data )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Download_CRM )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
            /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Stored_CRM )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [out] */ unsigned char __RPC_FAR CRM[ 65536 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Stored_Crystal_Position )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Stored_Crystal_Time )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalTime[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Stored_Energy_Peaks )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPeaks[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Stored_Settings )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *List_Setup_Archives )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR *pNumFound,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *List,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load_Setup_Archive )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove_Setup_Archive )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save_Setup_Archive )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Store_CRM )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [in] */ unsigned char __RPC_FAR CRM[ 65536 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Store_Crystal_Position )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Store_Crystal_Time )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalTime[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Store_Energy_Peaks )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long __RPC_FAR CrystalPeaks[ 32768 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Store_Settings )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Send,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read_Master )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write_Master )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ long __RPC_FAR *__RPC_FAR *Data,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Master_Crystal_Location )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Archive_Progress )( 
            IDUMain __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ unsigned char __RPC_FAR StageStr[ 2048 ],
            /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Archive_Abort )( 
            IDUMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquisition_Abort )( 
            IDUMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KillSelf )( 
            IDUMain __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save_Head )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pSize,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load_Head )( 
            IDUMain __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Acquisition_Statistics )( 
            IDUMain __RPC_FAR * This,
            /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
            /* [out] */ unsigned long __RPC_FAR *pBadAddress,
            /* [out] */ unsigned long __RPC_FAR *pSyncProb);
        
        END_INTERFACE
    } IDUMainVtbl;

    interface IDUMain
    {
        CONST_VTBL struct IDUMainVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDUMain_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDUMain_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDUMain_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDUMain_Acquire_Singles_Data(This,DataMode,Configuration,HeadSelect,SourceSelect,LayerSelect,AcquisitionType,Amount,lld,uld,pStatus)	\
    (This)->lpVtbl -> Acquire_Singles_Data(This,DataMode,Configuration,HeadSelect,SourceSelect,LayerSelect,AcquisitionType,Amount,lld,uld,pStatus)

#define IDUMain_Acquire_Coinc_Data(This,DataMode,Configuration,HeadSelect,LayerSelect,ModulePair,Transmission,AcquisitionType,Amount,lld,uld,pStatus)	\
    (This)->lpVtbl -> Acquire_Coinc_Data(This,DataMode,Configuration,HeadSelect,LayerSelect,ModulePair,Transmission,AcquisitionType,Amount,lld,uld,pStatus)

#define IDUMain_Acquisition_Progress(This,pPercent,HeadStatus)	\
    (This)->lpVtbl -> Acquisition_Progress(This,pPercent,HeadStatus)

#define IDUMain_Current_Head_Data(This,DataMode,Configuration,Head,Layer,pTime,pSize,Data,pStatus)	\
    (This)->lpVtbl -> Current_Head_Data(This,DataMode,Configuration,Head,Layer,pTime,pSize,Data,pStatus)

#define IDUMain_Current_Block_Data(This,DataMode,Configuration,Head,Layer,Block,pSize,Data,pStatus)	\
    (This)->lpVtbl -> Current_Block_Data(This,DataMode,Configuration,Head,Layer,Block,pSize,Data,pStatus)

#define IDUMain_Download_CRM(This,Configuration,BlockSelect,BlockStatus)	\
    (This)->lpVtbl -> Download_CRM(This,Configuration,BlockSelect,BlockStatus)

#define IDUMain_Get_Stored_CRM(This,Configuration,Head,Layer,Block,CRM,pStatus)	\
    (This)->lpVtbl -> Get_Stored_CRM(This,Configuration,Head,Layer,Block,CRM,pStatus)

#define IDUMain_Get_Stored_Crystal_Position(This,Configuration,Head,Layer,CrystalPosition,pStatus)	\
    (This)->lpVtbl -> Get_Stored_Crystal_Position(This,Configuration,Head,Layer,CrystalPosition,pStatus)

#define IDUMain_Get_Stored_Crystal_Time(This,Configuration,Head,Layer,CrystalTime,pStatus)	\
    (This)->lpVtbl -> Get_Stored_Crystal_Time(This,Configuration,Head,Layer,CrystalTime,pStatus)

#define IDUMain_Get_Stored_Energy_Peaks(This,Configuration,Head,Layer,CrystalPeaks,pStatus)	\
    (This)->lpVtbl -> Get_Stored_Energy_Peaks(This,Configuration,Head,Layer,CrystalPeaks,pStatus)

#define IDUMain_Get_Stored_Settings(This,Configuration,Head,Settings,pStatus)	\
    (This)->lpVtbl -> Get_Stored_Settings(This,Configuration,Head,Settings,pStatus)

#define IDUMain_List_Setup_Archives(This,Configuration,HeadSelect,pNumFound,List,pStatus)	\
    (This)->lpVtbl -> List_Setup_Archives(This,Configuration,HeadSelect,pNumFound,List,pStatus)

#define IDUMain_Load_Setup_Archive(This,Archive,HeadSelect,HeadStatus)	\
    (This)->lpVtbl -> Load_Setup_Archive(This,Archive,HeadSelect,HeadStatus)

#define IDUMain_Remove_Setup_Archive(This,Archive,pStatus)	\
    (This)->lpVtbl -> Remove_Setup_Archive(This,Archive,pStatus)

#define IDUMain_Save_Setup_Archive(This,Configuration,HeadSelect,HeadStatus)	\
    (This)->lpVtbl -> Save_Setup_Archive(This,Configuration,HeadSelect,HeadStatus)

#define IDUMain_Store_CRM(This,Configuration,Head,Layer,Block,CRM,pStatus)	\
    (This)->lpVtbl -> Store_CRM(This,Configuration,Head,Layer,Block,CRM,pStatus)

#define IDUMain_Store_Crystal_Position(This,Configuration,Head,Layer,CrystalPosition,pStatus)	\
    (This)->lpVtbl -> Store_Crystal_Position(This,Configuration,Head,Layer,CrystalPosition,pStatus)

#define IDUMain_Store_Crystal_Time(This,Send,Configuration,Head,Layer,CrystalTime,pStatus)	\
    (This)->lpVtbl -> Store_Crystal_Time(This,Send,Configuration,Head,Layer,CrystalTime,pStatus)

#define IDUMain_Store_Energy_Peaks(This,Send,Configuration,Head,Layer,CrystalPeaks,pStatus)	\
    (This)->lpVtbl -> Store_Energy_Peaks(This,Send,Configuration,Head,Layer,CrystalPeaks,pStatus)

#define IDUMain_Store_Settings(This,Send,Configuration,Head,Settings,pStatus)	\
    (This)->lpVtbl -> Store_Settings(This,Send,Configuration,Head,Settings,pStatus)

#define IDUMain_Read_Master(This,DataMode,Configuration,Head,Layer,pSize,Data,pStatus)	\
    (This)->lpVtbl -> Read_Master(This,DataMode,Configuration,Head,Layer,pSize,Data,pStatus)

#define IDUMain_Write_Master(This,DataMode,Configuration,Head,Layer,Size,Data,pStatus)	\
    (This)->lpVtbl -> Write_Master(This,DataMode,Configuration,Head,Layer,Size,Data,pStatus)

#define IDUMain_Get_Master_Crystal_Location(This,Configuration,Head,Layer,CrystalPosition,pStatus)	\
    (This)->lpVtbl -> Get_Master_Crystal_Location(This,Configuration,Head,Layer,CrystalPosition,pStatus)

#define IDUMain_Archive_Progress(This,pPercent,StageStr,HeadStatus)	\
    (This)->lpVtbl -> Archive_Progress(This,pPercent,StageStr,HeadStatus)

#define IDUMain_Archive_Abort(This)	\
    (This)->lpVtbl -> Archive_Abort(This)

#define IDUMain_Acquisition_Abort(This)	\
    (This)->lpVtbl -> Acquisition_Abort(This)

#define IDUMain_KillSelf(This)	\
    (This)->lpVtbl -> KillSelf(This)

#define IDUMain_Save_Head(This,Configuration,Head,pSize,buffer,pStatus)	\
    (This)->lpVtbl -> Save_Head(This,Configuration,Head,pSize,buffer,pStatus)

#define IDUMain_Load_Head(This,Configuration,Head,Size,buffer,pStatus)	\
    (This)->lpVtbl -> Load_Head(This,Configuration,Head,Size,buffer,pStatus)

#define IDUMain_Acquisition_Statistics(This,pNumKEvents,pBadAddress,pSyncProb)	\
    (This)->lpVtbl -> Acquisition_Statistics(This,pNumKEvents,pBadAddress,pSyncProb)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Acquire_Singles_Data_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
    /* [in] */ long __RPC_FAR SourceSelect[ 16 ],
    /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
    /* [in] */ long AcquisitionType,
    /* [in] */ long Amount,
    /* [in] */ long lld,
    /* [in] */ long uld,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Acquire_Singles_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Acquire_Coinc_Data_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
    /* [in] */ long __RPC_FAR LayerSelect[ 16 ],
    /* [in] */ long ModulePair,
    /* [in] */ long Transmission,
    /* [in] */ long AcquisitionType,
    /* [in] */ long Amount,
    /* [in] */ long lld,
    /* [in] */ long uld,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Acquire_Coinc_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Acquisition_Progress_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pPercent,
    /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);


void __RPC_STUB IDUMain_Acquisition_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Current_Head_Data_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR *pTime,
    /* [out] */ long __RPC_FAR *pSize,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Current_Head_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Current_Block_Data_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pSize,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Current_Block_Data_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Download_CRM_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR BlockSelect[ 2048 ],
    /* [out] */ long __RPC_FAR BlockStatus[ 2048 ]);


void __RPC_STUB IDUMain_Download_CRM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Stored_CRM_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long Block,
    /* [out] */ unsigned char __RPC_FAR CRM[ 65536 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Stored_CRM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Stored_Crystal_Position_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Stored_Crystal_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Stored_Crystal_Time_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR CrystalTime[ 32768 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Stored_Crystal_Time_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Stored_Energy_Peaks_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR CrystalPeaks[ 32768 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Stored_Energy_Peaks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Stored_Settings_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR Settings[ 4096 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Stored_Settings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_List_Setup_Archives_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
    /* [out] */ long __RPC_FAR *pNumFound,
    /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *List,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_List_Setup_Archives_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Load_Setup_Archive_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
    /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
    /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);


void __RPC_STUB IDUMain_Load_Setup_Archive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Remove_Setup_Archive_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ unsigned char __RPC_FAR Archive[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Remove_Setup_Archive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Save_Setup_Archive_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long __RPC_FAR HeadSelect[ 16 ],
    /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);


void __RPC_STUB IDUMain_Save_Setup_Archive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Store_CRM_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long Block,
    /* [in] */ unsigned char __RPC_FAR CRM[ 65536 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Store_CRM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Store_Crystal_Position_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long __RPC_FAR CrystalPosition[ 278784 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Store_Crystal_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Store_Crystal_Time_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Send,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long __RPC_FAR CrystalTime[ 32768 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Store_Crystal_Time_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Store_Energy_Peaks_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Send,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long __RPC_FAR CrystalPeaks[ 32768 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Store_Energy_Peaks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Store_Settings_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Send,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long __RPC_FAR Settings[ 4096 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Store_Settings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Read_Master_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR *pSize,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Data,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Read_Master_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Write_Master_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long Size,
    /* [size_is][size_is][in] */ long __RPC_FAR *__RPC_FAR *Data,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Write_Master_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Get_Master_Crystal_Location_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [out] */ long __RPC_FAR CrystalPosition[ 278784 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Get_Master_Crystal_Location_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Archive_Progress_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pPercent,
    /* [out] */ unsigned char __RPC_FAR StageStr[ 2048 ],
    /* [out] */ long __RPC_FAR HeadStatus[ 16 ]);


void __RPC_STUB IDUMain_Archive_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Archive_Abort_Proxy( 
    IDUMain __RPC_FAR * This);


void __RPC_STUB IDUMain_Archive_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Acquisition_Abort_Proxy( 
    IDUMain __RPC_FAR * This);


void __RPC_STUB IDUMain_Acquisition_Abort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_KillSelf_Proxy( 
    IDUMain __RPC_FAR * This);


void __RPC_STUB IDUMain_KillSelf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Save_Head_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pSize,
    /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Save_Head_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Load_Head_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Size,
    /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IDUMain_Load_Head_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IDUMain_Acquisition_Statistics_Proxy( 
    IDUMain __RPC_FAR * This,
    /* [out] */ unsigned long __RPC_FAR *pNumKEvents,
    /* [out] */ unsigned long __RPC_FAR *pBadAddress,
    /* [out] */ unsigned long __RPC_FAR *pSyncProb);


void __RPC_STUB IDUMain_Acquisition_Statistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDUMain_INTERFACE_DEFINED__ */



#ifndef __DUCOMLib_LIBRARY_DEFINED__
#define __DUCOMLib_LIBRARY_DEFINED__

/* library DUCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DUCOMLib;

EXTERN_C const CLSID CLSID_DUMain;

#ifdef __cplusplus

class DECLSPEC_UUID("3FA04976-1CFD-4C2E-A4E7-865D208DCC3E")
DUMain;
#endif
#endif /* __DUCOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
