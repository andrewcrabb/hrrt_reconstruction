// CHeader.h: interface for the CHeader class.
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
#include <boost/filesystem.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"


using std::string;

string const HDR_INTERFILE = "!INTERFILE";
string const HDR_DEAD_TIME_CORRECTION_FACTOR = "Dead time correction factor";
string const HDR_DOSAGE_STRENGTH = "Dosage Strength";
string const HDR_BRANCHING_FACTOR = "branching factor";
string const HDR_ISOTOPE_HALFLIFE = "isotope halflife";
string const HDR_DOSE_TYPE = "Dose type";
string const HDR_PATIENT_SEX = "Patient sex";
string const HDR_PATIENT_DOB = "Patient DOB";
string const HDR_PATIENT_NAME = "Patient name";
string const HDR_FRAME_DEFINITION = "Frame definition";
string const HDR_IMAGE_DURATION = "image duration";
string const HDR_ENERGY_WINDOW_UPPER_LEVEL = "energy window upper level[1]";
string const HDR_ENERGY_WINDOW_LOWER_LEVEL = "energy window lower level[1]";
string const HDR_MAXIMUM_RING_DIFFERENCE = "maximum ring difference";
string const HDR_AXIAL_COMPRESSION = "axial compression";
string const HDR_PATIENT_ID = "Patient ID";
string const HDR_DATA_FORMAT = "data format";
string const HDR_PET_DATA_TYPE = "!PET data type";
string const HDR_STUDY_TIME = "!study time (hh:mm:ss)";
string const HDR_STUDY_DATE = "!study date (dd:mm:yryr)";
string const HDR_NAME_OF_DATA_FILE = "!name of data file";
string const HDR_ORIGINATING_SYSTEM = "!originating system";
string const HDR_DOSE_ASSAY_TIME   = "dose_assay_time (hh:mm:ss)";
string const HDR_DOSE_ASSAY_DATE = "dose_assay_date (dd:mm:yryr)";
string const HDR_SOFTWARE_VERSION = ";software version";
string const HDR_PROGRAM_BUILD_ID = ";program build ID";
string const HDR_NUMBER_FORMAT = "number format";
string const HDR_NUMBER_OF_DIMENSIONS = "number of dimensions";
string const HDR_NUMBER_OF_BYTES_PER_PIXEL = "number of bytes per pixel";
string const HDR_MATRIX_SIZE_1 = "matrix size [1]";
string const HDR_MATRIX_SIZE_2 = "matrix size [2]";
string const HDR_MATRIX_SIZE_3 = "matrix size [3]";
string const HDR_SCALING_FACTOR_1 = "scaling factor (mm/pixel) [1]";
string const HDR_SCALING_FACTOR_2 = "scaling factor [2]";
string const HDR_SCALING_FACTOR_3 = "scaling factor (mm/pixel) [3]";
string const HDR_IMAGE_RELATIVE_START_TIME = "image relative start time";
string const HDR_DECAY_CORRECTION_FACTOR = "decay correction factor";
string const HDR_DECAY_CORRECTION_FACTOR_2 = "decay correction factor2";
string const HDR_FRAME = "frame";
string const HDR_TOTAL_PROMPTS = "Total Prompts";
string const HDR_TOTAL_RANDOMS = "Total Randoms";
string const HDR_TOTAL_NET_TRUES = "Total Net Trues";
string const HDR_SINOGRAM_DATA_TYPE = "Sinogram data type";

struct Tag {
	std::string key;
	std::string value;
};
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;
typedef std::vector<Tag>::const_iterator tag_iterator_const;

class CHeader {
public:
	int Readdouble(string &key, double &val);        // Get a double value from memory table
	int Readfloat (string &key, float  &val);	       // Get a float  value from memory table
	int Readlong  (string &key, long   &val);		   // Get a long   value from memory table
	int Readint   (string &key, int    &val);		   // Get a int    value from memory table
	int Readchar  (string &key, string &val);        // Get a string value from memory table 
	int ReadTime  (string &key, boost::posix_time::ptime &time);
	int WriteTag  (string &key, string &val);  // Put a string value in memory table
	int WriteTag  (string &key, char const *val);  // Put a string value in memory table
	int WriteTag  (string &key, boost::filesystem::path const &val);  // Put a string value in memory table
	int WriteTag  (string &key, double val);	       // Put a double value in memory table
	int WriteTag  (string &key, int val);		       // Put a int    value in memory table
	int WriteTag  (string &key, int64_t);		       // Put a int64  value in memory table
	int CloseFile();
	void GetFileName(string &filename);
	int IsFileOpen();
	int OpenFile(string const &filename);			// Loads specified filename in memory table
	int OpenFile(boost::filesystem::path &filename);
	// int WriteFile(string const &filename, int p39_flag=0);
	int WriteFile(string &filename);
	int WriteFile(boost::filesystem::path const &filename);
	template <typename T>int ReadHeaderNum(string &filename, string &tag, T &val);
	CHeader();
	virtual ~CHeader();
protected:
	int ReadFile();
	int InsertTag(std::string buffer);
	tag_iterator FindTag(string &key) ;
	template <typename T>int convertString(string &s, T &val);
	template <typename T>int ReadNum(string &tag, T &val);

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
	tag_vector tags_;
	long numtags;
	// extern std::shared_ptr<spdlog::logger> g_logger;

};
