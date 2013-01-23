/********************************************************************
/
/ PftDrv.h - Include for CTI's Petlink Fibre Translator driver
/
/	Created : 2feb01, J.H. Reed
/
/*******************************************************************/

// include general bit defines
#include "bits.h"

// Make life a little simpler
#define LOCAL static

// Various constants
#define	PFT_NT_DEVICE_NAME		L"\\Device\\Pft0"
#define	PFT_WIN32_DEVICE_NAME	L"\\DosDevices\\PFT1"
#define	PFT_DRIVER_NAME			L"FibreAdapters"
#define PFT_SUBKEY				L"Parameters\\Device0"

// Common Buffer
#define CB_SIZE 0x400000

// Common Buffer Status
#define FREE	0
#define ACQING	1
#define ACQD	2
#define PENDING 3

// PFT Local bus FIFO address
#define PFT_LB_FA	0x1000000

typedef struct _CB
{
	PHYSICAL_ADDRESS pa_cb ;		// Physical address of common buffer
	PVOID la_cb ;					// Logical address of common buffer
	int state ;						// State of buffer
	unsigned long events ;			// Number of events in buffer
	int status ;					// status of acquisition
} CB, *PCB ;

// Devivce Extension for the PFT card
typedef struct PFT_DEVICE_EXTENSION 
{
	PDEVICE_OBJECT PhysicalDeviceObject;	// Back pointer to physical device object
	PDEVICE_OBJECT FunctionalDeviceObject;	// Functional device object
	PDEVICE_OBJECT LowerDeviceObject;		// Lower device object to send IRP's to
	PVOID PlxAddress ;						// Base address register 0's mapped memory address
	ULONG PlxLength ;						// Length of the Plx Address space
	PVOID PftAddress ;						// Base address register 2's mapped memory address
	ULONG PftLength ;						// Length of the Plx Address space
	KSPIN_LOCK IsrSpinLock ;				// Spin Lock used to sync with the ISR
	PKINTERRUPT InterruptObject ;			// System allocated interrupt object
	ULONG IRQL ;							// Interrupt reqest level
	ULONG Vector ;							// Interrupt request vector
	ULONG Affinity ;						// Interrupt processor affinity
	PADAPTER_OBJECT AdapterObject ;			// Adapter Object for DMA
	PMDL Mdl;								// Memory descriptor list
	ULONG cb_len ;							// Common buffer length
	CB cb0 ;								// Common Buffer 0
	CB cb1 ;								// Common Buffer 1
	PCB CurrentBuffer ;						// Holds pointer to currently acquireing buffer
	PCB NextBuffer ;						// Holds Pointer to buffer to be used for next acquire
	PCB StoreBuffer ;						// Holds Pointer to buffer to be stored by application
	PKEVENT Event ;							// The named event object pointer
	HANDLE EventHandle ;					// Handle of named event to notify application buffer is ready
	PVOID UserSpace ;						// Mapped user address of kernel mode buffer
	PFILE_OBJECT FileObject ;				// Pointer to IRP FileObject that requested the mapped memory
	KTIMER StatsTimer ;						// Timer to signal update of statistics information
	KDPC StatsDpc ;							// DPC object for the statistics update
	LARGE_INTEGER ScanStartTime ;			// Timer for computing event rate
	LARGE_INTEGER ScanStopTime ;			// Holds the scan stop time
	LARGE_INTEGER ElapsedTime ;				// Holds the elpased scan time
	ULONG LastDmaCounter ;					// A count holder needed to compute total counts
	__int64 TotalEvents ;					// Running total counts during a scan
	ULONG EventRate ;						// Instantaneous computed event rate
	ULONG LastAddCounts ;					// Last delta count for spike prevention
	LMSTATS lmstats ;						// Storage for listmode stats for each scan
	int acqstatus ;							// Listmode acquisition status
	int DmaUnderFlowCount ;					// Keep up with the time the DMA counter fails
	int DmaOverFlowCount ;					// Keep up with the time the DMA counter fails
	int MaxUnderFlow ;						// Maximum number of events DMA counter underflowed by
	int MaxOverFlow ;						// Maximum number of events DMA counter overflowed by
	int NDmaErrors ;						// Total number of errors
	int NBadEvents ;						// Cumulative number of bad events
	BOOLEAN SendEvent ;						// Flag for DPC routine to send the sync event
} PFT_DEVICE_EXTENSION, *PPFT_DEVICE_EXTENSION ;

