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
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "spdlog/spdlog.h"
// #include "spdlog/sinks/stdout_color_sinks.h"
// #include "spdlog/sinks/basic_file_sink.h"
#include "my_spdlog.hpp"
// Cast CHeaderError to its underlying type (int) for LOG_foo macros.  http://bit.ly/2PSzLJY
#include <type_traits>

using std::string;

enum class CHeaderError {
  E_OK                  =  0,
  E_FILE_NOT_READ	    = -1,
  E_TAG_NOT_FOUND       = -2,
  E_FILE_ALREADY_OPEN   = -3,
  E_COULD_NOT_OPEN_FILE = -4,
  E_FILE_NOT_WRITE	    = -5,
  E_NOT_AN_INT		    = -6,
  E_NOT_A_LONG		    = -7,
  E_NOT_A_FLOAT		    = -8,
  E_NOT_A_DOUBLE		= -9,
  E_NOT_A_TIME          = -10,
  E_BAD_LEXICAL_CAST    = -11,
  E_TAG_APPENDED        = -12 // Not an error
};


static std::map<CHeaderError, std::string> CHdrErrorString = {
	{CHeaderError::E_OK                 , "OK"},
	{CHeaderError::E_FILE_NOT_READ      , "Attempt to read a file that is not open as read"},
	{CHeaderError::E_TAG_NOT_FOUND      , "Specified tag was not found"},
	{CHeaderError::E_FILE_ALREADY_OPEN  , "File already open"},
	{CHeaderError::E_COULD_NOT_OPEN_FILE, "Could not open specified file"},
	{CHeaderError::E_FILE_NOT_WRITE     , "Attempt to write to a file that is not open as write"},
	{CHeaderError::E_NOT_AN_INT         , "Not an int"},
	{CHeaderError::E_NOT_A_LONG         , "Not a long"},
	{CHeaderError::E_NOT_A_FLOAT        , "Not a float"},
	{CHeaderError::E_NOT_A_DOUBLE       , "Not a double"},
	{CHeaderError::E_NOT_A_TIME         , "Not a time"},
	{CHeaderError::E_BAD_LEXICAL_CAST   , "Bad lexical cast"},
	{CHeaderError::E_TAG_APPENDED       , "Tag was appended (not an error)"}
};

string const HDR_AXIAL_COMPRESSION = "axial compression";
string const HDR_AVERAGE_SINGLES_PER_BLOCK = "average singles per block";
string const HDR_BRANCHING_FACTOR = "branching factor";
string const HDR_DATA_FORMAT = "data format";
string const HDR_DEAD_TIME_CORRECTION_FACTOR = "Dead time correction factor";
string const HDR_DECAY_CORRECTION_FACTOR = "decay correction factor";
string const HDR_DECAY_CORRECTION_FACTOR_2 = "decay correction factor2";
string const HDR_DOSAGE_STRENGTH = "Dosage Strength";
string const HDR_DOSE_ASSAY_DATE = "dose_assay_date (dd:mm:yryr)";
string const HDR_DOSE_ASSAY_TIME   = "dose_assay_time (hh:mm:ss)";
string const HDR_DOSE_TYPE = "Dose type";
string const HDR_ENERGY_WINDOW_LOWER_LEVEL = "energy window lower level[1]";
string const HDR_ENERGY_WINDOW_UPPER_LEVEL = "energy window upper level[1]";
string const HDR_FRAME = "frame";
string const HDR_FRAME_DEFINITION = "Frame definition";
string const HDR_HISTOGRAMMER_REVISION = "!histogrammer revision";
string const HDR_IMAGE_DURATION = "image duration";
string const HDR_IMAGE_RELATIVE_START_TIME = "image relative start time";
string const HDR_INTERFILE = "!INTERFILE";
string const HDR_ISOTOPE_HALFLIFE = "isotope halflife";
string const HDR_LM_rebinner_method = "!LM rebinner method";
string const HDR_LMHISTOGRAM_BUILD_ID = "!lmhistogram build ID";
string const HDR_LMHISTOGRAM_VERSION = "!lmhistogram version";
string const HDR_MATRIX_SIZE_1 = "matrix size [1]";
string const HDR_MATRIX_SIZE_2 = "matrix size [2]";
string const HDR_MATRIX_SIZE_3 = "matrix size [3]";
string const HDR_MAXIMUM_RING_DIFFERENCE = "maximum ring difference";
string const HDR_NAME_OF_DATA_FILE = "!name of data file";
string const HDR_NAME_OF_TRUE_DATA_FILE = "!name of true data file";
string const HDR_NUMBER_FORMAT = "number format";
string const HDR_NUMBER_OF_BYTES_PER_PIXEL = "number of bytes per pixel";
string const HDR_NUMBER_OF_DIMENSIONS = "number of dimensions";
string const HDR_ORIGINATING_SYSTEM = "!originating system";
string const HDR_PATIENT_DOB = "Patient DOB";
string const HDR_PATIENT_ID = "Patient ID";
string const HDR_PATIENT_NAME = "Patient name";
string const HDR_PATIENT_SEX = "Patient sex";
string const HDR_PET_DATA_TYPE = "!PET data type";
string const HDR_PROGRAM_BUILD_ID = ";program build ID";
string const HDR_SCALING_FACTOR_1 = "scaling factor (mm/pixel) [1]";
string const HDR_SCALING_FACTOR_2 = "scaling factor [2]";
string const HDR_SCALING_FACTOR_3 = "scaling factor (mm/pixel) [3]";
string const HDR_SINOGRAM_DATA_TYPE = "Sinogram data type";
string const HDR_SOFTWARE_VERSION = ";software version";
string const HDR_STUDY_DATE = "!study date (dd:mm:yryr)";
string const HDR_STUDY_TIME = "!study time (hh:mm:ss)";
string const HDR_TOTAL_NET_TRUES = "Total Net Trues";
string const HDR_TOTAL_PROMPTS = "Total Prompts";
string const HDR_TOTAL_RANDOMS = "Total Randoms";

