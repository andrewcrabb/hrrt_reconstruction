// GCIResponseCodes.h - Return codes for GCI API commands
//

// general command responses
#define GCI_ERROR				-1
#define GCI_SUCCESS				0
#define COMM_TIMEOUT			1

// invalid item returns
#define INVALID_BUCKET			10
#define INVALID_BUCKET_BLOCK	11
#define INVALID_BLOCK			12

// setup poll responses
#define SETUP_GOOD				20
#define SETUP_WARNING			21
#define SETUP_PROGRESSING		22
#define SETUP_ABORTED			23
#define SETUP_UNKNOWN			24

// select / start responses
#define SETUP_IN_PROGRESS		30

// status report
#define CRC_IN_PROGRESS			40
#define STATUS_IN_PROGRESS		41
