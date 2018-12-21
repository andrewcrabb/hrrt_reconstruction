#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "CHeader.hpp"
// #include <typeinfo>
#include <iostream>
#include <cstdlib>
#define BOOST_NO_CXX11_SCOPED_ENUMS
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#undef BOOST_NO_CXX11_SCOPED_ENUMS
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "my_spdlog.hpp"

namespace bf = boost::filesystem;

std::string g_logfile;
std::shared_ptr<spdlog::logger> g_logger;
bf::path datafile("/home/ahc/DEV/hrrt_open_2011/data/test_EM.l64.hdr");
bf::path temp_file;
string const VALID_CHAR_TAG   = HDR_ORIGINATING_SYSTEM;
string const VALID_CHAR_VAL   = "HRRT";
string const VALID_FLOAT_TAG  = HDR_ISOTOPE_HALFLIFE;
float const VALID_FLOAT_VAL   = 100.00;
string const VALID_DOUBLE_TAG = HDR_ISOTOPE_HALFLIFE;
double const VALID_DOUBLE_VAL = 100.00;
string const VALID_INT_TAG    = HDR_IMAGE_DURATION;
int const VALID_INT_VAL   = 5400;

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
  auto logger = spdlog::get("CHeader");
  if (logger)
    return;

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

  // Initialize valid temp file.
  temp_file = boost::filesystem::temp_directory_path();
  bf::path tpath = boost::filesystem::unique_path("cheader_test-%%%%");
  temp_file /= tpath;
}

// Was going to do this with vector<int>

bool not_ok_or_notfound(CHeaderError val) {
  bool ret = ((val != CHeaderError::E_TAG_NOT_FOUND) && (val != CHeaderError::E_OK));
  return ret;
}

TEST_CASE("Initialization", "[classic]") {
  init_logging();
  auto logger = spdlog::get("CHeader");

  SECTION("Empty CHeader") {
    std::string str;
    // auto logger = spdlog::get("CHeader");
    LOG_ERROR(logger, "Test of error: {} ", 1);
    LOG_INFO(logger, "Test of info");
    LOG_DEBUG(logger, "Test of debug");

    LOG_INFO(logger, "datafile: {}", datafile.string());
    LOG_INFO(logger, "temp_file: {}", temp_file.string());

    // Make a new CHeader and test that it is empty
    CHeader *chdr = new CHeader;
    REQUIRE(chdr != nullptr);
    LOG_TRACE(logger, "Test: Have not opened file yet");
    REQUIRE(chdr->IsFileOpen() == false);
    REQUIRE_FALSE(chdr->CloseFile() == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should be no tags");
    REQUIRE(chdr->NumTags() == 0);
    LOG_TRACE(logger, "Test: Should not find a bad char tag");
    REQUIRE_FALSE(chdr->Readchar("nosuchtag", str) == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should not find a good char tag");
    REQUIRE_FALSE(chdr->Readchar(VALID_CHAR_TAG, str) == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should not find a good int tag");
    REQUIRE_FALSE(chdr->Readchar(VALID_INT_TAG, str) == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should not find a good float tag");
    REQUIRE_FALSE(chdr->Readchar(VALID_FLOAT_TAG, str) == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should not find a good double tag");
    REQUIRE_FALSE(chdr->Readchar(VALID_DOUBLE_TAG, str) == CHeaderError::E_OK);
    LOG_TRACE(logger, "Test: Should not write to empty file {}", temp_file.string());
    REQUIRE_FALSE(chdr->WriteFile(temp_file.string()) == CHeaderError::E_OK);
    REQUIRE_FALSE(chdr->WriteFile(temp_file) == CHeaderError::E_OK);
    chdr->GetFileName(str);
    REQUIRE(str.length() == 0);
  }

  SECTION("Open CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");
    CHeader *chdr = new CHeader;
    std::string str;

    LOG_TRACE(logger, "Open valid file {}", datafile.string());
    chdr->OpenFile(datafile);
    REQUIRE(chdr->IsFileOpen() == true);
    LOG_TRACE(logger, "Close file");
    chdr->CloseFile();
    REQUIRE(chdr->IsFileOpen() == false);
    delete chdr;

    chdr = new CHeader;
    chdr->OpenFile(datafile);
    // Number of lines in the file
    std::ifstream infile(datafile.c_str());
    int numlines = std::count(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), '\n');
    LOG_TRACE(logger, "Should have {} tags", numlines - 1);
    REQUIRE(chdr->NumTags() == (numlines - 1));
    LOG_TRACE(logger, "Should have name {}", datafile.string());
    chdr->GetFileName(str);
    REQUIRE(str.compare(datafile.string()) == 0);

    std::string s;
    int i;
    float f;
    double d;
    LOG_TRACE(logger, "Should find char tag {} = {}", VALID_CHAR_TAG, VALID_CHAR_VAL);
    REQUIRE(chdr->Readchar(VALID_CHAR_TAG, s) == CHeaderError::E_OK);
    REQUIRE(s.compare(VALID_CHAR_VAL) == 0);
    LOG_TRACE(logger, "Should find int tag {} = {}", VALID_INT_TAG, VALID_INT_VAL);
    REQUIRE(chdr->Readint(VALID_INT_TAG, i) == CHeaderError::E_OK);
    REQUIRE(i == VALID_INT_VAL);
    LOG_TRACE(logger, "Should find float tag {} = {}", VALID_FLOAT_TAG, VALID_FLOAT_VAL);
    REQUIRE(chdr->Readfloat(VALID_FLOAT_TAG, f) == CHeaderError::E_OK);
    REQUIRE(f == Approx(VALID_FLOAT_VAL));
    LOG_TRACE(logger, "Should find double tag {} = {}", VALID_DOUBLE_TAG, VALID_DOUBLE_VAL);
    REQUIRE(chdr->Readdouble(VALID_DOUBLE_TAG, d) == CHeaderError::E_OK);
    REQUIRE(d == Approx(VALID_DOUBLE_VAL));
  }

  SECTION("Write CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");

    CHeader *chdr = new CHeader;
    chdr->OpenFile(datafile);
    std::string str;
    LOG_TRACE(logger, "Write to and validate temp_file {}", temp_file.string());
    CHeaderError ret = chdr->WriteFile(temp_file);
    REQUIRE(ret == CHeaderError::E_OK);

    CHeader *temphdr = new CHeader;
    temphdr->OpenFile(temp_file);
    REQUIRE(chdr->NumTags() == temphdr->NumTags());
  }

  SECTION("Create CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");

    CHeader *chdr = new CHeader;    
  }


}