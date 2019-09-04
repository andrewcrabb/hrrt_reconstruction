// CHeader.hpp: interface for the CHeader class.

#pragma once

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
// #include "boost/date_time/posix_time/posix_time.hpp"
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "hrrt_util.hpp"
#include "date_time.hpp"

using std::string;

enum class CHeaderError {
	OK                  =  0,
	FILE_NOT_READ       = -1,
	TAG_NOT_FOUND       = -2,
	FILE_ALREADY_OPEN   = -3,
	COULD_NOT_OPEN_FILE = -4,
	FILE_NOT_WRITE      = -5,
	NOT_AN_INT          = -6,
	NOT_A_LONG          = -7,
	NOT_A_FLOAT         = -8,
	NOT_A_DOUBLE        = -9,
	NOT_A_DATE          = -10,
	NOT_A_TIME          = -11,
	INVALID_DATE        = -12,
	BAD_LEXICAL_CAST    = -13,
	ERROR               = -14,
	TAG_APPENDED        = -15 // Not an error
};

static std::map<CHeaderError, string> CHdrErrorString = {
	{CHeaderError::OK                 , "OK"},
	{CHeaderError::FILE_NOT_READ      , "Attempt to read a file that is not open as read"},
	{CHeaderError::TAG_NOT_FOUND      , "Specified tag was not found"},
	{CHeaderError::FILE_ALREADY_OPEN  , "File already open"},
	{CHeaderError::COULD_NOT_OPEN_FILE, "Could not open specified file"},
	{CHeaderError::FILE_NOT_WRITE     , "Attempt to write to a file that is not open as write"},
	{CHeaderError::NOT_AN_INT         , "Not an int"},
	{CHeaderError::NOT_A_LONG         , "Not a long"},
	{CHeaderError::NOT_A_FLOAT        , "Not a float"},
	{CHeaderError::NOT_A_DOUBLE       , "Not a double"},
	{CHeaderError::NOT_A_DATE         , "Not a date"},
	{CHeaderError::NOT_A_TIME         , "Not a time"},
	{CHeaderError::INVALID_DATE       , "Invalid date"},
	{CHeaderError::BAD_LEXICAL_CAST   , "Bad lexical cast"},
	{CHeaderError::ERROR              , "Error (other)"},
	{CHeaderError::TAG_APPENDED       , "Tag was appended (not an error)"}
};

struct Tag {
	Tag(string k, string v) : key(k), value(v) {};
	string key;
	string value;
	string  sayit(void) const;
};
typedef std::vector<Tag>           tag_vector;
typedef std::vector<Tag>::iterator tag_iterator;
typedef std::vector<Tag>::const_iterator const_tag_iterator;

class CHeader {
public:
	CHeaderError ReadDouble(string const &key, double &val) const;
	CHeaderError ReadFloat (string const &key, float  &val) const;
	CHeaderError ReadLong  (string const &key, long   &val) const;
	CHeaderError ReadInt   (string const &key, int    &val) const;
	CHeaderError ReadChar  (string const &key, string &val)  const;
	CHeaderError ReadTime  (string const &key, std::unique_ptr<DateTime> &t_date_time) const;
	CHeaderError ReadDate  (string const &key, std::unique_ptr<DateTime> &t_date_time) const;
	CHeaderError ReadDateTime (string const &t_tag, DateTime::Format t_format, std::unique_ptr<DateTime> &t_date_time) const;

	CHeaderError WriteChar   (string const &key, string                  const & val);
	CHeaderError WritePath   (string const &key, boost::filesystem::path const & val);
	CHeaderError WriteDouble (string const &key, double  val);
	CHeaderError WriteFloat  (string const &key, float   val);
	CHeaderError WriteInt    (string const &key, int     val);
	CHeaderError WriteLong   (string const &key, int64_t val);

	CHeaderError WriteDate(string const &t_tag, string const &t_date_time_str);
	CHeaderError WriteDate(string const &t_tag, std::unique_ptr<DateTime> t_date_time);
	CHeaderError WriteTime(string const &t_tag, string const &t_date_time_str);
	CHeaderError WriteTime(string const &t_tag, std::unique_ptr<DateTime> t_date_time);

