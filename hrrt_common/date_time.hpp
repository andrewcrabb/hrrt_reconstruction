// date_time.hpp
// 201903 ahc
// Class and methods to handle dates and times
// Stored internally as a Boost Posix time, interface mostly uses strings.

#include <boost/date_time.hpp>

namespace bt = boost::posix_time;

class DateTime {
public:
  enum class Format {
    ecat_date,     // 18:09:2017
    ecat_time,     // 15:26:00
    ecat_datetime  // 18:09:2017 15:26:00
  };

  static std::map<Format, std::string> formats_ = {
    {Format::ecat_date    , "%d:%m:%Y"},          // !study date (dd:mm:yryr) := 18:09:2017
    {Format::ecat_time    , "%H:%M:%S"},          // !study time (hh:mm:ss) := 15:26:00
    {Format::ecat_datetime, "%d:%m:%Y %H:%M:%S"}  //
  };

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

  void ParseDateTimeString(std::string const &t_datetime_str, Format t_format);
  std::string ToString(Format t_format) const;
  bool IsValid(void) const;
  bool ValidDateRange(void) const;


  // static ErrorCode ParseInterfileDatetime(const std::string &datestr, DateFormat t_format, boost::posix_time::ptime &pt);
  // static ErrorCode ParseInterfileTime(const std::string &timestr, boost::posix_time::ptime &pt);
  // static ErrorCode ParseInterfileDate(const std::string &datestr, boost::posix_time::ptime &pt);
  // static ErrorCode StringToPTime(string const &t_time, DateTime::Format t_format, boost::posix_time::ptime &t_datetime);
  // static ErrorCode PTimeToString(boost::posix_time::ptime const &t_time, string const &t_format, string &t_datetime);
  // static ErrorCode ValidDate(boost::posix_time::ptime const &t_datetime);

private:
  // Clients must call Create() and can't invoke the constructor directly.
  DateTime();

  boost::posix_time::ptime date_time_;

};