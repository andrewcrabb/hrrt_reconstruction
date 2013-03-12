// Header.h: interface for the CHeader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_)
#define AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>

#define TRUE 1
#define FALSE 0

class CHeader  
{
public:
	int Readdouble(char *tag, double *val); // Get a double value from memory table
	int Readfloat(char *tag, float *val);	// Get a float value from memory table
	int Readlong(char* tag, long* val);		// Get a long value from memory table
	int Readint(char* tag, int* val);		// Get a int value from memory table
	int WriteTag(char* tag, const char* val);		// Put a string value in memory table
	int WriteTag(char* tag, double val);	// Put a double value in memory table
	int WriteTag(char* tag, int val);		// Put a int value in memory table
	int WriteTag(char* tag, __int64);		// Put a int64 value in memory table
	int Readchar(char* tag, char* val, int len); // Get a string value from memory table 
	int CloseFile();
	void GetFileName(char* filename);
	int IsFileOpen();
	int OpenFile(const char* filename);			// Loads specified filename in memory table
	int WriteFile(const char *filename = 0);
	CHeader();
	virtual ~CHeader();

protected:
	int ReadFile();
	int InsertTag(char *buffer);
	bool SortData(char*HdrLine, char *tag, char* Data);
	int m_FileOpen;	// 0 = close, 1 = openread, 2 = openwrite
	char m_FileName[256];
	FILE* hdr_file;
	char *tags[2048];
	char *data[2048];
	long numtags;
};

#endif // !defined(AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_)
