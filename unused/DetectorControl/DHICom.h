/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 5.01.0164 */
/* at Mon May 01 17:26:34 2006
 */
/* Compiler settings for C:\PVCSWORK\DetectorControl\DHICom.idl:
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

#ifndef __DHICom_h__
#define __DHICom_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAbstract_DHI_FWD_DEFINED__
#define __IAbstract_DHI_FWD_DEFINED__
typedef interface IAbstract_DHI IAbstract_DHI;
#endif 	/* __IAbstract_DHI_FWD_DEFINED__ */


#ifndef __Abstract_DHI_FWD_DEFINED__
#define __Abstract_DHI_FWD_DEFINED__

#ifdef __cplusplus
typedef class Abstract_DHI Abstract_DHI;
#else
typedef struct Abstract_DHI Abstract_DHI;
#endif /* __cplusplus */

#endif 	/* __Abstract_DHI_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __IAbstract_DHI_INTERFACE_DEFINED__
#define __IAbstract_DHI_INTERFACE_DEFINED__

/* interface IAbstract_DHI */
/* [unique][helpstring][uuid][object] */ 

typedef struct  Information
    {
    long Address;
    long Mode;
    long Config;
    long Layer;
    long LLD;
    long ULD;
    long SLD;
    }	HeadInfo;

typedef struct  PhysicalInformation
    {
    long Address;
    long HighVoltage;
    float Plus5Volts;
    float Minus5Volts;
    float OtherVoltage;
    float LTI_Temperature;
    float DHI_Temperature[ 16 ];
    }	PhysicalInfo;


