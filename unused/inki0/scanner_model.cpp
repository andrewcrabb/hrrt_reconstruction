#include "scanner_model.h"
#include <stdio.h>
/*
model number, 
number of direct planes, 
number of elements per projection, 
number of angles or projections, 
number of crystals per ring, 
detector radius in mm, 
plane separation in mm, 
bin size in mm,
intrinsic tilt in degrees (float)

These specs have been confirmed by Mike Casey (26/02/1996)
You can add any future model, from your scanner console using info from ecat7 data_base

from ecat7 console start CAPP by typing capp_restart (idl_send_command deamon) then capp
the following works even in demo_mode (i.e. run_time license)

st = db_getmodelinfo(model_info,model='code')
help,/str,model_info

where 'code' is the CTI model number for the scanner (e.g. 966 for the monster)

Modifications
-------------
December 1996:
intrinsic_tilt sign changed to be compatible with ecat7
change HR tilt value from 12.86 to 13 to be compatible with ecat7
January 1997:
add defAngles in structure to fill attenuation subheader in ecat_osem 
it already accounts for defangularcomp (mash) 1 for models 961, 962 and 966
March 1997:
correct plane separation for 951-953-921-925: 6.75 is 3.375 mm...
May 1997:
correct bin_size for 962-966: 2.25 mm...
September 1997: 
microPET model (Thomas Farquhar - UCLA School of Medicine)
November 1997: 
add crystals_per_rings
November 1999:
add HRRT model as 999
November 2001
add P39-2 ECAM Gantry prototype
February 2002
P390 P39 Real gantry with 3-6 heads : parameters to be checked.
September 2002
Add 960 to test span influence on FORE (Defrise simulation package)
Add 963 to test flat data reconstruction from 962
Add 2000/2001 Micropet P4 and R4
Add model name default span and maxdel in model
December 2002
Set plane separation to 2.208 for P39 serie F.K.
August 2003
Add ACCEL 13x13 with various size of FOV
August 2005
Add Cardinal system (4 rings) at full and reduced resolution (994/995)
Note the model numbers in PET/CT world is 1094 (994+100)

2006	   Add MRPET brain Insert as 3000.
31-July-2008 : Added HRRT Low Resolution 2mm bin size
15-July-2008: Clean code to temove warning messages
*/
static ScannerModel
_scanner_model_951 = {"951","ECAT_951",  16, 192, 256,  512, 510.0f, 3.375f,  3.129f,  0.f,1,15}; /* no span in ecat 6*/
static ScannerModel 
_scanner_model_953 = {"953","ECAT_953",  16, 160, 192,  384, 382.5f, 3.375f,  3.129f,  15.f,1,15}; /* no span in ecat 6*/
static ScannerModel
_scanner_model_921 = {"921","EXACT_921", 24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.,7,17}; 
static ScannerModel
_scanner_model_923 = {"923","EXACT_923", 24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.f,7,17};
static ScannerModel
_scanner_model_925 = {"925","ART_925",   24, 192, 192,  384, 412.5f, 3.375f,  3.375f,  15.f,7,17}; 
static ScannerModel /* simulated - Defrise */
_scanner_model_960 = {"960","HR sim",    24, 256, 128,  784, 412.0f, 3.125f,  1.65f,    0.f,7,17}; 
static ScannerModel
_scanner_model_961 = {"961","HR",        24, 336, 196,  784, 412.0f, 3.125f,  1.65f,   13.f,7,17}; 
static ScannerModel
_scanner_model_962 = {"962","HR+",       32, 288, 144,  576, 412.5f, 2.425f,  2.25f,    0.f,7,17}; 
static ScannerModel /* resampled - Casey */ 
_scanner_model_963 = {"963","HR+ resamp",32, 256, 128,  576, 412.5f, 2.425f,  2.53125f, 0.f,7,17}; 
static ScannerModel  /* HR++ */
_scanner_model_966 = {"966","HR++",      48, 288, 144,  576, 412.5f, 2.425f,  2.25f,    0.f,9,40};
static ScannerModel  /* ACCEL13 - trimmed FOV*/
_scanner_model_980 = {"980","ACCEL_13",  41, 336, 336,  672, 419.5f, 2.f,     2.f,      0.f,11,27};
static ScannerModel /* ACCEL13 - ext FOV*/
_scanner_model_981 = {"981","ACCEL_13",  41, 448, 336,  672, 419.5f, 2.f,     2.f,      0.f,11,27}; 
static ScannerModel /* ACCEL13 - no tomographic gaps*/
_scanner_model_982 = {"982","ACCEL_13",  41, 312, 312,  624, 419.5f, 2.f,     2.f,      0.f,11,27}; 
static ScannerModel /* Cardinal 4 rings full resolution */
_scanner_model_994 = {"994","CARD_4",    55, 336, 336,  672, 428.0f, 2.005f,  2.005f,   0.f,11,38}; 
static ScannerModel /* Cardinal 4 rings at intermediate resolution */
_scanner_model_995 = {"995","CARD_4",    27, 168, 128,  672, 428.0f, 4.084f,   4.01f,    0.f,5,17}; 
static ScannerModel /* Cardinal 4 rings at low resolution */
_scanner_model_996 = {"996","CARD_4",    27, 128, 128,  672, 428.0f, 4.084f,   5.263125f,0.f,5,17}; 