struct Tag {
	std::string key;
	std::string value;
};
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;
typedef std::vector<Tag>::const_iterator tag_iterator_const;

class CHeader {
public:
	CHeaderError Readdouble(string const &key, double &val);        // Get a double value from memory table
	CHeaderError Readfloat (string const &key, float  &val);	       // Get a float  value from memory table
	CHeaderError Readlong  (string const &key, long   &val);		   // Get a long   value from memory table
	CHeaderError Readint   (string const &key, int    &val);		   // Get a int    value from memory table
	CHeaderError Readchar  (string const &key, string &val);        // Get a string value from memory table 
	CHeaderError ReadTime  (string const &key, boost::posix_time::ptime &time);
	CHeaderError WriteString  (string const &key, string const &val);  // Put a string value in memory table
	CHeaderError WriteChar  (string const &key, char const *val);  // Put a string value in memory table
	CHeaderError WritePath  (string const &key, boost::filesystem::path const &val);  // Put a string value in memory table
	CHeaderError WriteDouble  (string const &key, double val);	       // Put a double value in memory table
	CHeaderError WriteInt  (string const &key, int val);		       // Put a int    value in memory table
	CHeaderError WriteLong  (string const &key, int64_t);		       // Put a int64  value in memory table
	CHeaderError CloseFile();
	CHeaderError GetFileName(string &filename);
	CHeaderError OpenFile(string const &filename);			// Loads specified filename in memory table
	CHeaderError OpenFile(boost::filesystem::path &filename);
	CHeaderError WriteFile(string &filename);
	CHeaderError WriteFile(boost::filesystem::path const &filename);
	int IsFileOpen();
	int NumTags(void);
	template <typename T>int ReadHeaderNum(string &filename, string &tag, T &val);
	CHeader();
	virtual ~CHeader();
protected:
	CHeaderError ReadFile();
	CHeaderError InsertTag(std::string buffer);
	tag_iterator FindTag(string const &key) ;
	template <typename T>CHeaderError convertString(string &s, T &val);
	template <typename T>CHeaderError ReadNum(string const &tag, T &val);

	// bool SortData(char*HdrLine, char *tag, char* Data);
	std::string m_FileName_;
	std::ifstream hdr_file_;
	tag_vector tags_;
  	std::shared_ptr<spdlog::logger> logger_;


};