// PLX 9080 Local Configuration Registers
typedef struct {
	ULONG las0rr ;						// offset 00H - Local Address Space 0 Range Register
	ULONG las0ba ;						// offset 04H - Local Address Space 0 Local Base Address
	ULONG marbr ;						// offset 08H - Model Arbitration Register
	ULONG bigend ;						// offset 0CH - Big/Little Endian Descriptor
	ULONG eromrr ;						// offset 10H - Expansion ROM Range Register
	ULONG eromba ;						// offset 14H - Expansion Rom Local Base Address
	ULONG lrbrd0 ;						// offset 18H - Local Address Space 0/Expansion ROM Bus Region Register
	ULONG dmrr ;						// offset 1CH - Local Range Register for Direct Master to PCI
	ULONG dmlbam ;						// offset 20H - Local Buss Base Address Register for Direct Master to PCI Memory
	ULONG dmlbai ;						// offset 24H - Local Base Address Register for Direct Master to PCI IO/CFG
	ULONG dmpbam ;						// offset 28H - PCI Base Address (Remap) Register for Direct Master to PCI Memory
	ULONG dmcfga ;						// offset 2CH - PCI Configuration Register for Direct Master to PCI IO/CFG
	ULONG oplfis ;						// offset 30H - Outbound Post List FIFO Interrupt Status Register
	ULONG oplfim ;						// offset 34H - Outbound Post List FIFO Interrupt Mask Register
	ULONG rsvd1 ;						// offset 38H -	Reserved 1
	ULONG rsvd2 ;						// offset 3CH - Reserved 2
	ULONG mbox0 ;						// offset 40H - Mailbox Register 0
	ULONG mbox1 ;						// offset 44H - Mailbox Register 1
	ULONG mbox2 ;						// offset 48H - Mailbox Register 2
	ULONG mbox3 ;						// offset 4CH - Mailbox Register 3
	ULONG mbox4 ;						// offset 50H - Mailbox Register 4
	ULONG mbox5 ;						// offset 54H - Mailbox Register 5
	ULONG mbox6 ;						// offset 58H - Mailbox Register 6
	ULONG mbox7 ;						// offset 5CH - Mailbox Register 7
	ULONG p2ldbell ;					// offset 60H - PCI-to-Local Doorbell Register
	ULONG l2pdbell ;					// offset 64H - Local-to-PCI Dorrbell Register
	ULONG intcsr ;						// offset 68H - Interrupt Control/Status Register
	ULONG cntrl ;						// offset 6CH - Serial EEPROM Control, PCI Command Codes, User IO, Init
	ULONG pcihidr ;						// offset 70H - PCI Permanent Configuration ID Register
	ULONG pcihrev ;						// offset 74H - PCI Permanent Revision ID Register
	ULONG rsvd3 ;						// offset 78H - Reserved 3
	ULONG rsvd4 ;						// offset 7CH - Reserved 4
	ULONG dmamode0 ;					// offset 80H - DMA Channel 0 Mode Register
	ULONG dmapadr0 ;					// offset 84H - DMA Channel 0 PCI Address Register
	ULONG dmaladr0 ;					// offset 88H - DMA Channel 0 Local Address Register
	ULONG dmasiz0 ;						// offset 8CH - DMA Channel 0 Transfer Size (bytes) Register
	ULONG dmadpr0 ;						// offset 90H - DMA Channel 0 Descriptor Pointer Register
	ULONG dmamode1 ;					// offset 94H - DMA Channel 1 Mode Register
	ULONG dmapadr1 ;					// offset 98H - DMA Channel 0 PCI Address Register
	ULONG dmaladr1 ;					// offset 9CH - DMA Channel 0 Local Address Register
	ULONG dmasiz1 ;						// offset A0H - DMA Channel 0 Transfer Size (bytes) Register
	ULONG dmadpr1 ;						// offset A4H - DMA Channel 0 Descriptor Pointer Register
	ULONG dmacsr ;						// offset A8H - DMA Command Status Register (A8 = 0, A9 = 1)
	ULONG dmaarb ;						// offset ACH - DMA Arbitration Register
	ULONG dmathr ;						// offset B0H - DMA Threshold Register
	ULONG rsvd5 ;						// offset B4H - Reserved 5
	ULONG rsvd6 ;						// offset B8H - Reserved 6
	ULONG rsvd7 ;						// offset BCH - Reserved 7
	ULONG mqcr ;						// offset C0H - Messaging Queue Configuration Register
	ULONG qbar ;						// offset C4H - Queue Base Address Register
	ULONG ifhpr ;						// offset C8H - Inbound Free Header Pointer Register
	ULONG iftpr ;						// offset CCH - Inbound Free Tail Pointer Register
	ULONG iphpr ;						// offset D0H - Inbound Post Head Pointer Register
	ULONG iptpr ;						// offset D4H - Inbound Post Tail Pointer Register
	ULONG ofhpr ;						// offset D8H - Outbound Free Head Pointer Register
	ULONG oftpr ;						// offset DCH - Outbound Free Tail Pointer Register
	ULONG ophpr ;						// offset E0H - Outbound Post Head Pointer Register
	ULONG optpr ;						// offset E4H - Outbound Post Tail Pointer Register
	ULONG qsr ;							// offset E8H - Que Status/Control Register
	ULONG las1rr ;						// offset F0H - Local Address Space 1 Range Register
	ULONG las1ba ;						// offset F4H - Local Address Space 1 Local Base Address
	ULONG lbrd1 ;						// offset F8H - Local Address 1 Bus Region Descriptor Register
} PLX_LC_REGS, *PPLX_LC_REGS;

