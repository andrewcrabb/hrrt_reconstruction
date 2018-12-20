// my_spdlog.hpp
// 
// Macros to print spdlog messages with filename, function, and line number.
// 
// Note that __FILENAME__ must be defined as the file name only in __FILE__. In CMake:
// # This is to get only the file name in __FILE__ for spdlog debug messages http://bit.ly/2PMCS61
// set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__FILENAME__='\"$(subst ${CMAKE_SOURCE_DIR}/,,$(abspath $<))\"'")

#pragma once

// This needs to be a macro so that the calling line number is preserved.
#ifndef LOG_INFO_DEFINED
#define LOG_INFO(logger, ...) if (logger->should_log(spdlog::level::info)){logger->info("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_INFO_DEFINED 1
#endif

#ifndef LOG_ERROR_DEFINED
#define LOG_ERROR(logger, ...) if (logger->should_log(spdlog::level::debug)){logger->error("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_ERROR_DEFINED 1
#endif

#ifndef LOG_DEBUG_DEFINED
#define LOG_DEBUG(logger, ...) if (logger->should_log(spdlog::level::debug)){logger->debug("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_DEBUG_DEFINED 1
#endif

#ifndef LOG_TRACE_DEFINED
#define LOG_TRACE(logger, ...) if (logger->should_log(spdlog::level::trace)){logger->trace("{}::{}:{}: {} ", __FILENAME__ , __func__, __LINE__, fmt::format(__VA_ARGS__));}
#define LOG_TRACE_DEFINED 1
#endif
