// date_time.cpp
// 201903 ahc
// C++ patterns/practices I am implementing in this class:
//   Returning types rather than return codes
//   Factory constructor method

#include "date_time.hpp"
#include "my_spdlog.hpp"
#include "hrrt_util.hpp"

namespace bt = boost::posix_time;
namespace bx = boost::xpressive;

// namespace df = DateTime::Format;

// const std::map<DateTime::Format, std::string> DateTime::formats_ = {
//     {Format::ecat_date    , "%d:%m:%Y"},          // !study date (dd:mm:yryr) := 18:09:2017
//     {Format::ecat_time    , "%H:%M:%S"},          // !study time (hh:mm:ss) := 15:26:00
//     {Format::ecat_datetime, "%d:%m:%Y %H:%M:%S"}  //
//   };

const std::map<DateTime::Format, DateTime::FormatElement> DateTime::formats_ = {
    {Format::ecat_date    , {"%d:%m:%Y"         , bx::sregex::compile("^\\d{2}:\\d{2}:\\d{4}$")                     }},  // !study date (dd:mm:yryr) := 18:09:2017
    {Format::ecat_time    , {"%H:%M:%S"         , bx::sregex::compile("^\\d{2}:\\d{2}:\\d{2}$")                     }},  // !study time (hh:mm:ss) := 15:26:00
    {Format::ecat_datetime, {"%d:%m:%Y %H:%M:%S", bx::sregex::compile("^\\d{2}:\\d{2}:\\d{4} \\d{2}:\\d{2}:\\d{2}") }}
  };

const std::vector<DateTime::TestData> DateTime::test_data_ = {
  // Valid datetime-format combinations
    {"18:09:2017"           , DateTime::Format::ecat_date    , true , 2017, 9, 18, 0 , 0 , 0},
    {"18:09:2017 15:26:00"  , DateTime::Format::ecat_datetime, true , 2017, 9, 18, 15, 26, 0},
    // Invalid date-times or formats
    {"09:18:2017"           , DateTime::Format::ecat_datetime, false, 2017, 9, 18, 0 , 0 , 0},
    {"09:18:2017 15:26:00"  , DateTime::Format::ecat_datetime, false, 2017, 9, 18, 15, 26, 0}
  };

bool DateTime::operator==(DateTime const &other) const {
  bool ret = (date_time_ == other.get_date_time());
  LOG_DEBUG("this {} other {}: returning {}", bt::to_simple_string(date_time_), bt::to_simple_string(other.get_date_time()), hrrt_util::BoolToString(ret));
  return ret;
}

// Factory method to create and initialize a DateTime
// Returns nullptr on failure.

std::unique_ptr<DateTime> DateTime::Create(std::string const &t_datetime_str, DateTime::Format t_format) {
  // std::unique_ptr<DateTime> date_time = std::make_unique<DateTime>();
  std::unique_ptr<DateTime> date_time(new DateTime());
  date_time->ParseDateTimeString(t_datetime_str, t_format);
  // return  ? date_time : std::unique_ptr<DateTime>(nullptr);
  if (!date_time->IsValid())
    return nullptr;
  return date_time;
}

std::string DateTime::FormatString(Format t_format) {
  return DateTime::formats_.at(t_format).format_string;
}

bx::sregex DateTime::FormatPattern(Format t_format) {
  return DateTime::formats_.at(t_format).format_pattern;
}

/**
 * @brief      Parse DateTime-compatible date-time string into Boost ptime member
 *
 * @param[in]  t_datestr   Date-Time string
 * @param[in]  t_format    Format index into formats_
 *
 * @return     void
 */
