cmake_minimum_required (VERSION 3.7)
project (hrrt_recon)
set (CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set (hrrt_recon_VERSION_MAJOR 1)
set (hrrt_recon_VERSION_MINOR 0)
 
include(GNUInstallDirs)
set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# configure a header file to pass some of the CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/hrrt_recon_config.h.in"
  "${PROJECT_BINARY_DIR}/hrrt_recon_config.h"
  )

# Use of target_include_directories is preferred over include_directories  http://bit.ly/2N6BrmA
include_directories (${CMAKE_SOURCE_DIR}/spdlog/include
					 ${CMAKE_SOURCE_DIR}/hrrt_common
					 ${CMAKE_SOURCE_DIR}/cheader_lib
	)

add_compile_options("-Wall")

# https://gitlab.kitware.com/cmake/cmake/issues/15943
cmake_policy(SET CMP0025 NEW)

# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/catch2)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})
# message("CATCH_INCLUDE_DIR is ${CATCH_INCLUDE_DIR}")

if (MSVC)
	message("This is MSVC")
	set(BOOST_ROOT "C:/BIN/boost_1_68_0")
	find_package(Boost)
	message(include_directories(${Boost_INCLUDE_DIRS}))
endif()

if (APPLE)
	message("This is a Mac")
	add_compile_options(--stdlib=libc++ -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/sys)
endif()

add_subdirectory(fmt-5.1.0)
add_subdirectory(hrrt_common)
add_subdirectory(gen_delays_lib)
add_subdirectory(cheader_lib)
# add_subdirectory(gen_delays_main)  # Calls gen_delays with 7 params not 10 as required
add_subdirectory(interfile_lib)
add_subdirectory(lmhistogram_mp)
add_subdirectory(ecatx)
add_subdirectory(ecatx/utils)
add_subdirectory(hrrt_osem3d)
add_subdirectory(je_hrrt_osem3d)
add_subdirectory(norm_process)
add_subdirectory(air_ecat)
add_subdirectory(motion_distance)
add_subdirectory(e7_tools_HRRT)
add_subdirectory(gsmooth)
add_subdirectory(compute_norm)
add_subdirectory(if2e7)

# Leaving this out for now - requires AIR
# add_subdirectory(motion_correction)
# add_subdirectory(TX_TV3DReg)  # Uses X11 and I can't be fagged installing it
# add_subdirectory(ecat2DICOM)  # Has errors/omits

# # add the MathFunctions library?
# if (USE_MYMATH)
#   include_directories ("${PROJECT_SOURCE_DIR}/MathFunctions")
#   add_subdirectory (MathFunctions)
#   set (EXTRA_LIBS ${EXTRA_LIBS} MathFunctions)
# endif (USE_MYMATH)

# should we use our own math functions?
option (USE_MYMATH "Use hrrt_recon provided math implementation" ON) 
