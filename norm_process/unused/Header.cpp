// Header.cpp: implementation of the CHeader class.
//
//////////////////////////////////////////////////////////////////////
/*------------------------------------------------------------------
Modification History (HRRT User Community):
        04/30/09: Integrate Peter Bloomfield __linux__ support
-------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Header.h"



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHeader::CHeader()
{
	// initialize the file flag to zero
	m_FileOpen = 0;
	hdr_file = 0;
	numtags = 0;
	
}

CHeader::~CHeader()
{
	// close the file if user forgot to do so...
	if ((m_FileOpen != 0)&& (hdr_file != NULL))
		fclose(hdr_file);
}

/**
 * Loads specified filename in memory table.
 * Returns 1 if OK or a negative number (-ERR_CODE) otherwise.
 * Error codes:
 * HdrErrors[-ERR_CODE] contains the text error message.
 */
int CHeader::OpenFile(char *filename)
{
	// Check to see if the file is open
	if (m_FileOpen == 0)
	{
		hdr_file = fopen(filename, "r");
		if (hdr_file != NULL)
			m_FileOpen = 1;

		if (m_FileOpen != 0)
		{
			sprintf(m_FileName, "%s",filename);
			ReadFile();  // load the database
			return m_FileOpen;
		}
		else
		{
			return E_COULD_NOT_OPEN_FILE;
		}
	}
	// the file is open
	else
		return E_FILE_ALREADY_OPEN;
}

int CHeader::InsertTag(char *buffer)
{
	char *cptr,*cptr1;

	// remove trailing newline
	if ((cptr = strchr(buffer,'\n')))
		*cptr = '\0';

	// if this is not a comment
	if ((cptr = strstr(buffer,":=")))
	{
		cptr1 = cptr - 1;
		while (*cptr1 == ' ')
			cptr1--;
		cptr1++;
		*cptr1 = '\0';
		if (strcasecmp(buffer,"!Interfile") != 0)
		{
			tags[numtags] = strdup(buffer);
			cptr += 2;
			while (*cptr == ' ')
				cptr++;
			data[numtags] = strdup(cptr);
			numtags++;
		}
	}

	return 0;
}

int CHeader::ReadFile()
{
	char buffer[1024];
	while (fgets(buffer,1024,hdr_file))
		InsertTag(buffer);

	return 0;
}

int CHeader::WriteFile(char *fname) {
	FILE *fp;

	if (fname)  // new name has been selected
		fp = fopen(fname,"w");
	else
		fp = hdr_file;

	if (!fp)
		return 1;

	fprintf(fp,"!INTERFILE := \n");
	for (int i = 0; i < numtags; i++) {
		fprintf(fp,"%s := %s\n",tags[i],data[i]);
	}
	fclose(fp);
	return 0;
}

int CHeader::IsFileOpen() {
	return m_FileOpen;
}

void CHeader::GetFileName(char *filename) {
	// return the file name only if a file is open
	if (m_FileOpen != 0)
		sprintf(filename, "%s",m_FileName);

}

int CHeader::CloseFile() {
	// close the file
	if ((m_FileOpen != 0)&& (hdr_file != NULL)) {
		fclose(hdr_file);
		*m_FileName = '\0';
	}
	return OK;
}


int CHeader::Readchar(char *tag, char* val, int len) {
	for (int i = 0; i < numtags; i++) {
		if (strstr(tags[i],tag)) {
			strncpy(val,data[i],len);
			val[len-1] = '\0';
			return 0;
		}
	}
	return E_TAG_NOT_FOUND;
}

bool CHeader::SortData(char *HdrLine, char *tag, char *Data) {
	// 
	char str[256], sep[3];
	int str1len, str2len, x,i;
	str1len = strlen(HdrLine);
	str2len = strlen(tag);

	// is the tag longer than the line
	if (str1len<=str2len)
		return FALSE;
	//str = (char *)calloc(str2len,sizeof( char ));
	
	for(x=0;x<str2len;x++)
		str[x] = HdrLine[x];	
	str[x] = '\0';
	if (strcmp(str,tag) == 0)
	{
		// find the ":"
		char* pCol = strchr(HdrLine,':');
		x = str1len-strlen(pCol) - 1;
		//  the next text should be ":= "
		for (i=x;i<x+3;i++)
			sep[i-x] = HdrLine[i];
		sep[i-x] = '\0';
		if(strcmp(sep,":= ") == 0)
		{
			for(x=i;x<str1len;x++)
				Data[x-i] = HdrLine[x];
			// get rid of carraige return and replace with NULL
			Data[x-i-1] = '\0';
			return TRUE;
		}
		else
			return FALSE;
	}
	else
		return FALSE;
}

int CHeader::WriteTag(const char *tag, double val)
{
	char buffer[256];
	sprintf(buffer,"%f",val);
	return WriteTag(tag,buffer);
}

int CHeader::WriteTag(const char *tag, int val)
{
	char buffer[256];
	sprintf(buffer,"%d",val);
	return WriteTag(tag,buffer);
}


int CHeader::WriteTag(const char *tag, const char *val)
{
	for (int i = 0; i < numtags; i++)
		if (!strcmp(tag,tags[i])) // match?
		{
			free(data[i]);
			data[i] = strdup(val);
			return 0;
		}

	data[numtags] = strdup(val);
	tags[numtags] = strdup(tag);
	numtags++;

	return 1;
}

int CHeader::Readint(char *tag, int *val)
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
			return E_NOT_AN_INT;
		}
	}
	else
		return result;
}

int CHeader::Readlong(char *tag, long *val)
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

int CHeader::Readfloat(char *tag, float *val)
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

int CHeader::Readdouble(char *tag, double *val)
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
