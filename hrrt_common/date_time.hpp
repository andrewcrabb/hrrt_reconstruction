// date_time.hpp
// 201903 ahc
// Class and methods to handle dates and times
// Stored internally as a Boost Posix time, interface mostly uses strings.

#pragma once

#include <boost/date_time.hpp>
#include <boost/xpressive/xpressive.hpp>

namespace bt = boost::posix_time;
namespace bx = boost::xpressive;

class DateTime {
public:
  enum class Format {
    ecat_date,     // 18:09:2017
    ecat_time,     // 15:26:00
    ecat_datetime  // 18:09:2017 15:26:00
  };

  // Precompile patterns for checking date strings.
  // Necessary since parsing '30:12:2009 15:50:00' with '%H:%M:%S' gives '1400-Jan-02 06:12:20' instead of error.
  struct FormatElement {
    std::string              format_string;   // '%H:%M:%S'
    boost::xpressive::sregex format_pattern;  // '^\d{1,2}:\d{1,2}:\d{1,2}$'
  };

  struct TestData {
    std::string date_time_str;
    Format format;
    bool valid;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
  };

// static const std::map<Format, std::string> formats_;
static const std::map<Format, FormatElement> formats_;
static const std::vector<TestData> test_data_;

  // enum class ErrorCode {
  //   OK,
  //   NOT_A_DATE,
  //   NOT_A_TIME,
  //   NOT_A_DATETIME,
  //   INVALID_DATE,
  //   INVALID_TIME,
  //   INVALID_DATETIME
  // };

  // Instance methods

  // Class methods
  // https://abseil.io/tips/42
  // Factory method: creates and returns a DateTime. Returns null on failure.
  static std::unique_ptr<DateTime> Create(std::string const &t_datetime_str, Format t_format);
  static std::string FormatString(Format t_format);
  static bx::sregex FormatPattern(Format t_format);

  void ParseDateTimeString(std::string const &t_datetime_str, Format t_format);
  std::string ToString(Format t_format) const;
  boost::posix_time::ptime get_date_time(void) const;
  bool IsValid(void) const;
  bool ValidDateRange(void) const;
  bool operator==(DateTime const &other) const;

  // static ErrorCode ParseInterfileDatetime(const std::string &datestr, DateFormat t_format, boost::posix_time::ptime &pt);
  // static ErrorCode ParseInterfileTime(const std::string &timestr, boost::posix_time::ptime &pt);
  // static ErrorCode ParseInterfileDate(const std::string &datestr, boost::posix_time::ptime &pt);
  // static ErrorCode StringToPTime(string const &t_time, DateTime::Format t_format, boost::posix_time::ptime &t_datetime);
  // static ErrorCode PTimeToString(boost::posix_time::ptime const &t_time, string const &t_format, string &t_datetime);
  // static ErrorCode ValidDate(boost::posix_time::ptime const &t_datetime);

private:
  // Clients must call Create() and can't invoke the constructor directly.
  DateTime() {};

  boost::posix_time::ptime date_time_;

};