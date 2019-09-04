#include <iostream>
#include <fmt/format.h>

#include "my_spdlog.hpp"
#include "hrrt_util.hpp"
#include <boost/filesystem.hpp>

namespace my_spdlog {

std::string g_logfile;
std::shared_ptr<spdlog::logger> g_logger;

void init_logging(std::string progname ) {
  auto logger = spdlog::get(progname);
  if (logger)
    return;

  boost::filesystem::path inpath = progname;
  std::string basename = inpath.filename().string();

  spdlog::set_level(spdlog::level::info); // Set global log level to info
  if (g_logfile.length() == 0) {
    g_logfile = fmt::format("catch_{}.log", hrrt_util::time_string());
  }

  std::vector<spdlog::sink_ptr> sinks;
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  std::string log_pattern = fmt::format("[{} %^%l%$] %v", basename);
  // console_sink->set_pattern("[%n %^%l%$] %v");
  console_sink->set_pattern(log_pattern);
  sinks.push_back(console_sink);

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(g_logfile, true);
  file_sink->set_level(spdlog::level::info);
  sinks.push_back(file_sink);

  auto multi_logger = std::make_shared<spdlog::logger>(progname, begin(sinks), end(sinks));
  // g_logger = std::make_shared<spdlog::logger>(progname, begin(sinks), end(sinks));
  g_logger = multi_logger;
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

  void set_level_debug(void) {
    g_logger->set_lvel(spdlog::level::debug);
  }

  void set_level_trace(void) {
    g_logger->set_lvel(spdlog::level::trace);
  }

}