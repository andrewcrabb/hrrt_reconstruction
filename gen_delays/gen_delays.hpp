// gen_delays.hpp
// Includes globals set by the command line program gen_delays and used in gen_delays_lib

#pragma once

#include <memory>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h" //support for stdout logging
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

std::shared_ptr<spdlog::logger> g_logger;

boost::filesystem::path g_logfile;
boost::filesystem::path g_coincidence_histogram_file;
boost::filesystem::path g_crystal_singles_file;
boost::filesystem::path g_delayed_coincidence_file;
boost::filesystem::path g_output_csings_file;

// boost::filesystem::path g_rebinner_lut_file;

