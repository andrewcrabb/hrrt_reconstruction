// include file for gci

#define LOCAL static
#define OK 0
#define ERR -1

// gantry configuration for HR+
#define GCI_MAXBUCKETS	24		// max buckets on a PetCT

// constants for command path
#define GC		0		// constant for command path to Gantry Controller
#define BKT		1		// command path to bucket
#define IPCP	2 		// command path Image Plane Coincidence Processor
#define PT		3		// pass through command

// constants for block histograms
#define POSITION_SCATTER	0
#define POSITION_PROFILE	1
#define TUBE_ENERGY			2
#define CRYSTAL_ENERGY		3
#define TIME_SPECTRUM		4

// constants for bucket output mode
#ifndef BKTOUTPUTMODE
#define BKTOUTPUTMODE
#define SINGLE_CHANNEL	0
#define DUAL_CHANNEL	1
#endif

// constants for gantry data sizes
#define DATAPAGESIZE		32768
#define XFRTIME				60
#define DATASIZE			16

// notification structure
typedef struct {
	int error ;
	int severity ;
	char str[128] ;
} GCINOTIFY ;

// callback for acquisition
class CCallback {
	//notifications
	virtual void notify (GCINOTIFY*) ;
} ;

// structure for singles polling
typedef struct {
	int singles[GCI_MAXBUCKETS] ;
	int prompts ;
	int randoms ;
} SINGLES_POLL ;

// structure for threading bucket db loads
typedef struct {
	int startbkt ;
	int endbkt ;
	char* file ;
	ULONG addr ;
	int cnt ;
} DBARGS;


// define some default values
#define ENERGYWAIT				5000		// 5 seconds for energy settings
#define BKTRESET_TIMEOUT		120			// 120 second timeout for bucket reset

// gci header indexes
#define GN_HDR 0
#define GT_HDR 1
#define GM_HDR 2
#define SN_HDR 3
#define IP_HDR 4
#define IR_HDR 5
#define ES_HDR 6
#define RS_HDR 7
#define ER_HDR 8
#define UD_HDR 9
#define UT_HDR 10
#define TO_HDR 11

LOCAL char *gci_headers[] = {
	"GN",	// gantry controller command response header
	"GT",	// ascii text header for transparent commands 
	"GM",	// multiple record response header 
	"SN",	// bucket singles report header 
	"IP",	// image plane counts report header 
	"IR",	// instantaneous singles report for entire gantry 
	"ES",	// E-Stop notification header 
	"RS",	// remote start header 
	"ER",	// error report header
	":1",	// upload data record header (intellec format)
	":0",	// upload termination record header (intellec format)
	"TO",
	"\0",	// null terminator
} ;

// server side pipe name assignments

// command pipe
LOCAL char GCIPipe[]		= "\\\\.\\pipe\\GCIPipe" ;

// Pipes for bucket singles and IPCP messages and Instantaneous rates reports
LOCAL char GCISinglesPipe[]	= "\\\\.\\pipe\\GCISinglesPipe" ;
LOCAL char GCIRatesPipe[]	= "\\\\.\\pipe\\GCIRatesPipe" ;

// unsolicited asynchronous notification pipe
LOCAL char GCIAsyncPipe[]	= "\\\\.\\pipe\\GCIAsyncPipe" ;
