
#define OK		0
#define ERR		-1

#define MEDCOMP_OK	1
#define	MEDCOMP_ERR	0

#define HRRT_SCANNER	0
#define ECAM_SCANNER	1
#define HRP_SCANNER		2

#define MAX_NFRAMES				100
#define EM_ONLY					0
#define TX_ONLY					1
#define SIMU_EM_TX 				2

#define NET_TRUES				0
#define PROMPTS_AND_RANDOMS		1
#define PROMPTS_ONLY			2
#define RANDOMS_ONLY			3

#define EM_PROMPT_MASK			0x40000000
#define EM_RANDOM_MASK			0x00000000
#define TX_PROMPT_MASK			0x60000000
#define TX_RANDOM_MASK			0x20000000

#define EM_PROMPT		0
#define EM_RANDOM		1
#define TX_PROMPT		2
#define TX_RADNOM		3

#define MAX_NPASSES 16
#define	MAX_DETECTOR_BLOCKS		936
#define MAX_3D_SEGMENTS			256

#define	TAG_UNKNOWN						0
#define	TAG_SCAN_TIME					1
#define	TAG_SCAN_SINGLE					2
#define	TAG_GANTRY_DETECTOR_ROTATION	4
#define	TAG_GANTRY_VERTICAL_BED			8
#define	TAG_GANTRY_HORIZONTAL_BED		16
#define	TAG_GANTRY_RODS					32
#define	TAG_PATIENT_GATING				64
#define	TAG_PATIENT_HEAD_MOTION			128

#define	TAG_NOACTION		-1
#define	TAG_TALLIED			-2
#define	TAG_END_OF_FRAME	-3
#define TAG_END_OF_SCAN		-4

#define FRAME_DEF_IN_SECONDS	0
#define FRAME_DEF_IN_NET_TRUES	1

#define HISTO_INITIALIZED		0
#define	HISTO_UNCONFIGURED		1		
#define HISTO_CONFIGURED		2
#define	HISTO_ACTIVE			3
#define	HISTO_WAIT				4
#define	HISTO_COMPLETE			5
#define HISTO_ERR				6
#define HISTO_STOPPED			7
#define	HISTO_STORE_COMPLETE	8
#define HISTO_STORE_ERR			9
#define	HISTO_ABORTED			10

#define HISTO_NO_PENDING_COMMANDS	0
#define	HISTO_STOP_PENDING			-1
#define HISTO_ABORT_PENDING			-2

// structure representing histogram parameters

typedef
   struct _histo_st {
	  // basic histogram attributes for PET
	  int	nprojs ;		// number of projections
	  int	nviews ;		// number of views
      int	acqm ;			// acquisition mode, 0=em_only, 1=tx_only, 2=em & tx
   	  int	em_sinm ;		// emission sinogram mode, 0=net trues, 1=prompts and randoms
	  int	n_em_sinos ;	// total number of emission sinograms
      int	tx_sinm ;		// transmission sinogram mode
	  int	n_tx_sinos ;	// total number of transmission sinograms
	  int	nrings ;		// total number of detector rings
	  int	ndetblks ;		// number of detector blocks, for block_singles_st

	  // derived parameters for histogram processing
      short int *sino_base ;// pointer to virtual memory address of histogram array in memory
      long	emp_offset ;	// the histogram memory offset for emission prompts
      long	emr_offset ; 	// the histogram memory offset for emission randoms
      int	max_em_offset ;	// total number of emission sinogram elements per frame
      long	txp_offset ; 	// the histogram memory offset for trans prompts
      long	txr_offset ;	// the histogram memory offset for trans randoms
      int	max_tx_offset ;	// total number of transmission sinogram elements per frame

	  // control parameters and flags for the histogram process
	  char	*em_filename ;
	  char	*tx_filename ;
	  char	*list_file ;
	  int	online_flag ;
	  int	npasses ;
	  int	start_offsets[MAX_NPASSES] ;
	  int	end_offsets[MAX_NPASSES] ;
	  int	pass ;
	  int	usablePhyMem ;
	  long	*lm_input_buff ;

} _histo_st ;


//  structure to define framing criteria
typedef
    struct _frame_def_st {
       int	nframes ;						// total number of frames defined
	   int	cur_frame ;						// current active frame number
	   int	frame_criteron ;				// 0=preset time, 1=total net trues
	   long	frame_start_time[MAX_NFRAMES] ; 	// relative frame start in seconds
       long	frame_duration[MAX_NFRAMES] ;   	// frame duration in seconds
       long	frame_switch_time[MAX_NFRAMES] ;	// timing tag value when switched
       float horizontal_bedoffset[MAX_NFRAMES] ;// horizontal bed offset in cm
	   // ZYB
       __int64	total_net_trues[MAX_NFRAMES] ;		// total counts frame-definition criterion
       __int64	total_em_prompts[MAX_NFRAMES] ;		// total emission prompts tallied
       __int64	total_em_randoms[MAX_NFRAMES] ;		// total emission randoms tallied
       __int64	total_tx_prompts[MAX_NFRAMES] ;		// total transmission prompts tallied
       __int64	total_tx_randoms[MAX_NFRAMES] ; 	// total transmission randoms tallied
} _frame_def_st ;


// structure for detector block singles rate

typedef
	struct _block_singles_st {
   	  int	ndetblks ;
      int	nsamples[MAX_DETECTOR_BLOCKS] ;
	  int	time_samples[MAX_DETECTOR_BLOCKS] ;
      float	uncor_rate[MAX_DETECTOR_BLOCKS] ;
      long  total_uncor_rate ;
} _block_singles_st ;


// structure to define all tagwords related information

typedef
   struct _tag_st {
		long ntime_tags ;
		long nsingles_tags ;
		__int64 ellapsed_time ;		// ZYB long					// ellapsed time in seconds
		_block_singles_st block_singles_st ;
} _tag_st;

// structure to define all header related information

typedef
struct _header_st {
	int	nrings;
	int ndetblks;
	float binsize;
	float plane_separation;
	int nsegments ;
	int	segment_table[MAX_3D_SEGMENTS] ;
} _header_st ;



