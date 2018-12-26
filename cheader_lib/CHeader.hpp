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

static std::map<CHeaderError, string> CHdrErrorString = {
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

struct Tag {
	Tag(string k, string v) : key(k), value(v) {};
	string key;
	string value;
	string  sayit(void) const;
};
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;

class CHeader {
public:
	CHeaderError Readdouble(string const &key, double &val);        // Get a double value from memory table
	CHeaderError Readfloat (string const &key, float  &val);	       // Get a float  value from memory table
	CHeaderError Readlong  (string const &key, long   &val);		   // Get a long   value from memory table
	CHeaderError Readint   (string const &key, int    &val);		   // Get a int    value from memory table
	CHeaderError Readchar  (string const &key, string &val);        // Get a string value from memory table 
	CHeaderError ReadTime  (string const &key, boost::posix_time::ptime &time);
	// CHeaderError WriteString (string const &key, string const &val);  // Put a string value in memory table
	// CHeaderError WriteChar   (string const &key, char const *val);  // Put a string value in memory table
	CHeaderError WriteChar   (string const &key, string const &val);  // Put a string value in memory table
	CHeaderError WritePath   (string const &key, boost::filesystem::path const &val);  // Put a string value in memory table
	CHeaderError WriteDouble (string const &key, double val);	       // Put a double value in memory table
	CHeaderError WriteFloat (string const &key, float val);	       // Put a float value in memory table
	CHeaderError WriteInt    (string const &key, int val);		       // Put a int    value in memory table
	CHeaderError WriteLong   (string const &key, int64_t);		       // Put a int64  value in memory table
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

static string const HDR_AXIAL_COMPRESSION           ;
static string const HDR_AVERAGE_SINGLES_PER_BLOCK   ;
static string const HDR_BRANCHING_FACTOR            ;
static string const HDR_DATA_FORMAT                 ;
static string const HDR_DEAD_TIME_CORRECTION_FACTOR ;
static string const HDR_DECAY_CORRECTION_FACTOR     ;
static string const HDR_DECAY_CORRECTION_FACTOR_2   ;
static string const HDR_DOSAGE_STRENGTH             ;
static string const HDR_DOSE_ASSAY_DATE             ;
static string const HDR_DOSE_ASSAY_TIME             ;
static string const HDR_DOSE_TYPE                   ;
static string const HDR_ENERGY_WINDOW_LOWER_LEVEL   ;
static string const HDR_ENERGY_WINDOW_UPPER_LEVEL   ;
static string const HDR_FRAME                       ;
static string const HDR_FRAME_DEFINITION            ;
static string const HDR_HISTOGRAMMER_REVISION       ;
static string const HDR_IMAGE_DURATION              ;
static string const HDR_IMAGE_RELATIVE_START_TIME   ;
static string const HDR_INTERFILE                   ;
static string const HDR_ISOTOPE_HALFLIFE            ;
static string const HDR_LM_REBINNER_METHOD          ;
static string const HDR_LMHISTOGRAM_BUILD_ID        ;
static string const HDR_LMHISTOGRAM_VERSION         ;
static string const HDR_MATRIX_SIZE_1               ;
static string const HDR_MATRIX_SIZE_2               ;
static string const HDR_MATRIX_SIZE_3               ;
static string const HDR_MAXIMUM_RING_DIFFERENCE     ;
static string const HDR_NAME_OF_DATA_FILE           ;
static string const HDR_NAME_OF_TRUE_DATA_FILE      ;
static string const HDR_NUMBER_FORMAT               ;
static string const HDR_NUMBER_OF_BYTES_PER_PIXEL   ;
static string const HDR_NUMBER_OF_DIMENSIONS        ;
static string const HDR_ORIGINATING_SYSTEM          ;
static string const HDR_PATIENT_DOB                 ;
static string const HDR_PATIENT_ID                  ;
static string const HDR_PATIENT_NAME                ;
static string const HDR_PATIENT_SEX                 ;
static string const HDR_PET_DATA_TYPE               ;
static string const HDR_PROGRAM_BUILD_ID            ;
static string const HDR_SCALING_FACTOR_1            ;
static string const HDR_SCALING_FACTOR_2            ;
static string const HDR_SCALING_FACTOR_3            ;
static string const HDR_SINOGRAM_DATA_TYPE          ;
static string const HDR_SOFTWARE_VERSION            ;
static string const HDR_STUDY_DATE                  ;
static string const HDR_STUDY_TIME                  ;
static string const HDR_TOTAL_NET_TRUES             ;
static string const HDR_TOTAL_PROMPTS               ;
static string const HDR_TOTAL_RANDOMS               ;

// Test data
static Tag const VALID_CHAR;
static Tag const VALID_DOUBLE;
static Tag const VALID_FLOAT;
static Tag const VALID_INT;

protected:
	CHeaderError ReadFile();
	CHeaderError InsertTag(string buffer);
	tag_iterator FindTag(string const &key) ;
	template <typename T>CHeaderError convertString(string &s, T &val);
	template <typename T>CHeaderError ReadNum(string const &tag, T &val);

	// bool SortData(char*HdrLine, char *tag, char* Data);
	string m_FileName_;
	std::ifstream hdr_file_;
	tag_vector tags_;
  	std::shared_ptr<spdlog::logger> logger_;


};
