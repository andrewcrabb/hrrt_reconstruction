#define CATCH_CONFIG_MAIN

#include <iostream>
#include <cstdlib>

#include "my_spdlog.hpp"
#include "hrrt_util.hpp"
#include "date_time.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "catch.hpp"

namespace bg = boost::gregorian;
namespace bt = boost::posix_time;

using std::string;

int TestDateTime(DateTime::TestData const &t_test_data) {
  std::unique_ptr<DateTime> date_time = DateTime::Create(t_test_data.date_time_str, t_test_data.format);
  if (!date_time)
    return 1;
  bool is_valid = date_time->IsValid();
  bt::ptime test_posix_time = date_time->get_date_time();
  bg::date test_date = test_posix_time.date();
  is_valid &= (test_date.year()  == t_test_data.year );
  is_valid &= (test_date.month() == t_test_data.month);
  is_valid &= (test_date.day()   == t_test_data.day  );
  is_valid &= (test_posix_time.time_of_day().hours()   == t_test_data.hour  );
  is_valid &= (test_posix_time.time_of_day().minutes() == t_test_data.minute);
  is_valid &= (test_posix_time.time_of_day().seconds() == t_test_data.second);

  return is_valid ? 0 : 1;
}

TEST_CASE("Initialization", "[classic]") {
  my_spdlog::init_logging("hrrt_common_tests");

  SECTION("Test logging") {
    std::string str;

    LOG_ERROR("Test of error: {} ", 1);
    LOG_INFO("Test of info");
    LOG_DEBUG("Test of debug");
  }

  SECTION("Dates and times") {
    for (DateTime::TestData test_data : DateTime::test_data_) {
      int ret = TestDateTime(test_data);
      int expected = test_data.valid ? 0 : 1;
      LOG_TRACE("Testing {} with format {}, expected {}", test_data.date_time_str, DateTime::FormatString(test_data.format), expected);
      REQUIRE(ret == expected);
    }

  }
}