// PFT registers
typedef struct {
	ULONG isppr ;						// ISP Programming Register
	ULONG csr ;							// Control Register
	ULONG esr ;							// Exception Status Register
	ULONG esrtcr ;						// Exception Status Read To Clear Register
	ULONG imr ;							// Interrupt Mask Register
	ULONG dmaoqbcr ;					// DMA Output QuadByte Counter Register
	ULONG dmaiqbcr ;					// DMA Input QuadByte Counter Register
	ULONG wi32bpr ;						// Word Insertion 32 Bit Packet Register
	ULONG wif32of64bpr ;				// Word Insertion First 32 of 64 Bit Packet Register
	ULONG wis32of64bpr ;				// Word Insertion Second 32 of 64 Bit Packet Register
	ULONG fcrntlir ;					// Fibre Channel Receiver Next-to-Last Input Register
	ULONG fcrlir ;						// Fibre Channel Receiver Last Input Register
} PFT_FC_REGS, *PPFT_FC_REGS ;

// PLX DMA CSR bit definitions
#define DMA_CHN0_ENA		BIT0
#define DMA_CHN0_START		BIT1
#define DMA_CHN0_ABORT		BIT2
#define DMA_CHN0_CLR_INT	BIT3
#define DMA_CHN0_DONE		BIT4
#define DMA_CHN1_ENA		BIT8
#define DMA_CHN1_START		BIT9
#define DMA_CHN1_ABORT		BIT10
#define DMA_CHN1_CLR_INT	BIT11
#define DMA_CHN1_DONE		BIT12

// PLX INT CSR bit definitions
#define INTCSR_PCI_INT_ENABLE	BIT8

// PLX DMA Mode 0 Regsiter contents - 0x21D03
#define PLX_DMA_MODE0_VAL	(BIT17 | BIT12 | BIT11 | BIT10 | BIT8 | BIT1 | BIT0)

// PFT CSR bit definitions
#define PFT_CSR_FCFRDEN		BIT3
#define PFT_CSR_LTOPDRE		BIT4

// PFT RESET bit definition
#define PFT_RESET			BIT7

// Function Prototypes

// DriverEntry
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT,
	IN PUNICODE_STRING
	) ;

// AddDevice called by the PnP manager
NTSTATUS
PftAddDevice(
	IN PDRIVER_OBJECT pDriverObj,
	IN PDEVICE_OBJECT pPhyDevObj
	) ;

// Major function PnP dispatch
NTSTATUS
PftDispatchPnp(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	) ;

// PnP Start Device routine
NTSTATUS
PftStartDevice(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	) ;

// PnP start device completion routine
NTSTATUS
PftPnpComplete(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp,
	IN PVOID Context
	) ;

// Dispatch device create
NTSTATUS
PftDispatchCreate(
	IN PDEVICE_OBJECT,
	IN PIRP
	);

// Dispatch device close
NTSTATUS
PftDispatchClose(
	IN PDEVICE_OBJECT,
	IN PIRP
	);

// Handles IOCTL's dispatches
NTSTATUS
PftDispatchIoctl(
	IN PDEVICE_OBJECT,
	IN PIRP
	);

// PftAcquire
LOCAL NTSTATUS
PftAcquire (
	IN PPFT_DEVICE_EXTENSION
	) ;

// PftStopAcquisition
LOCAL BOOLEAN
PftStopAcquisition (
	IN PPFT_DEVICE_EXTENSION
	) ;

// PftGetBufferInfo
LOCAL BOOLEAN
PftGetBufferInfo (
	IN PVOID
	) ;

// PftTagBufferFree
LOCAL BOOLEAN
PftTagBufferFree (
	IN PVOID
	) ;

// PftIsr
BOOLEAN
PftIsr (
	IN PKINTERRUPT,
	IN OUT PVOID
	) ;

// PftDpcForIsr
LOCAL VOID PftDpcForIsr (
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    ) ;

// PftUpdateStats
LOCAL VOID
PftUpdateStats (
	IN PRKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	) ;

// PftDisableInterrupts
LOCAL NTSTATUS
PftDisableInterrupts (
	IN PPFT_DEVICE_EXTENSION
	) ;

// PftUnloadDriver
VOID
PftDriverUnload(
	IN PDRIVER_OBJECT
	) ;

// Dispatch routine for NT cleanup
NTSTATUS
PftDispatchCleanup(
	IN PDEVICE_OBJECT,
	IN PIRP
	) ;

// Read a register in PFT register address space
LOCAL LONG
PftReadRegister (
	IN PPFT_DEVICE_EXTENSION,
	IN ULONG
	) ;

// Read a register in PLX register address space
LOCAL LONG
PftReadPlxRegister(
	IN PPFT_DEVICE_EXTENSION,
	ULONG
	) ;

// Write a register in PFT register address space
LOCAL VOID
PftWriteRegister(
	PPFT_DEVICE_EXTENSION,
	ULONG,
	ULONG
	) ;

// Write a register in PLX register address space
LOCAL VOID
PftWritePlxRegister(
	PPFT_DEVICE_EXTENSION,
	ULONG,
	ULONG
	) ;

