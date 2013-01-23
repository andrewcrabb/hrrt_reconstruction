/********************************************************************
/
/ PftDrvW2K.c - Driver for CTI's Petlink Fibre Translator - PFT
/
/	Created : 3sep02, J.H. Reed
/
/
/	This driver supports the PFT in the
/	2000/XP plug and play environment.
/
/*******************************************************************/


//
// Header files...
//
#include <wdm.h>
#include <devioctl.h>
#include "PftIoctl.h"
#include "Pft.h"

// definitions
#define LOCAL static


//++
// Function:
//		DriverEntry
//
// Description:
//		The DriverEntry routine that is called by the
//		I/O manager. It primarily sets up the I/O and plug
//		and play dispatch routines.
//
// Arguments:
//		DRIVER_OBJECT pointer
//		Registry path pointer 
//
// Return Value:
//		STATUS_XXX
//--

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS status = STATUS_SUCCESS ;
    DbgPrint("\nCPS WDM PetLink Fibre Transceiver Driver v1.0 -- Compiled %s %s\n",__DATE__, __TIME__);
    DbgPrint("(c) 1997-2002 CPS, Inc.\n\n") ;

    // Establish dispatch entry points for the functions we support
    DriverObject->MajorFunction[IRP_MJ_CREATE]         =  PftDispatchCreate ;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          =  PftDispatchClose ;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =  PftDispatchIoctl ;
	//DriverObject->MajorFunction[ IRP_MJ_CLEANUP]	   =  PftDispatchCleanup ;
    DriverObject->MajorFunction[IRP_MJ_READ]           =  NULL ;
    DriverObject->MajorFunction[IRP_MJ_WRITE]          =  NULL ;

    // PnP and Power entry points
    DriverObject->MajorFunction[IRP_MJ_PNP]            =  PftDispatchPnp ;

	// We don't handle power on this device
	//DriverObject->MajorFunction[IRP_MJ_POWER]             =  PftDispatchPower ;

    // AddDevice Function
    DriverObject->DriverExtension->AddDevice = PftAddDevice;

    // Unload function
	DriverObject->DriverUnload = PftDriverUnload;


	// That's it
	DbgPrint("DriverEntry: Pft entry complete...\n") ;

	return (status) ;
}

//++
// Function:
//		PftAddDevice
//
// Description:
//		This function is called by the PnP manager
//
// Arguments:
//		Pointer to Driver Object
//		Pointer to Device Object
//
// Return Value:
//		NTSTATUS - success or fail
//--

NTSTATUS
PftAddDevice(
	IN PDRIVER_OBJECT pDriverObj,
	IN PDEVICE_OBJECT pPhyDevObj
	)
{
	UNICODE_STRING NtDeviceName, Win32DeviceName ;
	PDEVICE_OBJECT pFdo ;
	PPFT_DEVICE_EXTENSION pDevExt ;
	NTSTATUS status ;


	DbgPrint("PftDispatchPnP: Entering Add Device...\n") ;

	// Make a UNICODE name to use.
	RtlInitUnicodeString (&NtDeviceName, PFT_NT_DEVICE_NAME) ;

	// Create the Device Object so we can get started
	status = IoCreateDevice (pDriverObj,
							 sizeof (PFT_DEVICE_EXTENSION),
							 &NtDeviceName,
							 FILE_DEVICE_UNKNOWN,
							 0,
							 FALSE,
							 &pFdo) ;

	if( !NT_SUCCESS( status ) )
	{
		DbgPrint("PftAddDevice: IoCreateDevice Failed... %08x\n", status) ;
		return status;
	}

	// Zap the device extension
	pDevExt = (PPFT_DEVICE_EXTENSION)pFdo->DeviceExtension;
	RtlZeroMemory( pDevExt, sizeof( PFT_DEVICE_EXTENSION ) );

	// Initialize the device extension
	pDevExt->PhysicalDeviceObject = pPhyDevObj ;
	pDevExt->FunctionalDeviceObject = pFdo ;

	DbgPrint ("PftAddDevice: pFdo = %08x\n", pFdo) ;

	// Let NT know how we plan to handle things
	pFdo->Flags |= DO_BUFFERED_IO ;

	// Pile the new FDO on top of the existing lower stack
	pDevExt->LowerDeviceObject = IoAttachDeviceToDeviceStack (pFdo, pPhyDevObj) ;

	// Form and create the Win32 symbolic link name.
	RtlInitUnicodeString (&Win32DeviceName, PFT_WIN32_DEVICE_NAME) ;
	status = IoCreateSymbolicLink (&Win32DeviceName, &NtDeviceName) ;
	if (!NT_SUCCESS(status))
	{
		// if it fails now, must delete Device object
		DbgPrint ("PftAddDevice: Could not create link... %08x\n", status) ;
		IoDeleteDevice( pFdo );
		return status;
	}

	// Initialize the statistics timer
	KeInitializeTimerEx (&pDevExt->StatsTimer, SynchronizationTimer) ;

	// Set up a DPC stats update
	KeInitializeDpc (&pDevExt->StatsDpc, (PKDEFERRED_ROUTINE)PftUpdateStats, pDevExt) ;

	// Initialize the ISR spin lock
	KeInitializeSpinLock (&pDevExt->IsrSpinLock) ;

    //  Clear the Device Initializing bit since the FDO was created outside of DriverEntry.
    pFdo->Flags &= ~DO_DEVICE_INITIALIZING;

	DbgPrint( "PftAddDevice: Routine complete...\n") ;

	return (status) ;
}

//++
// Function:
//		PftDispatchPnp
//
// Description:
//		This function dispatches the PnP IRP
//		from the I/O manager.
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_SUCCESS
//--

