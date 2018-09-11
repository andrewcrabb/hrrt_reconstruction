cmake_minimum_required (VERSION 3.7)
project (hrrt_recon)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if (POLICY CMP0025)
#   cmake_policy(SET CMP0025 NEW)
# endif ()


# The version number.
set (hrrt_recon_VERSION_MAJOR 1)
set (hrrt_recon_VERSION_MINOR 0)
 
include(GNUInstallDirs)
set (CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

# configure a header file to pass some of the CMake settings to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/hrrt_recon_config.h.in"
  "${PROJECT_BINARY_DIR}/hrrt_recon_config.h"
  )
 
# add the binary tree to the search path for include files
# so that we will find hrrt_recon_config.h
# include_directories(/usr/include ${PROJECT_BINARY_DIR})
# Use of include_directories is discouraged http://bit.ly/2N6BrmA
# include_directories(${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/spdlog )

# My compiler switches (from setup.sh)
# Might want to use GLIBCXX_USE_CX11 more sparingly as it is disabling some posix features like getopt and swab
# add_definitions(-D_LINUX -D__LINUX__ -D_GLIBCXX_USE_CXX11_ABI=0)
# set (GCC_COMPILE_FLAGS "-std=c99")

 # ahc https://goo.gl/hHNWmc for strdup (else it's undefined and assume int so complains about type)
# ahc 9/4/18 took this out trying to fix problems with fmt::format linking
# add_definitions(-D_XOPEN_SOURCE=600 -D _GLIBCXX_USE_CXX11_ABI=0 -D_FILE_OFFSET_BITS=64)
# add_definitions(-D_XOPEN_SOURCE=600 -D _GLIBCXX_USE_CXX11_ABI=0 -D_FILE_OFFSET_BITS=64)

# https://gitlab.kitware.com/cmake/cmake/issues/15943
cmake_policy(SET CMP0025 NEW)


# add_definitions(-D_LINUX -D__LINUX__ ${GCC_COMPILE_FLAGS})
# Removed definition: Test for Linux defined at https://sourceforge.net/p/predef/wiki/OperatingSystems/: __linux__
# add_definitions(-D_LINUX -D__LINUX__)
# add_compile_options(-I${PROJECT_SOURCE_DIR}/AIR5.3.0/src -L${PROJECT_SOURCE_DIR}/AIR5.3.0/src)

if (MSVC)
	message("Hello Andrew this is MSVC")
	set(BOOST_ROOT "C:/BIN/boost_1_68_0")
	find_package(Boost)
	message(include_directories(${Boost_INCLUDE_DIRS}))

	# add_compile_options(--stdlib=libc++ -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/sys)
endif()


if (APPLE)
	message("Hello Andrew this is a Mac")
	add_compile_options(--stdlib=libc++ -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/sys)
endif()

# From original Makefile:
# SUBDIRS = gen_delays_lib interfile_lib ecatx ecatx/utils \
# 	hrrt_osem3d je_hrrt_osem3d \
# 	norm_process air_ecat motion_distance gsmooth e7_tools_HRRT compute_norm \
# 	calcRingRoiRatio if2e7 gen_delays_main motion_correction gsmooth lmhistogram_mp \
# 	TX_TV3DReg \
# 	common

# add_subdirectory(libscatter)
add_subdirectory(spdlog)
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

# add the executable
# add_executable(hrrt_recon hrrt_recon.cpp)
# target_link_libraries (hrrt_recon  ${EXTRA_LIBS})

# should we use our own math functions?
option (USE_MYMATH "Use hrrt_recon provided math implementation" ON) 

# add the install targets
# install (TARGETS hrrt_recon DESTINATION bin)
# install (FILES "${PROJECT_BINARY_DIR}/hrrt_recon_config.h"        
#          DESTINATION include)
