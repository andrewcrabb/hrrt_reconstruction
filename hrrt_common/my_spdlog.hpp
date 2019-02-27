// my_spdlog.hpp
// 
// Macros to print spdlog messages with filename, function, and line number.
// 
// Note that __FILENAME__ must be defined as the file name only in __FILE__. In CMake:
// # This is to get only the file name in __FILE__ for spdlog debug messages http://bit.ly/2PMCS61
// set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")


#pragma once

#include <memory>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <type_traits>    // Cast scoped enums to underlying type (int) for LOG_foo macros.  http://bit.ly/2PSzLJY


// Needs to come before the macros.

namespace my_spdlog {
	extern std::shared_ptr<spdlog::logger> g_logger;
	void init_logging(std::string = "HRRT");
}

// Cast scoped enums to underlying type (int) for LOG_foo macros.  http://bit.ly/2PSzLJY
// NOTE this is a c++14 construct.
template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
// using ::to_underlying;

// https://stackoverflow.com/questions/8487986/file-macro-shows-full-path
#include <libgen.h>
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

using my_spdlog::g_logger;

// This needs to be a macro (not library function), so that the calling line number is preserved.
#ifndef LOG_INFO_DEFINED
#define LOG_INFO(...)  if (g_logger->should_log(spdlog::level::info )){g_logger->info("{}::{}:{}: {} " , __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_INFO_DEFINED 1
#endif

#ifndef LOG_ERROR_DEFINED
#define LOG_ERROR(...) if (g_logger->should_log(spdlog::level::err)){  g_logger->error("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_ERROR_DEFINED 1
#endif

#ifndef LOG_TRACE_DEFINED
#define LOG_TRACE(...) if (g_logger->should_log(spdlog::level::trace)){g_logger->trace("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_TRACE_DEFINED 1
#endif

#ifndef LOG_EXIT_DEFINED
#define LOG_EXIT(...) if (g_logger->should_log(spdlog::level::err)){  g_logger->error("{}::{}:{}: {} " , __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));	exit(1); }
#define LOG_EXIT_DEFINED 1
#endif

#ifndef LOG_DEBUG_DEFINED
#define LOG_DEBUG(...) if (g_logger->should_log(spdlog::level::debug)){g_logger->debug("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_DEBUG_DEFINED 1
#endif



