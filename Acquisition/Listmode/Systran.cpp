// Systran.cpp -  This code implements the CSldc class that controls
//				  the Systran SLDC Fibre Channel card for acquisitions.
//
//
// Created:	27feb02	-	jhr
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
*	CSldc::Reset	- Reset the SLDC and make sure the Fibre
*					  Channel link is stable
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::Reset ()
{
	int nTimes = 10, err ;
	BOOL csrOk = FALSE ;
	ULONG csr ;

	// make sure we have an open handle
	if (!m_Handle) return (FALSE) ;

	while (nTimes-- && !csrOk)
	{
		// perform the reset
		IoControl (IOCTL_SLDC_RECV_RESET, NULL, 0, NULL, 0, &err) ;

		// check the CSR
		IoControl (IOCTL_SLDC_GET_CSR, NULL, 0, &csr, sizeof (ULONG), &err) ;
		if ((csrOk & 0x000000ff) == 0) csrOk = TRUE ;
	}
	return (csrOk) ;
}

/*********************************************************************
*
*
*	CSldc::GetMappedBuffer	- Map the kernel mode memory to the
*							  applications user space
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::GetMappedBuffer ()
{
	BOOL ret ;
	int err ;


	// make sure we have not already mapped it
	cout << "CSldc::GetMappedBuffer - Inside map..." << endl ;
	if (m_pMappedBuffer) return (TRUE) ;

	// go map it
	ret = IoControl (IOCTL_SLDC_GET_MAPPED_BUFFER, NULL, 0, &m_pMappedBuffer, sizeof (ULONG), &err) ;
	if (!ret) cout << "CSldc::GetMappedBuffer - GetMappedBuffer Failed..." << endl ; 
	return (ret) ;
}

/*********************************************************************
*
*
*	CSldc::Acquire		- Start an acquisition
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::Acquire ()
{
	// drag the old baggage along
	ULONG ret, nxfers  = 0x7fffffff;
	int err ;

	// issue the start
	return (IoControl (IOCTL_SLDC_READ_BUFR_0, &nxfers, sizeof (ULONG), &ret, sizeof (ULONG), &err)) ;
}
	
/*********************************************************************
*
*
*	CSldc::BufferWaitClear		- Tag current buffer clear and wait
*							      for next one to fill up
*	
*	IN	BUFR_CLR_WAIT* bcw		- Pointer to controlling input structure
*	OUT BUFR_CLR_WAIT_RET* bcwr	- Pointer to returned control structure
*
*	Returns:					- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::BufferClearWait (BUFR_CLR_WAIT* bcw, BUFR_CLR_WAIT_RET* bcwr)
{
	int err ;

	// issue the call - this can block for a long time !!
	return (IoControl (IOCTL_SLDC_BUFR_CLR_WAIT, bcw, sizeof(BUFR_CLR_WAIT), 
								bcwr, sizeof(BUFR_CLR_WAIT_RET), &err)) ;
}

/*********************************************************************
*
*
*	CSldc::GetInfo			- Get acquisition statistics info
*	
*	IN	(NONE)				- Pointer to controlling input structure
*	OUT LM_ACQ_INFO*		- Pointer to returned stats structure
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::GetInfo (void* Info)
{
	int err ;

	// issue the call
	return (IoControl (IOCTL_SLDC_LM_ACQ_INFO, NULL, 0, Info, sizeof(LM_ACQ_INFO), &err)) ;
}

/*********************************************************************
*
*
*	CSldc::UnmapBuffer		- Unmap the kernel mode memory from the
*							  applications user space
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:				- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::UnmapBuffer ()
{
	int err ;
	BOOL ret ;

	// make sure we have not already mapped it
	if (!m_pMappedBuffer) return (TRUE) ;

	// go map it
	ret = IoControl (IOCTL_SLDC_UNMAP_BUFFER, NULL, 0, &m_pMappedBuffer, sizeof (ULONG), &err) ;

	// clean up
	if (ret) m_pMappedBuffer = NULL ;

	return (ret) ;
}

/*********************************************************************
*
*
*	CSldc::Stop			- Stop the acquisition
*	
*	IN	(NONE)
*	OUT (NONE)
*
*	Returns:			- TRUE on success or FALSE on failure
*
**********************************************************************/

BOOL
CSldc::Stop ()
{
	int err ;

	// make sure we have an open handle
	if (!m_Handle) return (FALSE) ;

	// do a stop
	return ( IoControl (IOCTL_SLDC_HALT, NULL, 0, NULL, 0, &err)) ;
}

