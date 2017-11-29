# Install script for directory: /home/ahc/DEV/hrrt_open_2011/e7_tools_HRRT

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
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libatten/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libcommon/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libconvert/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libecat7/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libexception/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libfft/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libfilter/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libimage/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libinterfile/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libipc/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libparser/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/libsinogram/cmake_install.cmake")
  include("/home/ahc/DEV/hrrt_open_2011/build_ninja/e7_tools_HRRT/e7_atten_prj/cmake_install.cmake")

endif()

