#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "CHeader.hpp"
// #include <typeinfo>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace bf = boost::filesystem;

std::string g_logfile;
std::shared_ptr<spdlog::logger> g_logger;

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
  // spdlog::set_level(spdlog::level::info); // Set global log level to info
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("catch_{}.log", time_string());
  }

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern("[CHeader] [%^%l%$] %v");
  sinks.push_back(console_sink);

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(g_logfile, true);
  file_sink->set_level(spdlog::level::trace);
  sinks.push_back(file_sink);

  auto multi_logger = std::make_shared<spdlog::logger>("CHeader", begin(sinks), end(sinks));
  spdlog::register_logger(multi_logger);
}

TEST_CASE("Initialization", "[classic]") {
  bf::path datafile("/home/ahc/DEV/hrrt_open_2011/data/test_EM.l64.hdr");
  init_logging();

  SECTION("Create a CHeader") {
    auto logger = spdlog::get("CHeader");
    CHeader *chdr = new CHeader;

    logger->info("datafile: {}", datafile.string());

    // CHeader *hdr = nullptr;
    logger->info("chdr not null");
    REQUIRE(chdr != nullptr);
    logger->info("chdr file not open");
    REQUIRE(chdr->IsFileOpen() == false);
    logger->info("chdr open file {}", datafile.string());
    // chdr->OpenFile(datafile);
    // logger->info("chdr file open true");
    // REQUIRE(chdr->IsFileOpen() == true);

  }

}