NTSTATUS
PftDispatchPnp(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION pIrpStack ;
	PPFT_DEVICE_EXTENSION pDevExt ;
	PDEVICE_OBJECT targetDev ;
	UNICODE_STRING Win32DeviceName ;
	KEVENT WaitForLowerDevice ;
	NTSTATUS status = STATUS_SUCCESS ;


	// Obtain the current IRP stack location
	pIrpStack = IoGetCurrentIrpStackLocation (Irp) ;

	// Pick up the device extension
	pDevExt = pDo->DeviceExtension ;
	DbgPrint ("PftDispatchPnP: pDo = 0x%08x  pDevExt = 0x%08x\n", pDo, pDevExt) ;

	// Initialize the event
	KeInitializeEvent (&WaitForLowerDevice, NotificationEvent, FALSE) ;

	// Deal with it
	DbgPrint ("PftDispatchPnP: Dispatch %02x\n", pIrpStack->MinorFunction) ;
	switch (pIrpStack->MinorFunction)
	{
		DbgPrint ("PftDispatchPnP: Dispatch %d\n", pIrpStack->MinorFunction) ;

		// Handle the PnP minor START_DEVICE IRP
		case IRP_MN_START_DEVICE:
			DbgPrint ("PftDispatchPnP: PnP dispatching IRP_MN_START_DEVICE\n") ;

			// Got to give it to the PDO first
			IoCopyCurrentIrpStackLocationToNext (Irp) ;

			// Set the IO completion routine
			IoSetCompletionRoutine (Irp, PftPnpComplete, &WaitForLowerDevice, TRUE, TRUE, TRUE) ;

			// Now, send the Irp to the PDO
			status = IoCallDriver (pDevExt->LowerDeviceObject, Irp) ;
			if (status == STATUS_PENDING)
			{
				// Wait for the PDO to finish with the IRP
				KeWaitForSingleObject (&WaitForLowerDevice, Executive, KernelMode, FALSE, NULL) ;

				// Pick up the status that the PDO left
				status = Irp->IoStatus.Status ;
			}

			// If it was successful start the device
			if (NT_SUCCESS (status))
			{
				status = PftStartDevice (pDo, Irp) ;
				if (!NT_SUCCESS (status))
				{
					// The PDO declined to start the device
					DbgPrint ("PftDispatchPnP: IRP_MN_START_DEVICE PDO would not start device... %08x\n", status) ;
				}
			}

			// We must now complete the IRP 
			Irp->IoStatus.Status = status ;
			Irp->IoStatus.Information = 0 ;
			IoCompleteRequest (Irp, IO_NO_INCREMENT) ;

			break ;

		// Handle the PnP minor QUERY_REMOVE IRP
		case IRP_MN_QUERY_REMOVE_DEVICE:
			DbgPrint ("PftDispatchPnP: PnP dispatching IRP_MN_QUERY_REMOVE_DEVICE\n") ;

			Irp->IoStatus.Status = STATUS_SUCCESS ;
			IoSkipCurrentIrpStackLocation (Irp) ;
			return (IoCallDriver (pDevExt->LowerDeviceObject, Irp)) ;

		// Handle the PnP minor QUERY_REMOVE IRP
		case IRP_MN_REMOVE_DEVICE:
			DbgPrint ("PftDispatchPnP: PnP dispatching IRP_MN_REMOVE_DEVICE\n") ;

			// Disable interrupts
			PftDisableInterrupts (pDevExt) ;
			
			// Disconnect the interrupt
			IoDisconnectInterrupt(pDevExt->InterruptObject) ;

			// Free the common buffer
			if( pDevExt->cb0.la_cb )
			{
				HalFreeCommonBuffer( pDevExt->AdapterObject, pDevExt->cb_len, pDevExt->cb0.pa_cb, pDevExt->cb0.la_cb, FALSE ) ;
			}

			// Free the memory descriptor list
			IoFreeMdl( pDevExt->Mdl ) ;

			// Release the memory resources
			MmUnmapIoSpace (pDevExt->PlxAddress, pDevExt->PlxLength) ;
			MmUnmapIoSpace (pDevExt->PftAddress, pDevExt->PftLength) ;

			// Remove the symbolic link from the object name space.
			RtlInitUnicodeString(&Win32DeviceName, PFT_WIN32_DEVICE_NAME) ;
			IoDeleteSymbolicLink(&Win32DeviceName);

			// Detach from the PDO
			IoDetachDevice (pDevExt->LowerDeviceObject) ;

			targetDev = pDevExt->LowerDeviceObject ;

			// Return the device object to the system
			IoDeleteDevice (pDevExt->FunctionalDeviceObject) ;

			// Setup to pass IRP on down the stack
			Irp->IoStatus.Status = STATUS_SUCCESS ;
			IoSkipCurrentIrpStackLocation (Irp) ;
			status = IoCallDriver (targetDev, Irp) ;
			return (status) ;

		default:
			// pass it on down
			DbgPrint ("PftDispatchPnP: default, gonna pass it on...\n") ;
            IoSkipCurrentIrpStackLocation (Irp);
            status = IoCallDriver (pDevExt->LowerDeviceObject, Irp);
			break ;
	}

	return (status) ;
}

//++
// Function:
//		PftStartDevice
//
// Description:
//		This function is called from the dispatch
//		PnP manager when an IRP_MN_START_DEVICE is
//		issued.
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_SUCCESS
//--