	CHeaderError CloseFile();
	CHeaderError GetFileName(string &filename) const;
	CHeaderError OpenFile(string const &filename);			// Loads specified filename in memory table
	CHeaderError OpenFile(boost::filesystem::path &filename);
	CHeaderError WriteFile(string &filename);
	CHeaderError WriteFile(boost::filesystem::path const &filename);
	int IsFileOpen() const;
	int NumTags(void) const;
	CHeader();
	virtual ~CHeader();

	static string const AXIAL_COMPRESSION           ;
	static string const AVERAGE_SINGLES_PER_BLOCK   ;
	static string const BRANCHING_FACTOR            ;
	static string const DATA_FORMAT                 ;
	static string const DEAD_TIME_CORRECTION_FACTOR ;
	static string const DECAY_CORRECTION_FACTOR     ;
	static string const DECAY_CORRECTION_FACTOR_2   ;
	static string const DOSAGE_STRENGTH             ;
	static string const DOSE_ASSAY_DATE             ;
	static string const DOSE_ASSAY_TIME             ;
	static string const DOSE_TYPE                   ;
	static string const ENERGY_WINDOW_LOWER_LEVEL   ;
	static string const ENERGY_WINDOW_UPPER_LEVEL   ;
	static string const FRAME                       ;
	static string const FRAME_DEFINITION            ;
	static string const HISTOGRAMMER_REVISION       ;
	static string const IMAGE_DURATION              ;
	static string const IMAGE_RELATIVE_START_TIME   ;
	static string const INTERFILE                   ;
	static string const ISOTOPE_HALFLIFE            ;
	static string const LM_REBINNER_METHOD          ;
	static string const LMHISTOGRAM_BUILD_ID        ;
	static string const LMHISTOGRAM_VERSION         ;
	static string const MATRIX_SIZE_1               ;
	static string const MATRIX_SIZE_2               ;
	static string const MATRIX_SIZE_3               ;
	static string const MAXIMUM_RING_DIFFERENCE     ;
	static string const NAME_OF_DATA_FILE           ;
	static string const NAME_OF_TRUE_DATA_FILE      ;
	static string const NUMBER_FORMAT               ;
	static string const NUMBER_OF_BYTES_PER_PIXEL   ;
	static string const NUMBER_OF_DIMENSIONS        ;
	static string const ORIGINATING_SYSTEM          ;
	static string const PATIENT_DOB                 ;
	static string const PATIENT_ID                  ;
	static string const PATIENT_NAME                ;
	static string const PATIENT_SEX                 ;
	static string const PET_DATA_TYPE               ;
	static string const PROGRAM_BUILD_ID            ;
	static string const SCALING_FACTOR_1            ;
	static string const SCALING_FACTOR_2            ;
	static string const SCALING_FACTOR_3            ;
	static string const SINOGRAM_DATA_TYPE          ;
	static string const SOFTWARE_VERSION            ;
	static string const STUDY_DATE                  ;
	static string const STUDY_TIME                  ;
	static string const TOTAL_NET_TRUES             ;
	static string const TOTAL_PROMPTS               ;
	static string const TOTAL_RANDOMS               ;

	// Test data
	static Tag const VALID_CHAR;
	static Tag const VALID_DOUBLE;
	static Tag const VALID_FLOAT;
	static Tag const VALID_INT;
	static Tag const VALID_DATE;
	static Tag const VALID_TIME;

protected:
	CHeaderError ReadFile();
	CHeaderError InsertTag(string buffer);
	tag_iterator       FindTag(string const &key);
	const_tag_iterator FindTag(string const &key) const;
	template <typename T>CHeaderError convertString(string &s, T &val) const;
	template <typename T>CHeaderError ReadNum(string const &tag, T &val) const;
	template <typename T>int ReadHeaderNum(string & filename, string & tag, T & val);

	// CHeaderError WriteDateTime(string const &t_tag, string const &t_format, boost::posix_time::ptime const &t_pt);

	CHeaderError WriteDateTime(string const &t_tag, DateTime::Format t_format, string const &t_date_time_str);
	CHeaderError WriteDateTime(string const &t_tag, DateTime::Format t_format, std::unique_ptr<DateTime> &t_date_time);

	// bool SortData(char*HdrLine, char *tag, char* Data);
	string m_FileName_;
	std::ifstream hdr_file_;
	tag_vector tags_;
};
