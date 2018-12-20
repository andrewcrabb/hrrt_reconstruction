#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "CHeader.hpp"
#include "Errors.hpp"
// #include <typeinfo>
#include <iostream>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "my_spdlog.hpp"

namespace bf = boost::filesystem;

std::string g_logfile;
std::shared_ptr<spdlog::logger> g_logger;
bf::path datafile("/home/ahc/DEV/hrrt_open_2011/data/test_EM.l64.hdr");

const std::string time_string(void) {
  time_t rawtime;
  struct tm * timeinfo;
  char buffer [80];

  time (&rawtime);
  timeinfo = localtime (&rawtime);
  strftime (buffer, 80, "%y%m%d_%H%M%S", timeinfo);
  std::string s(buffer);
  return s;
}

void init_logging(void) {
  spdlog::set_level(spdlog::level::info); // Set global log level to info
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("catch_{}.log", time_string());
  }

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern("[CHeader] [%^%l%$] %v");
  sinks.push_back(console_sink);

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(g_logfile, true);
  file_sink->set_level(spdlog::level::info);
  sinks.push_back(file_sink);

  auto multi_logger = std::make_shared<spdlog::logger>("CHeader", begin(sinks), end(sinks));
  if (std::getenv("LOG_DEBUG")) {
    multi_logger->set_level(spdlog::level::debug);
    file_sink->set_level(spdlog::level::debug);
    console_sink->set_level(spdlog::level::debug);
  }
  if (std::getenv("LOG_TRACE")) {
    multi_logger->set_level(spdlog::level::trace);
    file_sink->set_level(spdlog::level::trace);
    console_sink->set_level(spdlog::level::trace);
  }
  spdlog::register_logger(multi_logger);
}

TEST_CASE("Initialization", "[classic]") {
  auto logger = spdlog::get("CHeader");
  if (!logger)
    init_logging();
  logger = spdlog::get("CHeader");

  SECTION("Create a CHeader") {
    std::string str;
    // auto logger = spdlog::get("CHeader");
    LOG_ERROR(logger, "Test of error: {} ", 1);
    LOG_INFO(logger, "Test of info");
    LOG_DEBUG(logger, "Test of debug");

    LOG_INFO(logger, "datafile: {}", datafile.string());

    // Make a new CHeader and test that it is empty
    CHeader *chdr = new CHeader;
    REQUIRE(chdr != nullptr);
    LOG_TRACE(logger, "Test: Have not opened file yet")
    REQUIRE(chdr->IsFileOpen() == false);
    LOG_TRACE(logger, "Test: Should be no tags")
    REQUIRE(chdr->NumTags() == 0);
    LOG_TRACE(logger, "Test: Should not find a good tag")
    REQUIRE(chdr->Readchar(HDR_ORIGINATING_SYSTEM, str) != E_OK);
    LOG_TRACE(logger, "Test: Should not write an empty file")
    REQUIRE(chdr->WriteFile("/tmp/test_catch2.out") != E_OK);
    chdr->GetFileName(str);
    REQUIRE(str.length() == 0);

    // Read in a header file
    chdr->OpenFile(datafile);
    REQUIRE(chdr->IsFileOpen() == true);
    // Number of lines in the file
    std::ifstream infile(datafile.c_str());
    int numlines = std::count(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), '\n');
    REQUIRE(chdr->NumTags() == (numlines - 1));
    chdr->GetFileName(str);
    REQUIRE(str.compare(datafile.string()) == 0);
  }

  SECTION("Test ReadX functions") {
    CHeader *chdr = new CHeader;

    chdr->OpenFile(datafile);
    REQUIRE(chdr->IsFileOpen() == true);
    // ReadChar
    std::string s;
    REQUIRE(chdr->Readchar(HDR_ORIGINATING_SYSTEM, s) == E_OK);
    REQUIRE(s.compare("HRRT") == 0);
    std::string foo("foo");
    REQUIRE(chdr->Readchar("foo", s) == E_TAG_NOT_FOUND);
  }

}