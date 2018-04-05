# pragma once

typedef struct _ScannerModel {
	// ahc
	// char *number;  		/* model number as an ascii string */
	// char *model_name;	/* model name as an ascii string  sept 2002   */	
	const char *number;  		/* model number as an ascii string */
	const char *model_name;	/* model name as an ascii string  sept 2002   */	
	int dirPlanes;		/* number of direct planes */
	int defElements;	/* default number of elements */
	int defAngles;		/* default number of angles */
	int crystals_per_ring;	/* number of crystal */
	float crystalRad;	/* detector radius in mm*/
	float planesep;		/* plane separation in mm*/
	float binsize;		/* bin size in mm (spacing of transaxial elements) */
	float intrTilt;		/* intrinsic tilt (not an integer please)*/
	int  span;		/* default span 		sept 2002	*/
	int  maxdel;		/* default ring difference 	sept 2002	*/
} ScannerModel;


extern ScannerModel *scanner_model(int);
