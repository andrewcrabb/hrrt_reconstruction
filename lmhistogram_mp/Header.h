// Header.h: interface for the CHeader class.
//
//////////////////////////////////////////////////////////////////////
/*
   Modification history:
          24-JUL-2009: Use const char for keywords
*/
#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "boost/date_time/posix_time/posix_time.hpp"

using std::string;

const string HDR_INTERFILE = "!INTERFILE";
const string HDR_DEAD_TIME_CORRECTION_FACTOR = "Dead time correction factor";
const string HDR_DOSAGE_STRENGTH = "Dosage Strength";
const string HDR_BRANCHING_FACTOR = "branching factor";
const string HDR_ISOTOPE_HALFLIFE = "isotope halflife";
const string HDR_DOSE_TYPE = "Dose type";
const string HDR_PATIENT_SEX = "Patient sex";
const string HDR_PATIENT_DOB = "Patient DOB";
const string HDR_PATIENT_NAME = "Patient name";
const string HDR_FRAME_DEFINITION = "Frame definition";
const string HDR_IMAGE_DURATION = "image duration";
const string HDR_ENERGY_WINDOW_UPPER_LEVEL = "energy window upper level[1]";
const string HDR_ENERGY_WINDOW_LOWER_LEVEL = "energy window lower level[1]";
const string HDR_MAXIMUM_RING_DIFFERENCE = "maximum ring difference";
const string HDR_AXIAL_COMPRESSION = "axial compression";
const string HDR_PATIENT_ID = "Patient ID";
const string HDR_DATA_FORMAT = "data format";
const string HDR_PET_DATA_TYPE = "!PET data type";
const string HDR_STUDY_TIME = "!study time (hh:mm:ss)";
const string HDR_STUDY_DATE = "!study date (dd:mm:yryr)";
const string HDR_NAME_OF_DATA_FILE = "!name of data file";
const string HDR_ORIGINATING_SYSTEM = "!originating system";
const string HDR_DOSE_ASSAY_TIME   = "dose_assay_time (hh:mm:ss)";
const string HDR_DOSE_ASSAY_DATE = "dose_assay_date (dd:mm:yryr)";


struct Tag {
	std::string key;
	std::string value;
};
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;

class CHeader {
public:
	int Readdouble(const string &key, double &val);        // Get a double value from memory table
	int Readfloat (const string &key, float  &val);	       // Get a float  value from memory table
	int Readlong  (const string &key, long   &val);		   // Get a long   value from memory table
	int Readint   (const string &key, int    &val);		   // Get a int    value from memory table
	int ReadTime  (const string &key, boost::posix_time::ptime &time);
	int WriteTag  (const string &key, const string &val);  // Put a string value in memory table
	int WriteTag  (const string &key, double val);	       // Put a double value in memory table
	int WriteTag  (const string &key, int val);		       // Put a int    value in memory table
	int WriteTag  (const string &key, int64_t);		       // Put a int64  value in memory table
	int Readchar  (const string &key, string &val);        // Get a string value from memory table 
	int CloseFile();
	void GetFileName(string &filename);
	int IsFileOpen();
	int OpenFile(const string &filename);			// Loads specified filename in memory table
	int WriteFile(const string &filename, int p39_flag=0);
	CHeader();
	virtual ~CHeader();
protected:
	int ReadFile();
	// int InsertTag(char *buffer);
	int InsertTag(std::string buffer);
	tag_iterator FindTag(const string &key);
	template <typename T>int convertString(const string &s, T &val);
	template <typename T>int ReadNum(const string &tag, T &val);

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
