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

typedef struct {
	std::string key;
	std::string value;
} Tag;
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;

class CHeader {
public:
	int Readdouble(const string &key, double &val);        // Get a double value from memory table
	int Readfloat (const string &key, float  &val);	       // Get a float  value from memory table
	int Readlong  (const string &key, long   &val);		   // Get a long   value from memory table
	int Readint   (const string &key, int    &val);		   // Get a int    value from memory table
	int WriteTag(  const string &key, const string &val);  // Put a string value in memory table
	int WriteTag(  const string &key, double val);	       // Put a double value in memory table
	int WriteTag(  const string &key, int val);		       // Put a int    value in memory table
	int WriteTag(  const string &key, int64_t);		       // Put a int64  value in memory table
	int Readchar(  const string &key, string &val);        // Get a string value from memory table 
	int CloseFile();
	void GetFileName(string &filename);
	int IsFileOpen();
	int OpenFile(const string &filename);			// Loads specified filename in memory table
	int WriteFile(const string &filename, int p39_flag=0);
	CHeader();
	virtual ~CHeader();
	// ahc
	std::vector<Tag>::iterator FindTag(const string &key);

protected:
	int ReadFile();
	// int InsertTag(char *buffer);
	int InsertTag(std::string buffer);
	template <typename T> int convertString(const string &s, T &val);
	template <typename T>int CHeader::ReadNum(const string &tag, T &val);

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
	tag_vector tags;
	long numtags;
};

#endif // !defined(AFX_HEADER_H__CD62988B_7F4D_4EE6_95CF_27C8C0B834C7__INCLUDED_)
