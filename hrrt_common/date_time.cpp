// date_time.cpp
// 201903 ahc
// C++ patterns/practices I am implementing in this class:
//   Returning types rather than return codes
//   Factory constructor method

#include "date_time.hpp"

namespace bt = boost::posix_time;

// Factory method to create and initialize a DateTime
// Returns nullptr on failure.

static std::unique_ptr<DateTime> DateTime::Create(std::string const &t_datetime_str, DateTime::Format t_format) {
  std::unique_ptr<DateTime> date_time = std::make_unique<DateTime>();
  date_time->ParseDateTimeString(t_datetime_str, t_format);
  return date_time.IsValid() ? date_time : nullptr;
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
  std::string format_string = date_formats_[t_format];
  bt::time_input_facet *facet = new bt::time_input_facet(format_string);
  const std::locale loc(std::locale::classic(), facet);
  LOG_DEBUG("t_datetime_str {} format_string {}", t_datetime_str, format_string);
  std::istringstream iss(t_datetime_str);
  iss.imbue(loc);
  // bt::ptime posix_time;
  iss >> date_time_;
  if (date_time_.is_not_a_date_time())
    LOG_ERROR("Not a date_time: datestr {}, format {}", t_datetime_str, DateTime::formats_.at(t_format));
  // DateTime:: ret = t_pt.is_not_a_date_time();
  // std::string timestr("FILLMEIN");
  return;
}

// Return date/time to string in given format.
// TODO: This doesn't do any error checking

std::string DateTime::ToString(DateTime::Format t_format) const {
  bt::time_facet *time_facet = new bt::time_facet();
  time_facet->format(t_format.c_str());
  std::ostringstream oss;
  oss.imbue(std::locale(oss.getloc(), time_facet));
  oss << t_ptime;
  std::string datetime = oss.str();
  LOG_TRACE("ptime {} format {} returning {}", bt::to_iso_string(t_ptime), t_format, datetime);
  return datetime;
}

bool DateTime::IsValid(void) const {
  return !date_time_.is_not_a_date_time();
}

// Check that given date is between 1900 and 2100.

bool DateTime::ValidDateRange(void) const {
  bt::ptime low_time( boost::gregorian::date(1900,01,01), bt::time_duration( 0,0,0));
  bt::ptime high_time(boost::gregorian::date(2099,12,31), bt::time_duration(24,0,0));

  bool ret = ((t_datetime > low_time) && (t_datetime < high_time));
  LOG_DEBUG("datetime {} returning {}", ToString(), hrrt_util::BoolToString(ret));
  return ret;
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