NTSTATUS
PftStartDevice(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION pIrpStack ;
	PPFT_DEVICE_EXTENSION pDevExt ;
	PCM_RESOURCE_LIST ResourceList ;
	PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor ;
	PCM_PARTIAL_RESOURCE_LIST PartialList ;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor ;
	PADAPTER_OBJECT AdapterObject ;
	DEVICE_DESCRIPTION DeviceDescription ;
	PHYSICAL_ADDRESS cbPhysicalAddress, translated ;
	PVOID cbLogicalAddress, vaddress ;
	PPFT_FC_REGS pft ;
	PMDL Mdl ;
	ULONG length, nMapRegs, cbLength ;
	NTSTATUS status ;
	int i, barcnt ;

	// Pick up the IRP stack position and the device extension
	pIrpStack = IoGetCurrentIrpStackLocation (Irp) ;
	pDevExt = (PPFT_DEVICE_EXTENSION)pDo->DeviceExtension ;

	// Get set up to parse the descriptor list
	ResourceList = pIrpStack->Parameters.StartDevice.AllocatedResourcesTranslated ;
	FullDescriptor = &ResourceList->List[0] ;
	PartialList = &FullDescriptor->PartialResourceList ;

	barcnt = 0 ;
	status = STATUS_SUCCESS ;
	
	// Parse the descriptor and determine where our resources are allocated
	for (i=0;i<(int)PartialList->Count;i++)
	{
		// Pick up partial descriptor
		PartialDescriptor = &PartialList->PartialDescriptors[i] ;

		// Pull out the resource allocations
		switch (PartialDescriptor->Type)
		{
			// Handle any I/O port resources
			case CmResourceTypePort:
				// We are not concerned with the I/O space on this board
				DbgPrint ("PftStartDevice: I/O space, skipping...\n") ;
				break ;

			// Handle the memory resources
			case CmResourceTypeMemory:
				translated = PartialDescriptor->u.Memory.Start ;
				length = PartialDescriptor->u.Memory.Length ;
				DbgPrint ("PftStartDevice: Memory space...\n") ;
				vaddress = MmMapIoSpace (translated, length, FALSE) ;
				DbgPrint ("PftStartDevice: Memory space...\n") ;

				// Make sure we got the I/O space mapped to memory
				if (vaddress == NULL)
				{
					DbgPrint ("PftStartDevice: MmMapIoSpace Failed...\n") ;
					return (STATUS_UNSUCCESSFUL) ;
				}

				// Store in device extension according to which BAR it came from
				if (!barcnt)
				{
					barcnt++ ;
					pDevExt->PlxAddress = vaddress ;
					pDevExt->PlxLength = length ;
					DbgPrint ("PftStartDevice: PftBar0 = %08x  Length = %d\n", pDevExt->PlxAddress, length) ;
				}
				else // assume it came from 2
				{
					pDevExt->PftAddress = vaddress ;
					pDevExt->PlxLength = length ;

					// Make sure the board is reset
					pft = ( PPFT_FC_REGS )pDevExt->PftAddress ;
					pft->isppr = PFT_RESET ;
					
					DbgPrint ("PftStartDevice: PftBar2 = %08x  Length = %d\n", pDevExt->PftAddress, length) ;
				}
				break ;

			// Handle the memory resources
			case CmResourceTypeInterrupt:
				// Pick up the interrupt information while we are here
				pDevExt->IRQL = PartialDescriptor->u.Interrupt.Level ;
				pDevExt->Vector = PartialDescriptor->u.Interrupt.Vector ;
				pDevExt->Affinity = PartialDescriptor->u.Interrupt.Affinity ;
				DbgPrint ("PftStartDevice: Interrupt, Level = %d, Vector = %x\n",
						   PartialDescriptor->u.Interrupt.Level,
						   PartialDescriptor->u.Interrupt.Vector) ;

				// Prepare a DPC Object for the Isr
				IoInitializeDpcRequest( pDevExt->FunctionalDeviceObject, PftDpcForIsr );

				// Connect the Interrupt
				status = IoConnectInterrupt (
									&pDevExt->InterruptObject,
									PftIsr,
									pDevExt->FunctionalDeviceObject,
									&pDevExt->IsrSpinLock,
									pDevExt->Vector,
									(KIRQL)pDevExt->IRQL,
									(KIRQL)pDevExt->IRQL,
									LevelSensitive,
									TRUE,
									pDevExt->Affinity,
									FALSE ) ;

				// Make sure we got it
				if ( !NT_SUCCESS( status ) )
				{
					DbgPrint ("PftStartDevice: IoConnectInterrupt Failed with status %08x\n", status) ;
					return( STATUS_DEVICE_CONFIGURATION_ERROR ) ;
				}

				// Fill out a DEVICE_DESCRIPTION so we can request a DMA Adapter object
				RtlZeroMemory( &DeviceDescription, sizeof( DEVICE_DESCRIPTION ) );
				cbLength = CB_SIZE * 2 ;
				DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION ;
				DeviceDescription.Master = TRUE ;
				DeviceDescription.ScatterGather = TRUE ;
				DeviceDescription.Dma32BitAddresses = TRUE ;
				DeviceDescription.InterfaceType = PCIBus;
				DeviceDescription.MaximumLength = cbLength;
				DeviceDescription.DmaWidth = Width32Bits;

				// Now we need an adapter object for the DMA
				nMapRegs = 0;
				if( ( AdapterObject = IoGetDmaAdapter(pDevExt->PhysicalDeviceObject,
													  &DeviceDescription,
													  &nMapRegs ) ) == NULL )
				{
					DbgPrint( "PftStartDevice: IoGetDmaAdapter Failed...\n" );
					if( pDo ) IoDeleteDevice( pDo );
					return( STATUS_DEVICE_CONFIGURATION_ERROR ) ;
				}

				pDevExt->AdapterObject = AdapterObject ;
				DbgPrint( "PftStartDevice: IoGetDmaAdapter %08x return %d Map Registers\n", AdapterObject, nMapRegs );

				// Allocate the common buffers for DMA operations if we get this far
				cbLogicalAddress = HalAllocateCommonBuffer(
													AdapterObject,
													cbLength,
													&cbPhysicalAddress,
													FALSE
													);

				// Make sure it worked
				if( cbLogicalAddress == NULL )
				{
					DbgPrint( "HalAllocateCommonBuffer Failed %08x   %08x,%08x\n",
										cbLogicalAddress,
										cbPhysicalAddress.LowPart,
										cbPhysicalAddress.HighPart
										);
					if( pDo ) IoDeleteDevice( pDo );
					return( STATUS_DEVICE_CONFIGURATION_ERROR );
				}

				DbgPrint( "PftStartDevice: cbPhysicalAddress = %08x, %08x   cbLogicalAddress = %08x\n",
							cbPhysicalAddress.LowPart,
							cbPhysicalAddress.HighPart,
							cbLogicalAddress
							);

				// Alocate and build a MDL
				Mdl = IoAllocateMdl(cbLogicalAddress, cbLength, FALSE, FALSE, NULL) ;

				if( !Mdl )
				{
				   DbgPrint( "Could not allocate MDL\n" );
				   if( pDo ) IoDeleteDevice( pDo );
				   HalFreeCommonBuffer( AdapterObject, cbLength, cbPhysicalAddress, cbLogicalAddress, FALSE ) ;
				   return( STATUS_INSUFFICIENT_RESOURCES );
				}

				MmBuildMdlForNonPagedPool( Mdl );


				// Fill in more of the device extension
				pDevExt->Mdl				 = Mdl ;
				pDevExt->cb_len				 = cbLength ;

				// Initialize the common buffers
				pDevExt->cb0.pa_cb.QuadPart	 = cbPhysicalAddress.QuadPart ;
				pDevExt->cb1.pa_cb.QuadPart	 = pDevExt->cb0.pa_cb.QuadPart ;
				pDevExt->cb1.pa_cb.QuadPart	 += (LONGLONG) CB_SIZE; 
				pDevExt->cb0.la_cb			 = cbLogicalAddress ;
				pDevExt->cb1.la_cb			 = (PVOID) ( (ULONG)cbLogicalAddress + CB_SIZE ) ;
				pDevExt->cb0.state			 = FREE ;
				pDevExt->cb1.state			 = FREE ;

				// Setup the buffer pointers in the device extension
				pDevExt->CurrentBuffer = &pDevExt->cb0 ;
				pDevExt->NextBuffer = &pDevExt->cb1 ;

				// That's it
				break ;

			default:
				break ;
		}
	}

	return (status) ;
}

//++
// Function:
//		PftPnpComplete
//
// Description:
//		This is the completion routine for the major PNP dispatch
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//		Pointer to context input
//
// Return Value:
//		Lower level driver status
//--

NTSTATUS
PftPnpComplete(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp,
	IN PVOID Context
	)
{
	PIO_STACK_LOCATION iostack ;
	PKEVENT event ;
	NTSTATUS status ;

	status = STATUS_SUCCESS ;
	event = (PKEVENT) Context ;

	iostack = IoGetCurrentIrpStackLocation (Irp) ;

	switch (iostack->MajorFunction)
	{
		case IRP_MJ_PNP:
			// Set the event the START_DEVICE is waiting on
			KeSetEvent (event, 0, FALSE) ;
			return (STATUS_MORE_PROCESSING_REQUIRED) ;
			break ;

		case IRP_MJ_POWER:
			// We don't support this in this driver
			DbgPrint ("PftPnpComplete: No MJ_POWER support\n") ;
			break ;

		default:
			DbgPrint ("PftPnpComplete: Not MJ_PNP or MJ_POWER ?? \n") ;
			break ;
	}

	return (status) ;
}

