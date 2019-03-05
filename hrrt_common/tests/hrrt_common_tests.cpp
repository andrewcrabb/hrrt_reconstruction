#define CATCH_CONFIG_MAIN

#include <iostream>
#include <cstdlib>

#include "my_spdlog.hpp"
#include "hrrt_util.hpp"
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "catch.hpp"

namespace bg = boost::gregorian;
namespace bt = boost::posix_time;

using std::string;

int test_date(const string &str) {
  bt::ptime t0;
  bool ret = hrrt_util::parse_interfile_date(str, t0);
  // cout << "parse_interfile_date(" << str << ") returned " << ret << ": " << t0 << endl;
  LOG_TRACE("parse_interfile_date({}) returned {}: {}", str, ret, t0);
  return 0;
}

int test_time (const string &str) {
  bt::ptime pt;
  boost::io::ios_all_saver ias( std::cout );
  bt::time_facet* facet(new bt::time_facet("%H%M%S"));
  std::cout.imbue(std::locale(std::cout.getloc(), facet));
  hrrt_util::parse_interfile_time(str, pt);
  // cout << "test_time(" << str << "): " << pt << endl;
  LOG_TRACE("test_time({}): {}", str, pt);
  return 0;
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
    std::vector<string> dates = {"18:09:2017"};
    for (auto &date : dates) {
      int ret = test_date(date);
      REQUIRE(ret == 0);
    }
    std::vector<string> times = {"10:30:00", "10:35:00"};
    for (auto &time : times) {
      test_time(time);
    }

    bt::ptime t0, t1;
    hrrt_util::parse_interfile_time("10:30:00", t0);
    hrrt_util::parse_interfile_time("10:35:00", t1);
    bt::time_duration diff = t1 - t0;
    std::cout << "diff: " << diff.total_seconds() << std::endl;

  }
}
