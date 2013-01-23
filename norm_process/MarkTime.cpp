/*-----------------------------------------------------------------
   What: MarkTime.cpp

    Why: This file contains routines for implementing timers.
		 The most useful routines are StartTimer(), StopTimer(), 
		 and PrintTimerList(). These routines create a linked
		 list of named timers for benchmarking.

    Who: Judd Jones

   When: 3/31/2004
   Modification History
   30-Apr-2009: Use gen_delays_lib C++ library for rebinning and remove dead code
              Integrate Peter Bloomfield _LINUX support


Copyright (C) CPS Innovations 2002-2003-2004 All Rights Reserved.
-------------------------------------------------------------------*/

//modifications for non-cluster application 6/25/2004 jjones

/*------------------------------------------------------------------
Modification History (HRRT User Community):
        04/30/09: Integrate Peter Bloomfield _LINUX support
-------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define MARKTIME
#include "MarkTime.h"

struct Mark	 *MarkRoot  = NULL;		// pointer to beginning of list
struct Timer *TimerRoot = NULL; 	//same for timers

// allocates memory for new mark

struct Mark *NewMark()
{
	struct Mark *M;

	M = (struct Mark *) malloc(sizeof(struct Mark));

	M->time = M->delt = 0.0;
	M->text = NULL;
	M->next = NULL;

	return M;
}

// prints one mark

void PrintMark(struct Mark *M){
	printf("Mark[%d]:\t%.1f\t%.1f\t%s\n", M->index,M->time,M->delt,M->text);
}
void PrintfMark(struct Mark *M){
	printf("Mark[%d]:\t%.1f\t%.1f\t%s\n", M->index,M->time,M->delt,M->text);
}

// prints all marks

void PrintMarkList()	//node
{
	struct Mark *ThisMark = MarkRoot;
	while( ThisMark != NULL ){
		PrintMark(ThisMark);
		ThisMark = ThisMark->next;
	}
}
void PrintfMarkList()	//server
{
	struct Mark *ThisMark = MarkRoot;
	while( ThisMark != NULL ){
		PrintfMark(ThisMark);
		ThisMark = ThisMark->next;
	}
}

// creates a new mark in the list

struct Mark *MarkTime(char *MarkText)
{
	static int initialized = 0;
	static int index = 0;
	static double TimeBase = 0.0;
	static struct Mark *Current;
	
	#ifdef _LINUX
		struct timeval	tnow ;
	#else
		struct _timeb	tnow;
	#endif
	double TimeNow;

	struct Mark *M;

	// get current time

	#ifdef _LINUX
		gettimeofday( &tnow , NULL ) ; TimeNow = ( (double) tnow.tv_sec + ( (double) tnow.tv_usec / 1000000.0 ) );
	#else
	  _ftime(&tnow); TimeNow = ( (double) tnow.time + ( (double) tnow.millitm / 1000.0 ) ); 
  #endif
	// get new mark, record text label

	M		 = NewMark();
	M->text  = (char *) malloc(strlen(MarkText)+1); strcpy(M->text,MarkText);
	M->index = index++;
	
	// initialization

	if( !initialized ){
		TimeBase	= TimeNow;
		MarkRoot	= M;
		Current		= M;
		initialized = 1;
		return M;
	}
	
	M->time			= TimeNow - TimeBase;			//current time relative to base
	M->delt			= M->time - Current->time;		//delta time since last mark
	Current->next	= M;							//update pointers
	Current			= Current->next;
	return Current;
}


// Linked List of Timers

void PrintTimer(struct Timer *T){
	 /* if( Node0 ) /* */ printf("%10.1f   : <%s>\n",T->total,T->name);
}
void PrintfTimer(struct Timer *T){
	 /* if( Node0 ) /* */ printf("%10.1f   : <%s>\n",T->total,T->name);
}

void PrintTimerList(){
	struct Timer *T = TimerRoot;
	while( T != NULL ){ PrintTimer(T);	T = T->next; }
}
void PrintfTimerList(){
	struct Timer *T = TimerRoot;
	while( T != NULL ){ PrintfTimer(T);	T = T->next; }
}

void FilePrintTimerList(){
	struct Timer *T = TimerRoot;
	char *FileName = "TimerFile.txt";
	FILE *TimerFile;

	TimerFile = fopen(FileName,"w");

	while( T != NULL ){ 
		fprintf(TimerFile,"%10.3f   : <%s>\n",T->total,T->name);
		T = T->next;
	}

	fclose(TimerFile);
}

void LogPrintTimerList(){
	struct Timer *T = TimerRoot;

	while( T != NULL ){ 
		printf("%10.3f   : <%s>\n",T->total,T->name);
		T = T->next;
	}

}

struct Timer *NewTimer(char *name)
{
	struct Timer *T;

	T = (struct Timer *) malloc(sizeof(struct Timer));
	T->name = (char *) malloc(strlen(name)+1);
	strcpy(T->name,name);
	T->last  = 0.0;
	T->total = 0.0;
	T->next = NULL;

	return T;
}

struct Timer *FindTimer(char *name)
{
	struct Timer *P = TimerRoot;

	while( P != NULL ){ 
		if( !strcmp(name,P->name) )	return P;
		P = P->next;
	}

	return NULL;

}

struct Timer *StartTimer(char *name)
{
	struct Timer *T,*P;
	struct Mark	 *M;

	if( TimerRoot == NULL )	T = TimerRoot = NewTimer(name);
	else 
	if( (T=FindTimer(name)) == NULL ){
		P = TimerRoot;	
		while( P->next != NULL ) P = P->next;
		P->next = T = NewTimer(name);
	}
	
	M = MarkTime(name);
	T->last = M->time;

	return T;
}

struct Timer *StopTimer(char *name)
{
	struct Timer *T;
	double time;

	if( (T=FindTimer(name)) == NULL ) /* this is an error */ return NULL;

	time = MarkTime(name)->time;
	
	T->total += time - T->last;
	T->last   = time;

	return T;
}

