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

typedef std::unique_ptr<DateTime> date_time_ptr;

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
}

TEST_CASE("DateTime", "[classic]") {
  my_spdlog::init_logging("hrrt_common_tests");

  SECTION("Dates and times") {
    for (DateTime::TestData test_data : DateTime::test_data_) {
      int ret = TestDateTime(test_data);
      int expected = test_data.valid ? 0 : 1;
      LOG_TRACE("Testing {} with format {}, expected {}", test_data.date_time_str, DateTime::FormatString(test_data.format), expected);
      REQUIRE(ret == expected);
    }
  }

  SECTION("Operators") {
    date_time_ptr t0 = DateTime::Create("20190313_153000"    , DateTime::Format::compact_datetime);
    date_time_ptr t1 = DateTime::Create("13:03:2019 15:30:00", DateTime::Format::ecat_datetime);
    date_time_ptr t2 = DateTime::Create("13:03:2019 15:35:00", DateTime::Format::ecat_datetime);

    REQUIRE(t0);
    REQUIRE(t1);
    REQUIRE(t2);
    LOG_TRACE("Equality");
    REQUIRE(*t0 == *t1);
    REQUIRE_FALSE(*t0 == *t2);
    LOG_TRACE("Difference");
    REQUIRE((*t1 - *t0) == 0);
    REQUIRE((*t2 - *t0) == 300);
  }
}

TEST_CASE("Extrema", "[classic]") {
  my_spdlog::init_logging("hrrt_common_tests");

  SECTION("find_extrema") {
    short int dint_arr[]  {3, 1, 4, 1, 5, 9};
    // int       int_arr[]   {3, 1, 4, 1, 5, 9};
    // float     float_arr[] {3.0, 1.0, 4.0, 1.0, 5.0, 9.0};
    // char      char_arr[]  {'t', 'h', 'r', 'e', 'e'};

    hrrt_util::ExtremaTestData<short int> short_data{dint_arr, 6, 1, 9};
    // hrrt_util::Extrema<short int> short_extrema = hrrt_util::find_extrema<short int>(short_data.data, short_data.length);
    // REQUIRE(short_extrema.min == short_data.min);
    // REQUIRE(short_extrema.max == short_data.max);
    REQUIRE(hrrt_util::test_find_extrema(short_data));
  }
}