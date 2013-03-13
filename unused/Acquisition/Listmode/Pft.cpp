// Pft.cpp -  This code implements the PFT class that controls
//			  the PETLINK FIBRE TRANSCEIVER card for acquisitions.
//
//
// Created:	04mar02	-	jhr
//
//

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <iostream.h>
#include "acs3acq.h"

/*********************************************************************
*
*
*	CPft::Reset		- Reset the PFT card
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:		- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::Reset ()
{
	// does nothing now until we learn
	// more about the PFT card
	return (TRUE) ;
}

/*********************************************************************
*
*
*	CPft::GetMappedBuffer	- Map the kernel mode memory to the
*							  applications user space
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::GetMappedBuffer ()
{
	int err ;

	cout << "CPft::GetMappedBuffer - Inside map..." << endl ;

	// make sure we have not already mapped it
	if (m_pMappedBuffer) return (TRUE) ;

	// go map it
	return (IoControl (IOCTL_PFT_GET_MAPPED_BUFFER, NULL, 0, &m_pMappedBuffer, sizeof (ULONG), &err)) ;
}

/*********************************************************************
*
*
*	CPft::Acquire		- Start an acquisition
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::Acquire ()
{
	int err ;

	// issue the start
	cout << "Acquire: Starting PFT..." << endl ;
	return (IoControl (IOCTL_PFT_ACQUIRE, NULL, 0, NULL, 0, &err)) ;
}
	
/*********************************************************************
*
*
*	CPft::GetInfo			- Get acquisition statistics info
*	
*	IN	(NONE)				- Pointer to controlling input structure
*	OUT LMSTATS*			- Pointer to returned control structure
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::GetInfo (void* Info)
{
	int err ;

	// issue the call
	return (IoControl (IOCTL_PFT_GET_CB_INFO, NULL, 0, Info, sizeof(LMSTATS), &err)) ;
}

/*********************************************************************
*
*
*	CPft::TagBufferFree			- Tag current buffer free for use
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:					- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::TagBufferFree ()
{
	int err ;

	// go tell the driver the buffer is free
	return (IoControl (IOCTL_PFT_TAG_BUFFER_FREE, NULL, 0, NULL, 0, &err)) ;
}

/*********************************************************************
*
*
*	CPft::UnmapBuffer		- Unmap the kernel mode memory from the
*							  applications user space
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::UnmapBuffer ()
{
	int err ;
	BOOL ret ;

	// make sure we have not already mapped it
	if (!m_pMappedBuffer) return (TRUE) ;

	// go map it
	ret = IoControl (IOCTL_PFT_UNMAP_BUFFER, NULL, 0, &m_pMappedBuffer, sizeof (ULONG), &err) ;

	// clean up
	if (ret) m_pMappedBuffer = NULL ;

	return (ret) ;
}

/*********************************************************************
*
*
*	CPft::Stop			- Stop the acquisition
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::Stop ()
{
	BOOL ret ;
	int err ;

	// make sure we have an open handle
	cout << "Stop: Entering Stop..." << endl ;
	if (!m_Handle) return (FALSE) ;

	// do a stop
	cout << "Stop: Sending stop..." << endl ;
	ret = IoControl (IOCTL_PFT_STOP, NULL, 0, NULL, 0, &err) ;
	if (!ret) cout << "Stop: Failed..." << endl ;
	return (ret) ;
}

/*********************************************************************
*
*
*	CPft::SendNotificationHandle	- Send the event handle to driver
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:	- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CPft::SendNotificationHandle (HANDLE* ReadyEvent)
{
	BOOL ret ;
	int err ;

	// make sure we have an open handle
	cout << "SendNotificationHandle: Sending handle..." << endl ;
	if (!m_Handle) return (FALSE) ;

	// send the handle to the driver
	ret = IoControl (IOCTL_PFT_SET_NOTIFICATION_EVENT, (HANDLE*)ReadyEvent, sizeof (HANDLE), (void*) NULL, 0, &err) ;
	if (!ret)
	{
		cout << "AcqControl: IOCTL failed to send event" << endl ;
		CloseHandle (ReadyEvent) ;
		return (FALSE) ;
	}

	return (TRUE) ;
}