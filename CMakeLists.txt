cmake_minimum_required (VERSION 3.7)
project (hrrt_recon)
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
include_directories("${PROJECT_BINARY_DIR}")

# My compiler switches (from setup.sh)
add_definitions(-D_LINUX -D__LINUX__ -D_GLIBCXX_USE_CXX11_ABI=0)
add_compile_options(-I${PROJECT_SOURCE_DIR}/AIR5.3.0/src -L${PROJECT_SOURCE_DIR}/AIR5.3.0/src)


if (APPLE)
	message("Hello Andrew this is a Mac")
	add_compile_options(-I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/sys)
endif()

add_subdirectory(gen_delays_lib)
add_subdirectory(interfile_lib)
add_subdirectory(ecatx)
add_subdirectory(ecatx/utils)

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
