// PetCTGCIdll.h -	Include file for exported GCI DLL functions
//
//		Created:	jhr, 3feb01
//

#ifndef PETCTGCIDLLINCLUDE
#define PETCTGCIDLLINCLUDE

#ifndef PETCTGCIDLLEXPORT
	#define PETCTGCIDLL __declspec(dllimport)
#else
	#define PETCTGCIDLL __declspec(dllexport)
#endif

#include "gantryModelClass.h"

// constants
#ifndef BKTOUTPUTMODE
#define BKTOUTPUTMODE
#define SINGLE_CHANNEL	0
#define DUAL_CHANNEL	1
#endif

#ifdef __cplusplus
extern "C" {
#endif

// debug mode
PETCTGCIDLL void GCIdebug (BOOL, BOOL) ;

// initialization
PETCTGCIDLL BOOL GciInit (CGantryModel* gm) ;

// acquisition interface prototypes
PETCTGCIDLL BOOL GciPing				(BOOL& status) ;
PETCTGCIDLL BOOL GciEnergyQuery			(int, int *, int *, int *, int) ;
PETCTGCIDLL BOOL GciEnergySetup			(int, int, int, int) ;
PETCTGCIDLL BOOL GciSingles				(BOOL) ;
PETCTGCIDLL BOOL GciPollSingles			(int* singles, int* prompts, int* randoms, int* scatter) ;
PETCTGCIDLL BOOL GciPollInstantaneous	(int* corsingles, int* uncorsingles, int* prompts, int* randoms, int* scatter) ;
//PETCTGCIDLL BOOL GciAcqInit			(char*, CCallback*) ;

// pass through
PETCTGCIDLL int GCIpassthru				(char*, char*)	;
PETCTGCIDLL int GCIUtilPassThru			(char *cmd, char *resp) ;
	
// gantry power control and status
PETCTGCIDLL int disable_high_voltage	(void) ;
PETCTGCIDLL int enable_high_voltage		(void) ;
PETCTGCIDLL int get_high_voltage		(void) ;

// gantry polling
PETCTGCIDLL int disable_singles_polling (void) ;
PETCTGCIDLL int enable_singles_polling  (void) ;

// block select
PETCTGCIDLL int select_block			(int bucket, int block) ;

// detector setup
PETCTGCIDLL int calibrate_cfd_binary	(int bucket, int *cal_error) ;
PETCTGCIDLL int perform_timealign		(int bucket) ;
PETCTGCIDLL int perform_xy_offset_adjust(int bucket) ;
PETCTGCIDLL int autosetup_block			(int bucket) ;

// status
PETCTGCIDLL int poll_cfd				(int bucket, int *code) ;
PETCTGCIDLL int poll_timealign			(int bucket, int *code) ;
PETCTGCIDLL int poll_xy					(int bucket, int *code) ;
PETCTGCIDLL int poll_autosetup			(int bucket, int *code) ;

// abort
PETCTGCIDLL int abort_hist				(int bucket) ;
PETCTGCIDLL int abort_cfd				(int bucket) ;
PETCTGCIDLL int abort_timealign			(int bucket) ;
PETCTGCIDLL int abort_xy				(int bucket) ;

// tube gain
PETCTGCIDLL int get_tube_gains (int bucket, int *gain0, int *gain1, int *gain2, int *gain3) ;

// reset control and status
PETCTGCIDLL BOOL ResetBuckets				(int startbkt, int endbkt, int resetmode) ;
PETCTGCIDLL int reset_bucket				(int bucket) ;
PETCTGCIDLL int reset_bucket_all_defaults	(int bucket) ;
PETCTGCIDLL int reset_bucket_block_defaults (int bucket) ;
PETCTGCIDLL int report_bucket_status		(int bucket, int *code) ;

// histogram routines
PETCTGCIDLL BOOL AcquireHist				(int htype, int bkt, int blk, int acqtime, char* file) ;
PETCTGCIDLL BOOL ReleaseMsgPointer			(WPARAM wp) ;
PETCTGCIDLL BOOL SetWndHandle				(HWND hwnd) ;
PETCTGCIDLL BOOL ProcessSettingsOp			(int Op, int startbkt, int endbkt, int startblk, int endblk, int arg1, int arg2) ;

// upload download routines
PETCTGCIDLL int LoadBucketDB				(int startbkt, int endbkt, char* file, int direction, int controller) ;
PETCTGCIDLL int FirmwareVer					(int addr) ;
PETCTGCIDLL BOOL LoadFirmware				(int startaddr, int endaddr, char* file, int controller) ;

// bucket data output mode routines
PETCTGCIDLL BOOL SetGantryOutputMode		(int mode) ;
PETCTGCIDLL BOOL ChkGantryOutputMode		(int mode) ;
PETCTGCIDLL int	 SetBktOutputMode			(int bkt, int mode) ;
PETCTGCIDLL int	 GetBktOutputMode			(int bkt) ;

#ifdef __cplusplus
}
#endif
#endif

