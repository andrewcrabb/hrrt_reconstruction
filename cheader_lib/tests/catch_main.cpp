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
#include "hrrt_util.hpp"

namespace bf = boost::filesystem;

std::string g_logfile;
// std::shared_ptr<spdlog::logger> g_logger;
bf::path datafile("/home/ahc/DEV/hrrt_open_2011/data/test_EM.l64.hdr");
bf::path temp_file;
// string const VALID_CHAR_TAG   = CHeader::ORIGINATING_SYSTEM;
// string const VALID_CHAR_VAL   = "HRRT";
// string const VALID_FLOAT_TAG  = CHeader::ISOTOPE_HALFLIFE;
// float const VALID_FLOAT_VAL   = 100.00;
// string const VALID_DOUBLE_TAG = CHeader::ISOTOPE_HALFLIFE;
// double const VALID_DOUBLE_VAL = 100.00;
// string const VALID_INT_TAG    = CHeader::IMAGE_DURATION;
// int const VALID_INT_VAL       = 5400;

void init_logging(void) {
  auto logger = spdlog::get("CHeader");
  if (logger)
    return;

  spdlog::set_level(spdlog::level::info); // Set global log level to info
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("catch_{}.log", hrrt_util::time_string());
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

int test_read_tags(CHeader *chdr) {
    auto logger = spdlog::get("CHeader");
    std::string infile;

    chdr->GetFileName(infile);

    std::string s;
    LOG_TRACE("Should find char tag {}", CHeader::VALID_CHAR.sayit());
    REQUIRE(chdr->ReadChar(CHeader::VALID_CHAR.key, s) == CHeaderError::OK);
    REQUIRE(s.compare(CHeader::VALID_CHAR.value) == 0);

    int i;
    LOG_TRACE("Should find int tag {}", CHeader::VALID_INT.sayit());
    REQUIRE(chdr->ReadInt(CHeader::VALID_INT.key, i) == CHeaderError::OK);
    REQUIRE(i == std::stoi(CHeader::VALID_INT.value));

    float f;
    LOG_TRACE("Should find float tag {}", CHeader::VALID_FLOAT.sayit());
    REQUIRE(chdr->ReadFloat(CHeader::VALID_FLOAT.key, f) == CHeaderError::OK);
    REQUIRE(f == Approx(std::stof(CHeader::VALID_FLOAT.value)));

    double d;
    LOG_TRACE("Should find double tag {}", CHeader::VALID_DOUBLE.sayit());
    REQUIRE(chdr->ReadDouble(CHeader::VALID_DOUBLE.key, d) == CHeaderError::OK);
    REQUIRE(d == Approx(std::stod(CHeader::VALID_DOUBLE.value)));

    boost::posix_time::ptime the_time;
    boost::posix_time::ptime valid_time;
    LOG_TRACE("Should find time tag {} in {}", CHeader::VALID_TIME.sayit(), infile);
    REQUIRE(chdr->ReadTime(CHeader::VALID_TIME.key, the_time) == CHeaderError::OK);
    REQUIRE_FALSE(hrrt_util::ParseInterfileTime(CHeader::VALID_TIME.value, valid_time));
    REQUIRE(the_time == valid_time);

    boost::posix_time::ptime the_date;
    boost::posix_time::ptime valid_date;
    LOG_TRACE("Should find date tag {}", CHeader::VALID_DATE.sayit());
    REQUIRE(chdr->ReadDate(CHeader::VALID_DATE.key, the_date) == CHeaderError::OK);
    REQUIRE_FALSE(hrrt_util::ParseInterfileDate(CHeader::VALID_DATE.value, valid_date));
    REQUIRE(the_date == valid_date);

    return 0;
}

// Was going to do this with vector<int>

bool not_ok_or_notfound(CHeaderError val) {
  bool ret = ((val != CHeaderError::TAG_NOT_FOUND) && (val != CHeaderError::OK));
  return ret;
}

TEST_CASE("Initialization", "[classic]") {
  init_logging();
  auto logger = spdlog::get("CHeader");

  SECTION("Empty CHeader") {
    std::string str;
    // auto logger = spdlog::get("CHeader");
    LOG_ERROR("Test of error: {} ", 1);
    LOG_INFO("Test of info");
    LOG_DEBUG("Test of debug");

    LOG_INFO("datafile: {}", datafile.string());
    LOG_INFO("temp_file: {}", temp_file.string());

    // Make a new CHeader and test that it is empty
    CHeader *chdr = new CHeader;
    REQUIRE(chdr != nullptr);
    LOG_TRACE("Test: Have not opened file yet");
    REQUIRE(chdr->IsFileOpen() == false);
    REQUIRE_FALSE(chdr->CloseFile() == CHeaderError::OK);
    LOG_TRACE("Test: Should be no tags");
    REQUIRE(chdr->NumTags() == 0);
    LOG_TRACE("Test: Should not find a bad char tag");
    REQUIRE(chdr->ReadChar("nosuchtag", str) == CHeaderError::TAG_NOT_FOUND);
    LOG_TRACE("Test: Should not find a good char tag");
    REQUIRE_FALSE(chdr->ReadChar(CHeader::VALID_CHAR.key, str) == CHeaderError::OK);
    LOG_TRACE("Test: Should not find a good int tag");
    REQUIRE_FALSE(chdr->ReadChar(CHeader::VALID_INT.key, str) == CHeaderError::OK);
    LOG_TRACE("Test: Should not find a good float tag");
    REQUIRE_FALSE(chdr->ReadChar(CHeader::VALID_FLOAT.key, str) == CHeaderError::OK);
    LOG_TRACE("Test: Should not find a good double tag");
    REQUIRE_FALSE(chdr->ReadChar(CHeader::VALID_DOUBLE.key, str) == CHeaderError::OK);
    LOG_TRACE("Test: Should not write to empty file {}", temp_file.string());
    REQUIRE_FALSE(chdr->WriteFile(temp_file.string()) == CHeaderError::OK);
    REQUIRE_FALSE(chdr->WriteFile(temp_file) == CHeaderError::OK);
    chdr->GetFileName(str);
    REQUIRE(str.length() == 0);
    delete(chdr);
  }

  SECTION("Open CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");
    CHeader *chdr = new CHeader;
    CHeaderError ret;
    std::string str;
    bf::path bad_path("/no/such/file");

    LOG_TRACE("Open invalid file {}", bad_path.string());
    ret = chdr->OpenFile(bad_path);
    REQUIRE(ret == CHeaderError::COULD_NOT_OPEN_FILE);
    REQUIRE_FALSE(chdr->IsFileOpen() == true);

    LOG_TRACE("Open valid file {}", datafile.string());
    chdr->OpenFile(datafile);
    REQUIRE(chdr->IsFileOpen() == true);
    LOG_TRACE("Close file");
    chdr->CloseFile();
    REQUIRE(chdr->IsFileOpen() == false);
    delete chdr;

    chdr = new CHeader;
    chdr->OpenFile(datafile);
    // Number of lines in the file
    std::ifstream infile(datafile.c_str());
    int numlines = std::count(std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>(), '\n');
    LOG_TRACE("Should have {} tags", numlines - 1);
    REQUIRE(chdr->NumTags() == (numlines - 1));
    LOG_TRACE("Should have name {}", datafile.string());
    chdr->GetFileName(str);
    REQUIRE(str.compare(datafile.string()) == 0);
    LOG_DEBUG("Calling test_read_tags for {}", datafile.string());
    test_read_tags(chdr);
    delete(chdr);
  }

  SECTION("Invalid tags") {
    init_logging();
    auto logger = spdlog::get("CHeader");
    CHeader *chdr = new CHeader;
    std::string s;
    int i;
    float f;
    double d;
    long l;
    boost::posix_time::ptime t;

    chdr->OpenFile(datafile);
    LOG_TRACE("Test: Should not find an int in a good char tag");
    REQUIRE(chdr->ReadInt(CHeader::VALID_CHAR.key, i)    == CHeaderError::NOT_AN_INT);
    LOG_TRACE("Test: Should not find a long in a good char tag");
    REQUIRE(chdr->ReadLong(CHeader::VALID_CHAR.key, l)   == CHeaderError::NOT_A_LONG);
    LOG_TRACE("Test: Should not find a float in a good char tag");
    REQUIRE(chdr->ReadFloat(CHeader::VALID_CHAR.key, f)  == CHeaderError::NOT_A_FLOAT);
    LOG_TRACE("Test: Should not find a double in a good char tag");
    REQUIRE(chdr->ReadDouble(CHeader::VALID_CHAR.key, d) == CHeaderError::NOT_A_DOUBLE);
    LOG_TRACE("Test: Should not find a date in a good char tag");
    REQUIRE(chdr->ReadDate(CHeader::VALID_CHAR.key, t)   == CHeaderError::NOT_A_DATE);
    LOG_TRACE("Test: Should not find a time in a good char tag");
    REQUIRE(chdr->ReadTime(CHeader::VALID_CHAR.key, t)   == CHeaderError::NOT_A_TIME);
  }


  SECTION("Write CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");
    bf::path bad_path("/no/such/file");

    CHeader *chdr = new CHeader;
    chdr->OpenFile(datafile);
    std::string str;
    LOG_TRACE("Write to and validate temp_file {}", temp_file.string());
    CHeaderError ret = chdr->WriteFile(temp_file);
    REQUIRE(ret == CHeaderError::OK);

    CHeader *temphdr = new CHeader;
    temphdr->OpenFile(temp_file);
    REQUIRE(chdr->NumTags() == temphdr->NumTags());

    LOG_TRACE("Write to invalid file {}", bad_path.string());
    REQUIRE(chdr->WriteFile(bad_path) == CHeaderError::COULD_NOT_OPEN_FILE);

    delete(chdr);
  }

  SECTION("Create CHeader") {
    init_logging();
    auto logger = spdlog::get("CHeader");

    CHeader *chdr = new CHeader;    
    LOG_TRACE("Writing char tag {}", CHeader::VALID_CHAR.sayit());
    REQUIRE(chdr->WriteChar(CHeader::VALID_CHAR.key, CHeader::VALID_CHAR.value)                  == CHeaderError::TAG_APPENDED);
    LOG_TRACE("Writing int tag {}", CHeader::VALID_INT.sayit());
    REQUIRE(chdr->WriteInt(CHeader::VALID_INT.key, std::stoi(CHeader::VALID_INT.value))          == CHeaderError::TAG_APPENDED);
    LOG_TRACE("Writing float tag {}", CHeader::VALID_FLOAT.sayit());
    REQUIRE(chdr->WriteFloat(CHeader::VALID_FLOAT.key, std::stof(CHeader::VALID_FLOAT.value))    == CHeaderError::TAG_APPENDED);
    LOG_TRACE("Writing double tag {}", CHeader::VALID_DOUBLE.sayit());
    REQUIRE(chdr->WriteDouble(CHeader::VALID_DOUBLE.key, std::stod(CHeader::VALID_DOUBLE.value)) == CHeaderError::TAG_APPENDED);

    // Boost posix time requires datetime-format input string for time.
    std::string datetime_string = fmt::format("{} {}", CHeader::VALID_DATE.value, CHeader::VALID_TIME.value);
    LOG_TRACE("Writing date tag {} from {}", CHeader::VALID_DATE.key, CHeader::VALID_DATE.value);
    REQUIRE(chdr->WriteDate(CHeader::VALID_DATE.key, CHeader::VALID_DATE.value)                  == CHeaderError::TAG_APPENDED);
    LOG_TRACE("Writing time tag {} from {}", CHeader::VALID_TIME.key, datetime_string);
    REQUIRE(chdr->WriteTime(CHeader::VALID_TIME.key, datetime_string)                  == CHeaderError::TAG_APPENDED);

    LOG_TRACE("Write to temp_file {}", temp_file.string());
    CHeaderError ret = chdr->WriteFile(temp_file);
    REQUIRE(ret == CHeaderError::OK);
    delete(chdr);

    chdr = new CHeader;
    LOG_TRACE("Read from temp_file {}", temp_file.string());
    chdr->OpenFile(temp_file);
    REQUIRE(chdr->NumTags() == 6);

    LOG_DEBUG("Calling test_read_tags for {}", temp_file.string());
    test_read_tags(chdr);
  }

}