// GciDecl.h - This file contains LOCAL (static) forward prototype declarations for PetCTGCIDll
//
// Note: See PetCTGCIDll.h for prototypes of exported functions

#ifndef GCIDECL
#define GCIDECL

#define LOCAL static

// Gantry Communication routines
LOCAL void GCIAsyncRead					() ;					// read asynchronous messages
LOCAL int send_command					(int, char*, char*) ;	// send command and handle response accordingly
LOCAL int parse_gc_response				(char*, int* ) ;		// parse response from gantry controller
LOCAL int parse_fesb_response			(char*, char*, int*) ;  // parse response from FESB
LOCAL int get_hdr_code					(char*) ;				// get the header code

// Bucket Histogramming Support routines
LOCAL BOOL acquire_histogram			(int, int, int, int) ;				// acquires block histogram data
LOCAL BOOL acquire_histogram_data		() ;								// acquire and process the block data
LOCAL int  poll_histogram				(int) ;								// polls bucket status for histgram completion
LOCAL BOOL get_position_scatter_profile (HANDLE, int) ;						// upload and store position scatter data
LOCAL BOOL get_position_profile_data	(HANDLE, int, int) ;				// upload and store position profile data
LOCAL BOOL get_energy_profile			(HANDLE, int, int) ;				// upload and store tube energy
LOCAL BOOL get_crystal_energy_profile	(HANDLE, int, int, int) ;			// upload and store crystal and peak energy data
LOCAL BOOL get_time_histogram_profile	(HANDLE, int) ;						// upload and store time histogram data
LOCAL BOOL upload_block_data			(int, char*, ULONG, int) ;			// upload data from buckets
LOCAL BOOL upload_boundary_data			(int, int, char*, char*, char*) ;	// get boundary data for position profile
LOCAL BOOL process_block_data			(HANDLE, int, int, int) ;			// get the data and store it
LOCAL BOOL save_position_profile		(HANDLE, char*, int) ;				// write position profile to file
LOCAL BOOL save_boundary_peak_data		(HANDLE, char*, char*, char*) ;		// save boundary and peak position data
LOCAL VOID free_position_profile_memory (char*, char*, char*, char*) ;		// free allocated position profile memory
LOCAL BOOL parse_intellec_record		(char*, char*, ULONG) ;				// parses intel intellec 8/MDS record
LOCAL BOOL ascii_to_hex					(char*, int*, int) ;				// convert ascii to hex binary
LOCAL BOOL write_data					(HANDLE, int) ;						// generic ascii write subroutine

// Routines for supporting Gantry Settings Utility
LOCAL BOOL execute_gantry_operation		() ;					// execute a gantry settings operation
LOCAL void op_tube_gains				(int startbkt, int endbkt, int startblk, int endblk, int arg1, int arg2) ;	
LOCAL void op_bucket_temps				(int startbkt, int endbkt) ;
LOCAL void op_water_temp				() ;
LOCAL void op_water_shutdown			(int low, int high) ;
LOCAL bool op_bucket_singles			(int startbkt, int endbkt, char* arg1) ;
LOCAL void op_low_voltage				(int startbkt, int endbkt) ;
LOCAL void op_high_voltage				(int startbkt, int endbkt);
LOCAL void op_energy_settings			(int startbkt, int endbkt, char desc) ;
LOCAL void op_cfd_settings				(int startbkt, int endbkt, int startblk, int endblk) ;
LOCAL void op_prom_version				(int startbkt, int endbkt, int prom) ;
LOCAL void op_block_singles				(int startbkt, int endbkt, int startblk, int endblk) ;
LOCAL void op_hv						(int Op) ;
LOCAL BOOL op_zap_bucket				(int startbkt, int endbkt) ;
LOCAL void op_reset_bucket				(int startbkt, int endbkt) ;
LOCAL void op_bucket_status				(int startbkt, int endbkt) ;
LOCAL void post_settings_update			(int Op, int bkt, int blk, char* resp) ;
LOCAL void post_settings_complete		(int Op, char* msg) ;
LOCAL void post_status_update			(int progress, int message, char* str) ; // update the status or progress bar
LOCAL PostWindowMessage					(HWND hWnd, int message, WPARAM wp) ;  // post a meesage to the GUI
LOCAL tGantryResult* NewGantryResult	(void) ;				// allocate an update structure

// Upload / Download databases and firmware support
LOCAL BOOL upload_bucket_db				(DBARGS*) ;
LOCAL BOOL download_bucket_db			(DBARGS* dbargs) ;
LOCAL void load_gc_firmware				(void*) ;
LOCAL void load_bkt_firmware			(void*) ;

LOCAL BOOL monitor_bkt_reset			(int startbkt, int endbkt, int message, bool report) ;

// Single / Dual channel modes
LOCAL BOOL set_bkt_output_mode			(int bkt, int mode) ;

#endif

