// Header.h: interface for the CHeader class.
//
//////////////////////////////////////////////////////////////////////
/*
   Modification history:
          24-JUL-2009: Use const char for keywords

*/
#if !defined(AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_)
#define AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>

// ahc
#include <iostream>
#include <fstream>
#include <string>

#define TRUE 1
#define FALSE 0

#ifndef WIN32
typedef long long __int64;
#endif

typedef struct {
	std::string key;
	std::string value;
} Tag;

class CHeader {
public:
	int Readdouble(const char *key, double *val);          // Get a double value from memory table
	int Readfloat( const char *key, float *val);	       // Get a float  value from memory table
	int Readlong(  const char *key, long* val);		       // Get a long   value from memory table
	int Readint(   const char *key, int* val);		       // Get a int    value from memory table
	int WriteTag(  const char *key, const char* val);	   // Put a string value in memory table
	int WriteTag(  const char *key, double val);	       // Put a double value in memory table
	int WriteTag(  const char *key, int val);		       // Put a int    value in memory table
	int WriteTag(  const char *key, __int64);		       // Put a int64  value in memory table
	int Readchar(  const char *key, char* val, int len);   // Get a string value from memory table 
	int CloseFile();
	void GetFileName(char* filename);
	int IsFileOpen();
	int OpenFile(char* filename);			// Loads specified filename in memory table
	int WriteFile(char *filename = 0, int p39_flag=0);
	CHeader();
	virtual ~CHeader();
	// ahc
	std::vector<Tag>::iterator FindTag(const string key);

protected:
	int ReadFile();
	// int InsertTag(char *buffer);
	int InsertTag(std::string buffer);
	// bool SortData(char*HdrLine, char *tag, char* Data);
	// int m_FileOpen;	// 0 = close, 1 = openread, 2 = openwrite
	// char m_FileName[256];
	std::string m_FileName;
	// FILE* hdr_file;
	std::ifstream hdr_file;
	// char *tags[2048];
	// char *data[2048];
	// std::vector<std::string> tags;
	// std::vector<std::string> data;
	// std::vector<std::array<std::string, 2>> tagpairs;
	std::vector<Tag> tags;
	long numtags;
};

#endif // !defined(AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_)
