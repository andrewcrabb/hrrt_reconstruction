// Header.cpp: implementation of the CHeader class.
//
//      Copyright (C) CPS Innovations 2004 All Rights Reserved.
//////////////////////////////////////////////////////////////////////

/*
Modification history:
					01/07/2009 : Warnings cleaning
          24-JUL-2009: Use const char for keywords
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <ctype.h>
#include <iostream>
#include "Header.h"
#include "Errors.h"
#include <hrrt_util.h>

#include <boost/xpressive/xpressive.hpp>


using std::cin;
using std::cout;
using std::endl;
using std::string;

#ifdef WIN32
#define strcasecmp _stricmp
#define strdup _strdup
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

enum KEYVAL { KEY, VAL };

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeader::CHeader()
{
	// initialize the file flag to zero
	// m_FileOpen = 0;
	// hdr_file = 0;
	// numtags = 0;
	
}

CHeader::~CHeader()
{
	// close the file if user forgot to do so...
	// if ((m_FileOpen != 0)&& (hdr_file != NULL))
	// 	fclose(hdr_file);
	if (hdr_file.is_open())
		hdr_file.close();
}

/**
 * Loads specified filename in memory table.
 * Returns 1 if OK or a negative number (-ERR_CODE) otherwise.
 * Error codes:
 * HdrErrors[-ERR_CODE] contains the text error message.
 */
// int CHeader::OpenFile(char *filename) {
// 	// Check to see if the file is open
// 	if (m_FileOpen == 0) {
// 		hdr_file = fopen(filename, "r");
// 		if (hdr_file != NULL)
// 			m_FileOpen = 1;

// 		if (m_FileOpen != 0) {
// 			sprintf(m_FileName, "%s",filename);
// 			ReadFile();  // load the database
// 			return m_FileOpen;
// 		} else {
// 			return E_COULD_NOT_OPEN_FILE;
// 		}
// 	} else {
// 	// the file is open
// 		return E_FILE_ALREADY_OPEN;
// 	}
// }

int CHeader::OpenFile(char *filename) {
	if (hdr_file.is_open())
		return(E_FILE_ALREADY_OPEN);
	hdr_file.open(filename, std::ifstream::in);
	if (!hdr_file.is_open())
		return(E_COULD_NOT_OPEN_FILE);
	m_FileName.assign(filename);
	return ReadFile();
}

// int CHeader::InsertTag(char *buffer)
// {
// 	char *cptr,*cptr1;

// 	// remove trailing newline
// 	if ((cptr = strchr(buffer,'\n')))
// 		*cptr = '\0';

// 	// if this is not a comment
// 	if ((cptr = strstr(buffer,":=")))
// 	{
// 		cptr1 = cptr - 1;
// 		while (*cptr1 == ' ')
// 			cptr1--;
// 		cptr1++;
// 		*cptr1 = '\0';
// 		if (strcasecmp(buffer,"!Interfile") != 0)
// 		{
// 			tags[numtags] = strdup(buffer);
// 			cptr += 2;
// 			while (*cptr == ' ')
// 				cptr++;
// 			data[numtags] = strdup(cptr);
// 			numtags++;
// 		}
// 	}

// 	return 0;
// }

int CHeader::InsertTag(string t_buffer) {
	cout << "CHeader::Inserttag(" << t_buffer << ")" << endl;
  using namespace boost::xpressive;

  smatch match;
  sregex m_reg = sregex::compile("^\\s*(?P<key>.+)\\s+:=\\s+(?P<value>.+)\\s*$");
  if (regex_match(t_buffer, match, m_reg)) {
    tags.push_back({match["key"], match["value"]});
    // cout << "tag: '" << match["tag"] << "'" << endl;
    // cout << "val: '" << match["val"] << "'" << endl;
    // string m_val{match["val"]};
    // tags.push_back(string{match["tag"]});
    // data.push_back(string{match["val"]});
    // tags[numtags] = strdup(m_tag.c_str());
    // data[numtags] = strdup(m_val.c_str());
    // numtags++;
}

	// char *cptr,*cptr1;
	// if ((cptr = strstr(buffer,":="))) {
	// 	// this is not a comment
	// 	cptr1 = cptr - 1;
	// 	while (*cptr1 == ' ')
	// 		cptr1--;
	// 	cptr1++;
	// 	*cptr1 = '\0';
	// 	if (strcasecmp(buffer,"!Interfile") != 0)
	// 	{
	// 		tags[numtags] = strdup(buffer);
	// 		cptr += 2;
	// 		while (*cptr == ' ')
	// 			cptr++;
	// 		data[numtags] = strdup(cptr);
	// 		numtags++;
	// 	}
	// }

	return 0;
}