/* Flat pannels */
/* HRRT High Resolution mode */
static ScannerModel
_scanner_model_999 = {"999","HRRT",     104, 256, 288, 1152, 234.5f, 1.21875f,1.21875f, 0.f,9,67}; /* also 3,67 */ 
/* HRRT Low Resolution mode 2 mm bin size */
static ScannerModel
_scanner_model_3281 = {"9991","HRRT",      52, 128, 144,  576, 234.5f, 2.4375f, 2.0f,  0.f,7,38};
/* HRRT Low Resolution mode 2.4375 mm bin size */
static ScannerModel 
_scanner_model_3282 = {"9992","HRRT",      52, 128, 144,  576, 234.5f, 2.4375f, 2.4375f,  0.f,7,38};
static ScannerModel
_scanner_model_390 = {"390","P39-2h",    84, 256, 256, 1024, 390.0f, 2.208f,  2.145f,   0.f,7,66}; 
static ScannerModel 
_scanner_model_392 = {"392","P39-2h",    84, 256, 256, 1024, 390.0f, 2.208f,  2.145f,   0.,7,66}; 
static ScannerModel
_scanner_model_393 = {"393","P39-3h",   120, 320, 256, 1024, 436.0f, 2.208f,  2.2f,     0.,7,87};
static ScannerModel
_scanner_model_394 = {"394","P39-4h",   120, 320, 256, 1024, 436.0f, 2.208f,  2.2f,     0.,7,87};
static ScannerModel
_scanner_model_395 = {"395","P39-5h",   120, 320, 256, 1024, 436.0f, 2.208f,  2.2f,     0.,7,87};  
/* User defined model */
static ScannerModel
_scanner_model_ext = {"0","User-defined",0,   0,   0,    0,   0.0f, 0.0,    0.0,     0.,0, 0};  

/* Micro PET 1 */
static ScannerModel
_scanner_model_171 = {"171","muPET1",     8, 100, 120,  480,  86.0f, 1.125,  1.125,   0.,1, 7}; /* no span */
/* Concorde micro PET Models use effective radius and binsize */
static ScannerModel
_scanner_model_2000 = {"2000","muPET-P4",32, 192, 168,  336, 131.0f, 1.2115f,  1.267f,  0.f,3,31}; 
static ScannerModel /* g_R_mm=74.2+4.58 ; g_bs_mm=1.21409 */
_scanner_model_2001 = {"2001","muPET-R4",32,  84,  96,  192, 78.78f, 1.2115f,  1.289f,  0.f,3,31}; 
static ScannerModel
_scanner_model_3000 = {"3000","MRPET",  77, 192, 192,  384, 197.96f, 1.25f,     1.25f,  0.f,9,67}; /* MR-PET*/
static ScannerModel /* Corconde gachon*/
_scanner_model_4000 = {"4000","f120-inki",  48, 128, 144,  288, 73.602f , 0.795f,     0.815f,  0.f,3,47}; 
static ScannerModel  /* Corconde gachon*/
_scanner_model_5000 = {"5000","f220-inki",  48, 288, 252,  576, 73.602f , 0.795f,     0.795f,  0.f,3,47};

static ScannerModel  /* ACCEL13 - no tomographic gaps*/
_scanner_model_1022 = {"1022","Biograph",  215, 168, 168,  624, 419.5f, 2.f,    4.f,      0.f,11,27};

ScannerModel *scanner_model(int system_type)
{
	switch(system_type) {
		case 951:
			return (&_scanner_model_951);
		case 953:
			return (&_scanner_model_953);
		case 921:
			return (&_scanner_model_921);
		case 923:
			return (&_scanner_model_923);
		case 925:
			return (&_scanner_model_925);
		case 961:
			return (&_scanner_model_961);
		case 962:
			return (&_scanner_model_962);
		case 966:
			return (&_scanner_model_966);
		case 980:
			return (&_scanner_model_980);
		case 981:
			return (&_scanner_model_981);
		case 982:
			return (&_scanner_model_982);
		case 994:
			return (&_scanner_model_994);
		case 995:
			return (&_scanner_model_995);
		case 996:
			return (&_scanner_model_996);
		case 171:
			return (&_scanner_model_171);
		case 328:
		case 999:
			return (&_scanner_model_999);
		case 3281:
			return (&_scanner_model_3281);
		case 3282:
			return (&_scanner_model_3282);
		case 390:
			return (&_scanner_model_390);
		case 392:
			return (&_scanner_model_392);
		case 393:
			return (&_scanner_model_393);
		case 394:
			return (&_scanner_model_394);
		case 395:
			return (&_scanner_model_395);
		case 2000:
			return (&_scanner_model_2000);
		case 2001:
			return (&_scanner_model_2001);
		case 3000:
			return (&_scanner_model_3000);
		case 1022:
			return (&_scanner_model_1022);
		case 4000:
			return (&_scanner_model_4000);
	case 5000:
			return (&_scanner_model_5000);
		default: 
			{ 	printf(" Model %d unknown, full specs should be given by -M option \n",system_type);
			return (&_scanner_model_ext);
			}
	}
	return 0;
}