//++
// Function:
//		PftDispatchCreate
//
// Description:
//		This function dispatches
//		CreateFile from Win32
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_SUCCESS
//--

NTSTATUS
PftDispatchCreate(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	DbgPrint ("PftDispatchCreate: In Create Routine...\n") ;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//++
// Function:
//		PftDispatchClose
//
// Description:
//		This function dispatches
//		CloseHandle from Win32
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_SUCCESS
//--

NTSTATUS
PftDispatchClose(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	return STATUS_SUCCESS;
}

//++
// Function:
//		PftDispatchIoctl
//
// Description:
//		This function handles Ioctl dispatches
//		caused by DeviceIoControl calls from Win32
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_XXX
//--

NTSTATUS
PftDispatchIoctl(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	LARGE_INTEGER StatsPollTime ;
	PIO_STACK_LOCATION IrpStack;
	ULONG ControlCode, OutputLength, RegisterValue ;
	PULONG UserBuffer ;
	NTSTATUS status;
	PPFT_DEVICE_EXTENSION pDevExt;
	UNICODE_STRING EventName ;
	HANDLE EventHandle ;
	PPFT_FC_REGS pft ;

	// Pick up the IRP stack and extract the Control Code and the output buffer length
	IrpStack = IoGetCurrentIrpStackLocation( Irp );
	ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	// Pick up the device extension
	pDevExt = pDo->DeviceExtension;

	Irp->IoStatus.Information = 0;
	DbgPrint ("PftDispatchIoctl: ControlCode = %08x\n", ControlCode) ;
	switch( ControlCode )
	{
		// The user notification event is passed in here
		case IOCTL_PFT_SET_NOTIFICATION_EVENT:
			UserBuffer = (PULONG)Irp->AssociatedIrp.SystemBuffer ;
			EventHandle = (HANDLE)UserBuffer[0] ;
			DbgPrint ("PftDispatchIoctl: Reference object handle %x\n", EventHandle) ;
			status = ObReferenceObjectByHandle (EventHandle,
											    SYNCHRONIZE,
												*ExEventObjectType,
												Irp->RequestorMode,
												&pDevExt->EventHandle,
												NULL
												) ;
			if (!NT_SUCCESS (status))
			{
				DbgPrint ("PftDispatchIoctl: Unable to reference user mode event\n") ;
				break ;
			}

			status = STATUS_SUCCESS ;
			break ;
		// This IOCTL fires the event for debugging
		case IOCTL_PFT_FIRE_NOTIFICATION_EVENT:
			DbgPrint ("PftDispatchIoctl: Attempting to fire named event\n") ;
			if (pDevExt->EventHandle)
			{
				KeSetEvent ((PKEVENT)pDevExt->EventHandle, 0, FALSE) ;
			}

			status = STATUS_SUCCESS ;
			break ;
		// Get an address and map the kernel memory to user space
		case IOCTL_PFT_GET_MAPPED_BUFFER:
		case IOCTL_GET_MAPPED_BUFFER:
		 	if( OutputLength < sizeof( PVOID ))
		 	{
				status = STATUS_INVALID_BUFFER_SIZE ;
				Irp->IoStatus.Status = status ;
				Irp->IoStatus.Information = 0 ;
				IoCompleteRequest( Irp, IO_NO_INCREMENT ) ;
				return status;
		 	}

			// Now try to map the memory
		 	pDevExt->UserSpace = MmMapLockedPages(pDevExt->Mdl, UserMode) ;
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer ;
		 	*UserBuffer = ( ULONG )pDevExt->UserSpace ;
		 	Irp->IoStatus.Information = sizeof( ULONG ) ;
			pDevExt->FileObject = IrpStack->FileObject ;
			DbgPrint ("PftDispatchIoctl: Mapped user address = %08x\n", *UserBuffer) ;
			DbgPrint ("PftDispatchIoctl: FileObject = %08x\n", pDevExt->FileObject) ;
		 	status = STATUS_SUCCESS ;
			break ;
		// Unmap the kernel mode buffer from the user space
		case IOCTL_PFT_UNMAP_BUFFER:
		case IOCTL_UNMAP_BUFFER:
			if( pDevExt->UserSpace )
			{
	   			MmUnmapLockedPages(pDevExt->UserSpace, pDevExt->Mdl);
	   			pDevExt->UserSpace = NULL;
			}
		 	Irp->IoStatus.Information = 0;
			DbgPrint ("PftDispatchIoctl: Mapped pages unmapped\n") ;
			status = STATUS_SUCCESS;
			break;
		// Start an acquisition
		case IOCTL_PFT_ACQUIRE:
			RtlZeroMemory (&pDevExt->lmstats, sizeof (LMSTATS)) ;
			pDevExt->CurrentBuffer = &pDevExt->cb0 ;
			pDevExt->NextBuffer = &pDevExt->cb1 ;
			pDevExt->CurrentBuffer->state = FREE ;
			pDevExt->NextBuffer->state = FREE ;
			pDevExt->CurrentBuffer->events = 0 ;
			pDevExt->NextBuffer->events = 0 ;
			pDevExt->LastDmaCounter = 0 ;
			pDevExt->TotalEvents = 0 ;
			pDevExt->LastAddCounts = 0 ;
			pft = ( PPFT_FC_REGS )pDevExt->PftAddress ;
			pft->isppr = PFT_RESET ;
			status = PftAcquire (pDevExt) ;
			KeQuerySystemTime (&pDevExt->ScanStartTime) ;
			StatsPollTime.QuadPart = (LONGLONG)(-1 * 10 * 1000 * 1000) ;
			KeSetTimer (&pDevExt->StatsTimer, StatsPollTime, &pDevExt->StatsDpc) ;
			pDevExt->acqstatus = ACQ_ACTIVE ;
			break ;
		// Stop the acquisition
		case IOCTL_PFT_STOP:
			if( !KeSynchronizeExecution( pDevExt->InterruptObject, PftStopAcquisition, pDevExt ))
				 status = STATUS_UNSUCCESSFUL ;
			else status = STATUS_SUCCESS ;
			break ;
		// Pass back the information about the ready to store buffer
		case IOCTL_PFT_GET_CB_INFO:
		case IOCTL_GET_CB_INFO:
			if( !KeSynchronizeExecution( pDevExt->InterruptObject, PftGetBufferInfo, pDevExt ))
				 status = STATUS_UNSUCCESSFUL ;
			else status = STATUS_SUCCESS ;
			RtlCopyBytes (Irp->AssociatedIrp.SystemBuffer, &pDevExt->lmstats, sizeof (LMSTATS)) ;
		 	Irp->IoStatus.Information = sizeof( LMSTATS ) ;
			break ;
		// Tag the 'just stored' buffer as FREE
		case IOCTL_PFT_TAG_BUFFER_FREE:
			if( !KeSynchronizeExecution( pDevExt->InterruptObject, PftTagBufferFree, pDevExt ))
				 status = STATUS_UNSUCCESSFUL ;
			else status = STATUS_SUCCESS ;
			break ;
		// Reset the DMA monitor variables
		case IOCTL_PFT_RESET_DMA_MON:
			pDevExt->DmaUnderFlowCount = 0 ;
			pDevExt->DmaOverFlowCount = 0 ;	
			pDevExt->MaxUnderFlow = 0 ;	
			pDevExt->MaxOverFlow = 0 ;	
			pDevExt->NDmaErrors = 0 ;	
			pDevExt->NBadEvents = 0 ;
			break ;
		// Get the DMA monitor variables and return to caller
		case IOCTL_PFT_GET_DMA_MON:
			RtlCopyBytes (Irp->AssociatedIrp.SystemBuffer, &pDevExt->DmaUnderFlowCount, sizeof (int) * 6) ;
		 	Irp->IoStatus.Information = sizeof(int) * 6 ;
			break ;
		// Get the logical address of the buffer - this is used for low level testing
		case IOCTL_GET_CB_LOGICAL:
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer ;
			*UserBuffer = ( ULONG )pDevExt->cb0.pa_cb.LowPart ;
		 	Irp->IoStatus.Information = sizeof( ULONG );
		 	status = STATUS_SUCCESS;
		 	break;
		// Read a register from the PFT address space
		case IOCTL_READ_PFT_REG:
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer ;
			RegisterValue = PftReadRegister ( pDevExt, *UserBuffer ) ;
		 	*UserBuffer = RegisterValue;
		 	Irp->IoStatus.Information = sizeof( ULONG );
		 	status = STATUS_SUCCESS;
		 	break;
		// Read a register from the PLX address space
		case IOCTL_READ_PLX_REG:
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer ;
			RegisterValue = PftReadPlxRegister ( pDevExt, *UserBuffer ) ;
		 	*UserBuffer = RegisterValue;
		 	Irp->IoStatus.Information = sizeof( ULONG );
		 	status = STATUS_SUCCESS;
			break ;
		// Write a register in the PFT address space
		case IOCTL_WRT_PFT_REG:
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer;
			PftWriteRegister ( pDevExt, UserBuffer[0], UserBuffer[1] ) ;
		 	Irp->IoStatus.Information = 0;
		 	status = STATUS_SUCCESS;
		 	break;
		// Write a register in the PLX address space
		case IOCTL_WRT_PLX_REG:
		 	UserBuffer = ( PULONG )Irp->AssociatedIrp.SystemBuffer;
			PftWritePlxRegister ( pDevExt, UserBuffer[0], UserBuffer[1] ) ;
		 	Irp->IoStatus.Information = 0;
		 	status = STATUS_SUCCESS;
			break ;
		default:
		 	DbgPrint( "PftDispatchIoctl: Invalid IOCTL...\n" );
		 	status = STATUS_INVALID_DEVICE_REQUEST;
		 	break;
	}
	Irp->IoStatus.Status = status;
	IoCompleteRequest( Irp, IO_SERIAL_INCREMENT );
	return status;
}

//++
// Function:
//		PftAcquire
//
// Description:
//		Start an acquisition
//
// Arguments:
//
// Return Value:
//		NTSTATUS STATUS_XXX
//--

LOCAL NTSTATUS
PftAcquire (
	IN PPFT_DEVICE_EXTENSION pDevExt
	)
{
	PCB cbufr, nbufr ;
	PPLX_LC_REGS plx = pDevExt->PlxAddress ;
	PPFT_FC_REGS pft = pDevExt->PftAddress ;
	ULONG dmacsr ;

	DbgPrint ("PftAcquire: Inside Acquire routine\n") ;

	// Grab the current buffer
	cbufr = pDevExt->CurrentBuffer ;
	cbufr->events = 0 ;

	// Enable the PCI interrupts
	plx->intcsr |= INTCSR_PCI_INT_ENABLE ;

	// Clear the DMA counter
	pft->dmaoqbcr = 0x0 ;

	// Setup DMA Channel Mode 0 Register
	plx->dmamode0 = PLX_DMA_MODE0_VAL ;

	// Setup the DMA target address
	plx->dmapadr0 = cbufr->pa_cb.LowPart ;

	// Setup the Local Bus source address
	plx->dmaladr0 = PFT_LB_FA ;

	// Setup the transfer count (bytes)
	plx->dmasiz0 = CB_SIZE ;

	// Setup channel zero DMA descriptor pointer register
	plx->dmadpr0 = BIT3 ;

	// Make sure interrupt is clear and start DMA
	plx->dmacsr = (BIT3) ;
	plx->dmacsr = (BIT1 | BIT0) ;

	// enable fifo on PFT
	pft->csr = ( PFT_CSR_FCFRDEN | PFT_CSR_LTOPDRE ) ;

	// Check it
	dmacsr = plx->dmacsr ;
	if (!(dmacsr & BIT4)) 
	{
		DbgPrint ("PftAcquire: Acquire finished successfully\n") ;
		cbufr->state = ACQING ;
		return (STATUS_SUCCESS) ;
	}
	else
	{
		DbgPrint ("PftAcquire: Acquire failed\n") ;
		return (STATUS_UNSUCCESSFUL) ;
	}
}

//++
// Function:
//		PftStopAcquisition
//
// Description:
//		Stop the acquisition. Runs from KeSynchronizeExecution.
//
// Arguments: Pointer to device extension
//
// Return Value:
//		VOID
//--

LOCAL BOOLEAN
PftStopAcquisition (
	IN PPFT_DEVICE_EXTENSION pDevExt
	)
{
	PPLX_LC_REGS plx = pDevExt->PlxAddress ;
	PPFT_FC_REGS pft = pDevExt->PftAddress ;

	// set the acquisition status to stopped
	pDevExt->acqstatus = ACQ_STOPPED ;

	// clear the csr bits
	pft->csr &= ~( PFT_CSR_FCFRDEN | PFT_CSR_LTOPDRE ) ;

	// Stall and let the FIFO empty
	KeStallExecutionProcessor (2) ;

	// stop the acquisition
	plx->dmacsr |= DMA_CHN0_ABORT ;

	return TRUE ;
}

//++
// Function:
//		PftGetBufferInfo
//
// Description:
//		Gets the information necessary for the
//		application to determine what to do.
//		This routine runs under Sync Critical Section control.
//
// Arguments:
//
// Return Value:
//		TRUE
//--

LOCAL BOOLEAN
PftGetBufferInfo (
	IN PVOID Context
	)
{
	PPFT_DEVICE_EXTENSION pDevExt ;

	// Cast the device extension
	pDevExt = (PPFT_DEVICE_EXTENSION)Context ;

	// Get the total events
	pDevExt->lmstats.total_events = pDevExt->TotalEvents ;

	// Get the event rate
	pDevExt->lmstats.event_rate = pDevExt->EventRate ;

	// Get acquisition status
	pDevExt->lmstats.status = pDevExt->acqstatus ;

	// Get the DMA counter errors
	//pDevExt->lmstats.n_damoverflows = pDevExt->DmaOverFlowCount ;
	//pDevExt->lmstats.n_damunderflows = pDevExt->DmaUnderFlowCount ;

	return TRUE ;
}

//++
// Function:
//		PftTagBufferFree
//
// Description:
//		Sets the state of the Specified buffer to FREE
//		and restart acquisition if necessary. This routine
//		runs under Sync Critical Section control.
//
// Arguments:
//
// Return Value:
//		TRUE
//--

LOCAL BOOLEAN
PftTagBufferFree (
	IN PVOID Context
	)
{
	PCB cbufr, nbufr ;

	// Cast the device extension
	PPFT_DEVICE_EXTENSION pDevExt = (PPFT_DEVICE_EXTENSION)Context ;

	// Pick up buffer pointers
	cbufr = pDevExt->CurrentBuffer ;
	nbufr = pDevExt->NextBuffer ;

	// Check to see if we have to restart
	if (nbufr->state == PENDING)
	{
		pDevExt->CurrentBuffer = nbufr ;
		pDevExt->NextBuffer = cbufr ;
		PftAcquire (pDevExt) ;
	}
	else nbufr->state = FREE ;

	return TRUE ;
}

//++
// Function:
//		PftIsr
//
// Description:
//		The Interrupt Service Routine for DMA interrupts
//
// Arguments:
//		Pointer to the Kernel Interrupt Object
//		Pointer to the PFT_DEVICE_EXTENSION
//
// Return Value:
//		None
//--

BOOLEAN
PftIsr (
	IN PKINTERRUPT Interrupt,
	IN OUT PVOID Context
	)
{
	PDEVICE_OBJECT pDevObj ;
	PPFT_DEVICE_EXTENSION pDevExt ;
	PPLX_LC_REGS plx ;
	PPFT_FC_REGS pft ;
	ULONG dmacsr ;
	PCB current, next ;
	int nevents, dmacnt ;

	DbgPrint ("PftIsr: Inside the main service routine\n") ;

	// Pick up the device object and extension
	pDevObj = (PDEVICE_OBJECT)Context ;
	pDevExt = pDevObj->DeviceExtension ;

	// Check to see which DMA channel interrupted
	plx = ( PPLX_LC_REGS )pDevExt->PlxAddress ;
	dmacsr = plx->dmacsr ;

	// Make sure it is us
	if (dmacsr & DMA_CHN0_DONE)
	{
		DbgPrint ("PftIsr: Clearing the interrupt, before register = %08x\n", plx->dmacsr ) ;

		// Clear the interrupt
		dmacsr |= DMA_CHN0_CLR_INT ;
		plx->dmacsr = dmacsr ;
		DbgPrint ("PftIsr: After register = %08x\n", plx->dmacsr ) ;

		// Pick up the buffers
		current = pDevExt->CurrentBuffer ;
		next = pDevExt->NextBuffer ;

		// Get the number of events
		pft = ( PPFT_FC_REGS )pDevExt->PftAddress ;
		current->events = pft->dmaoqbcr ;

		// BEGIN - Patch to adjust DMA counter errors
		dmacnt = CB_SIZE / 4 ;

		// Underflow
		nevents = current->events ;

		if (pDevExt->acqstatus != ACQ_STOPPED)
		{
			if (nevents < dmacnt)
			{
				pDevExt->NBadEvents += (dmacnt - nevents) ;
				pDevExt->DmaUnderFlowCount++ ;
				if (nevents > pDevExt->MaxUnderFlow) pDevExt->MaxUnderFlow = nevents ;
				pDevExt->NDmaErrors++ ;
				current->events = dmacnt ;
			}

			// Overflow
			if (nevents > dmacnt)
			{
				pDevExt->NBadEvents += (nevents - dmacnt) ;
				pDevExt->DmaOverFlowCount++ ;
				if (nevents > pDevExt->MaxOverFlow) pDevExt->MaxOverFlow = nevents ;
				pDevExt->NDmaErrors++ ;
				current->events = dmacnt ;
			}
		}

		// END - DMA miscount code

		pDevExt->lmstats.events = current->events ;
		pDevExt->lmstats.total_events = pDevExt->TotalEvents + current->events - pDevExt->LastDmaCounter ;

		
		// Look at buffer states and decide what to do
		switch (next->state)
		{
			case FREE:
				// Update buffers, swap, and continue to acquire
				current->state = ACQD ;
				pDevExt->CurrentBuffer = next ;
				pDevExt->NextBuffer = current ;
				if (pDevExt->acqstatus == ACQ_ACTIVE) PftAcquire (pDevExt) ;
				else KeQuerySystemTime (&pDevExt->ScanStopTime) ;
				break ;
			case ACQD:
				// We're stuck, tag buffers and do not continue acquisition
				current->state = ACQD ;
				next->state = PENDING ;
				break ;
			default:
				break ;
		}

		// Throw it to the DPC
		IoRequestDpc (pDevObj, pDevObj->CurrentIrp, pDevExt) ;
		
	}
	
	return (TRUE) ;
}

//++
// Function:
//		PftDpcForIsr
//
// Description:
//		DPC for the PFT Interrupt Service
//
// Arguments:
//		Pointer to the Kernel's DPC object
//		Pointer to the DEVICE_OBJECT
//		Pointer to IRP being serviced
//		Context Pointer (in this case device extension)
//
// Return Value:
//		VOID
//--

LOCAL VOID 
PftDpcForIsr (
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
	PPFT_DEVICE_EXTENSION pDevExt ;

	DbgPrint ("PftDpcForIsr: Inside the ISR DPC routine\n") ;

	// Pick up the device extension
	pDevExt = (PPFT_DEVICE_EXTENSION)Context ;

	// For now all we need to do is toggle the notification event
	if (pDevExt->EventHandle)
	{
		KeSetEvent (pDevExt->EventHandle, 0, FALSE) ;
		KeClearEvent (pDevExt->EventHandle) ;
	}

	// That's it
	return ;
}

//++
// Function:
//		PftUpdateStats
//
// Description:
//		This routine is the DPC for updating 
//		statistics during an acquisition.
//
// Arguments:
//		Pointer to the Kernel's DPC object
//		Pointer to Context, the device extension in this case
//		Pointer to input arg1
//		Pointer to input arg2
//
// Return Value:
//		VOID
//--

LOCAL VOID
PftUpdateStats (
	IN PRKDPC Dpc,
	IN PVOID DeferredContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2
	)
{
	LARGE_INTEGER StatsPollTime, CurrentTime ;
	ULONG current_count , add_counts ;
	PPFT_DEVICE_EXTENSION pDevExt ;
	PPFT_FC_REGS pft ;
	KIRQL OldIrq ;
	ULONG acqstatus ;

	ULONG upper_limit, lower_limit ;
	ULONG debounce_count, last_add_counts ;

	DbgPrint ("PftUpdateStats: Stats timer expired...\n") ;

	// Compute the new totals

	// Pick up device extension
	pDevExt = (PPFT_DEVICE_EXTENSION)DeferredContext ;

	// Acquire the ISR spin lock
	KeAcquireSpinLock ( &pDevExt->IsrSpinLock, &OldIrq ) ;

	// Get the current system time
	KeQuerySystemTime (&CurrentTime) ;

	// Pick up the PFT and PLX base address
	pft = (PPFT_FC_REGS)pDevExt->PftAddress ;

	// Debounce DMA count until we understand why
	debounce_count = 0 ;
	while (1)
	{
		// Get the current counter value
		current_count = pft->dmaoqbcr ;
		DbgPrint ("PftUpdateStats: Current %d  LastDmaCounter %d\n", current_count, pDevExt->LastDmaCounter) ;
		if (current_count < pDevExt->LastDmaCounter) add_counts = ((CB_SIZE / 4) - pDevExt->LastDmaCounter) + current_count ;
		else add_counts = current_count - pDevExt->LastDmaCounter ;
		last_add_counts = pDevExt->LastAddCounts ;
		debounce_count++ ;
		if (last_add_counts > 0)
		{
			upper_limit = (last_add_counts / 5) + last_add_counts ;
			lower_limit = last_add_counts - (last_add_counts / 5) ;
			DbgPrint ("PftUpdateStats: Debouncer - add %d  last %d  upper %d  lower %d\n",
							add_counts, last_add_counts, upper_limit, lower_limit) ;
			if ((add_counts >= lower_limit) && (add_counts <= upper_limit)) break ;
		} else break ;

		// Put a limit on the number of times so we don't deadlock
		if (debounce_count > 5) break ;
	}

	DbgPrint ("PftUpdateStats: Debounce count = %d\n", debounce_count) ;

	pDevExt->LastAddCounts = add_counts ;
	pDevExt->LastDmaCounter = current_count ;
	pDevExt->TotalEvents += (__int64)add_counts ;

	acqstatus = pDevExt->acqstatus ;

	DbgPrint ("PftUpdateStats: Current count %d   Add Counts  %d   Last Count %d   Total Counts %d\n",
				current_count, add_counts, current_count % CB_SIZE, pDevExt->TotalEvents) ;

	// Release the ISR spin lock
	KeReleaseSpinLock ( &pDevExt->IsrSpinLock, OldIrq ) ;

	// If the acquisition status is inactive cancel the update timer
	if (acqstatus != ACQ_STOPPED)
	{
		pDevExt->ElapsedTime.QuadPart = CurrentTime.QuadPart - pDevExt->ScanStartTime.QuadPart ;
		DbgPrint ("PftUpdateStats: Acquisition status %08x resetting timer elapsed time %I64d\n",
							acqstatus, pDevExt->ElapsedTime.QuadPart) ;
		StatsPollTime.QuadPart = (LONGLONG)(-1 * 10 * 1000 * 1000) ;
		KeSetTimer (&pDevExt->StatsTimer, StatsPollTime, &pDevExt->StatsDpc) ;
	}
	else
	{
		pDevExt->ElapsedTime.QuadPart = pDevExt->ScanStopTime.QuadPart - pDevExt->ScanStartTime.QuadPart ;
		DbgPrint ("PftUpdateStats: Acquisition status %08x elapsed time %I64d",
							acqstatus, pDevExt->ElapsedTime.QuadPart) ;
	}

	// Compute the event rate
	pDevExt->EventRate = (ULONG)(pDevExt->TotalEvents / (pDevExt->ElapsedTime.QuadPart / 10000000)) ;
	DbgPrint ("PftUpdateStats: Event Rate = %d\n", pDevExt->EventRate) ;

	return ;
}
			

//++
// Function:
//		PftDisableInterrupts
//
// Description:
//		This routine disables the PCI interrupts in the 9080 
//
// Arguments:
//		Pointer to the device extension
//
// Return Value:
//		VOID
//--

LOCAL NTSTATUS
PftDisableInterrupts (
	IN PPFT_DEVICE_EXTENSION pDevExt
	)
{
	PPLX_LC_REGS plx ;

	// Pick up the address to the PLX registers
	plx = ( PPLX_LC_REGS )pDevExt->PlxAddress ;

	// Disable the PCI interrupts
	plx->intcsr &= ~INTCSR_PCI_INT_ENABLE ;

	return (STATUS_SUCCESS) ;
}


//++
// Function:
//		PftDriverUnload
//
// Description:
//		This function dynamically unloads the driver
//		Called by the Windows IO Manager
//
// Arguments:
//		Pointer to the DRIVER_OBJECT
//
// Return Value:
//		None
//--

VOID
PftDriverUnload (
	IN PDRIVER_OBJECT DriverObject
	)
{
	DbgPrint( "PftDriverUnload: Inside Unload...\n" );
	return ;
}

//++
// Function:
//		PftDispatchCleanup
//
// Description:
//		This function handles the cleanup of queued IRP's
//		when the IO manager responds to terminated threads
//
// Arguments:
//		Pointer to Device object
//		Pointer to IRP for this request
//
// Return Value:
//		STATUS_XXX
//--

NTSTATUS
PftDispatchCleanup(
	IN PDEVICE_OBJECT pDo,
	IN PIRP Irp
	)
{
	PIRP cancelIrp, requeueIrp;
	PIO_STACK_LOCATION cancelIrpStack, cleanupIrpStack;
	LIST_ENTRY cancelList, requeueList;
	PLIST_ENTRY listHead;
	PKDEVICE_QUEUE_ENTRY queueEntry;
	PKDEVICE_QUEUE deviceQueue;
	PPFT_DEVICE_EXTENSION pDevExt ;
	KIRQL oldIrql;

	DbgPrint( "PftDispatchCleanup: Inside Cleanup...\n" );
	//return STATUS_SUCCESS ;

	// Pick up device extension pointer
	pDevExt = pDo->DeviceExtension ;

	// first disable interrupts
	PftDisableInterrupts (pDevExt) ;
	
	// get the cleanup IRP stack location
	cleanupIrpStack = IoGetCurrentIrpStackLocation( Irp ) ;
	DbgPrint( "PftDispatchCleanup: FileObject = %08x\n", cleanupIrpStack->FileObject );

	// unmap the locked memory
	if( pDevExt->UserSpace )
	{
		if (cleanupIrpStack->FileObject == pDevExt->FileObject)
		{
			DbgPrint( "PftDispatchCleanup: Unmapping memory...\n" );
	   		MmUnmapLockedPages(pDevExt->UserSpace, pDevExt->Mdl);
	   		pDevExt->UserSpace = NULL;
			pDevExt->FileObject = NULL;
		}
	}

	// First initialize the linked lists
	InitializeListHead( &cancelList );
	InitializeListHead( &requeueList ) ;

	// Get the Cancel Spin Lock
	DbgPrint( "PftDispatchCleanup: Acquiring cancel spinlock...\n" );
	IoAcquireCancelSpinLock( &oldIrql );

	// If the list is empty there is nothing to do
	deviceQueue = &pDo->DeviceQueue;
	if( IsListEmpty( &deviceQueue->DeviceListHead ))
	{
		DbgPrint( "PftDispatchCleanup: Nothing in queue, exiting...\n" );
		IoReleaseCancelSpinLock( oldIrql );
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest( Irp, IO_NO_INCREMENT );
		DbgPrint( "PftDispatchCleanup: Releasing cancel spinlock...\n" );
		return STATUS_SUCCESS;
	}

	// Empty the device queue into a cancel list and a requeue list
	while(( queueEntry = KeRemoveDeviceQueue( deviceQueue )) != NULL )
	{
		// pick up base address of the queue instance
		cancelIrp = CONTAINING_RECORD( queueEntry, IRP, Tail.Overlay.DeviceQueueEntry );

		// pickup cancel IRP stack and see which list it goes in
		cancelIrpStack = IoGetCurrentIrpStackLocation( cancelIrp );
		if( cancelIrpStack->FileObject == cleanupIrpStack->FileObject )
		{
			// put it in the cancel list
			cancelIrp->Cancel = TRUE;
			cancelIrp->CancelIrql = oldIrql;
			cancelIrp->CancelRoutine = NULL;
			InsertTailList( &cancelList, &cancelIrp->Tail.Overlay.ListEntry );
		} else {
			// stick it in the requeue list
			InsertTailList( &requeueList, &cancelIrp->Tail.Overlay.ListEntry );
		}
	}

	// First traverse the requeue list and handle those IRPs
	while( !IsListEmpty( &requeueList ))
	{
		listHead = RemoveHeadList( &requeueList );
		requeueIrp = CONTAINING_RECORD( listHead, IRP, Tail.Overlay.ListEntry );
		if( !KeInsertDeviceQueue( deviceQueue, &requeueIrp->Tail.Overlay.DeviceQueueEntry ))
		{
			KeInsertDeviceQueue( deviceQueue, &requeueIrp->Tail.Overlay.DeviceQueueEntry );
		}

		// release the spin lock
		IoReleaseCancelSpinLock( oldIrql );
		DbgPrint( "PftDispatchCleanup: Releasing cancel spinlock...\n" );
	}

	// Now traverse the cancel list and complete all the cancelled IRPs
	while( !IsListEmpty( &cancelList ))
	{
		listHead = RemoveHeadList( &cancelList );
		cancelIrp = CONTAINING_RECORD( listHead, IRP, Tail.Overlay.ListEntry );
		cancelIrp->IoStatus.Status = STATUS_CANCELLED;
		cancelIrp->IoStatus.Information = 0;
		IoCompleteRequest( cancelIrp, IO_NO_INCREMENT) ;
	}

	// That's it for the cleanup now deal with the dispatch complete
	DbgPrint( "PftDispatchCleanup: Completing IRP\n" );
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	// All done
	return STATUS_SUCCESS;
}

//++
// Function:
//		PftReadRegister
//
// Description:
//		Read a specified register in the PFT address space
//
// Arguments:
//		Pointer to Device Extension
//		ULONG - offset into address space
//
// Return Value:
//		ULONG - value from register space
//--

LOCAL LONG
PftReadRegister(
	PPFT_DEVICE_EXTENSION pDevExt,
	ULONG offset
	)
{
	ULONG *lptr, val ;

	lptr = ( ULONG * )pDevExt->PftAddress ;
	val = lptr[offset>>2];
	DbgPrint ("PftReadRegister: Read %x from address %08x  offset %08x\n", val, lptr, offset>>2) ;

	return( val );
}

//++
// Function:
//		PftReadPlxRegister
//
// Description:
//		Read a specified register in the PLX 9080 address space
//
// Arguments:
//		Pointer to Device Extension
//		ULONG - offset into address space
//
// Return Value:
//		ULONG - value from register space
//--

LOCAL LONG
PftReadPlxRegister(
	PPFT_DEVICE_EXTENSION pDevExt,
	ULONG offset
	)
{
	ULONG *lptr, val ;

	lptr = ( ULONG * )pDevExt->PlxAddress ;
	val = lptr[offset>>2];
	DbgPrint ("PftReadPlxRegister: Read %x from address %08x  offset %08x\n", val, lptr, offset>>2) ;

	return( val );
}

//++
// Function:
//		PftWriteRegister
//
// Description:
//		Write a specified register in the PFT address space
//
// Arguments:
//		Pointer to Device Extension
//		ULONG - offset into address space
//		ULONG - value to poke
//
// Return Value:
//		STATUS_SUCCESS | STATUS_UNSUCCESSFUL
//--

LOCAL VOID
PftWriteRegister(
	PPFT_DEVICE_EXTENSION pDevExt,
	ULONG offset,
	ULONG val
	)
{
	ULONG *lptr;

	lptr = ( ULONG * )pDevExt->PftAddress ;
	lptr[offset>>2] = val;
	DbgPrint ("PftWriteRegister: Wrote %x to address %08x  offset %08x\n", val, lptr, offset>>2) ;
}

//++
// Function:
//		PftWritePlxRegister
//
// Description:
//		Write a specified register in the PLX 9080 address space
//
// Arguments:
//		Pointer to Device Extension
//		ULONG - offset into address space
//		ULONG - value to poke
//
// Return Value:
//		STATUS_SUCCESS | STATUS_UNSUCCESSFUL
//--

LOCAL VOID
PftWritePlxRegister(
	PPFT_DEVICE_EXTENSION pDevExt,
	ULONG offset,
	ULONG val
	)
{
	ULONG *lptr;

	lptr = ( ULONG * )pDevExt->PlxAddress ;
	lptr[offset>>2] = val;
	DbgPrint ("PftWritePlxRegister: Wrote %x to address %08x  offset %08x\n", val, lptr, offset>>2) ;
}