int CHeader::ReadFile()
{
	// char buffer[1024];
	// while (fgets(buffer,1024,hdr_file))
	// 	InsertTag(buffer);
	string buffer;
	while (safeGetline(hdr_file, buffer))
		InsertTag(buffer);

	return 0;
}

// int CHeader::WriteFile(char *fname, int p39_flag)
// {
// 	FILE *fp;

// 	if (fname)  // new name has been selected
// 		fp = fopen(fname,"w");
// 	else
// 		fp = hdr_file;

// 	if (!fp) {
// 		return 1;
// 	}

// 	fprintf(fp,"!INTERFILE := \n");
// 	for (int i = 0; i < numtags; i++) {
// 		// Ignore Frame definition in P39 headers b/c unsupported by e7_tools
// 		if (p39_flag && strcmp(tags[i], "Frame definition")==0) continue;
// 		fprintf(fp,"%s := %s\n",tags[i],data[i]);
// 	}
// 	fclose(fp);

// 	return 0;
// }

int CHeader::WriteFile(char *fname, int p39_flag) {
	string outname = (fname) ? fname : m_FileName;
	std::ofstream fp;

	fp.open(outname, std::ofstream::out);
	if (!fp.is_open()) {
		return 1;
	}

	fp << "!INTERFILE := \n";
	for (auto &tag : tags) {
		// Ignore Frame definition in P39 headers b/c unsupported by e7_tools
		if (p39_flag && !(tag[KEY].compare("Frame definition")))
			continue;
		fp << tag[KEY] << " := " << tag[VAL] << endl;
	}
	fp.close();

	return 0;
}

int CHeader::IsFileOpen() {
	return hdr_file.is_open();
}

void CHeader::GetFileName(char *filename) {
	// return the file name only if a file is open
	// if (m_FileOpen != 0)
	// 	sprintf(filename, "%s",m_FileName);
	if (IsFileOpen())
		strcpy(filename, m_FileName.c_str());
}

int CHeader::CloseFile() {
	if (IsFileOpen()) {
		hdr_file.close();
		m_FileName.erase();
	}
	return OK;
}

// Locate tag having given key

std::vector<Tag>::iterator FindTag(const string key) {
	auto pred = [key](const Tag & tag) { return tag.key == key; };
	return std::find_if(std::begin(tags), std::end(tags), pred);
}

// Fill in val if tag is found.

int CHeader::Readchar(const char *key, char* val, int len) {
	int ret = 0;
	std::vector<Tag>::iterator it = FindTag(key);
	if (it == std::end(tags)) {
		ret = E_TAG_NOT_FOUND;
	} else {
		strcpy(val, it->value.c_str());
	}
	return ret;
}

// int CHeader::Readchar(const char *tag, char* val, int len) {
// 	for (int i = 0; i < numtags; i++) {
// 		std::cout << "XXX tag " << tag << " comparing to " << tags[i] << std::endl;
// 		if (strstr(tags[i],tag)) {
// 			std::cout << "XXX tag " << tag << " found " << tags[i] << std::endl;
// 			strncpy(val, data[i], len);
// 			val[len-1] = '\0';
// 			std::cout << "XXX Readchar(" << tag << ") returning '" << val << "'" << std::endl;
// 			return 0;
// 		}
// 	}
// 	return E_TAG_NOT_FOUND;
// }

// As far as I can tell, this awful method is never called.

// bool CHeader::SortData(char *HdrLine, char *tag, char *Data)
// {
// 	char str[256], sep[3];
// 	int hdr_len, tag_len, x,i;
// 	hdr_len = strlen(HdrLine);
// 	tag_len = strlen(tag);

// 	// is the tag longer than the line
// 	if (hdr_len<=tag_len)
// 		return FALSE;
// 	//str = (char *)calloc(tag_len,sizeof( char ));
	
