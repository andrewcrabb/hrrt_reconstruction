// HR+962.h - Include file with HR+ gantry specific definitions
//
//
//	Created:	26jun01 - jhr
//
//

// EEPROM base addresses
#define EEPROMBASE	0x2000		// EEPROM base address
#define ROWBOUNDARY	0x1000		// Row Boundary offset
#define COLBOUNDARY	0x1300		// Column Boundary offset
#define PKPOSITION	0x0800		// Peak Position offset
#define PKENERGY	0x0230		// Peak Energy offset

// Bucket memory address definitions
#define HISTOADDR	0x4000		// Bucket Histogram Ram base address

// Data size definitions for different profiles
#define POSPROSIZE	8192		// Size in bytes of 962 bucket Position Profile size
#define POSSCATTERSIZE 1024		// Crystal Position Scatter data size
#define TENGSIZE	512			// Time Energy data size
#define CENGSIZE	8192		// Crystal Energy data size
#define PKENGSIZE	64			// Peak Energy data size
#define TIMHISTSIZE 512			// Time Histogram data size
#define BOUNDSIZE	64			// Row, Column, and Peak data size
#define XYDIMENSION 8			// X and Y dimension size

// Bucket/Block definitioins
#define XYSIZE		64			// size for X[8] and y[8] sizes
#define NBLOCKS		12			// number of blocks/bucket on HR+
#define NBUCKETS	24			// number of buckets on HR+

// Structure definitions for GCI
typedef struct {
	int htype ;				// histogram type
	int bkt ;				// bucket number
	int blk ;				// block number
	int acqtime ;			// acquisition time
	char filename[128] ;	// file name to store data in
} HISTARGS ;

typedef struct {
	int Op ;				// gantry settings operation code
	int startbkt ;			// starting bucket
	int endbkt ;			// ending bucket
	int startblk ;			// starting block
	int endblk ;			// ending block
	int arg1 ;				// operation dependent argument #1
	int arg2 ;				// operation dependent argument #2
} GSARGS ;