void DateTime::ParseDateTimeString(std::string const &t_datetime_str, DateTime::Format t_format) {
  // Ensure that t_datetime_str exactly matches t_format
  bx::smatch match;
  if (!bx::regex_match(t_datetime_str, match, DateTime::formats_.at(t_format).format_pattern)) {
    LOG_ERROR("datetime_string {} does not match {}", t_datetime_str, DateTime::formats_.at(t_format).format_string);
    return;
  }

  std::string format_string = formats_.at(t_format).format_string;
  bt::time_input_facet *facet = new bt::time_input_facet(format_string);
  const std::locale loc(std::locale::classic(), facet);
  std::istringstream iss(t_datetime_str);
  iss.imbue(loc);
  // bt::ptime posix_time;
  iss >> date_time_;
  if (date_time_.is_not_a_date_time()) {
    LOG_ERROR("Not a date_time: datestr {}, format {}", t_datetime_str, DateTime::formats_.at(t_format).format_string);
  } else {
    LOG_DEBUG("t_datetime_str {} format_string {} gives {}", t_datetime_str, format_string, bt::to_simple_string(date_time_));
  }
  // DateTime:: ret = t_pt.is_not_a_date_time();
  // std::string timestr("FILLMEIN");
  return;
}

// Return date/time to string in given format.
// TODO: This doesn't do any error checking

std::string DateTime::ToString(DateTime::Format t_format) const {
  bt::time_facet *time_facet = new bt::time_facet();
  time_facet->format(formats_.at(t_format).format_string.c_str());
  std::ostringstream oss;
  oss.imbue(std::locale(oss.getloc(), time_facet));
  oss << date_time_;
  std::string datetime = oss.str();
  LOG_TRACE("ptime {} format {} returning {}", bt::to_iso_string(date_time_), formats_.at(t_format).format_string, datetime);
  return datetime;
}

bool DateTime::IsValid(void) const {
  return !date_time_.is_not_a_date_time();
}

// Check that given date is between 1900 and 2100.

bool DateTime::ValidDateRange(void) const {
  bt::ptime low_time( boost::gregorian::date(1900,01,01), bt::time_duration( 0,0,0));
  bt::ptime high_time(boost::gregorian::date(2099,12,31), bt::time_duration(24,0,0));

  bool ret = ((date_time_ > low_time) && (date_time_ < high_time));
  LOG_DEBUG("datetime {} returning {}", ToString(Format::ecat_datetime), hrrt_util::BoolToString(ret));
  return ret;
}

boost::posix_time::ptime DateTime::get_date_time(void) const {
  return date_time_;
}

// ------------- Methods below here are from the first implementation and can be re-implemented or deleted ---------------------
/*
static DateTime::ErrorCode DateTime::ParseInterfileDate(std::string const &datestr, bt::ptime &pt) {
  return ParseInterfileDatetime(datestr, DateFormat::ecat_date, pt);
}

static DateTime::ErrorCode DateTime::ParseInterfileTime(std::string const &timestr, bt::ptime &pt) {
  return ParseInterfileDatetime(timestr, DateFormat::ecat_time, pt);
}


// Convert datetime string to a ptime
// Can't convert a time by itself: must be a datetime

static DateTime::ErrorCode DateTime::StringToPTime(string const &t_datestr, string const &t_format, bt::ptime &t_datetime) {
  bt::time_input_facet *tfacet = new bt::time_input_facet();
  tfacet->format(t_format.c_str());
  std::istringstream iss(t_datestr);
  iss.imbue(std::locale(iss.getloc(), tfacet));
  // bt::ptime the_datetime;
  iss >> t_datetime;
  ErrorCode ret = ErrorCode::OK;
  if (t_datetime.is_not_a_date_time()) {
    ret = ErrorCode::INVALID_DATE;
    LOG_ERROR("datestr {} format {} ptime {}", iss.str(), t_format, bt::to_iso_string(t_datetime) );
  } else {
    LOG_TRACE("datestr {} format {} ptime {}", iss.str(), t_format, bt::to_iso_string(t_datetime) );
    // t_datetime = t_datetime;
  }
  return ret;
}
*/
