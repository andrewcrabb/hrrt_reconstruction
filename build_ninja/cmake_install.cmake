# Install script for directory: /home/ahc/DEV/hrrt_open_2011

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/ahc/DEV/hrrt_open_2011")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/gen_delays_lib/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/interfile_lib/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/ecatx/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/ecatx/utils/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/hrrt_osem3d/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/je_hrrt_osem3d/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/norm_process/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/air_ecat/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/motion_distance/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/ahc/DEV/hrrt_open_2011/build_ninja/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
