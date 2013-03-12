#ifdef UseWithMedCom

#ifndef MedComLmHistStats_h
#define MedComLmHistStats_h

// statistics from the LM module
class MedComLM_stats {
public:
	MedComLM_stats(void) {};
	LARGE_INTEGER getTotalEvents(void);
DECLARE_MSC(MedComLM_stats)
	ULONG n_updates;		// # of times stats have been updated
	ULONG time_elapsed;		// in seconds
	ULONG time_remaining;	// in seconds
	ULONG total_event_rate;
	ULONG total_stream_events_low;	// low part of __int64
	LONG total_stream_events_high;	// high part of __int64
};
#ifndef NoImpl
LARGE_INTEGER MedComLM_stats::getTotalEvents(void) {
	LARGE_INTEGER largeInt;
	largeInt.LowPart = total_stream_events_low;
	largeInt.HighPart = total_stream_events_high;
	return largeInt;
}

IMPLEMENT_MSC(MedComLM_stats, G(n_updates) G(time_elapsed) G(time_remaining) \
			  G(total_event_rate) G(total_stream_events_low) 
			  G(total_stream_events_high) ) 
#endif


// statistics from the HIST module
class MedComHIST_stats {
public:
	MedComHIST_stats(void) {n_updates=0;}
DECLARE_MSC(MedComHIST_stats)
	ULONG n_updates;
	ULONG time_elapsed;
	ULONG time_remaining;
	// ZYB
	__int64 prompts_rate;
	__int64 random_rate;
	__int64 net_trues_rate;
	__int64 total_singles_rate;
};
#ifndef NoImpl
IMPLEMENT_MSC(MedComHIST_stats, G(n_updates) G(time_elapsed) G(time_remaining) \
			  G(prompts_rate) G(random_rate) G(net_trues_rate) \
			  G(total_singles_rate) )
#endif
#endif

#else

#ifndef LmHistStats_h
#define LmHistStats_h

// statistics from the LM module
class LM_stats {
public:
	LM_stats(void) {}
	ULONG n_updates;		// # of times stats have been updated
	ULONG time_elapsed;		// in seconds
	ULONG time_remaining;	// in seconds
	ULONG total_event_rate;
	LARGE_INTEGER total_stream_events;
};

// statistics from the HIST module
class HIST_stats {
public:
	HIST_stats(void) {n_updates=0;}
	ULONG n_updates;
	ULONG time_elapsed;
	ULONG time_remaining;
	// ZYB
	__int64 prompts_rate;
	__int64 random_rate;
	__int64 net_trues_rate;
	__int64 total_singles_rate;
};
#endif

#endif
