/*-----------------------------------------------------------------
   What: MarkTime.h

    Why: Definitions, etc., for timers.

    Who: Judd Jones

   When: 1/28/2002

Copyright (C) CPS Innovations 2002-2003-2004 All Rights Reserved.
Modification History (HRRT User Community):
        04/30/09: Integrate Peter Bloomfield _LINUX support

-------------------------------------------------------------------*/

//#include	<windows.h>
#include	<sys/types.h>
#ifdef WIN32
#include	<sys/timeb.h>
#else
#include <sys/time.h>
#endif
#include	<sys/malloc.h>

// The Mark Structure

struct Mark {				// structure of a "Mark"
	int	index;				// index in the list
	double time;			// time of mark
	double delt;			// time since last mark
	char  *text;			// user-supplied text label
	struct Mark *next;		// pointer to next mark in list
};

// The Timer Structure

struct Timer {
		char  *name;
		double last;
		double total;
		struct Timer *next;
};

// Prototypes

struct Mark *NewMark();
struct Mark *MarkTime(char *MarkText);
void PrintMark(struct Mark *M);
void PrintMarkList();
void PrintfMark(struct Mark *M);
void PrintfMarkList();

struct Timer *StartTimer(char *name);
struct Timer *StopTimer(char *name);

void PrintTimer(struct Timer *T);
void PrintTimerList();
void PrintfTimer(struct Timer *T);
void PrintfTimerList();
void FilePrintTimerList();
void LogPrintTimerList();