// 	for(x=0;x<tag_len;x++)
// 		str[x] = HdrLine[x];	
// 	str[x] = '\0';
// 	if (strcmp(str,tag) == 0) {
// 		// find the ":"
// 		// char *pCol = strchr(HdrLine,":=");
// 		// ahc strchr must take an int not a string.
// 		// I think you should search for '=' not ':'
// 		char *pCol = strchr(HdrLine, '=');
// 		std::cerr << "================================================================================" << endl;
// 		std::cerr << "*** NOTE Cheader::SortData check correct pcol " << pCol << ", tag " << tag << endl;
// 		std::cerr << "================================================================================" << endl;
// 		x = hdr_len - strlen(pCol) - 1;
// 		//  the next text should be ":= "
// 		for (i = x; i < x + 3; i++)
// 			sep[i - x] = HdrLine[i];
// 		sep[i - x] = '\0';
// 		if(strcmp(sep, ":= ") == 0) {
// 			for(x=i;x<hdr_len;x++)
// 				Data[x-i] = HdrLine[x];
// 			// get rid of carraige return and replace with NULL
// 			Data[x-i-1] = '\0';
// 			return TRUE;
// 		}
// 		else
// 			return FALSE;
// 	}
// 	else
// 		return FALSE;
// }

int CHeader::WriteTag(const char *key, double val) {
	char buffer[256];
	sprintf(buffer,"%f",val);
	return WriteTag(key,buffer);
}

int CHeader::WriteTag(const char *key, int val) {
	char buffer[256];
	sprintf(buffer,"%d",val);
	return WriteTag(key,buffer);
}

int CHeader::WriteTag(const char *key, __int64 val) {
	char buffer[256];
	sprintf(buffer,"%I64d",val);
	return WriteTag(key,buffer);
}

// If tag with given key is found, update its value.
// Else append new tag with given key and value.

int CHeader::WriteTag(const char *key, const char *val) {
	int ret = 0;
	std::vector<Tag>::iterator it = FindTag(key);
	if (it == std::end(tags)) {
		tags.push_back(key, val);
		ret = 1;
	} else {
		it->value.assign(val);
	}
	return ret;

	// for (int i = 0; i < numtags; i++)
	// 	if (!strcmp(key,tags[i])) {
	// 		free(data[i]);
	// 		data[i] = strdup(val);
	// 		return 0;
	// 	}

	// data[numtags] = strdup(val);
	// tags[numtags] = strdup(key);
	// numtags++;

	// return 1;
}

int CHeader::Readint(const char *tag, int *val)
{
	int result;
	char str[256];
	strcpy(str,"");
	result = Readchar(tag, str,256 );
	if (result == 0)
	{
		int a = isdigit(str[0]);
		if (a != 0)
		{
			*val = atoi(str);
			return OK;
		}
		else
		{
			*val = 0;
			return E_NOT_AN_INIT;
		}
	}
	else
		return result;
}

int CHeader::Readlong(const char *tag, long *val)
{
	
	int result;
	char str[256];
	result = Readchar(tag, str,256 );
	if (result == 0)
	{
		int a = isdigit(str[0]);
		if (a != 0)
		{
			*val = atol(str);
			return OK;
		}
		else
		{
			*val = 0;
			return E_NOT_A_LONG;
		}
	}
	else
		return result;
}

int CHeader::Readfloat(const char *tag, float *val)
{
	int result;
	char str[256];
	result = Readchar(tag, str,256 );
	if (result == 0)
	{
		int a = isdigit(str[0]);
		if (a != 0)
		{
			*val = (float)atof(str);
			return OK;
		}
		else
		{
			*val = 0;
			return E_NOT_A_FLOAT;
		}
	}
	else
		return result;
}

int CHeader::Readdouble(const char *tag, double *val)
{
	int result;
	char str[256];
	result = Readchar(tag, str,256 );
	if (result == 0)
	{
		int a = isdigit(str[0]);
		if (a != 0)
		{
			*val = atof(str);
			return OK;
		}
		else
		{
			*val = 0;
			return E_NOT_A_DOUBLE;
		}
	}
	else
		return result;
}