EXTERN_C const IID IID_IAbstract_DHI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("82C49018-554A-4AD8-92E9-25E8849CAD33")
    IAbstract_DHI : public IUnknown
    {
    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Error_Lookup( 
            /* [in] */ long ErrorCode,
            /* [out] */ long __RPC_FAR *Code,
            /* [out] */ unsigned char __RPC_FAR ErrorString[ 2048 ]) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Zap( 
            /* [in] */ long ZapMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Determine_Delay( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Determine_Offsets( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Coincidence_Mode( 
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Time_Mode( 
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE PassThru_Mode( 
            /* [in] */ long __RPC_FAR Head[ 16 ],
            /* [in] */ long __RPC_FAR PtSrc[ 16 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE TagWord_Mode( 
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Test_Mode( 
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Head_Mode( 
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize_Scan( 
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [in] */ long Configuration,
            /* [in] */ long Layer,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Start_Scan( 
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Stop_Scan( 
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Refresh_Analog_Settings( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Analog_Settings( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Analog_Settings( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE File_Upload( 
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
            /* [out] */ long __RPC_FAR *Filesize,
            /* [out] */ unsigned long __RPC_FAR *CheckSum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE File_Download( 
            /* [in] */ long Head,
            /* [in] */ long Filesize,
            /* [in] */ unsigned long CheckSum,
            /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
            /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE High_Voltage( 
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Point_Source( 
            /* [in] */ long Head,
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Point_Source_Status( 
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pOnOff,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Statistics( 
            /* [out] */ long __RPC_FAR Statistics[ 19 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Singles( 
            /* [in] */ long Head,
            /* [in] */ long SinglesType,
            /* [out] */ long __RPC_FAR Singles[ 128 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Progress( 
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Ping( 
            /* [in] */ long Head,
            /* [out] */ unsigned char __RPC_FAR BuildID[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Report_Temperature( 
            /* [out] */ float __RPC_FAR Temperature[ 35 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Report_Voltage( 
            /* [out] */ float __RPC_FAR Voltage[ 64 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Report_HRRT_High_Voltage( 
            /* [out] */ long __RPC_FAR Voltage[ 80 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Hardware_Configuration( 
            /* [out] */ long __RPC_FAR *Filesize,
            /* [out] */ unsigned long __RPC_FAR *CheckSum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Check_Head_Mode( 
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pDataMode,
            /* [out] */ long __RPC_FAR *pConfiguration,
            /* [out] */ long __RPC_FAR *pLayer,
            /* [out] */ long __RPC_FAR *pLLD,
            /* [out] */ long __RPC_FAR *pULD,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Check_CP_Mode( 
            /* [out] */ long __RPC_FAR *pMode,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Temperature_Limits( 
            /* [in] */ float Low,
            /* [in] */ float High,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reset_CP( 
            /* [in] */ long Reset,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Reset_Head( 
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Load_RAM( 
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long RAM_Type,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Time_Window( 
            /* [in] */ long WindowSize,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE OS_Command( 
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Pass_Command( 
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
            /* [out] */ unsigned char __RPC_FAR ResponseStr[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Tag_Insert( 
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *Tagword,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Tag_Control( 
            /* [in] */ long SinglesTag_OffOn,
            /* [in] */ long TimeTag_OffOn,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Transmission_Trajectory( 
            /* [in] */ long Transaxial_Speed,
            /* [in] */ long Axial_Step,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Diagnostics( 
            /* [out] */ long __RPC_FAR *pResult,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Force_Reload( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Health_Check( 
            /* [out] */ long __RPC_FAR *pNumFailed,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Failed,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE DataStream( 
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Initialize_PET( 
            /* [in] */ long ScanType,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Head_Status( 
            /* [out] */ long __RPC_FAR *pNumHeads,
            /* [size_is][size_is][out] */ HeadInfo __RPC_FAR *__RPC_FAR *Info,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE CountRate( 
            /* [out] */ long __RPC_FAR *pCorrectedSingles,
            /* [out] */ long __RPC_FAR *pUncorrectedSingles,
            /* [out] */ long __RPC_FAR *pPrompts,
            /* [out] */ long __RPC_FAR *pRandoms,
            /* [out] */ long __RPC_FAR *pScatters,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Ring_Singles( 
            /* [out] */ long __RPC_FAR *pNumRings,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *RingSingles,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE KillSelf( void) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetLog( 
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Report_Temperature_Voltage( 
            /* [out] */ float __RPC_FAR *pCP_Temperature,
            /* [out] */ float __RPC_FAR *pMinimum_Temperature,
            /* [out] */ float __RPC_FAR *pMaximum_Temperature,
            /* [out] */ long __RPC_FAR *pNumber_DHI_Temperatures,
            /* [out] */ long __RPC_FAR *pNumHeads,
            /* [size_is][size_is][out] */ PhysicalInfo __RPC_FAR *__RPC_FAR *Info,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Block_Crystal_Position( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Peaks[ 512 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Block_Crystal_Peaks( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Peaks[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Get_Block_Time_Correction( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Correction[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Block_Time_Correction( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Correction[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Block_Crystal_Peaks( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Peaks[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Set_Block_Crystal_Position( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Peaks[ 512 ],
            /* [in] */ long StdDev,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Build_CRM( 
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus) = 0;
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE Last_Async_Message( 
            /* [out] */ unsigned char __RPC_FAR LastMsg[ 2048 ]) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAbstract_DHIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAbstract_DHI __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAbstract_DHI __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Error_Lookup )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long ErrorCode,
            /* [out] */ long __RPC_FAR *Code,
            /* [out] */ unsigned char __RPC_FAR ErrorString[ 2048 ]);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Zap )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long ZapMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Determine_Delay )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Determine_Offsets )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Coincidence_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Time_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PassThru_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long __RPC_FAR Head[ 16 ],
            /* [in] */ long __RPC_FAR PtSrc[ 16 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *TagWord_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Test_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Head_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long DataMode,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long Layer,
            /* [in] */ long Block,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize_Scan )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Transmission,
            /* [in] */ long ModulePairs,
            /* [in] */ long Configuration,
            /* [in] */ long Layer,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start_Scan )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop_Scan )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Refresh_Analog_Settings )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Analog_Settings )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Analog_Settings )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long __RPC_FAR Settings[ 4096 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *File_Upload )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
            /* [out] */ long __RPC_FAR *Filesize,
            /* [out] */ unsigned long __RPC_FAR *CheckSum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *File_Download )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Filesize,
            /* [in] */ unsigned long CheckSum,
            /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
            /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *High_Voltage )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Point_Source )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Point_Source_Status )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pOnOff,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Statistics )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR Statistics[ 19 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Singles )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long SinglesType,
            /* [out] */ long __RPC_FAR Singles[ 128 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Progress )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pPercent,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Ping )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ unsigned char __RPC_FAR BuildID[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Report_Temperature )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ float __RPC_FAR Temperature[ 35 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Report_Voltage )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ float __RPC_FAR Voltage[ 64 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Report_HRRT_High_Voltage )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR Voltage[ 80 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Hardware_Configuration )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *Filesize,
            /* [out] */ unsigned long __RPC_FAR *CheckSum,
            /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Check_Head_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pDataMode,
            /* [out] */ long __RPC_FAR *pConfiguration,
            /* [out] */ long __RPC_FAR *pLayer,
            /* [out] */ long __RPC_FAR *pLLD,
            /* [out] */ long __RPC_FAR *pULD,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Check_CP_Mode )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pMode,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Temperature_Limits )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ float Low,
            /* [in] */ float High,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset_CP )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Reset,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset_Head )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load_RAM )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Configuration,
            /* [in] */ long Head,
            /* [in] */ long RAM_Type,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Time_Window )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long WindowSize,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OS_Command )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Pass_Command )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
            /* [out] */ unsigned char __RPC_FAR ResponseStr[ 2048 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Tag_Insert )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Size,
            /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *Tagword,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Tag_Control )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long SinglesTag_OffOn,
            /* [in] */ long TimeTag_OffOn,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Transmission_Trajectory )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Transaxial_Speed,
            /* [in] */ long Axial_Step,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Diagnostics )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pResult,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Force_Reload )( 
            IAbstract_DHI __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Health_Check )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pNumFailed,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Failed,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DataStream )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize_PET )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long ScanType,
            /* [in] */ long LLD,
            /* [in] */ long ULD,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Head_Status )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pNumHeads,
            /* [size_is][size_is][out] */ HeadInfo __RPC_FAR *__RPC_FAR *Info,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CountRate )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pCorrectedSingles,
            /* [out] */ long __RPC_FAR *pUncorrectedSingles,
            /* [out] */ long __RPC_FAR *pPrompts,
            /* [out] */ long __RPC_FAR *pRandoms,
            /* [out] */ long __RPC_FAR *pScatters,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Ring_Singles )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ long __RPC_FAR *pNumRings,
            /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *RingSingles,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *KillSelf )( 
            IAbstract_DHI __RPC_FAR * This);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLog )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long OnOff,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Report_Temperature_Voltage )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ float __RPC_FAR *pCP_Temperature,
            /* [out] */ float __RPC_FAR *pMinimum_Temperature,
            /* [out] */ float __RPC_FAR *pMaximum_Temperature,
            /* [out] */ long __RPC_FAR *pNumber_DHI_Temperatures,
            /* [out] */ long __RPC_FAR *pNumHeads,
            /* [size_is][size_is][out] */ PhysicalInfo __RPC_FAR *__RPC_FAR *Info,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Block_Crystal_Position )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Peaks[ 512 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Block_Crystal_Peaks )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Peaks[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Get_Block_Time_Correction )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR Correction[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Block_Time_Correction )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Correction[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Block_Crystal_Peaks )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Peaks[ 256 ],
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Set_Block_Crystal_Position )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [in] */ long __RPC_FAR Peaks[ 512 ],
            /* [in] */ long StdDev,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Build_CRM )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [in] */ long Head,
            /* [in] */ long Block,
            /* [out] */ long __RPC_FAR *pStatus);
        
        /* [helpstring] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Last_Async_Message )( 
            IAbstract_DHI __RPC_FAR * This,
            /* [out] */ unsigned char __RPC_FAR LastMsg[ 2048 ]);
        
        END_INTERFACE
    } IAbstract_DHIVtbl;

    interface IAbstract_DHI
    {
        CONST_VTBL struct IAbstract_DHIVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAbstract_DHI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAbstract_DHI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAbstract_DHI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAbstract_DHI_Error_Lookup(This,ErrorCode,Code,ErrorString)	\
    (This)->lpVtbl -> Error_Lookup(This,ErrorCode,Code,ErrorString)

#define IAbstract_DHI_Zap(This,ZapMode,Configuration,Head,Block,pStatus)	\
    (This)->lpVtbl -> Zap(This,ZapMode,Configuration,Head,Block,pStatus)

#define IAbstract_DHI_Determine_Delay(This,Configuration,Head,Block,pStatus)	\
    (This)->lpVtbl -> Determine_Delay(This,Configuration,Head,Block,pStatus)

#define IAbstract_DHI_Determine_Offsets(This,Configuration,Head,Block,pStatus)	\
    (This)->lpVtbl -> Determine_Offsets(This,Configuration,Head,Block,pStatus)

#define IAbstract_DHI_Coincidence_Mode(This,Transmission,ModulePairs,pStatus)	\
    (This)->lpVtbl -> Coincidence_Mode(This,Transmission,ModulePairs,pStatus)

#define IAbstract_DHI_Time_Mode(This,Transmission,ModulePairs,pStatus)	\
    (This)->lpVtbl -> Time_Mode(This,Transmission,ModulePairs,pStatus)

#define IAbstract_DHI_PassThru_Mode(This,Head,PtSrc,pStatus)	\
    (This)->lpVtbl -> PassThru_Mode(This,Head,PtSrc,pStatus)

#define IAbstract_DHI_TagWord_Mode(This,pStatus)	\
    (This)->lpVtbl -> TagWord_Mode(This,pStatus)

#define IAbstract_DHI_Test_Mode(This,pStatus)	\
    (This)->lpVtbl -> Test_Mode(This,pStatus)

#define IAbstract_DHI_Set_Head_Mode(This,DataMode,Configuration,Head,Layer,Block,LLD,ULD,pStatus)	\
    (This)->lpVtbl -> Set_Head_Mode(This,DataMode,Configuration,Head,Layer,Block,LLD,ULD,pStatus)

#define IAbstract_DHI_Initialize_Scan(This,Transmission,ModulePairs,Configuration,Layer,LLD,ULD,pStatus)	\
    (This)->lpVtbl -> Initialize_Scan(This,Transmission,ModulePairs,Configuration,Layer,LLD,ULD,pStatus)

#define IAbstract_DHI_Start_Scan(This,pStatus)	\
    (This)->lpVtbl -> Start_Scan(This,pStatus)

#define IAbstract_DHI_Stop_Scan(This,pStatus)	\
    (This)->lpVtbl -> Stop_Scan(This,pStatus)

#define IAbstract_DHI_Refresh_Analog_Settings(This,Configuration,Head,pStatus)	\
    (This)->lpVtbl -> Refresh_Analog_Settings(This,Configuration,Head,pStatus)

#define IAbstract_DHI_Get_Analog_Settings(This,Configuration,Head,Settings,pStatus)	\
    (This)->lpVtbl -> Get_Analog_Settings(This,Configuration,Head,Settings,pStatus)

#define IAbstract_DHI_Set_Analog_Settings(This,Configuration,Head,Settings,pStatus)	\
    (This)->lpVtbl -> Set_Analog_Settings(This,Configuration,Head,Settings,pStatus)

#define IAbstract_DHI_File_Upload(This,Head,Filename,Filesize,CheckSum,buffer,pStatus)	\
    (This)->lpVtbl -> File_Upload(This,Head,Filename,Filesize,CheckSum,buffer,pStatus)

#define IAbstract_DHI_File_Download(This,Head,Filesize,CheckSum,Filename,buffer,pStatus)	\
    (This)->lpVtbl -> File_Download(This,Head,Filesize,CheckSum,Filename,buffer,pStatus)

#define IAbstract_DHI_High_Voltage(This,OnOff,pStatus)	\
    (This)->lpVtbl -> High_Voltage(This,OnOff,pStatus)

#define IAbstract_DHI_Point_Source(This,Head,OnOff,pStatus)	\
    (This)->lpVtbl -> Point_Source(This,Head,OnOff,pStatus)

#define IAbstract_DHI_Point_Source_Status(This,Head,pOnOff,pStatus)	\
    (This)->lpVtbl -> Point_Source_Status(This,Head,pOnOff,pStatus)

#define IAbstract_DHI_Get_Statistics(This,Statistics,pStatus)	\
    (This)->lpVtbl -> Get_Statistics(This,Statistics,pStatus)

#define IAbstract_DHI_Singles(This,Head,SinglesType,Singles,pStatus)	\
    (This)->lpVtbl -> Singles(This,Head,SinglesType,Singles,pStatus)

#define IAbstract_DHI_Progress(This,Head,pPercent,pStatus)	\
    (This)->lpVtbl -> Progress(This,Head,pPercent,pStatus)

#define IAbstract_DHI_Ping(This,Head,BuildID,pStatus)	\
    (This)->lpVtbl -> Ping(This,Head,BuildID,pStatus)

#define IAbstract_DHI_Report_Temperature(This,Temperature,pStatus)	\
    (This)->lpVtbl -> Report_Temperature(This,Temperature,pStatus)

#define IAbstract_DHI_Report_Voltage(This,Voltage,pStatus)	\
    (This)->lpVtbl -> Report_Voltage(This,Voltage,pStatus)

#define IAbstract_DHI_Report_HRRT_High_Voltage(This,Voltage,pStatus)	\
    (This)->lpVtbl -> Report_HRRT_High_Voltage(This,Voltage,pStatus)

#define IAbstract_DHI_Hardware_Configuration(This,Filesize,CheckSum,buffer,pStatus)	\
    (This)->lpVtbl -> Hardware_Configuration(This,Filesize,CheckSum,buffer,pStatus)

#define IAbstract_DHI_Check_Head_Mode(This,Head,pDataMode,pConfiguration,pLayer,pLLD,pULD,pStatus)	\
    (This)->lpVtbl -> Check_Head_Mode(This,Head,pDataMode,pConfiguration,pLayer,pLLD,pULD,pStatus)

#define IAbstract_DHI_Check_CP_Mode(This,pMode,pStatus)	\
    (This)->lpVtbl -> Check_CP_Mode(This,pMode,pStatus)

#define IAbstract_DHI_Set_Temperature_Limits(This,Low,High,pStatus)	\
    (This)->lpVtbl -> Set_Temperature_Limits(This,Low,High,pStatus)

#define IAbstract_DHI_Reset_CP(This,Reset,pStatus)	\
    (This)->lpVtbl -> Reset_CP(This,Reset,pStatus)

#define IAbstract_DHI_Reset_Head(This,Head,pStatus)	\
    (This)->lpVtbl -> Reset_Head(This,Head,pStatus)

#define IAbstract_DHI_Load_RAM(This,Configuration,Head,RAM_Type,pStatus)	\
    (This)->lpVtbl -> Load_RAM(This,Configuration,Head,RAM_Type,pStatus)

#define IAbstract_DHI_Time_Window(This,WindowSize,pStatus)	\
    (This)->lpVtbl -> Time_Window(This,WindowSize,pStatus)

#define IAbstract_DHI_OS_Command(This,Head,CommandString,pStatus)	\
    (This)->lpVtbl -> OS_Command(This,Head,CommandString,pStatus)

#define IAbstract_DHI_Pass_Command(This,Head,CommandString,ResponseStr,pStatus)	\
    (This)->lpVtbl -> Pass_Command(This,Head,CommandString,ResponseStr,pStatus)

#define IAbstract_DHI_Tag_Insert(This,Size,Tagword,pStatus)	\
    (This)->lpVtbl -> Tag_Insert(This,Size,Tagword,pStatus)

#define IAbstract_DHI_Tag_Control(This,SinglesTag_OffOn,TimeTag_OffOn,pStatus)	\
    (This)->lpVtbl -> Tag_Control(This,SinglesTag_OffOn,TimeTag_OffOn,pStatus)

#define IAbstract_DHI_Transmission_Trajectory(This,Transaxial_Speed,Axial_Step,pStatus)	\
    (This)->lpVtbl -> Transmission_Trajectory(This,Transaxial_Speed,Axial_Step,pStatus)

#define IAbstract_DHI_Diagnostics(This,pResult,pStatus)	\
    (This)->lpVtbl -> Diagnostics(This,pResult,pStatus)

#define IAbstract_DHI_Force_Reload(This)	\
    (This)->lpVtbl -> Force_Reload(This)

#define IAbstract_DHI_Health_Check(This,pNumFailed,Failed,pStatus)	\
    (This)->lpVtbl -> Health_Check(This,pNumFailed,Failed,pStatus)

#define IAbstract_DHI_DataStream(This,OnOff,pStatus)	\
    (This)->lpVtbl -> DataStream(This,OnOff,pStatus)

#define IAbstract_DHI_Initialize_PET(This,ScanType,LLD,ULD,pStatus)	\
    (This)->lpVtbl -> Initialize_PET(This,ScanType,LLD,ULD,pStatus)

#define IAbstract_DHI_Head_Status(This,pNumHeads,Info,pStatus)	\
    (This)->lpVtbl -> Head_Status(This,pNumHeads,Info,pStatus)

#define IAbstract_DHI_CountRate(This,pCorrectedSingles,pUncorrectedSingles,pPrompts,pRandoms,pScatters,pStatus)	\
    (This)->lpVtbl -> CountRate(This,pCorrectedSingles,pUncorrectedSingles,pPrompts,pRandoms,pScatters,pStatus)

#define IAbstract_DHI_Ring_Singles(This,pNumRings,RingSingles,pStatus)	\
    (This)->lpVtbl -> Ring_Singles(This,pNumRings,RingSingles,pStatus)

#define IAbstract_DHI_KillSelf(This)	\
    (This)->lpVtbl -> KillSelf(This)

#define IAbstract_DHI_SetLog(This,OnOff,pStatus)	\
    (This)->lpVtbl -> SetLog(This,OnOff,pStatus)

#define IAbstract_DHI_Report_Temperature_Voltage(This,pCP_Temperature,pMinimum_Temperature,pMaximum_Temperature,pNumber_DHI_Temperatures,pNumHeads,Info,pStatus)	\
    (This)->lpVtbl -> Report_Temperature_Voltage(This,pCP_Temperature,pMinimum_Temperature,pMaximum_Temperature,pNumber_DHI_Temperatures,pNumHeads,Info,pStatus)

#define IAbstract_DHI_Get_Block_Crystal_Position(This,Head,Block,Peaks,pStatus)	\
    (This)->lpVtbl -> Get_Block_Crystal_Position(This,Head,Block,Peaks,pStatus)

#define IAbstract_DHI_Get_Block_Crystal_Peaks(This,Head,Block,Peaks,pStatus)	\
    (This)->lpVtbl -> Get_Block_Crystal_Peaks(This,Head,Block,Peaks,pStatus)

#define IAbstract_DHI_Get_Block_Time_Correction(This,Head,Block,Correction,pStatus)	\
    (This)->lpVtbl -> Get_Block_Time_Correction(This,Head,Block,Correction,pStatus)

#define IAbstract_DHI_Set_Block_Time_Correction(This,Head,Block,Correction,pStatus)	\
    (This)->lpVtbl -> Set_Block_Time_Correction(This,Head,Block,Correction,pStatus)

#define IAbstract_DHI_Set_Block_Crystal_Peaks(This,Head,Block,Peaks,pStatus)	\
    (This)->lpVtbl -> Set_Block_Crystal_Peaks(This,Head,Block,Peaks,pStatus)

#define IAbstract_DHI_Set_Block_Crystal_Position(This,Head,Block,Peaks,StdDev,pStatus)	\
    (This)->lpVtbl -> Set_Block_Crystal_Position(This,Head,Block,Peaks,StdDev,pStatus)

#define IAbstract_DHI_Build_CRM(This,Head,Block,pStatus)	\
    (This)->lpVtbl -> Build_CRM(This,Head,Block,pStatus)

#define IAbstract_DHI_Last_Async_Message(This,LastMsg)	\
    (This)->lpVtbl -> Last_Async_Message(This,LastMsg)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Error_Lookup_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long ErrorCode,
    /* [out] */ long __RPC_FAR *Code,
    /* [out] */ unsigned char __RPC_FAR ErrorString[ 2048 ]);


void __RPC_STUB IAbstract_DHI_Error_Lookup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Zap_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long ZapMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Zap_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Determine_Delay_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Determine_Delay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Determine_Offsets_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Determine_Offsets_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Coincidence_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Transmission,
    /* [in] */ long ModulePairs,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Coincidence_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Time_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Transmission,
    /* [in] */ long ModulePairs,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Time_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_PassThru_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long __RPC_FAR Head[ 16 ],
    /* [in] */ long __RPC_FAR PtSrc[ 16 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_PassThru_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_TagWord_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_TagWord_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Test_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Test_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Head_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long DataMode,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long Layer,
    /* [in] */ long Block,
    /* [in] */ long LLD,
    /* [in] */ long ULD,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Head_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Initialize_Scan_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Transmission,
    /* [in] */ long ModulePairs,
    /* [in] */ long Configuration,
    /* [in] */ long Layer,
    /* [in] */ long LLD,
    /* [in] */ long ULD,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Initialize_Scan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Start_Scan_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Start_Scan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Stop_Scan_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Stop_Scan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Refresh_Analog_Settings_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Refresh_Analog_Settings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Analog_Settings_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR Settings[ 4096 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Get_Analog_Settings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Analog_Settings_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long __RPC_FAR Settings[ 4096 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Analog_Settings_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_File_Upload_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
    /* [out] */ long __RPC_FAR *Filesize,
    /* [out] */ unsigned long __RPC_FAR *CheckSum,
    /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_File_Upload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_File_Download_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Filesize,
    /* [in] */ unsigned long CheckSum,
    /* [in] */ unsigned char __RPC_FAR Filename[ 2048 ],
    /* [size_is][size_is][in] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_File_Download_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_High_Voltage_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_High_Voltage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Point_Source_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Point_Source_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Point_Source_Status_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pOnOff,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Point_Source_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Statistics_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR Statistics[ 19 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Get_Statistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Singles_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long SinglesType,
    /* [out] */ long __RPC_FAR Singles[ 128 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Singles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Progress_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pPercent,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Progress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Ping_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ unsigned char __RPC_FAR BuildID[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Ping_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Report_Temperature_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ float __RPC_FAR Temperature[ 35 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Report_Temperature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Report_Voltage_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ float __RPC_FAR Voltage[ 64 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Report_Voltage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Report_HRRT_High_Voltage_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR Voltage[ 80 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Report_HRRT_High_Voltage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Hardware_Configuration_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *Filesize,
    /* [out] */ unsigned long __RPC_FAR *CheckSum,
    /* [size_is][size_is][out] */ unsigned char __RPC_FAR *__RPC_FAR *buffer,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Hardware_Configuration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Check_Head_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pDataMode,
    /* [out] */ long __RPC_FAR *pConfiguration,
    /* [out] */ long __RPC_FAR *pLayer,
    /* [out] */ long __RPC_FAR *pLLD,
    /* [out] */ long __RPC_FAR *pULD,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Check_Head_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Check_CP_Mode_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pMode,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Check_CP_Mode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Temperature_Limits_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ float Low,
    /* [in] */ float High,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Temperature_Limits_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Reset_CP_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Reset,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Reset_CP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Reset_Head_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Reset_Head_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Load_RAM_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Configuration,
    /* [in] */ long Head,
    /* [in] */ long RAM_Type,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Load_RAM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Time_Window_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long WindowSize,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Time_Window_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_OS_Command_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_OS_Command_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Pass_Command_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ unsigned char __RPC_FAR CommandString[ 2048 ],
    /* [out] */ unsigned char __RPC_FAR ResponseStr[ 2048 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Pass_Command_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Tag_Insert_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Size,
    /* [size_is][size_is][in] */ MIDL_uhyper __RPC_FAR *__RPC_FAR *Tagword,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Tag_Insert_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Tag_Control_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long SinglesTag_OffOn,
    /* [in] */ long TimeTag_OffOn,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Tag_Control_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Transmission_Trajectory_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Transaxial_Speed,
    /* [in] */ long Axial_Step,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Transmission_Trajectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Diagnostics_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pResult,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Diagnostics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Force_Reload_Proxy( 
    IAbstract_DHI __RPC_FAR * This);


void __RPC_STUB IAbstract_DHI_Force_Reload_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Health_Check_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumFailed,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *Failed,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Health_Check_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_DataStream_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_DataStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Initialize_PET_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long ScanType,
    /* [in] */ long LLD,
    /* [in] */ long ULD,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Initialize_PET_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Head_Status_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumHeads,
    /* [size_is][size_is][out] */ HeadInfo __RPC_FAR *__RPC_FAR *Info,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Head_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_CountRate_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pCorrectedSingles,
    /* [out] */ long __RPC_FAR *pUncorrectedSingles,
    /* [out] */ long __RPC_FAR *pPrompts,
    /* [out] */ long __RPC_FAR *pRandoms,
    /* [out] */ long __RPC_FAR *pScatters,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_CountRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Ring_Singles_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ long __RPC_FAR *pNumRings,
    /* [size_is][size_is][out] */ long __RPC_FAR *__RPC_FAR *RingSingles,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Ring_Singles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_KillSelf_Proxy( 
    IAbstract_DHI __RPC_FAR * This);


void __RPC_STUB IAbstract_DHI_KillSelf_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_SetLog_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long OnOff,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_SetLog_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Report_Temperature_Voltage_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ float __RPC_FAR *pCP_Temperature,
    /* [out] */ float __RPC_FAR *pMinimum_Temperature,
    /* [out] */ float __RPC_FAR *pMaximum_Temperature,
    /* [out] */ long __RPC_FAR *pNumber_DHI_Temperatures,
    /* [out] */ long __RPC_FAR *pNumHeads,
    /* [size_is][size_is][out] */ PhysicalInfo __RPC_FAR *__RPC_FAR *Info,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Report_Temperature_Voltage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Crystal_Position_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Peaks[ 512 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Get_Block_Crystal_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Crystal_Peaks_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Peaks[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Get_Block_Crystal_Peaks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Get_Block_Time_Correction_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR Correction[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Get_Block_Time_Correction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Time_Correction_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Correction[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Block_Time_Correction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Crystal_Peaks_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Peaks[ 256 ],
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Block_Crystal_Peaks_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Set_Block_Crystal_Position_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [in] */ long __RPC_FAR Peaks[ 512 ],
    /* [in] */ long StdDev,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Set_Block_Crystal_Position_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Build_CRM_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [in] */ long Head,
    /* [in] */ long Block,
    /* [out] */ long __RPC_FAR *pStatus);


void __RPC_STUB IAbstract_DHI_Build_CRM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring] */ HRESULT STDMETHODCALLTYPE IAbstract_DHI_Last_Async_Message_Proxy( 
    IAbstract_DHI __RPC_FAR * This,
    /* [out] */ unsigned char __RPC_FAR LastMsg[ 2048 ]);


void __RPC_STUB IAbstract_DHI_Last_Async_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAbstract_DHI_INTERFACE_DEFINED__ */



#ifndef __DHICOMLib_LIBRARY_DEFINED__
#define __DHICOMLib_LIBRARY_DEFINED__

/* library DHICOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DHICOMLib;

EXTERN_C const CLSID CLSID_Abstract_DHI;

#ifdef __cplusplus

class DECLSPEC_UUID("96686904-6ECB-46AC-B4F2-F839C61C1C7A")
Abstract_DHI;
#endif
#endif /* __DHICOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
