// PftIoctl.h - PFT I/O Control Codes for use with Win32 DeviceIoControl
//

#ifndef _INCLUDE_PFTIOCTL_
#define _INCLUDE_PFTIOCTL_

// Start I/O code with user definable code numbers

// Read and Write Register I/O Controls
#define IOCTL_READ_PFT_REG		CTL_CODE( FILE_DEVICE_UNKNOWN, 801, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_READ_PLX_REG		CTL_CODE( FILE_DEVICE_UNKNOWN, 802, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_WRT_PFT_REG		CTL_CODE( FILE_DEVICE_UNKNOWN, 803, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_WRT_PLX_REG		CTL_CODE( FILE_DEVICE_UNKNOWN, 804, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// For backward compatibility
#define IOCTL_GET_MAPPED_BUFFER	CTL_CODE( FILE_DEVICE_UNKNOWN, 805, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_UNMAP_BUFFER		CTL_CODE( FILE_DEVICE_UNKNOWN, 806, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Start an acquisition
#define IOCTL_PFT_ACQUIRE		CTL_CODE( FILE_DEVICE_UNKNOWN, 807, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Tag buffer as FREE after data is stored
#define IOCTL_PFT_TAG_BUFFER_FREE	CTL_CODE( FILE_DEVICE_UNKNOWN, 808, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Get the logical address of the common buffer - used for low level testing
#define IOCTL_GET_CB_LOGICAL	CTL_CODE( FILE_DEVICE_UNKNOWN, 809, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// For backward compatibility
#define IOCTL_GET_CB_INFO		CTL_CODE( FILE_DEVICE_UNKNOWN, 810, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Stop an acquisition
#define IOCTL_PFT_STOP			CTL_CODE( FILE_DEVICE_UNKNOWN, 811, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Map Kernel memory to User space I/O Control
#define IOCTL_PFT_GET_MAPPED_BUFFER	CTL_CODE( FILE_DEVICE_UNKNOWN, 812, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_PFT_UNMAP_BUFFER		CTL_CODE( FILE_DEVICE_UNKNOWN, 813, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Get the 'Ready to be Stored' buffer information
#define IOCTL_PFT_GET_CB_INFO	CTL_CODE( FILE_DEVICE_UNKNOWN, 814, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Some IOCTL's for the notification event
#define IOCTL_PFT_FIRE_NOTIFICATION_EVENT CTL_CODE( FILE_DEVICE_UNKNOWN, 821, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_PFT_SET_NOTIFICATION_EVENT CTL_CODE( FILE_DEVICE_UNKNOWN, 822, METHOD_BUFFERED,	FILE_ANY_ACCESS )

// Debug IOCTL's for PFT DMA counter
#define IOCTL_PFT_RESET_DMA_MON CTL_CODE( FILE_DEVICE_UNKNOWN, 831, METHOD_BUFFERED,	FILE_ANY_ACCESS )
#define IOCTL_PFT_GET_DMA_MON CTL_CODE( FILE_DEVICE_UNKNOWN, 832, METHOD_BUFFERED,	FILE_ANY_ACCESS )


// Structure definitions for Ioctl's that need to transfer info

// Listmode statistics structure for use by applications
typedef struct _LMSTATS
{
	ULONG status ;					// status of listmode acquisition
	__int64 total_events ;			// total events DMA'd to memory
	ULONG events ;					// events to store if storing
	ULONG event_rate ;				// event rate
} LMSTATS, *PLMSTATS ;

// Listmode DMA monitor values structure for use by debug routines
typedef struct _DMAMON
{
	int DmaUnderFlowCount ;			// Keep up with the time the DMA counter fails
	int DmaOverFlowCount ;			// Keep up with the time the DMA counter fails
	int MaxUnderFlow ;				// Maximum number of events DMA counter underflowed by
	int MaxOverFlow ;				// Maximum number of events DMA counter overflowed by
	int NDmaErrors ;				// Total number of errors
	int NBadEvents ;				// Cumulative number of bad events
} DMAMON, *PDMAMON ;

// Some definitions that both the application and driver will use
// Acquisition status
#define ACQ_STOPPED 0
#define ACQ_ACTIVE  1
#define ACQ_PRESET  3


	
#endif _INCLUDE_PFTIOCTL_






