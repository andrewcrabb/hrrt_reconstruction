#pragma once

namespace ecat_model {

static int MaxBuckets = 56;   		/* maximum number of buckets */
static int MaxAxialCrystals = 32;  // max crystals in axial direction
static int MaxCrossPlanes = 6;

enum class TransmissionSource {none, Ring, Rod};
enum class Septa {NoSepta, Fixed, Retractable};
enum class PPorder {PP_LtoR, PP_RtoL};

struct EcatModel {
	char *number;  			/* model number as an ascii string */
	int rings;			/* number of rings of buckets */
	int nbuckets;			/* total number of buckets */
	int transBlocksPerBucket;	/* transaxial blocks per bucket */
	int axialBlocksPerBucket;	/* axial blocks per bucket */
	int blocks;
	int tubesPerBlock;		/* PMTs per block */
	int axialCrystalsPerBlock;	/* number of crystals in the axial direction */
	int angularCrystalsPerBlock;	/* number of transaxial crystals */
	int dbStartAddr;		/* bucket database start address */
	int dbSize;			/* size of bucket database in bytes */
	int timeCorrectionBits;		/* no. bits used to store time correction */
	int maxcodepage;		/* number of highest code page */
	PPorder ppOrder;		/* display order for position profile */

	int dirPlanes;			/* number of direct planes */
	int def2DSpan;			/* default span for 2D plane definitions */
	int def3DSpan;			/* default span for 3D plane definitions */
	int defMaxRingDiff;		/* default maximum ring difference for 3D */
	TransmissionSource txsrc;  /* transmission source type */
	float txsrcrad;			/* transmission source radius */
	Septa septa;		/* septa type */
	int defElements;		/* default number of elements */
	int defAngles;			/* default number of angles */
	int defMashVal;			/* default angular compression (mash) value */
	float defAxialFOV;		/* default axial FOV (one bed pos in cm) */
	float transFOV;			/* transaxial FOV */
	float crystalRad;		/* detector radius */
	float maxScanLen;		/* maximum allowed axial scan length (cm) */
	int defUld;			/* default ULD */
	int defLld;			/* default LLD */
	int defScatThresh;		/* default scatter threshold */
	float planesep;			/* plane separation */
	float binsize;			/* bin size (spacing of transaxial elements) */
	float pileup;			/*pileup correction factor for count losses */
	float planecor;			/* plane correction factor for count losses */
	int rodoffset;			/* Rod encoder offset of zero point */
	float hbedstep;			/*  horizontal bed step */
	float vbedoffset;		/* vertical bed step */
	float intrTilt;			/* intrinsic tilt */
	int wobSpeed;			/* default wobble speed */
	int tiltZero;			/* tilt Zero */
	int rotateZero;			/* rotate Zero */
	int wobbleZero;			/* wobble Zero */
	int bedOverlap;			/* number of planes to overlap between bed positions */
	int prt;			/* flag to indicate if scanner is partial ring tomograph */

/* parameters from (in 70rel3) 	*/
	float blockdepth;		/*  */
	int minelements;		/*  */
	int bktsincoincidence;		/*  */
	int cormask;			/*  */
	int analogasibucket;		/*  */

/* geometric correction parameters for normalization (in 70rel0)
	int corrBump;
	float cOffset;
	float integTime;
	float bCorr;
	int sBump;
	int eBump;
*/

};

}  // namespcae ecat_model

// extern "C" {
// EcatModel *ecat_model(int);
// }
