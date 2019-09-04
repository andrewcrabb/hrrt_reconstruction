// Header.h: interface for the CHeader class.
//
//////////////////////////////////////////////////////////////////////
/*------------------------------------------------------------------
Modification History (HRRT User Community):
        04/30/09: Integrate Peter Bloomfield __linux__ support
-------------------------------------------------------------------*/
#pragma once

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define		OK	0
#define		E_FILE_NOT_READ	-1
#define		E_TAG_NOT_FOUND -2
#define		E_FILE_ALREADY_OPEN -3
#define		E_COULD_NOT_OPEN_FILE -4
#define		E_FILE_NOT_WRITE	-5
#define		E_NOT_AN_INT		-6
#define		E_NOT_A_LONG		-7
#define		E_NOT_A_FLOAT		-8
#define		E_NOT_A_DOUBLE		-9

// These strings are not used anywhere.
static const char *HdrErrors[] =
{
	"OK",
	"Attempt to read a file that is not open as read",
	"Specified tag was not found",
	"File already open",
	"Could not open specified file",
	"Attempt to write to a file that is not open as write",
	"Attempt to read a none decimal digit in readint function",
	"Attempt to read a none decimal digit in readlong function",
	"Attempt to read a none decimal digit in readfloat function",
	"Attempt to read a none decimal digit in readdouble function"
};

class CHeader  
{
public:
	int Readdouble(char *tag, double *val); // Get a double value from memory table
	int Readfloat(char *tag, float *val);	// Get a float value from memory table
	int Readlong(char* tag, long* val);		// Get a long value from memory table
	int Readint(char* tag, int* val);		// Get a int value from memory table
	int WriteTag(const char* tag, const char* val);		// Put a string value in memory table
	int WriteTag(const char* tag, double val);	// Put a double value in memory table
	int WriteTag(const char* tag, int val);		// Put a int value in memory table
	int Readchar(char* tag, char* val, int len); // Get a string value from memory table 
	int CloseFile();
	void GetFileName(char* filename);
	int IsFileOpen();
	int OpenFile(char* filename);			// Loads specified filename in memory table
	int WriteFile(char *filename = 0);
	CHeader();
	virtual ~CHeader();

protected:
	int ReadFile();
	int InsertTag(char *buffer);
	bool SortData(char*HdrLine, char *tag, char* Data);  // Not in CHeader.hpp but not called.
	int m_FileOpen;	// 0 = close, 1 = openread, 2 = openwrite
	char m_FileName[256];
	FILE* hdr_file;
	char *tags[2048];
	char *data[2048];
	long numtags;
};
